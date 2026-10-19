#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#include "comic2/bootstrap.hpp"
#include "comic2/entity_runtime.hpp"
#include "comic2/projectiles.hpp"
#include "comic2/room_loader.hpp"
#include "comic2/tile_collision.hpp"

namespace {

void expect(bool condition, const char *message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void expect_eq(std::size_t actual, std::size_t expected, const char *message) {
  if (actual != expected) {
    throw std::runtime_error(message);
  }
}

bool require_original_assets() {
  const char *value = std::getenv("COMIC2_REQUIRE_ORIGINAL_ASSETS");
  if (value == nullptr || *value == '\0') {
    return false;
  }

  std::string normalized(value);
  std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  return normalized == "1" || normalized == "true" || normalized == "yes" ||
         normalized == "on";
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
make_test_room_resource_bytes(std::uint16_t level, std::uint16_t room,
                              std::uint16_t tile_w, std::uint16_t tile_h,
                              std::uint8_t seed = 0x00) {
  std::vector<std::uint8_t> decoded_room_bytes(0x2D2, 0x00);
  decoded_room_bytes[0] = seed;
  decoded_room_bytes[0x2A0] = 0x00;
  decoded_room_bytes[0x2A1] = 0x00;
  decoded_room_bytes[0x2A2] = 0x04;
  decoded_room_bytes[0x2A3] = 0x00;
  decoded_room_bytes[0x2A4] = 0x08;
  decoded_room_bytes[0x2A5] = 0x00;
  decoded_room_bytes[0x2B0] = 0x00;
  decoded_room_bytes[0x2B1] = 0x00;

  const std::vector<std::uint8_t> encoded =
      encode_literal_signed_rle(decoded_room_bytes);
  std::vector<std::uint8_t> resource_bytes(0x20 + encoded.size(), 0x00);
  resource_bytes[2] = static_cast<std::uint8_t>(level & 0xFFU);
  resource_bytes[3] = static_cast<std::uint8_t>((level >> 8) & 0xFFU);
  resource_bytes[0x04] = static_cast<std::uint8_t>(tile_w & 0xFFU);
  resource_bytes[0x05] = static_cast<std::uint8_t>((tile_w >> 8) & 0xFFU);
  resource_bytes[0x06] = static_cast<std::uint8_t>(tile_h & 0xFFU);
  resource_bytes[0x07] = static_cast<std::uint8_t>((tile_h >> 8) & 0xFFU);
  resource_bytes[0x08] = 0x20;
  resource_bytes[0x09] = 0x00;
  std::copy(encoded.begin(), encoded.end(), resource_bytes.begin() + 0x20);
  (void)room;
  return resource_bytes;
}

void test_entity_runtime_prunes_inactive_slots() {
  std::vector<comic2::RuntimeEntitySlot32> slots(2);
  slots[0].mapped_object_ptr = 0x1234;
  slots[1].mapped_object_ptr = 0;

  comic2::prune_inactive_runtime_slots(slots);

  expect(slots.size() == 1, "prune should keep only active runtime slots");
  expect(slots[0].mapped_object_ptr == 0x1234,
         "active slot should be preserved");
}

void test_ent_build_room_entity_list_filters_by_room() {
  std::vector<comic2::MappedObject12> mapped_objects = {
      {.room_x = 0,
       .room_y = 0,
       .descriptor_ptr = 0x100,
       .state_flags = 0,
       .world_x = 50,
       .world_y = 100},
      {.room_x = 1,
       .room_y = 0,
       .descriptor_ptr = 0x200,
       .state_flags = 0,
       .world_x = 150,
       .world_y = 100},
      {.room_x = 0,
       .room_y = 0,
       .descriptor_ptr = 0x300,
       .state_flags = 0,
       .world_x = 250,
       .world_y = 100},
  };

  std::vector<comic2::ActiveEntity8> active_entities;
  comic2::EntityActivationState activation_state;
  comic2::ent_build_room_entity_list(mapped_objects, 0, 0, active_entities,
                                     activation_state);

  expect_eq(active_entities.size(), 2, "should find 2 entities in room (0,0)");
  expect(active_entities[0].x == 50 && active_entities[0].y == 100,
         "first entity position mismatch");
  expect(active_entities[1].x == 250 && active_entities[1].y == 100,
         "second entity position mismatch");
  expect(activation_state.active_count == 2, "activation state count mismatch");
}

void test_ent_build_runtime_slots_for_viewport_culls() {
  std::vector<comic2::ActiveEntity8> active_entities = {
      {.x = 10, .y = 10, .flags_or_slot = 0, .sprite_or_obj = 0},
      {.x = 500, .y = 10, .flags_or_slot = 0, .sprite_or_obj = 1},
      {.x = 100, .y = 10, .flags_or_slot = 0, .sprite_or_obj = 2},
  };

  std::vector<comic2::MappedObject12> mapped_objects = {
      {.room_x = 16,
       .room_y = 16,
       .descriptor_ptr = 0x100,
       .state_flags = 0,
       .world_x = 10,
       .world_y = 10},
      {.room_x = 16,
       .room_y = 16,
       .descriptor_ptr = 0x200,
       .state_flags = 0,
       .world_x = 500,
       .world_y = 10},
      {.room_x = 16,
       .room_y = 16,
       .descriptor_ptr = 0x300,
       .state_flags = 0,
       .world_x = 100,
       .world_y = 10},
  };

  std::vector<comic2::RuntimeEntitySlot32> runtime_slots;
  comic2::EntityActivationState activation_state;
  comic2::EntityViewportBounds viewport{
      .min_x = 0, .max_x = 319, .min_y = 0, .max_y = 199};
  std::uint16_t activation_toggle = 1;

  comic2::ent_build_runtime_slots_for_viewport(
      active_entities, mapped_objects, runtime_slots, viewport,
      activation_state, activation_toggle);

  expect_eq(runtime_slots.size(), 6, "should have 6 runtime slots");
  expect(activation_state.active_count <= 2,
         "should activate at most 2 slots (entities in viewport)");

  const auto active_count = std::count_if(
      runtime_slots.begin(), runtime_slots.end(),
      [](const auto &slot) { return slot.mapped_object_ptr != 0; });
  expect(active_count <= 2, "should have at most 2 active runtime slots");
}

void test_ent_copy_descriptor_to_runtime_slot_initializes() {
  comic2::MappedObject12 descriptor = {
      .room_x = 16,
      .room_y = 16,
      .descriptor_ptr = 0x100,
      .state_flags = 1,
      .world_x = 100,
      .world_y = 50,
  };

  comic2::RuntimeEntitySlot32 slot = {};
  std::uint16_t activation_toggle = 1;

  comic2::ent_copy_descriptor_to_runtime_slot(descriptor, slot, 0x42,
                                              activation_toggle);

  expect(slot.mapped_object_ptr == 0x42,
         "mapped_object_ptr should be set to behavior_ptr");
  expect(slot.x == 100 && slot.y == 50,
         "position should be copied from descriptor");
  expect(slot.hitbox_w == 16 && slot.hitbox_h == 16,
         "hitbox should be copied from descriptor");
  expect(slot.behavior_state == 1,
         "behavior_state should be copied from state_flags");
  expect(slot.dir_toggle == 1,
         "dir_toggle should be initialized to 1 (activation_toggle & 3)");
  expect(activation_toggle == 2,
         "activation_toggle should be XORed with 3 (1 ^ 3 = 2)");
}

void test_deactivate_runtime_slot_clears() {
  comic2::RuntimeEntitySlot32 slot = {
      .hitbox_w = 0,
      .hitbox_h = 0,
      .type_flags = 0x10,
      .behavior_state = 5,
      .anim_span = 0,
      .anim_period = 0,
      .unknown_0c = 0,
      .unknown_0e = 0,
      .x = 100,
      .y = 50,
      .param_a = 0,
      .param_b = 0,
      .dir_toggle = 0,
      .anim_tick = 0,
      .unknown_1c = 0,
      .mapped_object_ptr = 0x1234,
  };

  comic2::deactivate_runtime_slot(slot);

  expect(slot.mapped_object_ptr == 0, "mapped_object_ptr should be cleared");
  expect(slot.behavior_state == 0, "behavior_state should be cleared");
  expect(slot.type_flags == 0, "type_flags should be cleared");
  expect(slot.x == 100 && slot.y == 50, "position should be preserved");
}

void test_projectile_updates_and_despawns_out_of_bounds() {
  std::vector<comic2::ProjectileState> projectiles;
  comic2::spawn_projectile(projectiles, 10, 10, 3, 0);

  comic2::update_projectiles(
      projectiles,
      comic2::ProjectileBounds{
          .min_x = 0, .max_x = 11, .min_y = 0, .max_y = 50},
      comic2::RoomTileGrid{}, 0, 0, 200, 152);

  expect(projectiles.size() == 1, "spawn should append one projectile");
  expect(!projectiles[0].active,
         "projectile should deactivate when out of bounds");
}

void test_spawn_player_projectile_with_facing() {
  std::vector<comic2::ProjectileState> projectiles;

  comic2::ProjectileSpawnParams params;
  params.x = 100;
  params.y = 50;
  params.facing_right = true;
  params.is_airborne = false;
  params.firepower = 1;

  comic2::spawn_player_projectile(projectiles, params);

  expect_eq(projectiles.size(), 1, "should spawn one projectile");
  expect(projectiles[0].x == 104, "projectile x should be offset by 4");
  expect(projectiles[0].y == 58,
         "projectile y should be player y + 8 when grounded");
  expect(projectiles[0].x_vel == 0x0010,
         "projectile x_vel should be positive when facing right");
  expect(projectiles[0].y_vel == -0x000C,
         "projectile y_vel should be negative (upward arc)");
  expect(projectiles[0].active, "projectile should be active");
}

void test_spawn_player_projectile_airborne_adjustment() {
  std::vector<comic2::ProjectileState> projectiles;

  comic2::ProjectileSpawnParams params;
  params.x = 100;
  params.y = 50;
  params.facing_right = false;
  params.is_airborne = true;
  params.firepower = 1;

  comic2::spawn_player_projectile(projectiles, params);

  expect_eq(projectiles.size(), 1, "should spawn one projectile");
  expect(
      projectiles[0].x_vel == -0x0010,
      "projectile x_vel should remain -0x0010 when facing left and airborne");
  expect(projectiles[0].y == 54,
         "projectile y should be player y + 4 when airborne");
}

void test_projectile_tile_collision_detection() {
  comic2::RoomTileGrid room_grid;
  room_grid.tile_w = 40;
  room_grid.tile_h = 24;
  room_grid.row_pointers.resize(24, 0);
  room_grid.tile_data.resize(40 * 24 * 2, 0x01);

  comic2::ProjectileState projectile;
  projectile.x = 50;
  projectile.y = 50;
  projectile.active = true;

  const bool collision =
      comic2::check_projectile_tile_collision(projectile, room_grid);
  expect(collision, "projectile should collide with non-zero tile");
}

void test_projectile_viewport_culling() {
  std::vector<comic2::ProjectileState> projectiles;
  comic2::spawn_projectile(projectiles, 100, 100, -20, 0);

  comic2::RoomTileGrid empty_grid;
  empty_grid.tile_w = 40;
  empty_grid.tile_h = 24;

  comic2::update_projectiles(
      projectiles,
      comic2::ProjectileBounds{
          .min_x = 0, .max_x = 319, .min_y = 0, .max_y = 199},
      empty_grid, 100, 0, 200, 152);

  expect(!projectiles[0].active, "projectile should deactivate when outside "
                                 "viewport bounds (x < viewport_min_x - 8)");
}

void test_projectile_anim_frame_cycles() {
  std::vector<comic2::ProjectileState> projectiles;
  comic2::spawn_projectile(projectiles, 100, 100, 1, 0);

  comic2::RoomTileGrid empty_grid;
  empty_grid.tile_w = 40;
  empty_grid.tile_h = 24;

  for (int i = 0; i < 10; ++i) {
    comic2::update_projectiles(
        projectiles,
        comic2::ProjectileBounds{
            .min_x = 0, .max_x = 319, .min_y = 0, .max_y = 199},
        empty_grid, 0, 0, 200, 152);
  }

  expect(projectiles[0].anim_frame == 2, "anim_frame should cycle modulo 8");
}

void test_ent_activation_pipeline_integration() {
  std::vector<comic2::MappedObject12> mapped_objects = {
      {.room_x = 10,
       .room_y = 20,
       .descriptor_ptr = 0x100,
       .state_flags = 0,
       .world_x = 50,
       .world_y = 50},
      {.room_x = 10,
       .room_y = 20,
       .descriptor_ptr = 0x200,
       .state_flags = 0,
       .world_x = 150,
       .world_y = 50},
      {.room_x = 0,
       .room_y = 0,
       .descriptor_ptr = 0x300,
       .state_flags = 0,
       .world_x = 10,
       .world_y = 10},
  };

  std::vector<comic2::ActiveEntity8> active_entities;
  comic2::EntityActivationState activation_state;
  comic2::ent_build_room_entity_list(mapped_objects, 10, 20, active_entities,
                                     activation_state);

  expect_eq(active_entities.size(), 2,
            "should find 2 entities in room (10,20)");
  expect(active_entities[0].sprite_or_obj == 0,
         "first active entity should point to mapped_objects[0]");
  expect(active_entities[1].sprite_or_obj == 1,
         "second active entity should point to mapped_objects[1]");

  std::vector<comic2::RuntimeEntitySlot32> runtime_slots;
  comic2::EntityViewportBounds viewport{
      .min_x = 0, .max_x = 319, .min_y = 0, .max_y = 199};
  std::uint16_t activation_toggle = 1;

  comic2::ent_build_runtime_slots_for_viewport(
      active_entities, mapped_objects, runtime_slots, viewport,
      activation_state, activation_toggle);

  expect(runtime_slots[0].mapped_object_ptr == 0,
         "first runtime slot should point to mapped_objects[0]");
  expect(runtime_slots[1].mapped_object_ptr == 1,
         "second runtime slot should point to mapped_objects[1]");
  expect(runtime_slots[0].type_flags == 0x100,
         "first runtime slot type_flags mismatch");
  expect(runtime_slots[1].type_flags == 0x200,
         "second runtime slot type_flags mismatch");
}

void test_room_loader_decodes_frdata_entry() {
  const std::vector<std::uint8_t> bytes = {
      0x28, 0x00, // tile_w = 40
      0x18, 0x00, // tile_h = 24
      0x34, 0x12, // rle_data_off = 0x1234
  };

  const std::optional<comic2::FrdataRoomEntry> entry =
      comic2::decode_frdata_room_entry(bytes, 0);
  expect(entry.has_value(), "decode should succeed on full room entry payload");
  expect(entry->tile_w == 40, "tile_w decode mismatch");
  expect(entry->tile_h == 24, "tile_h decode mismatch");
  expect(entry->rle_data_off == 0x1234, "rle_data_off decode mismatch");
}

void test_room_loader_resolves_room_payload_location() {
  std::vector<std::uint8_t> bytes(0x40, 0x00);
  bytes[2] = 0x03;
  bytes[3] = 0x00;
  bytes[0x04] = 0x04;
  bytes[0x05] = 0x00;
  bytes[0x06] = 0x03;
  bytes[0x07] = 0x00;
  bytes[0x08] = 0x20;
  bytes[0x09] = 0x00;

  const std::optional<comic2::RoomLoadSpec> spec =
      comic2::resolve_room_load_spec("/tmp/FRDATA.0", bytes, 3, 0,
                                     comic2::ResourceAssetKind::RoomPayload);
  expect(spec.has_value(), "resolver should map a valid room payload");
  expect(spec->source_path == std::filesystem::path("/tmp/FRDATA.0"),
         "resolver should preserve the source path");
  expect(spec->table_offset == 0x04, "resolver should report table offset");
  expect(spec->room_entry_offset == 0x04,
         "resolver should report room entry offset");
  expect(spec->resource_offset == 0x20,
         "resolver should report room payload offset");
  expect(spec->room_entry.tile_w == 4, "resolver should decode tile_w");
  expect(spec->room_entry.tile_h == 3, "resolver should decode tile_h");
  expect(spec->room_entry.rle_data_off == 0x20,
         "resolver should decode payload offset");

  const std::optional<comic2::RoomLoadSpec> table_spec =
      comic2::resolve_room_load_spec("/tmp/FRDATA.0", bytes, 3, 0,
                                     comic2::ResourceAssetKind::RoomTable);
  expect(table_spec.has_value(), "resolver should map room table access");
  expect(table_spec->resource_offset == 0x04,
         "room table access should point at the room entry");
}

void test_room_loader_rejects_huge_offset() {
  const std::vector<std::uint8_t> bytes = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  const std::optional<comic2::FrdataRoomEntry> entry =
      comic2::decode_frdata_room_entry(bytes,
                                       std::numeric_limits<std::size_t>::max());
  expect(!entry.has_value(), "decode should reject oversized offsets safely");
}

void test_room_loader_parses_room_count_and_level_id_header() {
  std::vector<std::uint8_t> bytes(0x40, 0x00);
  bytes[0] = 0x02;
  bytes[1] = 0x00;
  bytes[2] = 0x03;
  bytes[3] = 0x00;
  bytes[0x04] = 0x04;
  bytes[0x05] = 0x00;
  bytes[0x06] = 0x03;
  bytes[0x07] = 0x00;
  bytes[0x08] = 0x20;
  bytes[0x09] = 0x00;
  bytes[0x0A] = 0x08;
  bytes[0x0B] = 0x00;
  bytes[0x0C] = 0x06;
  bytes[0x0D] = 0x00;
  bytes[0x0E] = 0x20;
  bytes[0x0F] = 0x00;

  const auto header = comic2::parse_room_table_header(bytes);
  expect(header.has_value(),
         "room loader should parse an explicit FR###.1 header");
  expect(header->room_count == 2,
         "room table header should expose the explicit room count");
  expect(header->level_id == 3,
         "room table header should expose the explicit level id");

  const auto spec = comic2::resolve_room_load_spec(
      "/tmp/FR003.1", bytes, 3, 1, comic2::ResourceAssetKind::RoomPayload);
  expect(spec.has_value(),
         "resolver should honor the explicit room_count + level_id header");
  expect(spec->room == 1, "resolver should select the requested room index");
  expect(spec->room_entry.tile_w == 8,
         "resolver should decode the selected room width from the table");
  expect(spec->room_entry.tile_h == 6,
         "resolver should decode the selected room height from the table");
}

void test_room_loader_rejects_out_of_bounds_room_index() {
  std::vector<std::uint8_t> bytes(0x20, 0x00);
  bytes[0] = 0x01;
  bytes[1] = 0x00;
  bytes[2] = 0x03;
  bytes[3] = 0x00;
  bytes[0x04] = 0x04;
  bytes[0x05] = 0x00;
  bytes[0x06] = 0x03;
  bytes[0x07] = 0x00;
  bytes[0x08] = 0x20;
  bytes[0x09] = 0x00;

  comic2::RuntimeState state;
  const bool loaded =
      comic2::load_room_tilemap_from_resource_buffer(state, bytes, 3, 1);
  expect(!loaded,
         "room loader should reject a room index that is outside the table");
}

void test_room_loader_rejects_malformed_room_header() {
  std::vector<std::uint8_t> bytes = {0x05, 0x00, 0x03, 0x00};

  const auto header = comic2::parse_room_table_header(bytes);
  expect(header.has_value(),
         "header parser should accept a structurally valid FR###.1 header");
  expect(header->room_count == 5, "header parser should retain room_count");

  const auto spec = comic2::resolve_room_load_spec(
      "/tmp/FR003.1", bytes, 3, 0, comic2::ResourceAssetKind::RoomPayload);
  expect(!spec.has_value(), "resolver should reject a malformed room header "
                            "when the table is truncated");

  std::vector<std::uint8_t> truncated = {0x02, 0x00, 0x03};
  expect(!comic2::parse_room_table_header(truncated).has_value(),
         "header parser should reject a truncated header");
}

void test_room_loader_rejects_sentinel_entries() {
  std::vector<std::uint8_t> bytes(0x10, 0x00);
  bytes[2] = 0x03;
  bytes[3] = 0x00;
  bytes[0x04] = 0xFF;
  bytes[0x05] = 0xFF;
  bytes[0x06] = 0xFF;
  bytes[0x07] = 0xFF;
  bytes[0x08] = 0xFF;
  bytes[0x09] = 0xFF;

  const std::optional<comic2::RoomLoadSpec> spec =
      comic2::resolve_room_load_spec("/tmp/FRDATA.0", bytes, 3, 0,
                                     comic2::ResourceAssetKind::RoomPayload);
  expect(!spec.has_value(),
         "resolver should reject sentinel room entries as invalid");
}

void test_room_loader_populates_runtime_state_from_resource_buffer() {
  std::vector<std::uint8_t> decoded_room_bytes(0x2C4, 0x00);
  decoded_room_bytes[0] = 0x11;
  decoded_room_bytes[1] = 0x22;
  decoded_room_bytes[2] = 0x33;
  decoded_room_bytes[3] = 0x44;
  decoded_room_bytes[4] = 0x55;
  decoded_room_bytes[5] = 0x66;

  decoded_room_bytes[0x2A0] = 0x00;
  decoded_room_bytes[0x2A1] = 0x00;
  decoded_room_bytes[0x2A2] = 0x04;
  decoded_room_bytes[0x2A3] = 0x00;
  decoded_room_bytes[0x2A4] = 0x08;
  decoded_room_bytes[0x2A5] = 0x00;

  const std::vector<std::uint8_t> encoded_room_bytes =
      encode_literal_signed_rle(decoded_room_bytes);

  std::vector<std::uint8_t> resource_bytes(0x20, 0x00);
  resource_bytes[2] = 0x03;
  resource_bytes[3] = 0x00;

  resource_bytes.resize(0x20, 0x00);
  resource_bytes.resize(0x20 + encoded_room_bytes.size(), 0x00);
  std::copy(encoded_room_bytes.begin(), encoded_room_bytes.end(),
            resource_bytes.begin() + 0x20);

  resource_bytes[0x04] = 0x04;
  resource_bytes[0x05] = 0x00;
  resource_bytes[0x06] = 0x03;
  resource_bytes[0x07] = 0x00;
  resource_bytes[0x08] = 0x20;
  resource_bytes[0x09] = 0x00;

  comic2::RuntimeState state;
  const bool loaded = comic2::load_room_tilemap_from_resource_buffer(
      state, resource_bytes, 3, 0);

  expect(loaded, "room loader should populate runtime state from resource");
  expect(state.current_level == 3, "room loader should set current level");
  expect(state.current_room == 0, "room loader should set current room");
  expect(state.room_entry.tile_w == 4, "room entry tile_w should be stored");
  expect(state.room_entry.tile_h == 3, "room entry tile_h should be stored");
  expect(state.room_grid.tile_w == 4, "room grid tile_w should match entry");
  expect(state.room_grid.tile_h == 3, "room grid tile_h should match entry");
  expect(state.room_grid.row_pointers == std::vector<std::uint16_t>{0, 4, 8},
         "room loader should build row pointer table");
  const std::vector<std::uint8_t> expected_tile_data =
      std::vector<std::uint8_t>{0x11, 0x22, 0x33, 0x44, 0x55, 0x66,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  expect(state.room_grid.tile_data == expected_tile_data,
         "room loader should store only room tile indices, not the trailing "
         "row-pointer and object metadata");
}

void test_room_loader_decodes_signed_rle_room_tilemap_dimensions() {
  std::vector<std::uint8_t> decoded_room_bytes(0x2D2, 0x00);
  const std::vector<std::uint8_t> tile_data = {
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B};
  std::copy(tile_data.begin(), tile_data.end(), decoded_room_bytes.begin());
  decoded_room_bytes[0x2A0] = 0x00;
  decoded_room_bytes[0x2A1] = 0x00;
  decoded_room_bytes[0x2A2] = 0x04;
  decoded_room_bytes[0x2A3] = 0x00;
  decoded_room_bytes[0x2A4] = 0x08;
  decoded_room_bytes[0x2A5] = 0x00;

  const auto encoded = encode_literal_signed_rle(decoded_room_bytes);
  std::vector<std::uint8_t> resource_bytes(0x20 + encoded.size(), 0x00);
  resource_bytes[2] = 0x00;
  resource_bytes[3] = 0x00;
  resource_bytes[0x04] = 0x04;
  resource_bytes[0x05] = 0x00;
  resource_bytes[0x06] = 0x03;
  resource_bytes[0x07] = 0x00;
  resource_bytes[0x08] = 0x20;
  resource_bytes[0x09] = 0x00;
  std::copy(encoded.begin(), encoded.end(), resource_bytes.begin() + 0x20);

  comic2::RuntimeState state;
  state.level_tileset.resize(16);
  const bool loaded = comic2::load_room_tilemap_from_resource_buffer(
      state, resource_bytes, 0, 0);

  expect(
      loaded,
      "valid Signed-RLE room tilemap payload should decode into runtime state");
  expect(state.room_grid.tile_w == 4 && state.room_grid.tile_h == 3,
         "room loader should preserve Level 0 Room 0 dimensions");
  expect(state.room_grid.tile_data == tile_data,
         "room loader should decode tile indices without trailing metadata");
}

void test_room_loader_validates_tile_indices_against_tileset_size() {
  std::vector<std::uint8_t> decoded_room_bytes(0x2D2, 0x00);
  const std::vector<std::uint8_t> tile_data = {
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B};
  std::copy(tile_data.begin(), tile_data.end(), decoded_room_bytes.begin());
  decoded_room_bytes[0x2A0] = 0x00;
  decoded_room_bytes[0x2A1] = 0x00;
  decoded_room_bytes[0x2A2] = 0x04;
  decoded_room_bytes[0x2A3] = 0x00;
  decoded_room_bytes[0x2A4] = 0x08;
  decoded_room_bytes[0x2A5] = 0x00;

  const auto encoded = encode_literal_signed_rle(decoded_room_bytes);
  std::vector<std::uint8_t> resource_bytes(0x20 + encoded.size(), 0x00);
  resource_bytes[2] = 0x00;
  resource_bytes[3] = 0x00;
  resource_bytes[0x04] = 0x04;
  resource_bytes[0x05] = 0x00;
  resource_bytes[0x06] = 0x03;
  resource_bytes[0x07] = 0x00;
  resource_bytes[0x08] = 0x20;
  resource_bytes[0x09] = 0x00;
  std::copy(encoded.begin(), encoded.end(), resource_bytes.begin() + 0x20);

  comic2::RuntimeState state;
  state.level_tileset.resize(16);
  const bool loaded = comic2::load_room_tilemap_from_resource_buffer(
      state, resource_bytes, 0, 0);

  expect(loaded, "tile indices within the loaded tileset bounds should pass");
  expect(state.room_grid.tile_data[0] == 0x00,
         "the first validated room tile should remain in range");
}

void test_room_loader_rejects_out_of_range_tile_indices_cleanly() {
  std::vector<std::uint8_t> decoded_room_bytes(0x2D2, 0x00);
  const std::vector<std::uint8_t> tile_data = {
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x10,
  };
  std::copy(tile_data.begin(), tile_data.end(), decoded_room_bytes.begin());
  decoded_room_bytes[0x2A0] = 0x00;
  decoded_room_bytes[0x2A1] = 0x00;
  decoded_room_bytes[0x2A2] = 0x04;
  decoded_room_bytes[0x2A3] = 0x00;
  decoded_room_bytes[0x2A4] = 0x08;
  decoded_room_bytes[0x2A5] = 0x00;

  const auto encoded = encode_literal_signed_rle(decoded_room_bytes);
  std::vector<std::uint8_t> resource_bytes(0x20 + encoded.size(), 0x00);
  resource_bytes[2] = 0x00;
  resource_bytes[3] = 0x00;
  resource_bytes[0x04] = 0x04;
  resource_bytes[0x05] = 0x00;
  resource_bytes[0x06] = 0x03;
  resource_bytes[0x07] = 0x00;
  resource_bytes[0x08] = 0x20;
  resource_bytes[0x09] = 0x00;
  std::copy(encoded.begin(), encoded.end(), resource_bytes.begin() + 0x20);

  comic2::RuntimeState state;
  state.level_tileset.resize(16);
  const bool loaded = comic2::load_room_tilemap_from_resource_buffer(
      state, resource_bytes, 0, 0);

  expect(!loaded,
         "tile index 0x10 should fail validation when the tileset is smaller");
  expect(state.room_grid.tile_data.empty(),
         "failed validation should leave room tile data unset for safety");
}

void test_room_loader_handles_self_referential_resource_bytes_span() {
  std::vector<std::uint8_t> decoded_room_bytes(0x2C4, 0x00);
  decoded_room_bytes[0x2A0] = 0x00;
  decoded_room_bytes[0x2A1] = 0x00;
  decoded_room_bytes[0x2A2] = 0x04;
  decoded_room_bytes[0x2A3] = 0x00;
  decoded_room_bytes[0x2A4] = 0x08;
  decoded_room_bytes[0x2A5] = 0x00;

  const std::vector<std::uint8_t> encoded_room_bytes =
      encode_literal_signed_rle(decoded_room_bytes);

  std::vector<std::uint8_t> resource_bytes(0x20 + encoded_room_bytes.size(),
                                           0x00);
  resource_bytes[2] = 0x03;
  resource_bytes[3] = 0x00;
  resource_bytes[0x04] = 0x04;
  resource_bytes[0x05] = 0x00;
  resource_bytes[0x06] = 0x03;
  resource_bytes[0x07] = 0x00;
  resource_bytes[0x08] = 0x20;
  resource_bytes[0x09] = 0x00;
  std::copy(encoded_room_bytes.begin(), encoded_room_bytes.end(),
            resource_bytes.begin() + 0x20);

  comic2::RuntimeState state;
  state.room_resource_bytes = resource_bytes;

  const bool loaded = comic2::load_room_tilemap_from_resource_buffer(
      state, state.room_resource_bytes, 3, 0);

  expect(loaded, "room loader should tolerate a self-referential span");
  expect(state.room_resource_bytes == resource_bytes,
         "room loader should preserve the copied resource bytes");
}

void test_room_loader_decodes_mapped_objects_from_payload() {
  std::vector<std::uint8_t> decoded_room_bytes(0x2D2, 0x00);
  decoded_room_bytes[0x2B0] = 0x02;
  decoded_room_bytes[0x2B1] = 0x00;

  // Record 0
  decoded_room_bytes[0x2B2] = 0x01;
  decoded_room_bytes[0x2B3] = 0x00;
  decoded_room_bytes[0x2B4] = 0x03;
  decoded_room_bytes[0x2B5] = 0x00;
  decoded_room_bytes[0x2B6] = 0x34;
  decoded_room_bytes[0x2B7] = 0x12;
  decoded_room_bytes[0x2B8] = 0x02;
  decoded_room_bytes[0x2B9] = 0x00;
  decoded_room_bytes[0x2BA] = 0x64;
  decoded_room_bytes[0x2BB] = 0x00;
  decoded_room_bytes[0x2BC] = 0x32;
  decoded_room_bytes[0x2BD] = 0x00;

  // Record 1
  decoded_room_bytes[0x2BE] = 0x01;
  decoded_room_bytes[0x2BF] = 0x00;
  decoded_room_bytes[0x2C0] = 0x03;
  decoded_room_bytes[0x2C1] = 0x00;
  decoded_room_bytes[0x2C2] = 0x78;
  decoded_room_bytes[0x2C3] = 0x56;
  decoded_room_bytes[0x2C4] = 0x01;
  decoded_room_bytes[0x2C5] = 0x00;
  decoded_room_bytes[0x2C6] = 0x2C;
  decoded_room_bytes[0x2C7] = 0x01;
  decoded_room_bytes[0x2C8] = 0x5A;
  decoded_room_bytes[0x2C9] = 0x00;

  const auto mapped = comic2::decode_room_mapped_objects(decoded_room_bytes);
  expect(mapped.has_value(), "mapped object decode should succeed");
  expect_eq(mapped->size(), 2, "mapped object decode should return 2 records");
  expect((*mapped)[0].descriptor_ptr == 0x1234,
         "first descriptor pointer mismatch");
  expect((*mapped)[1].descriptor_ptr == 0x5678,
         "second descriptor pointer mismatch");
  expect((*mapped)[1].world_x == 300 && (*mapped)[1].world_y == 90,
         "second mapped object world coords mismatch");
}

void test_room_loader_wires_runtime_tables_from_loaded_mapped_objects() {
  std::vector<std::uint8_t> decoded_room_bytes(0x2D2, 0x00);
  decoded_room_bytes[0x2A0] = 0x00;
  decoded_room_bytes[0x2A1] = 0x00;
  decoded_room_bytes[0x2A2] = 0x04;
  decoded_room_bytes[0x2A3] = 0x00;
  decoded_room_bytes[0x2A4] = 0x08;
  decoded_room_bytes[0x2A5] = 0x00;

  decoded_room_bytes[0x2B0] = 0x02;
  decoded_room_bytes[0x2B1] = 0x00;

  // Record 0 in room (0,3), inside viewport
  decoded_room_bytes[0x2B2] = 0x00;
  decoded_room_bytes[0x2B3] = 0x00;
  decoded_room_bytes[0x2B4] = 0x03;
  decoded_room_bytes[0x2B5] = 0x00;
  decoded_room_bytes[0x2B6] = 0x10;
  decoded_room_bytes[0x2B7] = 0x00;
  decoded_room_bytes[0x2B8] = 0x07;
  decoded_room_bytes[0x2B9] = 0x00;
  decoded_room_bytes[0x2BA] = 0x14;
  decoded_room_bytes[0x2BB] = 0x00;
  decoded_room_bytes[0x2BC] = 0x1E;
  decoded_room_bytes[0x2BD] = 0x00;

  // Record 1 in room (0,3), outside viewport
  decoded_room_bytes[0x2BE] = 0x00;
  decoded_room_bytes[0x2BF] = 0x00;
  decoded_room_bytes[0x2C0] = 0x03;
  decoded_room_bytes[0x2C1] = 0x00;
  decoded_room_bytes[0x2C2] = 0x20;
  decoded_room_bytes[0x2C3] = 0x00;
  decoded_room_bytes[0x2C4] = 0x08;
  decoded_room_bytes[0x2C5] = 0x00;
  decoded_room_bytes[0x2C6] = 0xF4;
  decoded_room_bytes[0x2C7] = 0x01;
  decoded_room_bytes[0x2C8] = 0x14;
  decoded_room_bytes[0x2C9] = 0x00;

  const std::vector<std::uint8_t> encoded_room_bytes =
      encode_literal_signed_rle(decoded_room_bytes);

  std::vector<std::uint8_t> resource_bytes(0x30 + encoded_room_bytes.size(),
                                           0x00);
  resource_bytes[2] = 0x03;
  resource_bytes[3] = 0x00;
  resource_bytes[0x04] = 0x04;
  resource_bytes[0x05] = 0x00;
  resource_bytes[0x06] = 0x03;
  resource_bytes[0x07] = 0x00;
  resource_bytes[0x08] = 0x30;
  resource_bytes[0x09] = 0x00;
  std::copy(encoded_room_bytes.begin(), encoded_room_bytes.end(),
            resource_bytes.begin() + 0x30);

  comic2::RuntimeState state;
  const bool loaded = comic2::load_room_tilemap_from_resource_buffer(
      state, resource_bytes, 3, 0);

  expect(loaded, "room loader should succeed with mapped-object payload");
  expect_eq(state.mapped_objects.size(), 2,
            "room loader should populate mapped object table from payload");
  expect_eq(state.active_entities.size(), 2,
            "room loader should build active entity list from mapped objects");
  expect_eq(state.runtime_slots.size(), 6,
            "room loader should provision runtime slot table");
  expect(state.activation_state.active_count == 1,
         "runtime slot wiring should cull out-of-viewport descriptor");
  expect(state.runtime_slots[0].mapped_object_ptr == 0,
         "first runtime slot should point at first mapped object");
  expect(state.room_event_anchor.active,
         "room loader should seed an active room-event anchor from mapped "
         "objects");
  expect(state.room_event_anchor.x == 20 && state.room_event_anchor.y == 30,
         "room loader should seed the room-event anchor from the event object");
  expect(state.room_event_anchor.velocity_x != 0 ||
             state.room_event_anchor.velocity_y != 0,
         "room loader should seed a moving room-event anchor");
  expect(state.room_event_script.loaded,
         "room loader should mark the raw room-event script state as loaded");
  expect(state.room_event_script.raw_script_bytes == state.room_resource_bytes,
         "room loader should preserve the raw room-event script bytes");
}

void test_room_loader_handles_corrupt_mapped_object_payload_stably() {
  std::vector<std::uint8_t> decoded_room_bytes(0x2C4, 0x00);
  decoded_room_bytes[0x2A0] = 0x00;
  decoded_room_bytes[0x2A1] = 0x00;
  decoded_room_bytes[0x2A2] = 0x04;
  decoded_room_bytes[0x2A3] = 0x00;
  decoded_room_bytes[0x2A4] = 0x08;
  decoded_room_bytes[0x2A5] = 0x00;
  decoded_room_bytes[0x2B0] = 0x08;
  decoded_room_bytes[0x2B1] = 0x00;

  const std::vector<std::uint8_t> encoded_room_bytes =
      encode_literal_signed_rle(decoded_room_bytes);

  std::vector<std::uint8_t> resource_bytes(0x30 + encoded_room_bytes.size(),
                                           0x00);
  resource_bytes[2] = 0x03;
  resource_bytes[3] = 0x00;
  resource_bytes[0x04] = 0x04;
  resource_bytes[0x05] = 0x00;
  resource_bytes[0x06] = 0x03;
  resource_bytes[0x07] = 0x00;
  resource_bytes[0x08] = 0x30;
  resource_bytes[0x09] = 0x00;
  std::copy(encoded_room_bytes.begin(), encoded_room_bytes.end(),
            resource_bytes.begin() + 0x30);

  comic2::RuntimeState state;
  state.mapped_objects.push_back(
      comic2::MappedObject12{.room_x = 1,
                             .room_y = 1,
                             .descriptor_ptr = 0x1234,
                             .state_flags = 0,
                             .world_x = 10,
                             .world_y = 10});

  const bool loaded = comic2::load_room_tilemap_from_resource_buffer(
      state, resource_bytes, 3, 0);

  expect(loaded,
         "corrupt mapped-object payload should not fail room tilemap load");
  expect(state.mapped_objects.empty(),
         "corrupt mapped-object payload should clear mapped object table");
  expect(state.active_entities.empty(),
         "corrupt mapped-object payload should clear active entities");
  expect(state.runtime_slots.empty(),
         "corrupt mapped-object payload should keep runtime slots controlled");
}

void test_frpak_catalog_builds_file_index() {
  const std::vector<std::uint8_t> bytes = {
      0x40, 0x00, 0x01, 0x02, 0x03,
  };

  const auto file =
      comic2::build_frpak_catalog_file("/tmp/FRPAK.001", bytes, 1);
  expect(file.has_value(), "frpak catalog should build for valid payload");
  expect(file->pak_id == 1, "frpak catalog should preserve pak id");
  expect(file->file_size == bytes.size(),
         "frpak catalog should store file size");
  expect(file->records.size() == 1,
         "frpak catalog should expose a single direct-stream record");
  expect(file->records[0].record_id == 0,
         "frpak direct-stream record id should start at zero");
  expect(file->records[0].data_offset == 0,
         "frpak direct-stream record offset should be zero");
  expect(file->records[0].data_size == bytes.size(),
         "frpak direct-stream record size should match file size");
  expect(file->records[0].row_span_bytes == 0x40,
         "frpak direct-stream row span should decode from file header");
}

void test_frpak_catalog_rejects_truncated_header() {
  const std::vector<std::uint8_t> bytes = {0x40};
  const auto file =
      comic2::build_frpak_catalog_file("/tmp/FRPAK.001", bytes, 1);
  expect(!file.has_value(),
         "frpak catalog should reject files smaller than header size");
}

void test_frpak_catalog_rejects_zero_row_span_header() {
  const std::vector<std::uint8_t> bytes = {0x00, 0x00, 0x01};
  const auto file =
      comic2::build_frpak_catalog_file("/tmp/FRPAK.001", bytes, 1);
  expect(!file.has_value(),
         "frpak catalog should reject zero row-span headers");
}

void test_frpak_catalog_record_bounds_validation() {
  const comic2::FrpakCatalogRecord valid{
      .pak_id = 1,
      .record_id = 0,
      .data_offset = 2,
      .data_size = 4,
      .row_span_bytes = 0x20,
  };
  expect(comic2::validate_frpak_catalog_record_bounds(valid, 6),
         "frpak bounds validator should accept in-range records");

  const comic2::FrpakCatalogRecord offset_past_end{
      .pak_id = 1,
      .record_id = 1,
      .data_offset = 7,
      .data_size = 0,
      .row_span_bytes = 0x20,
  };
  expect(!comic2::validate_frpak_catalog_record_bounds(offset_past_end, 6),
         "frpak bounds validator should reject offsets beyond file size");

  const comic2::FrpakCatalogRecord size_overflow{
      .pak_id = 1,
      .record_id = 2,
      .data_offset = 4,
      .data_size = 3,
      .row_span_bytes = 0x20,
  };
  expect(!comic2::validate_frpak_catalog_record_bounds(size_overflow, 6),
         "frpak bounds validator should reject records exceeding file size");
}

void test_bootstrap_populates_frpak_catalog_for_known_files() {
  const auto root =
      std::filesystem::temp_directory_path() / "comic2_frpak_catalog_fixture";
  std::filesystem::remove_all(root);
  std::filesystem::create_directories(root);

  const std::vector<std::uint8_t> frpak_bytes = {
      0x40, 0x00, 0xAA, 0xBB, 0xCC,
  };
  {
    std::ofstream output(root / "FRPAK.001", std::ios::binary);
    output.write(reinterpret_cast<const char *>(frpak_bytes.data()),
                 static_cast<std::streamsize>(frpak_bytes.size()));
  }

  comic2::RuntimeState state;
  const auto summary = comic2::load_initial_bootstrap_resources(state, root);

  expect(summary.sprite_files_tried > 0,
         "bootstrap should attempt sprite candidate discovery");
  expect(state.frpak_catalog.files.size() == 1,
         "bootstrap should index valid FRPAK files in catalog");
  expect(state.frpak_catalog.files[0].pak_id == 1,
         "bootstrap catalog should preserve FRPAK numeric id");
  expect(state.frpak_catalog.files[0].records.size() == 1,
         "bootstrap catalog should include one record for direct payload file");

  const auto record =
      comic2::find_frpak_catalog_record(state.frpak_catalog, 1, 0);
  expect(record.has_value(),
         "catalog lookup should resolve known pak/record pair");
  expect(record->data_size == frpak_bytes.size(),
         "catalog record size should match source file size");

  std::filesystem::remove_all(root);
}

std::optional<std::filesystem::path> find_original_asset_root() {
  std::vector<std::filesystem::path> seeds;

  const std::filesystem::path file_path(__FILE__);
  if (file_path.is_absolute()) {
    seeds.push_back(file_path.parent_path().parent_path());
  }

  seeds.push_back(std::filesystem::current_path());

  if (const char *github_workspace = std::getenv("GITHUB_WORKSPACE");
      github_workspace != nullptr && *github_workspace != '\0') {
    seeds.emplace_back(github_workspace);
  }

  for (const auto &seed : seeds) {
    std::filesystem::path cursor = seed;
    for (;;) {
      const auto reference_candidate = cursor / "reference" / "original";
      if (std::filesystem::exists(reference_candidate) &&
          std::filesystem::is_directory(reference_candidate)) {
        for (const auto &entry :
             std::filesystem::directory_iterator(reference_candidate)) {
          const auto name = entry.path().filename().string();
          if (name.rfind("FRDATA.", 0) == 0 && entry.is_regular_file()) {
            return reference_candidate;
          }
        }
      }

      const auto original_candidate = cursor / "original";
      if (std::filesystem::exists(original_candidate) &&
          std::filesystem::is_directory(original_candidate)) {
        for (const auto &entry :
             std::filesystem::directory_iterator(original_candidate)) {
          const auto name = entry.path().filename().string();
          if (name.rfind("FRDATA.", 0) == 0 && entry.is_regular_file()) {
            return original_candidate;
          }
        }
      }

      const auto parent = cursor.parent_path();
      if (parent == cursor || parent.empty()) {
        break;
      }
      cursor = parent;
    }
  }

  return std::nullopt;
}

void test_room_loader_prefers_exact_level_tuple_room_file() {
  const auto root =
      std::filesystem::temp_directory_path() / "comic2_room_tuple_exact";
  std::filesystem::remove_all(root);
  std::filesystem::create_directories(root);

  const auto target_bytes = make_test_room_resource_bytes(4, 0, 4, 3, 0xAAU);
  const auto fallback_bytes = make_test_room_resource_bytes(3, 0, 4, 3, 0x55U);

  std::ofstream(root / "FR004.1", std::ios::binary)
      .write(reinterpret_cast<const char *>(target_bytes.data()),
             static_cast<std::streamsize>(target_bytes.size()));
  std::ofstream(root / "FR003.1", std::ios::binary)
      .write(reinterpret_cast<const char *>(fallback_bytes.data()),
             static_cast<std::streamsize>(fallback_bytes.size()));
  std::ofstream(root / "FR001.0", std::ios::binary).write("\x00", 1);
  std::ofstream(root / "FR004.2", std::ios::binary).write("\x00", 1);
  std::ofstream(root / "FR004.3", std::ios::binary).write("\x00", 1);

  comic2::RuntimeState state;
  const bool loaded =
      comic2::load_room_tilemap_from_asset_root(state, root, 4, 0);
  expect(loaded,
         "asset-root room load should succeed with the exact level tuple");
  expect(state.room_grid.tile_data[0] == 0xAA,
         "asset-root loader should prefer the canonical FR004.1 room payload");

  std::filesystem::remove_all(root);
}

void test_room_loader_falls_back_when_canonical_room_file_is_corrupt() {
  const auto root =
      std::filesystem::temp_directory_path() / "comic2_room_tuple_fallback";
  std::filesystem::remove_all(root);
  std::filesystem::create_directories(root);

  const auto fallback_bytes = make_test_room_resource_bytes(4, 0, 4, 3, 0x66U);
  std::ofstream(root / "FR004.1", std::ios::binary).write("\x00\x00\x00", 3);
  std::ofstream(root / "FR004.9", std::ios::binary)
      .write(reinterpret_cast<const char *>(fallback_bytes.data()),
             static_cast<std::streamsize>(fallback_bytes.size()));
  std::ofstream(root / "FR001.0", std::ios::binary).write("\x00", 1);
  std::ofstream(root / "FR004.2", std::ios::binary).write("\x00", 1);
  std::ofstream(root / "FR004.3", std::ios::binary).write("\x00", 1);

  comic2::RuntimeState state;
  const bool loaded =
      comic2::load_room_tilemap_from_asset_root(state, root, 4, 0);
  expect(loaded, "asset-root room loader should fallback when the canonical "
                 "room file is corrupt");
  expect(state.room_grid.tile_data[0] == 0x66,
         "fallback scan should keep using a valid room payload when the "
         "canonical file is unusable");

  std::filesystem::remove_all(root);
}

void test_room_loader_loads_level_tileset_from_asset_root_tuple() {
  const auto root =
      std::filesystem::temp_directory_path() / "comic2_room_tileset_tuple";
  std::filesystem::remove_all(root);
  std::filesystem::create_directories(root);

  const std::vector<std::uint8_t> tileset_payload = {
      0x00, 0x00, 0x1A, 0x00, 0x28, 0x00, 0x80, 0x00, 0x80, 0x01, 0x00,
  };
  const auto room_bytes = make_test_room_resource_bytes(0, 0, 4, 3, 0x01U);

  std::ofstream(root / "FR000.0", std::ios::binary)
      .write(reinterpret_cast<const char *>(tileset_payload.data()),
             static_cast<std::streamsize>(tileset_payload.size()));
  std::ofstream(root / "FR000.1", std::ios::binary)
      .write(reinterpret_cast<const char *>(room_bytes.data()),
             static_cast<std::streamsize>(room_bytes.size()));
  std::ofstream(root / "FR000.2", std::ios::binary).write("\x00", 1);
  std::ofstream(root / "FR004.3", std::ios::binary).write("\x00", 1);

  comic2::RuntimeState state;
  const bool loaded =
      comic2::load_room_tilemap_from_asset_root(state, root, 0, 0);
  expect(
      loaded,
      "asset-root loader should load a canonical tuple with a valid tileset");
  expect_eq(state.level_tileset.size(), 2,
            "level tileset should decode exactly two 16x16 4-plane tiles");
  expect(state.tile_hazard_bounds[0] == 0x001A &&
             state.tile_hazard_bounds[1] == 0x0028,
         "tuple tileset hazard bounds should be committed to runtime state");
  expect(state.room_grid.tile_data[0] == 0x01,
         "room payload should still load after tuple tileset hydrate");

  std::filesystem::remove_all(root);
}

std::vector<std::uint8_t> make_test_masked_sprite_sheet_bytes() {
  auto append_record =
      [](std::vector<std::uint8_t> &out, std::uint16_t width,
         std::uint16_t height, const std::vector<std::uint8_t> &mask,
         const std::array<std::vector<std::uint8_t>, 4> &planes) {
        const std::size_t image_offset = mask.size();
        const std::size_t record_size = 6U + image_offset + (4U * mask.size());
        std::vector<std::uint8_t> record(record_size, 0x00);
        record[0] = static_cast<std::uint8_t>(width & 0xFFU);
        record[1] = static_cast<std::uint8_t>((width >> 8) & 0xFFU);
        record[2] = static_cast<std::uint8_t>(height & 0xFFU);
        record[3] = static_cast<std::uint8_t>((height >> 8) & 0xFFU);
        record[4] = static_cast<std::uint8_t>(image_offset & 0xFFU);
        record[5] = static_cast<std::uint8_t>((image_offset >> 8) & 0xFFU);
        std::copy(mask.begin(), mask.end(), record.begin() + 6);
        const std::size_t plane_bytes = mask.size();
        for (std::size_t plane = 0; plane < 4; ++plane) {
          std::copy(planes[plane].begin(), planes[plane].end(),
                    record.begin() +
                        static_cast<std::ptrdiff_t>(6U + image_offset +
                                                    plane * plane_bytes));
        }
        out.insert(out.end(), record.begin(), record.end());
      };

  std::vector<std::uint8_t> payload;

  std::vector<std::uint8_t> record1_mask(48, 0x00);
  for (std::size_t i = 0; i < record1_mask.size(); ++i) {
    record1_mask[i] = static_cast<std::uint8_t>((i % 5U) + 1U);
  }
  std::array<std::vector<std::uint8_t>, 4> record1_planes = {
      std::vector<std::uint8_t>(48, 0x11U),
      std::vector<std::uint8_t>(48, 0x22U),
      std::vector<std::uint8_t>(48, 0x33U),
      std::vector<std::uint8_t>(48, 0x44U),
  };
  append_record(payload, 16, 24, record1_mask, record1_planes);

  std::vector<std::uint8_t> record2_mask(8, 0x00);
  for (std::size_t i = 0; i < record2_mask.size(); ++i) {
    record2_mask[i] = static_cast<std::uint8_t>((i % 7U) + 2U);
  }
  std::array<std::vector<std::uint8_t>, 4> record2_planes = {
      std::vector<std::uint8_t>(8, 0xAAU),
      std::vector<std::uint8_t>(8, 0xBBU),
      std::vector<std::uint8_t>(8, 0xCCU),
      std::vector<std::uint8_t>(8, 0xDDU),
  };
  append_record(payload, 8, 8, record2_mask, record2_planes);

  std::vector<std::uint8_t> terminator(6, 0x00);
  payload.insert(payload.end(), terminator.begin(), terminator.end());
  return payload;
}

void test_phase11_masked_sprite_sheet_decode_synthetic() {
  const auto payload = make_test_masked_sprite_sheet_bytes();
  const auto decoded = comic2::decode_masked_sprite_sheet(payload);

  expect(decoded.has_value(), "synthetic sprite payload should decode");
  expect_eq(decoded->size(), 2,
            "synthetic sprite sheet should decode exactly two records");

  expect(decoded->at(0).width_pixels == 16,
         "first sprite width should match the synthetic record");
  expect(decoded->at(0).height_rows == 24,
         "first sprite height should match the synthetic record");
  expect(decoded->at(0).image_data_off == 48,
         "first sprite image offset should match mask byte count");
  expect(decoded->at(0).mask_bytes.size() == 48,
         "first sprite mask should be 2 bytes x 24 rows");
  expect(decoded->at(0).planes[0].size() == 48,
         "first sprite plane 0 should contain the expected image bytes");
  expect(decoded->at(0).planes[0][0] == 0x11U,
         "first sprite plane 0 should preserve its synthetic image bytes");
  expect(decoded->at(0).planes[3][47] == 0x44U,
         "first sprite plane 3 should preserve the last synthetic image byte");

  expect(decoded->at(1).width_pixels == 8,
         "second sprite width should match the synthetic record");
  expect(decoded->at(1).mask_bytes.size() == 8,
         "second sprite mask size should match 8x8 / 8 bytes");
  expect(decoded->at(1).planes[1][0] == 0xBBU,
         "second sprite plane 1 should preserve raw image data");
  expect(decoded->at(1).planes[3][7] == 0xDDU,
         "second sprite plane 3 should preserve the last raw image byte");
}

void test_phase11_fr000_masked_sprite_sheet_decode_parity() {
  const auto root = find_original_asset_root();
  if (!root.has_value()) {
    if (require_original_assets()) {
      throw std::runtime_error(
          "COMIC2_REQUIRE_ORIGINAL_ASSETS is set, but reference/original "
          "assets containing FRDATA.* were not found");
    }
    std::cerr << "Skipping FR000 sprite-sheet parity test: reference/original "
                 "assets are not present.\n";
    return;
  }

  const auto path = root.value() / "FR000.2";
  expect(std::filesystem::exists(path),
         "FR000.2 should exist in the original asset root");

  const auto bytes = comic2::load_file_bytes(path);
  expect(bytes.has_value(),
         "FR000.2 should be readable for sprite decode parity");

  const auto decoded =
      comic2::decode_masked_sprite_sheet(std::span<const std::uint8_t>(*bytes));
  expect(decoded.has_value(), "FR000.2 should decode as a valid sprite sheet");
  expect_eq(decoded->size(), 32,
            "FR000.2 should contain 32 sequential masked records");

  const std::array<std::size_t, 4> sample_points = {0U, 1U, 8U, 31U};
  for (const auto index : sample_points) {
    expect(index < decoded->size(),
           "FR000.2 sample record index should stay within the decoded sheet");
    const auto &sprite = decoded->at(index);
    expect(sprite.width_pixels > 0 && sprite.height_rows > 0,
           "FR000.2 sample sprite should have valid non-zero dimensions");
    expect(sprite.mask_bytes.size() ==
               ((sprite.width_pixels + 7U) / 8U) * sprite.height_rows,
           "FR000.2 mask payload should match the packed 1-bit bitmap sizing");
    expect(sprite.planes.size() == 4,
           "FR000.2 sample sprite should expose the four EGA image planes");
    for (const auto &plane : sprite.planes) {
      expect(plane.size() == sprite.mask_bytes.size(),
             "FR000.2 sprite plane payloads should match the mask dimensions");
    }
  }

  expect(decoded->front().width_pixels == 16,
         "first sprite in FR000.2 should be a 16-pixel wide Comic frame");
  expect(decoded->front().height_rows == 24,
         "first sprite in FR000.2 should be 24 rows tall");
  expect(decoded->front().mask_bytes.size() == 48,
         "first FR000.2 mask should be 48 bytes for 16x24 data");
  expect(decoded->front().planes.size() == 4,
         "FR000.2 sprite plane array should carry four image planes");
  expect(decoded->front().planes[0].size() == 48,
         "FR000.2 first sprite plane 0 should be 48 bytes");
  expect(decoded->front().planes[1].size() == 48,
         "FR000.2 first sprite plane 1 should be 48 bytes");
  expect(decoded->front().planes[2].size() == 48,
         "FR000.2 first sprite plane 2 should be 48 bytes");
  expect(decoded->front().planes[3].size() == 48,
         "FR000.2 first sprite plane 3 should be 48 bytes");
}

void test_phase11_level_resource_tuple_catalog() {
  const auto root = find_original_asset_root();
  if (!root.has_value()) {
    if (require_original_assets()) {
      throw std::runtime_error(
          "COMIC2_REQUIRE_ORIGINAL_ASSETS is set, but reference/original "
          "assets containing FRDATA.* were not found");
    }
    std::cerr << "Skipping level resource tuple catalog test: "
                 "reference/original assets are not present.\n";
    return;
  }

  for (std::uint16_t level = 0; level <= 14; ++level) {
    const auto tuple =
        comic2::resolve_level_resource_tuple(level, root.value());
    expect(tuple.has_value(), "all 15 canonical level tuples should resolve in "
                              "the original asset root");
    expect(!tuple->tileset_filename.empty(),
           "level tuple should carry a tileset filename");
    expect(!tuple->room_layout_filename.empty(),
           "level tuple should carry a room-layout filename");
    expect(!tuple->sprite_sheet_filename.empty(),
           "level tuple should carry a sprite-sheet filename");
    expect(std::filesystem::exists(root.value() / tuple->tileset_filename),
           "level tuple tileset should exist in the original asset root");
    expect(std::filesystem::exists(root.value() / tuple->room_layout_filename),
           "level tuple room file should exist in the original asset root");
    expect(std::filesystem::exists(root.value() / tuple->sprite_sheet_filename),
           "level tuple sprite file should exist in the original asset root");
    if (tuple->has_script()) {
      expect(std::filesystem::exists(root.value() / tuple->script_filename),
             "level tuple script should exist in the original asset root");
    }
  }

  const auto level0 = comic2::resolve_level_resource_tuple(0, root.value());
  expect(level0.has_value(), "level 0 tuple should resolve to original files");
  expect(level0->tileset_filename == "FR000.0",
         "level 0 tileset filename should match original catalog");
  expect(level0->room_layout_filename == "FR000.1",
         "level 0 room filename should match original catalog");
  expect(level0->sprite_sheet_filename == "FR000.2",
         "level 0 sprite filename should match original catalog");

  const auto level4 = comic2::resolve_level_resource_tuple(4, root.value());
  expect(level4.has_value(), "level 4 tuple should resolve to original files");
  expect(level4->tileset_filename == "FR001.0",
         "level 4 should reuse FR001.0 tileset");
  expect(level4->room_layout_filename == "FR004.1",
         "level 4 room filename should match original catalog");

  const auto invalid = comic2::resolve_level_resource_tuple(99, root.value());
  expect(!invalid.has_value(), "out-of-range level ids should fail cleanly");
}

void test_phase11_fr000_room0_layout_loads_from_asset_root() {
  const auto root = find_original_asset_root();
  if (!root.has_value()) {
    if (require_original_assets()) {
      throw std::runtime_error(
          "COMIC2_REQUIRE_ORIGINAL_ASSETS is set, but reference/original "
          "assets containing FRDATA.* were not found");
    }
    std::cerr << "Skipping FR000 room 0 parity test: reference/original "
                 "assets are not present.\n";
    return;
  }

  const auto tileset_bytes = comic2::load_file_bytes(root.value() / "FR000.0");
  expect(tileset_bytes.has_value(), "FR000.0 should be readable");
  const auto tileset = comic2::decode_level_tileset(
      std::span<const std::uint8_t>(*tileset_bytes));
  expect(tileset.has_value(), "FR000.0 should decode to a valid tileset");

  comic2::RuntimeState state;
  state.level_tileset = tileset->tiles;
  state.tile_hazard_bounds = tileset->hazard_bounds;

  const bool loaded =
      comic2::load_room_tilemap_from_asset_root(state, root.value(), 0, 0);
  expect(loaded,
         "Level 0 Room 0 should load from FR000.1 in the original asset root");
  expect(state.room_grid.tile_w == 80,
         "Level 0 Room 0 should resolve to the original 80-tile width");
  expect(state.room_grid.tile_h == 30,
         "Level 0 Room 0 should resolve to the original 30-tile height");
  expect_eq(state.room_grid.tile_data.size(), 2400U,
            "Level 0 Room 0 should unpack the full 80x30 room tile buffer");
  expect(state.room_grid.row_pointers.size() == 30,
         "Level 0 Room 0 row-pointer table should cover every room row");
  expect(!state.room_grid.tile_data.empty(),
         "Level 0 Room 0 tile data should remain populated after load");
  for (const auto tile_index : state.room_grid.tile_data) {
    expect(static_cast<std::size_t>(tile_index) < state.level_tileset.size(),
           "room tile indices should remain in bounds for the loaded tileset");
  }
  expect(!state.room_resource_bytes.empty(),
         "Level 0 Room 0 should retain the loaded room bytes for downstream "
         "parsing");
}

void test_phase11_fr000_tileset_decode_parity() {
  const auto root = find_original_asset_root();
  if (!root.has_value()) {
    if (require_original_assets()) {
      throw std::runtime_error(
          "COMIC2_REQUIRE_ORIGINAL_ASSETS is set, but reference/original "
          "assets containing FRDATA.* were not found");
    }
    std::cerr << "Skipping FR000 tileset parity test: reference/original "
                 "assets are not present.\n";
    return;
  }

  const auto bytes = comic2::load_file_bytes(root.value() / "FR000.0");
  expect(bytes.has_value(), "FR000.0 should be readable for decode parity");

  const auto decoded =
      comic2::decode_level_tileset(std::span<const std::uint8_t>(*bytes));
  expect(decoded.has_value(), "FR000.0 should decode as a valid tileset");
  expect(decoded->hazard_bounds[0] == 0x002A,
         "hazard min should be parsed from the FR000.0 header");
  expect(decoded->hazard_bounds[1] == 0x0033,
         "hazard max should be parsed from the FR000.0 header");
  expect_eq(decoded->tiles.size(), 144,
            "FR000.0 should decode to exactly 144 16x16 4-plane tiles");
  expect(decoded->tiles[0].row_span_bytes == 32,
         "tile row span should reflect 2 bytes x 16 rows per plane");
  expect(decoded->tiles[0].width_bytes == 2,
         "tile width bytes should be 2 bytes for a 16-pixel row");
  expect(decoded->tiles[0].height_rows == 16,
         "tile height should remain 16 rows for each 16x16 tile");
  expect(decoded->tiles[0].planes[0].size() == 32,
         "each plane should contribute exactly 32 bytes in a 16x16 tile");
}

void test_phase11_player_animation_state_mapping_contract() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.level_sprites.resize(16);
  for (std::size_t i = 0; i < state.level_sprites.size(); ++i) {
    auto &sprite = state.level_sprites[i];
    sprite.width_pixels = 16;
    sprite.height_rows = 24;
    sprite.mask_bytes.assign(48, 0xFFU);
    for (std::size_t plane = 0; plane < sprite.planes.size(); ++plane) {
      sprite.planes[plane].assign(48, static_cast<std::uint8_t>(i + plane));
    }
  }

  auto assert_index = [&](std::uint8_t animation_state, std::uint8_t frame,
                          bool facing_right, std::size_t expected) {
    state.player.animation_state = animation_state;
    state.player.animation_frame = frame;
    state.player.facing_right = facing_right;
    const auto actual = comic2::select_player_sprite_frame(state);
    expect(actual == expected,
           "player animation contract should map to the expected sprite index");
  };

  assert_index(static_cast<std::uint8_t>(comic2::PlayerAnimationState::Idle), 0,
               true, 0U);
  assert_index(
      static_cast<std::uint8_t>(comic2::PlayerAnimationState::WalkCycle), 0,
      true, 1U);
  assert_index(
      static_cast<std::uint8_t>(comic2::PlayerAnimationState::WalkCycle), 1,
      true, 2U);
  assert_index(
      static_cast<std::uint8_t>(comic2::PlayerAnimationState::WalkCycle), 2,
      true, 3U);
  assert_index(
      static_cast<std::uint8_t>(comic2::PlayerAnimationState::JumpRise), 0,
      true, 4U);
  assert_index(static_cast<std::uint8_t>(comic2::PlayerAnimationState::Attack),
               0, true, 5U);
  assert_index(static_cast<std::uint8_t>(comic2::PlayerAnimationState::Hurt), 0,
               true, 6U);
  assert_index(static_cast<std::uint8_t>(comic2::PlayerAnimationState::Death),
               0, true, 7U);

  assert_index(static_cast<std::uint8_t>(comic2::PlayerAnimationState::Idle), 0,
               false, 8U);
  assert_index(
      static_cast<std::uint8_t>(comic2::PlayerAnimationState::WalkCycle), 2,
      false, 11U);
  assert_index(
      static_cast<std::uint8_t>(comic2::PlayerAnimationState::JumpRise), 0,
      false, 12U);
  assert_index(static_cast<std::uint8_t>(comic2::PlayerAnimationState::Attack),
               0, false, 13U);
  assert_index(static_cast<std::uint8_t>(comic2::PlayerAnimationState::Hurt), 0,
               false, 14U);
  assert_index(static_cast<std::uint8_t>(comic2::PlayerAnimationState::Death),
               0, false, 15U);
}

} // namespace

void run_subsystem_scaffold_tests() {
  test_entity_runtime_prunes_inactive_slots();
  test_ent_build_room_entity_list_filters_by_room();
  test_ent_build_runtime_slots_for_viewport_culls();
  test_ent_copy_descriptor_to_runtime_slot_initializes();
  test_deactivate_runtime_slot_clears();
  test_projectile_updates_and_despawns_out_of_bounds();
  test_spawn_player_projectile_with_facing();
  test_spawn_player_projectile_airborne_adjustment();
  test_projectile_tile_collision_detection();
  test_projectile_viewport_culling();
  test_projectile_anim_frame_cycles();
  test_ent_activation_pipeline_integration();
  test_room_loader_decodes_frdata_entry();
  test_room_loader_resolves_room_payload_location();
  test_room_loader_parses_room_count_and_level_id_header();
  test_room_loader_rejects_huge_offset();
  test_room_loader_rejects_out_of_bounds_room_index();
  test_room_loader_rejects_malformed_room_header();
  test_room_loader_rejects_sentinel_entries();
  test_room_loader_populates_runtime_state_from_resource_buffer();
  test_room_loader_decodes_signed_rle_room_tilemap_dimensions();
  test_room_loader_validates_tile_indices_against_tileset_size();
  test_room_loader_rejects_out_of_range_tile_indices_cleanly();
  test_room_loader_handles_self_referential_resource_bytes_span();
  test_room_loader_decodes_mapped_objects_from_payload();
  test_room_loader_wires_runtime_tables_from_loaded_mapped_objects();
  test_room_loader_handles_corrupt_mapped_object_payload_stably();
  test_frpak_catalog_builds_file_index();
  test_frpak_catalog_rejects_truncated_header();
  test_frpak_catalog_rejects_zero_row_span_header();
  test_frpak_catalog_record_bounds_validation();
  test_bootstrap_populates_frpak_catalog_for_known_files();
  test_room_loader_prefers_exact_level_tuple_room_file();
  test_room_loader_falls_back_when_canonical_room_file_is_corrupt();
  test_room_loader_loads_level_tileset_from_asset_root_tuple();
  test_phase11_masked_sprite_sheet_decode_synthetic();
  test_phase11_player_animation_state_mapping_contract();
  test_phase11_fr000_masked_sprite_sheet_decode_parity();
  test_phase11_level_resource_tuple_catalog();
  test_phase11_fr000_room0_layout_loads_from_asset_root();
  test_phase11_fr000_tileset_decode_parity();
}
