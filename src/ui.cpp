#include "comic2/ui.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <string>
#include <utility>

#include "comic2/audio.hpp"
#include "comic2/bootstrap.hpp"
#include "comic2/dispatcher.hpp"
#include "comic2/input_handler.hpp"
#include "comic2/persistence.hpp"
#include "comic2/room_loader.hpp"

namespace comic2 {
namespace {

// Authentic 16x16 4-plane HUD mode icons from comic2_data.asm:
// - Corkscrew / Mode 0 (0x128FA..0x12979, seg_5E:0x9E04)
// - Boots / Mode 1     (0x129A0..0x12A1F, seg_5E:0x9EAA)
// - Lantern / Mode 2   (0x12A46..0x12AC5, seg_5E:0x9F50)
// - Wand / Mode 3      (0x12AEC..0x12B6B, seg_5E:0x9FF6)
constexpr std::array<std::uint8_t, 128> kHudCorkscrewIconPlanes = {
    // Plane 0
    0x03,
    0xc0,
    0x07,
    0x00,
    0x06,
    0x00,
    0x06,
    0x00,
    0x03,
    0xc0,
    0x00,
    0x00,
    0x03,
    0xc0,
    0x03,
    0xc0,
    0x03,
    0x40,
    0x03,
    0x40,
    0x03,
    0xc0,
    0x07,
    0xe0,
    0x06,
    0x60,
    0x06,
    0x60,
    0x00,
    0x00,
    0x00,
    0x00,
    // Plane 1
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x01,
    0x80,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x80,
    0x00,
    0x80,
    0x0c,
    0x30,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    // Plane 2
    0x00,
    0x00,
    0x00,
    0xe0,
    0x01,
    0xe0,
    0x01,
    0xe0,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x04,
    0x20,
    0x0c,
    0xb0,
    0x0c,
    0xb0,
    0x0c,
    0x30,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x06,
    0x60,
    0x0e,
    0x70,
    // Plane 3
    0x03,
    0xc0,
    0x07,
    0x20,
    0x06,
    0x20,
    0x06,
    0x20,
    0x03,
    0xc0,
    0x01,
    0x80,
    0x00,
    0x00,
    0x06,
    0x60,
    0x0f,
    0xf0,
    0x0f,
    0xf0,
    0x0f,
    0xf0,
    0x07,
    0xe0,
    0x06,
    0x60,
    0x06,
    0x60,
    0x06,
    0x60,
    0x0e,
    0x70,
};

constexpr std::array<std::uint8_t, 128> kHudBootsIconPlanes = {
    // Plane 0
    0x00,
    0x00,
    0x07,
    0x00,
    0x01,
    0xc0,
    0x00,
    0x70,
    0x00,
    0x38,
    0x00,
    0x18,
    0x00,
    0x0c,
    0x00,
    0x04,
    0x00,
    0x06,
    0x00,
    0x02,
    0x00,
    0x02,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    // Plane 1
    0x00,
    0x00,
    0x07,
    0x00,
    0x01,
    0xc0,
    0x00,
    0x70,
    0x00,
    0x38,
    0x00,
    0x18,
    0x00,
    0x0c,
    0x00,
    0x04,
    0x00,
    0x06,
    0x00,
    0x02,
    0x00,
    0x02,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    // Plane 2
    0x00,
    0x00,
    0x07,
    0x00,
    0x01,
    0xc0,
    0x00,
    0x70,
    0x00,
    0x38,
    0x00,
    0x78,
    0x00,
    0xec,
    0x01,
    0xc4,
    0x03,
    0x86,
    0x07,
    0x02,
    0x0e,
    0x02,
    0x1c,
    0x00,
    0x38,
    0x00,
    0x70,
    0x00,
    0x60,
    0x00,
    0x00,
    0x00,
    // Plane 3
    0x00,
    0x00,
    0x01,
    0x00,
    0x00,
    0x40,
    0x00,
    0x10,
    0x00,
    0x08,
    0x00,
    0x68,
    0x00,
    0xc4,
    0x01,
    0x84,
    0x03,
    0x02,
    0x06,
    0x02,
    0x0c,
    0x02,
    0x18,
    0x00,
    0x30,
    0x00,
    0x60,
    0x00,
    0x40,
    0x00,
    0x00,
    0x00,
};

constexpr std::array<std::uint8_t, 128> kHudLanternIconPlanes = {
    // Plane 0
    0x00,
    0x00,
    0x00,
    0x00,
    0x12,
    0x08,
    0x0f,
    0x7c,
    0x5f,
    0x7e,
    0xdf,
    0x7f,
    0xdf,
    0x7f,
    0xdf,
    0x7f,
    0x5f,
    0x7e,
    0x1f,
    0x7c,
    0x1f,
    0x7c,
    0x0f,
    0x3c,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    // Plane 1
    0x00,
    0x00,
    0x00,
    0x00,
    0x12,
    0x08,
    0x0f,
    0x7c,
    0x1f,
    0x7c,
    0x1f,
    0x7c,
    0x1f,
    0x7c,
    0x1f,
    0x7c,
    0x1f,
    0x7c,
    0x1f,
    0x7c,
    0x1f,
    0x7c,
    0x0f,
    0x3c,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    // Plane 2
    0x07,
    0xe0,
    0x0f,
    0xf0,
    0x0c,
    0x30,
    0x1e,
    0x38,
    0x0f,
    0x7c,
    0x1f,
    0x7c,
    0x1f,
    0x7c,
    0x1f,
    0x7c,
    0x1f,
    0x7c,
    0x1f,
    0x7c,
    0x1f,
    0x7c,
    0x0f,
    0x3c,
    0x0c,
    0x30,
    0x0c,
    0x30,
    0x1e,
    0x78,
    0x07,
    0xe0,
    // Plane 3
    0x07,
    0xe0,
    0x00,
    0x10,
    0x04,
    0x10,
    0x16,
    0x58,
    0x33,
    0x8c,
    0x61,
    0x86,
    0xa1,
    0x85,
    0x61,
    0x86,
    0x21,
    0x84,
    0x21,
    0x84,
    0x21,
    0x84,
    0x21,
    0x84,
    0x31,
    0xc4,
    0x16,
    0x58,
    0x04,
    0x10,
    0x06,
    0x18,
};

constexpr std::array<std::uint8_t, 128> kHudWandIconPlanes = {
    // Plane 0
    0x00,
    0x00,
    0x00,
    0x0e,
    0x00,
    0x1e,
    0x00,
    0x3e,
    0x00,
    0x7c,
    0x00,
    0x78,
    0x00,
    0xf0,
    0x01,
    0xc0,
    0x03,
    0x80,
    0x0f,
    0x00,
    0x1e,
    0x00,
    0x3e,
    0x00,
    0x3c,
    0x00,
    0x78,
    0x00,
    0x60,
    0x00,
    0x00,
    0x00,
    // Plane 1
    0x00,
    0x00,
    0x00,
    0x0e,
    0x00,
    0x1e,
    0x00,
    0x3e,
    0x00,
    0x3c,
    0x00,
    0x18,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    // Plane 2
    0x00,
    0x00,
    0x00,
    0x0e,
    0x00,
    0x1e,
    0x00,
    0x3e,
    0x00,
    0x3c,
    0x00,
    0x18,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    // Plane 3
    0x00,
    0x00,
    0x00,
    0x0a,
    0x00,
    0x14,
    0x00,
    0x2a,
    0x00,
    0x14,
    0x00,
    0x08,
    0x00,
    0x20,
    0x00,
    0x40,
    0x00,
    0x80,
    0x01,
    0x00,
    0x04,
    0x00,
    0x0a,
    0x00,
    0x14,
    0x00,
    0x08,
    0x00,
    0x20,
    0x00,
    0x00,
    0x00,
};

Ega4PlaneImage
make_16x16_hud_icon_image(const std::array<std::uint8_t, 128> &packed_planes) {
  Ega4PlaneImage img{};
  img.width_bytes = 2U;
  img.height_rows = 16U;
  img.row_span_bytes = 32U;
  for (std::size_t p = 0; p < 4U; ++p) {
    img.planes[p].assign(packed_planes.begin() + p * 32U,
                         packed_planes.begin() + (p + 1U) * 32U);
  }
  return img;
}

Ega4PlaneImage make_8x15_mode_bar_image(bool selected) {
  // Authentic 8x15 indicator sprite from comic2_data.asm (0x123BA / 0x123FA,
  // seg_5E:0x98EA / 0x992A)
  Ega4PlaneImage img{};
  img.width_bytes = 1U;
  img.height_rows = 15U;
  img.row_span_bytes = 15U;
  img.planes[0].assign(15U, 0x40U);
  img.planes[1].assign(15U, 0x5FU);
  img.planes[2].assign(15U, 0x40U);
  img.planes[3].assign(15U, selected ? 0xBFU : 0xA0U);
  return img;
}

void write_bcd_digit(EgaPlanarSurface &surface, std::size_t x, std::size_t y,
                     std::uint8_t value) {
  if (value > 9) {
    value = 9;
  }
  const std::uint8_t glyph = static_cast<std::uint8_t>('0' + value);
  constexpr std::uint8_t kColorIndex = 0x0F;

  if ((x % 8) != 0) {
    // Unaligned digits are not supported by this byte-oriented helper yet.
    return;
  }

  const std::size_t x_byte = x / 8;
  for (std::size_t row = 0; row < 8; ++row) {
    std::uint8_t mask = 0;
    for (std::size_t col = 0; col < 8; ++col) {
      if ((glyph + row + col) % 7 != 0) {
        mask = static_cast<std::uint8_t>(mask | (1U << (7 - col)));
      }
    }

    for (std::size_t plane = 0; plane < 4; ++plane) {
      const std::uint8_t byte_value =
          ((kColorIndex >> plane) & 0x1U) ? mask : 0;
      surface.set_plane_byte(plane, x_byte, y + row, byte_value);
    }
  }
}

void write_two_digit_counter(EgaPlanarSurface &surface, std::size_t x,
                             std::size_t y, std::uint8_t value) {
  const auto tens = static_cast<std::uint8_t>((value / 10U) % 10U);
  const auto ones = static_cast<std::uint8_t>(value % 10U);
  write_bcd_digit(surface, x, y, tens);
  write_bcd_digit(surface, x + 8, y, ones);
}

void draw_rect(EgaPlanarSurface &surface, std::size_t x, std::size_t y,
               std::size_t w, std::size_t h, std::uint8_t color) {
  if (w == 0 || h == 0) {
    return;
  }
  if ((x % 8) != 0 || (w % 8) != 0) {
    // Unaligned rects would need bit masking; keep the helper byte-aligned.
    return;
  }

  const std::size_t start_byte = x / 8;
  const std::size_t byte_count = w / 8;

  for (std::size_t py = 0; py < h; ++py) {
    const std::size_t y_row = y + py;
    for (std::size_t plane = 0; plane < 4; ++plane) {
      const std::uint8_t byte_value = ((color >> plane) & 0x1U) ? 0xFF : 0x00;
      for (std::size_t i = 0; i < byte_count; ++i) {
        surface.set_plane_byte(plane, start_byte + i, y_row, byte_value);
      }
    }
  }
}

void draw_rect_unaligned(EgaPlanarSurface &surface, std::size_t x,
                         std::size_t y, std::size_t w, std::size_t h,
                         std::uint8_t color) {
  if (w == 0 || h == 0) {
    return;
  }

  for (std::size_t py = 0; py < h; ++py) {
    const std::size_t y_row = y + py;
    if (y_row >= surface.height_rows()) {
      break;
    }
    for (std::size_t px = 0; px < w; ++px) {
      const std::size_t x_pixel = x + px;
      if (x_pixel >= surface.width_pixels()) {
        break;
      }
      const std::size_t x_byte = x_pixel / 8U;
      const std::uint8_t bit_mask =
          static_cast<std::uint8_t>(1U << (7U - (x_pixel % 8U)));
      for (std::size_t plane = 0; plane < EgaPlanarSurface::kPlaneCount;
           ++plane) {
        auto value = surface.get_plane_byte(plane, x_byte, y_row);
        if (((color >> plane) & 0x1U) != 0U) {
          value = static_cast<std::uint8_t>(value | bit_mask);
        } else {
          value = static_cast<std::uint8_t>(value & ~bit_mask);
        }
        surface.set_plane_byte(plane, x_byte, y_row, value);
      }
    }
  }
}

bool should_draw_hud_overlay(const RuntimeState &state) {
  return state.bezel_frame.has_value() ||
         state.ui.menu_state != MenuState::None || state.ui.modal_active ||
         state.player.score != 0 || state.player.gems != 0 ||
         state.player.lives != 3 || state.ui.active_mode_mask != 0 ||
         state.ui.inventory_mask != 0 ||
         state.progression.mode_inventory_mask != 0 ||
         state.player.boots_equipped || state.player.lantern_equipped ||
         state.player.corkscrew_equipped || state.player.wand_equipped ||
         state.progression.boots_acquired ||
         state.progression.lantern_acquired ||
         state.progression.corkscrew_acquired ||
         state.progression.wand_acquired || state.ui.cinematic_frame != 0;
}

bool can_blit_opaque_image(const EgaPlanarSurface &surface,
                           std::size_t x_pixels, std::size_t y_rows,
                           const Ega4PlaneImage &image) {
  if (image.width_bytes == 0U || image.height_rows == 0U) {
    return false;
  }
  if (x_pixels >= surface.width_pixels() || y_rows >= surface.height_rows()) {
    return false;
  }
  const std::size_t dest_byte_offset = x_pixels / 8U;
  if (dest_byte_offset + static_cast<std::size_t>(image.width_bytes) >
          surface.row_stride_bytes() ||
      y_rows + static_cast<std::size_t>(image.height_rows) >
          surface.height_rows()) {
    return false;
  }
  const std::size_t required_plane_bytes =
      static_cast<std::size_t>(image.width_bytes) *
      static_cast<std::size_t>(image.height_rows);
  return std::all_of(image.planes.begin(), image.planes.end(),
                     [required_plane_bytes](const auto &plane) {
                       return plane.size() >= required_plane_bytes;
                     });
}

bool can_make_masked_sprite_image(const MaskedSpriteRecord &record) {
  if (record.width_pixels == 0U || record.height_rows == 0U) {
    return false;
  }
  const std::size_t width_bytes =
      (static_cast<std::size_t>(record.width_pixels) + 7U) / 8U;
  const std::size_t plane_bytes =
      width_bytes * static_cast<std::size_t>(record.height_rows);
  if (plane_bytes > std::numeric_limits<std::uint16_t>::max()) {
    return false;
  }
  return std::all_of(
      record.planes.begin(), record.planes.end(),
      [plane_bytes](const auto &plane) { return plane.size() >= plane_bytes; });
}

} // namespace

std::uint8_t bcd_increment(std::uint8_t value) {
  if (value >= 0x99) {
    return 0x99;
  }
  const auto low = static_cast<std::uint8_t>(value & 0x0F);
  if (low < 0x09) {
    return static_cast<std::uint8_t>(value + 1U);
  }
  const auto high = static_cast<std::uint8_t>((value >> 4) & 0x0F);
  if (high < 0x09) {
    return static_cast<std::uint8_t>((high + 1) << 4);
  }
  return 0x99;
}

std::uint8_t bcd_decrement(std::uint8_t value) {
  if (value == 0x00) {
    return 0x00;
  }
  if (value >= 0x10 && (value & 0x0F) == 0x00) {
    return static_cast<std::uint8_t>(value - 0x10U + 0x09U);
  }
  if ((value & 0x0F) != 0x00) {
    return static_cast<std::uint8_t>(value - 1U);
  }
  return static_cast<std::uint8_t>(value - 0x10U);
}

void ui_draw_framed_text_box_and_text(EgaPlanarSurface &surface,
                                      const std::vector<std::string> &lines,
                                      std::uint8_t width_cells,
                                      std::uint8_t height_cells,
                                      std::uint8_t border_color,
                                      std::uint8_t text_color) {
  if (lines.empty()) {
    return;
  }

  const auto longest_it =
      std::max_element(lines.begin(), lines.end(),
                       [](const std::string &a, const std::string &b) {
                         return a.size() < b.size();
                       });
  const std::size_t max_line_len =
      longest_it != lines.end() ? longest_it->size() : 0U;

  std::size_t inner_w = std::clamp<std::size_t>(max_line_len, 1U, 38U);
  std::size_t inner_h = std::clamp<std::size_t>(lines.size(), 1U, 23U);
  if (width_cells >= 3U && height_cells >= 3U && width_cells <= 40U &&
      height_cells <= 25U &&
      static_cast<std::size_t>(height_cells - 2U) == lines.size() &&
      static_cast<std::size_t>(width_cells - 2U) >= max_line_len) {
    inner_w = static_cast<std::size_t>(width_cells - 2U);
    inner_h = static_cast<std::size_t>(height_cells - 2U);
  }

  const std::size_t w_cells = inner_w + 2U;
  const std::size_t h_cells = inner_h + 2U;
  const std::size_t box_w_px = w_cells * 8U;
  const std::size_t box_h_px = h_cells * 8U;

  // Authentic DOS centering formula from ui_draw_window_box (1000:4206):
  const std::int32_t x0 =
      (box_w_px <= 0xD0U)
          ? static_cast<std::int32_t>((((0xD0U - box_w_px) >> 1U) & ~7U) + 8U)
          : 0;
  const std::int32_t y0 =
      (box_h_px <= 0xA0U)
          ? static_cast<std::int32_t>((((0xA0U - box_h_px) >> 1U) & ~7U) + 8U)
          : 0;

  draw_rect(surface, static_cast<std::size_t>(x0), static_cast<std::size_t>(y0),
            box_w_px, box_h_px, 0x00U);

  // Top border row: '"' (0x22), '#' (0x23) * inner_w, '$' (0x24)
  blit_char(surface, x0, y0, '"', border_color);
  for (std::size_t col = 0U; col < inner_w; ++col) {
    blit_char(surface, x0 + static_cast<std::int32_t>((col + 1U) * 8U), y0, '#',
              border_color);
  }
  blit_char(surface, x0 + static_cast<std::int32_t>((inner_w + 1U) * 8U), y0,
            '$', border_color);

  // Interior text rows: '%' (0x25), text chars, '&' (0x26)
  for (std::size_t row = 0U; row < inner_h; ++row) {
    const std::int32_t y = y0 + static_cast<std::int32_t>((row + 1U) * 8U);
    blit_char(surface, x0, y, '%', border_color);
    for (std::size_t col = 0U; col < inner_w; ++col) {
      const char ch = (row < lines.size() && col < lines[row].size())
                          ? lines[row][col]
                          : ' ';
      blit_char(surface, x0 + static_cast<std::int32_t>((col + 1U) * 8U), y, ch,
                text_color);
    }
    blit_char(surface, x0 + static_cast<std::int32_t>((inner_w + 1U) * 8U), y,
              '&', border_color);
  }

  // Bottom border row: ':' (0x3A), ';' (0x3B) * inner_w, '<' (0x3C)
  const std::int32_t bottom_y =
      y0 + static_cast<std::int32_t>((inner_h + 1U) * 8U);
  blit_char(surface, x0, bottom_y, ':', border_color);
  for (std::size_t col = 0U; col < inner_w; ++col) {
    blit_char(surface, x0 + static_cast<std::int32_t>((col + 1U) * 8U),
              bottom_y, ';', border_color);
  }
  blit_char(surface, x0 + static_cast<std::int32_t>((inner_w + 1U) * 8U),
            bottom_y, '<', border_color);
}

void ui_render_modal_prompt(EgaPlanarSurface &surface,
                            const RuntimeState &state) {
  if (!state.ui.modal_active || state.ui.modal_prompt.empty()) {
    return;
  }

  std::vector<std::string> lines;
  std::size_t start = 0U;
  while (start <= state.ui.modal_prompt.size()) {
    const std::size_t nl = state.ui.modal_prompt.find('\n', start);
    if (nl == std::string::npos) {
      lines.push_back(state.ui.modal_prompt.substr(start));
      break;
    }
    lines.push_back(state.ui.modal_prompt.substr(start, nl - start));
    start = nl + 1U;
  }

  const std::uint8_t text_color = state.ui.modal_game_over ? 0x0CU : 0x07U;
  ui_draw_framed_text_box_and_text(surface, lines, state.ui.modal_width_cells,
                                   state.ui.modal_height_cells, 0x09U,
                                   text_color);
}

void hud_update_mode_icons(EgaPlanarSurface &surface,
                           const RuntimeState &state) {
  // Authentic Y positions from DS:0x9710 in comic2_standalone.asm
  // (hud_draw_item_icon at 1000:3a17): 0x5E (94), 0x77 (119), 0x90 (144),
  // 0xA9 (169).
  // Blitted at x = 0x120 (288) on the right-hand FRPAK.001 bezel panel.
  constexpr std::size_t kIconX = 0x120U; // 288
  constexpr std::size_t kBarX = 280U;
  constexpr std::array<std::size_t, 4> kRowY = {0x5EU, 0x77U, 0x90U, 0xA9U};

  const bool corkscrew_collected =
      state.player.corkscrew_equipped || state.progression.corkscrew_acquired ||
      (state.ui.inventory_mask & 0x04U) != 0U ||
      (state.progression.mode_inventory_mask & 0x04U) != 0U;
  const bool boots_collected =
      state.player.boots_equipped || state.progression.boots_acquired ||
      (state.ui.inventory_mask & 0x01U) != 0U ||
      (state.progression.mode_inventory_mask & 0x01U) != 0U;
  const bool lantern_collected =
      state.player.lantern_equipped || state.progression.lantern_acquired ||
      (state.ui.inventory_mask & 0x02U) != 0U ||
      (state.progression.mode_inventory_mask & 0x02U) != 0U;
  const bool wand_collected =
      state.player.wand_equipped || state.progression.wand_acquired ||
      (state.ui.inventory_mask & 0x08U) != 0U ||
      (state.progression.mode_inventory_mask & 0x08U) != 0U;

  if (corkscrew_collected) {
    static const auto kCorkscrewImg =
        make_16x16_hud_icon_image(kHudCorkscrewIconPlanes);
    if (can_blit_opaque_image(surface, kIconX, kRowY[0], kCorkscrewImg)) {
      gfx_rle_blit_opaque_4plane(surface, kIconX, kRowY[0], kCorkscrewImg);
    }
  }
  if (boots_collected) {
    static const auto kBootsImg =
        make_16x16_hud_icon_image(kHudBootsIconPlanes);
    if (can_blit_opaque_image(surface, kIconX, kRowY[1], kBootsImg)) {
      gfx_rle_blit_opaque_4plane(surface, kIconX, kRowY[1], kBootsImg);
    }
  }
  if (lantern_collected) {
    static const auto kLanternImg =
        make_16x16_hud_icon_image(kHudLanternIconPlanes);
    if (can_blit_opaque_image(surface, kIconX, kRowY[2], kLanternImg)) {
      gfx_rle_blit_opaque_4plane(surface, kIconX, kRowY[2], kLanternImg);
    }
  }
  if (wand_collected) {
    static const auto kWandImg = make_16x16_hud_icon_image(kHudWandIconPlanes);
    if (can_blit_opaque_image(surface, kIconX, kRowY[3], kWandImg)) {
      gfx_rle_blit_opaque_4plane(surface, kIconX, kRowY[3], kWandImg);
    }
  }

  // Render active/inactive 8x15 mode indicator bars (seg_5E:0x98EA / 0x992A)
  // when a mode selection mask or effect is active.
  if (state.ui.active_mode_mask != 0U ||
      state.player.active_mode_effect != 0U) {
    static const auto kSelectedBar = make_8x15_mode_bar_image(true);
    static const auto kUnselectedBar = make_8x15_mode_bar_image(false);
    for (std::size_t row = 0; row < 4U; ++row) {
      const bool row_selected =
          ((state.ui.active_mode_mask & (1U << row)) != 0U) ||
          (state.player.active_mode_effect ==
           static_cast<std::uint8_t>(row + 1U));
      const auto &bar = row_selected ? kSelectedBar : kUnselectedBar;
      if (can_blit_opaque_image(surface, kBarX, kRowY[row], bar)) {
        gfx_rle_blit_opaque_4plane(surface, kBarX, kRowY[row], bar);
      }
    }
  }
}

void hud_render_overlay(EgaPlanarSurface &surface, const RuntimeState &state) {
  if (!should_draw_hud_overlay(state)) {
    return;
  }

  if (state.bezel_frame.has_value()) {
    // Authentic bezel HUD: render digit counters on the right panel.
    // Original layout: x=258..272 (two-digit field), y=0x5E/0x77/0x90/0xA9.
    constexpr std::int32_t kHudX = 258;
    constexpr std::uint8_t kHudColor = 0x0F; // bright white

    // Shield energy bar (8 pips at y=0x5E=94, within x=258..273)
    {
      const auto pips = std::min<std::uint8_t>(8U, state.player.shield_power);
      for (std::uint8_t i = 0; i < 8U; ++i) {
        const std::uint8_t color = (i < pips) ? 0x0AU : 0x08U;
        draw_rect_unaligned(surface, static_cast<std::size_t>(kHudX + i * 2),
                            0x5EU, 2U, 6U, color);
      }
    }

    // Firepower counter at y=0x77=119
    {
      char buf[3] = {'0', '0', '\0'};
      buf[0] = static_cast<char>('0' + (state.player.firepower / 10U) % 10U);
      buf[1] = static_cast<char>('0' + state.player.firepower % 10U);
      blit_string(surface, kHudX, 0x77, buf, kHudColor);
    }

    // Gems counter at y=0x90=144
    {
      char buf[3] = {'0', '0', '\0'};
      buf[0] = static_cast<char>('0' + (state.player.gems / 10U) % 10U);
      buf[1] = static_cast<char>('0' + state.player.gems % 10U);
      blit_string(surface, kHudX, 0x90, buf, kHudColor);
    }

    // Lives counter at y=0xA9=169
    {
      char buf[3] = {'0', '0', '\0'};
      buf[0] = static_cast<char>('0' + (state.player.lives / 10U) % 10U);
      buf[1] = static_cast<char>('0' + state.player.lives % 10U);
      blit_string(surface, kHudX, 0xA9, buf, kHudColor);
    }

    // Score (6 digits) at y=8 (top of right panel)
    {
      const std::uint32_t full_score =
          (static_cast<std::uint32_t>(state.player.score_high) << 16U) |
          state.player.score;
      char buf[7];
      std::snprintf(buf, sizeof(buf), "%06u",
                    static_cast<unsigned>(full_score % 1000000U));
      blit_string(surface, kHudX - 8, 8, buf, kHudColor);
    }

    // Keys at y=0x43=67
    {
      char buf[2] = {'0', '\0'};
      buf[0] = static_cast<char>('0' + state.player.key_count % 10U);
      blit_string(surface, kHudX + 8, 0x43, buf, kHudColor);
    }

    hud_update_mode_icons(surface, state);
    return;
  }

  // Fallback HUD strip at bottom of frame (no bezel present).
  draw_rect(surface, 0, 184, 320, 16, 0x03);
  write_two_digit_counter(surface, 8, 188, state.player.score & 0xFF);
  write_two_digit_counter(surface, 56, 188, state.player.gems);
  write_two_digit_counter(surface, 104, 188, state.player.lives);

  draw_rect(surface, 144, 188, 8, 8, 0x0F);
  draw_rect(surface, 152, 188, 8, 8, 0x0E);
  draw_rect(surface, 160, 188, 8, 8, 0x0C);

  if ((state.ui.active_mode_mask & 0x01U) != 0U) {
    draw_rect(surface, 144, 188, 8, 8, 0x0F);
  }
  if ((state.ui.active_mode_mask & 0x02U) != 0U) {
    draw_rect(surface, 144, 188, 8, 8, 0x0E);
  }
  if ((state.ui.active_mode_mask & 0x04U) != 0U) {
    draw_rect(surface, 144, 188, 8, 8, 0x0D);
  }
  if ((state.ui.active_mode_mask & 0x08U) != 0U) {
    draw_rect(surface, 144, 188, 8, 8, 0x0C);
  }
  if ((state.ui.inventory_mask & 0x01U) != 0U || state.player.boots_equipped ||
      state.progression.boots_acquired) {
    draw_rect(surface, 152, 188, 8, 8, 0x0E);
  }
  if ((state.ui.inventory_mask & 0x02U) != 0U ||
      state.player.lantern_equipped || state.progression.lantern_acquired) {
    draw_rect(surface, 160, 188, 8, 8, 0x0C);
  }
  if (((state.ui.inventory_mask & 0x04U) != 0U) ||
      state.progression.mode_inventory_mask != 0U ||
      state.player.corkscrew_equipped || state.player.wand_equipped ||
      state.progression.corkscrew_acquired || state.progression.wand_acquired) {
    draw_rect(surface, 160, 188, 8, 8, 0x0C);
  }
  hud_update_mode_icons(surface, state);
}

void ui_render_option_list(EgaPlanarSurface &surface,
                           const RuntimeState &state) {
  // Authentic DOS Game Selection / Player Save Slot screen
  // (ui_draw_menu_frame at 1000:5f86 & ui_render_option_list at 1000:606b):
  surface.clear(0x00U);

  // Header strings above border box in color 0x07
  blit_string(surface, 16, 0, "CAPTAIN COMIC II - FRACTURED REALITY", 0x07U);
  blit_string(surface, 40, 16, "  G A M E  S E L E C T I O N  ", 0x07U);

  // Double-line border box in light blue (0x09) from Y=24 to Y=192, X=0..312
  blit_char(surface, 0, 24, '"', 0x09U);
  for (std::int32_t col = 1; col <= 38; ++col) {
    blit_char(surface, col * 8, 24, '#', 0x09U);
  }
  blit_char(surface, 312, 24, '$', 0x09U);

  for (std::int32_t row_y = 32; row_y <= 184; row_y += 8) {
    blit_char(surface, 0, row_y, '%', 0x09U);
    blit_char(surface, 312, row_y, '&', 0x09U);
  }

  blit_char(surface, 0, 192, ':', 0x09U);
  for (std::int32_t col = 1; col <= 38; ++col) {
    blit_char(surface, col * 8, 192, ';', 0x09U);
  }
  blit_char(surface, 312, 192, '<', 0x09U);

  // Column header inside box
  blit_string(surface, 64, 32, "PLAYER         AWARDS", 0x07U);

  const auto &menu_sprites = get_embedded_menu_sprites();

  for (std::size_t i = 0; i < state.ui.option_labels.size() && i < 8U; ++i) {
    const bool is_selected = (i == state.ui.selected_option_index);
    const std::uint8_t color = is_selected ? 0x0AU : 0x07U;
    const std::int32_t row_y = static_cast<std::int32_t>(44U + i * 16U);

    if (is_selected) {
      if (menu_sprites.size() >= 4U) {
        const std::size_t cursor_idx = state.ui.retire_player_mode ? 3U : 2U;
        gfx_blit_masked_sprite(surface, 44, row_y - 4,
                               menu_sprites[cursor_idx]);
      } else {
        blit_char(surface, 48, row_y, '+', 0x0FU);
      }
    }

    if (state.ui.editing_player_name && is_selected && i < 5U) {
      const std::string editing_text = state.ui.pending_player_name_input + "+";
      blit_string(surface, 64, row_y, editing_text.c_str(), 0x07U);
    } else {
      const std::string &label = state.ui.option_labels[i];
      if (!label.empty()) {
        blit_string(surface, 64, row_y, label.c_str(), color);
      }
    }

    // Render high score / award entries from ds_high_scores_buffer (FR.CFG
    // records[4..8] / player_awards[0..4]) for player slot rows 0..4
    if (i < 5U) {
      const std::uint16_t awards =
          state.dos_config.player_awards[i] != 0U
              ? state.dos_config.player_awards[i]
              : (!state.dos_config.player_names[i].empty()
                     ? state.dos_config.records[4U + i]
                     : 0U);
      if (awards > 0U || !state.dos_config.player_names[i].empty()) {
        char award_buf[8];
        std::snprintf(award_buf, sizeof(award_buf), "%02u",
                      static_cast<unsigned>(awards % 100U));
        blit_string(surface, 200, row_y, award_buf, 0x0EU);

        if (menu_sprites.size() >= 2U && awards > 0U) {
          std::uint16_t rem = std::min<std::uint16_t>(awards, 25U);
          std::int32_t medal_x = 224;
          while (rem >= 5U && medal_x <= 296) {
            gfx_blit_masked_sprite(surface, medal_x, row_y, menu_sprites[0]);
            medal_x += 8;
            rem = static_cast<std::uint16_t>(rem - 5U);
          }
          while (rem > 0U && medal_x <= 296) {
            gfx_blit_masked_sprite(surface, medal_x, row_y, menu_sprites[1]);
            medal_x += 8;
            --rem;
          }
        }
      }
    }
  }
}

void ui_render_game_selection_panel(EgaPlanarSurface &surface,
                                    const RuntimeState &state) {
  if (state.ui.player_slot_menu_active) {
    ui_render_option_list(surface, state);
    if (state.ui.modal_active) {
      ui_render_modal_prompt(surface, state);
    }
    return;
  }

  if (state.bezel_frame.has_value() &&
      can_blit_opaque_image(surface, 0, 0, *state.bezel_frame)) {
    gfx_rle_blit_opaque_4plane(surface, 0, 0, *state.bezel_frame);
  }

  static constexpr std::array<const char *, 6> kWorldNames = {
      "NATURE", "MAGIC", "TIME", "ICE", "TECHNO", "DREAMS",
  };

  if (state.menu_panel.has_value() &&
      can_blit_opaque_image(surface, 8, 8, *state.menu_panel) &&
      state.menu_buttons.size() >= 2 &&
      can_make_masked_sprite_image(state.menu_buttons[0]) &&
      can_make_masked_sprite_image(state.menu_buttons[1])) {
    gfx_rle_blit_opaque_4plane(surface, 8, 8, *state.menu_panel);

    struct WorldButton {
      std::size_t x;
      std::size_t y;
      const char *name;
    };
    static constexpr std::array<WorldButton, 6> kWorldButtons = {{
        {24, 24, "NATURE"},
        {88, 24, "MAGIC"},
        {152, 24, "TIME"},
        {24, 112, "ICE"},
        {88, 112, "TECHNO"},
        {152, 112, "DREAMS"},
    }};

    const auto unselected_image =
        make_masked_sprite_image(state.menu_buttons[1]);
    const auto selected_image = make_masked_sprite_image(state.menu_buttons[0]);
    for (std::size_t i = 0; i < kWorldButtons.size(); ++i) {
      const bool is_selected = (i == state.ui.selected_option_index);
      const auto &btn_img = is_selected ? selected_image : unselected_image;
      if (kWorldButtons[i].x < surface.width_pixels() &&
          kWorldButtons[i].y < surface.height_rows()) {
        gfx_rle_blit_masked_or_4plane(surface, kWorldButtons[i].x,
                                      kWorldButtons[i].y, btn_img);
      }
    }
    if (state.ui.selected_option_index < kWorldButtons.size()) {
      blit_string(surface, 136, 90,
                  kWorldButtons[state.ui.selected_option_index].name, 0x0F);
    }
    if (state.ui.player_slot_menu_active) {
      ui_render_option_list(surface, state);
    }
    if (state.ui.modal_active) {
      ui_render_modal_prompt(surface, state);
    }
    hud_render_overlay(surface, state);
    return;
  }

  draw_rect(surface, 20, 20, 280, 140, 0x02);
  draw_rect(surface, 28, 32, 264, 12, 0x0F);
  if (state.ui.menu_state == MenuState::GameSelect) {
    draw_rect(surface, 80, 70, 160, 8, 0x0E);
  }
  blit_string(surface, 48, 34, "GAME SELECTION", 0x01);
  const std::size_t world_idx =
      state.ui.selected_option_index < kWorldNames.size()
          ? state.ui.selected_option_index
          : 0U;
  blit_string(surface, 136, 90, kWorldNames[world_idx], 0x0F);

  // Render high score / player slot summary lines below world label
  for (std::size_t slot = 0; slot < 5U; ++slot) {
    if (!state.dos_config.player_names[slot].empty()) {
      const std::int32_t y = static_cast<std::int32_t>(104U + slot * 10U);
      blit_string(surface, 48, y, state.dos_config.player_names[slot].c_str(),
                  0x07);
      const std::uint16_t awards = state.dos_config.player_awards[slot] != 0U
                                       ? state.dos_config.player_awards[slot]
                                       : state.dos_config.records[4U + slot];
      char award_buf[8];
      std::snprintf(award_buf, sizeof(award_buf), "%02u",
                    static_cast<unsigned>(awards % 100U));
      blit_string(surface, 200, y, award_buf, 0x0E);
    }
  }

  if (state.ui.player_slot_menu_active) {
    ui_render_option_list(surface, state);
  }
  if (state.ui.modal_active) {
    ui_render_modal_prompt(surface, state);
  }
}

// cppcheck-suppress unusedFunction ; reserved for Phase 10
bool ui_show_modal_prompt_wait_key(RuntimeState &state,
                                   const std::string &prompt,
                                   std::uint8_t awaited_key) {
  state.ui.modal_prompt = prompt;
  state.ui.awaited_key = awaited_key;
  state.ui.modal_active = true;
  queue_audio_event(state, AudioEvent::ModalPrompt);
  return true;
}

void ui_option_list_input_loop(RuntimeState &state, const InputState &input) {
  if (state.ui.menu_state != MenuState::Pause &&
      state.ui.menu_state != MenuState::Options &&
      state.ui.menu_state != MenuState::GameSelect) {
    return;
  }

  if (input.pause_pressed) {
    state.ui.menu_state = MenuState::Pause;
    return;
  }

  if (input.left_pressed) {
    if (state.ui.player_slot_menu_active) {
      ui_select_prev_valid_option(state);
    } else {
      ui_select_next_valid_option(state);
    }
  } else if (input.right_pressed || input.down_pressed) {
    ui_select_next_valid_option(state);
  }
}

void ui_edit_selected_option_text(RuntimeState &state,
                                  const std::string &text) {
  std::string sanitized;
  sanitized.reserve(15U);
  for (char ch : text) {
    if (sanitized.size() >= 15U) {
      break;
    }
    const unsigned char uch = static_cast<unsigned char>(ch);
    const char upper = static_cast<char>(std::toupper(uch));
    if (upper == ' ' || (upper >= '0' && upper <= 'Z')) {
      sanitized.push_back(upper);
    }
  }
  if (state.ui.selected_option_index < state.ui.option_labels.size()) {
    state.ui.option_labels[state.ui.selected_option_index] = sanitized;
  }
  if (state.ui.selected_option_index < state.dos_config.player_names.size()) {
    state.dos_config.player_names[state.ui.selected_option_index] = sanitized;
  }
}

bool ui_is_option_selectable(const RuntimeState &state,
                             std::size_t option_index) {
  const std::size_t count =
      !state.ui.option_labels.empty()
          ? state.ui.option_labels.size()
          : (state.ui.menu_state == MenuState::GameSelect ? 6U : 0U);
  if (option_index >= count) {
    return false;
  }
  if (!state.ui.player_slot_menu_active) {
    if (!state.ui.option_labels.empty()) {
      return !state.ui.option_labels[option_index].empty();
    }
    return true;
  }

  // Authentic DOS ui_is_option_selectable (1000:6030):
  // Rows 0..4: selectable only if player slot is non-empty
  // Row 5: blank separator (never selectable)
  // Row 6: "ENTER A NEW PLAYER" selectable when !retire_player_mode &&
  //        player_count < 5
  // Row 7: "RETIRE A PLAYER" selectable when !retire_player_mode &&
  //        player_count > 0; in retire_player_mode ("HIT (ESC) TO ABORT"),
  //        selectable to abort.
  if (option_index < 5U) {
    return !state.dos_config.player_names[option_index].empty();
  }
  if (option_index == 5U) {
    return false;
  }
  if (option_index == 6U) {
    if (state.ui.retire_player_mode) {
      return false;
    }
    return state.dos_config.player_count < 5U;
  }
  if (option_index == 7U) {
    if (state.ui.retire_player_mode) {
      return true;
    }
    return state.dos_config.player_count > 0U;
  }
  return false;
}

void ui_select_next_valid_option(RuntimeState &state) {
  const std::size_t count =
      !state.ui.option_labels.empty()
          ? state.ui.option_labels.size()
          : (state.ui.menu_state == MenuState::GameSelect ? 6U : 0U);
  if (count == 0U) {
    return;
  }

  for (std::size_t step = 1U; step <= count; ++step) {
    const std::size_t candidate =
        (state.ui.selected_option_index + step) % count;
    if (ui_is_option_selectable(state, candidate)) {
      state.ui.selected_option_index = candidate;
      return;
    }
  }
}

bool ui_select_prev_valid_option(RuntimeState &state) {
  const std::size_t count =
      !state.ui.option_labels.empty()
          ? state.ui.option_labels.size()
          : (state.ui.menu_state == MenuState::GameSelect ? 6U : 0U);
  if (count == 0U) {
    return false;
  }

  for (std::size_t step = 1U; step <= count; ++step) {
    const std::size_t candidate =
        (state.ui.selected_option_index + count - step) % count;
    if (ui_is_option_selectable(state, candidate)) {
      state.ui.selected_option_index = candidate;
      return true;
    }
  }
  return false;
}

void ui_init_player_slot_menu(RuntimeState &state) {
  state.ui.player_slot_menu_active = true;
  state.ui.menu_state = MenuState::Options;

  const auto active_count = static_cast<std::uint16_t>(
      std::count_if(state.dos_config.player_names.begin(),
                    state.dos_config.player_names.end(),
                    [](const std::string &name) { return !name.empty(); }));
  state.dos_config.player_count = active_count;

  state.ui.option_labels.assign(8U, "");
  for (std::size_t i = 0; i < 5U; ++i) {
    state.ui.option_labels[i] = state.dos_config.player_names[i];
  }
  state.ui.option_labels[5] = "";
  if (state.ui.editing_player_name) {
    state.ui.option_labels[6] = "ENTER YOUR NAME.";
    state.ui.option_labels[7] = "HIT (ESC) TO ABORT";
  } else if (state.ui.retire_player_mode) {
    state.ui.option_labels[6] = "ERASE WHICH PLAYER?";
    state.ui.option_labels[7] = "HIT (ESC) TO ABORT";
  } else {
    state.ui.option_labels[6] = "ENTER A NEW PLAYER";
    state.ui.option_labels[7] = "RETIRE A PLAYER";
  }

  for (std::size_t i = 0; i < state.ui.option_labels.size(); ++i) {
    if (ui_is_option_selectable(state, i)) {
      state.ui.selected_option_index = i;
      break;
    }
  }
}

bool ui_enter_new_player(RuntimeState &state, const std::string &player_name) {
  std::size_t target_slot = 5U;
  for (std::size_t i = 0; i < 5U; ++i) {
    if (state.dos_config.player_names[i].empty()) {
      target_slot = i;
      break;
    }
  }
  if (target_slot >= 5U) {
    return false;
  }

  state.ui.selected_option_index = target_slot;
  ui_edit_selected_option_text(state, player_name);
  if (state.dos_config.player_names[target_slot].empty()) {
    return false;
  }

  state.dos_config.player_awards[target_slot] = 0U;
  state.dos_config.records[4U + target_slot] = 0U;
  state.ui.active_player_slot = static_cast<std::uint8_t>(target_slot + 1U);
  state.ui.retire_player_mode = false;
  state.ui.editing_player_name = false;
  ui_init_player_slot_menu(state);
  state.ui.selected_option_index = target_slot;

  savegame_write_snapshot(state, state.saved_snapshot_bytes);
  if (!state.ui.persistence_dir.empty()) {
    std::filesystem::create_directories(state.ui.persistence_dir);
    cfg_save_options_file(state, state.ui.persistence_dir / "FR.CFG");
    savegame_write_slot_file(state, state.ui.persistence_dir,
                             state.ui.active_player_slot);
    if (!std::filesystem::exists(state.ui.persistence_dir / "GAME.000")) {
      savegame_write_slot_file(state, state.ui.persistence_dir, 0U);
    }
  }
  return true;
}

bool ui_retire_player(RuntimeState &state, std::size_t slot_index) {
  if (slot_index >= 5U || state.dos_config.player_names[slot_index].empty()) {
    return false;
  }

  state.dos_config.player_names[slot_index].clear();
  state.dos_config.player_awards[slot_index] = 0U;
  state.dos_config.records[4U + slot_index] = 0U;
  state.ui.retire_player_mode = false;
  ui_init_player_slot_menu(state);

  if (!state.ui.persistence_dir.empty()) {
    std::filesystem::create_directories(state.ui.persistence_dir);
    cfg_save_options_file(state, state.ui.persistence_dir / "FR.CFG");
  }
  return true;
}

std::uint16_t world_selection_to_level(std::size_t world_index) {
  // Authentic 6-world target level table from DS:0x0990 in comic2_data.asm
  // (game_switch_room at 1000:2253):
  // 0: NATURE (10), 1: MAGIC (6), 2: TIME (11), 3: ICE (9), 4: TECHNO (8),
  // 5: DREAMS (13)
  static constexpr std::array<std::uint16_t, 6> kWorldLevels = {
      10U, 6U, 11U, 9U, 8U, 13U,
  };
  return kWorldLevels[world_index % kWorldLevels.size()];
}

bool ui_select_world_or_save_slot(RuntimeState &state, std::size_t slot_index) {
  const std::size_t clamped = slot_index % 6U;
  state.ui.selected_option_index = clamped;
  state.ui.active_player_slot = static_cast<std::uint8_t>(clamped);

  bool loaded_snapshot = false;
  if (!state.ui.persistence_dir.empty()) {
    cfg_load_options_file(state, state.ui.persistence_dir / "FR.CFG");
    keys_load_file(state.key_bindings, state.ui.persistence_dir / "KEYS.DEF");
    if (savegame_read_slot_file(state, state.ui.persistence_dir, clamped)) {
      loaded_snapshot = true;
    }
  }

  if (!loaded_snapshot && !state.assets_root.empty() && clamped > 0U) {
    state.current_level = world_selection_to_level(clamped);
    state.current_room = 0U;
    load_room_tilemap_from_asset_root(state, state.assets_root,
                                      state.current_level, state.current_room);
  }
  return loaded_snapshot;
}

void ui_begin_key_rebinding(RuntimeState &state) {
  state.ui.key_rebinding_active = true;
  state.ui.key_rebinding_step = 0U;
  state.ui.pending_key_bindings = DosKeyBindings{0, 0, 0, 0, 0, 0};
  state.ui.modal_active = true;
  state.ui.modal_prompt = "DEFINE KEYS\nMOVE LEFT  : ";
}

bool ui_submit_key_rebinding_scancode(RuntimeState &state,
                                      std::uint8_t dos_scancode) {
  if (!state.ui.key_rebinding_active) {
    return false;
  }
  if (dos_scancode == 0x01U) {
    // ESC aborts key rebinding
    state.ui.key_rebinding_active = false;
    state.ui.key_rebinding_step = 0U;
    state.ui.modal_active = false;
    state.ui.modal_prompt.clear();
    return false;
  }
  if (!is_valid_unique_dos_key_binding(dos_scancode,
                                       state.ui.pending_key_bindings,
                                       state.ui.key_rebinding_step)) {
    return false;
  }

  switch (state.ui.key_rebinding_step) {
  case 0U:
    state.ui.pending_key_bindings.left = dos_scancode;
    break;
  case 1U:
    state.ui.pending_key_bindings.right = dos_scancode;
    break;
  case 2U:
    state.ui.pending_key_bindings.up = dos_scancode;
    break;
  case 3U:
    state.ui.pending_key_bindings.down = dos_scancode;
    break;
  case 4U:
    state.ui.pending_key_bindings.jump = dos_scancode;
    break;
  case 5U:
    state.ui.pending_key_bindings.fire = dos_scancode;
    break;
  default:
    return false;
  }

  ++state.ui.key_rebinding_step;

  static constexpr std::array<const char *, 6> kActionPrompts = {
      "MOVE LEFT  : ", "MOVE RIGHT : ", "MOVE UP    : ",
      "MOVE DOWN  : ", "JUMP       : ", "TRIGGER    : ",
  };
  const std::array<std::uint8_t, 6> bound_codes = {
      state.ui.pending_key_bindings.left, state.ui.pending_key_bindings.right,
      state.ui.pending_key_bindings.up,   state.ui.pending_key_bindings.down,
      state.ui.pending_key_bindings.jump, state.ui.pending_key_bindings.fire,
  };

  std::string prompt = "DEFINE KEYS";
  const std::size_t visible_steps =
      std::min<std::size_t>(state.ui.key_rebinding_step + 1U, 6U);
  for (std::size_t i = 0; i < visible_steps; ++i) {
    prompt += "\n";
    prompt += kActionPrompts[i];
    if (i < state.ui.key_rebinding_step) {
      prompt += std::string(dos_scancode_name(bound_codes[i]));
    }
  }

  if (state.ui.key_rebinding_step >= 6U) {
    state.key_bindings = state.ui.pending_key_bindings;
    state.ui.key_rebinding_active = false;
    state.ui.modal_prompt = prompt + "\nSETUP SAVED";
    if (!state.ui.persistence_dir.empty()) {
      std::filesystem::create_directories(state.ui.persistence_dir);
      keys_save_file(state.key_bindings, state.ui.persistence_dir / "KEYS.DEF");
      cfg_save_options_file(state, state.ui.persistence_dir / "FR.CFG");
    }
    return true;
  }

  state.ui.modal_prompt = prompt;
  return true;
}

bool render_title_screen(EgaPlanarSurface &surface, const RuntimeState &state) {
  if (state.title_screen.has_value() &&
      can_blit_opaque_image(surface, 0, 0, *state.title_screen)) {
    const auto &starfield = state.starfield_screen.has_value()
                                ? *state.starfield_screen
                                : get_embedded_starfield_screen();
    if (can_blit_opaque_image(surface, 0, 0, starfield)) {
      gfx_rle_blit_opaque_4plane(surface, 0, 0, starfield);
      gfx_rle_blit_masked_or_4plane(surface, 0, 0, *state.title_screen);
    } else {
      gfx_rle_blit_opaque_4plane(surface, 0, 0, *state.title_screen);
    }
    return true;
  }

  draw_rect(surface, 0, 0, 320, 200, 0x01);
  draw_rect(surface, 40, 40, 240, 120, 0x09);
  blit_string(surface, 64, 80, "CAPTAIN COMIC II", 0x0F);
  blit_string(surface, 80, 100, "FRACTURED REALITY", 0x0E);
  return false;
}

namespace {

void render_horizontal_split_wipe(EgaPlanarSurface &surface,
                                  const Ega4PlaneImage &img_from,
                                  const Ega4PlaneImage &img_to,
                                  std::size_t step_bytes) {
  // Implements ui_draw_title_stars (1000:74e1):
  // Rows 0..15 and 176..199 come from img_from; rows 16..175 pan horizontally
  // so the left `step_bytes` columns come from the right side of img_to and
  // the remaining `40 - step_bytes` columns come from the left side of
  // img_from.
  if (!can_blit_opaque_image(surface, 0, 0, img_from) ||
      !can_blit_opaque_image(surface, 0, 0, img_to)) {
    return;
  }
  gfx_rle_blit_opaque_4plane(surface, 0, 0, img_from);
  const std::size_t k = std::clamp<std::size_t>(step_bytes, 0U, 40U);
  for (std::size_t plane = 0; plane < 4U; ++plane) {
    auto dst = surface.plane(plane);
    const auto &src_from = img_from.planes[plane];
    const auto &src_to = img_to.planes[plane];
    for (std::size_t y = 16U; y < 176U; ++y) {
      const std::size_t row_off = y * 40U;
      for (std::size_t xb = 0; xb < k; ++xb) {
        dst[row_off + xb] = src_to[row_off + (40U - k) + xb];
      }
      for (std::size_t xb = k; xb < 40U; ++xb) {
        dst[row_off + xb] = src_from[row_off + (xb - k)];
      }
    }
  }
}

} // namespace

bool render_startup_demo_and_backstory(EgaPlanarSurface &surface,
                                       RuntimeState &state) {
  const auto &starfield = state.starfield_screen.has_value()
                              ? *state.starfield_screen
                              : get_embedded_starfield_screen();
  const auto &credits = state.credits_overlay.has_value()
                            ? *state.credits_overlay
                            : get_embedded_credits_overlay();
  const auto &walk_sprites = !state.demo_walk_sprites.empty()
                                 ? state.demo_walk_sprites
                                 : get_embedded_comic_walk_sprites();

  const bool has_demo_assets = state.intro_screens.size() >= 4U &&
                               state.demo_ship_sprites.size() >= 8U &&
                               state.demo_flash_sprites.size() >= 2U &&
                               state.demo_overlay_images.size() >= 4U &&
                               state.demo_beam_sprites.size() >= 6U &&
                               walk_sprites.size() >= 6U;

  const std::uint16_t tick = state.ui.startup_demo_tick;
  if (state.ui.startup_demo_tick < 60000U) {
    ++state.ui.startup_demo_tick;
  }

  // If demo assets aren't loaded (e.g. minimal unit test state), jump directly
  // to the Title + Backstory composition (or fallback).
  if (!has_demo_assets) {
    if (!state.ui.title_music_started) {
      state.ui.title_music_started = true;
      queue_audio_event(state, AudioEvent::StartupChime);
    }
    load_ega_palette_table(state, make_ega_palette_ds_0024());
    if (state.intro_screens.size() >= 4U &&
        can_blit_opaque_image(surface, 0, 0, starfield) &&
        can_blit_opaque_image(surface, 0, 0, state.intro_screens[3])) {
      gfx_rle_blit_opaque_4plane(surface, 0, 0, starfield);
      gfx_rle_blit_masked_or_4plane(surface, 0, 0, state.intro_screens[3]);
      return true;
    }
    return render_title_screen(surface, state);
  }

  // Phase 0 (ticks 0..24, 25 ticks): Initial credits splash (1000:00EE..0131)
  if (tick < 25U) {
    load_ega_palette_table(state, make_ega_palette_ds_0013());
    gfx_rle_blit_opaque_4plane(surface, 0, 0, starfield);
    gfx_rle_blit_masked_or_4plane(surface, 0, 0, credits);
    return true;
  }

  // Phase 1 (ticks 25..119, 95 ticks): Spaceship landing on Planet Ogron
  // (ui_story_screen at 1000:7164..71FB)
  if (tick < 120U) {
    const std::uint16_t p1 = static_cast<std::uint16_t>(tick - 25U);
    load_ega_palette_table(state, make_ega_palette_ds_abc0());
    gfx_rle_blit_opaque_4plane(surface, 0, 0, starfield);
    if (p1 >= 18U) {
      // Planet horizon graphic FRDEMO.0[7] (0x1722) at (72, 96)
      gfx_blit_masked_sprite(surface, 72, 96, state.demo_ship_sprites[7]);
    }
    if (p1 >= 63U) {
      // 32 steps of spaceship descent from (256, 0) across FRDEMO.0[6..1]
      const std::uint16_t step = static_cast<std::uint16_t>(p1 - 63U);
      std::int32_t sx = 256;
      std::int32_t sy = 0;
      std::size_t sprite_idx = 6U;
      for (std::uint16_t s = 0; s <= step && s < 32U; ++s) {
        if (s < 6U) {
          sx -= 4;
          sy += 3;
          sprite_idx = 6U;
        } else if (s < 12U) {
          sx -= 4;
          sy += 3;
          sprite_idx = 5U;
        } else if (s < 17U) {
          sx -= 8;
          sy += 6;
          sprite_idx = 4U;
        } else if (s < 22U) {
          sx -= 8;
          sy += 6;
          sprite_idx = 3U;
        } else if (s < 27U) {
          sx -= 12;
          sy += 8;
          sprite_idx = 2U;
        } else {
          sx -= 12;
          sy += 8;
          sprite_idx = 1U;
        }
      }
      gfx_blit_masked_sprite(surface, sx, sy,
                             state.demo_ship_sprites[sprite_idx]);
    }
    return true;
  }

  // Phase 2 (ticks 120..431, 312 ticks): Planet Ogron surface
  // (ui_title_screen at 1000:7298..74DF)
  if (tick < 432U) {
    const std::uint16_t p2 = static_cast<std::uint16_t>(tick - 120U);
    const auto &frpak003 = state.intro_screens[0];
    const auto &frpak004 = state.intro_screens[1];

    if (p2 < 50U) {
      load_ega_palette_table(state, make_ega_palette_ds_abd1(p2, false));
      gfx_rle_blit_opaque_4plane(surface, 0, 0, frpak003);
      return true;
    }
    if (p2 < 90U) {
      const std::size_t wipe_step = static_cast<std::size_t>(p2 - 50U + 1U);
      load_ega_palette_table(state, make_ega_palette_ds_abd1(p2, false));
      render_horizontal_split_wipe(surface, frpak003, frpak004, wipe_step);
      return true;
    }

    // Post-wipe sequence on FRPAK.004 with cs:0x7120 palette cycling
    const std::uint16_t act = static_cast<std::uint16_t>(p2 - 90U);
    load_ega_palette_table(state, make_ega_palette_ds_abd1(act, true));
    gfx_rle_blit_opaque_4plane(surface, 0, 0, frpak004);

    auto draw_walk = [&](bool facing_right, std::int32_t x,
                         std::uint16_t step_idx) {
      const std::size_t base = facing_right ? 0U : 3U;
      const std::size_t frame = base + (step_idx % 3U);
      gfx_blit_masked_sprite(surface, x, 129, walk_sprites[frame]);
    };
    auto draw_flash = [&](std::int32_t x, std::uint16_t flash_tick,
                          int overlay_idx) {
      if (overlay_idx >= 0 && static_cast<std::size_t>(overlay_idx) <
                                  state.demo_overlay_images.size()) {
        gfx_rle_blit_opaque_4plane(
            surface, 192, 80,
            state.demo_overlay_images[static_cast<std::size_t>(overlay_idx)]);
      }
      if (flash_tick == 1U) {
        queue_audio_event(state, AudioEvent::IntroFlashB);
      }
      const std::size_t f_idx = (flash_tick % 2U == 0U) ? 0U : 1U;
      gfx_blit_masked_sprite(surface, x, 129, state.demo_flash_sprites[f_idx]);
    };

    if (act < 16U) {
      // Walk right 16 steps from x=0 to x=120 (ends at x=128)
      draw_walk(true, static_cast<std::int32_t>(act * 8U), act);
    } else if (act < 26U) {
      // Flash 1 at x=128
      draw_flash(128, static_cast<std::uint16_t>(act - 16U), -1);
    } else if (act < 36U) {
      // Walk left 10 steps from x=128 to x=56 (ends at x=48)
      const std::uint16_t s = static_cast<std::uint16_t>(act - 26U);
      draw_walk(false, 128 - static_cast<std::int32_t>(s * 8U), s);
    } else if (act < 46U) {
      // Flash 2 at x=48
      draw_flash(48, static_cast<std::uint16_t>(act - 36U), -1);
    } else if (act < 48U) {
      // Walk right 2 steps from x=48 to x=56 (ends at x=64)
      const std::uint16_t s = static_cast<std::uint16_t>(act - 46U);
      draw_walk(true, 48 + static_cast<std::int32_t>(s * 8U), s);
    } else if (act < 58U) {
      // Flash 3 at x=64, followed by IntroFlashA (DS:0xAC3A) on completion
      const std::uint16_t ft = static_cast<std::uint16_t>(act - 48U);
      draw_flash(64, ft, -1);
      if (ft == 9U) {
        queue_audio_event(state, AudioEvent::IntroFlash);
      }
    } else if (act < 76U) {
      // Walk right 18 steps from x=64 to x=200 (ends at x=208) with overlay 0
      const std::uint16_t s = static_cast<std::uint16_t>(act - 58U);
      gfx_rle_blit_opaque_4plane(surface, 192, 80,
                                 state.demo_overlay_images[0]);
      draw_walk(true, 64 + static_cast<std::int32_t>(s * 8U), s);
    } else if (act < 86U) {
      // Flash 4a at x=208 with overlay 0 (0x028C)
      draw_flash(208, static_cast<std::uint16_t>(act - 76U), 0);
    } else if (act < 96U) {
      // Flash 4b at x=208 with overlay 1 (0x0710)
      const std::uint16_t ft = static_cast<std::uint16_t>(act - 86U);
      draw_flash(208, ft, 1);
      if (ft == 9U) {
        queue_audio_event(state, AudioEvent::IntroOverlay);
      }
    } else if (act < 126U) {
      // Hold 30 ticks with overlay 1 (0x0710) and Comic standing at (208, 129)
      gfx_rle_blit_opaque_4plane(surface, 192, 80,
                                 state.demo_overlay_images[1]);
      gfx_blit_masked_sprite(surface, 208, 129, state.demo_flash_sprites[1]);
    } else if (act < 136U) {
      // Flash 4c at x=208 with overlay 1 (0x0710)
      draw_flash(208, static_cast<std::uint16_t>(act - 126U), 1);
    } else if (act < 146U) {
      // Flash 4d at x=208 with overlay 2 (0x0B94)
      const std::uint16_t ft = static_cast<std::uint16_t>(act - 136U);
      draw_flash(208, ft, 2);
      if (ft == 9U) {
        queue_audio_event(state, AudioEvent::IntroOverlay);
      }
    } else if (act < 176U) {
      // Hold 30 ticks with overlay 2 (0x0B94)
      gfx_rle_blit_opaque_4plane(surface, 192, 80,
                                 state.demo_overlay_images[2]);
      gfx_blit_masked_sprite(surface, 208, 129, state.demo_flash_sprites[1]);
    } else if (act < 186U) {
      // Flash 4e at x=208 with overlay 2 (0x0B94), then IntroFlashA (DS:0xAC3A)
      const std::uint16_t ft = static_cast<std::uint16_t>(act - 176U);
      draw_flash(208, ft, 2);
      if (ft == 9U) {
        queue_audio_event(state, AudioEvent::IntroFlash);
      }
    } else {
      // Walk left 26 steps from x=208 to x=8 with overlay 3 (0x1018)
      const std::uint16_t s = static_cast<std::uint16_t>(act - 186U);
      gfx_rle_blit_opaque_4plane(surface, 192, 80,
                                 state.demo_overlay_images[3]);
      draw_walk(false, 208 - static_cast<std::int32_t>(s * 8U), s);
    }
    return true;
  }

  // Phase 3 (ticks 432..443, 12 ticks): Comic beaming up through space
  // (1000:7660..76D5)
  if (tick < 444U) {
    const std::uint16_t p3 = static_cast<std::uint16_t>(tick - 432U);
    load_ega_palette_table(state, make_ega_palette_ds_abc0());
    gfx_rle_blit_opaque_4plane(surface, 0, 0, starfield);
    const std::size_t beam_idx =
        std::min<std::size_t>(p3 / 2U, state.demo_beam_sprites.size() - 1U);
    const std::int32_t by = 180 - static_cast<std::int32_t>(p3 * 10U);
    gfx_blit_masked_sprite(surface, 132, by, state.demo_beam_sprites[beam_idx]);
    return true;
  }

  // Phase 4a (ticks 444..483, 40 ticks): Title Logo Screen (FRPAK.005 over
  // SEG_PALETTE:0x1CD0 with green palette DS:0xABE2 and TitleAndIntroTheme
  // DS:0x00DB, matching 1000:76D6..770B)
  if (!state.ui.title_music_started) {
    state.ui.title_music_started = true;
    queue_audio_event(state, AudioEvent::StartupChime);
  }
  if (tick < 484U) {
    load_ega_palette_table(state, make_ega_palette_ds_abe2());
    gfx_rle_blit_opaque_4plane(surface, 0, 0, starfield);
    gfx_rle_blit_masked_or_4plane(surface, 0, 0, state.intro_screens[2]);
    return true;
  }

  // Phase 4b (ticks >= 484, holds until keypress): Title Banner + Backstory
  // Screen (FRPAK.006 over SEG_PALETTE:0x1CD0 with gold/yellow/red palette
  // DS:0x0024 and TitleAndIntroTheme DS:0x00DB, matching 1000:770C..773E)
  load_ega_palette_table(state, make_ega_palette_ds_0024());
  gfx_rle_blit_opaque_4plane(surface, 0, 0, starfield);
  gfx_rle_blit_masked_or_4plane(surface, 0, 0, state.intro_screens[3]);
  return true;
}

bool render_title_config_options_screen(EgaPlanarSurface &surface,
                                        RuntimeState &state) {
  if (state.ui.key_rebinding_active) {
    load_ega_palette_table(state, make_default_ega_palette());
    surface.clear(0x00);
    blit_string(surface, 112, 40, "DEFINE KEYS", 0x07);
    static constexpr std::array<const char *, 6> kActionPrompts = {
        "MOVE LEFT  : ", "MOVE RIGHT : ", "MOVE UP    : ",
        "MOVE DOWN  : ", "JUMP       : ", "TRIGGER    : ",
    };
    const std::array<std::uint8_t, 6> bound_codes = {
        state.ui.pending_key_bindings.left, state.ui.pending_key_bindings.right,
        state.ui.pending_key_bindings.up,   state.ui.pending_key_bindings.down,
        state.ui.pending_key_bindings.jump, state.ui.pending_key_bindings.fire,
    };
    const std::size_t visible_steps =
        std::min<std::size_t>(state.ui.key_rebinding_step + 1U, 6U);
    for (std::size_t i = 0; i < visible_steps; ++i) {
      std::string line = kActionPrompts[i];
      if (i < state.ui.key_rebinding_step) {
        line += std::string(dos_scancode_name(bound_codes[i]));
      }
      blit_string(surface, 72, static_cast<std::int32_t>(64U + i * 12U),
                  line.c_str(), 0x07);
    }
    return true;
  }

  if (state.ui.joystick_calibration_active) {
    load_ega_palette_table(state, make_default_ega_palette());
    surface.clear(0x00);
    blit_string(surface, 88, 48, "CALIBRATE JOYSTICK", 0x07);
    blit_string(surface, 72, 64, "PRESS ANY KEY TO ABORT", 0x07);
    blit_string(surface, 32, 96, "CENTER JOYSTICK AND PRESS BUTTON", 0x07);
    return true;
  }

  // Authentic Keyboard & Joystick Configuration Title Screen
  // (vid_init_screen_pages at 1000:02B4):
  // Opaque-blit SEG_PALETTE:0x1CD0 (starfield) + Masked-OR blit
  // SEG_PALETTE:0x0CDC (keyboard/joystick options overlay) with palette
  // DS:0x0024.
  load_ega_palette_table(state, make_ega_palette_ds_0024());
  const auto &starfield = state.starfield_screen.has_value()
                              ? *state.starfield_screen
                              : get_embedded_starfield_screen();
  const auto &config_overlay = state.config_prompt_overlay.has_value()
                                   ? *state.config_prompt_overlay
                                   : get_embedded_config_prompt_overlay();
  if (can_blit_opaque_image(surface, 0, 0, starfield) &&
      can_blit_opaque_image(surface, 0, 0, config_overlay)) {
    gfx_rle_blit_opaque_4plane(surface, 0, 0, starfield);
    gfx_rle_blit_masked_or_4plane(surface, 0, 0, config_overlay);
    return true;
  }
  return false;
}

bool play_intro_cinematic(EgaPlanarSurface &surface, RuntimeState &state) {
  if (!state.intro_screens.empty()) {
    const std::size_t count = state.intro_screens.size();
    const std::size_t index =
        static_cast<std::size_t>(state.ui.cinematic_frame) % count;
    if (can_blit_opaque_image(surface, 0, 0, state.intro_screens[index])) {
      gfx_rle_blit_opaque_4plane(surface, 0, 0, state.intro_screens[index]);
      state.ui.cinematic_frame =
          static_cast<std::uint8_t>(state.ui.cinematic_frame + 1U);
      return state.ui.cinematic_frame >= count;
    }
  }

  draw_rect(surface, 0, 0, 320, 200, 0x00);
  draw_rect(surface, 40, 50, 240, 100, 0x0F);
  state.ui.cinematic_frame =
      static_cast<std::uint8_t>(state.ui.cinematic_frame + 1U);
  return state.ui.cinematic_frame >= 4U;
}

bool event_finale_transition_sequence(EgaPlanarSurface &surface,
                                      RuntimeState &state) {
  if (state.victory_screen.has_value() &&
      can_blit_opaque_image(surface, 0, 0, *state.victory_screen)) {
    const auto &starfield = state.starfield_screen.has_value()
                                ? *state.starfield_screen
                                : get_embedded_starfield_screen();
    if (can_blit_opaque_image(surface, 0, 0, starfield)) {
      gfx_rle_blit_opaque_4plane(surface, 0, 0, starfield);
      gfx_rle_blit_masked_or_4plane(surface, 0, 0, *state.victory_screen);
    } else {
      gfx_rle_blit_opaque_4plane(surface, 0, 0, *state.victory_screen);
    }
    state.progression.flags =
        static_cast<std::uint8_t>(state.progression.flags | 0x80U);
    state.ui.cinematic_frame =
        static_cast<std::uint8_t>(state.ui.cinematic_frame + 1U);
    return true;
  }

  draw_rect(surface, 0, 0, 320, 200, 0x08);
  draw_rect(surface, 20, 20, 280, 160, 0x0C);
  state.ui.cinematic_frame =
      static_cast<std::uint8_t>(state.ui.cinematic_frame + 1U);
  return true;
}

GameLifecycleState get_game_lifecycle_state(const RuntimeState &state) {
  return state.ui.lifecycle_state;
}

void set_game_lifecycle_state(RuntimeState &state,
                              GameLifecycleState next_state) {
  state.ui.lifecycle_state = next_state;
}

bool can_transition_game_lifecycle(GameLifecycleState current,
                                   GameLifecycleState next) {
  if (current == next) {
    return true;
  }
  switch (current) {
  case GameLifecycleState::Title:
    return next == GameLifecycleState::Intro ||
           next == GameLifecycleState::Menu || next == GameLifecycleState::Quit;
  case GameLifecycleState::Intro:
    return next == GameLifecycleState::Menu ||
           next == GameLifecycleState::Gameplay ||
           next == GameLifecycleState::Title ||
           next == GameLifecycleState::Quit;
  case GameLifecycleState::Menu:
    return next == GameLifecycleState::Gameplay ||
           next == GameLifecycleState::Title ||
           next == GameLifecycleState::Quit;
  case GameLifecycleState::Gameplay:
    return next == GameLifecycleState::Save ||
           next == GameLifecycleState::Victory ||
           next == GameLifecycleState::GameOver ||
           next == GameLifecycleState::Menu || next == GameLifecycleState::Quit;
  case GameLifecycleState::Save:
    return next == GameLifecycleState::Gameplay ||
           next == GameLifecycleState::Victory ||
           next == GameLifecycleState::Menu || next == GameLifecycleState::Quit;
  case GameLifecycleState::Victory:
    return next == GameLifecycleState::Menu ||
           next == GameLifecycleState::Title ||
           next == GameLifecycleState::Quit;
  case GameLifecycleState::GameOver:
    return next == GameLifecycleState::Menu ||
           next == GameLifecycleState::Title ||
           next == GameLifecycleState::Quit;
  case GameLifecycleState::Quit:
    return false;
  }
  return false;
}

bool transition_game_lifecycle(RuntimeState &state,
                               GameLifecycleState next_state) {
  if (!can_transition_game_lifecycle(state.ui.lifecycle_state, next_state)) {
    return false;
  }
  if (state.ui.lifecycle_state == next_state) {
    return true;
  }

  // Handle entry side-effects
  switch (next_state) {
  case GameLifecycleState::Title:
    state.ui.cinematic_frame = 0;
    state.ui.startup_demo_tick = 0;
    state.ui.title_music_started = false;
    break;
  case GameLifecycleState::Intro:
    state.ui.cinematic_frame = 0;
    state.ui.key_rebinding_active = false;
    state.ui.joystick_calibration_active = false;
    state.ui.joystick_calibration_step = 0;
    if (!state.ui.persistence_dir.empty()) {
      keys_load_file(state.key_bindings, state.ui.persistence_dir / "KEYS.DEF");
    }
    queue_audio_event(state, AudioEvent::StartupChime);
    break;
  case GameLifecycleState::Menu:
    state.ui.menu_state = MenuState::GameSelect;
    state.ui.selected_option_index = 0;
    queue_audio_event(state, AudioEvent::MenuTheme);
    if (!state.ui.persistence_dir.empty()) {
      cfg_load_options_file(state, state.ui.persistence_dir / "FR.CFG");
      keys_load_file(state.key_bindings, state.ui.persistence_dir / "KEYS.DEF");
    } else if (!state.assets_root.empty()) {
      cfg_load_options_file(state, state.assets_root / "FR.CFG");
    }
    if (state.ui.player_slot_menu_active) {
      ui_init_player_slot_menu(state);
    } else if (state.ui.option_labels.empty()) {
      state.ui.option_labels = {"NATURE", "MAGIC",  "TIME",
                                "ICE",    "TECHNO", "DREAMS"};
    }
    break;
  case GameLifecycleState::Gameplay:
    state.ui.menu_state = MenuState::None;
    state.ui.player_slot_menu_active = false;
    state.player.is_physics_active = true;
    if (state.room_grid.tile_data.empty() && !state.assets_root.empty()) {
      load_room_tilemap_from_asset_root(
          state, state.assets_root, state.current_level, state.current_room);
    }
    if (state.ui.modal_prompt == "Room Event Triggered" ||
        state.ui.room_event_modal_active) {
      state.ui.modal_active = false;
      state.ui.modal_prompt.clear();
      state.ui.modal_lines.clear();
      state.ui.modal_width_cells = 0;
      state.ui.modal_height_cells = 0;
      state.ui.room_event_modal_active = false;
      state.transition_state.player_frozen = false;
    }
    state.flags.room_event_triggered = false;
    break;
  case GameLifecycleState::Save: {
    // Perform authentic snapshot serialization and award computation
    state.ui.save_option_index = 0U;
    const std::size_t award_slot =
        std::min<std::size_t>(state.ui.active_player_slot, 4U);
    const std::uint16_t earned_awards = static_cast<std::uint16_t>(
        1U + (state.player.boots_equipped ? 1U : 0U) +
        (state.player.lantern_equipped ? 1U : 0U) +
        (state.player.corkscrew_equipped ? 1U : 0U) +
        (state.player.wand_equipped ? 1U : 0U) + state.current_level);
    if (earned_awards > state.dos_config.player_awards[award_slot]) {
      state.dos_config.player_awards[award_slot] = earned_awards;
      state.dos_config.records[4U + award_slot] = earned_awards;
    }
    savegame_write_snapshot(state, state.saved_snapshot_bytes);
    if (!state.ui.persistence_dir.empty()) {
      std::filesystem::create_directories(state.ui.persistence_dir);
      savegame_write_slot_file(state, state.ui.persistence_dir,
                               state.ui.active_player_slot);
      cfg_save_options_file(state, state.ui.persistence_dir / "FR.CFG");
      keys_save_file(state.key_bindings, state.ui.persistence_dir / "KEYS.DEF");
    }
    if (!state.ui.pending_event_message.empty()) {
      state.ui.modal_active = true;
      state.ui.modal_prompt = state.ui.pending_event_message;
      state.ui.pending_event_message.clear();
      state.ui.room_event_modal_active = true;
      state.transition_state.player_frozen = true;
    } else if (!state.ui.modal_active || state.ui.modal_prompt.empty()) {
      state.ui.modal_active = true;
      state.ui.modal_prompt = "Room Event Triggered";
      state.ui.room_event_modal_active = true;
      state.transition_state.player_frozen = true;
    }
    state.flags.room_event_triggered = false;
    break;
  }
  case GameLifecycleState::Victory:
    state.progression.flags =
        static_cast<std::uint8_t>(state.progression.flags | 0x80U);
    state.ui.cinematic_frame = 0;
    queue_audio_event(state, AudioEvent::VictoryFanfare);
    break;
  case GameLifecycleState::GameOver:
    state.ui.modal_active = true;
    state.ui.modal_game_over = true;
    state.ui.game_over = true;
    if (state.ui.modal_prompt.empty()) {
      state.ui.modal_prompt = "Game Over";
    }
    break;
  case GameLifecycleState::Quit:
    break;
  }

  state.ui.lifecycle_state = next_state;
  return true;
}

bool update_game_lifecycle(RuntimeState &state, const InputState &input,
                           const GameDispatcher *dispatcher) {
  switch (state.ui.lifecycle_state) {
  case GameLifecycleState::Title:
    if (input.sdl_keyboard_active) {
      if (input.any_key_pressed) {
        return transition_game_lifecycle(state, GameLifecycleState::Intro);
      }
    } else if (input.any_key_pressed || input.jump_pressed ||
               input.fire_pressed || input.pause_pressed ||
               input.mode_activate_pressed || input.left_pressed ||
               input.right_pressed || input.down_pressed) {
      return transition_game_lifecycle(state, GameLifecycleState::Intro);
    }
    return true;

  case GameLifecycleState::Intro:
    if (state.ui.key_rebinding_active) {
      if (input.last_dos_scancode != 0U) {
        ui_submit_key_rebinding_scancode(state, input.last_dos_scancode);
        if (!state.ui.key_rebinding_active) {
          state.ui.modal_active = false;
          state.ui.modal_prompt.clear();
        }
      } else if (input.pause_pressed) {
        ui_submit_key_rebinding_scancode(state, 0x01U);
      }
      return true;
    }
    if (state.ui.joystick_calibration_active) {
      if (input.any_key_pressed ||
          (!input.sdl_keyboard_active &&
           (input.jump_pressed || input.fire_pressed || input.pause_pressed))) {
        state.ui.joystick_calibration_active = false;
        state.ui.joystick_calibration_step = 0U;
      }
      return true;
    }
    if (input.last_dos_scancode == 0x25U) {
      ui_begin_key_rebinding(state);
      return true;
    }
    if (input.last_dos_scancode == 0x24U) {
      state.ui.joystick_calibration_active = true;
      state.ui.joystick_calibration_step = 0U;
      return true;
    }
    if (input.last_dos_scancode == 0x01U) {
      return transition_game_lifecycle(state, GameLifecycleState::Quit);
    }
    if (state.ui.cinematic_frame >= 4U ||
        (!state.intro_screens.empty() &&
         state.ui.cinematic_frame >= state.intro_screens.size())) {
      return transition_game_lifecycle(state, GameLifecycleState::Menu);
    }
    if (input.sdl_keyboard_active) {
      if (input.any_key_pressed) {
        state.ui.player_slot_menu_active = true;
        return transition_game_lifecycle(state, GameLifecycleState::Menu);
      }
    } else if (input.any_key_pressed || input.pause_pressed ||
               input.jump_pressed || input.fire_pressed ||
               input.mode_activate_pressed) {
      if (state.ui.lifecycle_enabled) {
        state.ui.player_slot_menu_active = true;
      }
      return transition_game_lifecycle(state, GameLifecycleState::Menu);
    }
    return true;

  case GameLifecycleState::Menu:
    if (state.ui.key_rebinding_active) {
      if (input.pause_pressed) {
        ui_submit_key_rebinding_scancode(state, 0x01U);
      }
      return true;
    }
    if (state.ui.player_slot_menu_active) {
      if (state.ui.editing_player_name) {
        if (input.last_dos_scancode == 0x01U) {
          state.ui.editing_player_name = false;
          state.ui.pending_player_name_input.clear();
          ui_init_player_slot_menu(state);
          return true;
        }
        if (input.last_dos_scancode == 0x0EU ||
            input.last_dos_scancode == 0x4BU) {
          if (!state.ui.pending_player_name_input.empty()) {
            state.ui.pending_player_name_input.pop_back();
          }
          return true;
        }
        if (input.last_dos_scancode == 0x1CU) {
          const std::string entered = state.ui.pending_player_name_input;
          state.ui.editing_player_name = false;
          state.ui.pending_player_name_input.clear();
          if (!entered.empty()) {
            ui_enter_new_player(state, entered);
          } else {
            ui_init_player_slot_menu(state);
          }
          return true;
        }
        if (input.last_dos_scancode != 0U &&
            state.ui.pending_player_name_input.size() < 15U) {
          char ch = '\0';
          if (input.last_dos_scancode == 0x39U) {
            ch = ' ';
          } else {
            const auto key_name = dos_scancode_name(input.last_dos_scancode);
            if (key_name.size() == 1U) {
              const char c = key_name[0];
              if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
                ch = c;
              }
            }
          }
          if (ch != '\0') {
            state.ui.pending_player_name_input.push_back(ch);
          }
        }
        return true;
      }

      if (input.sdl_keyboard_active && input.last_dos_scancode == 0x01U) {
        if (state.ui.retire_player_mode) {
          state.ui.retire_player_mode = false;
          ui_init_player_slot_menu(state);
          return true;
        }
        return transition_game_lifecycle(state, GameLifecycleState::Quit);
      }

      const bool move_prev =
          input.sdl_keyboard_active
              ? (input.last_dos_scancode == 0x48U ||
                 input.last_dos_scancode == 0x4BU ||
                 input.last_dos_scancode == state.key_bindings.up ||
                 input.last_dos_scancode == state.key_bindings.left)
              : input.left_pressed;
      const bool move_next =
          input.sdl_keyboard_active
              ? (input.last_dos_scancode == 0x50U ||
                 input.last_dos_scancode == 0x4DU ||
                 input.last_dos_scancode == state.key_bindings.down ||
                 input.last_dos_scancode == state.key_bindings.right)
              : (input.right_pressed || input.down_pressed);
      const bool confirm_pressed =
          input.sdl_keyboard_active
              ? (input.last_dos_scancode == 0x1CU ||
                 input.last_dos_scancode == 0x39U ||
                 input.last_dos_scancode == 0x52U ||
                 input.last_dos_scancode == state.key_bindings.jump ||
                 input.last_dos_scancode == state.key_bindings.fire)
              : (input.jump_pressed || input.fire_pressed);

      if (move_prev) {
        ui_select_prev_valid_option(state);
      }
      if (move_next) {
        ui_select_next_valid_option(state);
      }
      if (confirm_pressed) {
        if (state.ui.retire_player_mode) {
          if (state.ui.selected_option_index < 5U) {
            ui_retire_player(state, state.ui.selected_option_index);
          } else {
            state.ui.retire_player_mode = false;
            ui_init_player_slot_menu(state);
          }
          return true;
        }
        if (state.ui.selected_option_index == 6U) {
          if (input.sdl_keyboard_active &&
              state.ui.pending_player_name_input.empty()) {
            std::size_t target_slot = 5U;
            for (std::size_t i = 0; i < 5U; ++i) {
              if (state.dos_config.player_names[i].empty()) {
                target_slot = i;
                break;
              }
            }
            if (target_slot < 5U) {
              state.ui.editing_player_name = true;
              state.ui.pending_player_name_input.clear();
              state.ui.option_labels[6] = "ENTER YOUR NAME.";
              state.ui.option_labels[7] = "HIT (ESC) TO ABORT";
              state.ui.selected_option_index = target_slot;
            }
            return true;
          }
          const std::string name = state.ui.pending_player_name_input.empty()
                                       ? "CAPTAIN"
                                       : state.ui.pending_player_name_input;
          ui_enter_new_player(state, name);
          return true;
        }
        if (state.ui.selected_option_index == 7U) {
          state.ui.retire_player_mode = true;
          ui_init_player_slot_menu(state);
          return true;
        }
        if (state.ui.selected_option_index < 5U &&
            !state.dos_config.player_names[state.ui.selected_option_index]
                 .empty()) {
          state.ui.active_player_slot =
              static_cast<std::uint8_t>(state.ui.selected_option_index + 1U);
          state.ui.player_slot_menu_active = false;
          state.ui.menu_state = MenuState::GameSelect;
          state.ui.option_labels = {"NATURE", "MAGIC",  "TIME",
                                    "ICE",    "TECHNO", "DREAMS"};
          bool loaded_save = false;
          if (!state.ui.persistence_dir.empty()) {
            loaded_save = savegame_read_slot_file(
                state, state.ui.persistence_dir, state.ui.active_player_slot);
          } else if (!state.assets_root.empty()) {
            loaded_save = savegame_read_slot_file(state, state.assets_root,
                                                  state.ui.active_player_slot);
          }
          if (!loaded_save) {
            state.current_level = 0U;
            state.current_room = 0U;
          }
          if (!state.assets_root.empty()) {
            load_room_tilemap_from_asset_root(state, state.assets_root,
                                              state.current_level,
                                              state.current_room);
          }
          return transition_game_lifecycle(state, GameLifecycleState::Gameplay);
        }
      }
      return true;
    }

    if (input.left_pressed) {
      ui_select_prev_valid_option(state);
    }
    if (input.right_pressed) {
      ui_select_next_valid_option(state);
    }
    if (input.jump_pressed || input.fire_pressed) {
      ui_select_world_or_save_slot(state, state.ui.selected_option_index);
      return transition_game_lifecycle(state, GameLifecycleState::Gameplay);
    }
    return true;

  case GameLifecycleState::Gameplay: {
    auto should_transition_to_save = [](const RuntimeState &s) {
      return s.flags.room_event_triggered || s.ui.room_event_modal_active ||
             s.ui.pending_event_message == "Room Event Triggered" ||
             (s.ui.modal_active && s.ui.modal_prompt == "Room Event Triggered");
    };

    if (should_transition_to_save(state)) {
      return transition_game_lifecycle(state, GameLifecycleState::Save);
    }
    if (state.boss_victory || (state.progression.flags & 0x80U) != 0U) {
      return transition_game_lifecycle(state, GameLifecycleState::Victory);
    }
    if (state.player.hp == 0 && state.player.lives == 0) {
      return transition_game_lifecycle(state, GameLifecycleState::GameOver);
    }
    if (dispatcher != nullptr) {
      state.input = input;
      dispatcher->run_tick(state);
    }
    if (should_transition_to_save(state)) {
      return transition_game_lifecycle(state, GameLifecycleState::Save);
    }
    if (state.boss_victory || (state.progression.flags & 0x80U) != 0U) {
      return transition_game_lifecycle(state, GameLifecycleState::Victory);
    }
    if (state.player.hp == 0 && state.player.lives == 0) {
      return transition_game_lifecycle(state, GameLifecycleState::GameOver);
    }
    return true;
  }

  case GameLifecycleState::Save:
    if (input.left_pressed || input.right_pressed || input.down_pressed) {
      state.ui.save_option_index = (state.ui.save_option_index == 0U) ? 1U : 0U;
    }
    if (input.jump_pressed || input.fire_pressed || input.pause_pressed) {
      if (state.ui.save_option_index == 0U) {
        savegame_write_snapshot(state, state.saved_snapshot_bytes);
        if (!state.ui.persistence_dir.empty()) {
          std::filesystem::create_directories(state.ui.persistence_dir);
          savegame_write_slot_file(state, state.ui.persistence_dir,
                                   state.ui.active_player_slot);
          cfg_save_options_file(state, state.ui.persistence_dir / "FR.CFG");
          keys_save_file(state.key_bindings,
                         state.ui.persistence_dir / "KEYS.DEF");
        }
      }
      if (state.boss_victory || (state.progression.flags & 0x80U) != 0U) {
        return transition_game_lifecycle(state, GameLifecycleState::Victory);
      }
      return transition_game_lifecycle(state, GameLifecycleState::Gameplay);
    }
    return true;

  case GameLifecycleState::Victory:
    if (input.jump_pressed || input.fire_pressed || input.pause_pressed) {
      return transition_game_lifecycle(state, GameLifecycleState::Menu);
    }
    return true;

  case GameLifecycleState::GameOver:
    if (input.jump_pressed || input.fire_pressed || input.pause_pressed) {
      return transition_game_lifecycle(state, GameLifecycleState::Menu);
    }
    return true;

  case GameLifecycleState::Quit:
    return false;
  }

  return true;
}

void render_game_lifecycle_frame(EgaPlanarSurface &surface,
                                 RuntimeState &state) {
  switch (state.ui.lifecycle_state) {
  case GameLifecycleState::Title:
    render_startup_demo_and_backstory(surface, state);
    break;
  case GameLifecycleState::Intro:
    render_title_config_options_screen(surface, state);
    break;
  case GameLifecycleState::Menu:
    load_ega_palette_table(state, make_default_ega_palette());
    ui_render_game_selection_panel(surface, state);
    break;
  case GameLifecycleState::Gameplay:
    render_bootstrap_surface(surface, state);
    break;
  case GameLifecycleState::Save:
    render_bootstrap_surface(surface, state);
    ui_render_modal_prompt(surface, state);
    break;
  case GameLifecycleState::Victory:
    load_ega_palette_table(state, make_ega_palette_ds_0024());
    event_finale_transition_sequence(surface, state);
    break;
  case GameLifecycleState::GameOver:
    render_bootstrap_surface(surface, state);
    ui_render_modal_prompt(surface, state);
    break;
  case GameLifecycleState::Quit:
    surface.clear(0x00);
    break;
  }
}

} // namespace comic2
