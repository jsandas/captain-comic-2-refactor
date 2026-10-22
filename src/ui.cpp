#include "comic2/ui.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <utility>

namespace comic2 {
namespace {

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

bool should_draw_hud_overlay(const RuntimeState &state) {
  return state.ui.menu_state != MenuState::None || state.ui.modal_active ||
         state.player.score != 0 || state.player.gems != 0 ||
         state.player.lives != 3 || state.ui.active_mode_mask != 0 ||
         state.ui.inventory_mask != 0 ||
         state.progression.mode_inventory_mask != 0 ||
         state.ui.cinematic_frame != 0;
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

void ui_render_modal_prompt(EgaPlanarSurface &surface,
                            const RuntimeState &state) {
  if (!state.ui.modal_active || state.ui.modal_prompt.empty()) {
    return;
  }

  draw_rect(surface, 48, 72, 224, 56, 0x04);
  draw_rect(surface, 56, 80, 208, 40, 0x0F);
  draw_rect(surface, 64, 92, 192, 8, 0x0E);

  if (state.ui.modal_game_over) {
    draw_rect(surface, 64, 104, 192, 8, 0x0C);
  } else {
    draw_rect(surface, 64, 104, 192, 8, 0x0E);
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

    // Shield energy bar (8 pips at y=0x5E=94)
    {
      const auto pips = std::min<std::uint8_t>(8U, state.player.shield_power);
      for (std::uint8_t i = 0; i < 8U; ++i) {
        const std::uint8_t color = (i < pips) ? 0x0AU : 0x08U;
        draw_rect(surface, static_cast<std::size_t>(kHudX + i * 4), 0x5EU, 3U,
                  6U, color);
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
  if ((state.ui.inventory_mask & 0x01U) != 0U) {
    draw_rect(surface, 152, 188, 8, 8, 0x0E);
  }
  if ((state.ui.inventory_mask & 0x02U) != 0U) {
    draw_rect(surface, 160, 188, 8, 8, 0x0C);
  }
  if (((state.ui.inventory_mask & 0x04U) != 0U) ||
      state.progression.mode_inventory_mask != 0U) {
    draw_rect(surface, 160, 188, 8, 8, 0x0C);
  }
}

void ui_render_option_list(EgaPlanarSurface &surface,
                           const RuntimeState &state) {
  draw_rect(surface, 40, 20, 240, 120, 0x04);
  for (std::size_t i = 0; i < state.ui.option_labels.size(); ++i) {
    const bool is_selected = i == state.ui.selected_option_index;
    const std::uint8_t color = is_selected ? 0x0F : 0x07;
    draw_rect(surface, 56, 36 + static_cast<std::size_t>(i) * 20, 180, 10,
              color);
  }
}

// cppcheck-suppress unusedFunction ; reserved for Phase 10
void ui_render_game_selection_panel(EgaPlanarSurface &surface,
                                    const RuntimeState &state) {
  draw_rect(surface, 20, 20, 280, 140, 0x02);
  draw_rect(surface, 28, 32, 264, 12, 0x0F);
  if (state.ui.menu_state == MenuState::GameSelect) {
    draw_rect(surface, 80, 70, 160, 8, 0x0E);
  }
}

// cppcheck-suppress unusedFunction ; reserved for Phase 10
bool ui_show_modal_prompt_wait_key(RuntimeState &state,
                                   const std::string &prompt,
                                   std::uint8_t awaited_key) {
  state.ui.modal_prompt = prompt;
  state.ui.awaited_key = awaited_key;
  state.ui.modal_active = true;
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

  if (input.left_pressed || input.right_pressed) {
    ui_select_next_valid_option(state);
  }
}

// cppcheck-suppress unusedFunction ; reserved for Phase 10
void ui_edit_selected_option_text(RuntimeState &state,
                                  const std::string &text) {
  if (state.ui.selected_option_index < state.ui.option_labels.size()) {
    state.ui.option_labels[state.ui.selected_option_index] = text;
  }
}

void ui_select_next_valid_option(RuntimeState &state) {
  if (state.ui.option_labels.empty()) {
    return;
  }

  const std::size_t count = state.ui.option_labels.size();
  std::size_t next = state.ui.selected_option_index + 1U;
  if (next >= count) {
    next = 0U;
  }
  state.ui.selected_option_index = next;
}

// cppcheck-suppress unusedFunction ; reserved for Phase 10
bool ui_select_prev_valid_option(RuntimeState &state) {
  if (state.ui.option_labels.empty()) {
    return false;
  }

  const std::size_t count = state.ui.option_labels.size();
  std::size_t prev = state.ui.selected_option_index;
  if (prev == 0U) {
    prev = count - 1U;
  } else {
    prev -= 1U;
  }
  state.ui.selected_option_index = prev;
  return true;
}

// cppcheck-suppress unusedFunction ; reserved for Phase 10
bool play_intro_cinematic(EgaPlanarSurface &surface, RuntimeState &state) {
  draw_rect(surface, 0, 0, 320, 200, 0x00);
  draw_rect(surface, 40, 50, 240, 100, 0x0F);
  state.ui.cinematic_frame =
      static_cast<std::uint8_t>(state.ui.cinematic_frame + 1U);
  return true;
}

bool event_finale_transition_sequence(EgaPlanarSurface &surface,
                                      RuntimeState &state) {
  draw_rect(surface, 0, 0, 320, 200, 0x08);
  draw_rect(surface, 20, 20, 280, 160, 0x0C);
  state.ui.cinematic_frame =
      static_cast<std::uint8_t>(state.ui.cinematic_frame + 1U);
  return true;
}

} // namespace comic2
