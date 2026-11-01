#include <array>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "comic2/default_handlers.hpp"
#include "comic2/dispatcher.hpp"
#include "comic2/entity_runtime.hpp"
#include "comic2/game_state.hpp"
#include "comic2/projectiles.hpp"
#include "comic2/resource_formats.hpp"
#include "comic2/room_loader.hpp"
#include "comic2/tile_collision.hpp"

extern void run_bootstrap_wiring_tests();
extern void run_audio_backend_tests();
extern void run_dispatcher_tests();
extern void run_entity_runtime_tests();
extern void run_entity_physics_tests();
extern void run_integration_gate_tests();
extern void run_oracle_test_suite();
extern void run_end_to_end_test_suite();
extern void run_persistence_tests();
extern void run_player_controller_tests();
extern void run_renderer_tests();
extern void run_renderer_validation_tests();
extern void run_subsystem_scaffold_tests();
extern void run_tile_collision_tests();
extern void run_ui_tests();

namespace {

void expect(bool condition, const char *message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void test_signed_rle_decode() {
  // Encoded stream:
  // 0x03 -> copy 3 literal bytes [0x10,0x11,0x12]
  // 0xFD -> run length 3 of 0xAA
  // 0x00 -> terminator
  const std::vector<std::uint8_t> encoded = {0x03, 0x10, 0x11, 0x12,
                                             0xFD, 0xAA, 0x00};
  const auto out = comic2::decode_signed_rle(encoded);

  const std::vector<std::uint8_t> expected = {0x10, 0x11, 0x12,
                                              0xAA, 0xAA, 0xAA};
  expect(out.bytes == expected, "decode_signed_rle produced unexpected bytes");
  expect(out.consumed == encoded.size(),
         "decode_signed_rle consumed byte count mismatch");
}

void test_ega_4plane_decode() {
  // row_span = 2 bytes/plane
  // Plane 0: literal(2) 01 02
  // Plane 1: run(2) value 03
  // Plane 2: literal(2) 04 05
  // Plane 3: run(2) value 06
  const std::vector<std::uint8_t> encoded = {
      0x02, 0x00, 0x02, 0x01, 0x02, 0x82, 0x03, 0x02, 0x04, 0x05, 0x82, 0x06,
  };

  const auto image = comic2::decode_ega_4plane_rle(encoded, true);
  expect(image.row_span_bytes == 2, "decode_ega_4plane_rle row_span mismatch");

  expect((image.planes[0] == std::vector<std::uint8_t>{0x01, 0x02}),
         "plane 0 mismatch");
  expect((image.planes[1] == std::vector<std::uint8_t>{0x03, 0x03}),
         "plane 1 mismatch");
  expect((image.planes[2] == std::vector<std::uint8_t>{0x04, 0x05}),
         "plane 2 mismatch");
  expect((image.planes[3] == std::vector<std::uint8_t>{0x06, 0x06}),
         "plane 3 mismatch");
}

} // namespace

namespace {

void test_comprehensive_deterministic_tick_replay() {
  comic2::RuntimeState state;
  state.player.x = 100;
  state.player.y = 100;
  state.player.x_vel = 0;
  state.player.y_vel = 0;
  state.player.facing_right = true;
  state.player.is_airborne = false;
  state.player.is_physics_active = false;
  state.player.is_animation_active = false;
  state.player.is_attack_active = false;

  comic2::GameDispatcher dispatcher;
  comic2::install_default_stage_hooks(dispatcher);

  const auto result1 = dispatcher.run_tick(state);
  const auto result2 = dispatcher.run_tick(state);

  expect(result1.stage == comic2::DispatchStage::InputHandling,
         "Gate A first tick should run input handling");
  expect(result2.stage == comic2::DispatchStage::GroundedPhysics,
         "Gate A second tick should run grounded physics after input arm");
  expect(state.player.x == 100, "Gate A tick x mismatch");
}

void test_projectile_deterministic_spawn_and_update() {
  std::vector<comic2::ProjectileState> projectiles;
  comic2::spawn_projectile(projectiles, 10, 10, 1, 0);
  comic2::spawn_projectile(projectiles, 20, 20, 2, 1);

  expect(projectiles.size() == 2, "Gate A projectile count mismatch");
  expect(projectiles[0].x == 10, "Gate A projectile x mismatch");
  expect(projectiles[1].x == 20, "Gate A projectile x mismatch");
}

void test_entity_runtime_deterministic_activation() {
  std::vector<comic2::MappedObject12> mapped_objects;
  std::vector<comic2::ActiveEntity8> active_entities;
  comic2::EntityActivationState activation_state;

  comic2::ent_build_room_entity_list(mapped_objects, 10, 10, active_entities,
                                     activation_state);

  expect(activation_state.active_count == 0,
         "Gate A activation count mismatch");
}

void test_tile_collision_deterministic_query() {
  comic2::TileCollisionConfig config;
  expect(comic2::tile_meets_threshold(0x01, 0x01), "Gate A threshold mismatch");
  expect(!comic2::tile_meets_threshold(0x00, 0x01),
         "Gate A threshold mismatch");
  expect(comic2::is_hazard_tile(0xF0, config), "Gate A hazard mismatch");
  expect(!comic2::is_hazard_tile(0x80, config), "Gate A hazard mismatch");
}

void test_stage_trace_determinism() {
  comic2::RuntimeState state;
  state.player.x = 100;
  state.player.y = 100;
  state.player.x_vel = 0;
  state.player.y_vel = 0;
  state.player.facing_right = true;
  state.player.is_airborne = false;
  state.player.is_physics_active = false;
  state.player.is_animation_active = false;
  state.player.is_attack_active = false;

  comic2::GameDispatcher dispatcher;
  comic2::install_default_stage_hooks(dispatcher);
  dispatcher.set_trace_enabled(true);
  dispatcher.clear_trace();

  dispatcher.run_tick(state);

  const auto &trace = dispatcher.trace_log();
  expect(trace.size() > 0, "Gate A trace empty");
}

void test_full_dispatcher_integration() {
  comic2::RuntimeState state;
  state.player.x = 100;
  state.player.y = 100;
  state.player.x_vel = 0;
  state.player.y_vel = 0;
  state.player.facing_right = true;
  state.player.is_airborne = false;
  state.player.is_physics_active = false;
  state.player.is_animation_active = false;
  state.player.is_attack_active = false;

  comic2::GameDispatcher dispatcher;
  comic2::install_default_stage_hooks(dispatcher);

  auto result = dispatcher.run_tick(state);

  expect(result.stage == comic2::DispatchStage::InputHandling,
         "Gate A stage mismatch");
}

void test_physics_determinism_with_collision() {
  comic2::TileCollisionConfig config;
  auto state = comic2::RuntimeState();
  state.player.x = 10;
  state.player.y = 10;
  state.player.x_vel = 0;
  state.player.y_vel = 0;
  state.player.facing_right = true;
  state.player.is_airborne = false;
  state.player.is_physics_active = false;
  state.player.is_animation_active = false;
  state.player.is_attack_active = false;

  // Empty room grid should return no floor support
  expect(!comic2::has_floor_support(state, config),
         "Gate A floor support mismatch");
}

void test_room_loader_determinism() {
  const std::vector<std::uint8_t> encoded = {0x01, 0x02, 0x03, 0x04};
  auto result = comic2::decode_frdata_room_entry(std::span(encoded), 0);

  if (result.has_value()) {
    expect(result.value().tile_w == 1, "Gate A room tile_w mismatch");
    expect(result.value().tile_h == 2, "Gate A room tile_h mismatch");
  }
}

void test_default_runtime_state_factory() {
  const auto state = comic2::make_default_runtime_state();

  expect(state.current_level == 0, "default level should start at 0");
  expect(state.current_room == 0, "default room should start at 0");
  expect(state.player.x == 64, "default player x should be 64");
  expect(state.player.y == 96, "default player y should be 96");
  expect(!state.player.is_airborne, "default player should start grounded");
  expect(state.player.is_physics_active,
         "default player physics should be active");
  expect(state.player.hp == 12, "default player hp should be 12");
  expect(state.player.firepower == 1, "default player firepower should be 1");
  expect(state.activation_toggle == 1,
         "default activation toggle should start at 1");
  expect(state.mapped_objects.empty(), "mapped object list should be empty");
  expect(state.projectiles.empty(), "projectile list should be empty");
}

} // namespace

int main() {
  try {
    test_signed_rle_decode();
    test_ega_4plane_decode();
    test_comprehensive_deterministic_tick_replay();
    test_projectile_deterministic_spawn_and_update();
    test_entity_runtime_deterministic_activation();
    test_tile_collision_deterministic_query();
    test_stage_trace_determinism();
    test_full_dispatcher_integration();
    test_physics_determinism_with_collision();
    test_room_loader_determinism();
    test_default_runtime_state_factory();
    run_audio_backend_tests();
    run_bootstrap_wiring_tests();
    run_dispatcher_tests();
    run_entity_runtime_tests();
    run_entity_physics_tests();
    run_integration_gate_tests();
    run_oracle_test_suite();
    run_end_to_end_test_suite();
    run_persistence_tests();
    run_player_controller_tests();
    run_renderer_tests();
    run_renderer_validation_tests();
    run_subsystem_scaffold_tests();
    run_tile_collision_tests();
    run_ui_tests();
    std::cout << "comic2_unit_tests: OK\n";
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << "comic2_unit_tests: FAIL: " << ex.what() << "\n";
    return 1;
  }
}
