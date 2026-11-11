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

class GameDispatcher;

GameLifecycleState get_game_lifecycle_state(const RuntimeState &state);
void set_game_lifecycle_state(RuntimeState &state,
                              GameLifecycleState next_state);
bool can_transition_game_lifecycle(GameLifecycleState current,
                                   GameLifecycleState next);
bool transition_game_lifecycle(RuntimeState &state,
                               GameLifecycleState next_state);
bool update_game_lifecycle(RuntimeState &state, const InputState &input,
                           const GameDispatcher *dispatcher = nullptr);
void render_game_lifecycle_frame(EgaPlanarSurface &surface,
                                 RuntimeState &state);

} // namespace comic2
