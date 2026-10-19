#include <stdexcept>

#include "comic2/player_controller.hpp"

namespace {

void expect(bool condition, const char *message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

comic2::RoomTileGrid make_flat_floor_grid() {
  comic2::RoomTileGrid grid;
  grid.tile_w = 2;
  grid.tile_h = 2;
  grid.row_pointers = {0, 2};
  grid.tile_data = {
      0x00,
      0x00,
      0x02,
      0x02,
  };
  return grid;
}

comic2::RoomTileGrid make_three_row_floor_grid() {
  comic2::RoomTileGrid grid;
  grid.tile_w = 2;
  grid.tile_h = 4;
  grid.row_pointers = {0, 2, 4, 6};
  grid.tile_data = {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00,
  };
  return grid;
}

void test_apply_input_tick_moves_right() {
  comic2::RuntimeState state;
  state.input.right_pressed = true;

  comic2::apply_input_tick(state, comic2::PlayerMotionConfig{});

  expect(state.player.x == 8, "right input should move x by walk step");
  expect(state.player.x_vel == 8, "right input should set x_vel");
}

void test_apply_input_tick_consumes_jump_counter() {
  comic2::RuntimeState state;
  state.input.jump_pressed = true;
  state.player.jump_counter = 1;

  comic2::apply_input_tick(state, comic2::PlayerMotionConfig{});

  expect(state.player.y_vel == -5, "jump input should apply jump impulse");
  expect(state.player.jump_counter == 0,
         "jump input should consume jump counter");
  expect(state.player.is_physics_active, "jump input should activate physics");
  expect(state.player.is_airborne, "jump input should enter airborne state");
}

void test_apply_input_tick_updates_facing_direction_from_horizontal_input() {
  comic2::RuntimeState left_state;
  left_state.input.left_pressed = true;

  comic2::apply_input_tick(left_state, comic2::PlayerMotionConfig{});

  expect(!left_state.player.facing_right,
         "left input should set player facing left");

  comic2::RuntimeState right_state;
  right_state.input.right_pressed = true;

  comic2::apply_input_tick(right_state, comic2::PlayerMotionConfig{});

  expect(right_state.player.facing_right,
         "right input should set player facing right");
}

void test_apply_physics_tick_lands_on_ground() {
  comic2::RuntimeState state;
  state.room_grid = make_flat_floor_grid();
  state.player.y = -3;
  state.player.y_vel = 2;
  state.player.is_airborne = true;
  state.player.is_physics_active = true;

  comic2::apply_physics_tick(state, comic2::PlayerMotionConfig{},
                             comic2::TileCollisionConfig{});

  expect(state.player.y == -16,
         "physics tick should snap player to tile floor");
  expect(state.player.y_vel == 0,
         "physics tick should zero vertical velocity on ground");
  expect(!state.player.is_airborne,
         "physics tick should clear airborne on ground contact");
  expect(!state.player.is_physics_active,
         "physics tick should clear physics active on ground contact");
}

void test_jump_while_moving_left_and_right() {
  comic2::RuntimeState left_state;
  left_state.input.left_pressed = true;
  left_state.input.jump_pressed = true;
  left_state.player.jump_counter = 1;

  comic2::apply_input_tick(left_state, comic2::PlayerMotionConfig{});

  expect(left_state.player.x == -8, "left+jump should move left by walk step");
  expect(left_state.player.x_vel == -8,
         "left+jump should set negative x velocity");
  expect(left_state.player.y_vel == -5, "left+jump should apply jump impulse");
  expect(left_state.player.jump_counter == 0,
         "left+jump should consume jump counter");

  comic2::RuntimeState right_state;
  right_state.input.right_pressed = true;
  right_state.input.jump_pressed = true;
  right_state.player.jump_counter = 1;

  comic2::apply_input_tick(right_state, comic2::PlayerMotionConfig{});

  expect(right_state.player.x == 8,
         "right+jump should move right by walk step");
  expect(right_state.player.x_vel == 8,
         "right+jump should set positive x velocity");
  expect(right_state.player.y_vel == -5,
         "right+jump should apply jump impulse");
  expect(right_state.player.jump_counter == 0,
         "right+jump should consume jump counter");
}

void test_short_hop_vs_full_jump_counter_usage() {
  comic2::RuntimeState short_hop;
  short_hop.player.jump_counter = 2;
  short_hop.input.jump_pressed = true;

  comic2::apply_input_tick(short_hop, comic2::PlayerMotionConfig{});
  short_hop.input.jump_pressed = false;
  comic2::apply_airborne_physics_tick(short_hop, comic2::PlayerMotionConfig{},
                                      comic2::TileCollisionConfig{});

  expect(short_hop.player.jump_counter == 1,
         "short-hop should consume exactly one jump counter unit");

  comic2::RuntimeState full_jump;
  full_jump.player.jump_counter = 2;
  full_jump.input.jump_pressed = true;

  comic2::apply_input_tick(full_jump, comic2::PlayerMotionConfig{});
  comic2::apply_airborne_physics_tick(full_jump, comic2::PlayerMotionConfig{},
                                      comic2::TileCollisionConfig{});

  expect(full_jump.player.jump_counter == 0,
         "holding jump during airborne rise should consume additional jump "
         "counter");
}

void test_ledge_walk_off_transitions_into_fall() {
  comic2::RuntimeState state;
  state.player.x = 4;
  state.player.y = 0;
  state.player.is_airborne = false;
  state.player.is_physics_active = true;
  state.input.right_pressed = true;

  const comic2::TileCollisionConfig collision{};

  comic2::apply_input_tick(state, comic2::PlayerMotionConfig{});
  expect(state.player.x == 12,
         "input tick should move player past platform edge");

  comic2::apply_grounded_physics_tick(state, comic2::PlayerMotionConfig{},
                                      collision);

  expect(state.player.is_airborne,
         "walking off ledge should enter airborne state");
  expect(state.player.is_physics_active,
         "walking off ledge should keep physics active");
  expect(state.player.y_vel > 0,
         "walking off ledge should start downward fall velocity");
}

void test_grounded_physics_respects_tile_height_without_ground_y() {
  comic2::RuntimeState state;
  state.room_grid = make_three_row_floor_grid();
  state.player.x = 0;
  state.player.y = 18;
  state.player.y_vel = 0;
  state.player.is_airborne = false;
  state.player.is_physics_active = true;

  const comic2::TileCollisionConfig collision{
      .solid_tile_threshold = 2,
  };

  comic2::apply_grounded_physics_tick(state, comic2::PlayerMotionConfig{},
                                      collision);

  expect(state.player.y == 16,
         "grounded physics should align player to the actual tile top");
  expect(state.player.y_vel == 0,
         "grounded physics should zero vertical velocity after snapping");
  expect(!state.player.is_airborne,
         "grounded physics should clear airborne state when supported");
  expect(!state.player.is_physics_active,
         "grounded physics should clear physics-active state when supported");
}

void test_grounded_physics_does_not_snap_when_rising() {
  comic2::RuntimeState state;
  state.room_grid = make_three_row_floor_grid();
  state.player.x = 0;
  state.player.y = 0;
  state.player.y_vel = -3;
  state.player.is_airborne = false;
  state.player.is_physics_active = true;

  const comic2::TileCollisionConfig collision{
      .solid_tile_threshold = 2,
  };

  comic2::apply_grounded_physics_tick(state, comic2::PlayerMotionConfig{},
                                      collision);

  expect(state.player.y == 0,
         "rising motion should preserve player y when not landing");
  expect(state.player.y_vel == -3,
         "rising motion should preserve upward velocity");
  expect(state.player.is_airborne,
         "rising motion should remain in airborne state");
  expect(state.player.is_physics_active,
         "rising motion should keep physics active");
}

void test_mode_effects_modify_motion_config_for_speed_and_jump() {
  comic2::RuntimeState state;
  state.player.active_mode_effect = 0x01U;
  state.player.mode_effect_ticks = 5U;

  const comic2::PlayerMotionConfig base_motion{};
  const comic2::PlayerMotionConfig effective_motion =
      comic2::get_effective_motion_config(state, base_motion);

  expect(effective_motion.walk_step == 16,
         "speed-boost mode should double walk speed");
  expect(effective_motion.air_drift_step == 2,
         "speed-boost mode should shift drift step to the boosted value");

  state.player.active_mode_effect = 0x04U;
  const comic2::PlayerMotionConfig jump_boost_motion =
      comic2::get_effective_motion_config(state, base_motion);

  expect(jump_boost_motion.jump_impulse == -7,
         "jump-boost mode should increase jump impulse magnitude");
}

void test_inactive_mode_effects_leave_motion_config_unchanged() {
  comic2::RuntimeState state;
  state.player.active_mode_effect = 0x01U;
  state.player.mode_effect_ticks = 0U;

  const comic2::PlayerMotionConfig base_motion{};
  const comic2::PlayerMotionConfig effective_motion =
      comic2::get_effective_motion_config(state, base_motion);

  expect(effective_motion.walk_step == base_motion.walk_step,
         "expired mode effects should not alter movement config");
  expect(effective_motion.jump_impulse == base_motion.jump_impulse,
         "expired mode effects should not alter jump config");
}

void test_verified_dos_motion_constants() {
  expect(comic2::kOriginalDosWalkStep == 8, "DOS walk step should be 8");
  expect(comic2::kOriginalDosJumpImpulse == -7,
         "DOS jump impulse should be -7");
  expect(comic2::kOriginalDosAirDriftStep == 1,
         "DOS air drift step should be 1");
  expect(comic2::kOriginalDosMaxAirDrift == 5,
         "DOS max air drift bound should be 5");
  expect(comic2::kOriginalDosGravityPerTick == 5,
         "DOS gravity per tick should be 5");
  expect(comic2::kOriginalDosMaxFallSpeed == 23,
         "DOS max fall speed should be 23");
  expect(comic2::kOriginalDosFallStartVelocity == 5,
         "DOS fall start velocity should be 5");
  expect(comic2::kOriginalDosFeetProbeOffsetY == 13,
         "DOS feet probe offset Y should be 13");
  expect(comic2::kOriginalDosDefaultJumpTicks == 5,
         "DOS default jump ticks should be 5");
}

void test_verified_dos_jump_physics_curve() {
  comic2::RuntimeState state;
  state.player.jump_counter = 5;
  state.input.jump_pressed = true;

  const auto dos_motion = comic2::make_original_dos_motion_config();
  expect(dos_motion.jump_impulse == -7, "DOS config jump impulse should be -7");
  expect(dos_motion.gravity_per_tick == 5, "DOS config gravity should be 5");
  expect(dos_motion.max_fall_speed == 23,
         "DOS config max fall speed should be 23");

  // Tick 1: Jump pressed on ground
  comic2::apply_input_tick(state, dos_motion);
  expect(state.player.y_vel == -7, "DOS jump tick 1 should set y_vel to -7");
  expect(state.player.jump_counter == 4, "DOS jump tick 1 should decrement jump counter");
  expect(state.player.is_airborne, "DOS jump tick 1 should be airborne");

  // Tick 2: Airborne with jump held -> gravity applies
  comic2::apply_airborne_physics_tick(state, dos_motion,
                                      comic2::TileCollisionConfig{});
  // y_vel was -7, airborne tick with jump_pressed & y_vel < 0 applies impulse (-7),
  // y updates by y_vel, then gravity adds 5: -7 + 5 = -2
  expect(state.player.y_vel == -2, "DOS jump tick 2 should advance y_vel by gravity");
  expect(state.player.jump_counter == 3, "DOS jump tick 2 should consume jump tick");

  // Fall without input to terminal velocity
  state.input.jump_pressed = false;
  state.player.jump_counter = 0;
  state.player.y_vel = 20;
  comic2::apply_airborne_physics_tick(state, dos_motion,
                                      comic2::TileCollisionConfig{});
  // 20 + 5 = 25, clamped to max_fall_speed (23)
  expect(state.player.y_vel == 23,
         "DOS fall velocity should clamp to max fall speed 23");
}

void test_verified_dos_short_hop_truncation() {
  comic2::RuntimeState state;
  state.player.jump_counter = 5;
  state.input.jump_pressed = true;

  const auto dos_motion = comic2::make_original_dos_motion_config();

  // Tick 1: start jump
  comic2::apply_input_tick(state, dos_motion);
  expect(state.player.jump_counter == 4, "first tick consumes 1 jump unit");

  // Player releases jump key early
  state.input.jump_pressed = false;
  comic2::apply_airborne_physics_tick(state, dos_motion,
                                      comic2::TileCollisionConfig{});

  // Releasing jump early with truncate_jump_on_release zeroes jump counter (loc_2a2c)
  expect(state.player.jump_counter == 0,
         "DOS short hop should truncate jump counter to 0 on release");
}

void test_verified_dos_feet_probe_tile_collision() {
  comic2::RuntimeState state;
  state.room_grid = make_flat_floor_grid(); // 2x2 grid, row 0: {0,0}, row 1 (y>=16): {2,2}
  const comic2::TileCollisionConfig collision{.solid_tile_threshold = 2};

  // With feet_offset_y = 13:
  // At player y = 2, probe_y = 2 + 13 = 15 -> tile row 0 (air, tile 0) -> no collision
  expect(!comic2::player_check_jump_tile_collision(state, 0, 2, collision,
                                                   comic2::kOriginalDosFeetProbeOffsetY),
         "feet probe at y=2 should not collide with floor at y=16");

  // At player y = 3, probe_y = 3 + 13 = 16 -> tile row 1 (solid, tile 2) -> collision!
  expect(comic2::player_check_jump_tile_collision(state, 0, 3, collision,
                                                  comic2::kOriginalDosFeetProbeOffsetY),
         "feet probe at y=3 should collide with floor at y=16");
}

void test_confirmed_inventory_fields_and_boots_effect() {
  comic2::RuntimeState state;
  expect(!state.player.boots_equipped, "boots should be unequipped by default");
  expect(!state.player.lantern_equipped, "lantern should be unequipped by default");
  expect(!state.player.corkscrew_equipped, "corkscrew should be unequipped by default");
  expect(!state.player.wand_equipped, "wand should be unequipped by default");
  expect(state.player.key_count == 0, "key count should start at 0");
  expect(state.player.shield_power == 5, "shield power should start at default 5");

  const comic2::PlayerMotionConfig base_motion{};
  const auto unequipped = comic2::get_effective_motion_config(state, base_motion);

  state.player.boots_equipped = true;
  const auto equipped = comic2::get_effective_motion_config(state, base_motion);

  expect(equipped.jump_impulse == unequipped.jump_impulse - 2,
         "equipped boots should enhance jump impulse magnitude");
  expect(equipped.default_jump_ticks == unequipped.default_jump_ticks + 1,
         "equipped boots should increase default jump ticks");
}

} // namespace

void run_player_controller_tests() {
  test_apply_input_tick_moves_right();
  test_apply_input_tick_consumes_jump_counter();
  test_apply_input_tick_updates_facing_direction_from_horizontal_input();
  test_apply_physics_tick_lands_on_ground();
  test_jump_while_moving_left_and_right();
  test_short_hop_vs_full_jump_counter_usage();
  test_ledge_walk_off_transitions_into_fall();
  test_grounded_physics_respects_tile_height_without_ground_y();
  test_grounded_physics_does_not_snap_when_rising();
  test_mode_effects_modify_motion_config_for_speed_and_jump();
  test_inactive_mode_effects_leave_motion_config_unchanged();
  test_verified_dos_motion_constants();
  test_verified_dos_jump_physics_curve();
  test_verified_dos_short_hop_truncation();
  test_verified_dos_feet_probe_tile_collision();
  test_confirmed_inventory_fields_and_boots_effect();
}
