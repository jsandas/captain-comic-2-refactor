#include "comic2/bootstrap.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <thread>
#include <vector>

#include "comic2/default_handlers.hpp"
#include "comic2/input_handler.hpp"
#include "comic2/oracle.hpp"
#include "comic2/resource_loader.hpp"
#include "comic2/ui.hpp"

namespace comic2 {

namespace {
std::size_t fallback_player_sprite_frame_index(const RuntimeState &state) {
  const auto animation_state =
      static_cast<PlayerAnimationState>(state.player.animation_state);
  const auto facing_offset = state.player.facing_right ? 0U : 8U;

  switch (animation_state) {
  case PlayerAnimationState::WalkCycle:
    return static_cast<std::size_t>(state.player.animation_frame) +
           facing_offset;
  case PlayerAnimationState::JumpRise:
  case PlayerAnimationState::JumpFall:
    return 4U + facing_offset;
  case PlayerAnimationState::Attack:
    return 5U + facing_offset;
  case PlayerAnimationState::Hurt:
    return 6U + facing_offset;
  case PlayerAnimationState::Death:
    return 7U + facing_offset;
  case PlayerAnimationState::Idle:
  default:
    return static_cast<std::size_t>(state.player.hp % 4U) + facing_offset;
  }
}

bool has_valid_player_sprite_sheet(const RuntimeState &state) {
  if (state.level_sprites.size() < 16U) {
    return false;
  }

  return std::all_of(state.level_sprites.begin(), state.level_sprites.end(),
                     [](const MaskedSpriteRecord &sprite) {
                       return sprite.width_pixels > 0U &&
                              sprite.height_rows > 0U &&
                              !sprite.mask_bytes.empty() &&
                              !sprite.planes[0].empty();
                     });
}

std::size_t mapped_player_sprite_frame_index(const RuntimeState &state) {
  const auto animation_state =
      static_cast<PlayerAnimationState>(state.player.animation_state);
  const auto facing_offset = state.player.facing_right ? 0U : 8U;

  std::size_t base_index = 0U;
  switch (animation_state) {
  case PlayerAnimationState::Idle:
    base_index = 0U;
    break;
  case PlayerAnimationState::WalkCycle:
    base_index =
        1U + (static_cast<std::size_t>(state.player.animation_frame) % 3U);
    break;
  case PlayerAnimationState::JumpRise:
  case PlayerAnimationState::JumpFall:
    base_index = 4U;
    break;
  case PlayerAnimationState::Attack:
    base_index = 5U;
    break;
  case PlayerAnimationState::Hurt:
    base_index = 6U;
    break;
  case PlayerAnimationState::Death:
    base_index = 7U;
    break;
  default:
    base_index = 0U;
    break;
  }

  return base_index + facing_offset;
}

bool draw_level_sprite_record(EgaPlanarSurface &frame,
                              const RuntimeState &state,
                              std::size_t sprite_index) {
  if (sprite_index >= state.level_sprites.size()) {
    return false;
  }

  const auto &record = state.level_sprites[sprite_index];
  if (record.width_pixels == 0U || record.height_rows == 0U ||
      record.mask_bytes.size() != record.mask_byte_count() ||
      record.planes[0].size() != record.mask_byte_count() ||
      record.mask_bytes.empty() || record.planes[0].empty()) {
    return false;
  }

  Ega4PlaneImage sprite{};
  sprite.width_bytes =
      static_cast<std::uint16_t>((record.width_pixels + 7U) / 8U);
  sprite.height_rows = record.height_rows;
  sprite.row_span_bytes = static_cast<std::uint16_t>(
      sprite.width_bytes * static_cast<std::uint16_t>(record.height_rows));

  for (std::size_t plane = 0; plane < record.planes.size(); ++plane) {
    const auto &source = record.planes[plane];
    auto &target = sprite.planes[plane];
    target.assign(source.begin(), source.end());
  }

  const std::int32_t px = state.player.x - state.camera_x;
  const std::int32_t py = state.player.y - state.camera_y;
  if (!is_sprite_in_viewport(px, py,
                             static_cast<std::int32_t>(record.width_pixels),
                             static_cast<std::int32_t>(record.height_rows))) {
    return true;
  }

  const std::size_t clamped_px =
      static_cast<std::size_t>(std::max<std::int16_t>(
          0, std::min<std::int16_t>(px, frame.width_pixels() -
                                            record.width_pixels)));
  const std::size_t clamped_py =
      static_cast<std::size_t>(std::max<std::int16_t>(
          0, std::min<std::int16_t>(py,
                                    frame.height_rows() - record.height_rows)));
  gfx_rle_blit_masked_or_4plane(frame, clamped_px, clamped_py, sprite);
  return true;
}
} // namespace

std::size_t select_player_sprite_frame(const RuntimeState &state) {
  if (has_valid_player_sprite_sheet(state)) {
    return mapped_player_sprite_frame_index(state);
  }

  return fallback_player_sprite_frame_index(state);
}

bool should_render_player_sprite(const RuntimeState &state) {
  if (state.player.invuln_ticks == 0) {
    return true;
  }

  return (state.player.invuln_ticks % 2U) == 0U;
}

bool should_render_timed_overlay(const RuntimeState &state) {
  return state.player.overlay_active && state.player.overlay_ticks > 0;
}

namespace {

constexpr int kDefaultBootstrapTicks = 2;
constexpr std::int16_t kTileSizePixels = 16;
constexpr std::chrono::milliseconds kOriginalTickInterval{110};

// Global keyboard input handler (initialized on first use)
KeyboardInputHandler *g_keyboard_handler = nullptr;

KeyboardInputHandler *get_keyboard_handler() {
  static KeyboardInputHandler handler;
  return &handler;
}

bool has_room_grid_data(const RuntimeState &state) {
  return state.room_grid.tile_w > 0 && state.room_grid.tile_h > 0 &&
         !state.room_grid.row_pointers.empty() &&
         !state.room_grid.tile_data.empty();
}

bool has_valid_level_tileset(const RuntimeState &state) {
  if (state.level_tileset.empty()) {
    return false;
  }

  for (const auto &tile : state.level_tileset) {
    if (tile.width_bytes != 2U || tile.height_rows != kTileSizePixels ||
        tile.row_span_bytes != 32U) {
      return false;
    }

    for (const auto &plane : tile.planes) {
      if (plane.size() != 32U) {
        return false;
      }
    }
  }

  return true;
}

void clamp_player_to_room_bounds(RuntimeState &state) {
  if (!has_room_grid_data(state)) {
    return;
  }

  const std::int32_t room_pixel_w =
      static_cast<std::int32_t>(state.room_grid.tile_w) * kTileSizePixels;
  const std::int32_t room_pixel_h =
      static_cast<std::int32_t>(state.room_grid.tile_h) * kTileSizePixels;

  const std::int32_t max_x =
      std::min<std::int32_t>(319, std::max<std::int32_t>(0, room_pixel_w - 8));
  const std::int32_t max_y = std::min<std::int32_t>(
      199, std::max<std::int32_t>(0, room_pixel_h - kTileSizePixels));

  if (state.player.x < 0) {
    state.player.x = 0;
  } else if (state.player.x > max_x) {
    state.player.x = static_cast<std::int16_t>(max_x);
  }

  if (state.player.y < 0) {
    state.player.y = 0;
  } else if (state.player.y > max_y) {
    state.player.y = static_cast<std::int16_t>(max_y);
  }
}

bool is_related_asset_root(const std::filesystem::path &lhs,
                           const std::filesystem::path &rhs) {
  if (lhs.empty() || rhs.empty()) {
    return false;
  }

  std::error_code ec_a;
  std::error_code ec_b;
  const auto left = std::filesystem::weakly_canonical(lhs, ec_a);
  const auto right = std::filesystem::weakly_canonical(rhs, ec_b);

  const auto canonical_left = ec_a ? lhs.lexically_normal() : left;
  const auto canonical_right = ec_b ? rhs.lexically_normal() : right;

  if (canonical_left == canonical_right) {
    return true;
  }

  std::filesystem::path cursor = canonical_left;
  for (;;) {
    if (cursor == canonical_right) {
      return true;
    }
    const auto parent = cursor.parent_path();
    if (parent == cursor || parent.empty()) {
      break;
    }
    cursor = parent;
  }

  cursor = canonical_right;
  for (;;) {
    if (cursor == canonical_left) {
      return true;
    }
    const auto parent = cursor.parent_path();
    if (parent == cursor || parent.empty()) {
      break;
    }
    cursor = parent;
  }

  return false;
}

std::vector<std::filesystem::path>
build_asset_root_candidates(const std::filesystem::path &root) {
  std::vector<std::filesystem::path> candidate_seeds;
  if (!root.empty()) {
    candidate_seeds.push_back(root);
  }

  const auto current = std::filesystem::current_path();
  if (!current.empty() && current != root &&
      is_related_asset_root(current, root)) {
    candidate_seeds.push_back(current);
  }

  std::vector<std::filesystem::path> candidates;
  auto add_candidate = [&](const std::filesystem::path &candidate) {
    if (candidate.empty()) {
      return;
    }

    std::error_code ec;
    const auto canonical = std::filesystem::weakly_canonical(candidate, ec);
    const auto key = ec ? candidate.lexically_normal() : canonical;

    const bool already_present =
        std::any_of(candidates.begin(), candidates.end(),
                    [&](const auto &existing) { return existing == key; });

    if (!already_present) {
      candidates.push_back(key);
    }
  };

  for (const auto &seed : candidate_seeds) {
    std::filesystem::path cursor = seed;
    for (;;) {
      add_candidate(cursor);
      add_candidate(cursor / "reference" / "original");
      add_candidate(cursor / "original");

      const auto parent = cursor.parent_path();
      if (parent == cursor || parent.empty()) {
        break;
      }
      cursor = parent;
    }
  }

  return candidates;
}

void init_keyboard_handler() {
  if (!g_keyboard_handler)
    g_keyboard_handler = get_keyboard_handler();
}

void set_pixel(EgaPlanarSurface &surface, std::int32_t x, std::int32_t y,
               std::uint8_t color_index) {
  if (x < 0 || y < 0 || x >= surface.width_pixels() ||
      y >= surface.height_rows()) {
    return;
  }

  const auto x_byte = static_cast<std::size_t>(x / 8);
  const auto bit = static_cast<std::uint8_t>(7 - (x % 8));
  const auto y_row = static_cast<std::size_t>(y);
  const auto mask = static_cast<std::uint8_t>(1U << bit);

  for (std::size_t plane = 0; plane < EgaPlanarSurface::kPlaneCount; ++plane) {
    auto value = surface.get_plane_byte(plane, x_byte, y_row);
    if ((color_index >> plane) & 0x1U) {
      value = static_cast<std::uint8_t>(value | mask);
    } else {
      value =
          static_cast<std::uint8_t>(value & static_cast<std::uint8_t>(~mask));
    }
    surface.set_plane_byte(plane, x_byte, y_row, value);
  }
}

std::uint8_t read_room_tile(const RuntimeState &state, std::size_t tile_x,
                            std::size_t tile_y) {
  if (tile_x >= state.room_grid.tile_w || tile_y >= state.room_grid.tile_h ||
      tile_y >= state.room_grid.row_pointers.size()) {
    return 0;
  }

  const std::size_t row_base = state.room_grid.row_pointers[tile_y];
  const std::size_t off = row_base + tile_x;
  if (off >= state.room_grid.tile_data.size()) {
    return 0;
  }
  return state.room_grid.tile_data[off];
}

void copy_bezel_canvas(EgaPlanarSurface &frame, const Ega4PlaneImage &bezel) {
  const std::size_t width_bytes =
      std::min<std::size_t>(bezel.width_bytes, frame.row_stride_bytes());
  const std::size_t height_rows =
      std::min<std::size_t>(bezel.height_rows, frame.height_rows());
  const std::size_t bezel_row_bytes =
      bezel.width_bytes == 0U ? 1U
                              : static_cast<std::size_t>(bezel.width_bytes);

  for (std::size_t plane = 0; plane < EgaPlanarSurface::kPlaneCount; ++plane) {
    const auto &source = bezel.planes[plane];
    if (source.empty()) {
      continue;
    }

    for (std::size_t y = 0; y < height_rows; ++y) {
      const auto source_row_offset = y * bezel_row_bytes;
      if (source_row_offset + width_bytes > source.size()) {
        break;
      }
      for (std::size_t x = 0; x < width_bytes; ++x) {
        frame.set_plane_byte(plane, x, y, source[source_row_offset + x]);
      }
    }
  }
}

void draw_fallback_background(EgaPlanarSurface &frame,
                              const RuntimeState &state) {
  frame.clear(static_cast<std::uint8_t>(
      (state.current_level * 5u + state.current_room * 3u + state.player.hp) &
      0x0F));
  for (std::size_t y = 0; y < frame.height_rows(); ++y) {
    const auto x = static_cast<std::int32_t>((y % 20U) * 16U);
    set_pixel(frame, x, static_cast<std::int32_t>(y), 0x0F);
    set_pixel(frame, x + 1, static_cast<std::int32_t>(y), 0x07);
  }
}

std::uint8_t expand_color_bit(bool enabled, std::uint8_t mask) {
  return enabled ? mask : 0x00;
}

void draw_room_tile(EgaPlanarSurface &frame, std::int32_t px0, std::int32_t py0,
                    std::uint8_t base_color, std::uint8_t accent_color) {
  if (px0 < 0 || py0 < 0 || px0 + kTileSizePixels > frame.width_pixels() ||
      py0 + kTileSizePixels > frame.height_rows()) {
    return;
  }

  const std::size_t x_byte0 = static_cast<std::size_t>(px0 / 8);
  const std::size_t x_byte1 = x_byte0 + 1;
  const std::uint8_t accent_top_bottom = 0xFF;

  for (std::size_t plane = 0; plane < EgaPlanarSurface::kPlaneCount; ++plane) {
    const bool base_plane_on = ((base_color >> plane) & 0x1U) != 0;
    const bool accent_plane_on = ((accent_color >> plane) & 0x1U) != 0;
    const std::uint8_t middle_left =
        static_cast<std::uint8_t>(expand_color_bit(base_plane_on, 0x7F) |
                                  expand_color_bit(accent_plane_on, 0x80));
    const std::uint8_t middle_right =
        static_cast<std::uint8_t>(expand_color_bit(base_plane_on, 0xFE) |
                                  expand_color_bit(accent_plane_on, 0x01));
    const std::uint8_t edge_fill =
        expand_color_bit(accent_plane_on, accent_top_bottom);

    for (std::int32_t py = 0; py < kTileSizePixels; ++py) {
      const auto y_row = static_cast<std::size_t>(py0 + py);
      if (py == 0 || py == kTileSizePixels - 1) {
        frame.set_plane_byte(plane, x_byte0, y_row, edge_fill);
        frame.set_plane_byte(plane, x_byte1, y_row, edge_fill);
      } else {
        frame.set_plane_byte(plane, x_byte0, y_row, middle_left);
        frame.set_plane_byte(plane, x_byte1, y_row, middle_right);
      }
    }
  }
}

void draw_room_tilemap(EgaPlanarSurface &frame, const RuntimeState &state) {
  const std::size_t visible_tiles_x =
      static_cast<std::size_t>(frame.width_pixels()) / kTileSizePixels;
  const std::size_t visible_tiles_y =
      static_cast<std::size_t>(frame.height_rows()) / kTileSizePixels;

  const std::size_t start_tile_x =
      state.camera_x > 0 ? static_cast<std::size_t>(state.camera_x) /
                               static_cast<std::size_t>(kTileSizePixels)
                         : 0U;
  const std::size_t start_tile_y =
      state.camera_y > 0 ? static_cast<std::size_t>(state.camera_y) /
                               static_cast<std::size_t>(kTileSizePixels)
                         : 0U;
  const std::size_t end_tile_x =
      std::min(static_cast<std::size_t>(state.room_grid.tile_w),
               start_tile_x + visible_tiles_x + 1U);
  const std::size_t end_tile_y =
      std::min(static_cast<std::size_t>(state.room_grid.tile_h),
               start_tile_y + visible_tiles_y + 1U);

  for (std::size_t tile_y = start_tile_y; tile_y < end_tile_y; ++tile_y) {
    for (std::size_t tile_x = start_tile_x; tile_x < end_tile_x; ++tile_x) {
      const std::uint8_t tile_id = read_room_tile(state, tile_x, tile_y);
      const std::uint8_t base_color = static_cast<std::uint8_t>(tile_id & 0x0F);
      const std::uint8_t accent_color =
          static_cast<std::uint8_t>((base_color + 2U) & 0x0F);
      const auto px0 =
          static_cast<std::int32_t>(tile_x * kTileSizePixels) - state.camera_x;
      const auto py0 =
          static_cast<std::int32_t>(tile_y * kTileSizePixels) - state.camera_y;

      if (px0 + kTileSizePixels <= 0 || px0 >= frame.width_pixels()) {
        continue;
      }
      if (py0 + kTileSizePixels <= 0 || py0 >= frame.height_rows()) {
        continue;
      }

      draw_room_tile(frame, px0, py0, base_color, accent_color);
    }
  }
}

bool normalize_asset_image_dimensions(Ega4PlaneImage &image) {
  if (image.width_bytes > 0 && image.height_rows > 0) {
    return true;
  }

  if (image.row_span_bytes == 0) {
    return false;
  }

  // Known FRPAK full-frame streams use a 40-byte row stride (320 pixels).
  if (image.row_span_bytes % 40 == 0) {
    image.width_bytes = 40;
    image.height_rows = static_cast<std::uint16_t>(image.row_span_bytes / 40);
    return image.height_rows > 0;
  }

  return false;
}

bool extract_tile_from_asset(const Ega4PlaneImage &atlas,
                             std::size_t tile_index, Ega4PlaneImage &tile) {
  if (atlas.width_bytes < 2 || atlas.height_rows < kTileSizePixels) {
    return false;
  }

  const std::size_t tiles_per_row = atlas.width_bytes / 2;
  const std::size_t tiles_per_col =
      static_cast<std::size_t>(atlas.height_rows) / kTileSizePixels;
  if (tiles_per_row == 0 || tiles_per_col == 0) {
    return false;
  }

  const std::size_t tile_count = tiles_per_row * tiles_per_col;
  const std::size_t wrapped_index = tile_index % tile_count;
  const std::size_t src_tile_x_bytes = (wrapped_index % tiles_per_row) * 2;
  const std::size_t src_tile_y_rows =
      (wrapped_index / tiles_per_row) * kTileSizePixels;

  tile = Ega4PlaneImage{};
  tile.width_bytes = 2;
  tile.height_rows = static_cast<std::uint16_t>(kTileSizePixels);
  tile.row_span_bytes = static_cast<std::uint16_t>(2 * kTileSizePixels);

  for (std::size_t plane = 0; plane < tile.planes.size(); ++plane) {
    const auto &src = atlas.planes[plane];
    auto &dst = tile.planes[plane];
    dst.resize(static_cast<std::size_t>(tile.width_bytes) * tile.height_rows);

    for (std::size_t row = 0; row < tile.height_rows; ++row) {
      const std::size_t src_off =
          (src_tile_y_rows + row) * atlas.width_bytes + src_tile_x_bytes;
      const std::size_t dst_off = row * tile.width_bytes;
      if (src_off + tile.width_bytes > src.size()) {
        return false;
      }
      dst[dst_off] = src[src_off];
      dst[dst_off + 1] = src[src_off + 1];
    }
  }

  return true;
}

bool draw_room_tilemap_from_level_tileset(EgaPlanarSurface &frame,
                                          const RuntimeState &state) {
  if (!has_room_grid_data(state) || !has_valid_level_tileset(state)) {
    return false;
  }

  const std::size_t visible_tiles_x =
      static_cast<std::size_t>(frame.width_pixels()) / kTileSizePixels;
  const std::size_t start_tile_x =
      state.camera_x > 0 ? static_cast<std::size_t>(state.camera_x) /
                               static_cast<std::size_t>(kTileSizePixels)
                         : 0U;
  const std::size_t end_tile_x =
      std::min(static_cast<std::size_t>(state.room_grid.tile_w),
               start_tile_x + visible_tiles_x + 1U);

  for (std::size_t tile_y = 0; tile_y < state.room_grid.tile_h; ++tile_y) {
    const std::int32_t py0 =
        static_cast<std::int32_t>(tile_y * kTileSizePixels) - state.camera_y;

    if (py0 + kTileSizePixels <= 0) {
      continue;
    }
    if (py0 >= static_cast<std::int32_t>(frame.height_rows())) {
      break;
    }
    if (py0 < 0) {
      continue;
    }
    if (py0 + kTileSizePixels >
        static_cast<std::int32_t>(frame.height_rows())) {
      break;
    }

    for (std::size_t tile_x = start_tile_x; tile_x < end_tile_x; ++tile_x) {
      const std::uint8_t tile_id = read_room_tile(state, tile_x, tile_y);
      if (tile_id >= state.level_tileset.size()) {
        return false;
      }

      const std::int32_t px0 =
          static_cast<std::int32_t>(tile_x * kTileSizePixels) - state.camera_x;
      if (px0 < 0 || px0 + kTileSizePixels > frame.width_pixels()) {
        continue;
      }

      gfx_rle_blit_opaque_4plane(frame, static_cast<std::size_t>(px0),
                                 static_cast<std::size_t>(py0),
                                 state.level_tileset[tile_id]);
    }
  }

  return true;
}

bool draw_room_tilemap_from_asset(EgaPlanarSurface &frame,
                                  const RuntimeState &state,
                                  const Ega4PlaneImage &atlas) {
  if (!has_room_grid_data(state)) {
    return false;
  }
  if (atlas.width_bytes < 2 || atlas.height_rows < kTileSizePixels) {
    return false;
  }

  const std::size_t visible_tiles_x =
      static_cast<std::size_t>(frame.width_pixels()) / kTileSizePixels;
  const std::size_t start_tile_x =
      state.camera_x > 0 ? static_cast<std::size_t>(state.camera_x) /
                               static_cast<std::size_t>(kTileSizePixels)
                         : 0U;
  const std::size_t end_tile_x =
      std::min(static_cast<std::size_t>(state.room_grid.tile_w),
               start_tile_x + visible_tiles_x + 1U);

  Ega4PlaneImage tile;
  for (std::size_t tile_y = 0; tile_y < state.room_grid.tile_h; ++tile_y) {
    const std::int32_t py0 =
        static_cast<std::int32_t>(tile_y * kTileSizePixels) - state.camera_y;

    // Skip tiles that are completely off-screen
    if (py0 + kTileSizePixels <= 0) {
      continue; // Tile is completely above viewport
    }
    if (py0 >= static_cast<std::int32_t>(frame.height_rows())) {
      break; // All remaining tiles are below viewport
    }
    if (py0 < 0) {
      continue; // Tile is partially above viewport (no clipping support yet)
    }
    if (py0 + kTileSizePixels >
        static_cast<std::int32_t>(frame.height_rows())) {
      break; // Tile is partially below viewport (no clipping support yet)
    }
    for (std::size_t tile_x = start_tile_x; tile_x < end_tile_x; ++tile_x) {
      const std::uint8_t tile_id = read_room_tile(state, tile_x, tile_y);
      if (!extract_tile_from_asset(atlas, tile_id, tile)) {
        return false;
      }

      const std::int32_t px0 =
          static_cast<std::int32_t>(tile_x * kTileSizePixels) - state.camera_x;
      if (px0 < 0 || px0 + kTileSizePixels > frame.width_pixels()) {
        continue; // Partial horizontal tiles unsupported (no clipping yet)
      }
      gfx_rle_blit_opaque_4plane(frame, static_cast<std::size_t>(px0),
                                 static_cast<std::size_t>(py0), tile);
    }
  }

  return true;
}

bool draw_player_sprite_from_asset(EgaPlanarSurface &frame,
                                   const RuntimeState &state,
                                   const Ega4PlaneImage &atlas) {
  if (has_valid_player_sprite_sheet(state)) {
    const std::size_t sprite_index = select_player_sprite_frame(state);
    if (draw_level_sprite_record(frame, state, sprite_index)) {
      return true;
    }
  }

  const std::size_t sprite_index = select_player_sprite_frame(state);
  Ega4PlaneImage sprite;
  if (!extract_tile_from_asset(atlas, sprite_index, sprite)) {
    return false;
  }

  const std::int32_t px = state.player.x - state.camera_x;
  const std::int32_t py = state.player.y - state.camera_y;
  if (!is_sprite_in_viewport(px, py, 16, 16)) {
    return true;
  }

  const std::size_t clamped_px =
      static_cast<std::size_t>(std::max<std::int16_t>(
          0, std::min<std::int16_t>(px, frame.width_pixels() - 16)));
  const std::size_t clamped_py =
      static_cast<std::size_t>(std::max<std::int16_t>(
          0, std::min<std::int16_t>(py, frame.height_rows() - 16)));
  gfx_rle_blit_masked_or_4plane(frame, clamped_px, clamped_py, sprite);
  return true;
}
bool draw_timed_overlay_sprite_from_asset(EgaPlanarSurface &frame,
                                          const RuntimeState &state,
                                          const Ega4PlaneImage &atlas) {
  if (!should_render_timed_overlay(state)) {
    return false;
  }

  Ega4PlaneImage sprite;
  const std::size_t sprite_index =
      static_cast<std::size_t>(state.player.overlay_sprite_frame);
  if (!extract_tile_from_asset(atlas, sprite_index, sprite)) {
    return false;
  }

  const std::int32_t px = state.player.x - state.camera_x;
  const std::int32_t py = state.player.y - state.camera_y;
  if (!is_sprite_in_viewport(px, py, 16, 16)) {
    return true;
  }

  const std::size_t clamped_px =
      static_cast<std::size_t>(std::max<std::int16_t>(
          0, std::min<std::int16_t>(px, frame.width_pixels() - 16)));
  const std::size_t clamped_py =
      static_cast<std::size_t>(std::max<std::int16_t>(
          0, std::min<std::int16_t>(py, frame.height_rows() - 16)));
  gfx_rle_blit_masked_or_4plane(frame, clamped_px, clamped_py, sprite);
  return true;
}
std::optional<Ega4PlaneImage> try_decode_bootstrap_asset(RuntimeState &state) {
  if (state.frpak_catalog.files.empty()) {
    return std::nullopt;
  }

  const auto &file = state.frpak_catalog.files.front();
  const auto decoded = decode_frpak_record(state, file.pak_id, 0);
  if (!decoded.has_value()) {
    return std::nullopt;
  }

  Ega4PlaneImage image = *decoded;
  if (!normalize_asset_image_dimensions(image)) {
    return std::nullopt;
  }
  return image;
}

void draw_player_marker(EgaPlanarSurface &frame, const RuntimeState &state) {
  const std::int32_t px0 = state.player.x - state.camera_x;
  const std::int32_t py0 = state.player.y - state.camera_y;
  const std::uint8_t body_color = state.player.is_airborne ? 0x0E : 0x0C;

  if (!is_sprite_in_viewport(px0, py0, 8, 16)) {
    return;
  }

  for (std::int32_t py = 0; py < 16; ++py) {
    for (std::int32_t px = 0; px < 8; ++px) {
      set_pixel(frame, px0 + px, py0 + py, body_color);
    }
  }
}

void draw_room_event_anchor_sprite(EgaPlanarSurface &frame,
                                   const RuntimeState &state) {
  const auto &anchor = state.room_event_anchor;
  if (!anchor.active) {
    return;
  }

  const std::int32_t px0 = anchor.x - state.camera_x;
  const std::int32_t py0 = anchor.y - state.camera_y;
  if (!is_sprite_in_viewport(px0, py0, 12, 12)) {
    return;
  }

  for (std::int32_t py = 0; py < 12; ++py) {
    for (std::int32_t px = 0; px < 12; ++px) {
      const bool is_border = py == 0 || py == 11 || px == 0 || px == 11;
      const bool is_center_cross = (px == 5 || px == 6 || py == 5 || py == 6);
      if (is_border || is_center_cross) {
        set_pixel(frame, px0 + px, py0 + py, 0x0B);
      }
    }
  }
}

} // namespace

SceneBootstrapSummary
initialize_runtime_scene(RuntimeState &state,
                         const std::filesystem::path &root) {
  SceneBootstrapSummary summary{};

  const auto candidates = build_asset_root_candidates(root);
  for (const auto &candidate : candidates) {
    RuntimeState candidate_state = state;
    const auto canonical_candidate =
        std::filesystem::weakly_canonical(candidate);
    const auto load =
        load_initial_bootstrap_resources(candidate_state, canonical_candidate);
    summary.metadata_files_tried += load.metadata_files_tried;
    summary.sprite_files_tried += load.sprite_files_tried;

    if (load.room_grid_loaded) {
      candidate_state.player.is_physics_active = true;
      clamp_player_to_room_bounds(candidate_state);
      candidate_state.assets_root = canonical_candidate;
      state = candidate_state;
      summary.room_grid_loaded = true;
      summary.using_placeholder = false;
      summary.assets_root_used = canonical_candidate;
      break;
    }
  }

  state.player.is_physics_active = true;

  return summary;
}

bool read_bootstrap_bool_env(const char *name) {
  const char *value = std::getenv(name);
  return value != nullptr && (*value == '1' || *value == 't' || *value == 'T' ||
                              *value == 'y' || *value == 'Y');
}

int read_bootstrap_tick_budget(int default_ticks) {
  const char *value = std::getenv("COMIC2_BOOTSTRAP_TICKS");
  if (value == nullptr || *value == '\0') {
    return default_ticks > 0 ? default_ticks : kDefaultBootstrapTicks;
  }

  try {
    const int parsed = std::stoi(value);
    return parsed > 0
               ? parsed
               : (default_ticks > 0 ? default_ticks : kDefaultBootstrapTicks);
  } catch (const std::exception &) {
    return default_ticks > 0 ? default_ticks : kDefaultBootstrapTicks;
  }
}

std::chrono::milliseconds
read_bootstrap_frame_interval(std::chrono::milliseconds default_interval) {
  const char *value = std::getenv("COMIC2_BOOTSTRAP_FRAME_MS");
  if (value == nullptr || *value == '\0') {
    return default_interval.count() > 0 ? default_interval
                                        : kOriginalTickInterval;
  }

  try {
    const int parsed = std::stoi(value);
    if (parsed > 0) {
      return std::chrono::milliseconds(parsed);
    }
  } catch (const std::exception &) {
  }

  return default_interval.count() > 0 ? default_interval
                                      : kOriginalTickInterval;
}

bool poll_bootstrap_input(RuntimeState &state) {
  init_keyboard_handler();

  if (g_keyboard_handler) {
    // Use real SDL2 keyboard input
    return g_keyboard_handler->poll_events(state.input);
  } else {
    // Fallback to environment variables
    state.input.jump_pressed = read_bootstrap_bool_env("COMIC2_INPUT_JUMP");
    state.input.left_pressed = read_bootstrap_bool_env("COMIC2_INPUT_LEFT");
    state.input.right_pressed = read_bootstrap_bool_env("COMIC2_INPUT_RIGHT");
    state.input.down_pressed = read_bootstrap_bool_env("COMIC2_INPUT_DOWN");
    return !read_bootstrap_bool_env("COMIC2_INPUT_QUIT");
  }
}

void capture_oracle_trace_if_enabled(RuntimeState &state,
                                     const GameDispatcher &dispatcher,
                                     const std::vector<InputState> &inputs,
                                     std::vector<ReplaySnapshot> &snapshots) {
  if (!read_bootstrap_bool_env("COMIC2_ORACLE_REPLAY")) {
    return;
  }

  const auto captured = capture_runtime_snapshots(
      const_cast<RuntimeState &>(state), dispatcher, inputs);
  snapshots.insert(snapshots.end(), captured.begin(), captured.end());

  const char *log_path_env = std::getenv("COMIC2_ORACLE_LOG");
  if (log_path_env == nullptr || *log_path_env == '\0') {
    return;
  }

  std::ofstream out(log_path_env, std::ios::app);
  if (!out) {
    return;
  }
  for (const auto &snapshot : captured) {
    out << "tick=" << snapshot.tick << " x=" << snapshot.player_x
        << " y=" << snapshot.player_y << " x_vel=" << snapshot.player_x_vel
        << " y_vel=" << snapshot.player_y_vel
        << " grounded=" << (snapshot.grounded ? 1 : 0)
        << " facing=" << (snapshot.facing_right ? 1 : 0)
        << " hp=" << snapshot.hp << " score=" << snapshot.score
        << " gems=" << snapshot.gems << " lives=" << snapshot.lives << "\n";
  }
}

std::size_t estimate_runtime_state_bytes(const RuntimeState &state) {
  std::size_t bytes = sizeof(RuntimeState);
  bytes += state.room_grid.tile_data.size() * sizeof(std::uint8_t);
  bytes += state.room_grid.row_pointers.size() * sizeof(std::uint16_t);
  bytes += state.mapped_objects.size() * sizeof(MappedObject12);
  bytes += state.active_entities.size() * sizeof(ActiveEntity8);
  bytes += state.runtime_slots.size() * sizeof(RuntimeEntitySlot32);
  bytes += state.projectiles.size() * sizeof(ProjectileState);
  return bytes;
}

void render_bootstrap_frame(IFramePresenter &presenter, RuntimeState &state) {
  EgaPlanarSurface frame(320, 200);
  const bool has_room_grid = has_room_grid_data(state);
  bool used_bezel_background = false;
  bool used_level_tileset_background = false;
  bool used_asset_background = false;
  std::optional<Ega4PlaneImage> asset;

  if (state.bezel_frame.has_value()) {
    copy_bezel_canvas(frame, *state.bezel_frame);
    used_bezel_background = true;
  }

  if (has_valid_level_tileset(state) && has_room_grid) {
    used_level_tileset_background =
        draw_room_tilemap_from_level_tileset(frame, state);
  }

  if (!used_level_tileset_background &&
      (asset = try_decode_bootstrap_asset(state)).has_value()) {
    used_asset_background = draw_room_tilemap_from_asset(frame, state, *asset);
  }

  if (!used_level_tileset_background && !used_asset_background &&
      has_room_grid) {
    draw_room_tilemap(frame, state);
  } else if (!used_level_tileset_background && !used_asset_background &&
             !used_bezel_background) {
    draw_fallback_background(frame, state);
  }

  // Phase 10.1: draw background first.
  draw_runtime_entity_sprites(frame, state);

  // Phase 10.1: draw projectiles above entities.
  draw_runtime_projectile_sprites(frame, state);

  // Phase 10.5: draw room-event anchors above projectiles.
  draw_room_event_anchor_sprite(frame, state);

  // Phase 10.1: draw the player above projectiles.
  if (used_asset_background && should_render_player_sprite(state)) {
    if (!draw_player_sprite_from_asset(frame, state, *asset)) {
      draw_player_marker(frame, state);
    }
  }

  if (!used_asset_background && should_render_player_sprite(state)) {
    draw_player_marker(frame, state);
  }

  // Phase 10.1: draw any timed overlay last before the HUD.
  if (used_asset_background && should_render_timed_overlay(state)) {
    if (!draw_timed_overlay_sprite_from_asset(frame, state, *asset)) {
      draw_player_marker(frame, state);
    }
  }

  if (!used_asset_background && should_render_timed_overlay(state)) {
    draw_player_marker(frame, state);
  }

  if (state.transition_state.active) {
    apply_transition_palette_tint(frame, state.transition_state);
    if (state.transition_state.effect_type == 0) {
      room_transition_palette_wave(frame, state.transition_state);
    } else {
      room_transition_reveal_sequence_a(frame, state.transition_state);
    }
  }

  if (state.ui.menu_state == MenuState::Pause ||
      state.ui.menu_state == MenuState::Options ||
      state.ui.menu_state == MenuState::Help ||
      state.ui.menu_state == MenuState::GameSelect) {
    ui_render_option_list(frame, state);
  }

  if (state.level_complete && state.ui.level_complete_modal &&
      state.ui.modal_active && state.ui.cinematic_frame != 0U) {
    event_finale_transition_sequence(frame, state);
  }

  if (state.ui.modal_active) {
    ui_render_modal_prompt(frame, state);
  }

  hud_render_overlay(frame, state);
  presenter.present(frame);
}

IntegratedLoopSummary
run_integrated_bootstrap_loop(const std::filesystem::path &root,
                              IFramePresenter &presenter, int frame_budget,
                              std::chrono::milliseconds frame_interval,
                              IAudioBackend *audio_backend) {
  auto state = make_default_runtime_state();
  const auto bootstrap = initialize_runtime_scene(state, root);

  auto dispatcher = make_default_game_dispatcher();
  dispatcher.set_trace_enabled(
      read_bootstrap_bool_env("COMIC2_TRACE_DISPATCH"));

  const auto loop = run_render_loop(state, dispatcher, presenter, frame_budget,
                                    frame_interval, audio_backend);

  return IntegratedLoopSummary{.bootstrap = bootstrap, .loop = loop};
}

FrameLoopSummary run_render_loop(RuntimeState &state,
                                 GameDispatcher &dispatcher,
                                 IFramePresenter &presenter, int frame_budget,
                                 std::chrono::milliseconds frame_interval,
                                 IAudioBackend *audio_backend) {
  FrameLoopSummary summary{};
  if (frame_budget <= 0) {
    return summary;
  }

  bool audio_enabled = false;
  if (audio_backend != nullptr) {
    audio_enabled = audio_backend->initialize();
    state.audio.audio_enabled = audio_enabled;
    if (audio_enabled) {
      queue_audio_event(state, AudioEvent::StartupChime);
      flush_audio_events(state, audio_backend);
    } else {
      // Clear any accumulated events if audio failed to initialize
      state.audio.pending_events.clear();
    }
  }

  std::vector<ReplaySnapshot> oracle_snapshots;
  auto next_tick = std::chrono::steady_clock::now();
  bool was_airborne = state.player.is_airborne;
  std::uint8_t previous_hp = state.player.hp;

  for (int frame = 0; frame < frame_budget; ++frame) {
    if (frame_interval.count() > 0) {
      const auto now = std::chrono::steady_clock::now();
      if (now < next_tick) {
        std::this_thread::sleep_until(next_tick);
      }
    }

    if (!poll_bootstrap_input(state)) {
      summary.quit_requested = true;
      break;
    }

    const auto tick_start = std::chrono::steady_clock::now();
    const auto result = dispatcher.run_tick(state);
    const auto tick_end = std::chrono::steady_clock::now();
    const auto tick_us = std::chrono::duration_cast<std::chrono::microseconds>(
                             tick_end - tick_start)
                             .count();
    summary.total_frame_time_us += static_cast<std::uint64_t>(tick_us);
    summary.max_frame_time_us = std::max(summary.max_frame_time_us,
                                         static_cast<std::uint64_t>(tick_us));

    if (audio_enabled) {
      if (!was_airborne && state.player.is_airborne) {
        queue_audio_event(state, AudioEvent::Jump);
      }
      if (previous_hp > 0 && state.player.hp == 0) {
        queue_audio_event(state, AudioEvent::Hazard);
      }
      flush_audio_events(state, audio_backend);
      audio_backend->update();
    }

    render_bootstrap_frame(presenter, state);

    if (read_bootstrap_bool_env("COMIC2_ORACLE_REPLAY")) {
      std::vector<InputState> single_frame_inputs(1, state.input);
      RuntimeState oracle_state = state;
      capture_oracle_trace_if_enabled(oracle_state, dispatcher,
                                      single_frame_inputs, oracle_snapshots);
    }

    summary.frames_rendered += 1;
    summary.ticks_processed += 1;
    summary.last_stage = result.stage;
    was_airborne = state.player.is_airborne;
    previous_hp = state.player.hp;

    if (frame_interval.count() > 0) {
      next_tick += frame_interval;
      const auto now = std::chrono::steady_clock::now();
      if (next_tick < now) {
        next_tick = now + frame_interval;
      }
    }
  }

  if (audio_enabled) {
    audio_backend->shutdown();
  }

  if (summary.frames_rendered > 0) {
    summary.average_frame_time_us =
        summary.total_frame_time_us /
        static_cast<std::uint64_t>(summary.frames_rendered);
  }
  summary.estimated_state_bytes = estimate_runtime_state_bytes(state);

  return summary;
}

int run_bootstrap_entry(const std::filesystem::path &root) {
  std::cout << "Starting comic2 bootstrap from: " << root.string() << "\n";

  auto state = make_default_runtime_state();
  const auto bootstrap = initialize_runtime_scene(state, root);

  if (!bootstrap.room_grid_loaded) {
    std::cerr << "WARNING: no bootstrap room grid loaded from " << root.string()
              << " (room loader remains provisional)\n";
  }

  std::cout << "Bootstrap resources: metadata_files="
            << bootstrap.metadata_files_tried
            << " sprite_files=" << bootstrap.sprite_files_tried
            << " room_grid_loaded=" << std::boolalpha
            << bootstrap.room_grid_loaded
            << " using_placeholder=" << bootstrap.using_placeholder
            << std::noboolalpha << "\n";
  if (!bootstrap.assets_root_used.empty()) {
    std::cout << "Assets root: " << bootstrap.assets_root_used.string() << "\n";
  }
  std::cout << "Loaded metadata bytes=" << state.level_metadata_bytes.size()
            << " room_bytes=" << state.room_resource_bytes.size()
            << " sprite_bytes=" << state.sprite_resource_bytes.size() << "\n";

  auto dispatcher = make_default_game_dispatcher();
  dispatcher.set_trace_enabled(true);

  MemoryFramePresenter presenter;
  const int tick_budget = read_bootstrap_tick_budget();
  const auto loop_summary = run_render_loop(
      state, dispatcher, presenter, tick_budget, std::chrono::milliseconds(0));

  std::cout << "Bootstrap loop frames=" << loop_summary.frames_rendered
            << " last_stage=" << to_string(loop_summary.last_stage)
            << " input_left=" << state.input.left_pressed
            << " input_right=" << state.input.right_pressed
            << " input_jump=" << state.input.jump_pressed
            << " input_down=" << state.input.down_pressed << "\n";

  std::cout << "Bootstrap ready: level=" << state.current_level
            << " room=" << state.current_room
            << " frame_captured=" << std::boolalpha << presenter.has_frame()
            << std::noboolalpha << "\n";

  return 0;
}

BootstrapTickSummary run_bootstrap_tick(RuntimeState &state,
                                        GameDispatcher &dispatcher,
                                        IFramePresenter &presenter) {

  (void)poll_bootstrap_input(state);
  const auto result = dispatcher.run_tick(state);
  render_bootstrap_frame(presenter, state);

  return BootstrapTickSummary{
      .stage = result.stage, .input_captured = true, .frame_presented = true};
}

} // namespace comic2
