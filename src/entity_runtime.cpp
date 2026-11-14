#include "comic2/entity_runtime.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>

#include "comic2/audio.hpp"
#include "comic2/entity_physics.hpp"
#include "comic2/game_state.hpp"
#include "comic2/tile_collision.hpp"

namespace comic2 {

namespace {
constexpr std::int16_t kPlayerHitboxWidth = 16;
constexpr std::int16_t kPlayerHitboxHeight = 32;
constexpr std::uint8_t kInvulnerabilityTicks = 12;
constexpr std::int16_t kDamageKnockback = 6;
[[maybe_unused]] constexpr std::uint16_t kBehaviorChaseValue = kBehaviorChase;
[[maybe_unused]] constexpr std::uint16_t kBehaviorBounceValue = kBehaviorBounce;
[[maybe_unused]] constexpr std::uint16_t kBehaviorJumpValue = kBehaviorJump;
[[maybe_unused]] constexpr std::uint16_t kBehaviorGravityValue =
    kBehaviorGravity;
[[maybe_unused]] constexpr std::uint16_t kBehaviorShootValue = kBehaviorShoot;

using BehaviorFn = void (*)(RuntimeEntitySlot32 &, const RuntimeState &,
                            const EntityPhysicsContext &);

void apply_chase_behavior(RuntimeEntitySlot32 &slot, const RuntimeState &state,
                          const EntityPhysicsContext &ctx) {
  if (ctx.grid == nullptr || ctx.grid->tile_w == 0) {
    if (state.player.x < slot.x) {
      slot.x -= 1;
    } else if (state.player.x > slot.x) {
      slot.x += 1;
    }
    return;
  }

  if (slot.dir_toggle == 0) {
    slot.dir_toggle = (state.player.x < slot.x) ? 2 : 1;
  }
  const std::int16_t speed = slot.param_a != 0 ? std::abs(slot.param_a) : 2;
  ent_move_x_and_bounce(ctx, slot, speed);
  ent_apply_gravity_and_move_y(ctx, slot);
}

void apply_bounce_behavior(RuntimeEntitySlot32 &slot, const RuntimeState &,
                           const EntityPhysicsContext &ctx) {
  if (ctx.grid == nullptr || ctx.grid->tile_w == 0) {
    slot.param_a = static_cast<std::int16_t>((slot.param_a + 1) % 2);
    if (slot.param_a == 0) {
      slot.y -= 1;
    } else {
      slot.y += 1;
    }
    return;
  }

  if (slot.param_b == 0) {
    slot.param_b = 4;
  }
  ent_move_y_and_bounce(ctx, slot);
}

void apply_jump_behavior(RuntimeEntitySlot32 &slot, const RuntimeState &state,
                         const EntityPhysicsContext &ctx) {
  if (ctx.grid == nullptr || ctx.grid->tile_w == 0) {
    if (slot.param_a == 0) {
      slot.param_a = 1;
      slot.y -= 3;
    } else {
      slot.y -= 1;
    }
    return;
  }

  const bool collided = ent_apply_gravity_and_move_y(ctx, slot);
  if (collided && slot.param_b == 0) {
    slot.param_b = -6;
    if (slot.dir_toggle == 0) {
      slot.dir_toggle = (state.player.x < slot.x) ? 2 : 1;
    }
  }
  const std::int16_t speed = slot.param_a != 0 ? std::abs(slot.param_a) : 2;
  ent_move_x_and_bounce(ctx, slot, speed);
}

void apply_gravity_behavior(RuntimeEntitySlot32 &slot, const RuntimeState &,
                            const EntityPhysicsContext &ctx) {
  if (ctx.grid == nullptr || ctx.grid->tile_w == 0) {
    slot.y += 1;
    return;
  }
  ent_apply_gravity_and_move_y(ctx, slot);
}

void apply_shoot_behavior(RuntimeEntitySlot32 &slot, const RuntimeState &state,
                          const EntityPhysicsContext &ctx) {
  if (ctx.grid == nullptr || ctx.grid->tile_w == 0) {
    if (state.player.x < slot.x) {
      slot.x -= 1;
    } else if (state.player.x > slot.x) {
      slot.x += 1;
    }
    if (state.player.y < slot.y) {
      slot.y -= 1;
    } else if (state.player.y > slot.y) {
      slot.y += 1;
    }
    return;
  }

  if (slot.dir_toggle == 0) {
    slot.dir_toggle = (state.player.x < slot.x) ? 2 : 1;
  }
  ent_move_x_and_bounce(ctx, slot, 1);
  if (slot.param_b == 0) {
    slot.param_b = (state.player.y < slot.y) ? -2 : 2;
  }
  ent_move_y_and_bounce(ctx, slot);
}

const std::array<BehaviorFn, 8> kBehaviorTable = {
    nullptr,
    apply_chase_behavior,
    apply_bounce_behavior,
    apply_jump_behavior,
    nullptr,
    nullptr,
    apply_gravity_behavior,
    apply_shoot_behavior,
};

bool collides_with_player(const RuntimeEntitySlot32 &slot,
                          const RuntimeState &state) {
  const std::int16_t player_left = state.player.x;
  const std::int16_t player_top = state.player.y;
  const std::int16_t player_right = player_left + kPlayerHitboxWidth;
  const std::int16_t player_bottom = player_top + kPlayerHitboxHeight;

  const std::int16_t slot_w = slot.hitbox_w > 0 ? slot.hitbox_w : 16;
  const std::int16_t slot_h = slot.hitbox_h > 0 ? slot.hitbox_h : 16;
  const std::int16_t slot_left = slot.x;
  const std::int16_t slot_top = slot.y;
  const std::int16_t slot_right = slot_left + slot_w;
  const std::int16_t slot_bottom = slot_top + slot_h;

  return (slot_right > player_left) && (slot_left < player_right) &&
         (slot_bottom > player_top) && (slot_top < player_bottom);
}

} // namespace

bool is_runtime_slot_active(const RuntimeEntitySlot32 &slot) {
  return slot.mapped_object_ptr != 0;
}

void deactivate_runtime_slot(RuntimeEntitySlot32 &slot) {
  slot.mapped_object_ptr = 0;
  slot.behavior_state = 0;
  slot.type_flags = 0;
}

void ent_deactivate_runtime_slot(RuntimeEntitySlot32 &slot) {
  deactivate_runtime_slot(slot);
}

void prune_inactive_runtime_slots(std::vector<RuntimeEntitySlot32> &slots) {
  slots.erase(std::remove_if(slots.begin(), slots.end(),
                             [](const RuntimeEntitySlot32 &slot) {
                               return !is_runtime_slot_active(slot);
                             }),
              slots.end());
}

void ent_build_room_entity_list(
    const std::vector<MappedObject12> &mapped_objects, std::uint16_t room_x,
    std::uint16_t room_y, std::vector<ActiveEntity8> &active_entities,
    EntityActivationState &activation_state) {
  active_entities.clear();
  activation_state = EntityActivationState{};

  for (std::size_t i = 0; i < mapped_objects.size(); ++i) {
    const auto &obj = mapped_objects[i];
    if (obj.room_x == room_x && obj.room_y == room_y) {
      ActiveEntity8 entity;
      entity.x = static_cast<std::int16_t>(obj.world_x);
      entity.y = static_cast<std::int16_t>(obj.world_y);
      entity.flags_or_slot = obj.state_flags;
      entity.sprite_or_obj = static_cast<std::uint16_t>(i);
      active_entities.push_back(entity);
    }
  }

  std::memset(activation_state.slot_data, 0xFF,
              sizeof(activation_state.slot_data));
  activation_state.first_active_slot = 0;
  activation_state.active_count =
      static_cast<std::uint8_t>(active_entities.size());
}

void ent_build_runtime_slots_for_viewport(
    const std::vector<ActiveEntity8> &active_entities,
    const std::vector<MappedObject12> &mapped_objects,
    std::vector<RuntimeEntitySlot32> &runtime_slots,
    const EntityViewportBounds &viewport_bounds,
    EntityActivationState &activation_state, std::uint16_t &activation_toggle) {
  runtime_slots.clear();
  runtime_slots.resize(6, RuntimeEntitySlot32{});

  std::uint8_t slot_index = 0;

  for (const auto &entity : active_entities) {
    if (slot_index >= 6) {
      break;
    }

    const std::int16_t entity_min_x = entity.x;
    const std::int16_t entity_max_x = entity.x + 16;
    const std::int16_t entity_min_y = entity.y;
    const std::int16_t entity_max_y = entity.y + 16;

    const bool intersects_viewport = (entity_max_x > viewport_bounds.min_x) &&
                                     (entity_min_x < viewport_bounds.max_x) &&
                                     (entity_max_y > viewport_bounds.min_y) &&
                                     (entity_min_y < viewport_bounds.max_y);

    if (intersects_viewport) {
      const std::uint16_t descriptor_idx = entity.sprite_or_obj;
      if (descriptor_idx < mapped_objects.size()) {
        ent_copy_descriptor_to_runtime_slot(mapped_objects[descriptor_idx],
                                            runtime_slots[slot_index],
                                            descriptor_idx, activation_toggle);
        slot_index++;
      }
    }
  }

  for (std::size_t i = slot_index; i < 6; ++i) {
    runtime_slots[i].mapped_object_ptr = 0;
  }

  activation_state.first_active_slot = 0;
  activation_state.active_count = slot_index;
}

void ent_copy_descriptor_to_runtime_slot(const MappedObject12 &descriptor,
                                         RuntimeEntitySlot32 &slot,
                                         std::uint16_t behavior_ptr,
                                         std::uint16_t &activation_toggle) {
  slot.hitbox_w = descriptor.room_x;
  slot.hitbox_h = descriptor.room_y;
  slot.type_flags = descriptor.descriptor_ptr;
  slot.behavior_state = descriptor.state_flags;
  slot.anim_span = descriptor.world_x;
  slot.anim_period = descriptor.world_y;
  slot.unknown_0c = 0;
  slot.unknown_0e = 0;
  slot.x = static_cast<std::int16_t>(descriptor.world_x);
  slot.y = static_cast<std::int16_t>(descriptor.world_y);
  slot.param_a = 0;
  slot.param_b = 0;
  slot.dir_toggle = activation_toggle & 3;
  slot.anim_tick = 0;
  slot.unknown_1c = 0;
  slot.mapped_object_ptr = behavior_ptr;

  activation_toggle ^= 3;
}

void update_entity_behaviors(RuntimeState &state) {
  ent_update_object_behaviors(state);
}

std::uint16_t advance_entity_animation_tick(RuntimeEntitySlot32 &slot) {
  if ((slot.param_a != 0 || slot.param_b != 0) && slot.anim_period > 0) {
    slot.anim_tick =
        static_cast<std::uint16_t>((slot.anim_tick + 1) % slot.anim_period);
    return static_cast<std::uint16_t>(slot.anim_span + slot.anim_tick);
  }
  if (slot.anim_span > 0) {
    slot.anim_tick =
        static_cast<std::uint16_t>((slot.anim_tick + 1) % slot.anim_span);
    return slot.anim_tick;
  }
  slot.anim_tick = static_cast<std::uint16_t>(slot.anim_tick + 1);
  return 0;
}

std::uint16_t compute_entity_animation_frame(const RuntimeEntitySlot32 &slot) {
  if ((slot.param_a != 0 || slot.param_b != 0) && slot.anim_period > 0) {
    return static_cast<std::uint16_t>(slot.anim_span +
                                      (slot.anim_tick % slot.anim_period));
  }
  if (slot.anim_span > 0) {
    return static_cast<std::uint16_t>(slot.anim_tick % slot.anim_span);
  }
  return 0;
}

bool spawn_enemy_projectile(GameState &state, std::int16_t x, std::int16_t y,
                            std::int16_t vx, std::int16_t vy) {
  auto it = std::find_if(
      state.enemy_projectiles.begin(), state.enemy_projectiles.end(),
      [](const EnemyProjectileSlot &slot) { return !slot.active; });
  if (it != state.enemy_projectiles.end()) {
    it->active = true;
    it->x = x;
    it->y = y;
    it->vx = vx;
    it->vy = vy;
    it->anim_frame = 0;
    it->timer = 0;
    return true;
  }
  return false;
}

void trigger_player_death(GameState &state) {
  state.comic_hp = 0;
  state.player.hp = 0;
  state.player.death_timer_ticks = 3;
  state.player.animation_state =
      static_cast<std::uint8_t>(PlayerAnimationState::Death);
  state.player.is_animation_active = true;
  state.transition_state.player_frozen = true;
  queue_audio_event(state, AudioEvent::Death);
  state.flags.player_special_state_active = true;
}

void update_enemy_projectiles(GameState &state) {
  const auto *grid = state.grid != nullptr ? state.grid : &state.room_grid;
  const std::int16_t cam_x = static_cast<std::int16_t>(state.camera_x);
  constexpr std::int16_t kViewW = 240;
  constexpr std::int16_t kViewH = 160;
  constexpr std::uint8_t kDefaultInvulnTicks = 12;

  for (auto &proj : state.enemy_projectiles) {
    if (!proj.active) {
      continue;
    }

    proj.x = static_cast<std::int16_t>(proj.x + proj.vx);
    proj.y = static_cast<std::int16_t>(proj.y + proj.vy);
    ++proj.timer;
    proj.anim_frame = static_cast<std::uint8_t>(proj.timer & 1);

    if (proj.x < cam_x || proj.x >= cam_x + kViewW || proj.y < 0 ||
        proj.y >= kViewH) {
      proj.active = false;
      continue;
    }

    if (grid != nullptr && grid->tile_w > 0) {
      const auto tile = get_tile_at_pixels(*grid, proj.x, proj.y);
      const std::int16_t thresh =
          state.threshold_up != -1 ? state.threshold_up : 1;
      if (!tile.has_value() || static_cast<std::int16_t>(*tile) <= thresh) {
        proj.active = false;
        continue;
      }
    }

    const std::int16_t comic_w = 16;
    const std::int16_t comic_h = 32;
    const std::int16_t proj_w = 8;
    const std::int16_t proj_h = 8;

    const bool x_overlap =
        (proj.x + proj_w > state.comic_x) && (proj.x < state.comic_x + comic_w);
    const bool y_overlap =
        (proj.y + proj_h > state.comic_y) && (proj.y < state.comic_y + comic_h);

    if (x_overlap && y_overlap) {
      proj.active = false;

      if (state.comic_invulnerability_ticks > 0 ||
          state.player.invuln_ticks > 0) {
        continue;
      }

      if (state.comic_shield > 0 || state.player.shield_power > 0) {
        if (state.comic_shield > 0) {
          --state.comic_shield;
        }
        if (state.player.shield_power > 0) {
          --state.player.shield_power;
        }
        state.comic_invulnerability_ticks = kDefaultInvulnTicks;
        state.player.invuln_ticks = kDefaultInvulnTicks;
        queue_audio_event(state, AudioEvent::Hit);
      } else {
        if (state.comic_hp > 0) {
          --state.comic_hp;
          state.player.hp = state.comic_hp;
        }
        state.comic_invulnerability_ticks = kDefaultInvulnTicks;
        state.player.invuln_ticks = kDefaultInvulnTicks;
        if (state.comic_hp == 0) {
          trigger_player_death(state);
        } else {
          queue_audio_event(state, AudioEvent::Hit);
        }
      }
    }
  }
}

void handle_boss_behavior(GameState &state, RuntimeEntitySlot32 &slot) {
  if (slot.unknown_0e == 0) {
    slot.unknown_0e = kDefaultBossHP;
  }
  const std::uint8_t hp = static_cast<std::uint8_t>(slot.unknown_0e);

  std::uint8_t invuln =
      static_cast<std::uint8_t>((slot.unknown_1c >> 8) & 0xFF);
  if (invuln > 0) {
    --invuln;
  }

  std::uint8_t phase = 1;
  if (hp <= 2) {
    phase = 3;
  } else if (hp <= 4) {
    phase = 2;
  }
  slot.unknown_1c = static_cast<std::uint16_t>(
      phase | (static_cast<std::uint16_t>(invuln) << 8));

  ++slot.anim_tick;

  EntityPhysicsContext ctx;
  ctx.grid = state.grid != nullptr ? state.grid : &state.room_grid;
  ctx.threshold_up = state.threshold_up != -1 ? state.threshold_up : 1;
  ctx.threshold_down = state.threshold_down != -1 ? state.threshold_down : 1;
  ctx.threshold_range = state.threshold_range;
  ctx.camera_x = static_cast<std::int16_t>(state.camera_x);
  ctx.camera_y = static_cast<std::int16_t>(state.camera_y);

  if (phase == 1) {
    ent_move_x_and_bounce(ctx, slot, 4);
    const bool landed = ent_apply_gravity_and_move_y(ctx, slot);
    if (landed && (slot.anim_tick % 16 == 0)) {
      slot.param_b = -4;
    }
    if (slot.anim_tick % 20 == 0) {
      const std::int16_t vx = (state.comic_x < slot.x) ? -4 : 4;
      spawn_enemy_projectile(state, slot.x,
                             static_cast<std::int16_t>(slot.y + 4), vx, 0);
    }
  } else if (phase == 2) {
    ent_move_x_and_bounce(ctx, slot, 6);
    const bool landed = ent_apply_gravity_and_move_y(ctx, slot);
    if (landed && (slot.anim_tick % 12 == 0)) {
      slot.param_b = -9;
    }
    if (slot.anim_tick % 16 == 0) {
      const std::int16_t vx = (state.comic_x < slot.x) ? -5 : 5;
      spawn_enemy_projectile(state, slot.x, slot.y, vx, -2);
    }
  } else {
    ent_move_x_and_bounce(ctx, slot, 8);
    const bool landed = ent_apply_gravity_and_move_y(ctx, slot);
    if (landed && (slot.anim_tick % 8 == 0)) {
      slot.param_b = -12;
    }
    if (slot.anim_tick % 10 == 0) {
      const std::int16_t vx = (state.comic_x < slot.x) ? -6 : 6;
      spawn_enemy_projectile(state, slot.x, slot.y, vx, 0);
      spawn_enemy_projectile(state, slot.x, slot.y, vx, -3);
    }
  }
}

void ent_execute_authentic_asm_behavior(GameState &state,
                                        RuntimeEntitySlot32 &slot,
                                        std::uint16_t asm_handler_index) {
  EntityPhysicsContext ctx;
  ctx.grid = state.grid != nullptr ? state.grid : &state.room_grid;
  ctx.threshold_up = state.threshold_up != -1 ? state.threshold_up : 1;
  ctx.threshold_down = state.threshold_down != -1 ? state.threshold_down : 1;
  ctx.threshold_range = state.threshold_range;
  ctx.camera_x = static_cast<std::int16_t>(state.camera_x);
  ctx.camera_y = static_cast<std::int16_t>(state.camera_y);

  switch (asm_handler_index) {
  case 1: { // 1000:50E7 - Behavior 1: Ledge Patrol Walker
    const std::int16_t speed = 4;
    const std::int16_t dx = (slot.dir_toggle == 2) ? -speed : speed;
    slot.param_a = dx;
    const std::int16_t new_x = static_cast<std::int16_t>(slot.x + dx);
    const std::int16_t probe_x =
        (slot.dir_toggle == 2)
            ? new_x
            : static_cast<std::int16_t>(new_x + slot.hitbox_w - 1);
    const bool blocked = ent_probe_x_collision_strict(state, slot, probe_x);
    if (!blocked) {
      const std::int16_t foot_y =
          static_cast<std::int16_t>(slot.y + slot.hitbox_h);
      const std::uint16_t thresh =
          state.threshold_down != -1
              ? static_cast<std::uint16_t>(state.threshold_down)
              : 1U;
      if (!ent_probe_tile_above_threshold(state, probe_x, foot_y, thresh)) {
        slot.x = new_x;
        return;
      }
    }
    slot.dir_toggle = (slot.dir_toggle == 2) ? 1 : 2;
    slot.param_a = static_cast<std::int16_t>(-slot.param_a);
    break;
  }
  case 2: { // 1000:5149 - Behavior 2: Proximity Hopper
    const bool landed = ent_apply_gravity_and_move_y(state, slot);
    if (!landed || slot.param_b != 0) {
      const std::int16_t speed =
          slot.unknown_1c != 0 ? static_cast<std::int16_t>(slot.unknown_1c) : 2;
      ent_move_x_and_bounce(ctx, slot, speed);
    } else {
      const std::int16_t dx = static_cast<std::int16_t>(slot.x - state.comic_x);
      if (std::abs(dx) <= 32) {
        slot.param_b = -9;
        if (dx >= 0) {
          slot.unknown_1c = (slot.dir_toggle == 2) ? 1 : 2;
          slot.dir_toggle = 2;
        } else {
          slot.unknown_1c = (slot.dir_toggle == 1) ? 1 : 2;
          slot.dir_toggle = 1;
        }
      } else {
        slot.param_a = 0;
        slot.param_b = 0;
      }
    }
    break;
  }
  case 3: { // 1000:51BD - Behavior 3: Oscillating Floater
    std::int16_t vy = slot.param_b;
    if (slot.unknown_1c == 0) {
      --vy;
      if (vy < -4)
        vy = -4;
    } else {
      ++vy;
      if (vy > 4)
        vy = 4;
    }
    slot.param_b = vy;
    if (ent_move_y_with_probe_and_bounce(state, slot)) {
      slot.unknown_1c ^= 1;
      slot.param_b = 0;
    }
    ent_move_x_with_range_probe_and_bounce(state, slot, 4);
    break;
  }
  case 4: { // 1000:51F9 - Behavior 4: Timid Follower
    const bool landed = ent_apply_gravity_and_move_y(state, slot);
    if (!landed || slot.param_b != 0) {
      ent_move_x_and_bounce(ctx, slot, 4);
    } else {
      const std::int16_t dx = static_cast<std::int16_t>(slot.x - state.comic_x);
      if (dx > 0) {
        slot.dir_toggle = 2;
        if (!state.player.facing_right) {
          if (ent_move_x_and_bounce(ctx, slot, 4)) {
            slot.param_b = -9;
          }
        } else {
          slot.param_a = 0;
        }
      } else if (dx < 0) {
        slot.dir_toggle = 1;
        if (state.player.facing_right) {
          if (ent_move_x_and_bounce(ctx, slot, 4)) {
            slot.param_b = -9;
          }
        } else {
          slot.param_a = 0;
        }
      } else {
        slot.param_a = 0;
      }
    }
    break;
  }
  case 5: { // 1000:5259 - Behavior 5: Swooper
    std::int16_t speed = 4;
    if (std::abs(state.comic_y - slot.y) <= 8) {
      speed = 8;
    }
    ent_move_x_with_range_probe_and_bounce(state, slot, speed);
    break;
  }
  case 6: { // 1000:5273 - Behavior 6: Flying Homing Chaser
    std::int16_t vy = 2;
    if (state.comic_y < slot.y)
      vy = -2;
    if (state.comic_y != slot.y) {
      slot.param_b = vy;
      ent_move_y_with_probe_and_bounce(state, slot);
    }
    if (slot.x != state.comic_x) {
      slot.dir_toggle = (slot.x > state.comic_x) ? 2 : 1;
      ent_move_x_with_range_probe_and_bounce(state, slot, 4);
    }
    break;
  }
  case 7: { // 1000:52BF - Behavior 7: Static Sentry
    slot.param_a = 0;
    slot.param_b = 0;
    ent_set_horizontal_chase_direction_to_comic(state, slot);
    break;
  }
  case 8: { // 1000:52CF - Behavior 8: Proximity Sentry
    if (slot.unknown_1c == 0) {
      slot.unknown_1c = slot.dir_toggle;
    }
    if (std::abs(slot.x - state.comic_x) <= 32) {
      slot.param_a = 0;
      slot.dir_toggle = slot.unknown_1c;
    } else {
      ent_execute_authentic_asm_behavior(state, slot, 1);
    }
    break;
  }
  case 9: { // 1000:5361 - Behavior 9: Heavy Leaper
    std::int16_t timer = static_cast<std::int16_t>(slot.unknown_1c);
    if (timer <= 0) {
      if (timer == 0) {
        slot.unknown_1c = 12;
        return;
      }
      ++timer;
      slot.unknown_1c = static_cast<std::uint16_t>(timer);
      slot.param_a = 0;
      slot.param_b = 0;
      return;
    }
    if (timer > 1 && slot.y <= state.comic_y) {
      slot.param_b = 7;
      ent_apply_gravity_and_move_y(state, slot);
      --slot.unknown_1c;
    } else {
      slot.param_b = -9;
      const bool landed = ent_apply_gravity_and_move_y(state, slot);
      if (landed && slot.unknown_1c == 1) {
        slot.param_a = 0;
        slot.param_b = 0;
        slot.unknown_1c = static_cast<std::uint16_t>(-3);
        return;
      }
    }
    slot.dir_toggle = (slot.x > state.comic_x) ? 2 : 1;
    if (slot.x != state.comic_x) {
      ent_move_x_and_bounce(ctx, slot, 4);
    }
    break;
  }
  case 10: { // 1000:53E9 - Behavior 10: Ghost Phaser
    const std::int16_t dy = static_cast<std::int16_t>(slot.y - state.comic_y);
    if (dy > 1) {
      slot.param_b = -3;
    } else if (dy < 0) {
      slot.param_b = 1;
    }
    slot.y = static_cast<std::int16_t>(slot.y + slot.param_b);
    ent_set_horizontal_chase_direction_to_comic(state, slot);
    if (slot.x != state.comic_x) {
      const std::int16_t dx = (slot.dir_toggle == 1) ? 4 : -4;
      slot.x = static_cast<std::int16_t>(slot.x + dx);
    }
    break;
  }
  case 11: { // 1000:5438 - Behavior 11: High Bouncer
    if (slot.unknown_1c == 0) {
      slot.unknown_1c = 1;
      slot.param_b = -8;
    }
    ent_move_y_and_bounce(ctx, slot);
    ent_move_x_and_bounce(ctx, slot, 8);
    break;
  }
  case 12: { // 1000:5455 - Behavior 12: Ceiling Drop Hazard
    const bool landed = ent_apply_gravity_and_move_y(state, slot);
    if (landed) {
      if (slot.y < state.comic_y && std::abs(slot.x - state.comic_x) <= 32) {
        slot.y = static_cast<std::int16_t>(slot.y + 8);
        slot.param_b = 7;
        slot.param_a = 0;
        return;
      }
      ent_execute_authentic_asm_behavior(state, slot, 1);
      slot.param_a = 0;
      return;
    }
    slot.param_a = 0;
    break;
  }
  case 13: { // 1000:54EE - Behavior 13: Fast Ledge Patrol
    const std::int16_t speed = 8;
    const std::int16_t dx = (slot.dir_toggle == 2) ? -speed : speed;
    slot.param_a = dx;
    const std::int16_t new_x = static_cast<std::int16_t>(slot.x + dx);
    const std::int16_t probe_x =
        (slot.dir_toggle == 2)
            ? new_x
            : static_cast<std::int16_t>(new_x + slot.hitbox_w - 1);
    const bool blocked = ent_probe_x_collision_strict(state, slot, probe_x);
    if (!blocked) {
      const std::int16_t foot_y =
          static_cast<std::int16_t>(slot.y + slot.hitbox_h);
      const std::uint16_t thresh =
          state.threshold_down != -1
              ? static_cast<std::uint16_t>(state.threshold_down)
              : 1U;
      if (!ent_probe_tile_above_threshold(state, probe_x, foot_y, thresh)) {
        slot.x = new_x;
        return;
      }
    }
    slot.dir_toggle = (slot.dir_toggle == 2) ? 1 : 2;
    slot.param_a = static_cast<std::int16_t>(-slot.param_a);
    break;
  }
  case 14: { // 1000:5550 - Behavior 14: Direction Locked Walker
    if (slot.unknown_1c == 0) {
      slot.unknown_1c = 1;
      ent_set_horizontal_chase_direction_to_comic(state, slot);
    }
    slot.param_b = 0;
    ent_apply_gravity_and_move_y(state, slot);
    ent_move_x_and_bounce(ctx, slot, 4);
    break;
  }
  case 15: { // 1000:5570 - Behavior 15: Ground Pounder
    slot.param_a = 0;
    if (std::abs(slot.x - state.comic_x) <= 8) {
      slot.param_b = -9;
    }
    ent_apply_gravity_and_move_y(state, slot);
    break;
  }
  case 16: { // 1000:5591 - Behavior 16: Diagonal Bouncer
    if (slot.unknown_1c == 0) {
      slot.unknown_1c = 1;
      slot.param_b = (slot.dir_toggle == 2) ? 4 : -4;
    }
    ent_move_y_and_bounce(ctx, slot);
    ent_move_x_and_bounce(ctx, slot, 4);
    break;
  }
  case 17: { // 1000:55B9 - Behavior 17: Trampoline Jumper
    ++slot.param_b;
    const bool landed = ent_apply_gravity_and_move_y(state, slot);
    if (landed) {
      if (slot.param_b >= 0) {
        slot.param_b = -12;
        ent_set_horizontal_chase_direction_to_comic(state, slot);
      }
    } else {
      ent_move_x_and_bounce(ctx, slot, 4);
    }
    break;
  }
  case 18: { // 1000:55DA - Behavior 18: Wall Jumping Runner
    const bool landed = ent_apply_gravity_and_move_y(state, slot);
    if (!landed || slot.param_b != 0) {
      ent_move_x_and_bounce(ctx, slot, 4);
    } else {
      ent_set_horizontal_chase_direction_to_comic(state, slot);
      if (slot.x != state.comic_x) {
        const bool blocked = ent_move_x_and_bounce(ctx, slot, 4);
        if (blocked) {
          slot.param_b = -9;
        }
      } else {
        slot.param_a = 0;
      }
    }
    break;
  }
  case 19: { // 1000:561B - Behavior 19: Drop Trap 1
    if (slot.unknown_1c == 0) {
      const std::int16_t dx = std::abs(state.comic_x - slot.x);
      const std::int16_t dy = static_cast<std::int16_t>(state.comic_y - slot.y);
      if (dy < 0 || dx > dy) {
        slot.param_a = 4;
        return;
      }
      slot.unknown_1c = 1;
    }
    slot.param_a = 0;
    if (slot.unknown_1c == 2) {
      ent_deactivate_runtime_slot(slot);
      return;
    }
    ++slot.param_b;
    if (ent_apply_gravity_and_move_y(state, slot)) {
      slot.unknown_1c = 2;
    }
    break;
  }
  case 20: { // 1000:566A - Behavior 20: Spike Drop Trap 2
    slot.param_a = 0;
    if (slot.unknown_1c == 2) {
      ent_deactivate_runtime_slot(slot);
      return;
    }
    ++slot.param_b;
    if (ent_apply_gravity_and_move_y(state, slot)) {
      slot.unknown_1c = 2;
    }
    break;
  }
  case 21: { // 1000:568C - Behavior 21: Sine Oscillator Hovercraft
    slot.dir_toggle = 2;
    const bool hit = ent_move_x_and_bounce(ctx, slot, 8);
    if (hit) {
      if (slot.x - state.camera_x > 8) {
        slot.x -= 8;
      }
      slot.param_b = -9;
    } else {
      if ((slot.x & 4) != 0) {
        slot.param_b -= 3;
      }
      if (slot.param_b < 2) {
        slot.param_b = 1;
      }
    }
    ent_apply_gravity_and_move_y(state, slot);
    break;
  }
  case 22: { // 1000:56D3 - Behavior 22: Two-Phase Drone
    if (slot.unknown_1c == 0) {
      ent_set_horizontal_chase_direction_to_comic(state, slot);
      slot.unknown_1c = 1;
    }
    if (slot.unknown_1c != 2) {
      const std::int16_t target_y =
          static_cast<std::int16_t>(state.comic_y - 24);
      if (slot.y < target_y) {
        slot.param_b = 3;
        ent_apply_gravity_and_move_y(state, slot);
      } else if (slot.y - target_y >= 4) {
        slot.param_b = -5;
        ent_apply_gravity_and_move_y(state, slot);
      }
      const std::uint16_t prev_dir = slot.dir_toggle;
      ent_move_x_and_bounce(ctx, slot, 8);
      ent_set_horizontal_chase_direction_to_comic(state, slot);
      if (slot.dir_toggle != prev_dir &&
          std::abs(slot.x - state.comic_x) >= 32) {
        slot.unknown_1c = 2;
      }
    } else {
      if (slot.y < state.comic_y) {
        slot.param_b = 7;
        ent_apply_gravity_and_move_y(state, slot);
      } else if (slot.y - state.comic_y >= 8) {
        slot.param_b = -9;
        ent_apply_gravity_and_move_y(state, slot);
      }
      ent_set_horizontal_chase_direction_to_comic(state, slot);
      if (std::abs(slot.x - state.comic_x) >= 8) {
        ent_move_x_and_bounce(ctx, slot, 8);
      }
    }
    break;
  }
  case 23: { // 1000:576D - Behavior 23: Fast Rammer
    const bool landed = ent_apply_gravity_and_move_y(state, slot);
    if (!landed || slot.param_b != 0) {
      ent_move_x_and_bounce(ctx, slot, 8);
    } else {
      ent_set_horizontal_chase_direction_to_comic(state, slot);
      if (slot.x != state.comic_x) {
        if (std::abs(slot.x - state.comic_x) <= 32) {
          slot.param_b = -8;
        }
        const bool blocked = ent_move_x_and_bounce(ctx, slot, 8);
        if (blocked) {
          slot.param_b = -9;
        }
      } else {
        slot.param_a = 0;
      }
    }
    break;
  }
  case 24: { // 1000:57C3 - Behavior 24: Ceiling Drop Ghost
    slot.param_a = 0;
    if (slot.unknown_1c == 0) {
      const std::int16_t dx = std::abs(state.comic_x - slot.x);
      const std::int16_t dy = static_cast<std::int16_t>(state.comic_y - slot.y);
      if (dy < 0 || dx > dy) {
        return;
      }
      slot.unknown_1c = 1;
    }
    ++slot.param_b;
    slot.y = static_cast<std::int16_t>(slot.y + slot.param_b);
    break;
  }
  case 25: { // 1000:5814 - Behavior 25: Inverted Ceiling Crawler
    slot.param_b = 4;
    slot.unknown_1c = 1;
    if (slot.dir_toggle != 2) {
      slot.unknown_1c = 2;
      slot.param_b = -4;
    }
    if (ent_move_y_and_bounce(ctx, slot)) {
      slot.dir_toggle = slot.unknown_1c;
    }
    break;
  }
  case 26: { // 1000:583F - Behavior 26: Straight Horiz Cruiser
    slot.param_b = 0;
    slot.param_a = 4;
    ent_move_x_and_bounce(ctx, slot, 4);
    break;
  }
  case 27: { // 1000:5852 - Behavior 27: Boss Walker
    handle_boss_behavior(state, slot);
    break;
  }
  case 28: { // 1000:58AD - Behavior 28: Boss Swooper
    handle_boss_behavior(state, slot);
    break;
  }
  case 29: { // 1000:5901 - Behavior 29: Leftward Dart
    slot.dir_toggle = 2;
    slot.param_b = 0;
    slot.param_a = 4;
    if (ent_move_x_and_bounce(ctx, slot, 4)) {
      ent_deactivate_runtime_slot(slot);
    }
    break;
  }
  case 30: { // 1000:5920 - Behavior 30: Torpedo Sprinter
    slot.param_a = 0;
    slot.param_b = 0;
    ent_set_horizontal_chase_direction_to_comic(state, slot);
    const std::int16_t dx = std::abs(slot.x - state.comic_x);
    if (slot.unknown_1c == 1 || dx <= 16) {
      slot.unknown_1c = 1;
      if (dx <= 48) {
        slot.dir_toggle = 1;
        slot.param_b = 0;
        slot.param_a = 8;
        if (ent_move_x_and_bounce(ctx, slot, 8)) {
          ent_deactivate_runtime_slot(slot);
        }
        return;
      }
    }
    slot.unknown_1c = 0;
    break;
  }
  case 31: { // 1000:596C - Behavior 31: Heavy Stalactite
    if (slot.unknown_1c == 0) {
      const std::int16_t dx = std::abs(state.comic_x - slot.x);
      const std::int16_t dy =
          static_cast<std::int16_t>(state.comic_y - slot.y - slot.hitbox_h);
      if (dy < 0 || dx > dy) {
        slot.param_a = 4;
        return;
      }
      slot.unknown_1c = 1;
    }
    slot.param_a = 0;
    if (slot.unknown_1c == 2) {
      ent_deactivate_runtime_slot(slot);
      return;
    }
    ++slot.param_b;
    if (ent_apply_gravity_and_move_y(state, slot)) {
      slot.unknown_1c = 2;
    }
    break;
  }
  case 32: { // 1000:59C1 - Behavior 32: Left Swoop Diver
    slot.dir_toggle = 2;
    if (slot.unknown_1c != 2) {
      slot.unknown_1c = 1;
      if (ent_move_x_and_bounce(ctx, slot, 12)) {
        ent_deactivate_runtime_slot(slot);
        return;
      }
      slot.param_b = 12;
      if (ent_apply_gravity_and_move_y(state, slot)) {
        slot.unknown_1c = 2;
        slot.param_b = 0;
        slot.param_a = 0;
      }
    } else {
      ent_deactivate_runtime_slot(slot);
    }
    break;
  }
  case 33: { // 1000:59C9 - Behavior 33: Right Swoop Diver
    if (slot.unknown_1c == 0) {
      const std::int16_t dx = state.comic_x - slot.x;
      if (dx < 0 || dx >= 72) {
        slot.param_b = 1;
        return;
      }
    }
    slot.dir_toggle = 1;
    if (slot.unknown_1c != 2) {
      slot.unknown_1c = 1;
      if (ent_move_x_and_bounce(ctx, slot, 12)) {
        ent_deactivate_runtime_slot(slot);
        return;
      }
      slot.param_b = 12;
      if (ent_apply_gravity_and_move_y(state, slot)) {
        slot.unknown_1c = 2;
        slot.param_b = 0;
        slot.param_a = 0;
      }
    } else {
      ent_deactivate_runtime_slot(slot);
    }
    break;
  }
  default:
    break;
  }
}

void ent_update_object_behaviors(GameState &state) {
  state.comic_x = state.player.x;
  state.comic_y = state.player.y;
  state.comic_hp = state.player.hp;
  state.comic_shield = state.player.shield_power;
  state.comic_invulnerability_ticks = state.player.invuln_ticks;

  EntityPhysicsContext ctx;
  ctx.grid = state.grid != nullptr ? state.grid : &state.room_grid;
  ctx.threshold_up =
      state.tile_hazard_bounds[0] != 0
          ? static_cast<std::int16_t>(state.tile_hazard_bounds[0])
          : (state.threshold_up != -1 ? state.threshold_up : 1);
  ctx.threshold_down =
      state.tile_hazard_bounds[1] != 0
          ? static_cast<std::int16_t>(state.tile_hazard_bounds[1])
          : (state.threshold_down != -1 ? state.threshold_down : 1);
  ctx.threshold_range = state.threshold_range;
  ctx.camera_x = static_cast<std::int16_t>(state.camera_x);
  ctx.camera_y = static_cast<std::int16_t>(state.camera_y);

  state.threshold_up = ctx.threshold_up;
  state.threshold_down = ctx.threshold_down;

  for (auto &slot : state.runtime_slots) {
    if (!is_runtime_slot_active(slot)) {
      continue;
    }

    auto behavior = slot.behavior_state;
    if (behavior == 0 && slot.type_flags != 0) {
      if (slot.type_flags <= 3) {
        behavior = kBehaviorJump;
      } else if (slot.type_flags <= 7) {
        behavior = kBehaviorBounce;
      }
    }

    if (behavior == kBehaviorBoss || behavior == 27 || behavior == 28) {
      handle_boss_behavior(state, slot);
    } else if (behavior == kBehaviorShoot) {
      apply_shoot_behavior(slot, state, ctx);
      ++slot.unknown_1c;
      if (slot.unknown_1c >= 24) {
        slot.unknown_1c = 0;
        const std::int16_t vx = (state.comic_x < slot.x) ? -4 : 4;
        spawn_enemy_projectile(state, slot.x,
                               static_cast<std::int16_t>(slot.y + 4), vx, 0);
      }
    } else if (behavior >= 1 && behavior <= 6) {
      const auto fn = kBehaviorTable[behavior];
      if (fn != nullptr) {
        fn(slot, state, ctx);
      }
    } else if (behavior >= 9 && behavior <= 33) {
      ent_execute_authentic_asm_behavior(state, slot, behavior);
    }

    advance_entity_animation_tick(slot);
  }

  update_enemy_projectiles(state);

  state.player.x = state.comic_x;
  state.player.y = state.comic_y;
  state.player.hp = state.comic_hp;
  state.player.shield_power = state.comic_shield;
  state.player.invuln_ticks = state.comic_invulnerability_ticks;
}

bool collides_with_projectile(const RuntimeEntitySlot32 &slot,
                              const ProjectileState &projectile) {
  if (!projectile.active || !is_runtime_slot_active(slot)) {
    return false;
  }
  const std::int16_t proj_left = projectile.x - 4;
  const std::int16_t proj_right = projectile.x + 12;
  const std::int16_t proj_top = projectile.y;
  const std::int16_t proj_bottom = projectile.y + 8;

  const std::int16_t ent_w = slot.hitbox_w > 0 ? slot.hitbox_w : 16;
  const std::int16_t ent_h = slot.hitbox_h > 0 ? slot.hitbox_h : 16;
  const std::int16_t ent_left = slot.x;
  const std::int16_t ent_top = slot.y;
  const std::int16_t ent_right = ent_left + ent_w;
  const std::int16_t ent_bottom = ent_top + ent_h;

  return (proj_right > ent_left) && (proj_left < ent_right) &&
         (proj_bottom > ent_top) && (proj_top < ent_bottom);
}

ProjectileCombatResult apply_projectile_entity_collisions(RuntimeState &state) {
  ProjectileCombatResult result;

  for (auto &projectile : state.projectiles) {
    if (!projectile.active) {
      continue;
    }

    for (auto &slot : state.runtime_slots) {
      if (!is_runtime_slot_active(slot)) {
        continue;
      }

      const auto behavior = slot.behavior_state & 0x000F;
      if (behavior == kBehaviorGem || behavior == kBehaviorPowerup) {
        continue;
      }

      if (!collides_with_projectile(slot, projectile)) {
        continue;
      }

      // Hit detected: projectile is consumed
      projectile.active = false;
      ++result.projectiles_hit;

      if ((slot.type_flags & kEntityInvulnerable) != 0) {
        break;
      }

      const bool is_boss =
          (behavior == kBehaviorBoss || slot.behavior_state == 8 ||
           slot.behavior_state == 27 || slot.behavior_state == 28);
      if (is_boss) {
        const std::uint8_t boss_invuln =
            static_cast<std::uint8_t>((slot.unknown_1c >> 8) & 0xFF);
        if (boss_invuln > 0) {
          break;
        }

        if (slot.unknown_0e == 0) {
          slot.unknown_0e = kDefaultBossHP;
        }

        --slot.unknown_0e;
        slot.unknown_1c =
            (slot.unknown_1c & 0x00FF) |
            (static_cast<std::uint16_t>(kBossInvulnerabilityTicks) << 8);

        if (slot.unknown_0e > 0) {
          ++result.entities_damaged;
          state.player.score =
              static_cast<std::uint16_t>(state.player.score + 200U);
          queue_audio_event(state, AudioEvent::Hit);
        } else {
          ++result.entities_damaged;
          ++result.entities_defeated;
          state.player.score =
              static_cast<std::uint16_t>(state.player.score + 1000U);
          queue_audio_event(state, AudioEvent::Death);
          state.boss_victory = true;
          state.level_complete = true;
          state.ui.level_complete_modal = true;
          if (slot.mapped_object_ptr < state.mapped_objects.size()) {
            state.mapped_objects[slot.mapped_object_ptr].state_flags = 2;
          }
          deactivate_runtime_slot(slot);
        }
        break;
      }

      // Multi-hit durability if param_b > 1
      if (slot.param_b > 1) {
        --slot.param_b;
        if (projectile.x_vel > 0) {
          slot.x += 4;
        } else if (projectile.x_vel < 0) {
          slot.x -= 4;
        }
        slot.anim_tick = 1;
        queue_audio_event(state, AudioEvent::Hit);
        ++result.entities_damaged;
      } else {
        // Lethal hit
        ++result.entities_damaged;
        ++result.entities_defeated;
        state.player.score =
            static_cast<std::uint16_t>(state.player.score + 100U);
        queue_audio_event(state, AudioEvent::Hit);

        if (slot.mapped_object_ptr < state.mapped_objects.size()) {
          state.mapped_objects[slot.mapped_object_ptr].state_flags = 2;
        }

        if ((slot.type_flags & kEntityDropPowerup) != 0) {
          slot.behavior_state = kBehaviorPowerup;
          slot.hitbox_w = 8;
          slot.hitbox_h = 8;
          slot.mapped_object_ptr = 0xFFFF;
          slot.type_flags = 0;
          slot.param_a = 0;
          slot.param_b = 0;
          ++result.items_dropped;
        } else if ((slot.type_flags & kEntityDropGem) != 0) {
          slot.behavior_state = kBehaviorGem;
          slot.hitbox_w = 8;
          slot.hitbox_h = 8;
          slot.mapped_object_ptr = 0xFFFF;
          slot.type_flags = 0;
          slot.param_a = 0;
          slot.param_b = 0;
          ++result.items_dropped;
        } else {
          deactivate_runtime_slot(slot);
        }
      }
      break;
    }
  }

  return result;
}

bool spawn_item_drop(RuntimeState &state, std::int16_t x, std::int16_t y,
                     std::uint16_t item_behavior) {
  const auto it =
      std::find_if(state.runtime_slots.begin(), state.runtime_slots.end(),
                   [](const RuntimeEntitySlot32 &slot) {
                     return !is_runtime_slot_active(slot);
                   });
  if (it != state.runtime_slots.end()) {
    it->hitbox_w = 8;
    it->hitbox_h = 8;
    it->type_flags = 0;
    it->behavior_state = item_behavior;
    it->anim_span = 0;
    it->anim_period = 0;
    it->unknown_0c = 0;
    it->unknown_0e = 0;
    it->x = x;
    it->y = y;
    it->param_a = 0;
    it->param_b = 0;
    it->dir_toggle = 0;
    it->anim_tick = 0;
    it->unknown_1c = 0;
    it->mapped_object_ptr = 0xFFFF;
    return true;
  }
  if (state.runtime_slots.size() < 6) {
    RuntimeEntitySlot32 slot{};
    slot.hitbox_w = 8;
    slot.hitbox_h = 8;
    slot.type_flags = 0;
    slot.behavior_state = item_behavior;
    slot.anim_span = 0;
    slot.anim_period = 0;
    slot.unknown_0c = 0;
    slot.unknown_0e = 0;
    slot.x = x;
    slot.y = y;
    slot.param_a = 0;
    slot.param_b = 0;
    slot.dir_toggle = 0;
    slot.anim_tick = 0;
    slot.unknown_1c = 0;
    slot.mapped_object_ptr = 0xFFFF;
    state.runtime_slots.push_back(slot);
    return true;
  }
  return false;
}

void apply_entity_combat(RuntimeState &state) {
  apply_projectile_entity_collisions(state);
  update_enemy_projectiles(state);

  const bool mode_invulnerability_active =
      state.player.active_mode_effect == 0x02U &&
      state.player.mode_effect_ticks > 0U;

  if (state.player.invuln_ticks > 0) {
    --state.player.invuln_ticks;
  }
  if (mode_invulnerability_active) {
    state.player.invuln_ticks =
        std::max(state.player.invuln_ticks, kInvulnerabilityTicks);
  }
  if (state.player.damage_recoil_ticks > 0) {
    --state.player.damage_recoil_ticks;
  }

  bool damage_applied = false;
  std::int16_t recoil_x = 0;
  std::int16_t recoil_y = 0;
  for (auto &slot : state.runtime_slots) {
    if (!is_runtime_slot_active(slot)) {
      continue;
    }

    if (!collides_with_player(slot, state)) {
      continue;
    }

    const auto behavior = slot.behavior_state & 0x000F;
    if (behavior == kBehaviorGem) {
      ++state.player.gems;
      state.player.score =
          static_cast<std::uint16_t>(state.player.score + 100U);
      deactivate_runtime_slot(slot);
      continue;
    }
    if (behavior == kBehaviorPowerup) {
      state.player.firepower = static_cast<std::uint8_t>(
          std::min<std::uint8_t>(state.player.firepower + 1, 8));
      state.player.score = static_cast<std::uint16_t>(state.player.score + 50U);
      deactivate_runtime_slot(slot);
      continue;
    }

    if (damage_applied || state.player.invuln_ticks != 0) {
      continue;
    }

    if (mode_invulnerability_active) {
      continue;
    }

    if (state.player.hp > 0) {
      --state.player.hp;
    }
    if (state.player.hp == 0) {
      queue_audio_event(state, AudioEvent::Death);
    } else {
      queue_audio_event(state, AudioEvent::Hit);
    }
    state.player.invuln_ticks = kInvulnerabilityTicks;
    state.player.damage_recoil_ticks = 4;
    state.player.animation_state =
        static_cast<std::uint8_t>(PlayerAnimationState::Hurt);
    state.player.animation_frame = 0;
    state.player.animation_ticks = 0;
    recoil_x -= kDamageKnockback;
    recoil_y -= 2;
    damage_applied = true;
  }

  state.player.x += recoil_x;
  state.player.y += recoil_y;
}

} // namespace comic2
