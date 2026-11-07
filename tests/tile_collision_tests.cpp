#include <optional>
#include <stdexcept>
#include <vector>

#include "comic2/room_loader.hpp"
#include "comic2/tile_collision.hpp"

namespace {

void expect(bool condition, const char *message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

comic2::RoomTileGrid make_grid_fixture() {
  comic2::RoomTileGrid grid;
  grid.tile_w = 4;
  grid.tile_h = 3;
  grid.row_pointers = {10, 20, 30};
  grid.tile_data.assign(64, 0);

  grid.tile_data[10] = 1;
  grid.tile_data[11] = 2;
  grid.tile_data[12] = 3;
  grid.tile_data[13] = 4;

  grid.tile_data[20] = 5;
  grid.tile_data[21] = 6;
  grid.tile_data[22] = 7;
  grid.tile_data[23] = 8;

  grid.tile_data[30] = 9;
  grid.tile_data[31] = 10;
  grid.tile_data[32] = 11;
  grid.tile_data[33] = 12;
  return grid;
}

void test_get_tile_at_pixels_maps_to_expected_tile_id() {
  const comic2::RoomTileGrid grid = make_grid_fixture();
  std::optional<std::uint8_t> tile_id = comic2::get_tile_at_pixels(grid, 0, 0);

  expect(tile_id.has_value(), "query at origin should succeed");
  expect(*tile_id == 1, "origin pixel should map to first tile");

  tile_id = comic2::get_tile_at_pixels(grid, 16, 0);
  expect(tile_id.has_value(), "query at next tile x should succeed");
  expect(*tile_id == 2, "x=16 should map to tile_x=1");

  tile_id = comic2::get_tile_at_pixels(grid, 63, 47);
  expect(tile_id.has_value(), "bottom-right in bounds should succeed");
  expect(*tile_id == 12, "bottom-right pixel should map to final tile");
}

void test_get_tile_at_pixels_rejects_out_of_bounds() {
  const comic2::RoomTileGrid grid = make_grid_fixture();

  expect(!comic2::get_tile_at_pixels(grid, -1, 0).has_value(),
         "negative x should fail");
  expect(!comic2::get_tile_at_pixels(grid, 0, -1).has_value(),
         "negative y should fail");
  expect(!comic2::get_tile_at_pixels(grid, 64, 0).has_value(),
         "x at width boundary should fail");
  expect(!comic2::get_tile_at_pixels(grid, 0, 48).has_value(),
         "y at height boundary should fail");
  expect(!comic2::get_tile_at_pixels(grid, 63, 48).has_value(),
         "bottom-right corner out of bounds should fail");
}

void test_has_floor_support_tile_grid_only() {
  comic2::RuntimeState state;
  state.room_grid = make_grid_fixture();
  state.player.x = 0;
  comic2::TileCollisionConfig config;
  config.solid_tile_threshold = 2;

  // feet probe at y=31 maps to tile row 1, id=5 (solid)
  state.player.y = -1;
  expect(comic2::has_floor_support(state, config),
         "should have support on solid tile");

  // feet probe at y=32 maps to tile row 2, id=9 (solid)
  state.player.y = 0;
  expect(comic2::has_floor_support(state, config),
         "should have support on lower solid tile");

  // feet probe at y=0 maps to tile row 0, id=1 (below threshold)
  state.player.y = -32;
  expect(!comic2::has_floor_support(state, config),
         "should not have support below threshold");

  // out of bounds below grid
  state.player.y = 48;
  expect(!comic2::has_floor_support(state, config),
         "should not have support out of bounds");
}

void test_resolve_ground_contact_snaps_to_tile_top() {
  comic2::RuntimeState state;
  state.room_grid = make_grid_fixture();
  state.player.x = 0;
  state.player.y = 2;
  state.player.y_vel = 3;
  state.player.is_airborne = true;
  state.player.is_physics_active = true;

  comic2::TileCollisionConfig config;
  config.solid_tile_threshold = 2;

  const bool landed = comic2::resolve_ground_contact(state, config);
  expect(landed, "resolve_ground_contact should land on solid support");
  expect(state.player.y == 0,
         "resolve_ground_contact should align player to tile top");
  expect(state.player.y_vel == 0,
         "resolve_ground_contact should clear vertical velocity");
  expect(!state.player.is_airborne,
         "resolve_ground_contact should clear airborne state");
  expect(!state.player.is_physics_active,
         "resolve_ground_contact should clear physics-active state");
}

void test_room_loader_builds_row_pointer_table() {
  std::vector<std::uint8_t> bytes(0x2C4, 0);

  bytes[0x2A0] = 0xB0;
  bytes[0x2A1] = 0x02;
  bytes[0x2A2] = 0xC0;
  bytes[0x2A3] = 0x02;

  const std::optional<std::vector<std::uint16_t>> row_pointers =
      comic2::build_room_row_pointer_table(bytes, 2);

  expect(row_pointers.has_value(), "row pointer table build should succeed");
  expect(row_pointers->size() == 2, "row pointer count should match tile_h");
  expect((*row_pointers)[0] == 0x02B0, "first row pointer mismatch");
  expect((*row_pointers)[1] == 0x02C0, "second row pointer mismatch");
}

void test_tile_threshold_and_hazard_checks() {
  expect(comic2::tile_meets_threshold(7, 7),
         "tile equal to threshold should pass");
  expect(comic2::tile_meets_threshold(8, 7),
         "tile above threshold should pass");
  expect(!comic2::tile_meets_threshold(6, 7),
         "tile below threshold should fail");

  comic2::TileCollisionConfig config;
  config.hazard_tile_min = 0xA0;
  config.hazard_tile_max = 0xAF;

  expect(!comic2::is_hazard_tile(0x9F, config),
         "below hazard band should fail");
  expect(comic2::is_hazard_tile(0xA0, config),
         "hazard band lower bound should pass");
  expect(comic2::is_hazard_tile(0xA7, config),
         "inside hazard band should pass");
  expect(comic2::is_hazard_tile(0xAF, config),
         "hazard band upper bound should pass");
  expect(!comic2::is_hazard_tile(0xB0, config),
         "above hazard band should fail");
}

void test_update_player_hazard_state_checks_feet_for_hazards() {
  comic2::RuntimeState state;
  state.room_grid = make_grid_fixture();
  state.player.x = 0;
  state.player.y = -1;

  comic2::TileCollisionConfig config;
  config.hazard_tile_min = 5;
  config.hazard_tile_max = 5;

  state.room_grid.tile_data[20] = 5;
  state.flags.tile_hazard_triggered = false;

  expect(comic2::update_player_hazard_state(state, config),
         "hazard should be triggered by tile under player feet");
  expect(state.flags.tile_hazard_triggered,
         "tile hazard flag should be set when feet probe is hazardous");
}

void test_set_tile_at_coords_and_pixels() {
  comic2::RoomTileGrid grid = make_grid_fixture();
  expect(comic2::set_tile_at_coords(grid, 0, 0, 42),
         "setting origin tile should succeed");
  auto tile = comic2::get_tile_at_pixels(grid, 0, 0);
  expect(tile.has_value() && *tile == 42,
         "origin tile should be updated to 42");

  expect(comic2::set_tile_at_pixels(grid, 16, 0, 99),
         "setting tile by pixel should succeed");
  tile = comic2::get_tile_at_pixels(grid, 16, 0);
  expect(tile.has_value() && *tile == 99,
         "tile at (16,0) should be updated to 99");

  // Rejections out of bounds
  expect(!comic2::set_tile_at_coords(grid, 4, 0, 1),
         "out of bounds x should fail");
  expect(!comic2::set_tile_at_coords(grid, 0, 3, 1),
         "out of bounds y should fail");
  expect(!comic2::set_tile_at_pixels(grid, -1, 0, 1),
         "negative x pixel should fail");
  expect(!comic2::set_tile_at_pixels(grid, 0, -1, 1),
         "negative y pixel should fail");
  expect(!comic2::set_tile_at_pixels(grid, 64, 0, 1),
         "x pixel at boundary should fail");
}

void test_tile_passability_checks() {
  comic2::TileCollisionConfig config{.solid_tile_threshold = 2};
  expect(comic2::is_tile_passable(0, config), "tile 0 should be passable");
  expect(comic2::is_tile_passable(1, config),
         "tile 1 should be passable (< threshold 2)");
  expect(!comic2::is_tile_passable(2, config),
         "tile 2 should be impassable (>= threshold 2)");
  expect(!comic2::is_tile_passable(comic2::kGrateTileId, config),
         "grate tile should be impassable");
  expect(!comic2::is_tile_passable(comic2::kLockedDoorTileId, config),
         "locked door tile should be impassable");

  comic2::RoomTileGrid grid = make_grid_fixture();
  comic2::set_tile_at_coords(grid, 0, 0, 0);
  expect(comic2::is_tile_passable_at_pixels(grid, 0, 0, config),
         "passable tile at origin");
  comic2::set_tile_at_coords(grid, 0, 0, comic2::kLockedDoorTileId);
  expect(!comic2::is_tile_passable_at_pixels(grid, 0, 0, config),
         "locked door tile at origin is impassable");
}

void test_drainage_corkscrew_grate_tile_transformation() {
  comic2::RuntimeState state;
  state.room_grid = make_grid_fixture();
  comic2::TileTransformationConfig config;

  // Place a grate tile at (16, 0)
  comic2::set_tile_at_coords(state.room_grid, 1, 0, config.grate_tile);
  expect(!comic2::is_tile_passable_at_pixels(state.room_grid, 16, 0),
         "grate tile must start impassable");

  // Attempt without corkscrew: fails, tile unchanged
  state.player.corkscrew_equipped = false;
  expect(!comic2::try_break_grate_tile(state, 16, 0, config),
         "break grate without corkscrew must fail");
  auto tile = comic2::get_tile_at_pixels(state.room_grid, 16, 0);
  expect(tile.has_value() && *tile == config.grate_tile,
         "grate tile must remain unchanged without corkscrew");

  // Attempt with corkscrew equipped: succeeds, transforms to passable
  state.player.corkscrew_equipped = true;
  expect(comic2::try_break_grate_tile(state, 16, 0, config),
         "break grate with corkscrew equipped must succeed");
  tile = comic2::get_tile_at_pixels(state.room_grid, 16, 0);
  expect(tile.has_value() && *tile == config.passable_tile,
         "grate tile must transform into passable tile in runtime grid");
  expect(comic2::is_tile_passable_at_pixels(state.room_grid, 16, 0),
         "transformed grate tile must now be passable");
}

void test_keys_door_tile_transformation() {
  comic2::RuntimeState state;
  state.room_grid = make_grid_fixture();
  comic2::TileTransformationConfig config;

  // Place a locked door tile at (32, 0)
  comic2::set_tile_at_coords(state.room_grid, 2, 0, config.locked_door_tile);
  expect(!comic2::is_tile_passable_at_pixels(state.room_grid, 32, 0),
         "locked door tile must start impassable");

  // Attempt with 0 keys: fails, key count stays 0, tile unchanged
  state.player.key_count = 0;
  expect(!comic2::try_unlock_door_tile(state, 32, 0, config),
         "unlock door with 0 keys must fail");
  expect(state.player.key_count == 0, "key count must remain 0");
  auto tile = comic2::get_tile_at_pixels(state.room_grid, 32, 0);
  expect(tile.has_value() && *tile == config.locked_door_tile,
         "door tile must remain locked");

  // Attempt with key_count = 2: succeeds, decrements key to 1, transforms to
  // passable
  state.player.key_count = 2;
  expect(comic2::try_unlock_door_tile(state, 32, 0, config),
         "unlock door with key must succeed");
  expect(state.player.key_count == 1, "key count must decrement by 1");
  tile = comic2::get_tile_at_pixels(state.room_grid, 32, 0);
  expect(tile.has_value() && *tile == config.passable_tile,
         "locked door tile must transform into passable tile");
  expect(comic2::is_tile_passable_at_pixels(state.room_grid, 32, 0),
         "unlocked door tile must now be passable");
}

void test_teleportation_wand_mechanics_and_beacon_transformation() {
  comic2::RuntimeState state;
  state.room_grid = make_grid_fixture();
  comic2::TileTransformationConfig config;

  state.room_entrance_x = 64;
  state.room_entrance_y = 96;
  state.player.x = 200;
  state.player.y = 150;
  state.player.x_vel = 5;
  state.player.y_vel = -4;
  state.player.is_airborne = true;

  // 1. Unequipped wand fails
  state.player.wand_equipped = false;
  expect(!comic2::try_use_teleport_wand(state, config),
         "teleport without wand equipped must fail");
  expect(state.player.x == 200 && state.player.y == 150,
         "player coordinates unchanged on failed wand use");

  // 2. Equipped wand teleports to room entrance
  state.player.wand_equipped = true;
  state.player.wand_charges = 3;
  expect(comic2::try_use_teleport_wand(state, config),
         "teleport with wand equipped must succeed");
  expect(state.player.x == 64 && state.player.y == 96,
         "player should be teleported to room entrance");
  expect(state.player.x_vel == 0 && state.player.y_vel == 0,
         "player velocities must be reset to 0 after teleport");
  expect(!state.player.is_airborne, "player airborne flag must be cleared");
  expect(state.player.wand_charges == 2, "wand charge must decrement");

  // 3. Activating warp beacon tile in room grid
  comic2::set_tile_at_coords(state.room_grid, 1, 1, config.warp_beacon_tile);
  expect(comic2::try_activate_warp_beacon(state, 16, 16, config),
         "activating warp beacon with wand equipped should succeed");
  auto tile = comic2::get_tile_at_pixels(state.room_grid, 16, 16);
  expect(tile.has_value() && *tile == config.active_warp_portal_tile,
         "warp beacon tile must transform into active warp portal tile");
  expect(state.warp_beacon.has_value(), "warp_beacon must be set in state");
  expect(state.warp_beacon->first == 16 && state.warp_beacon->second == 16,
         "warp beacon coords should match tile");

  // 4. Teleporting with active warp beacon teleports to beacon instead of
  // entrance
  state.player.x = 300;
  state.player.y = 200;
  expect(comic2::try_use_teleport_wand(state, config),
         "teleport with active beacon must succeed");
  expect(state.player.x == 16 && state.player.y == 16,
         "player should be teleported to active warp beacon position");
}

void test_process_player_tile_interactions_bounding_box_contact() {
  comic2::RuntimeState state;
  state.room_grid = make_grid_fixture();
  comic2::TileTransformationConfig config;

  // Place locked door at tile (1, 0) -> pixel x=16..31, y=0..15
  comic2::set_tile_at_coords(state.room_grid, 1, 0, config.locked_door_tile);

  // Player at x=0, y=0, facing right (width 16, height 32)
  // Right probe touches x=16, which is tile (1, 0)
  state.player.x = 0;
  state.player.y = 0;
  state.player.facing_right = true;
  state.player.key_count = 1;

  expect(comic2::process_player_tile_interactions(state, config),
         "contact with locked door should unlock and transform tile");
  expect(state.player.key_count == 0, "key should be consumed");
  auto tile = comic2::get_tile_at_pixels(state.room_grid, 16, 0);
  expect(tile.has_value() && *tile == config.passable_tile,
         "door tile should be transformed to passable passage");

  // Place grate tile at tile (0, 1) -> pixel x=0..15, y=16..31
  // Comic at x=0, y=0 overlaps tile (0, 1) with lower body
  comic2::set_tile_at_coords(state.room_grid, 0, 1, config.grate_tile);
  state.player.corkscrew_equipped = true;
  expect(comic2::process_player_tile_interactions(state, config),
         "contact with grate tile with corkscrew equipped should break grate");
  tile = comic2::get_tile_at_pixels(state.room_grid, 0, 16);
  expect(tile.has_value() && *tile == config.passable_tile,
         "grate tile should be transformed to passable passage");
}

} // namespace

void run_tile_collision_tests() {
  test_get_tile_at_pixels_maps_to_expected_tile_id();
  test_get_tile_at_pixels_rejects_out_of_bounds();
  test_room_loader_builds_row_pointer_table();
  test_tile_threshold_and_hazard_checks();
  test_update_player_hazard_state_checks_feet_for_hazards();
  test_has_floor_support_tile_grid_only();
  test_resolve_ground_contact_snaps_to_tile_top();
  test_set_tile_at_coords_and_pixels();
  test_tile_passability_checks();
  test_drainage_corkscrew_grate_tile_transformation();
  test_keys_door_tile_transformation();
  test_teleportation_wand_mechanics_and_beacon_transformation();
  test_process_player_tile_interactions_bounding_box_contact();
}
