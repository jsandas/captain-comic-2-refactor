#include <stdexcept>
#include <vector>

#include "comic2/entity_physics.hpp"
#include "comic2/room_loader.hpp"

namespace {

void expect(bool c, const char *m) {
  if (!c) {
    throw std::runtime_error(m);
  }
}

// 16x8 tile room; row 5 (y 80..95) is a solid floor (id 1), rest 0xFF open.
comic2::RoomTileGrid make_grid() {
  comic2::RoomTileGrid g;
  g.tile_w = 16;
  g.tile_h = 8;
  for (int r = 0; r < 8; ++r) {
    g.row_pointers.push_back(static_cast<std::uint16_t>(r * 16));
  }
  g.tile_data.assign(16 * 8, 0xFF);
  for (int x = 0; x < 16; ++x) {
    g.tile_data[5 * 16 + x] = 0x01;
  }
  g.tile_data[3 * 16 + 8] = 0x01; // wall block at tile (8,3)
  return g;
}

comic2::EntityPhysicsContext make_ctx(const comic2::RoomTileGrid &g) {
  comic2::EntityPhysicsContext c;
  c.grid = &g;
  c.threshold_up = 1;
  c.threshold_down = 1;
  return c;
}

void test_decode_level_script() {
  const std::vector<std::uint8_t> enc = {0x25, 0x24, 0x00, 0x25 ^ 0x7F};
  const auto dec = comic2::decode_level_script(enc);
  expect(dec.size() == 4 && dec[0] == 0 && dec[1] == 1 && dec[2] == 0x25 &&
             dec[3] == 0x7F,
         "xor25 decode mismatch");
  const auto off = comic2::level_script_offset(dec, 0);
  expect(off && *off == 0x0100, "script offset mismatch");
  expect(!comic2::level_script_offset(dec, 2), "offset oob should be nullopt");
}

void test_gravity_lands_and_snaps() {
  const auto g = make_grid();
  const auto ctx = make_ctx(g);
  comic2::RuntimeEntitySlot32 s{};
  s.hitbox_w = 16;
  s.hitbox_h = 16;
  s.x = 32;
  s.y = 40;
  bool landed = false;
  for (int i = 0; i < 20 && !landed; ++i) {
    landed = comic2::ent_apply_gravity_and_move_y(ctx, s);
  }
  expect(landed, "entity should land on floor");
  expect(s.y == 64 && s.param_b == 0, "entity should snap on top of floor");
}

void test_move_x_bounces_off_wall() {
  const auto g = make_grid();
  const auto ctx = make_ctx(g);
  comic2::RuntimeEntitySlot32 s{};
  s.hitbox_w = 16;
  s.hitbox_h = 16;
  s.x = 100;
  s.y = 48; // probe row = (48+16-8)/16 = 3
  s.dir_toggle = 1;
  bool bounced = false;
  for (int i = 0; i < 10 && !bounced; ++i) {
    bounced = comic2::ent_move_x_and_bounce(ctx, s, 2);
  }
  expect(bounced, "should bounce off wall");
  expect(s.dir_toggle == 2, "direction should flip");
  expect(s.param_a == -2, "param_a should be negated");
  // Now moving left: deterministic retreat.
  const auto x0 = s.x;
  expect(!comic2::ent_move_x_and_bounce(ctx, s, 2), "leftward move ok");
  expect(s.x == x0 - 2, "leftward move distance");
}

void test_move_y_bounce_reflects_velocity() {
  const auto g = make_grid();
  auto ctx = make_ctx(g);
  comic2::RuntimeEntitySlot32 s{};
  s.hitbox_w = 16;
  s.hitbox_h = 16;
  s.x = 32;
  s.y = 60;
  s.param_b = 8;
  expect(comic2::ent_move_y_and_bounce(ctx, s), "floor bounce expected");
  expect(s.param_b == -8 && s.y == 64, "reflected velocity and snapped y");
  expect(!comic2::ent_move_y_and_bounce(ctx, s), "upward move free");
  expect(s.y == 56, "upward move applied");
}

void test_tile_trigger_range() {
  const std::vector<std::uint8_t> b = {0x10, 0x12, 7, 8, 9, 0xAA};
  const auto r = comic2::parse_tile_trigger_range(b);
  expect(r && r->values.size() == 3, "range parse");
  expect(comic2::evaluate_tile_trigger(*r, 0x11) == 8, "in-range lookup");
  expect(!comic2::evaluate_tile_trigger(*r, 0x0F), "below range");
  expect(!comic2::evaluate_tile_trigger(*r, 0x13), "above range");
  const std::vector<std::uint8_t> bad = {0x10, 0x20, 1};
  expect(!comic2::parse_tile_trigger_range(bad), "truncated range rejected");
}

} // namespace

#include "comic2/level_data.hpp"

void test_extracted_level_data() {
  const auto *lvl0 = comic2::get_level_runtime_data(0);
  expect(lvl0 != nullptr, "lvl0 data exists");
  expect(lvl0->room_count == 8, "lvl0 has 8 rooms");
  expect(lvl0->touch_trigger.min_tile == 0x1A, "lvl0 touch trigger min");
  expect(lvl0->touch_trigger.max_tile == 0x1C, "lvl0 touch trigger max");
  expect(lvl0->touch_trigger.values.size() == 3,
         "lvl0 touch trigger values count");
  expect(lvl0->touch_trigger.values[0] == 57, "lvl0 touch trigger val 0");
  expect(lvl0->touch_trigger.values[1] == 80, "lvl0 touch trigger val 1");
  expect(lvl0->touch_trigger.values[2] == 139, "lvl0 touch trigger val 2");

  // Room 0 transition zones test
  expect(lvl0->transition_zones.size() == 8,
         "lvl0 transition zones room count");
  expect(lvl0->transition_zones[0].size() == 4,
         "room 0 has 4 transition zones");
  expect(lvl0->transition_zones[0][0].x == 0, "zone 0 x coordinate");
  expect(lvl0->transition_zones[0][0].y_min == 0, "zone 0 y_min");
  expect(lvl0->transition_zones[0][0].y_max == 16, "zone 0 y_max");
  expect(lvl0->transition_zones[0][0].target_room == 1, "zone 0 target room");

  // Trigger evaluation
  auto trig_res = comic2::evaluate_tile_trigger(lvl0->touch_trigger, 0x1B);
  expect(trig_res.has_value() && *trig_res == 80, "eval touch trigger 0x1B");

  // Check transition zone query
  auto hit_zone = comic2::check_room_transition_zone(0, 0, 0, 10);
  expect(hit_zone.has_value(), "transition zone hit at (0, 10)");
  expect(hit_zone->target_room == 1 && hit_zone->spawn_index == 0,
         "transition zone target room and spawn index");

  auto miss_zone = comic2::check_room_transition_zone(0, 0, 0, 30);
  expect(!miss_zone.has_value(), "transition zone miss at (0, 30)");

  // Level 14 check
  const auto *lvl14 = comic2::get_level_runtime_data(14);
  expect(lvl14 != nullptr, "lvl14 data exists");
  expect(lvl14->room_count == 12, "lvl14 has 12 rooms");
}

void run_entity_physics_tests() {
  test_decode_level_script();
  test_tile_trigger_range();
  test_extracted_level_data();
  test_gravity_lands_and_snaps();
  test_move_x_bounces_off_wall();
  test_move_y_bounce_reflects_velocity();
}
