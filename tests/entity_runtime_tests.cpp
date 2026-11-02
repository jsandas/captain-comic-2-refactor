#include <stdexcept>
#include <vector>

#include "comic2/entity_runtime.hpp"
#include "comic2/game_state.hpp"

namespace {

void expect(bool condition, const char *message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void test_entity_behavior_dispatch_handles_chase_bounce_and_jump() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.player.x = 80;
  state.player.y = 64;
  state.player.facing_right = true;

  state.runtime_slots.resize(3);
  state.runtime_slots[0].mapped_object_ptr = 1;
  state.runtime_slots[0].behavior_state = 0x0001;
  state.runtime_slots[0].x = 40;
  state.runtime_slots[0].y = 64;
  state.runtime_slots[0].param_a = 0;
  state.runtime_slots[0].param_b = 0;

  state.runtime_slots[1].mapped_object_ptr = 2;
  state.runtime_slots[1].behavior_state = 0x0002;
  state.runtime_slots[1].x = 60;
  state.runtime_slots[1].y = 70;
  state.runtime_slots[1].param_a = 0;
  state.runtime_slots[1].param_b = 0;

  state.runtime_slots[2].mapped_object_ptr = 3;
  state.runtime_slots[2].behavior_state = 0x0003;
  state.runtime_slots[2].x = 90;
  state.runtime_slots[2].y = 60;
  state.runtime_slots[2].param_a = 0;
  state.runtime_slots[2].param_b = 0;

  comic2::update_entity_behaviors(state);

  expect(state.runtime_slots[0].x > 40,
         "chase behavior should move toward player");
  expect(state.runtime_slots[1].x == 60,
         "bounce behavior should keep horizontal position stable");
  expect(state.runtime_slots[1].y != 70,
         "bounce behavior should change vertical position");
  expect(state.runtime_slots[2].y < 60, "jump behavior should raise entity");
}

void test_entity_damage_and_invulnerability_flow() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.player.x = 50;
  state.player.y = 50;
  state.player.hp = 8;
  state.player.invuln_ticks = 0;

  state.runtime_slots.resize(1);
  state.runtime_slots[0].mapped_object_ptr = 1;
  state.runtime_slots[0].behavior_state = 0x0000;
  state.runtime_slots[0].x = 48;
  state.runtime_slots[0].y = 48;
  state.runtime_slots[0].hitbox_w = 12;
  state.runtime_slots[0].hitbox_h = 12;

  comic2::apply_entity_combat(state);

  expect(state.player.hp == 7, "damage should reduce player hp");
  expect(state.player.invuln_ticks == 12,
         "damage should start invulnerability timer");
  expect(state.player.x < 50, "damage should apply knockback recoil");
}

void test_damage_sets_hurt_animation_state() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.player.x = 50;
  state.player.y = 50;
  state.player.hp = 8;
  state.player.invuln_ticks = 0;
  state.player.animation_state =
      static_cast<std::uint8_t>(comic2::PlayerAnimationState::Idle);

  state.runtime_slots.resize(1);
  state.runtime_slots[0].mapped_object_ptr = 1;
  state.runtime_slots[0].behavior_state = 0x0000;
  state.runtime_slots[0].x = 48;
  state.runtime_slots[0].y = 48;
  state.runtime_slots[0].hitbox_w = 12;
  state.runtime_slots[0].hitbox_h = 12;

  comic2::apply_entity_combat(state);

  expect(state.player.animation_state ==
             static_cast<std::uint8_t>(comic2::PlayerAnimationState::Hurt),
         "damage should switch the player into the hurt animation state");
  expect(state.player.animation_frame == 0,
         "hurt animation should reset the frame index for a fresh flash");
}

void test_pickups_update_state_and_remove_collectibles() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.player.x = 50;
  state.player.y = 50;
  state.player.gems = 2;
  state.player.firepower = 7;
  state.player.score = 10;

  state.runtime_slots.resize(2);
  state.runtime_slots[0].mapped_object_ptr = 1;
  state.runtime_slots[0].behavior_state = 0x0004;
  state.runtime_slots[0].x = 48;
  state.runtime_slots[0].y = 48;
  state.runtime_slots[0].hitbox_w = 8;
  state.runtime_slots[0].hitbox_h = 8;

  state.runtime_slots[1].mapped_object_ptr = 2;
  state.runtime_slots[1].behavior_state = 0x0005;
  state.runtime_slots[1].x = 60;
  state.runtime_slots[1].y = 50;
  state.runtime_slots[1].hitbox_w = 8;
  state.runtime_slots[1].hitbox_h = 8;

  comic2::apply_entity_combat(state);

  expect(state.player.gems == 3, "gem pickup should increase gem count");
  expect(state.player.firepower == 8,
         "powerup pickup should increase firepower up to the cap");
  expect(state.player.score == 160,
         "pickups should increase the player's score for each collectible");
  expect(state.runtime_slots[0].mapped_object_ptr == 0,
         "collected gem should deactivate runtime slot");
  expect(state.runtime_slots[1].mapped_object_ptr == 0,
         "collected powerup should deactivate runtime slot");
}

void test_invulnerability_mode_prevents_damage() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.player.x = 50;
  state.player.y = 50;
  state.player.hp = 8;
  state.player.invuln_ticks = 0;
  state.player.active_mode_effect = 0x02U;
  state.player.mode_effect_ticks = 5U;

  state.runtime_slots.resize(1);
  state.runtime_slots[0].mapped_object_ptr = 1;
  state.runtime_slots[0].behavior_state = 0x0000;
  state.runtime_slots[0].x = 48;
  state.runtime_slots[0].y = 48;
  state.runtime_slots[0].hitbox_w = 12;
  state.runtime_slots[0].hitbox_h = 12;

  comic2::apply_entity_combat(state);

  expect(state.player.hp == 8,
         "invulnerability mode should prevent damage from enemy contact");
  expect(state.player.invuln_ticks == 12,
         "invulnerability mode should grant a fresh invulnerability timer");
  expect(state.player.damage_recoil_ticks == 0,
         "invulnerability mode should avoid recoil from damage");
}

void test_projectile_entity_collision_and_damage_reaction() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.player.score = 0;

  // Setup an enemy with 2 HP at (100, 100)
  state.runtime_slots.resize(1);
  state.runtime_slots[0] = comic2::RuntimeEntitySlot32{};
  state.runtime_slots[0].mapped_object_ptr = 1;
  state.runtime_slots[0].behavior_state = comic2::kBehaviorChase;
  state.runtime_slots[0].x = 100;
  state.runtime_slots[0].y = 100;
  state.runtime_slots[0].hitbox_w = 16;
  state.runtime_slots[0].hitbox_h = 16;
  state.runtime_slots[0].param_b = 2; // 2 HP

  // Fire a projectile traveling right toward the enemy
  comic2::ProjectileState proj{};
  proj.x = 95;
  proj.y = 104;
  proj.x_vel = 16;
  proj.y_vel = 0;
  proj.active = true;
  state.projectiles.push_back(proj);

  // First hit: non-lethal damage reaction
  const auto res1 = comic2::apply_projectile_entity_collisions(state);
  expect(res1.projectiles_hit == 1, "projectile should register 1 hit");
  expect(res1.entities_damaged == 1, "entity should take damage");
  expect(res1.entities_defeated == 0,
         "entity with 2 HP should survive first hit");
  expect(!state.projectiles[0].active,
         "collided projectile should be deactivated");
  expect(state.runtime_slots[0].param_b == 1,
         "entity HP should decrement to 1");
  expect(state.runtime_slots[0].x == 104,
         "entity should recoil rightward (+4)");
  expect(state.runtime_slots[0].anim_tick == 1,
         "entity anim_tick should indicate hit");
  expect(comic2::is_runtime_slot_active(state.runtime_slots[0]),
         "entity should remain active");

  // Second hit: lethal hit
  state.projectiles[0].x = 100;
  state.projectiles[0].y = 104;
  state.projectiles[0].active = true;

  const auto res2 = comic2::apply_projectile_entity_collisions(state);
  expect(res2.projectiles_hit == 1, "second projectile should register 1 hit");
  expect(res2.entities_damaged == 1, "second hit should damage entity");
  expect(res2.entities_defeated == 1, "second hit should defeat 1 HP entity");
  expect(!state.projectiles[0].active,
         "second projectile should be deactivated");
  expect(!comic2::is_runtime_slot_active(state.runtime_slots[0]),
         "defeated entity without drop should be deactivated");
  expect(state.player.score == 100,
         "defeating enemy should award 100 score points");

  // Invulnerable entity test
  state.runtime_slots[0] = comic2::RuntimeEntitySlot32{};
  state.runtime_slots[0].mapped_object_ptr = 2;
  state.runtime_slots[0].behavior_state = comic2::kBehaviorChase;
  state.runtime_slots[0].type_flags = comic2::kEntityInvulnerable;
  state.runtime_slots[0].x = 100;
  state.runtime_slots[0].y = 100;
  state.runtime_slots[0].hitbox_w = 16;
  state.runtime_slots[0].hitbox_h = 16;
  state.runtime_slots[0].param_b = 5;

  state.projectiles[0].active = true;
  state.projectiles[0].x = 100;
  state.projectiles[0].y = 104;

  const auto res3 = comic2::apply_projectile_entity_collisions(state);
  expect(res3.projectiles_hit == 1,
         "projectile should hit invulnerable entity");
  expect(res3.entities_damaged == 0,
         "invulnerable entity should not take damage");
  expect(res3.entities_defeated == 0,
         "invulnerable entity should not be defeated");
  expect(!state.projectiles[0].active,
         "projectile should be absorbed/consumed");
  expect(state.runtime_slots[0].param_b == 5,
         "invulnerable HP should remain unchanged");

  // Projectile passing through collectible pickup test
  state.runtime_slots[0] = comic2::RuntimeEntitySlot32{};
  state.runtime_slots[0].mapped_object_ptr = 3;
  state.runtime_slots[0].behavior_state = comic2::kBehaviorGem;
  state.runtime_slots[0].x = 100;
  state.runtime_slots[0].y = 100;
  state.runtime_slots[0].hitbox_w = 8;
  state.runtime_slots[0].hitbox_h = 8;

  state.projectiles[0].active = true;
  state.projectiles[0].x = 100;
  state.projectiles[0].y = 104;

  const auto res4 = comic2::apply_projectile_entity_collisions(state);
  expect(res4.projectiles_hit == 0,
         "projectile should ignore collectible pickups");
  expect(state.projectiles[0].active,
         "projectile should pass through collectible pickup");
  expect(
      comic2::is_runtime_slot_active(state.runtime_slots[0]),
      "collectible pickup should not be damaged or deactivated by projectile");
}

void test_projectile_entity_defeat_drops_item() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.player.x = 200;
  state.player.y = 200;
  state.player.gems = 0;
  state.player.firepower = 1;

  // Slime enemy configured to drop a gem on defeat
  state.runtime_slots.resize(1);
  state.runtime_slots[0] = comic2::RuntimeEntitySlot32{};
  state.runtime_slots[0].mapped_object_ptr = 1;
  state.runtime_slots[0].behavior_state = comic2::kBehaviorJump;
  state.runtime_slots[0].type_flags = comic2::kEntityDropGem;
  state.runtime_slots[0].x = 80;
  state.runtime_slots[0].y = 80;
  state.runtime_slots[0].hitbox_w = 16;
  state.runtime_slots[0].hitbox_h = 16;
  state.runtime_slots[0].param_b = 0; // single hit defeat

  comic2::ProjectileState proj{};
  proj.x = 80;
  proj.y = 84;
  proj.x_vel = 16;
  proj.y_vel = 0;
  proj.active = true;
  state.projectiles.push_back(proj);

  // Shoot the enemy
  const auto res = comic2::apply_projectile_entity_collisions(state);
  expect(res.entities_defeated == 1, "enemy should be defeated");
  expect(res.items_dropped == 1, "gem drop should be spawned");
  expect(comic2::is_runtime_slot_active(state.runtime_slots[0]),
         "slot should remain active transformed into item pickup");
  expect(state.runtime_slots[0].behavior_state == comic2::kBehaviorGem,
         "slot behavior should be kBehaviorGem");
  expect(state.runtime_slots[0].hitbox_w == 8 &&
             state.runtime_slots[0].hitbox_h == 8,
         "dropped item should have 8x8 hitbox");
  expect(state.runtime_slots[0].x == 80 && state.runtime_slots[0].y == 80,
         "dropped item should appear at defeat position");

  // Move player over the dropped gem and apply combat
  state.player.x = 80;
  state.player.y = 80;
  comic2::apply_entity_combat(state);

  expect(state.player.gems == 1, "player should collect the dropped gem");
  expect(!comic2::is_runtime_slot_active(state.runtime_slots[0]),
         "collected gem slot should be deactivated");

  // Powerup drop test
  state.runtime_slots[0] = comic2::RuntimeEntitySlot32{};
  state.runtime_slots[0].mapped_object_ptr = 2;
  state.runtime_slots[0].behavior_state = comic2::kBehaviorBounce;
  state.runtime_slots[0].type_flags = comic2::kEntityDropPowerup;
  state.runtime_slots[0].x = 120;
  state.runtime_slots[0].y = 60;
  state.runtime_slots[0].hitbox_w = 16;
  state.runtime_slots[0].hitbox_h = 16;

  state.projectiles[0].x = 120;
  state.projectiles[0].y = 64;
  state.projectiles[0].active = true;

  comic2::apply_projectile_entity_collisions(state);
  expect(state.runtime_slots[0].behavior_state == comic2::kBehaviorPowerup,
         "slot behavior should be kBehaviorPowerup");

  state.player.x = 120;
  state.player.y = 60;
  comic2::apply_entity_combat(state);
  expect(state.player.firepower == 2,
         "player should collect the dropped firepower upgrade");
}

void test_spawn_item_drop_in_empty_slot() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.runtime_slots.clear();

  expect(comic2::spawn_item_drop(state, 50, 60, comic2::kBehaviorGem),
         "spawning in empty slot vector should succeed");
  expect(state.runtime_slots.size() == 1, "one slot should be allocated");
  expect(state.runtime_slots[0].x == 50 && state.runtime_slots[0].y == 60,
         "spawned drop position mismatch");
  expect(state.runtime_slots[0].behavior_state == comic2::kBehaviorGem,
         "spawned drop behavior mismatch");

  // Fill up to 6 slots
  for (int i = 1; i < 6; ++i) {
    expect(comic2::spawn_item_drop(state, static_cast<std::int16_t>(i * 10), 60,
                                   comic2::kBehaviorPowerup),
           "spawning up to 6 slots should succeed");
  }
  expect(state.runtime_slots.size() == 6, "total slots should reach 6");
  expect(!comic2::spawn_item_drop(state, 100, 60, comic2::kBehaviorGem),
         "spawning beyond 6 active slots should return false");

  // Deactivate one slot and verify spawn reuses it
  comic2::deactivate_runtime_slot(state.runtime_slots[2]);
  expect(comic2::spawn_item_drop(state, 99, 88, comic2::kBehaviorGem),
         "spawning should reuse deactivated slot");
  expect(state.runtime_slots[2].x == 99 && state.runtime_slots[2].y == 88,
         "reused slot should have new coordinates");
}

} // namespace

void run_entity_runtime_tests() {
  test_entity_behavior_dispatch_handles_chase_bounce_and_jump();
  test_entity_damage_and_invulnerability_flow();
  test_damage_sets_hurt_animation_state();
  test_pickups_update_state_and_remove_collectibles();
  test_invulnerability_mode_prevents_damage();
  test_projectile_entity_collision_and_damage_reaction();
  test_projectile_entity_defeat_drops_item();
  test_spawn_item_drop_in_empty_slot();
}
