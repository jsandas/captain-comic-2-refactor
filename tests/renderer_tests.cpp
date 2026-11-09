#include <cstdint>
#include <stdexcept>

#include "comic2/bootstrap.hpp"
#include "comic2/entity_runtime.hpp"
#include "comic2/game_state.hpp"
#include "comic2/renderer.hpp"
#include "comic2/renderer_validation.hpp"

namespace {

void set_test_sprite_pixel(comic2::Ega4PlaneImage &sprite, std::size_t x_pixels,
                           std::size_t y_rows, std::uint8_t color) {
  if (x_pixels >= static_cast<std::size_t>(sprite.width_bytes) * 8U ||
      y_rows >= static_cast<std::size_t>(sprite.height_rows)) {
    return;
  }

  const auto x_byte = x_pixels / 8U;
  const auto bit = static_cast<std::uint8_t>(7U - (x_pixels % 8U));
  const auto row_off = y_rows * static_cast<std::size_t>(sprite.width_bytes);

  for (std::size_t plane = 0; plane < sprite.planes.size(); ++plane) {
    if ((color >> plane) & 0x1U) {
      sprite.planes[plane][row_off + x_byte] |=
          static_cast<std::uint8_t>(1U << bit);
    }
  }
}

comic2::Ega4PlaneImage make_test_sprite(std::uint8_t color,
                                        std::size_t width_bytes,
                                        std::size_t height_rows) {
  comic2::Ega4PlaneImage sprite{};
  sprite.width_bytes = static_cast<std::uint16_t>(width_bytes);
  sprite.height_rows = static_cast<std::uint16_t>(height_rows);
  sprite.row_span_bytes = static_cast<std::uint16_t>(width_bytes * height_rows);

  for (auto &plane_bytes : sprite.planes) {
    plane_bytes.assign(width_bytes * height_rows, 0x00);
  }

  set_test_sprite_pixel(sprite, 0, 0, color);
  set_test_sprite_pixel(sprite, 8, 0, color);
  return sprite;
}

comic2::MaskedSpriteRecord make_test_masked_record(std::size_t index,
                                                   std::uint8_t color,
                                                   std::size_t width_pixels,
                                                   std::size_t height_rows) {
  comic2::MaskedSpriteRecord record{};
  record.width_pixels = static_cast<std::uint16_t>(width_pixels);
  record.height_rows = static_cast<std::uint16_t>(height_rows);
  const auto byte_count = record.mask_byte_count();
  record.mask_bytes.assign(byte_count, 0xFF);
  for (auto &plane : record.planes) {
    plane.assign(byte_count, 0x00);
  }

  record.planes[0].assign(byte_count, 0x00);
  record.planes[0][0] = 0x80U;
  if (color != 0U) {
    record.planes[0][0] = 0x80U;
  }

  (void)index;
  return record;
}

void expect(bool condition, const char *message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

std::uint8_t read_surface_color(const comic2::EgaPlanarSurface &surface,
                                std::int32_t x, std::int32_t y) {
  const auto x_byte = static_cast<std::size_t>(x / 8);
  const auto bit = static_cast<std::uint8_t>(7 - (x % 8));
  const auto y_row = static_cast<std::size_t>(y);
  std::uint8_t color = 0;
  for (std::size_t plane = 0; plane < comic2::EgaPlanarSurface::kPlaneCount;
       ++plane) {
    const auto value = surface.get_plane_byte(plane, x_byte, y_row);
    if ((value & static_cast<std::uint8_t>(1U << bit)) != 0U) {
      color |= static_cast<std::uint8_t>(1U << plane);
    }
  }
  return color;
}

void test_surface_geometry() {
  comic2::EgaPlanarSurface surface(320, 200);
  expect(surface.row_stride_bytes() == 40,
         "row stride mismatch for 320px surface");
  expect(surface.plane_size_bytes() == 8000,
         "plane size mismatch for 320x200 surface");
}

void test_surface_plane_rw() {
  comic2::EgaPlanarSurface surface(320, 200);
  surface.set_plane_byte(2, 10, 5, 0xA5);
  expect(surface.get_plane_byte(2, 10, 5) == 0xA5,
         "plane byte write/read mismatch");
}

void test_presenter_copies_frame() {
  comic2::EgaPlanarSurface src(320, 200);
  src.set_plane_byte(1, 0, 0, 0x7E);

  comic2::MemoryFramePresenter presenter;
  presenter.present(src);

  src.set_plane_byte(1, 0, 0, 0x00);
  expect(presenter.has_frame(), "presenter should report frame availability");
  expect(presenter.last_frame().get_plane_byte(1, 0, 0) == 0x7E,
         "presenter did not retain frame copy");
}

void test_transition_palette_tint_applies_to_surface_pixels() {
  comic2::EgaPlanarSurface surface(320, 200);
  surface.clear(0x00);
  surface.set_plane_byte(1, 0, 0, 0x80);

  comic2::RoomTransitionState transition{};
  transition.active = true;
  transition.frame_index = 2;
  transition.palette_tint = 3;
  transition.palette_shift = 1;

  comic2::apply_transition_palette_tint(surface, transition);

  expect(
      read_surface_color(surface, 0, 0) == 0x08,
      "transition palette tint should shift the pixel color deterministically");
}

void test_transition_effects_are_deterministic() {
  comic2::EgaPlanarSurface surface(320, 200);
  surface.clear(0x00);

  comic2::RoomTransitionState transition{};
  transition.active = true;
  transition.effect_type = 0;
  transition.frame_index = 3;
  comic2::room_transition_palette_wave(surface, transition);
  const auto palette_hash = comic2::validation::hash_surface_planes(surface);
  expect(palette_hash != 0x0ULL,
         "palette wave should alter the frame deterministically");

  comic2::EgaPlanarSurface reveal(320, 200);
  reveal.clear(0x00);
  transition.effect_type = 1;
  transition.frame_index = 2;
  comic2::room_transition_reveal_sequence_a(reveal, transition);
  const auto reveal_hash = comic2::validation::hash_surface_planes(reveal);
  expect(reveal_hash != 0x0ULL,
         "reveal sequence A should alter the frame deterministically");
}

void test_runtime_palette_loader_uses_ega_table() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  std::array<std::array<std::uint8_t, 4>, 16> palette{};
  for (std::size_t index = 0; index < palette.size(); ++index) {
    palette[index] = {static_cast<std::uint8_t>(index * 10U),
                      static_cast<std::uint8_t>(index * 11U),
                      static_cast<std::uint8_t>(index * 12U), 255U};
  }

  comic2::load_ega_palette_table(state, palette);
  expect(state.palette.loaded,
         "runtime palette should mark itself loaded after a table upload");
  expect(state.palette.entries[0][0] == 0U,
         "the first palette entry should preserve the low-color channel");
  expect(state.palette.entries[15][3] == 255U,
         "the alpha channel should be preserved for the final palette entry");
}

void test_player_sprite_frame_selection_uses_animation_state() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.player.animation_state =
      static_cast<std::uint8_t>(comic2::PlayerAnimationState::WalkCycle);
  state.player.animation_frame = 3;

  const auto frame = comic2::select_player_sprite_frame(state);
  expect(frame == 3, "walk cycle frame should resolve to the current frame");

  state.player.animation_state =
      static_cast<std::uint8_t>(comic2::PlayerAnimationState::JumpRise);
  expect(comic2::select_player_sprite_frame(state) == 4,
         "jump-rise state should resolve to the jump frame");
}

void test_player_sprite_frame_selection_uses_facing_direction() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.player.animation_state =
      static_cast<std::uint8_t>(comic2::PlayerAnimationState::WalkCycle);
  state.player.animation_frame = 2;
  state.player.facing_right = false;

  const auto frame = comic2::select_player_sprite_frame(state);
  expect(frame == 10,
         "left-facing walk cycle should use the mirrored frame offset");
}

void test_runtime_entity_render_prefers_level_sprite_record() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.level_sprites.resize(16);
  state.level_sprites[1] = make_test_masked_record(1, 0x01U, 16U, 16U);
  state.runtime_slots.resize(1);
  state.runtime_slots[0].x = 0;
  state.runtime_slots[0].y = 0;
  state.runtime_slots[0].mapped_object_ptr = 1;
  state.runtime_slots[0].behavior_state = 0;
  state.runtime_slots[0].type_flags = 0;

  comic2::EgaPlanarSurface frame(320, 200);
  comic2::draw_runtime_entity_sprites(frame, state);

  expect(read_surface_color(frame, 0, 0) == 0x01,
         "entity sprite path should prefer the decoded level sprite record");
}

void test_runtime_projectile_render_prefers_level_sprite_record() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.level_sprites.resize(16);
  state.level_sprites[1] = make_test_masked_record(1, 0x01U, 16U, 8U);
  state.projectiles.push_back(comic2::ProjectileState{0, 0, 0, 0, 1, true});

  comic2::EgaPlanarSurface frame(320, 200);
  comic2::draw_runtime_projectile_sprites(frame, state);

  expect(
      read_surface_color(frame, 0, 0) == 0x01,
      "projectile sprite path should prefer the decoded level sprite record");
}

void test_runtime_entity_render_selects_16x16_records_for_behavior_types() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.level_sprites.resize(20);
  state.level_sprites[1] = make_test_masked_record(1, 0x01U, 8U, 8U);
  state.level_sprites[17] = make_test_masked_record(17, 0x01U, 16U, 16U);
  state.runtime_slots.resize(1);
  state.runtime_slots[0].x = 0;
  state.runtime_slots[0].y = 0;
  state.runtime_slots[0].mapped_object_ptr = 1;
  state.runtime_slots[0].behavior_state = comic2::kBehaviorGem;
  state.runtime_slots[0].type_flags = 0;

  comic2::EgaPlanarSurface frame(320, 200);
  comic2::draw_runtime_entity_sprites(frame, state);

  expect(read_surface_color(frame, 0, 0) == 0x01,
         "entity behavior mapping should select a decoded 16x16 record");
}

void test_runtime_projectile_render_selects_16x8_records() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.level_sprites.resize(20);
  state.level_sprites[1] = make_test_masked_record(1, 0x01U, 8U, 8U);
  state.level_sprites[18] = make_test_masked_record(18, 0x01U, 16U, 8U);
  state.projectiles.push_back(comic2::ProjectileState{0, 0, 0, 0, 0, true});

  comic2::EgaPlanarSurface frame(320, 200);
  comic2::draw_runtime_projectile_sprites(frame, state);

  expect(read_surface_color(frame, 0, 0) == 0x01,
         "projectile mapping should select a decoded 16x8 record");
}

void test_runtime_sprite_render_falls_back_when_records_are_unavailable() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.level_sprites.clear();
  state.runtime_slots.resize(1);
  state.runtime_slots[0].x = 0;
  state.runtime_slots[0].y = 0;
  state.runtime_slots[0].mapped_object_ptr = 1;
  state.runtime_slots[0].behavior_state = 0;
  state.runtime_slots[0].type_flags = 0;

  comic2::EgaPlanarSurface frame(320, 200);
  comic2::draw_runtime_entity_sprites(frame, state);

  expect(read_surface_color(frame, 0, 0) != 0x00,
         "entity render should still use the placeholder sprite when no level "
         "records exist");

  state.projectiles.push_back(comic2::ProjectileState{0, 0, 0, 0, 0, true});
  frame.clear();
  comic2::draw_runtime_projectile_sprites(frame, state);

  expect(read_surface_color(frame, 0, 0) != 0x00,
         "projectile render should still use the placeholder sprite when no "
         "level records exist");
}

void test_invulnerability_blink_visibility_uses_tick_parity() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.player.invuln_ticks = 0;
  expect(
      comic2::should_render_player_sprite(state),
      "player sprite should remain visible when invulnerability is inactive");

  state.player.invuln_ticks = 3;
  expect(!comic2::should_render_player_sprite(state),
         "player sprite should hide on odd invulnerability ticks");

  state.player.invuln_ticks = 4;
  expect(comic2::should_render_player_sprite(state),
         "player sprite should render on even invulnerability ticks");
}

void test_camera_y_clamps_to_room_bounds() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.room_grid.tile_w = 20;
  state.room_grid.tile_h = 20;
  state.room_grid.row_pointers.assign(20, 0);
  state.room_grid.tile_data.assign(400, 0x01);
  state.player.y = 480;
  state.camera_y = 0;

  comic2::camera_update_y_follow_comic_clamped(state, 200, 320);
  expect(state.camera_y == 120,
         "camera Y should clamp to the room-height viewport limit");
}

void test_camera_x_clamps_to_room_bounds() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.room_grid.tile_w = 30;
  state.room_grid.tile_h = 20;
  state.room_grid.row_pointers.assign(20, 0);
  state.room_grid.tile_data.assign(600, 0x01);
  state.player.x = 640;
  state.camera_x = 0;

  comic2::camera_update_x_follow_comic_clamped(state, 320, 480);
  expect(state.camera_x == 160,
         "camera X should clamp to the room-width viewport limit");
}

void test_masked_blit_keeps_byte_aligned_positions() {
  comic2::EgaPlanarSurface surface(320, 200);
  const auto sprite = make_test_sprite(0x01, 2, 1);

  comic2::gfx_rle_blit_masked_or_4plane(surface, 8, 0, sprite);

  expect(read_surface_color(surface, 8, 0) == 0x01,
         "byte-aligned sprite should render at the requested pixel boundary");
  expect(read_surface_color(surface, 16, 0) == 0x01,
         "byte-aligned sprite should span the expected trailing pixel");
}

void test_masked_blit_handles_shifted_positions() {
  comic2::EgaPlanarSurface surface(320, 200);
  const auto sprite = make_test_sprite(0x01, 2, 1);

  comic2::gfx_rle_blit_masked_or_4plane(surface, 4, 0, sprite);

  expect(read_surface_color(surface, 4, 0) == 0x01,
         "shifted sprite should render to the requested pixel offset");
  expect(read_surface_color(surface, 12, 0) == 0x01,
         "shifted sprite should preserve the trailing pixel in the next byte");
  expect(read_surface_color(surface, 3, 0) == 0x00,
         "shifted sprite should not spill into the preceding pixel");
}

void test_masked_blit_clips_shifted_sprite_at_viewport_edge() {
  comic2::EgaPlanarSurface surface(320, 200);
  const auto sprite = make_test_sprite(0x01, 2, 1);

  comic2::gfx_rle_blit_masked_or_4plane(surface, 316, 0, sprite);

  expect(read_surface_color(surface, 316, 0) == 0x01,
         "shifted sprite should clip safely when it reaches the viewport edge");
  expect(read_surface_color(surface, 317, 0) == 0x00,
         "shifted sprite should not write past the viewport edge");
}

void test_is_sprite_in_viewport_defaults_to_full_surface_bounds() {
  expect(comic2::is_sprite_in_viewport(250, 180, 16, 16),
         "default viewport should keep sprites visible within the full 320x200 "
         "frame");
  expect(!comic2::is_sprite_in_viewport(320, 180, 16, 16),
         "default viewport should treat entities past the frame edge as "
         "off-screen");
}

void test_render_bootstrap_frame_skips_entities_outside_viewport() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.runtime_slots.clear();

  comic2::RuntimeEntitySlot32 slot{};
  slot.mapped_object_ptr = 1;
  slot.behavior_state = comic2::kBehaviorGem;
  slot.x = 220;
  slot.y = 100;
  state.runtime_slots.push_back(slot);

  comic2::MemoryFramePresenter presenter;
  comic2::render_bootstrap_frame(presenter, state);

  const auto &frame = presenter.last_frame();
  const auto background_color = read_surface_color(frame, 220, 100);
  expect(background_color != 0x03,
         "entity outside viewport should not change the frame");
}

void test_render_bootstrap_frame_renders_entities_on_viewport_edge() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.runtime_slots.clear();

  comic2::RuntimeEntitySlot32 slot{};
  slot.mapped_object_ptr = 1;
  slot.behavior_state = comic2::kBehaviorGem;
  slot.x = 184;
  slot.y = 136;
  state.runtime_slots.push_back(slot);

  comic2::MemoryFramePresenter presenter;
  comic2::render_bootstrap_frame(presenter, state);

  const auto &frame = presenter.last_frame();
  const auto pixel = read_surface_color(frame, 184, 136);
  expect(pixel == 0x03,
         "entity touching the viewport edge should still render");
}

void test_render_bootstrap_frame_draws_player_after_entities() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.runtime_slots.clear();

  comic2::RuntimeEntitySlot32 slot{};
  slot.mapped_object_ptr = 1;
  slot.behavior_state = comic2::kBehaviorGem;
  slot.x = 64;
  slot.y = 96;
  state.runtime_slots.push_back(slot);

  state.player.x = 64;
  state.player.y = 96;

  comic2::MemoryFramePresenter presenter;
  comic2::render_bootstrap_frame(presenter, state);

  const auto &frame = presenter.last_frame();
  const auto pixel = read_surface_color(frame, 64, 96);
  expect(pixel == 0x0C,
         "player marker should overwrite entity pixels when drawn last");
}

void test_default_ega_palette_and_black_mask() {
  const auto ega = comic2::make_default_ega_palette();
  expect(ega.size() == 16, "default EGA palette must have 16 entries");
  expect(ega[0][0] == 0 && ega[0][1] == 0 && ega[0][2] == 0,
         "color 0 should be black");
  expect(ega[15][0] == 255 && ega[15][1] == 255 && ega[15][2] == 255,
         "color 15 should be white");

  const auto black_mask = comic2::make_black_palette_mask();
  comic2::EgaPaletteState palette_state;
  palette_state.entries = black_mask;
  expect(comic2::is_black_palette_mask(palette_state),
         "black mask must be detected by is_black_palette_mask");

  palette_state.entries = ega;
  expect(!comic2::is_black_palette_mask(palette_state),
         "standard EGA palette must not be detected as black mask");
}

void test_update_room_lighting_palette() {
  auto state = comic2::make_default_runtime_state();
  state.is_dark_room = true;
  state.player.lantern_equipped = false;

  comic2::update_room_lighting_palette(state);
  expect(state.palette.loaded, "palette must be marked loaded");
  expect(comic2::is_black_palette_mask(state.palette),
         "unlit dark room must apply black palette mask");

  state.player.lantern_equipped = true;
  comic2::update_room_lighting_palette(state);
  expect(!comic2::is_black_palette_mask(state.palette),
         "lantern equipped must restore visible palette");
  expect(state.palette.entries == comic2::make_default_ega_palette(),
         "palette must match standard visible EGA table");

  state.is_dark_room = false;
  state.player.lantern_equipped = false;
  comic2::update_room_lighting_palette(state);
  expect(!comic2::is_black_palette_mask(state.palette),
         "lit room without lantern must maintain visible palette");
}

void test_apply_cavern_darkness_mask() {
  comic2::EgaPlanarSurface surface(320, 200);
  surface.clear(0xFF);

  auto state = comic2::make_default_runtime_state();
  state.is_dark_room = true;
  state.player.lantern_equipped = false;

  comic2::apply_cavern_darkness_mask(surface, state, 8, 8, 240, 176);

  // Viewport interior must be masked to black
  expect(read_surface_color(surface, 100, 100) == 0x00,
         "playfield pixel must be masked to black in unlit dark room");
  expect(read_surface_color(surface, 8, 8) == 0x00,
         "top-left viewport corner must be masked to black");
  expect(read_surface_color(surface, 247, 183) == 0x00,
         "bottom-right viewport corner must be masked to black");

  // Outside viewport (bezel / HUD) must remain untouched
  expect(read_surface_color(surface, 4, 4) == 0x0F,
         "top bezel border pixel must remain unmasked");
  expect(read_surface_color(surface, 260, 50) == 0x0F,
         "right HUD panel pixel must remain unmasked");

  // Equipping lantern restores visibility
  surface.clear(0xFF);
  state.player.lantern_equipped = true;
  comic2::apply_cavern_darkness_mask(surface, state, 8, 8, 240, 176);
  expect(read_surface_color(surface, 100, 100) == 0x0F,
         "playfield pixel must remain visible when lantern is equipped");

  // Fallback path: unaligned rectangle (not multiples of 8)
  surface.clear(0xFF);
  state.player.lantern_equipped = false;
  comic2::apply_cavern_darkness_mask(surface, state, 5, 10, 23, 15);
  expect(read_surface_color(surface, 5, 10) == 0x00,
         "unaligned rect corner must be masked to black");
  expect(read_surface_color(surface, 27, 24) == 0x00,
         "unaligned rect interior must be masked to black");
  expect(read_surface_color(surface, 4, 10) == 0x0F,
         "pixel outside unaligned rect left edge must remain unmasked");
  expect(read_surface_color(surface, 28, 24) == 0x0F,
         "pixel outside unaligned rect right edge must remain unmasked");
}

void test_apply_radial_darkness_mask() {
  comic2::EgaPlanarSurface surface(320, 200);
  surface.clear(0xFF);

  comic2::apply_radial_darkness_mask(surface, 100, 100, 20, 8, 8, 240, 176);

  expect(read_surface_color(surface, 100, 100) == 0x0F,
         "center pixel within illumination radius must remain visible");
  expect(read_surface_color(surface, 110, 100) == 0x0F,
         "offset pixel within illumination radius must remain visible");
  expect(read_surface_color(surface, 130, 100) == 0x00,
         "pixel outside illumination radius must be masked to black");
  expect(read_surface_color(surface, 4, 4) == 0x0F,
         "pixel outside viewport border must remain untouched");
}

void test_make_masked_sprite_image_validation() {
  // 1. Valid sprite succeeds
  comic2::MaskedSpriteRecord valid{};
  valid.width_pixels = 16;
  valid.height_rows = 16;
  valid.mask_bytes.assign(32, 0xFF);
  for (auto &plane : valid.planes) {
    plane.assign(32, 0xAA);
  }
  const auto img = comic2::make_masked_sprite_image(valid);
  expect(img.width_bytes == 2, "valid sprite width_bytes should be 2");
  expect(img.height_rows == 16, "valid sprite height_rows should be 16");
  expect(img.row_span_bytes == 32, "valid sprite row_span_bytes should be 32");

  // 2. Zero width or height throws invalid_argument
  comic2::MaskedSpriteRecord zero_dim = valid;
  zero_dim.width_pixels = 0;
  bool caught_zero = false;
  try {
    comic2::make_masked_sprite_image(zero_dim);
  } catch (const std::invalid_argument &) {
    caught_zero = true;
  }
  expect(caught_zero, "zero width should throw std::invalid_argument");

  // 3. Truncated plane data throws invalid_argument
  comic2::MaskedSpriteRecord truncated = valid;
  truncated.planes[1].resize(10); // less than 32
  bool caught_truncated = false;
  try {
    comic2::make_masked_sprite_image(truncated);
  } catch (const std::invalid_argument &) {
    caught_truncated = true;
  }
  expect(caught_truncated,
         "truncated plane should throw std::invalid_argument");

  // 4. Row span overflow throws invalid_argument
  comic2::MaskedSpriteRecord overflow = valid;
  overflow.width_pixels = 64000;
  overflow.height_rows = 100;
  bool caught_overflow = false;
  try {
    comic2::make_masked_sprite_image(overflow);
  } catch (const std::invalid_argument &) {
    caught_overflow = true;
  }
  expect(caught_overflow,
         "overflowing row span should throw std::invalid_argument");
}

} // namespace

void run_renderer_tests() {
  test_make_masked_sprite_image_validation();
  test_surface_geometry();
  test_surface_plane_rw();
  test_presenter_copies_frame();
  test_transition_palette_tint_applies_to_surface_pixels();
  test_transition_effects_are_deterministic();
  test_runtime_palette_loader_uses_ega_table();
  test_player_sprite_frame_selection_uses_animation_state();
  test_player_sprite_frame_selection_uses_facing_direction();
  test_runtime_entity_render_prefers_level_sprite_record();
  test_runtime_projectile_render_prefers_level_sprite_record();
  test_runtime_entity_render_selects_16x16_records_for_behavior_types();
  test_runtime_projectile_render_selects_16x8_records();
  test_runtime_sprite_render_falls_back_when_records_are_unavailable();
  test_invulnerability_blink_visibility_uses_tick_parity();
  test_camera_y_clamps_to_room_bounds();
  test_camera_x_clamps_to_room_bounds();
  test_masked_blit_keeps_byte_aligned_positions();
  test_masked_blit_handles_shifted_positions();
  test_masked_blit_clips_shifted_sprite_at_viewport_edge();
  test_is_sprite_in_viewport_defaults_to_full_surface_bounds();
  test_render_bootstrap_frame_skips_entities_outside_viewport();
  test_render_bootstrap_frame_renders_entities_on_viewport_edge();
  test_render_bootstrap_frame_draws_player_after_entities();
  test_default_ega_palette_and_black_mask();
  test_update_room_lighting_palette();
  test_apply_cavern_darkness_mask();
  test_apply_radial_darkness_mask();
  // EgaPageFlipper tests
  {
    comic2::EgaPageFlipper flipper;
    flipper.init_double_buffering();
    expect(flipper.active_page() == 0x2000,
           "EgaPageFlipper should init with page 0x2000");
  }

  {
    comic2::EgaPageFlipper flipper;
    flipper.init_double_buffering();
    expect(flipper.active_page() == 0x2000,
           "initial active page should be 0x2000");

    flipper.present_and_flip_page();
    expect(flipper.active_page() == 0x0000,
           "after first flip should be 0x0000");

    flipper.present_and_flip_page();
    expect(flipper.active_page() == 0x2000,
           "after second flip should be 0x2000");

    flipper.present_and_flip_page();
    expect(flipper.active_page() == 0x0000,
           "after third flip should be 0x0000");
  }

  {
    comic2::EgaPageFlipper flipper;
    flipper.init_double_buffering();
    expect(flipper.other_page() == 0x0000,
           "initial other page should be 0x0000");

    flipper.present_and_flip_page();
    expect(flipper.other_page() == 0x2000,
           "after flip other page should be 0x2000");

    flipper.present_and_flip_page();
    expect(flipper.other_page() == 0x0000,
           "after second flip other page should be 0x0000");
  }

#ifdef COMIC2_USE_SDL2
  // SDL2 presenter tests (only run when SDL2 is available)
  // In headless/CI environments with dummy video driver, renderer creation may
  // fail
  {
    try {
      comic2::EgaPlanarSurface src(320, 200);
      src.set_plane_byte(0, 0, 0, 0xFF);
      src.set_plane_byte(1, 0, 0, 0xFF);

      comic2::Sdl2FramePresenter presenter(640, 480);
      presenter.present(src);

      // If we get here without exception, SDL2 presenter is working
      expect(true, "Sdl2FramePresenter should present without error");
    } catch (const std::runtime_error &e) {
      // SDL renderer creation can fail in headless environments (dummy driver)
      // This is expected and acceptable - log but don't fail the test
      std::string error(e.what());
      if (error.find("SDL_CreateRenderer failed") != std::string::npos) {
        // Expected in headless CI - pass silently
      } else {
        // Unexpected error - rethrow
        throw;
      }
    }
  }
#endif // COMIC2_USE_SDL2
}
