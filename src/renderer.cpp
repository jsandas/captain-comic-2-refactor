#include "comic2/renderer.hpp"

#include <algorithm>
#include <stdexcept>

#ifdef COMIC2_USE_SDL2
#if __has_include(<SDL2/SDL.h>)
#include <SDL2/SDL.h>
#elif __has_include(<SDL.h>)
#include <SDL.h>
#else
#error "SDL2 headers not found"
#endif
#endif

namespace comic2 {
namespace {

std::size_t checked_offset(const EgaPlanarSurface &surface, std::size_t x_byte,
                           std::size_t y_row) {
  if (x_byte >= surface.row_stride_bytes()) {
    throw std::out_of_range("x_byte out of range");
  }
  if (y_row >= surface.height_rows()) {
    throw std::out_of_range("y_row out of range");
  }
  return y_row * surface.row_stride_bytes() + x_byte;
}

std::uint8_t read_surface_color(const EgaPlanarSurface &surface, std::int32_t x,
                                std::int32_t y) {
  if (x < 0 || y < 0 || x >= surface.width_pixels() ||
      y >= surface.height_rows()) {
    return 0;
  }

  const auto x_byte = static_cast<std::size_t>(x / 8);
  const auto bit = static_cast<std::uint8_t>(7 - (x % 8));
  const auto y_row = static_cast<std::size_t>(y);
  std::uint8_t color_index = 0;

  for (std::size_t plane = 0; plane < EgaPlanarSurface::kPlaneCount; ++plane) {
    const auto value = surface.get_plane_byte(plane, x_byte, y_row);
    if ((value & static_cast<std::uint8_t>(1U << bit)) != 0U) {
      color_index |= static_cast<std::uint8_t>(1U << plane);
    }
  }

  return color_index;
}

void set_surface_pixel(EgaPlanarSurface &surface, std::int32_t x,
                       std::int32_t y, std::uint8_t color_index) {
  if (x < 0 || y < 0 || x >= surface.width_pixels() ||
      y >= surface.height_rows()) {
    return;
  }

  const auto x_byte = static_cast<std::size_t>(x / 8);
  const auto bit = static_cast<std::uint8_t>(7 - (x % 8));
  const auto mask = static_cast<std::uint8_t>(1U << bit);
  const auto y_row = static_cast<std::size_t>(y);

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

void or_surface_pixel(EgaPlanarSurface &surface, std::int32_t x, std::int32_t y,
                      std::uint8_t color_index) {
  if (x < 0 || y < 0 || x >= surface.width_pixels() ||
      y >= surface.height_rows()) {
    return;
  }

  const auto x_byte = static_cast<std::size_t>(x / 8);
  const auto bit = static_cast<std::uint8_t>(7 - (x % 8));
  const auto mask = static_cast<std::uint8_t>(1U << bit);
  const auto y_row = static_cast<std::size_t>(y);

  for (std::size_t plane = 0; plane < EgaPlanarSurface::kPlaneCount; ++plane) {
    if (((color_index >> plane) & 0x1U) == 0U) {
      continue;
    }

    auto value = surface.get_plane_byte(plane, x_byte, y_row);
    value = static_cast<std::uint8_t>(value | mask);
    surface.set_plane_byte(plane, x_byte, y_row, value);
  }
}

std::uint8_t read_sprite_pixel_color(const Ega4PlaneImage &sprite,
                                     std::size_t x_pixels, std::size_t y_rows) {
  if (x_pixels >= static_cast<std::size_t>(sprite.width_bytes) * 8U ||
      y_rows >= static_cast<std::size_t>(sprite.height_rows)) {
    return 0;
  }

  const auto x_byte = x_pixels / 8U;
  const auto bit = static_cast<std::uint8_t>(7U - (x_pixels % 8U));
  const auto row_off = y_rows * static_cast<std::size_t>(sprite.width_bytes);
  std::uint8_t color_index = 0;

  for (std::size_t plane = 0; plane < sprite.planes.size(); ++plane) {
    const auto value = sprite.planes[plane][row_off + x_byte];
    if ((value & static_cast<std::uint8_t>(1U << bit)) != 0U) {
      color_index |= static_cast<std::uint8_t>(1U << plane);
    }
  }

  return color_index;
}

void fill_surface_rect(EgaPlanarSurface &surface, std::int32_t x0,
                       std::int32_t y0, std::int32_t width, std::int32_t height,
                       std::uint8_t color_index) {
  if (width <= 0 || height <= 0) {
    return;
  }

  for (std::int32_t y = y0; y < y0 + height; ++y) {
    for (std::int32_t x = x0; x < x0 + width; ++x) {
      set_surface_pixel(surface, x, y, color_index);
    }
  }
}

} // namespace

EgaPlanarSurface::EgaPlanarSurface(std::uint16_t width_pixels,
                                   std::uint16_t height_rows)
    : width_pixels_(width_pixels), height_rows_(height_rows),
      row_stride_bytes_(static_cast<std::size_t>(width_pixels + 7U) / 8U) {
  if (width_pixels == 0 || height_rows == 0) {
    throw std::invalid_argument("surface dimensions must be non-zero");
  }

  const auto plane_bytes = plane_size_bytes();
  for (auto &p : planes_) {
    p.assign(plane_bytes, 0);
  }
}

void EgaPlanarSurface::clear(std::uint8_t value) {
  for (auto &p : planes_) {
    std::fill(p.begin(), p.end(), value);
  }
}

// cppcheck-suppress functionConst
std::span<std::uint8_t> EgaPlanarSurface::plane(std::size_t index) {
  if (index >= kPlaneCount) {
    throw std::out_of_range("plane index out of range");
  }
  return planes_[index];
}

std::span<const std::uint8_t> EgaPlanarSurface::plane(std::size_t index) const {
  if (index >= kPlaneCount) {
    throw std::out_of_range("plane index out of range");
  }
  return planes_[index];
}

std::uint8_t EgaPlanarSurface::get_plane_byte(std::size_t plane_index,
                                              std::size_t x_byte,
                                              std::size_t y_row) const {
  if (plane_index >= kPlaneCount) {
    throw std::out_of_range("plane index out of range");
  }
  const auto off = checked_offset(*this, x_byte, y_row);
  return planes_[plane_index][off];
}

void EgaPlanarSurface::set_plane_byte(std::size_t plane_index,
                                      std::size_t x_byte, std::size_t y_row,
                                      std::uint8_t value) {
  if (plane_index >= kPlaneCount) {
    throw std::out_of_range("plane index out of range");
  }
  const auto off = checked_offset(*this, x_byte, y_row);
  planes_[plane_index][off] = value;
}

void EgaPageFlipper::init_double_buffering() {
  // Initialize double buffering with page 0x2000 as the active page
  // This matches the original assembly code behavior
  active_page_ = 0x2000;
}

void EgaPageFlipper::present_and_flip_page() {
  // Flip between pages by XORing with 0x2000
  // This matches the original assembly code behavior:
  // xor cs:word_773F, 2000h
  active_page_ ^= 0x2000;
}

namespace {

bool is_valid_masked_sprite_record(const MaskedSpriteRecord &record) {
  if (record.width_pixels == 0U || record.height_rows == 0U) {
    return false;
  }

  const auto expected_mask_bytes = record.mask_byte_count();
  if (expected_mask_bytes == 0U || record.mask_bytes.size() != expected_mask_bytes ||
      record.mask_bytes.empty()) {
    return false;
  }

  for (const auto &plane : record.planes) {
    if (plane.size() != expected_mask_bytes) {
      return false;
    }
  }

  return true;
}

Ega4PlaneImage make_masked_sprite_image(const MaskedSpriteRecord &record) {
  Ega4PlaneImage image{};
  image.width_bytes =
      static_cast<std::uint16_t>((record.width_pixels + 7U) / 8U);
  image.height_rows = record.height_rows;
  image.row_span_bytes = static_cast<std::uint16_t>(
      image.width_bytes * static_cast<std::uint16_t>(record.height_rows));

  for (std::size_t plane = 0; plane < record.planes.size(); ++plane) {
    const auto &source = record.planes[plane];
    auto &target = image.planes[plane];
    target.assign(source.begin(), source.end());
  }

  return image;
}

std::size_t map_runtime_entity_slot_to_sprite_index(
    const RuntimeEntitySlot32 &slot) {
  const std::uint16_t mixed =
      static_cast<std::uint16_t>(slot.type_flags ^ slot.behavior_state ^
                                 slot.mapped_object_ptr);
  return static_cast<std::size_t>(mixed & 0x0FU);
}

std::size_t map_projectile_to_sprite_index(const ProjectileState &projectile) {
  const auto frame = static_cast<std::int16_t>(projectile.anim_frame);
  const auto abs_frame =
      static_cast<std::uint16_t>(frame < 0 ? -frame : frame);
  return static_cast<std::size_t>(abs_frame & 0x0FU);
}

bool try_draw_level_sprite_record(EgaPlanarSurface &frame,
                                  const RuntimeState &state,
                                  std::size_t sprite_index,
                                  std::int32_t x_pixels,
                                  std::int32_t y_rows, std::int32_t width,
                                  std::int32_t height) {
  if (sprite_index >= state.level_sprites.size()) {
    return false;
  }

  const auto &record = state.level_sprites[sprite_index];
  if (!is_valid_masked_sprite_record(record)) {
    return false;
  }

  if (!is_sprite_in_viewport(x_pixels, y_rows, width, height)) {
    return true;
  }

  const auto sprite = make_masked_sprite_image(record);
  const std::int32_t max_x = std::max<std::int32_t>(
      0, static_cast<std::int32_t>(frame.width_pixels()) - width);
  const std::int32_t max_y = std::max<std::int32_t>(
      0, static_cast<std::int32_t>(frame.height_rows()) - height);
  const std::int32_t clamped_x =
      std::max<std::int32_t>(0, std::min(x_pixels, max_x));
  const std::int32_t clamped_y =
      std::max<std::int32_t>(0, std::min(y_rows, max_y));

  gfx_rle_blit_masked_or_4plane(frame, static_cast<std::size_t>(clamped_x),
                                static_cast<std::size_t>(clamped_y), sprite);
  return true;
}

std::uint8_t resolve_entity_color(const RuntimeEntitySlot32 &slot) {
  const auto behavior = slot.behavior_state & 0x000F;
  if (behavior == kBehaviorGem) {
    return 0x03U;
  }
  if (behavior == kBehaviorPowerup) {
    return 0x0CU;
  }

  const std::uint16_t seed = static_cast<std::uint16_t>(
      slot.behavior_state + slot.mapped_object_ptr + slot.type_flags);
  return static_cast<std::uint8_t>(((seed & 0x0F) | 0x01U) & 0x0FU);
}

void set_sprite_pixel(Ega4PlaneImage &sprite, std::size_t x_pixels,
                      std::size_t y_rows, std::uint8_t color) {
  if (x_pixels >= 16U || y_rows >= 16U) {
    return;
  }

  const std::size_t x_byte = x_pixels / 8U;
  const std::uint8_t bit = static_cast<std::uint8_t>(7U - (x_pixels % 8U));
  const std::size_t row_off =
      y_rows * static_cast<std::size_t>(sprite.width_bytes);

  for (std::size_t plane = 0; plane < sprite.planes.size(); ++plane) {
    if ((color >> plane) & 0x1U) {
      sprite.planes[plane][row_off + x_byte] |=
          static_cast<std::uint8_t>(1U << bit);
    }
  }
}

Ega4PlaneImage make_entity_placeholder_sprite(const RuntimeEntitySlot32 &slot) {
  Ega4PlaneImage sprite{};
  constexpr std::uint16_t kSpriteWidthBytes = 2;
  constexpr std::uint16_t kSpriteHeightRows = 16;
  sprite.width_bytes = kSpriteWidthBytes;
  sprite.height_rows = kSpriteHeightRows;
  sprite.row_span_bytes =
      static_cast<std::uint16_t>(kSpriteWidthBytes * kSpriteHeightRows);

  const std::uint8_t color = resolve_entity_color(slot);
  for (auto &plane_bytes : sprite.planes) {
    plane_bytes.assign(static_cast<std::size_t>(sprite.width_bytes) *
                           sprite.height_rows,
                       0x00);
  }

  const auto behavior = slot.behavior_state & 0x000F;
  if (behavior == kBehaviorGem) {
    set_sprite_pixel(sprite, 0, 0, color);
    set_sprite_pixel(sprite, 8, 4, color);
    set_sprite_pixel(sprite, 9, 4, color);
    set_sprite_pixel(sprite, 8, 5, color);
    set_sprite_pixel(sprite, 9, 5, color);
    set_sprite_pixel(sprite, 8, 6, color);
    return sprite;
  }

  if (behavior == kBehaviorPowerup) {
    set_sprite_pixel(sprite, 0, 0, color);
    set_sprite_pixel(sprite, 7, 4, color);
    set_sprite_pixel(sprite, 8, 4, color);
    set_sprite_pixel(sprite, 7, 5, color);
    set_sprite_pixel(sprite, 8, 5, color);
    set_sprite_pixel(sprite, 7, 6, color);
    set_sprite_pixel(sprite, 8, 6, color);
    return sprite;
  }

  for (std::size_t plane = 0; plane < sprite.planes.size(); ++plane) {
    if ((color >> plane) & 0x1U) {
      auto &plane_bytes = sprite.planes[plane];
      for (std::size_t row = 0; row < sprite.height_rows; ++row) {
        const std::size_t row_off = row * sprite.width_bytes;
        plane_bytes[row_off] = 0xFF;
        plane_bytes[row_off + 1] = 0xFF;
      }
    }
  }

  return sprite;
}

std::uint8_t resolve_projectile_color(const ProjectileState &projectile) {
  const std::uint16_t seed = static_cast<std::uint16_t>(
      projectile.anim_frame * 5U + (projectile.active ? 0x01U : 0x00U) +
      (projectile.x_vel != 0 ? 0x02U : 0x00U));
  return static_cast<std::uint8_t>(((seed & 0x0FU) | 0x04U) & 0x0FU);
}

Ega4PlaneImage
make_projectile_placeholder_sprite(const ProjectileState &projectile) {
  Ega4PlaneImage sprite{};
  constexpr std::uint16_t kSpriteWidthBytes = 2;
  constexpr std::uint16_t kSpriteHeightRows = 8;
  sprite.width_bytes = kSpriteWidthBytes;
  sprite.height_rows = kSpriteHeightRows;
  sprite.row_span_bytes =
      static_cast<std::uint16_t>(kSpriteWidthBytes * kSpriteHeightRows);

  const std::uint8_t color = resolve_projectile_color(projectile);
  for (std::size_t plane = 0; plane < sprite.planes.size(); ++plane) {
    auto &plane_bytes = sprite.planes[plane];
    plane_bytes.assign(static_cast<std::size_t>(sprite.width_bytes) *
                           sprite.height_rows,
                       0x00);
    if ((color >> plane) & 0x1U) {
      for (std::size_t row = 0; row < sprite.height_rows; ++row) {
        const std::size_t row_off = row * sprite.width_bytes;
        plane_bytes[row_off] = 0xFF;
        plane_bytes[row_off + 1] = 0xFF;
      }
    }
  }

  return sprite;
}

} // namespace

void gfx_rle_blit_opaque_4plane(EgaPlanarSurface &dest, std::size_t x_pixels,
                                std::size_t y_rows,
                                const Ega4PlaneImage &image_data) {
  // Validate destination coordinates
  if (x_pixels >= dest.width_pixels() || y_rows >= dest.height_rows()) {
    throw std::out_of_range("blit coordinates out of range");
  }

  // Validate image dimensions
  if (image_data.width_bytes == 0 || image_data.height_rows == 0) {
    throw std::invalid_argument(
        "image width_bytes and height_rows must be non-zero for blitting");
  }

  // Calculate destination row stride in bytes
  const auto dest_row_stride = dest.row_stride_bytes();
  const auto dest_byte_offset = x_pixels / 8;

  // For each plane, copy row-by-row using explicit image dimensions
  for (std::size_t plane_index = 0; plane_index < 4; ++plane_index) {
    const auto &source_plane = image_data.planes[plane_index];
    auto dest_plane_span = dest.plane(plane_index);
    auto dest_plane = dest_plane_span.data();

    for (std::size_t row = 0; row < image_data.height_rows; ++row) {
      const auto source_offset = row * image_data.width_bytes;
      const auto dest_offset =
          (y_rows + row) * dest_row_stride + dest_byte_offset;

      std::copy(
          source_plane.begin() + static_cast<std::ptrdiff_t>(source_offset),
          source_plane.begin() + static_cast<std::ptrdiff_t>(
                                     source_offset + image_data.width_bytes),
          dest_plane + static_cast<std::ptrdiff_t>(dest_offset));
    }
  }
}

bool is_sprite_in_viewport(std::int32_t x, std::int32_t y, std::int32_t width,
                           std::int32_t height, std::int32_t viewport_x,
                           std::int32_t viewport_y, std::int32_t viewport_width,
                           std::int32_t viewport_height) {
  const std::int32_t viewport_right = viewport_x + viewport_width;
  const std::int32_t viewport_bottom = viewport_y + viewport_height;

  return (x + width > viewport_x) && (x < viewport_right) &&
         (y + height > viewport_y) && (y < viewport_bottom);
}

void gfx_rle_blit_masked_or_4plane(EgaPlanarSurface &dest, std::size_t x_pixels,
                                   std::size_t y_rows,
                                   const Ega4PlaneImage &image_data) {
  // Validate destination coordinates
  if (x_pixels >= dest.width_pixels() || y_rows >= dest.height_rows()) {
    throw std::out_of_range("blit coordinates out of range");
  }

  // Validate image dimensions
  if (image_data.width_bytes == 0 || image_data.height_rows == 0) {
    throw std::invalid_argument(
        "image width_bytes and height_rows must be non-zero for blitting");
  }

  if ((x_pixels % 8U) == 0U) {
    const auto dest_row_stride = dest.row_stride_bytes();
    const auto dest_byte_offset = x_pixels / 8;

    for (std::size_t plane_index = 0; plane_index < 4; ++plane_index) {
      const auto &source_plane = image_data.planes[plane_index];
      auto dest_plane_span = dest.plane(plane_index);
      auto dest_plane = dest_plane_span.data();

      for (std::size_t row = 0; row < image_data.height_rows; ++row) {
        const auto source_offset = row * image_data.width_bytes;
        const auto dest_offset =
            (y_rows + row) * dest_row_stride + dest_byte_offset;

        for (std::size_t i = 0; i < image_data.width_bytes; ++i) {
          dest_plane[dest_offset + i] |= source_plane[source_offset + i];
        }
      }
    }
    return;
  }

  const auto sprite_width_pixels =
      static_cast<std::size_t>(image_data.width_bytes) * 8U;
  for (std::size_t row = 0; row < image_data.height_rows; ++row) {
    for (std::size_t src_x = 0; src_x < sprite_width_pixels; ++src_x) {
      const auto color_index = read_sprite_pixel_color(image_data, src_x, row);
      if (color_index == 0U) {
        continue;
      }

      const auto dest_x = static_cast<std::int32_t>(x_pixels) +
                          static_cast<std::int32_t>(src_x);
      const auto dest_y =
          static_cast<std::int32_t>(y_rows) + static_cast<std::int32_t>(row);
      or_surface_pixel(dest, dest_x, dest_y, color_index);
    }
  }
}

void draw_runtime_entity_sprites(EgaPlanarSurface &frame,
                                 const RuntimeState &state) {
  for (const auto &slot : state.runtime_slots) {
    if (!is_runtime_slot_active(slot)) {
      continue;
    }

    const std::int32_t px0 = slot.x - state.camera_x;
    const std::int32_t py0 = slot.y - state.camera_y;
    if (px0 < -16 || py0 < -16) {
      continue;
    }

    const auto sprite_index = map_runtime_entity_slot_to_sprite_index(slot);
    if (try_draw_level_sprite_record(frame, state, sprite_index, px0, py0, 16,
                                     16)) {
      continue;
    }

    if (!is_sprite_in_viewport(px0, py0, 16, 16)) {
      continue;
    }

    const std::int32_t max_x = std::max<std::int32_t>(
        0, static_cast<std::int32_t>(frame.width_pixels()) - 16);
    const std::int32_t max_y = std::max<std::int32_t>(
        0, static_cast<std::int32_t>(frame.height_rows()) - 16);
    const std::int32_t clamped_x =
        std::max<std::int32_t>(0, std::min(px0, max_x));
    const std::int32_t clamped_y =
        std::max<std::int32_t>(0, std::min(py0, max_y));
    const auto sprite = make_entity_placeholder_sprite(slot);
    gfx_rle_blit_masked_or_4plane(frame, static_cast<std::size_t>(clamped_x),
                                  static_cast<std::size_t>(clamped_y), sprite);
  }
}

void draw_runtime_projectile_sprites(EgaPlanarSurface &frame,
                                     const RuntimeState &state) {
  for (const auto &projectile : state.projectiles) {
    if (!projectile.active) {
      continue;
    }

    const std::int32_t px0 = projectile.x - state.camera_x;
    const std::int32_t py0 = projectile.y - state.camera_y;
    if (px0 < -8 || py0 < -8) {
      continue;
    }

    const auto sprite_index = map_projectile_to_sprite_index(projectile);
    if (try_draw_level_sprite_record(frame, state, sprite_index, px0, py0, 8,
                                     8)) {
      continue;
    }

    if (!is_sprite_in_viewport(px0, py0, 8, 8)) {
      continue;
    }

    const std::int32_t max_x = std::max<std::int32_t>(
        0, static_cast<std::int32_t>(frame.width_pixels()) - 8);
    const std::int32_t max_y = std::max<std::int32_t>(
        0, static_cast<std::int32_t>(frame.height_rows()) - 8);
    const std::int32_t clamped_x =
        std::max<std::int32_t>(0, std::min(px0, max_x));
    const std::int32_t clamped_y =
        std::max<std::int32_t>(0, std::min(py0, max_y));
    const auto sprite = make_projectile_placeholder_sprite(projectile);
    gfx_rle_blit_masked_or_4plane(frame, static_cast<std::size_t>(clamped_x),
                                  static_cast<std::size_t>(clamped_y), sprite);
  }
}

void apply_transition_palette_tint(EgaPlanarSurface &surface,
                                   const RoomTransitionState &transition) {
  if (!transition.active) {
    return;
  }

  const std::uint8_t tint = transition.palette_tint & 0x0F;
  const std::uint8_t shift = transition.palette_shift & 0x03;
  const std::uint8_t frame_bias =
      static_cast<std::uint8_t>((transition.frame_index % 8U) + shift);

  for (std::int32_t y = 0; y < surface.height_rows(); ++y) {
    for (std::int32_t x = 0; x < surface.width_pixels(); ++x) {
      const auto color = read_surface_color(surface, x, y);
      const auto base =
          static_cast<std::uint8_t>((color + tint + frame_bias) & 0x0F);
      set_surface_pixel(surface, x, y, base);
    }
  }
}

void load_ega_palette_table(
    RuntimeState &state,
    const std::array<std::array<std::uint8_t, 4>, 16> &palette_table) {
  state.palette.loaded = true;
  state.palette.entries = palette_table;
}

void room_transition_palette_wave(EgaPlanarSurface &surface,
                                  const RoomTransitionState &transition) {
  for (std::int32_t y = 0; y < surface.height_rows(); ++y) {
    const auto band =
        static_cast<std::int32_t>((y + transition.frame_index * 4) % 16);
    const auto color = static_cast<std::uint8_t>(band & 0x0F);
    if (band < 8) {
      for (std::int32_t x = 0; x < surface.width_pixels(); x += 8) {
        set_surface_pixel(surface, x, y, color);
      }
    }
  }
}

void room_transition_reveal_sequence_a(EgaPlanarSurface &surface,
                                       const RoomTransitionState &transition) {
  const std::int32_t size =
      16 + static_cast<std::int32_t>(transition.frame_index * 12);
  const std::int32_t x0 = (surface.width_pixels() - size) / 2;
  const std::int32_t y0 = (surface.height_rows() - size) / 2;
  room_transition_draw_reveal_quad(surface, x0, y0, size, size,
                                   static_cast<std::uint8_t>(0x0F));
}

// cppcheck-suppress unusedFunction ; reserved for Phase 10
void room_transition_reveal_sequence_b(EgaPlanarSurface &surface,
                                       const RoomTransitionState &transition) {
  const std::int32_t width =
      24 + static_cast<std::int32_t>(transition.frame_index * 8);
  const std::int32_t height =
      24 + static_cast<std::int32_t>(transition.frame_index * 6);
  const std::int32_t x0 =
      32 + static_cast<std::int32_t>(transition.frame_index % 4) * 8;
  const std::int32_t y0 =
      24 + static_cast<std::int32_t>(transition.frame_index % 3) * 10;
  room_transition_draw_reveal_quad(surface, x0, y0, width, height,
                                   static_cast<std::uint8_t>(0x07));
}

void room_transition_draw_reveal_quad(EgaPlanarSurface &surface,
                                      std::int32_t x0, std::int32_t y0,
                                      std::int32_t width, std::int32_t height,
                                      std::uint8_t color) {
  if (width <= 0 || height <= 0) {
    return;
  }

  fill_surface_rect(surface, x0, y0, width, 2, color);
  fill_surface_rect(surface, x0, y0 + height - 2, width, 2, color);
  fill_surface_rect(surface, x0, y0, 2, height, color);
  fill_surface_rect(surface, x0 + width - 2, y0, 2, height, color);
}

void room_transition_player_entry_sequence(RuntimeState &state) {
  auto &transition = state.transition_state;
  transition.player_entry_offset =
      static_cast<std::int16_t>(transition.frame_index * 2);
  transition.player_frozen = true;
}

void room_transition_player_exit_sequence(RuntimeState &state) {
  auto &transition = state.transition_state;
  transition.player_exit_offset =
      static_cast<std::int16_t>(transition.frame_index * 2);
  transition.player_frozen = true;
}

void camera_update_x_follow_comic_clamped(RuntimeState &state,
                                          std::int32_t viewport_width,
                                          std::int32_t room_width) {
  if (viewport_width <= 0) {
    state.camera_x = 0;
    return;
  }

  const std::int32_t room_pixels = std::max<std::int32_t>(room_width, 0);
  const std::int32_t max_camera_x =
      std::max<std::int32_t>(0, room_pixels - viewport_width);
  const std::int32_t target_x = state.player.x - viewport_width / 2;
  state.camera_x = std::clamp(target_x, 0, max_camera_x);
}

void camera_update_y_follow_comic_clamped(RuntimeState &state,
                                          std::int32_t viewport_height,
                                          std::int32_t room_height) {
  if (viewport_height <= 0) {
    state.camera_y = 0;
    return;
  }

  const std::int32_t room_pixels = std::max<std::int32_t>(room_height, 0);
  const std::int32_t max_camera_y =
      std::max<std::int32_t>(0, room_pixels - viewport_height);
  const std::int32_t target_y = state.player.y - viewport_height / 2;
  state.camera_y = std::clamp(target_y, 0, max_camera_y);
}

void MemoryFramePresenter::present(const EgaPlanarSurface &frame) {
  last_frame_ = frame;
  has_frame_ = true;
}

const EgaPlanarSurface &MemoryFramePresenter::last_frame() const {
  if (!has_frame_) {
    throw std::logic_error("no frame has been presented");
  }
  return last_frame_;
}

#ifdef COMIC2_USE_SDL2
// ============================================================================
// SDL2 Frame Presenter Implementation
// ============================================================================

struct Sdl2FramePresenter::Impl {
  SDL_Window *window{};
  SDL_Renderer *renderer{};
  SDL_Texture *texture{};
  SDL_PixelFormat *pixel_format{};
  std::array<Uint32, 16> palette{};
  std::uint16_t window_width{};
  std::uint16_t window_height{};

  Impl(std::uint16_t ww, std::uint16_t hh)
      : window_width(ww), window_height(hh) {}

  ~Impl() {
    if (texture) {
      SDL_DestroyTexture(texture);
    }
    if (renderer) {
      SDL_DestroyRenderer(renderer);
    }
    if (window) {
      SDL_DestroyWindow(window);
    }
    if (pixel_format) {
      SDL_FreeFormat(pixel_format);
    }
    SDL_Quit();
  }
};

Sdl2FramePresenter::Sdl2FramePresenter(std::uint16_t window_width,
                                       std::uint16_t window_height) {
  // Initialize SDL
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    throw std::runtime_error("SDL_Init failed: " + std::string(SDL_GetError()));
  }

  impl_ = new Impl(window_width, window_height);

  // Create window
  impl_->window =
      SDL_CreateWindow("Captain Comic II: Fractured Reality (Reimplementation)",
                       SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                       window_width, window_height, SDL_WINDOW_SHOWN);

  if (!impl_->window) {
    delete impl_;
    impl_ = nullptr;
    throw std::runtime_error("SDL_CreateWindow failed: " +
                             std::string(SDL_GetError()));
  }

  // Create renderer - try accelerated first, fall back to software for
  // headless/CI
  impl_->renderer = SDL_CreateRenderer(
      impl_->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

  if (!impl_->renderer) {
    // Acceleration not available (headless/dummy driver) - try software
    // renderer
    impl_->renderer =
        SDL_CreateRenderer(impl_->window, -1, SDL_RENDERER_SOFTWARE);
  }

  if (!impl_->renderer) {
    delete impl_;
    impl_ = nullptr;
    throw std::runtime_error("SDL_CreateRenderer failed: " +
                             std::string(SDL_GetError()));
  }

  // Create an explicit 32-bit RGBA format for display
  impl_->pixel_format = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888);
  if (!impl_->pixel_format) {
    delete impl_;
    impl_ = nullptr;
    throw std::runtime_error("SDL_AllocFormat failed: " +
                             std::string(SDL_GetError()));
  }

  constexpr std::uint8_t kEgaPalette[16][4] = {
      {0, 0, 0, 255},       // Black
      {0, 0, 200, 255},     // Blue
      {0, 200, 0, 255},     // Green
      {0, 200, 200, 255},   // Cyan
      {200, 0, 0, 255},     // Red
      {200, 0, 200, 255},   // Magenta
      {200, 100, 0, 255},   // Brown
      {200, 200, 200, 255}, // Light gray
      {100, 100, 100, 255}, // Dark gray
      {50, 50, 255, 255},   // Light blue
      {50, 255, 50, 255},   // Light green
      {50, 255, 255, 255},  // Light cyan
      {255, 50, 50, 255},   // Light red
      {255, 50, 255, 255},  // Light magenta
      {255, 255, 50, 255},  // Yellow
      {255, 255, 255, 255}, // White
  };
  for (std::size_t i = 0; i < impl_->palette.size(); ++i) {
    impl_->palette[i] =
        SDL_MapRGBA(impl_->pixel_format, kEgaPalette[i][0], kEgaPalette[i][1],
                    kEgaPalette[i][2], kEgaPalette[i][3]);
  }

  impl_->texture = SDL_CreateTexture(impl_->renderer, SDL_PIXELFORMAT_RGBA8888,
                                     SDL_TEXTUREACCESS_STREAMING,
                                     320, // EGA resolution width
                                     200  // EGA resolution height
  );

  if (!impl_->texture) {
    delete impl_;
    impl_ = nullptr;
    throw std::runtime_error("SDL_CreateTexture failed: " +
                             std::string(SDL_GetError()));
  }
}

Sdl2FramePresenter::~Sdl2FramePresenter() { delete impl_; }

void Sdl2FramePresenter::present(const EgaPlanarSurface &frame) {
  if (!impl_ || !impl_->renderer || !impl_->texture) {
    return;
  }

  // Convert 4-plane EGA data to packed 32-bit pixels
  // EGA uses 4 planes, each pixel is represented by 4 bits (one bit per plane)
  if (frame.width_pixels() != 320 || frame.height_rows() != 200) {
    throw std::invalid_argument(
        "Sdl2FramePresenter expects a 320x200 EGA frame");
  }

  void *pixels = nullptr;
  int pitch = 0;
  if (SDL_LockTexture(impl_->texture, nullptr, &pixels, &pitch) != 0) {
    return;
  }

  if (pitch % sizeof(Uint32) != 0) {
    SDL_UnlockTexture(impl_->texture);
    throw std::runtime_error("SDL texture pitch is not a multiple of Uint32");
  }

  auto *dst = static_cast<Uint32 *>(pixels);
  const auto row_pixels = static_cast<std::size_t>(pitch / sizeof(Uint32));
  const auto row_stride = frame.row_stride_bytes();
  const auto *plane0 = frame.plane(0).data();
  const auto *plane1 = frame.plane(1).data();
  const auto *plane2 = frame.plane(2).data();
  const auto *plane3 = frame.plane(3).data();

  for (std::size_t y = 0; y < 200; ++y) {
    for (std::size_t x = 0; x < 320; ++x) {
      const auto byte_x = x / 8;
      const auto bit_x = 7 - (x % 8);
      const auto off = y * row_stride + byte_x;

      std::uint8_t color_index = 0;
      if (plane0[off] & (1u << bit_x)) {
        color_index |= 0x1;
      }
      if (plane1[off] & (1u << bit_x)) {
        color_index |= 0x2;
      }
      if (plane2[off] & (1u << bit_x)) {
        color_index |= 0x4;
      }
      if (plane3[off] & (1u << bit_x)) {
        color_index |= 0x8;
      }

      dst[y * row_pixels + x] = impl_->palette[color_index];
    }
  }

  // Unlock texture and update
  SDL_UnlockTexture(impl_->texture);

  // Copy texture to renderer
  SDL_RenderClear(impl_->renderer);
  SDL_RenderCopy(impl_->renderer, impl_->texture, nullptr, nullptr);
  SDL_RenderPresent(impl_->renderer);
}
#endif // COMIC2_USE_SDL2

} // namespace comic2
