#include "comic2/player_controller.hpp"

#include <algorithm>

#include "comic2/audio.hpp"
#include "comic2/projectiles.hpp"

namespace comic2 {

namespace detail {

bool has_active_mode_effect(const RuntimeState &state,
                            std::uint8_t requested_mode) {
  return state.player.active_mode_effect == requested_mode &&
         state.player.mode_effect_ticks > 0U;
}

void update_facing_direction_from_input(RuntimeState &state) {
  const bool move_left = state.input.left_pressed;
  const bool move_right = state.input.right_pressed;

  if (move_left && !move_right) {
    state.player.facing_right = false;
  } else if (move_right && !move_left) {
    state.player.facing_right = true;
  }
}

void apply_horizontal_movement(RuntimeState &state,
                               const PlayerMotionConfig &motion) {
  const bool move_left = state.input.left_pressed;
  const bool move_right = state.input.right_pressed;

  update_facing_direction_from_input(state);

  if (move_left && !move_right) {
    state.player.x_vel = static_cast<std::int16_t>(-motion.walk_step);
    state.player.x += state.player.x_vel;
  } else if (move_right && !move_left) {
    state.player.x_vel = motion.walk_step;
    state.player.x += state.player.x_vel;
  } else {
    state.player.x_vel = 0;
  }
}

bool trigger_fall_if_no_support(RuntimeState &state,
                                const TileCollisionConfig &collision,
                                const PlayerMotionConfig &motion) {
  if (has_floor_support(state, collision)) {
    return false;
  }

  state.player.is_airborne = true;
  state.player.is_physics_active = true;
  if (state.player.y_vel < motion.fall_start_velocity) {
    state.player.y_vel = motion.fall_start_velocity;
  }
  return true;
}

} // namespace detail

void update_animation_state_from_physics(RuntimeState &state) {
  auto &player = state.player;

  // Death takes priority
  if (player.hp == 0) {
    player.animation_state = static_cast<std::uint8_t>(PlayerAnimationState::Death);
    return;
  }

  // Hurt animation
  if (player.damage_recoil_ticks > 0) {
    player.animation_state = static_cast<std::uint8_t>(PlayerAnimationState::Hurt);
    return;
  }

  // Attack animation (active when firing)
  if (player.weapon_cooldown > 0) {
    player.animation_state = static_cast<std::uint8_t>(PlayerAnimationState::Attack);
    return;
  }

  // Airborne states
  if (player.is_airborne) {
    if (player.y_vel < 0) {
      player.animation_state = static_cast<std::uint8_t>(PlayerAnimationState::JumpRise);
    } else {
      player.animation_state = static_cast<std::uint8_t>(PlayerAnimationState::JumpFall);
    }
    return;
  }

  // Horizontal movement
  if (player.x_vel != 0) {
    player.animation_state = static_cast<std::uint8_t>(PlayerAnimationState::WalkCycle);
    return;
  }

  // Default idle
  player.animation_state = static_cast<std::uint8_t>(PlayerAnimationState::Idle);
}

void increment_animation_frame(RuntimeState &state) {
  auto &player = state.player;
  const auto anim_state = static_cast<PlayerAnimationState>(player.animation_state);

  // Only increment frames for animated states
  if (is_animated_state(anim_state)) {
    player.animation_ticks++;
    // Cycle animation frame every 8 ticks
    if (player.animation_ticks >= 8) {
      player.animation_ticks = 0;
      player.animation_frame++;
      // Limit frame count based on state (4 frames is common)
      player.animation_frame = static_cast<std::uint8_t>(player.animation_frame % 4);
    }
  }
}

PlayerMotionConfig
get_effective_motion_config(const RuntimeState &state,
                            const PlayerMotionConfig &motion) {
  PlayerMotionConfig effective_motion = motion;

  if (detail::has_active_mode_effect(state, kModeSpeedBoost)) {
    effective_motion.walk_step =
        static_cast<std::int16_t>(effective_motion.walk_step * 2);
    effective_motion.air_drift_step =
        static_cast<std::int16_t>(effective_motion.air_drift_step * 2);
  }

  // Jump boost from active mode effect or equipped high-jump boots [DS:0x08ae]
  if (state.player.boots_equipped ||
      detail::has_active_mode_effect(state, kModeJumpBoost)) {
    effective_motion.jump_impulse =
        static_cast<std::int16_t>(effective_motion.jump_impulse - 2);
    effective_motion.default_jump_ticks =
        static_cast<std::uint8_t>(effective_motion.default_jump_ticks + 1);
  }

  return effective_motion;
}

bool player_check_jump_tile_collision(const RuntimeState &state,
                                      std::int16_t test_x, std::int16_t test_y,
                                      const TileCollisionConfig &collision,
                                      const PlayerMotionConfig &motion) {
  // Probes tile at feet level (matches player_do_jump 1000:2d61 probe at Y +
  // 0xD). Authentic probe semantics (1000:2c9d): the tile blocks when
  // tile_id <= the solid/ceiling threshold (ds:0x0292).
  const std::int16_t probe_y =
      static_cast<std::int16_t>(test_y + motion.feet_probe_offset_y);
  const auto tile = get_tile_at_pixels(state.room_grid, test_x, probe_y);
  if (!tile.has_value()) {
    return false;
  }
  const std::uint16_t ceiling =
      state.tile_ceiling_threshold != 0U
          ? state.tile_ceiling_threshold
          : static_cast<std::uint16_t>(collision.ceiling_tile_threshold);
  return *tile <= ceiling;
}

void apply_input_tick(RuntimeState &state, const PlayerMotionConfig &motion) {
  if (state.transition_state.player_frozen) {
    state.player.x_vel = 0;
    return;
  }

  const PlayerMotionConfig effective_motion =
      get_effective_motion_config(state, motion);
  const bool was_airborne = state.player.is_airborne;
  detail::apply_horizontal_movement(state, effective_motion);

  if (state.input.jump_pressed && state.player.jump_counter > 0) {
    state.player.is_airborne = true;
    state.player.is_physics_active = true;
    state.player.y_vel = effective_motion.jump_impulse;
    --state.player.jump_counter;
    if (effective_motion.accumulate_air_drift) {
      state.player.x_vel = 0;
    }
    if (!was_airborne) {
      queue_audio_event(state, AudioEvent::Jump);
    }
  }

  if (state.input.fire_pressed) {
    try_player_fire_projectile(state);
  }

  // Update animation state based on current physics
  update_animation_state_from_physics(state);

  // Increment animation frame counter
  increment_animation_frame(state);
}

void apply_grounded_physics_tick(RuntimeState &state,
                                 const PlayerMotionConfig &motion,
                                 const TileCollisionConfig &collision) {
  if (state.transition_state.player_frozen) {
    state.player.x_vel = 0;
    return;
  }

  const PlayerMotionConfig effective_motion =
      get_effective_motion_config(state, motion);
  const bool was_airborne = state.player.is_airborne;
  detail::apply_horizontal_movement(state, effective_motion);

  if (state.input.jump_pressed && state.player.jump_counter > 0) {
    state.player.is_airborne = true;
    state.player.is_physics_active = true;
    state.player.y_vel = effective_motion.jump_impulse;
    --state.player.jump_counter;
    if (effective_motion.accumulate_air_drift) {
      state.player.x_vel = 0;
    }
    if (!was_airborne) {
      queue_audio_event(state, AudioEvent::Jump);
    }
    return;
  }

  if (state.player.y_vel < 0) {
    state.player.is_airborne = true;
    state.player.is_physics_active = true;
    return;
  }

  if (detail::trigger_fall_if_no_support(state, collision, effective_motion)) {
    return;
  }

  resolve_ground_contact(state, collision);

  // Replenish jump counter when supported on ground and jump button released
  // (matches loc_2698: mov AL, [0x899] ... inc AL if boots ... mov [0x898], AL)
  if (!state.player.is_airborne && !state.input.jump_pressed) {
    state.player.jump_counter = effective_motion.default_jump_ticks;
    state.player.jump_counter_max = effective_motion.default_jump_ticks;
  }

  // Update animation state based on current physics
  update_animation_state_from_physics(state);

  // Increment animation frame counter
  increment_animation_frame(state);
}

void apply_airborne_physics_tick(RuntimeState &state,
                                 const PlayerMotionConfig &motion,
                                 const TileCollisionConfig &collision) {
  if (state.transition_state.player_frozen) {
    state.player.x_vel = 0;
    return;
  }

  const PlayerMotionConfig effective_motion =
      get_effective_motion_config(state, motion);

  // Variable jump height: releasing jump key early truncates jump counter
  // (matches loc_2a2c: mov byte [0x898], 0x0 and loc_249f: int 3 with 0x96F6)
  if (effective_motion.truncate_jump_on_release && !state.input.jump_pressed) {
    if (state.player.jump_counter > 0 && state.player.y_vel < 0) {
      queue_audio_event(state, AudioEvent::JumpTruncate);
    }
    state.player.jump_counter = 0;
  }

  if (state.input.jump_pressed && state.player.y_vel < 0 &&
      state.player.jump_counter > 0) {
    state.player.y_vel = effective_motion.jump_impulse;
    --state.player.jump_counter;
  }

  const bool move_left = state.input.left_pressed;
  const bool move_right = state.input.right_pressed;

  detail::update_facing_direction_from_input(state);

  if (effective_motion.accumulate_air_drift) {
    if (move_left && !move_right) {
      state.player.x_vel = static_cast<std::int16_t>(std::max<std::int32_t>(
          state.player.x_vel - effective_motion.air_drift_step,
          -effective_motion.max_air_drift));
    } else if (move_right && !move_left) {
      state.player.x_vel = static_cast<std::int16_t>(std::min<std::int32_t>(
          state.player.x_vel + effective_motion.air_drift_step,
          effective_motion.max_air_drift));
    } else {
      // Friction dampening towards 0 when no horizontal input (matches
      // loc_2a88-2a9e)
      if (state.player.x_vel > 0) {
        --state.player.x_vel;
      } else if (state.player.x_vel < 0) {
        ++state.player.x_vel;
      }
    }
    state.player.x += state.player.x_vel;
  } else {
    // Direct step mode: strictly enforce max_air_drift bound
    const std::int16_t clamped_step =
        (effective_motion.max_air_drift > 0)
            ? std::min(effective_motion.air_drift_step,
                       effective_motion.max_air_drift)
            : effective_motion.air_drift_step;

    if (move_left && !move_right) {
      state.player.x -= clamped_step;
      state.player.x_vel = static_cast<std::int16_t>(-clamped_step);
    } else if (move_right && !move_left) {
      state.player.x += clamped_step;
      state.player.x_vel = clamped_step;
    } else {
      state.player.x_vel = 0;
    }
  }

  state.player.y += state.player.y_vel;

  // Ceiling stop (player_update_physics 1000:2a9e..2ac5): while rising, a
  // solid tile at the head (tile <= ds:0x0292) zeroes the rise and snaps the
  // anchor back down to (y & ~15) + 10.
  if (state.player.y_vel < 0) {
    const auto head_tile =
        get_tile_at_pixels(state.room_grid, state.player.x, state.player.y);
    const std::uint16_t ceiling =
        state.tile_ceiling_threshold != 0U
            ? state.tile_ceiling_threshold
            : static_cast<std::uint16_t>(collision.ceiling_tile_threshold);
    if (head_tile.has_value() && *head_tile <= ceiling) {
      state.player.y_vel = 0;
      state.player.y =
          static_cast<std::int16_t>((state.player.y & 0xFFF0) + 10);
    }
  }

  state.player.y_vel = static_cast<std::int16_t>(std::min<std::int32_t>(
      state.player.y_vel + effective_motion.gravity_per_tick,
      effective_motion.max_fall_speed));

  if (resolve_ground_contact(state, collision)) {
    if (!state.input.jump_pressed) {
      state.player.jump_counter = effective_motion.default_jump_ticks;
      state.player.jump_counter_max = effective_motion.default_jump_ticks;
    }
    if (effective_motion.accumulate_air_drift) {
      state.player.x_vel = 0;
    }
    return;
  }

  state.player.is_airborne = true;
  state.player.is_physics_active = true;

  // Update animation state based on current physics
  update_animation_state_from_physics(state);

  // Increment animation frame counter
  increment_animation_frame(state);
}

void apply_physics_tick(RuntimeState &state, const PlayerMotionConfig &motion,
                        const TileCollisionConfig &collision) {
  if (state.player.is_airborne) {
    apply_airborne_physics_tick(state, motion, collision);
  } else {
    apply_grounded_physics_tick(state, motion, collision);
  }
}

} // namespace comic2
