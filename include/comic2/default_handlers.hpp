#pragma once

#include "comic2/dispatcher.hpp"

namespace comic2 {

void handle_level_transition(RuntimeState &state);
bool handle_level_completion_transition(RuntimeState &state);
void handle_special_logic1(RuntimeState &state);
void handle_special_logic2(RuntimeState &state);
void handle_airborne_physics(RuntimeState &state);
void handle_timed_overlay(RuntimeState &state);
void handle_grounded_physics(RuntimeState &state);
void handle_player_animation(RuntimeState &state);
void handle_attack_animation(RuntimeState &state);
void handle_distance_interaction(RuntimeState &state);
bool check_comic_near_room_event_anchor(const RuntimeState &state);
void update_room_event_anchor_motion(RuntimeState &state);
void update_room_event_anchor_sprite(RuntimeState &state);
std::optional<std::size_t>
resolve_interaction_script_message_index(RuntimeState &state,
                                         std::uint16_t descriptor_ptr);
bool ent_process_interaction_script_and_queue_message(
    RuntimeState &state, std::uint16_t descriptor_ptr);
void handle_tile_hazard(RuntimeState &state);
void handle_player_special_state(RuntimeState &state);
void reset_player_respawn_state(RuntimeState &state);
void update_player_mode_cycle(RuntimeState &state);
void update_player_mode_activation(RuntimeState &state);
void update_player_mode_effect(RuntimeState &state);
void update_progression_state(RuntimeState &state);
void handle_input_fallback(RuntimeState &state);

void install_default_stage_hooks(GameDispatcher &dispatcher);
GameDispatcher make_default_game_dispatcher();

} // namespace comic2
