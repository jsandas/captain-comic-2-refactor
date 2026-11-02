#include "comic2/entity_runtime.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>

#include "comic2/audio.hpp"
#include "comic2/entity_physics.hpp"
#include "comic2/game_state.hpp"

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
  EntityPhysicsContext ctx;
  ctx.grid = &state.room_grid;
  ctx.threshold_up =
      state.tile_hazard_bounds[0] != 0
          ? static_cast<std::int16_t>(state.tile_hazard_bounds[0])
          : 1;
  ctx.threshold_down =
      state.tile_hazard_bounds[1] != 0
          ? static_cast<std::int16_t>(state.tile_hazard_bounds[1])
          : 1;
  ctx.threshold_range = -1;
  ctx.camera_x = state.camera_x;
  ctx.camera_y = state.camera_y;

  for (auto &slot : state.runtime_slots) {
    if (!is_runtime_slot_active(slot)) {
      continue;
    }

    auto behavior = slot.behavior_state & 0x000F;
    if (behavior == 0 && slot.type_flags != 0) {
      if (slot.type_flags <= 3) {
        behavior = kBehaviorJump;
      } else if (slot.type_flags <= 7) {
        behavior = kBehaviorBounce;
      }
    }

    if (behavior < static_cast<std::uint16_t>(kBehaviorTable.size())) {
      const auto fn = kBehaviorTable[behavior];
      if (fn != nullptr) {
        fn(slot, state, ctx);
      }
    }
  }
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
