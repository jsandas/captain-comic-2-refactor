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
void hud_update_mode_icons(EgaPlanarSurface &surface,
                           const RuntimeState &state);
void ui_draw_framed_text_box_and_text(EgaPlanarSurface &surface,
                                      const std::vector<std::string> &lines,
                                      std::uint8_t width_cells = 0,
                                      std::uint8_t height_cells = 0,
                                      std::uint8_t border_color = 0x09U,
                                      std::uint8_t text_color = 0x07U);
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
bool ui_is_option_selectable(const RuntimeState &state,
                             std::size_t option_index);
void ui_select_next_valid_option(RuntimeState &state);
bool ui_select_prev_valid_option(RuntimeState &state);

void ui_init_player_slot_menu(RuntimeState &state);
bool ui_enter_new_player(RuntimeState &state, const std::string &player_name);
bool ui_retire_player(RuntimeState &state, std::size_t slot_index);
std::uint16_t world_selection_to_level(std::size_t world_index);
bool ui_select_world_or_save_slot(RuntimeState &state, std::size_t slot_index);

void ui_begin_key_rebinding(RuntimeState &state);
bool ui_submit_key_rebinding_scancode(RuntimeState &state,
                                      std::uint8_t dos_scancode);

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
