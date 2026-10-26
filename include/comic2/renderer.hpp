#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "comic2/game_state.hpp"
#include "comic2/resource_formats.hpp"

namespace comic2 {

class EgaPlanarSurface {
public:
  static constexpr std::size_t kPlaneCount = 4;
  static constexpr std::uint16_t kDefaultWidthPixels = 320;
  static constexpr std::uint16_t kDefaultHeightRows = 200;
  static constexpr std::uint16_t kPage0 = 0x0000;
  static constexpr std::uint16_t kPage1 = 0x2000;

  explicit EgaPlanarSurface(std::uint16_t width_pixels = kDefaultWidthPixels,
                            std::uint16_t height_rows = kDefaultHeightRows);

  std::uint16_t width_pixels() const noexcept { return width_pixels_; }
  std::uint16_t height_rows() const noexcept { return height_rows_; }
  std::size_t row_stride_bytes() const noexcept { return row_stride_bytes_; }
  std::size_t plane_size_bytes() const noexcept {
    return row_stride_bytes_ * static_cast<std::size_t>(height_rows_);
  }

  void clear(std::uint8_t value = 0);

  std::span<std::uint8_t> plane(std::size_t index);
  std::span<const std::uint8_t> plane(std::size_t index) const;

  std::uint8_t get_plane_byte(std::size_t plane_index, std::size_t x_byte,
                              std::size_t y_row) const;
  void set_plane_byte(std::size_t plane_index, std::size_t x_byte,
                      std::size_t y_row, std::uint8_t value);

private:
  std::uint16_t width_pixels_;
  std::uint16_t height_rows_;
  std::size_t row_stride_bytes_;
  std::array<std::vector<std::uint8_t>, kPlaneCount> planes_;
};

class IFramePresenter {
public:
  virtual ~IFramePresenter() = default;
  virtual void present(const EgaPlanarSurface &frame) = 0;
};

class MemoryFramePresenter final : public IFramePresenter {
public:
  void present(const EgaPlanarSurface &frame) override;

  bool has_frame() const noexcept { return has_frame_; }
  const EgaPlanarSurface &last_frame() const;

private:
  bool has_frame_ = false;
  EgaPlanarSurface last_frame_{};
};

#ifdef COMIC2_USE_SDL2
class Sdl2FramePresenter final : public IFramePresenter {
public:
  explicit Sdl2FramePresenter(std::uint16_t window_width = 640,
                              std::uint16_t window_height = 480);
  ~Sdl2FramePresenter() override;

  void present(const EgaPlanarSurface &frame) override;

  // Disable copying
  Sdl2FramePresenter(const Sdl2FramePresenter &) = delete;
  Sdl2FramePresenter &operator=(const Sdl2FramePresenter &) = delete;

private:
  struct Impl;
  Impl *impl_{};
};
#endif // COMIC2_USE_SDL2

// Page flipping abstraction for EGA double buffering
class EgaPageFlipper {
public:
  // Initialize double buffering with page 0x2000 as the active page
  void init_double_buffering();

  // Present current draw page and flip to the other page
  void present_and_flip_page();

  // Get the current active page address
  std::uint16_t active_page() const noexcept { return active_page_; }

  // Get the other page address
  std::uint16_t other_page() const noexcept { return active_page_ ^ 0x2000; }

private:
  std::uint16_t active_page_ = 0;
};

// Blit functions for EGA 4-plane sprites
//
// These functions implement the core blitting operations that mirror the
// original Captain Comic II assembly implementations:
// - gfx_rle_blit_opaque_4plane: Copies RLE-encoded 4-plane sprite data
// - gfx_rle_blit_masked_or_4plane: ORs RLE-encoded 4-plane sprite data (masked
// blitting)
//
// Parameters:
//   dest: The destination surface to blit to
//   x_pixels: X coordinate in pixels where to start blitting
//   y_rows: Y coordinate in rows where to start blitting
//   image_data: The RLE-encoded 4-plane sprite data to blit
void gfx_rle_blit_opaque_4plane(EgaPlanarSurface &dest, std::size_t x_pixels,
                                std::size_t y_rows,
                                const Ega4PlaneImage &image_data);
void gfx_rle_blit_masked_or_4plane(EgaPlanarSurface &dest, std::size_t x_pixels,
                                   std::size_t y_rows,
                                   const Ega4PlaneImage &image_data);

// Authentic in-game playfield viewport within the 320x200 bezel frame.
// The FRPAK.001 bezel has an 8-pixel border; the playfield sits within it.
constexpr std::int32_t kPlayfieldX = 8;
constexpr std::int32_t kPlayfieldY = 8;
constexpr std::int32_t kPlayfieldWidth = 240;
constexpr std::int32_t kPlayfieldHeight = 176;

bool is_sprite_in_viewport(
    std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height,
    std::int32_t viewport_x = 0, std::int32_t viewport_y = 0,
    std::int32_t viewport_width =
        static_cast<std::int32_t>(EgaPlanarSurface::kDefaultWidthPixels),
    std::int32_t viewport_height =
        static_cast<std::int32_t>(EgaPlanarSurface::kDefaultHeightRows));

// Returns the 8-byte monochrome bitmap for a given ASCII character (8x8 font).
// Each byte is one row; bit 7 is the leftmost pixel. Falls back to blank.
std::array<std::uint8_t, 8> get_font_glyph_8x8(char c);

// Blits an 8x8 character glyph onto the surface at pixel coords (x, y).
void blit_font_glyph(EgaPlanarSurface &surface, std::int32_t x, std::int32_t y,
                     char c, std::uint8_t color_index);

// Blits a null-terminated string of 8x8 glyphs, advancing x by 8 per char.
void blit_string(EgaPlanarSurface &surface, std::int32_t x, std::int32_t y,
                 const char *text, std::uint8_t color_index);

void draw_runtime_entity_sprites(EgaPlanarSurface &frame,
                                 const RuntimeState &state,
                                 std::int32_t x_offset = 0,
                                 std::int32_t y_offset = 0);
void draw_runtime_projectile_sprites(EgaPlanarSurface &frame,
                                     const RuntimeState &state,
                                     std::int32_t x_offset = 0,
                                     std::int32_t y_offset = 0);

void apply_transition_palette_tint(EgaPlanarSurface &surface,
                                   const RoomTransitionState &transition);
void load_ega_palette_table(
    RuntimeState &state,
    const std::array<std::array<std::uint8_t, 4>, 16> &palette_table);
void room_transition_palette_wave(EgaPlanarSurface &surface,
                                  const RoomTransitionState &transition);
void room_transition_reveal_sequence_a(EgaPlanarSurface &surface,
                                       const RoomTransitionState &transition);
void room_transition_reveal_sequence_b(EgaPlanarSurface &surface,
                                       const RoomTransitionState &transition);
void room_transition_draw_reveal_quad(EgaPlanarSurface &surface,
                                      std::int32_t x0, std::int32_t y0,
                                      std::int32_t width, std::int32_t height,
                                      std::uint8_t color);
void room_transition_player_entry_sequence(RuntimeState &state);
void room_transition_player_exit_sequence(RuntimeState &state);
void camera_update_x_follow_comic_clamped(RuntimeState &state,
                                          std::int32_t viewport_width,
                                          std::int32_t room_width);
void camera_update_y_follow_comic_clamped(RuntimeState &state,
                                          std::int32_t viewport_height,
                                          std::int32_t room_height);

} // namespace comic2
