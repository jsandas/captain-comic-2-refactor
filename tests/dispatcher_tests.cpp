#include <algorithm>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <vector>

#include "comic2/default_handlers.hpp"
#include "comic2/dispatcher.hpp"
#include "comic2/game_state.hpp"

namespace {

void expect(bool condition, const char *message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

std::vector<std::uint8_t>
encode_literal_signed_rle(const std::vector<std::uint8_t> &bytes) {
  std::vector<std::uint8_t> encoded;
  std::size_t offset = 0;
  while (offset < bytes.size()) {
    const std::size_t chunk = std::min<std::size_t>(127, bytes.size() - offset);
    encoded.push_back(static_cast<std::uint8_t>(chunk));
    encoded.insert(encoded.end(),
                   bytes.begin() + static_cast<std::ptrdiff_t>(offset),
                   bytes.begin() + static_cast<std::ptrdiff_t>(offset + chunk));
    offset += chunk;
  }
  encoded.push_back(0x00);
  return encoded;
}

std::vector<std::uint8_t>
make_room_resource_bytes(std::uint16_t level, std::uint16_t room,
                         std::uint16_t tile_w, std::uint16_t tile_h,
                         std::uint16_t payload_offset,
                         const std::vector<std::uint8_t> &decoded_room_bytes) {
  const std::vector<std::uint8_t> encoded_room_bytes =
      encode_literal_signed_rle(decoded_room_bytes);

  const std::size_t entry_offset = 0x04 + static_cast<std::size_t>(room) * 6;
  const std::size_t min_header_size = entry_offset + 6;
  const std::size_t min_payload_start =
      std::max<std::size_t>(min_header_size, payload_offset);

  std::vector<std::uint8_t> bytes(min_payload_start + encoded_room_bytes.size(),
                                  0x00);

  bytes[2] = static_cast<std::uint8_t>(level & 0xFF);
  bytes[3] = static_cast<std::uint8_t>((level >> 8) & 0xFF);

  bytes[entry_offset + 0] = static_cast<std::uint8_t>(tile_w & 0xFF);
  bytes[entry_offset + 1] = static_cast<std::uint8_t>((tile_w >> 8) & 0xFF);
  bytes[entry_offset + 2] = static_cast<std::uint8_t>(tile_h & 0xFF);
  bytes[entry_offset + 3] = static_cast<std::uint8_t>((tile_h >> 8) & 0xFF);
  bytes[entry_offset + 4] = static_cast<std::uint8_t>(payload_offset & 0xFF);
  bytes[entry_offset + 5] =
      static_cast<std::uint8_t>((payload_offset >> 8) & 0xFF);

  std::copy(encoded_room_bytes.begin(), encoded_room_bytes.end(),
            bytes.begin() + payload_offset);

  return bytes;
}

std::vector<std::uint8_t> make_decoded_room_bytes(std::uint8_t first_tile,
                                                  std::uint16_t row0,
                                                  std::uint16_t row1,
                                                  std::uint16_t row2) {
  std::vector<std::uint8_t> decoded(0x2C4, 0x00);
  decoded[0] = first_tile;
  decoded[0x2A0] = static_cast<std::uint8_t>(row0 & 0xFF);
  decoded[0x2A1] = static_cast<std::uint8_t>((row0 >> 8) & 0xFF);
  decoded[0x2A2] = static_cast<std::uint8_t>(row1 & 0xFF);
  decoded[0x2A3] = static_cast<std::uint8_t>((row1 >> 8) & 0xFF);
  decoded[0x2A4] = static_cast<std::uint8_t>(row2 & 0xFF);
  decoded[0x2A5] = static_cast<std::uint8_t>((row2 >> 8) & 0xFF);
  return decoded;
}

void test_priority_order() {
  comic2::RuntimeState state;
  comic2::GameDispatcher dispatcher;

  state.flags.level_transition_pending = true;
  state.player.is_airborne = true;
  state.player.is_physics_active = true;
  expect(dispatcher.choose_stage(state) ==
             comic2::DispatchStage::LevelTransition,
         "level transition must have highest priority");

  state.flags.level_transition_pending = false;
  expect(dispatcher.choose_stage(state) ==
             comic2::DispatchStage::AirbornePhysics,
         "airborne should beat grounded physics");

  state.player.is_airborne = false;
  expect(dispatcher.choose_stage(state) ==
             comic2::DispatchStage::GroundedPhysics,
         "grounded physics should trigger when active");
}

void test_input_fallback() {
  comic2::RuntimeState state;
  comic2::GameDispatcher dispatcher;

  expect(dispatcher.choose_stage(state) == comic2::DispatchStage::InputHandling,
         "dispatcher must fall back to input handling");
}

void test_stage_hook_execution() {
  comic2::RuntimeState state;
  comic2::GameDispatcher dispatcher;

  dispatcher.set_input_handling_hook(
      [](comic2::RuntimeState &s) { s.player.jump_counter = 7; });

  const auto result = dispatcher.run_tick(state);
  expect(result.stage == comic2::DispatchStage::InputHandling,
         "run_tick should select input stage");
  expect(result.hook_executed, "run_tick should report hook execution");
  expect(state.player.jump_counter == 7, "input hook should mutate state");
}

void test_default_handlers_basic_movement_and_jump() {
  comic2::RuntimeState state;
  comic2::GameDispatcher dispatcher;
  comic2::install_default_stage_hooks(dispatcher);

  state.player.jump_counter = 2;
  state.input.right_pressed = true;
  state.input.jump_pressed = true;

  const auto first = dispatcher.run_tick(state);
  expect(first.stage == comic2::DispatchStage::InputHandling,
         "first tick should handle input");
  expect(state.player.x == 8, "right input should advance x");
  expect(state.player.y_vel == -5, "jump should apply impulse");
  expect(state.player.is_physics_active, "jump should enable physics");

  state.input.right_pressed = false;
  state.input.jump_pressed = false;

  const auto second = dispatcher.run_tick(state);
  expect(second.stage == comic2::DispatchStage::GroundedPhysics ||
             second.stage == comic2::DispatchStage::AirbornePhysics,
         "second tick should execute physics");
  expect(state.player.y < 0, "physics step should move player upward first");
}

void test_deterministic_tick_replay() {
  auto run_sequence = [](const std::vector<comic2::InputState> &inputs) {
    // Create fresh dispatcher and state for each run
    comic2::GameDispatcher dispatcher;
    comic2::install_default_stage_hooks(dispatcher);

    comic2::RuntimeState state;
    state.player.jump_counter = 3;
    for (const auto &input : inputs) {
      state.input = input;
      dispatcher.run_tick(state);
    }
    return state;
  };

  const std::vector<comic2::InputState> sequence = {
      comic2::InputState{.jump_pressed = true,
                         .left_pressed = false,
                         .right_pressed = true,
                         .down_pressed = false},
      comic2::InputState{},
      comic2::InputState{},
      comic2::InputState{.jump_pressed = false,
                         .left_pressed = true,
                         .right_pressed = false,
                         .down_pressed = false},
      comic2::InputState{},
      comic2::InputState{},
  };

  // Run the sequence and verify specific expected outcomes
  const auto result = run_sequence(sequence);
  expect(result.player.x != 0,
         "player should have moved horizontally after input sequence");
  expect(result.player.y != 0,
         "player should have moved vertically after jump and physics");

  // Verify determinism: same inputs with fresh dispatcher produce identical
  // state
  const auto replay = run_sequence(sequence);
  expect(result.player.x == replay.player.x, "x mismatch");
  expect(result.player.y == replay.player.y, "y mismatch");
  expect(result.player.x_vel == replay.player.x_vel, "x_vel mismatch");
  expect(result.player.y_vel == replay.player.y_vel, "y_vel mismatch");
}

void test_dispatcher_trace_log() {
  comic2::GameDispatcher dispatcher;
  comic2::RuntimeState state;

  dispatcher.set_trace_enabled(true);

  state.flags.level_transition_pending = true;
  dispatcher.run_tick(state);

  state.flags.level_transition_pending = false;
  state.player.is_physics_active = true;
  dispatcher.run_tick(state);

  state.player.is_physics_active = false;
  dispatcher.run_tick(state);

  const std::vector<comic2::DispatchStage> expected = {
      comic2::DispatchStage::LevelTransition,
      comic2::DispatchStage::GroundedPhysics,
      comic2::DispatchStage::InputHandling,
  };
  expect(dispatcher.trace_log() == expected,
         "trace log should record selected stage per tick");

  dispatcher.clear_trace();
  expect(dispatcher.trace_log().empty(),
         "clear_trace should empty recorded stages");
}

void test_default_stage_hook_coverage() {
  comic2::GameDispatcher dispatcher;
  comic2::install_default_stage_hooks(dispatcher);

  const auto expect_stage = [&](comic2::RuntimeState &state,
                                comic2::DispatchStage expected_stage) {
    const auto result = dispatcher.run_tick(state);
    expect(result.stage == expected_stage,
           "stage selection mismatch while checking default hook coverage");
    expect(result.hook_executed,
           "default hook should be installed for every dispatch stage");
  };

  {
    comic2::RuntimeState state;
    state.flags.level_transition_pending = true;
    expect_stage(state, comic2::DispatchStage::LevelTransition);
  }
  {
    comic2::RuntimeState state;
    state.flags.special_logic1_active = true;
    expect_stage(state, comic2::DispatchStage::SpecialLogic1);
  }
  {
    comic2::RuntimeState state;
    state.flags.special_logic2_active = true;
    expect_stage(state, comic2::DispatchStage::SpecialLogic2);
  }
  {
    comic2::RuntimeState state;
    state.player.is_airborne = true;
    expect_stage(state, comic2::DispatchStage::AirbornePhysics);
  }
  {
    comic2::RuntimeState state;
    state.flags.timed_overlay_pending = true;
    expect_stage(state, comic2::DispatchStage::TimedOverlay);
  }
  {
    comic2::RuntimeState state;
    state.player.is_physics_active = true;
    expect_stage(state, comic2::DispatchStage::GroundedPhysics);
  }
  {
    comic2::RuntimeState state;
    state.player.is_animation_active = true;
    expect_stage(state, comic2::DispatchStage::PlayerAnimation);
  }
  {
    comic2::RuntimeState state;
    state.player.is_attack_active = true;
    expect_stage(state, comic2::DispatchStage::AttackAnimation);
  }
  {
    comic2::RuntimeState state;
    state.flags.distance_interaction_active = true;
    expect_stage(state, comic2::DispatchStage::DistanceInteraction);
  }
  {
    comic2::RuntimeState state;
    state.flags.tile_hazard_triggered = true;
    expect_stage(state, comic2::DispatchStage::TileHazard);
  }
  {
    comic2::RuntimeState state;
    state.flags.player_special_state_active = true;
    expect_stage(state, comic2::DispatchStage::PlayerSpecialState);
  }
  {
    comic2::RuntimeState state;
    expect_stage(state, comic2::DispatchStage::InputHandling);
  }
}

void test_player_animation_handler_advances_walk_cycle() {
  comic2::RuntimeState state;
  state.player.is_animation_active = true;
  state.player.animation_state =
      static_cast<std::uint8_t>(comic2::PlayerAnimationState::WalkCycle);
  state.player.animation_frame = 1;
  state.player.animation_ticks = 0;
  state.player.is_airborne = false;
  state.input.right_pressed = true;

  comic2::handle_player_animation(state);

  expect(state.player.animation_state ==
             static_cast<std::uint8_t>(comic2::PlayerAnimationState::WalkCycle),
         "walk cycle should remain the active animation state");
  expect(state.player.animation_frame == 2,
         "walk cycle should advance the frame counter");
  expect(state.player.animation_ticks == 1,
         "walk cycle should advance tick progress");
}

void test_attack_handler_uses_attack_overlay_state() {
  comic2::RuntimeState state;
  state.player.is_attack_active = true;
  state.player.animation_state =
      static_cast<std::uint8_t>(comic2::PlayerAnimationState::Idle);
  state.player.animation_frame = 0;
  state.player.attack_overlay_ticks = 2;

  comic2::handle_attack_animation(state);

  expect(state.player.animation_state ==
             static_cast<std::uint8_t>(comic2::PlayerAnimationState::Attack),
         "attack animation should switch to attack state");
  expect(state.player.attack_overlay_ticks == 1,
         "attack overlay timer should count down");
}

void test_timed_overlay_handler_starts_and_counts_down() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.flags.timed_overlay_pending = true;
  state.player.overlay_sprite_frame = 3;

  comic2::handle_timed_overlay(state);

  expect(!state.flags.timed_overlay_pending,
         "timed overlay flag should be consumed when overlay starts");
  expect(state.player.overlay_active,
         "timed overlay should activate when pending is set");
  expect(state.player.overlay_ticks == 30,
         "timed overlay should start with a fixed duration");
  expect(state.player.overlay_sprite_frame == 3,
         "timed overlay should preserve the configured sprite frame");

  for (int i = 0; i < 29; ++i) {
    comic2::handle_timed_overlay(state);
  }

  expect(state.player.overlay_active,
         "timed overlay should remain active until the countdown expires");
  expect(state.player.overlay_ticks == 1,
         "timed overlay should count down each handler tick");

  comic2::handle_timed_overlay(state);

  expect(!state.player.overlay_active,
         "timed overlay should clear once its countdown reaches zero");
  expect(state.player.overlay_ticks == 0,
         "timed overlay should finish with a cleared countdown value");
}

void test_death_flow_prompts_when_lives_are_exhausted() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.player.hp = 0;
  state.player.lives = 1;
  state.flags.player_special_state_active = true;

  comic2::handle_tile_hazard(state);
  expect(state.flags.player_special_state_active,
         "fatal damage should enter the death special-state flow");
  expect(state.player.animation_state ==
             static_cast<std::uint8_t>(comic2::PlayerAnimationState::Death),
         "fatal damage should switch the player into death animation");

  comic2::handle_player_special_state(state);
  expect(state.player.death_timer_ticks == 2,
         "death flow should decrement the countdown each tick");
  expect(!state.ui.modal_active,
         "death flow should wait for the countdown before opening a modal");

  comic2::handle_player_special_state(state);
  comic2::handle_player_special_state(state);
  expect(state.ui.modal_active,
         "death flow should open a modal prompt once the countdown completes");
  expect(state.ui.modal_game_over, "death flow should enter the game-over "
                                   "modal when the final life is exhausted");
}

void test_death_countdown_decrements_lives_and_respawns_at_spawn() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.player.hp = 0;
  state.player.lives = 2;
  state.player.x = 42;
  state.player.y = 18;
  state.player.x_vel = 4;
  state.player.y_vel = -3;
  state.player.is_airborne = true;
  state.player.facing_right = false;
  state.flags.player_special_state_active = true;

  comic2::handle_tile_hazard(state);
  comic2::handle_player_special_state(state);
  comic2::handle_player_special_state(state);
  comic2::handle_player_special_state(state);

  expect(state.player.lives == 1,
         "death countdown should decrement lives once when it completes");
  expect(state.ui.modal_active,
         "death countdown should open the continue prompt while lives remain");
  expect(state.ui.modal_prompt == "Continue?",
         "death countdown should present the continue prompt when respawn is "
         "available");

  state.ui.modal_confirmed = true;
  comic2::handle_player_special_state(state);

  expect(
      state.player.x == 160,
      "confirming continue should respawn the player at the default spawn x");
  expect(
      state.player.y == 160,
      "confirming continue should respawn the player at the default spawn y");
  expect(state.player.x_vel == 0,
         "confirming continue should clear horizontal velocity on respawn");
  expect(state.player.y_vel == 0,
         "confirming continue should clear vertical velocity on respawn");
  expect(!state.player.is_airborne,
         "confirming continue should clear airborne state on respawn");
}

void test_confirming_continue_respawns_player() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.player.hp = 0;
  state.player.lives = 2;
  state.ui.modal_active = true;
  state.ui.modal_prompt = "Continue?";
  state.ui.modal_game_over = false;
  state.player.animation_state =
      static_cast<std::uint8_t>(comic2::PlayerAnimationState::Death);

  state.ui.modal_confirmed = true;
  comic2::handle_player_special_state(state);

  expect(!state.ui.modal_active,
         "confirming continue should dismiss the modal and resume play");
  expect(state.player.hp == 12,
         "confirming continue should restore player health");
  expect(state.player.animation_state ==
             static_cast<std::uint8_t>(comic2::PlayerAnimationState::Idle),
         "confirming continue should restore the idle animation state");
  expect(!state.player.is_animation_active,
         "confirming continue should clear the death animation flag");
  expect(state.player.death_timer_ticks == 0,
         "confirming continue should clear the death countdown");
  expect(state.player.invuln_ticks == 0,
         "confirming continue should clear invulnerability state");
  expect(state.player.damage_recoil_ticks == 0,
         "confirming continue should clear recoil state");
  expect(!state.player.is_attack_active,
         "confirming continue should clear the attack overlay state");
}

void test_game_over_confirm_restarts_runtime_state() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.player.lives = 0;
  state.player.score = 125;
  state.player.gems = 3;
  state.player.firepower = 2;
  state.player.x = 77;
  state.player.y = 91;
  state.player.is_airborne = true;
  state.player.y_vel = -3;
  state.ui.modal_active = true;
  state.ui.modal_prompt = "Game Over";
  state.ui.modal_game_over = true;
  state.ui.game_over = true;
  state.flags.player_special_state_active = true;

  state.ui.modal_confirmed = true;
  comic2::handle_player_special_state(state);

  expect(!state.ui.modal_active,
         "confirming game over should clear the modal and begin restart");
  expect(!state.ui.game_over,
         "game-over confirmation should clear the terminal game-over state");
  expect(state.player.lives == 3,
         "game-over confirmation should restore the default life count");
  expect(state.player.score == 0,
         "game-over confirmation should reset the score for a fresh run");
  expect(state.player.gems == 0,
         "game-over confirmation should reset collected gems for a fresh run");
  expect(
      state.player.x == 64,
      "game-over confirmation should reset the player to the default spawn x");
  expect(
      state.player.y == 96,
      "game-over confirmation should reset the player to the default spawn y");
  expect(!state.flags.player_special_state_active,
         "game-over confirmation should exit the special state flow");
}

void test_game_over_input_restarts_without_modal_confirm() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.player.lives = 0;
  state.ui.modal_active = true;
  state.ui.modal_prompt = "Game Over";
  state.ui.modal_game_over = true;
  state.ui.game_over = true;
  state.flags.player_special_state_active = true;
  state.input.jump_pressed = true;

  comic2::handle_player_special_state(state);

  expect(!state.ui.game_over,
         "jump input should clear the game-over terminal state");
  expect(state.player.lives == 3,
         "jump input should restart the run from the default life count");
  expect(!state.ui.modal_active,
         "jump input should dismiss the game-over modal during restart");
}

void test_down_input_cycles_active_mode_mask() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.ui.active_mode_mask = 0;
  state.progression.mode_inventory_mask = 0x07U;
  state.input.down_pressed = true;

  comic2::update_player_mode_cycle(state);
  expect(state.ui.active_mode_mask == 1U,
         "down input should advance the active mode mask to the first slot");

  comic2::update_player_mode_cycle(state);
  expect(state.ui.active_mode_mask == 2U,
         "repeated down input should advance the active mode mask again");
}

void test_down_input_cycles_only_through_collected_modes() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.ui.active_mode_mask = 0x01U;
  state.progression.mode_inventory_mask = 0x05U;
  state.input.down_pressed = true;

  comic2::update_player_mode_cycle(state);
  expect(state.ui.active_mode_mask == 0x04U,
         "down input should skip uncollected modes and pick the next valid "
         "selection");

  comic2::update_player_mode_cycle(state);
  expect(state.ui.active_mode_mask == 0x01U,
         "down input should wrap from the last valid mode back to the first");
}

void test_down_input_is_a_noop_when_no_modes_are_collected() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.ui.active_mode_mask = 0x00U;
  state.progression.mode_inventory_mask = 0x00U;
  state.input.down_pressed = true;

  comic2::update_player_mode_cycle(state);
  expect(state.ui.active_mode_mask == 0x00U,
         "down input should not change the selection when no modes are owned");
}

void test_down_input_updates_selection_immediately_on_down_press() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.ui.active_mode_mask = 0x00U;
  state.progression.mode_inventory_mask = 0x07U;
  state.input.down_pressed = true;

  comic2::update_player_mode_cycle(state);
  expect(
      state.ui.active_mode_mask == 0x01U,
      "down input should update the selection immediately on a single press");
}

void test_mode_activation_starts_effect_and_consumes_selected_mode() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.ui.active_mode_mask = 0x02U;
  state.input.mode_activate_pressed = true;

  comic2::update_player_mode_activation(state);

  expect(state.ui.active_mode_mask == 0x02U,
         "mode activation should keep the selected mode visible");
  expect(state.player.active_mode_effect == 0x02U,
         "mode activation should start the matching active-mode effect");
  expect(state.player.mode_effect_ticks == 30U,
         "mode activation should initialize the effect countdown");
}

void test_mode_effect_countdown_expires_after_ticks() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.player.active_mode_effect = 0x02U;
  state.player.mode_effect_ticks = 1U;

  comic2::update_player_mode_effect(state);

  expect(state.player.active_mode_effect == 0U,
         "mode effect should clear when its countdown expires");
  expect(state.player.mode_effect_ticks == 0U,
         "mode effect countdown should reach zero when expired");
}

void test_input_fallback_expires_mode_effects_on_tick() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.player.active_mode_effect = 0x02U;
  state.player.mode_effect_ticks = 1U;

  comic2::handle_input_fallback(state);

  expect(state.player.active_mode_effect == 0U,
         "input fallback should clear an expired mode effect");
  expect(state.player.mode_effect_ticks == 0U,
         "input fallback should leave the mode countdown at zero once expired");
}

void test_room_event_trigger_detects_proximity_mapped_object() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.current_room = 0;
  state.player.x = 96;
  state.player.y = 98;
  state.mapped_objects.push_back(comic2::MappedObject12{
      .room_x = 0,
      .room_y = 0,
      .descriptor_ptr = 0x1234,
      .state_flags = 0x0001,
      .world_x = 90,
      .world_y = 90,
  });

  comic2::handle_input_fallback(state);

  expect(!state.flags.room_event_triggered,
         "proximity trigger should be consumed into the queued event message");
  expect(state.ui.pending_event_message == "Room Event Triggered",
         "proximity trigger should queue the room-event message");
  expect(state.ui.room_event_consumed,
         "proximity trigger should mark the event as consumed once");
}

void test_room_event_message_is_queued_for_display() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.flags.room_event_triggered = true;

  comic2::handle_input_fallback(state);

  expect(state.ui.pending_event_message == "Room Event Triggered",
         "room-event trigger should queue a displayable message");
  expect(!state.ui.modal_active,
         "queued room-event message should wait for the modal display step");
}

void test_room_event_message_becomes_modal_prompt() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.ui.pending_event_message = "Room Event Triggered";

  comic2::handle_input_fallback(state);

  expect(state.ui.modal_active,
         "pending room-event message should open the modal prompt path");
  expect(state.ui.modal_prompt == "Room Event Triggered",
         "pending room-event message should become the modal prompt");
  expect(state.ui.pending_event_message.empty(),
         "room-event message should be consumed once shown");
}

void test_room_event_message_is_only_queued_once_per_trigger() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.flags.room_event_triggered = true;

  comic2::handle_input_fallback(state);

  expect(state.ui.pending_event_message == "Room Event Triggered",
         "first room-event trigger should queue a message");
  expect(state.ui.room_event_consumed,
         "first room-event trigger should mark the one-shot state consumed");

  state.flags.room_event_triggered = true;
  state.ui.pending_event_message.clear();
  comic2::handle_input_fallback(state);

  expect(state.ui.pending_event_message.empty(),
         "repeated room-event triggers should not re-queue the message");
  expect(!state.ui.modal_active,
         "repeated room-event triggers should not reopen the modal prompt");
}

void test_object_interaction_script_queues_context_message() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.player.x = 96;
  state.player.y = 100;
  state.mapped_objects.push_back(comic2::MappedObject12{
      .room_x = 0,
      .room_y = 0,
      .descriptor_ptr = 0x1234,
      .state_flags = 0x0002,
      .world_x = 90,
      .world_y = 90,
  });

  comic2::handle_input_fallback(state);

  expect(state.ui.modal_active, "nearby interaction objects should surface a "
                                "context message through the modal path");
  expect(state.ui.modal_prompt == "Object Interaction",
         "interaction context should be promoted into the active modal prompt");
  expect(state.ui.pending_event_message.empty(),
         "interaction prompts should be consumed once delivered to the modal "
         "flow");
}

void test_room_event_anchor_motion_moves_and_clamps_to_room_bounds() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.room_grid.tile_w = 20;
  state.room_grid.tile_h = 15;
  state.room_grid.row_pointers.assign(15, 0);
  state.room_grid.tile_data.assign(300, 0x00);
  state.room_event_anchor.active = true;
  state.room_event_anchor.x = 5;
  state.room_event_anchor.y = 10;
  state.room_event_anchor.velocity_x = 8;
  state.room_event_anchor.velocity_y = 4;

  comic2::update_room_event_anchor_motion(state);

  expect(state.room_event_anchor.x == 13,
         "room event anchor should advance by its configured x velocity");
  expect(state.room_event_anchor.y == 14,
         "room event anchor should advance by its configured y velocity");
  expect(state.room_event_anchor.x <= 320,
         "room event anchor should remain within the screen-space bounds");
}

void test_room_event_anchor_proximity_arms_event_flow() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.room_event_anchor.active = true;
  state.room_event_anchor.x = 96;
  state.room_event_anchor.y = 100;
  state.player.x = 104;
  state.player.y = 96;

  const bool near = comic2::check_comic_near_room_event_anchor(state);

  expect(near,
         "room event anchor should arm when the player enters its proximity "
         "window");
  expect(state.room_event_anchor.active,
         "room event anchor should remain active while the player is near");
}

void test_room_event_anchor_sprite_reseeds_when_it_passes_the_player() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.room_event_anchor.active = true;
  state.room_event_anchor.x = 180;
  state.room_event_anchor.y = 80;
  state.player.x = 96;
  state.player.y = 100;

  comic2::update_room_event_anchor_sprite(state);

  expect(
      state.room_event_anchor.x == 0,
      "anchor sprite should reseed on the left side after passing the player");
  expect(
      state.room_event_anchor.y <= 184,
      "anchor sprite should remain within the visible room band after reseed");
}

void test_room_event_anchor_is_advanced_during_input_handling() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.room_event_anchor.active = true;
  state.room_event_anchor.x = 10;
  state.room_event_anchor.y = 12;
  state.room_event_anchor.velocity_x = 1;
  state.room_event_anchor.velocity_y = 0;
  state.player.x = 20;
  state.player.y = 12;

  comic2::handle_input_fallback(state);

  expect(state.room_event_anchor.x == 11,
         "input fallback should advance the room-event anchor position");
  expect(state.ui.pending_event_message == "Room Event Triggered",
         "input fallback should queue the room-event message when the player "
         "is near the anchor");
}

void test_level_completion_activates_when_gem_threshold_is_met() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.player.gems = 2;
  state.level_completion_gems_required = 2;

  comic2::handle_input_fallback(state);

  expect(state.level_complete, "gem collection should mark the level as "
                               "complete once the threshold is met");
  expect(state.ui.modal_active,
         "level completion should open the completion modal");
  expect(state.ui.modal_prompt == "Level Complete!",
         "level completion should present a clear completion prompt");
}

void test_level_completion_does_not_trigger_before_threshold() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.player.gems = 1;
  state.level_completion_gems_required = 2;

  comic2::handle_input_fallback(state);

  expect(
      !state.level_complete,
      "level completion should stay pending before the gem threshold is met");
  expect(
      !state.ui.modal_active,
      "level completion should not open a modal before the threshold is met");
}

void test_level_completion_confirm_advances_to_next_level_and_resets_transient_state() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.current_level = 0;
  state.current_room = 2;
  state.player.gems = 3;
  state.player.firepower = 2;
  state.player.lives = 2;
  state.player.x = 44;
  state.player.y = 58;
  state.player.x_vel = 4;
  state.player.y_vel = -3;
  state.player.is_airborne = true;
  state.player.is_physics_active = true;
  state.progression.mode_inventory_mask = 0x03U;
  state.ui.active_mode_mask = 0x02U;
  state.ui.inventory_mask = 0x05U;
  state.level_complete = true;
  state.ui.modal_active = true;
  state.ui.modal_prompt = "Level Complete!";
  state.ui.level_complete_modal = true;
  state.ui.modal_confirmed = true;
  state.flags.player_special_state_active = true;
  state.projectiles.push_back(comic2::ProjectileState{.active = true});
  state.active_entities.push_back(comic2::ActiveEntity8{});
  state.mapped_objects.push_back(comic2::MappedObject12{});
  state.activation_state = comic2::EntityActivationState{};
  state.activation_toggle = 7;

  std::vector<std::uint8_t> decoded_room_bytes(0x2C4, 0x00);
  decoded_room_bytes[0x2A0] = 0x00;
  decoded_room_bytes[0x2A1] = 0x00;
  decoded_room_bytes[0x2A2] = 0x04;
  decoded_room_bytes[0x2A3] = 0x00;
  decoded_room_bytes[0x2A4] = 0x03;
  decoded_room_bytes[0x2A5] = 0x00;
  const auto encoded_room_bytes = encode_literal_signed_rle(decoded_room_bytes);
  state.room_resource_bytes.resize(0x20 + encoded_room_bytes.size(), 0x00);
  state.room_resource_bytes[2] = 0x01;
  state.room_resource_bytes[3] = 0x00;
  state.room_resource_bytes[0x04] = 0x04;
  state.room_resource_bytes[0x05] = 0x00;
  state.room_resource_bytes[0x06] = 0x03;
  state.room_resource_bytes[0x07] = 0x00;
  state.room_resource_bytes[0x08] = 0x20;
  state.room_resource_bytes[0x09] = 0x00;
  std::copy(encoded_room_bytes.begin(), encoded_room_bytes.end(),
            state.room_resource_bytes.begin() + 0x20);

  comic2::handle_player_special_state(state);

  expect(state.current_level == 1,
         "confirming level completion should advance to the next level");
  expect(state.current_room == 0,
         "confirming level completion should reset to the start room");
  expect(state.player.x == 64,
         "confirming level completion should reset the player to the spawn x");
  expect(state.player.y == 96,
         "confirming level completion should reset the player to the spawn y");
  expect(state.player.x_vel == 0,
         "confirming level completion should clear horizontal velocity");
  expect(state.player.y_vel == 0,
         "confirming level completion should clear vertical velocity");
  expect(!state.player.is_airborne,
         "confirming level completion should clear airborne state");
  expect(state.projectiles.empty(),
         "confirming level completion should clear transient projectiles");
  expect(state.active_entities.empty(),
         "confirming level completion should clear transient entities");
  expect(state.mapped_objects.empty(),
         "confirming level completion should clear transient mapped objects");
  expect(state.player.gems == 3,
         "confirming level completion should preserve earned gems");
  expect(state.player.firepower == 2,
         "confirming level completion should preserve firepower progress");
  expect(state.player.lives == 2,
         "confirming level completion should preserve remaining lives");
  expect(state.progression.mode_inventory_mask == 0x03U,
         "confirming level completion should preserve the mode inventory");
}

void test_level_completion_confirm_keeps_current_level_when_next_level_is_unavailable() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.current_level = 1;
  state.current_room = 3;
  state.player.x = 88;
  state.player.y = 66;
  state.player.gems = 1;
  state.player.firepower = 1;
  state.player.lives = 1;
  state.level_complete = true;
  state.ui.modal_active = true;
  state.ui.modal_prompt = "Level Complete!";
  state.ui.level_complete_modal = true;
  state.ui.modal_confirmed = true;
  state.flags.player_special_state_active = true;
  state.projectiles.push_back(comic2::ProjectileState{.active = true});

  comic2::handle_player_special_state(state);

  expect(
      state.current_level == 1,
      "missing level resources should leave the player on the current level");
  expect(state.current_room == 3,
         "missing level resources should preserve the current room");
  expect(state.player.x == 88,
         "missing level resources should preserve the current player position");
  expect(
      state.player.y == 66,
      "missing level resources should preserve the current player y position");
  expect(state.player.gems == 1,
         "missing level resources should preserve collected gems");
  expect(state.player.firepower == 1,
         "missing level resources should preserve firepower progress");
  expect(state.player.lives == 1,
         "missing level resources should preserve lives");
  expect(!state.projectiles.empty(),
         "missing level resources should leave transient state unchanged");
}

void test_level_completion_finale_starts_when_next_level_is_unavailable() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.current_level = 1;
  state.current_room = 3;
  state.level_complete = true;
  state.ui.modal_active = true;
  state.ui.modal_prompt = "Level Complete!";
  state.ui.level_complete_modal = true;
  state.ui.modal_confirmed = true;
  state.flags.player_special_state_active = true;

  comic2::handle_player_special_state(state);

  expect(state.current_level == 1,
         "finale path should keep the player on the final level");
  expect(state.ui.modal_active,
         "finale path should keep the completion modal visible");
  expect(state.ui.modal_prompt == "The End",
         "finale path should switch to the end-of-game prompt before the final "
         "sequence");
  expect(state.ui.cinematic_frame == 1,
         "finale transition should advance the cinematic frame via the stub");
}

void test_player_mode_activation_tracks_mode_inventory() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.ui.active_mode_mask = 0x01U;
  state.ui.inventory_mask = 0U;
  state.input.mode_activate_pressed = true;

  comic2::update_player_mode_activation(state);

  expect(state.progression.mode_inventory_mask == 0x01U,
         "activating a mode should record it in the progression inventory");
  expect((state.ui.inventory_mask & 0x04U) != 0U,
         "activating a mode should expose the collection in the HUD inventory "
         "mask");
  expect(state.ui.active_mode_mask == 0x01U,
         "activating a mode should keep the active mode selection visible");
}

void test_progression_state_updates_inventory_bits() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.player.gems = 3;
  state.player.firepower = 2;
  state.player.lives = 2;
  state.ui.active_mode_mask = 0;
  state.ui.inventory_mask = 0;

  comic2::update_progression_state(state);

  expect((state.ui.inventory_mask & 0x01U) != 0U,
         "progression state should mark collected gems in the inventory mask");
  expect((state.ui.inventory_mask & 0x02U) != 0U,
         "progression state should mark increased firepower in the inventory "
         "mask");
  expect(state.ui.active_mode_mask == 0x01U,
         "progression state should activate the life-progression mode bit when "
         "lives are available");
}

void test_progression_state_keeps_inventory_bits_stable() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.player.gems = 0;
  state.player.firepower = 1;
  state.player.lives = 0;
  state.ui.active_mode_mask = 0x01;
  state.ui.inventory_mask = 0x03;

  comic2::update_progression_state(state);

  expect(state.ui.inventory_mask == 0x03U,
         "progression state should preserve already-known inventory bits");
  expect(state.ui.active_mode_mask == 0x01U,
         "progression state should preserve already-known active mode bits");
}

void test_respawn_reset_preserves_persistent_progression_state() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.player.gems = 3;
  state.player.firepower = 2;
  state.player.lives = 2;
  comic2::update_progression_state(state);

  const auto persisted = state.progression;
  comic2::reset_player_respawn_state(state);

  expect(state.progression.flags == persisted.flags,
         "respawn reset should preserve the persistent progression flags");
  expect(state.progression.gems_collected,
         "respawn reset should keep gem progression marked as collected");
  expect(state.progression.firepower_unlocked,
         "respawn reset should keep firepower progression marked as unlocked");
  expect(state.progression.lives_available,
         "respawn reset should keep life progression marked as available");
}

void test_tile_hazard_stage_instantly_kills_player() {
  comic2::GameDispatcher dispatcher;
  comic2::install_default_stage_hooks(dispatcher);

  comic2::RuntimeState state;
  state.player.hp = 3;
  state.player.y = -16;
  state.player.is_physics_active = true;

  state.room_grid.tile_w = 1;
  state.room_grid.tile_h = 2;
  state.room_grid.row_pointers = {0, 1};
  state.room_grid.tile_data = {0xF4, 0xF4};

  const auto first = dispatcher.run_tick(state);
  expect(first.stage == comic2::DispatchStage::GroundedPhysics,
         "physics stage should run before hazard stage is selected");
  expect(state.flags.tile_hazard_triggered,
         "grounded physics hook should set tile hazard when on a hazard tile");

  const auto second = dispatcher.run_tick(state);
  expect(second.stage == comic2::DispatchStage::TileHazard,
         "hazard flag should route dispatcher to tile hazard stage");
  expect(state.player.hp == 0,
         "tile hazard handler should kill the player immediately");
  expect(state.flags.player_special_state_active,
         "tile hazard handler should enter the death/special state path");
  expect(!state.flags.tile_hazard_triggered,
         "tile hazard handler should clear hazard flag after handling");
}

void test_stage_flags_are_consumed_by_default_handlers() {
  comic2::GameDispatcher dispatcher;
  comic2::install_default_stage_hooks(dispatcher);

  {
    comic2::RuntimeState state;
    state.flags.special_logic1_active = true;
    const auto first = dispatcher.run_tick(state);
    expect(first.stage == comic2::DispatchStage::SpecialLogic1,
           "special logic1 stage should run when flagged");
    const auto second = dispatcher.run_tick(state);
    expect(second.stage == comic2::DispatchStage::InputHandling,
           "special logic1 flag should be consumed after handler");
  }

  {
    comic2::RuntimeState state;
    state.flags.special_logic2_active = true;
    const auto first = dispatcher.run_tick(state);
    expect(first.stage == comic2::DispatchStage::SpecialLogic2,
           "special logic2 stage should run when flagged");
    const auto second = dispatcher.run_tick(state);
    expect(second.stage == comic2::DispatchStage::InputHandling,
           "special logic2 flag should be consumed after handler");
  }

  {
    comic2::RuntimeState state;
    state.flags.timed_overlay_pending = true;
    const auto first = dispatcher.run_tick(state);
    expect(first.stage == comic2::DispatchStage::TimedOverlay,
           "timed overlay stage should run when flagged");
    const auto second = dispatcher.run_tick(state);
    expect(second.stage == comic2::DispatchStage::InputHandling,
           "timed overlay flag should be consumed after handler");
  }

  {
    comic2::RuntimeState state;
    state.player.is_animation_active = true;
    const auto first = dispatcher.run_tick(state);
    expect(first.stage == comic2::DispatchStage::PlayerAnimation,
           "player animation stage should run when flagged");
    const auto second = dispatcher.run_tick(state);
    expect(second.stage == comic2::DispatchStage::InputHandling,
           "player animation flag should be consumed after handler");
  }

  {
    comic2::RuntimeState state;
    state.player.is_attack_active = true;
    const auto first = dispatcher.run_tick(state);
    expect(first.stage == comic2::DispatchStage::AttackAnimation,
           "attack animation stage should run when flagged");
    const auto second = dispatcher.run_tick(state);
    expect(second.stage == comic2::DispatchStage::InputHandling,
           "attack animation flag should be consumed after handler");
  }

  {
    comic2::RuntimeState state;
    state.flags.distance_interaction_active = true;
    const auto first = dispatcher.run_tick(state);
    expect(first.stage == comic2::DispatchStage::DistanceInteraction,
           "distance interaction stage should run when flagged");
    const auto second = dispatcher.run_tick(state);
    expect(second.stage == comic2::DispatchStage::InputHandling,
           "distance interaction flag should be consumed after handler");
  }
}

void test_input_fallback_arms_grounded_physics_for_next_tick() {
  comic2::GameDispatcher dispatcher;
  comic2::install_default_stage_hooks(dispatcher);

  comic2::RuntimeState state;
  state.player.is_airborne = false;
  state.player.is_physics_active = false;

  const auto first = dispatcher.run_tick(state);
  expect(first.stage == comic2::DispatchStage::InputHandling,
         "first tick should run input fallback");
  expect(state.player.is_physics_active,
         "input fallback should arm grounded physics for next tick");

  const auto second = dispatcher.run_tick(state);
  expect(second.stage == comic2::DispatchStage::GroundedPhysics,
         "second tick should execute grounded physics after arming");
}

void test_transition_completion_finalizes_room_and_player_position() {
  comic2::GameDispatcher dispatcher;
  comic2::install_default_stage_hooks(dispatcher);

  comic2::RuntimeState state;
  state.current_level = 1;
  state.current_room = 0;
  state.player.x = 12;
  state.player.y = 50;
  state.player.x_vel = 3;
  state.player.y_vel = -2;
  state.player.is_airborne = true;
  state.player.is_physics_active = true;
  state.transition_state.active = true;
  state.transition_state.completed = false;
  state.transition_state.target_room = 2;
  state.transition_state.target_player_x = 16;
  state.transition_state.tick_count = 31;
  state.transition_state.frame_index = 31;

  const auto result = dispatcher.run_tick(state);
  expect(result.stage == comic2::DispatchStage::InputHandling,
         "completed transition should still run through the input stage");
  expect(state.transition_state.completed,
         "transition should complete when its timer expires");
  expect(state.current_room == 2,
         "transition completion should update the current room");
  expect(state.player.x == 16,
         "transition completion should place the player at the target x");
  expect(state.player.x_vel == 0,
         "transition completion should clear horizontal velocity");
  expect(state.player.y_vel == 0,
         "transition completion should clear vertical velocity");
  expect(!state.player.is_airborne,
         "transition completion should clear airborne state");
  expect(!state.transition_state.player_frozen,
         "completed transition should release player control");
}

void test_level_transition_loads_room_tilemap() {
  comic2::RuntimeState state;
  state.current_level = 1;
  state.current_room = 0;
  state.flags.level_transition_pending = true;
  state.pending_room_transition =
      comic2::PendingRoomTransition{.target_room = 0,
                                    .target_player_x = 0,
                                    .target_level = std::nullopt,
                                    .target_player_y = std::nullopt};

  std::vector<std::uint8_t> decoded_room_bytes(0x2C4, 0x00);
  decoded_room_bytes[0x2A0] = 0x00;
  decoded_room_bytes[0x2A1] = 0x00;
  decoded_room_bytes[0x2A2] = 0x04;
  decoded_room_bytes[0x2A3] = 0x00;
  decoded_room_bytes[0x2A4] = 0x08;
  decoded_room_bytes[0x2A5] = 0x00;

  const std::vector<std::uint8_t> encoded_room_bytes =
      encode_literal_signed_rle(decoded_room_bytes);

  state.room_resource_bytes.resize(0x20 + encoded_room_bytes.size(), 0x00);
  state.room_resource_bytes[2] = 0x01;
  state.room_resource_bytes[3] = 0x00;
  state.room_resource_bytes[0x04] = 0x04;
  state.room_resource_bytes[0x05] = 0x00;
  state.room_resource_bytes[0x06] = 0x03;
  state.room_resource_bytes[0x07] = 0x00;
  state.room_resource_bytes[0x08] = 0x20;
  state.room_resource_bytes[0x09] = 0x00;
  std::copy(encoded_room_bytes.begin(), encoded_room_bytes.end(),
            state.room_resource_bytes.begin() + 0x20);

  comic2::GameDispatcher dispatcher;
  comic2::install_default_stage_hooks(dispatcher);

  const auto result = dispatcher.run_tick(state);
  expect(result.stage == comic2::DispatchStage::LevelTransition,
         "level transition should select level transition stage");
  expect(!state.flags.level_transition_pending,
         "level transition handler should clear its pending flag");
  expect(state.room_grid.tile_w == 4,
         "level transition should load the new room tile width");
  expect(state.room_grid.tile_h == 3,
         "level transition should load the new room tile height");
  expect(state.room_grid.row_pointers == std::vector<std::uint16_t>{0, 4, 8},
         "level transition should build the row pointer table");
}

void test_level_transition_right_edge_reloads_target_room_from_assets() {
  const auto root = std::filesystem::temp_directory_path() /
                    "comic2_transition_right_edge_assets";
  std::filesystem::remove_all(root);
  std::filesystem::create_directories(root);

  const auto decoded_room1 = make_decoded_room_bytes(0x22, 0, 5, 10);
  const auto room_bytes =
      make_room_resource_bytes(1, 1, 5, 3, 0x30, decoded_room1);
  {
    std::ofstream out(root / "FR900.0", std::ios::binary);
    out.write(reinterpret_cast<const char *>(room_bytes.data()),
              static_cast<std::streamsize>(room_bytes.size()));
  }

  comic2::GameDispatcher dispatcher;
  comic2::install_default_stage_hooks(dispatcher);

  comic2::RuntimeState state;
  state.current_level = 1;
  state.current_room = 0;
  state.assets_root = root;
  state.player.x = 320;
  state.player.is_airborne = false;
  state.player.is_physics_active = true;

  const auto first = dispatcher.run_tick(state);
  expect(first.stage == comic2::DispatchStage::GroundedPhysics,
         "right-edge transition should be detected during grounded physics");
  expect(state.flags.level_transition_pending,
         "right-edge transition should schedule a pending level transition");
  expect(state.pending_room_transition.has_value(),
         "right-edge transition should record target room metadata");
  expect(state.current_room == 0,
         "room index should not commit before transition load succeeds");

  const auto second = dispatcher.run_tick(state);
  expect(second.stage == comic2::DispatchStage::LevelTransition,
         "pending transition should route to level transition stage");
  expect(!state.flags.level_transition_pending,
         "level transition flag should be cleared after transition stage");
  expect(!state.pending_room_transition.has_value(),
         "pending transition metadata should be consumed");
  expect(state.current_room == 1,
         "successful transition should commit target room index");
  expect(state.player.x == 0,
         "successful right-edge transition should place player at left edge");
  expect(state.room_grid.tile_w == 5 && state.room_grid.tile_h == 3,
         "successful transition should load target room dimensions");
  expect(state.room_grid.row_pointers == std::vector<std::uint16_t>{0, 5, 10},
         "successful transition should rebuild target room row pointers");

  std::filesystem::remove_all(root);
}

void test_level_transition_left_edge_room0_clamps_without_transition() {
  comic2::GameDispatcher dispatcher;
  comic2::install_default_stage_hooks(dispatcher);

  comic2::RuntimeState state;
  state.current_room = 0;
  state.player.x = -1;
  state.player.is_airborne = false;
  state.player.is_physics_active = true;

  const auto tick = dispatcher.run_tick(state);
  expect(tick.stage == comic2::DispatchStage::GroundedPhysics,
         "left-edge clamp should be handled in grounded physics");
  expect(state.player.x == 0, "left-edge at room 0 should clamp player to x=0");
  expect(!state.flags.level_transition_pending,
         "left-edge at room 0 should not enqueue a room transition");
  expect(!state.pending_room_transition.has_value(),
         "left-edge at room 0 should not produce pending transition metadata");
  expect(state.current_room == 0,
         "left-edge at room 0 should keep room index unchanged");
}

void test_level_transition_failure_preserves_prior_runtime_state() {
  const auto root = std::filesystem::temp_directory_path() /
                    "comic2_transition_failure_assets";
  std::filesystem::remove_all(root);
  std::filesystem::create_directories(root);

  {
    std::ofstream out(root / "FR901.0", std::ios::binary);
    const std::vector<std::uint8_t> malformed = {0x10, 0x00, 0x00, 0x00,
                                                 0xFF, 0xFF, 0xFF, 0xFF};
    out.write(reinterpret_cast<const char *>(malformed.data()),
              static_cast<std::streamsize>(malformed.size()));
  }

  comic2::GameDispatcher dispatcher;
  comic2::install_default_stage_hooks(dispatcher);

  comic2::RuntimeState state;
  state.current_level = 1;
  state.current_room = 2;
  state.assets_root = root;
  state.player.x = 111;
  state.room_grid.tile_w = 7;
  state.room_grid.tile_h = 4;
  state.room_grid.row_pointers = {0, 7, 14, 21};
  state.room_grid.tile_data = {1, 2, 3, 4};
  const auto before = state;

  state.flags.level_transition_pending = true;
  state.pending_room_transition =
      comic2::PendingRoomTransition{.target_room = 3,
                                    .target_player_x = 0,
                                    .target_level = std::nullopt,
                                    .target_player_y = std::nullopt};

  const auto tick = dispatcher.run_tick(state);
  expect(tick.stage == comic2::DispatchStage::LevelTransition,
         "pending transition should execute level transition stage");
  expect(!state.flags.level_transition_pending,
         "failed transition should still clear pending transition flag");
  expect(!state.pending_room_transition.has_value(),
         "failed transition should consume pending metadata");
  expect(state.current_room == before.current_room,
         "failed transition should preserve prior room index");
  expect(state.player.x == before.player.x,
         "failed transition should preserve prior player x");
  expect(state.room_grid == before.room_grid,
         "failed transition should preserve prior room grid state");

  std::filesystem::remove_all(root);
}

comic2::RoomTileGrid make_projectile_floor_grid(std::uint8_t tile_id) {
  comic2::RoomTileGrid grid;
  grid.tile_w = 2;
  grid.tile_h = 2;
  grid.row_pointers = {0, 2};
  grid.tile_data = {0x00, 0x00, tile_id, tile_id};
  return grid;
}

void test_projectile_scripted_tick_updates_deterministically() {
  const auto run_sequence = [](const std::vector<comic2::InputState> &inputs) {
    comic2::GameDispatcher dispatcher;
    comic2::install_default_stage_hooks(dispatcher);

    comic2::RuntimeState state;
    state.projectiles.push_back(comic2::ProjectileState{.x = 10,
                                                        .y = 10,
                                                        .x_vel = 1,
                                                        .y_vel = 0,
                                                        .anim_frame = 0,
                                                        .active = true});

    for (const auto &input : inputs) {
      state.input = input;
      dispatcher.run_tick(state);
    }

    return state.projectiles;
  };

  const std::vector<comic2::InputState> sequence = {
      comic2::InputState{},
      comic2::InputState{},
      comic2::InputState{},
  };

  const auto first = run_sequence(sequence);
  const auto replay = run_sequence(sequence);

  expect(first.size() == replay.size(),
         "projectile scripted tick replay should keep identical sequence size");
  for (std::size_t i = 0; i < first.size(); ++i) {
    expect(first[i].x == replay[i].x,
           "projectile replay mismatch: x should be deterministic");
    expect(first[i].y == replay[i].y,
           "projectile replay mismatch: y should be deterministic");
    expect(first[i].x_vel == replay[i].x_vel,
           "projectile replay mismatch: x velocity should be deterministic");
    expect(first[i].y_vel == replay[i].y_vel,
           "projectile replay mismatch: y velocity should be deterministic");
    expect(first[i].anim_frame == replay[i].anim_frame,
           "projectile replay mismatch: animation should be deterministic");
    expect(first[i].active == replay[i].active,
           "projectile replay mismatch: active flag should be deterministic");
  }

  expect(first.size() == 1, "scripted projectile tick should keep one entry");
  expect(first[0].x == 13, "projectile should advance one pixel per tick");
  expect(first[0].anim_frame == 3,
         "projectile animation frame should advance modulo 8 each tick");
  expect(replay.size() == 1,
         "replayed scripted projectile tick should keep one entry");
  expect(replay[0].x == 13,
         "replayed projectile should advance one pixel per tick");
  expect(
      replay[0].anim_frame == 3,
      "replayed projectile animation frame should advance modulo 8 each tick");
}

void test_projectile_scripted_tick_deactivates_on_tile_collision() {
  comic2::GameDispatcher dispatcher;
  comic2::install_default_stage_hooks(dispatcher);

  comic2::RuntimeState state;
  state.room_grid = make_projectile_floor_grid(0x01);
  state.projectiles.push_back(comic2::ProjectileState{.x = 13,
                                                      .y = 13,
                                                      .x_vel = 0,
                                                      .y_vel = 0,
                                                      .anim_frame = 0,
                                                      .active = true});

  const auto result = dispatcher.run_tick(state);
  expect(result.stage == comic2::DispatchStage::InputHandling,
         "projectile tick should run through input handling stage");
  expect(!state.projectiles[0].active,
         "projectile should deactivate on tile collision during scripted tick");
}

void test_airborne_fallback_clamps_player_with_missing_room_support() {
  comic2::GameDispatcher dispatcher;
  comic2::install_default_stage_hooks(dispatcher);

  comic2::RuntimeState state;
  state.room_grid = {};
  state.player.y = 190;
  state.player.y_vel = 6;
  state.player.is_airborne = true;
  state.player.is_physics_active = true;

  const auto result = dispatcher.run_tick(state);
  expect(result.stage == comic2::DispatchStage::AirbornePhysics,
         "airborne player should route to airborne physics stage");
  expect(state.player.y == 168,
         "fallback clamp should keep player inside viewport bottom");
  expect(state.player.y_vel == 0,
         "fallback clamp should clear downward velocity at viewport bottom");
  expect(!state.player.is_airborne,
         "fallback clamp should ground player at viewport bottom");
}

void test_airborne_with_valid_room_data_skips_fallback_clamp() {
  comic2::GameDispatcher dispatcher;
  comic2::install_default_stage_hooks(dispatcher);

  comic2::RuntimeState state;
  state.room_grid.tile_w = 1;
  state.room_grid.tile_h = 1;
  state.room_grid.row_pointers = {0};
  state.room_grid.tile_data = {0x00};

  state.player.y = 190;
  state.player.y_vel = 6;
  state.player.is_airborne = true;
  state.player.is_physics_active = true;

  const auto result = dispatcher.run_tick(state);
  expect(result.stage == comic2::DispatchStage::AirbornePhysics,
         "airborne player should route to airborne physics stage");
  expect(state.player.y > 168,
         "fallback clamp should not force viewport floor when room data is "
         "available");
  expect(state.player.is_airborne,
         "player should remain airborne when fallback clamp is skipped");
}

} // namespace

void run_dispatcher_tests() {
  test_priority_order();
  test_input_fallback();
  test_stage_hook_execution();
  test_default_handlers_basic_movement_and_jump();
  test_deterministic_tick_replay();
  test_dispatcher_trace_log();
  test_default_stage_hook_coverage();
  test_player_animation_handler_advances_walk_cycle();
  test_attack_handler_uses_attack_overlay_state();
  test_timed_overlay_handler_starts_and_counts_down();
  test_death_flow_prompts_when_lives_are_exhausted();
  test_death_countdown_decrements_lives_and_respawns_at_spawn();
  test_confirming_continue_respawns_player();
  test_game_over_confirm_restarts_runtime_state();
  test_game_over_input_restarts_without_modal_confirm();
  test_down_input_cycles_active_mode_mask();
  test_down_input_cycles_only_through_collected_modes();
  test_down_input_is_a_noop_when_no_modes_are_collected();
  test_down_input_updates_selection_immediately_on_down_press();
  test_player_mode_activation_tracks_mode_inventory();
  test_mode_activation_starts_effect_and_consumes_selected_mode();
  test_mode_effect_countdown_expires_after_ticks();
  test_input_fallback_expires_mode_effects_on_tick();
  test_room_event_trigger_detects_proximity_mapped_object();
  test_room_event_message_is_queued_for_display();
  test_room_event_message_becomes_modal_prompt();
  test_room_event_message_is_only_queued_once_per_trigger();
  test_object_interaction_script_queues_context_message();
  test_room_event_anchor_motion_moves_and_clamps_to_room_bounds();
  test_room_event_anchor_proximity_arms_event_flow();
  test_room_event_anchor_sprite_reseeds_when_it_passes_the_player();
  test_room_event_anchor_is_advanced_during_input_handling();
  test_level_completion_activates_when_gem_threshold_is_met();
  test_level_completion_does_not_trigger_before_threshold();
  test_level_completion_confirm_advances_to_next_level_and_resets_transient_state();
  test_level_completion_confirm_keeps_current_level_when_next_level_is_unavailable();
  test_level_completion_finale_starts_when_next_level_is_unavailable();
  test_progression_state_updates_inventory_bits();
  test_progression_state_keeps_inventory_bits_stable();
  test_respawn_reset_preserves_persistent_progression_state();
  test_tile_hazard_stage_instantly_kills_player();
  test_stage_flags_are_consumed_by_default_handlers();
  test_input_fallback_arms_grounded_physics_for_next_tick();
  test_transition_completion_finalizes_room_and_player_position();
  test_level_transition_loads_room_tilemap();
  test_level_transition_right_edge_reloads_target_room_from_assets();
  test_level_transition_left_edge_room0_clamps_without_transition();
  test_level_transition_failure_preserves_prior_runtime_state();
  test_projectile_scripted_tick_updates_deterministically();
  test_projectile_scripted_tick_deactivates_on_tile_collision();
  test_airborne_fallback_clamps_player_with_missing_room_support();
  test_airborne_with_valid_room_data_skips_fallback_clamp();
}
