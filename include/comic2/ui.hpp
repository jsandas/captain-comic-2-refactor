#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "comic2/game_state.hpp"
#include "comic2/renderer.hpp"

namespace comic2 {

std::uint8_t bcd_increment(std::uint8_t value);
std::uint8_t bcd_decrement(std::uint8_t value);

void hud_render_overlay(EgaPlanarSurface &surface, const RuntimeState &state);
void ui_render_modal_prompt(EgaPlanarSurface &surface,
                            const RuntimeState &state);
void ui_render_option_list(EgaPlanarSurface &surface,
                           const RuntimeState &state);
void ui_render_game_selection_panel(EgaPlanarSurface &surface,
                                    const RuntimeState &state);

bool ui_show_modal_prompt_wait_key(RuntimeState &state,
                                   const std::string &prompt,
                                   std::uint8_t awaited_key);
void ui_option_list_input_loop(RuntimeState &state, const InputState &input);
void ui_edit_selected_option_text(RuntimeState &state, const std::string &text);
void ui_select_next_valid_option(RuntimeState &state);
bool ui_select_prev_valid_option(RuntimeState &state);

bool render_title_screen(EgaPlanarSurface &surface, const RuntimeState &state);
bool play_intro_cinematic(EgaPlanarSurface &surface, RuntimeState &state);
bool event_finale_transition_sequence(EgaPlanarSurface &surface,
                                      RuntimeState &state);

} // namespace comic2
