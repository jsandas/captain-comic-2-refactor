#include "comic2/dispatcher.hpp"

#include "comic2/renderer.hpp"

namespace comic2 {

namespace {
constexpr ProjectileBounds kDefaultProjectileBounds{};
constexpr std::int16_t kDefaultViewportMinX = 0;
constexpr std::int16_t kDefaultViewportMinY = 0;
constexpr std::int16_t kDefaultViewportWidth =
    static_cast<std::int16_t>(EgaPlanarSurface::kDefaultWidthPixels);
constexpr std::int16_t kDefaultViewportHeight =
    static_cast<std::int16_t>(EgaPlanarSurface::kDefaultHeightRows);
} // namespace

DispatchStage GameDispatcher::choose_stage(const RuntimeState &state) {
  const auto &flags = state.flags;
  const auto &player = state.player;

  if (state.transition_state.active) {
    return DispatchStage::InputHandling;
  }

  if (flags.level_transition_pending) {
    return DispatchStage::LevelTransition;
  }
  if (flags.special_logic1_active) {
    return DispatchStage::SpecialLogic1;
  }
  if (flags.special_logic2_active) {
    return DispatchStage::SpecialLogic2;
  }
  if (player.is_airborne) {
    return DispatchStage::AirbornePhysics;
  }
  if (flags.timed_overlay_pending) {
    return DispatchStage::TimedOverlay;
  }
  if (player.is_physics_active) {
    return DispatchStage::GroundedPhysics;
  }
  if (flags.player_special_state_active) {
    return DispatchStage::PlayerSpecialState;
  }
  if (player.is_animation_active) {
    return DispatchStage::PlayerAnimation;
  }
  if (player.is_attack_active) {
    return DispatchStage::AttackAnimation;
  }
  if (flags.distance_interaction_active) {
    return DispatchStage::DistanceInteraction;
  }
  if (flags.tile_hazard_triggered) {
    return DispatchStage::TileHazard;
  }
  return DispatchStage::InputHandling;
}

DispatchResult GameDispatcher::run_tick(RuntimeState &state) const {
  const auto stage = choose_stage(state);
  record_stage(stage);
  bool hook_executed = false;

  switch (stage) {
  case DispatchStage::LevelTransition:
    hook_executed = call_hook(level_transition_hook_, state);
    break;
  case DispatchStage::SpecialLogic1:
    hook_executed = call_hook(special_logic1_hook_, state);
    break;
  case DispatchStage::SpecialLogic2:
    hook_executed = call_hook(special_logic2_hook_, state);
    break;
  case DispatchStage::AirbornePhysics:
    hook_executed = call_hook(airborne_physics_hook_, state);
    break;
  case DispatchStage::TimedOverlay:
    hook_executed = call_hook(timed_overlay_hook_, state);
    break;
  case DispatchStage::GroundedPhysics:
    hook_executed = call_hook(grounded_physics_hook_, state);
    break;
  case DispatchStage::PlayerAnimation:
    hook_executed = call_hook(player_animation_hook_, state);
    break;
  case DispatchStage::AttackAnimation:
    hook_executed = call_hook(attack_animation_hook_, state);
    break;
  case DispatchStage::DistanceInteraction:
    hook_executed = call_hook(distance_interaction_hook_, state);
    break;
  case DispatchStage::TileHazard:
    hook_executed = call_hook(tile_hazard_hook_, state);
    break;
  case DispatchStage::PlayerSpecialState:
    hook_executed = call_hook(player_special_state_hook_, state);
    break;
  case DispatchStage::InputHandling:
    hook_executed = call_hook(input_handling_hook_, state);
    break;
  }

  advance_runtime_projectiles(state, kDefaultProjectileBounds,
                              kDefaultViewportMinX, kDefaultViewportMinY,
                              kDefaultViewportWidth, kDefaultViewportHeight);

  if (stage != DispatchStage::LevelTransition &&
      !state.transition_state.active) {
    // 1. Fireball meter counter [DS:0x08ea] (1000:26b1..26f4):
    // - When Fire is not held ([cs:0x25d] != 1) or active mode [0x896] != 0,
    //   calls hud_draw_number_word (1000:3ace), incrementing [0x8ea] up to
    //   0x17 (23).
    // - When Fire is held in mode 0 ([0x896] == 0) and [0x8ea] > 0, calls
    //   hud_draw_char (1000:3b12), decrementing [0x8ea].
    const bool mode0_active = (state.ui.active_mode_mask == 0U ||
                               (state.ui.active_mode_mask & 0x01U) != 0U) &&
                              state.player.active_mode_effect <= 1U;
    if (!state.input.fire_pressed || !mode0_active) {
      if (state.player.fireball_meter_counter < 0x17U) {
        ++state.player.fireball_meter_counter;
      }
    } else if (state.player.fireball_meter_counter > 0U) {
      --state.player.fireball_meter_counter;
    }

    // 2. Shield meter 12-tick fill animation [DS:0x08ed] -> [DS:0x08eb]
    // (1000:29c9..29d4 -> hud_flash_item at 1000:3b51):
    if (state.player.shield_fill_countdown > 0U) {
      --state.player.shield_fill_countdown;
      if (state.player.shield_meter_subbars < 12U) {
        ++state.player.shield_meter_subbars;
      }
    }
  }

  return DispatchResult{.stage = stage, .hook_executed = hook_executed};
}

bool GameDispatcher::call_hook(const StageHook &hook, RuntimeState &state) {
  if (!hook) {
    return false;
  }
  hook(state);
  return true;
}

void GameDispatcher::record_stage(DispatchStage stage) const {
  if (!trace_enabled_) {
    return;
  }
  trace_log_.push_back(stage);
}

const char *to_string(DispatchStage stage) noexcept {
  switch (stage) {
  case DispatchStage::LevelTransition:
    return "LevelTransition";
  case DispatchStage::SpecialLogic1:
    return "SpecialLogic1";
  case DispatchStage::SpecialLogic2:
    return "SpecialLogic2";
  case DispatchStage::AirbornePhysics:
    return "AirbornePhysics";
  case DispatchStage::TimedOverlay:
    return "TimedOverlay";
  case DispatchStage::GroundedPhysics:
    return "GroundedPhysics";
  case DispatchStage::PlayerAnimation:
    return "PlayerAnimation";
  case DispatchStage::AttackAnimation:
    return "AttackAnimation";
  case DispatchStage::DistanceInteraction:
    return "DistanceInteraction";
  case DispatchStage::TileHazard:
    return "TileHazard";
  case DispatchStage::PlayerSpecialState:
    return "PlayerSpecialState";
  case DispatchStage::InputHandling:
    return "InputHandling";
  }
  return "Unknown";
}

} // namespace comic2
