#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <vector>

#include "comic2/audio.hpp"
#include "comic2/bootstrap.hpp"
#include "comic2/default_handlers.hpp"
#include "comic2/dispatcher.hpp"
#include "comic2/entity_runtime.hpp"
#include "comic2/level_data.hpp"
#include "comic2/oracle.hpp"
#include "comic2/persistence.hpp"
#include "comic2/player_controller.hpp"
#include "comic2/projectiles.hpp"
#include "comic2/renderer.hpp"
#include "comic2/renderer_validation.hpp"
#include "comic2/room_loader.hpp"
#include "comic2/ui.hpp"

namespace {

void expect(bool condition, const char *message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

struct StateSnapshot {
  std::int16_t x;
  std::int16_t y;
  std::int16_t x_vel;
  std::int16_t y_vel;
  bool is_airborne;
  bool is_physics_active;
  bool tile_hazard_triggered;

  bool operator==(const StateSnapshot &) const = default;
};

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

comic2::RoomTileGrid make_single_tile_grid(std::uint8_t tile_id) {
  comic2::RoomTileGrid grid;
  grid.tile_w = 1;
  grid.tile_h = 1;
  grid.row_pointers = {0};
  grid.tile_data = {tile_id};
  return grid;
}

comic2::RoomTileGrid make_hazard_floor_grid(std::uint8_t tile_id) {
  comic2::RoomTileGrid grid;
  grid.tile_w = 1;
  grid.tile_h = 2;
  grid.row_pointers = {0, 1};
  grid.tile_data = {tile_id, tile_id};
  return grid;
}

StateSnapshot capture_snapshot(const comic2::RuntimeState &state) {
  return StateSnapshot{
      .x = state.player.x,
      .y = state.player.y,
      .x_vel = state.player.x_vel,
      .y_vel = state.player.y_vel,
      .is_airborne = state.player.is_airborne,
      .is_physics_active = state.player.is_physics_active,
      .tile_hazard_triggered = state.flags.tile_hazard_triggered,
  };
}

std::uint64_t make_room_fixture_alpha_hash() {
  comic2::EgaPlanarSurface frame(320, 200);
  frame.clear(0x00);

  const auto base =
      comic2::validation::make_8x8_solid_image(0x11, 0x22, 0x44, 0x88);
  const auto overlay =
      comic2::validation::make_8x8_solid_image(0x0F, 0xF0, 0x33, 0xCC);

  comic2::gfx_rle_blit_opaque_4plane(frame, 0, 0, base);
  comic2::gfx_rle_blit_masked_or_4plane(frame, 16, 4, overlay);

  return comic2::validation::hash_surface_planes(frame);
}

std::uint64_t make_room_fixture_beta_hash() {
  comic2::EgaPlanarSurface frame(320, 200);
  frame.clear(0xAA);

  const auto patterned = comic2::validation::make_16x16_patterned_image();
  const auto overlay =
      comic2::validation::make_8x8_solid_image(0x01, 0x02, 0x04, 0x08);

  comic2::gfx_rle_blit_opaque_4plane(frame, 32, 10, patterned);
  comic2::gfx_rle_blit_masked_or_4plane(frame, 32, 10, overlay);

  return comic2::validation::hash_surface_planes(frame);
}

void expect_hash_eq(std::uint64_t actual, std::uint64_t expected,
                    const char *context) {
  if (actual == expected) {
    return;
  }

  std::ostringstream oss;
  oss << context << " expected hash 0x" << std::hex << expected << " but got 0x"
      << actual;
  throw std::runtime_error(oss.str());
}

std::vector<comic2::DispatchStage>
capture_dispatcher_trace(comic2::GameDispatcher &dispatcher,
                         comic2::RuntimeState &state,
                         const std::vector<comic2::InputState> &inputs) {
  dispatcher.clear_trace();
  dispatcher.set_trace_enabled(true);

  for (const auto &input : inputs) {
    state.input = input;
    dispatcher.run_tick(state);
  }

  return dispatcher.trace_log();
}

void expect_dispatcher_trace_matches_oracle(
    const std::vector<comic2::DispatchStage> &actual,
    const std::vector<comic2::DispatchStage> &expected, const char *context) {
  if (actual == expected) {
    return;
  }

  std::ostringstream oss;
  oss << context << " expected trace [";
  for (std::size_t i = 0; i < expected.size(); ++i) {
    if (i > 0) {
      oss << ", ";
    }
    oss << comic2::to_string(expected[i]);
  }
  oss << "] but got [";
  for (std::size_t i = 0; i < actual.size(); ++i) {
    if (i > 0) {
      oss << ", ";
    }
    oss << comic2::to_string(actual[i]);
  }
  oss << "]";
  throw std::runtime_error(oss.str());
}

void test_gate_b_scripted_trace_matches_oracle() {
  comic2::GameDispatcher dispatcher;
  comic2::install_default_stage_hooks(dispatcher);

  comic2::RuntimeState state;
  state.player.jump_counter = 2;

  const std::vector<comic2::InputState> sequence = {
      comic2::InputState{.jump_pressed = true,
                         .left_pressed = false,
                         .right_pressed = true,
                         .down_pressed = false},
      comic2::InputState{},
      comic2::InputState{},
      comic2::InputState{.jump_pressed = false,
                         .left_pressed = true,
                         .right_pressed = false,
                         .down_pressed = false},
      comic2::InputState{},
      comic2::InputState{},
  };

  const auto actual = capture_dispatcher_trace(dispatcher, state, sequence);

  const std::vector<comic2::DispatchStage> expected = {
      comic2::DispatchStage::InputHandling,
      comic2::DispatchStage::AirbornePhysics,
      comic2::DispatchStage::AirbornePhysics,
      comic2::DispatchStage::AirbornePhysics,
      comic2::DispatchStage::AirbornePhysics,
      comic2::DispatchStage::AirbornePhysics,
  };
  expect_dispatcher_trace_matches_oracle(
      actual, expected, "Gate B oracle trace mismatch for scripted input");

  comic2::GameDispatcher replay_dispatcher;
  comic2::install_default_stage_hooks(replay_dispatcher);

  comic2::RuntimeState replay_state;
  replay_state.player.jump_counter = 2;
  const auto replay =
      capture_dispatcher_trace(replay_dispatcher, replay_state, sequence);

  expect_dispatcher_trace_matches_oracle(replay, actual,
                                         "Gate B replay trace must be stable");
}

void test_gate_b_oracle_stage_sequence_rotation() {
  comic2::GameDispatcher dispatcher;
  comic2::install_default_stage_hooks(dispatcher);

  comic2::RuntimeState state;
  state.flags.level_transition_pending = true;
  state.player.is_airborne = true;

  const std::vector<comic2::InputState> sequence = {
      comic2::InputState{},
      comic2::InputState{},
      comic2::InputState{},
  };

  const auto actual = capture_dispatcher_trace(dispatcher, state, sequence);
  const std::vector<comic2::DispatchStage> expected = {
      comic2::DispatchStage::LevelTransition,
      comic2::DispatchStage::AirbornePhysics,
      comic2::DispatchStage::AirbornePhysics,
  };

  expect_dispatcher_trace_matches_oracle(
      actual, expected, "Gate B oracle stage sequence rotation mismatch");
}

void test_gate_c_state_vectors_match_snapshots() {
  comic2::GameDispatcher dispatcher;
  comic2::install_default_stage_hooks(dispatcher);

  comic2::RuntimeState state;
  state.player.jump_counter = 1;

  const std::vector<comic2::InputState> sequence = {
      comic2::InputState{.jump_pressed = true,
                         .left_pressed = false,
                         .right_pressed = true,
                         .down_pressed = false},
      comic2::InputState{},
      comic2::InputState{},
      comic2::InputState{},
      comic2::InputState{},
  };

  std::vector<StateSnapshot> snapshots;
  snapshots.reserve(sequence.size());

  for (const auto &input : sequence) {
    state.input = input;
    dispatcher.run_tick(state);
    snapshots.push_back(capture_snapshot(state));
  }

  const std::vector<StateSnapshot> expected = {
      StateSnapshot{.x = 8,
                    .y = 0,
                    .x_vel = 8,
                    .y_vel = -5,
                    .is_airborne = true,
                    .is_physics_active = true,
                    .tile_hazard_triggered = false},
      StateSnapshot{.x = 8,
                    .y = -5,
                    .x_vel = 0,
                    .y_vel = -4,
                    .is_airborne = true,
                    .is_physics_active = true,
                    .tile_hazard_triggered = false},
      StateSnapshot{.x = 8,
                    .y = -9,
                    .x_vel = 0,
                    .y_vel = -3,
                    .is_airborne = true,
                    .is_physics_active = true,
                    .tile_hazard_triggered = false},
      StateSnapshot{.x = 8,
                    .y = -12,
                    .x_vel = 0,
                    .y_vel = -2,
                    .is_airborne = true,
                    .is_physics_active = true,
                    .tile_hazard_triggered = false},
      StateSnapshot{.x = 8,
                    .y = -14,
                    .x_vel = 0,
                    .y_vel = -1,
                    .is_airborne = true,
                    .is_physics_active = true,
                    .tile_hazard_triggered = false},
  };

  expect(snapshots == expected,
         "Gate C state snapshots do not match expected per-tick vectors");
}

void test_gate_c_hazard_flag_snapshot_progression() {
  comic2::GameDispatcher dispatcher;
  comic2::install_default_stage_hooks(dispatcher);

  comic2::RuntimeState state;
  state.player.hp = 2;
  state.player.y = -16;
  state.player.is_physics_active = true;
  state.room_grid = make_hazard_floor_grid(0xF4);

  const auto first = dispatcher.run_tick(state);
  expect(first.stage == comic2::DispatchStage::GroundedPhysics,
         "Gate C hazard setup tick should run grounded physics");
  expect(state.flags.tile_hazard_triggered,
         "Gate C hazard flag should be set after grounded physics tick");

  const auto second = dispatcher.run_tick(state);
  expect(second.stage == comic2::DispatchStage::TileHazard,
         "Gate C hazard resolution tick should route to tile hazard stage");
  expect(state.player.hp == 0,
         "Gate C hazard tick should kill the player immediately");
  expect(state.flags.player_special_state_active,
         "Gate C hazard tick should enter the death path");
  expect(!state.flags.tile_hazard_triggered,
         "Gate C hazard tick should clear hazard flag");
}

void test_gate_86a_resolver_mapping_is_deterministic() {
  std::vector<std::uint8_t> bytes(0x40, 0x00);
  bytes[2] = 0x01;
  bytes[3] = 0x00;
  bytes[0x04] = 0x04;
  bytes[0x05] = 0x00;
  bytes[0x06] = 0x03;
  bytes[0x07] = 0x00;
  bytes[0x08] = 0x20;
  bytes[0x09] = 0x00;

  const auto spec = comic2::resolve_room_load_spec(
      {}, bytes, 1, 0, comic2::ResourceAssetKind::RoomPayload);

  expect(spec.has_value(), "Gate 8.6-A should resolve a valid room fixture");
  expect(spec->level == 1, "Gate 8.6-A level should match the fixture");
  expect(spec->room == 0, "Gate 8.6-A room should match the fixture");
  expect(spec->resource_offset == 0x20,
         "Gate 8.6-A resource offset should match the fixture");
}

void test_gate_86d_room_load_persists_resource_bytes_for_reentry() {
  std::vector<std::uint8_t> room_bytes(0x2C4, 0x00);
  room_bytes[0] = 0x01;
  room_bytes[0x2A0] = 0x00;
  room_bytes[0x2A1] = 0x00;
  room_bytes[0x2A2] = 0x04;
  room_bytes[0x2A3] = 0x00;
  room_bytes[0x2A4] = 0x08;
  room_bytes[0x2A5] = 0x00;

  const std::vector<std::uint8_t> encoded_room_bytes =
      encode_literal_signed_rle(room_bytes);

  std::vector<std::uint8_t> resource_bytes(0x20 + encoded_room_bytes.size(),
                                           0x00);
  resource_bytes[2] = 0x01;
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
  state.current_level = 0;
  state.current_room = 0;

  const bool loaded = comic2::load_room_tilemap_from_resource_buffer(
      state, resource_bytes, 1, 0);

  expect(loaded, "Gate 8.6-D should load a valid room fixture");
  expect(state.room_resource_bytes == resource_bytes,
         "Gate 8.6-D should preserve the loaded room payload bytes");
}

void test_gate_d_room_fixture_alpha_hash_matches_oracle() {
  // Oracle capture hash for the deterministic room fixture alpha.
  constexpr std::uint64_t kOracleHash = 0x16e2201b96f48c65ULL;
  const auto actual = make_room_fixture_alpha_hash();
  expect_hash_eq(actual, kOracleHash, "Gate D alpha room hash");
}

void test_gate_d_room_fixture_beta_hash_matches_oracle() {
  // Oracle capture hash for the deterministic room fixture beta.
  constexpr std::uint64_t kOracleHash = 0x22ecbb9529f6165ULL;
  const auto actual = make_room_fixture_beta_hash();
  expect_hash_eq(actual, kOracleHash, "Gate D beta room hash");
}

void test_gate_e_hazard_death_routes_to_special_state() {
  comic2::GameDispatcher dispatcher;
  comic2::install_default_stage_hooks(dispatcher);

  comic2::RuntimeState state;
  state.player.hp = 1;
  state.flags.tile_hazard_triggered = true;

  const auto hazard = dispatcher.run_tick(state);
  expect(hazard.stage == comic2::DispatchStage::TileHazard,
         "Gate E should route to tile hazard stage when hazard flag is set");
  expect(state.player.hp == 0, "Gate E hazard should reduce hp to zero");
  expect(state.flags.player_special_state_active,
         "Gate E death should set special state flag");

  const auto special = dispatcher.run_tick(state);
  expect(special.stage == comic2::DispatchStage::PlayerSpecialState,
         "Gate E death path should transition into player special stage");
}

void test_gate_e_room_transition_boundary_sequence() {
  comic2::GameDispatcher dispatcher;
  comic2::install_default_stage_hooks(dispatcher);

  comic2::RuntimeState state;
  state.current_room = 3;
  state.player.x = 319;
  state.input.right_pressed = true;

  const auto input_tick = dispatcher.run_tick(state);
  expect(input_tick.stage == comic2::DispatchStage::InputHandling,
         "Gate E boundary setup tick should execute input stage");
  expect(state.flags.level_transition_pending,
         "Gate E boundary crossing should mark level transition pending");
  expect(state.pending_room_transition.has_value(),
         "Gate E boundary crossing should stage pending transition metadata");
  expect(state.pending_room_transition->target_room == 4,
         "Gate E boundary crossing should target next room");
  expect(state.pending_room_transition->target_player_x == 0,
         "Gate E right boundary crossing should target left-edge player x");
  expect(state.current_room == 3, "Gate E boundary crossing should defer room "
                                  "commit until transition load");

  state.input = comic2::InputState{};
  const auto transition_tick = dispatcher.run_tick(state);
  expect(transition_tick.stage == comic2::DispatchStage::LevelTransition,
         "Gate E transition tick should execute level transition stage");
  expect(!state.flags.level_transition_pending,
         "Gate E level transition hook should clear pending flag");
  expect(!state.pending_room_transition.has_value(),
         "Gate E level transition hook should consume pending metadata");
}

void test_gate_e_left_boundary_clamps_at_room_zero() {
  comic2::GameDispatcher dispatcher;
  comic2::install_default_stage_hooks(dispatcher);

  comic2::RuntimeState state;
  state.current_room = 0;
  state.player.x = 0;
  state.input.left_pressed = true;

  const auto tick = dispatcher.run_tick(state);
  expect(tick.stage == comic2::DispatchStage::InputHandling,
         "Gate E left boundary clamp should run via input stage");
  expect(state.player.x == 0,
         "Gate E left boundary at room zero should clamp player x to zero");
  expect(state.current_room == 0,
         "Gate E left boundary at room zero should not change room index");
  expect(!state.flags.level_transition_pending,
         "Gate E left boundary at room zero should not set transition pending");
}

void test_gate_phase_91_combat_and_pickup_flow() {
  comic2::GameDispatcher dispatcher;
  comic2::install_default_stage_hooks(dispatcher);

  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.player.x = 50;
  state.player.y = 50;
  state.player.hp = 3;
  state.player.is_physics_active = false;
  state.player.is_airborne = false;
  state.runtime_slots.resize(2);

  state.runtime_slots[0].mapped_object_ptr = 1;
  state.runtime_slots[0].behavior_state = 0x0000;
  state.runtime_slots[0].x = 48;
  state.runtime_slots[0].y = 48;
  state.runtime_slots[0].hitbox_w = 12;
  state.runtime_slots[0].hitbox_h = 12;

  state.runtime_slots[1].mapped_object_ptr = 2;
  state.runtime_slots[1].behavior_state = 0x0004;
  state.runtime_slots[1].x = 60;
  state.runtime_slots[1].y = 50;
  state.runtime_slots[1].hitbox_w = 8;
  state.runtime_slots[1].hitbox_h = 8;

  dispatcher.run_tick(state);

  expect(state.player.hp == 2,
         "Gate 9.1 combat should reduce player hp on entity collision");
  expect(state.player.gems == 1,
         "Gate 9.1 pickups should increment the gem counter");
  expect(state.runtime_slots[1].mapped_object_ptr == 0,
         "Gate 9.1 pickups should deactivate collected runtime slots");
}

void test_gate_e_projectile_collision_outcome() {
  std::vector<comic2::ProjectileState> projectiles;
  comic2::spawn_projectile(projectiles, 0, 0, 0, 0);

  comic2::RoomTileGrid grid = make_single_tile_grid(0x02);

  comic2::update_projectiles(projectiles,
                             comic2::ProjectileBounds{
                                 .min_x = 0,
                                 .max_x = 319,
                                 .min_y = 0,
                                 .max_y = 199,
                             },
                             grid, 0, 0, 200, 152);

  expect(projectiles.size() == 1,
         "Gate E projectile interaction should keep projectile storage stable");
  expect(!projectiles[0].active,
         "Gate E projectile should deactivate on tile collision interaction");
}

void test_gate_phase_93_transition_flow_freezes_player_and_tracks_camera() {
  comic2::GameDispatcher dispatcher;
  comic2::install_default_stage_hooks(dispatcher);

  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.room_grid.tile_w = 20;
  state.room_grid.tile_h = 20;
  state.room_grid.row_pointers.assign(20, 0);
  state.room_grid.tile_data.assign(400, 0x01);
  state.pending_room_transition = comic2::PendingRoomTransition{
      .target_room = 1,
      .target_player_x = 0,
      .target_level = std::nullopt,
      .target_player_y = std::nullopt,
  };
  state.flags.level_transition_pending = true;
  state.player.x = 64;
  state.player.y = 240;
  state.player.is_airborne = false;
  state.input.right_pressed = true;

  const auto transition_tick = dispatcher.run_tick(state);
  expect(
      transition_tick.stage == comic2::DispatchStage::LevelTransition,
      "Gate 9.3 transition flow should begin with the level transition stage");
  expect(state.transition_state.active,
         "Gate 9.3 transition flow should activate a room transition "
         "presentation state");
  expect(state.transition_state.player_frozen,
         "Gate 9.3 transition flow should freeze the player during the "
         "presentation");

  const auto before_x = state.player.x;
  state.input.right_pressed = true;
  const auto followup_tick = dispatcher.run_tick(state);
  expect(followup_tick.stage == comic2::DispatchStage::InputHandling,
         "Gate 9.3 follow-up tick should keep the dispatcher in the input "
         "stage while transition is active");
  expect(
      state.player.x == before_x,
      "Gate 9.3 transition flow should prevent player movement while frozen");
  expect(state.camera_y >= 0, "Gate 9.3 transition flow should update the "
                              "camera Y offset during presentation");
}

void test_gate_phase_10d_event_trigger_flow() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.player.x = 96;
  state.player.y = 100;
  state.mapped_objects.push_back(comic2::MappedObject12{
      .room_x = 0,
      .room_y = 0,
      .descriptor_ptr = 0x1234,
      .state_flags = 0x0001,
      .world_x = 90,
      .world_y = 90,
  });

  comic2::handle_input_fallback(state);

  expect(
      state.ui.pending_event_message == "Room Event Triggered",
      "Gate 10-D should queue the room-event message when a trigger object is "
      "proximity-triggered");
  expect(state.ui.room_event_consumed,
         "Gate 10-D should consume the trigger one time");

  state.ui.pending_event_message.clear();
  state.ui.room_event_consumed = true;
  comic2::handle_input_fallback(state);

  expect(state.ui.pending_event_message.empty(),
         "Gate 10-D should suppress re-triggering while the event is consumed");
}

void test_gate_phase_92_audio_events_trigger_from_gameplay() {
  class RecordingAudioBackend final : public comic2::IAudioBackend {
  public:
    bool initialize() override {
      initialized = true;
      return true;
    }
    void enqueue_event(comic2::AudioEvent event) override {
      events.emplace_back(event);
    }
    void update() override {}
    void shutdown() override { initialized = false; }
    bool is_available() const override { return initialized; }
    std::size_t events_enqueued() const override { return events.size(); }

    bool initialized = false;
    std::vector<comic2::AudioEvent> events;
  } audio;

  comic2::RuntimeState state = comic2::make_default_runtime_state();
  comic2::GameDispatcher dispatcher = comic2::make_default_game_dispatcher();
  comic2::MemoryFramePresenter presenter;

  state.player.jump_counter = 1;
  state.input.jump_pressed = true;
  state.input.left_pressed = false;
  state.input.right_pressed = false;

  const auto summary = comic2::run_render_loop(
      state, dispatcher, presenter, 1, std::chrono::milliseconds(0), &audio);

  expect(summary.frames_rendered == 1,
         "Gate 9.2 audio integration should process one frame");
  const bool saw_jump = std::any_of(
      audio.events.begin(), audio.events.end(),
      [](const auto event) { return event == comic2::AudioEvent::Jump; });
  expect(saw_jump,
         "Gate 9.2 audio integration should emit a jump sound from gameplay");
}

} // namespace

// Gate 11-E: Frame Compositing - bezel, viewport clipping, and HUD counters.

namespace {

// Read a single EGA pixel color index from a rendered frame surface.
std::uint8_t read_gate11_color(const comic2::EgaPlanarSurface &frame,
                               std::size_t x, std::size_t y) {
  const std::size_t x_byte = x / 8U;
  const std::uint8_t bit = static_cast<std::uint8_t>(7U - (x % 8U));
  std::uint8_t index = 0;
  for (std::size_t plane = 0; plane < comic2::EgaPlanarSurface::kPlaneCount;
       ++plane) {
    const auto value = frame.get_plane_byte(plane, x_byte, y);
    if (value & static_cast<std::uint8_t>(1U << bit)) {
      index = static_cast<std::uint8_t>(index |
                                        static_cast<std::uint8_t>(1U << plane));
    }
  }
  return index;
}

// Set a pixel on a 320x200 Ega4PlaneImage (row_stride = 40 bytes).
void set_gate11_bezel_pixel(comic2::Ega4PlaneImage &image, std::size_t x,
                            std::size_t y, std::uint8_t color_index) {
  const std::size_t x_byte = x / 8U;
  const std::uint8_t bit = static_cast<std::uint8_t>(7U - (x % 8U));
  const std::uint8_t mask = static_cast<std::uint8_t>(1U << bit);
  const std::size_t row_offset = y * 40U;
  for (std::size_t plane = 0; plane < image.planes.size(); ++plane) {
    auto &pd = image.planes[plane];
    if ((color_index >> plane) & 0x1U) {
      pd[row_offset + x_byte] |= mask;
    } else {
      pd[row_offset + x_byte] &= static_cast<std::uint8_t>(~mask);
    }
  }
}

comic2::Ega4PlaneImage
make_gate11_bezel_with_sentinel(std::uint8_t border_color,
                                std::uint8_t fill_color) {
  comic2::Ega4PlaneImage image;
  image.width_bytes = 40U;
  image.height_rows = 200U;
  image.row_span_bytes = static_cast<std::uint16_t>(40U * 200U);
  for (auto &plane : image.planes) {
    plane.assign(8000U, 0x00U);
  }
  // Fill entire surface with fill_color
  for (std::size_t y = 0; y < 200U; ++y) {
    for (std::size_t x = 0; x < 320U; ++x) {
      set_gate11_bezel_pixel(image, x, y, fill_color);
    }
  }
  // Paint a known sentinel in the 8-pixel bezel border (top-left corner)
  for (std::size_t y = 0; y < 8U; ++y) {
    for (std::size_t x = 0; x < 8U; ++x) {
      set_gate11_bezel_pixel(image, x, y, border_color);
    }
  }
  return image;
}

void test_gate_11e_bezel_border_not_overwritten_by_tiles() {
  // Arrange: state with a bezel that has known sentinel pixels in both the
  // top border (y < kPlayfieldY = 8) and bottom border (y >= 184), plus a
  // populated tileset + room grid large enough to exercise the clipping path.
  auto state = comic2::make_default_runtime_state();

  constexpr std::uint8_t kBorderColor = 0x05U; // magenta
  constexpr std::uint8_t kFillColor = 0x02U;   // green
  auto bezel = make_gate11_bezel_with_sentinel(kBorderColor, kFillColor);

  // Also mark the bottom bezel strip (y = 184..199) with the sentinel.
  for (std::size_t y = 184U; y < 200U; ++y) {
    for (std::size_t x = 0U; x < 8U; ++x) {
      set_gate11_bezel_pixel(bezel, x, y, kBorderColor);
    }
  }
  state.bezel_frame = bezel;

  // Room tall enough that camera_y=0 would expose the vertical clipping path:
  // 11 tiles * 16 px = 176 px, filling y=8..184 exactly.
  state.room_grid.tile_w = 1;
  state.room_grid.tile_h = 11;
  state.room_grid.row_pointers.resize(11);
  for (std::size_t i = 0; i < 11U; ++i) {
    state.room_grid.row_pointers[i] = static_cast<std::uint16_t>(i);
  }
  state.room_grid.tile_data.assign(11U, 0U);

  // One all-white tile so any stray blit is visible.
  comic2::Ega4PlaneImage tile;
  tile.width_bytes = 2U;
  tile.height_rows = 16U;
  tile.row_span_bytes = 32U;
  for (auto &plane : tile.planes) {
    plane.assign(32U, 0xFFU);
  }
  state.level_tileset = {tile};

  comic2::MemoryFramePresenter presenter;
  comic2::render_bootstrap_frame(presenter, state);

  expect(presenter.has_frame(), "Gate 11-E: frame must be presented");
  const auto &frame = presenter.last_frame();

  // Top border pixel at (3, 3) must keep sentinel color (y < kPlayfieldY=8).
  const auto top_pixel = read_gate11_color(frame, 3U, 3U);
  expect(top_pixel == kBorderColor,
         "Gate 11-E: top bezel border must not be overwritten by tile draw");

  // Bottom border pixel at (3, 190) must keep sentinel color (y >= 184).
  const auto bottom_pixel = read_gate11_color(frame, 3U, 190U);
  expect(bottom_pixel == kBorderColor,
         "Gate 11-E: bottom bezel border must not be overwritten by tile draw");
}

void test_gate_11e_no_bezel_tiles_reach_full_frame_width() {
  // Without a bezel the tileset renderer must draw across the full 320-px
  // frame (not stop at kPlayfieldWidth = 240). Use a 20-tile-wide room and
  // an all-white tile; pixels at x >= 240 (e.g. column 300) must be written.
  auto state = comic2::make_default_runtime_state();
  // No bezel - state.bezel_frame stays std::nullopt.

  state.room_grid.tile_w = 20; // 20 * 16 = 320 px wide
  state.room_grid.tile_h = 1;
  state.room_grid.row_pointers = {0};
  state.room_grid.tile_data.assign(20U, 0U);

  comic2::Ega4PlaneImage tile;
  tile.width_bytes = 2U;
  tile.height_rows = 16U;
  tile.row_span_bytes = 32U;
  for (auto &plane : tile.planes) {
    plane.assign(32U, 0xFFU); // all-white (color 0x0F)
  }
  state.level_tileset = {tile};

  comic2::MemoryFramePresenter presenter;
  comic2::render_bootstrap_frame(presenter, state);

  expect(presenter.has_frame(), "Gate 11-E no-bezel: frame must be presented");
  const auto &frame = presenter.last_frame();

  // Column 300 (tile 18, right portion) must be the tile color 0x0F.
  const auto right_pixel = read_gate11_color(frame, 300U, 0U);
  expect(right_pixel == 0x0FU,
         "Gate 11-E no-bezel: tiles must reach full frame width (x=300) when "
         "no bezel is active");
}

void test_gate_11e_no_bezel_tiles_reach_full_frame_height() {
  // Without a bezel the tileset renderer must not stop at kPlayfieldHeight=176.
  // Use a 12-tile-tall room (192 px); tile 11 covers y=176..191. With the
  // old broken code (vp_bottom = y_offset + kPlayfieldHeight = 176) that tile
  // would have been skipped; with the fix (vp_bottom = frame height = 200) it
  // is drawn. Check row 184, which sits inside tile 11.
  auto state = comic2::make_default_runtime_state();

  state.room_grid.tile_w = 1;
  state.room_grid.tile_h = 12; // 12 * 16 = 192 px; tile 11 = rows 176..191
  state.room_grid.row_pointers.resize(12);
  for (std::size_t i = 0; i < 12U; ++i) {
    state.room_grid.row_pointers[i] = static_cast<std::uint16_t>(i);
  }
  state.room_grid.tile_data.assign(12U, 0U);

  comic2::Ega4PlaneImage tile;
  tile.width_bytes = 2U;
  tile.height_rows = 16U;
  tile.row_span_bytes = 32U;
  for (auto &plane : tile.planes) {
    plane.assign(32U, 0xFFU); // all-white (color 0x0F)
  }
  state.level_tileset = {tile};

  comic2::MemoryFramePresenter presenter;
  comic2::render_bootstrap_frame(presenter, state);

  expect(presenter.has_frame(),
         "Gate 11-E no-bezel height: frame must be presented");
  const auto &frame = presenter.last_frame();

  // Row 184 is inside tile 11 (rows 176..191), which fits within the 200-row
  // frame. The old broken path would have stopped at vp_bottom=176 and never
  // drawn this tile.
  const auto bottom_pixel = read_gate11_color(frame, 0U, 184U);
  expect(bottom_pixel == 0x0FU,
         "Gate 11-E no-bezel: tiles must reach full frame height (y=184) when "
         "no bezel is active");
}

void test_gate_11e_hud_counters_appear_in_bezel_panel_region() {
  // Arrange: state with a bezel and non-trivial player stats so
  // hud_render_overlay actually paints something in the right-panel region
  // (x=258..286, y=90..175).
  auto state = comic2::make_default_runtime_state();
  state.bezel_frame = make_gate11_bezel_with_sentinel(0x05U, 0x00U);
  state.player.score = 1000U; // non-zero -> overlay guard passes
  state.player.gems = 3U;
  state.player.lives = 2U;
  state.player.firepower = 2U;
  state.player.shield_power = 4U;

  comic2::MemoryFramePresenter presenter;
  comic2::render_bootstrap_frame(presenter, state);

  expect(presenter.has_frame(), "Gate 11-E HUD: frame must be presented");
  const auto &frame = presenter.last_frame();

  // Scan the HUD panel area (x=258..286, y=90..175) for at least one
  // non-zero pixel written by the HUD renderer.
  bool found_hud_pixel = false;
  for (std::size_t y = 90U; y < 176U && !found_hud_pixel; ++y) {
    for (std::size_t x = 258U; x < 288U && !found_hud_pixel; ++x) {
      if (read_gate11_color(frame, x, y) != 0x00U) {
        found_hud_pixel = true;
      }
    }
  }
  expect(found_hud_pixel,
         "Gate 11-E HUD: at least one HUD pixel must appear in the bezel "
         "right-panel region");
}

void test_gate_11e_font_glyph_digit_is_nonzero() {
  // Verify that get_font_glyph_8x8 returns non-trivial data for digits 0-9.
  for (char d = '0'; d <= '9'; ++d) {
    const auto glyph = comic2::get_font_glyph_8x8(d);
    const bool any_set = std::any_of(glyph.begin(), glyph.end(),
                                     [](std::uint8_t b) { return b != 0U; });
    expect(any_set,
           "Gate 11-E font: digit glyph must have at least one set bit");
  }
}

void test_gate_11e_blit_string_writes_pixels_on_surface() {
  // blit_string of "01" should produce visible pixels on a blank surface.
  comic2::EgaPlanarSurface surface(320, 200);
  surface.clear(0x00);
  comic2::blit_string(surface, 10, 20, "01", 0x0FU);

  bool found = false;
  for (std::int32_t y = 20; y < 28 && !found; ++y) {
    for (std::int32_t x = 10; x < 26 && !found; ++x) {
      const auto x_byte = static_cast<std::size_t>(x) / 8U;
      const auto bit =
          static_cast<std::uint8_t>(7U - (static_cast<std::size_t>(x) % 8U));
      for (std::size_t plane = 0; plane < comic2::EgaPlanarSurface::kPlaneCount;
           ++plane) {
        if (surface.get_plane_byte(plane, x_byte, static_cast<std::size_t>(y)) &
            static_cast<std::uint8_t>(1U << bit)) {
          found = true;
          break;
        }
      }
    }
  }
  expect(found,
         "Gate 11-E: blit_string must write visible pixels on the surface");
}

std::vector<std::uint8_t> make_gate11f_room_resource_bytes() {
  constexpr std::uint16_t kRoomCount = 2U;
  constexpr std::uint16_t kLevelId = 0U;
  constexpr std::uint16_t kRoom0Width = 20U;
  constexpr std::uint16_t kRoom0Height = 10U;
  constexpr std::uint16_t kRoom1Width = 15U;
  constexpr std::uint16_t kRoom1Height = 8U;

  auto make_room_payload = [](std::uint16_t tile_w, std::uint16_t tile_h,
                              std::uint16_t object_count) {
    std::vector<std::uint8_t> decoded(0x2D2U, 0x00U);
    const std::size_t tile_count =
        static_cast<std::size_t>(tile_w) * static_cast<std::size_t>(tile_h);
    std::fill(decoded.begin(),
              decoded.begin() + static_cast<std::ptrdiff_t>(tile_count), 0x01U);

    for (std::uint16_t y = 0; y < tile_h; ++y) {
      const std::uint16_t row_offset = static_cast<std::uint16_t>(y * tile_w);
      const std::size_t entry = static_cast<std::size_t>(0x2A0U + (y * 2U));
      decoded[entry] = static_cast<std::uint8_t>(row_offset & 0xFFU);
      decoded[entry + 1U] =
          static_cast<std::uint8_t>((row_offset >> 8) & 0xFFU);
    }

    if (object_count > 0U) {
      decoded[0x2B0U] = static_cast<std::uint8_t>(object_count & 0xFFU);
      decoded[0x2B1U] = static_cast<std::uint8_t>((object_count >> 8) & 0xFFU);
      const std::size_t obj_off = 0x2B2U;
      decoded[obj_off + 0U] = 0x00U;
      decoded[obj_off + 1U] = 0x00U;
      decoded[obj_off + 2U] = 0x00U;
      decoded[obj_off + 3U] = 0x00U;
      decoded[obj_off + 4U] = 0x76U;
      decoded[obj_off + 5U] = 0x89U;
      decoded[obj_off + 6U] = 0x01U;
      decoded[obj_off + 7U] = 0x00U;
      decoded[obj_off + 8U] = 0x00U;
      decoded[obj_off + 9U] = 0x00U;
      decoded[obj_off + 10U] = 0x00U;
      decoded[obj_off + 11U] = 0x00U;
    }

    return decoded;
  };

  const auto room0_decoded = make_room_payload(kRoom0Width, kRoom0Height, 1U);
  const auto room1_decoded = make_room_payload(kRoom1Width, kRoom1Height, 1U);
  const auto room0_rle = encode_literal_signed_rle(room0_decoded);
  const auto room1_rle = encode_literal_signed_rle(room1_decoded);

  std::vector<std::uint8_t> bytes(0x20U + room0_rle.size() + room1_rle.size(),
                                  0x00U);
  bytes[0U] = static_cast<std::uint8_t>(kRoomCount & 0xFFU);
  bytes[1U] = static_cast<std::uint8_t>((kRoomCount >> 8) & 0xFFU);
  bytes[2U] = static_cast<std::uint8_t>(kLevelId & 0xFFU);
  bytes[3U] = static_cast<std::uint8_t>((kLevelId >> 8) & 0xFFU);

  bytes[4U] = static_cast<std::uint8_t>(kRoom0Width & 0xFFU);
  bytes[5U] = static_cast<std::uint8_t>((kRoom0Width >> 8) & 0xFFU);
  bytes[6U] = static_cast<std::uint8_t>(kRoom0Height & 0xFFU);
  bytes[7U] = static_cast<std::uint8_t>((kRoom0Height >> 8) & 0xFFU);
  bytes[8U] = 0x20U;
  bytes[9U] = 0x00U;

  bytes[10U] = static_cast<std::uint8_t>(kRoom1Width & 0xFFU);
  bytes[11U] = static_cast<std::uint8_t>((kRoom1Width >> 8) & 0xFFU);
  bytes[12U] = static_cast<std::uint8_t>(kRoom1Height & 0xFFU);
  bytes[13U] = static_cast<std::uint8_t>((kRoom1Height >> 8) & 0xFFU);
  const auto room1_off = static_cast<std::uint16_t>(0x20U + room0_rle.size());
  bytes[14U] = static_cast<std::uint8_t>(room1_off & 0xFFU);
  bytes[15U] = static_cast<std::uint8_t>((room1_off >> 8) & 0xFFU);

  std::copy(room0_rle.begin(), room0_rle.end(), bytes.begin() + 0x20);
  std::copy(room1_rle.begin(), room1_rle.end(), bytes.begin() + room1_off);
  return bytes;
}

std::filesystem::path find_reference_original_assets_root() {
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
      const auto candidate = cursor / "reference" / "original";
      if (std::filesystem::exists(candidate) &&
          std::filesystem::is_directory(candidate)) {
        for (const auto &entry :
             std::filesystem::directory_iterator(candidate)) {
          const auto name = entry.path().filename().string();
          if (name.rfind("FR000.", 0) == 0 && entry.is_regular_file()) {
            return candidate;
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

  return {};
}

void test_gate_11f_scripted_replay_is_deterministic_across_room_transition() {
  const auto room_resource_bytes = make_gate11f_room_resource_bytes();
  const std::vector<comic2::InputState> inputs(
      9U, comic2::InputState{.right_pressed = true});

  auto make_initial_state = [&]() {
    auto state = comic2::make_default_runtime_state();
    state.current_level = 0U;
    state.current_room = 0U;
    state.room_grid.tile_w = 20U;
    state.room_grid.tile_h = 10U;
    state.room_grid.row_pointers.assign(10U, 0U);
    state.room_grid.tile_data.assign(200U, 0x01U);
    state.player.x = 200;
    state.player.y = 128;
    state.player.is_physics_active = true;
    state.player.is_airborne = false;
    state.player.facing_right = true;
    state.room_resource_bytes = room_resource_bytes;
    return state;
  };

  auto state_a = make_initial_state();
  auto state_b = make_initial_state();

  auto dispatcher_a = comic2::make_default_game_dispatcher();
  auto dispatcher_b = comic2::make_default_game_dispatcher();

  const auto run_a =
      comic2::capture_runtime_snapshots(state_a, dispatcher_a, inputs);
  const auto run_b =
      comic2::capture_runtime_snapshots(state_b, dispatcher_b, inputs);

  const auto result = comic2::compare_snapshot_sequences(
      run_a, run_b,
      comic2::ReplayComparisonOptions{.position_tolerance_px = 0,
                                      .timing_tolerance_ticks = 0});
  expect(result.matches, "Gate 11-F: replay snapshots must match exactly "
                         "across the same room-transition script");
}

void test_gate_11f_room_transition_reloads_geometry_and_entity_slots() {
  const auto room_resource_bytes = make_gate11f_room_resource_bytes();
  auto state = comic2::make_default_runtime_state();
  state.current_level = 0U;
  state.current_room = 0U;
  state.player.x = 319;
  state.player.y = 128;
  state.player.is_physics_active = true;
  state.player.is_airborne = false;
  state.room_grid.tile_w = 20U;
  state.room_grid.tile_h = 10U;
  state.room_grid.row_pointers.assign(10U, 0U);
  state.room_grid.tile_data.assign(200U, 0x01U);
  state.room_resource_bytes = room_resource_bytes;
  state.runtime_slots.assign(6U, comic2::RuntimeEntitySlot32{});
  state.active_entities.assign(2U, comic2::ActiveEntity8{});
  state.flags.level_transition_pending = true;
  state.pending_room_transition =
      comic2::PendingRoomTransition{.target_room = 1U,
                                    .target_player_x = 0,
                                    .target_level = std::nullopt,
                                    .target_player_y = std::nullopt};

  auto dispatcher = comic2::make_default_game_dispatcher();
  const auto tick = dispatcher.run_tick(state);
  expect(tick.stage == comic2::DispatchStage::LevelTransition,
         "Gate 11-F: room transition should execute the transition stage");
  expect(state.current_room == 1U,
         "Gate 11-F: room transition should commit to the target room");
  expect(state.room_grid.tile_w == 15U,
         "Gate 11-F: room transition should reload the next room tile width");
  expect(state.room_grid.tile_h == 8U,
         "Gate 11-F: room transition should reload the next room tile height");
  expect(state.transition_state.active,
         "Gate 11-F: room transition should activate the transition state");
  expect(
      state.runtime_slots.size() <= 6U,
      "Gate 11-F: runtime entity slots should be refreshed during room reload");
}

void test_gate_11f_level0_room0_initializes_from_real_assets_if_present() {
  const auto original_root = find_reference_original_assets_root();
  if (original_root.empty()) {
    std::cout << "[skip] Gate 11-F: reference/original assets absent; live "
                 "bootstrap checks disabled\n";
    return;
  }

  const auto repo_root = original_root.parent_path().parent_path();
  auto state = comic2::make_default_runtime_state();
  const auto summary = comic2::initialize_runtime_scene(state, repo_root);

  expect(summary.room_grid_loaded,
         "Gate 11-F: Level 0 Room 0 should initialize from original assets "
         "when present");
  expect(
      state.room_grid.tile_w > 0U && state.room_grid.tile_h > 0U,
      "Gate 11-F: real-asset room bootstrap should load valid room dimensions");
  expect(
      state.player.x >= 0 && state.player.x <= 319,
      "Gate 11-F: player x should remain in room bounds after live bootstrap");
  expect(
      state.current_level == 0U,
      "Gate 11-F: live bootstrap should initialize the expected Level 0 state");
}

void test_gate_12a_frequency_parity_for_pit_1193_divisor() {
  const auto samples = comic2::synthesize_pit_samples(1193U, 4U, 44100);
  expect(!samples.empty(),
         "Gate 12-A: PIT divisor 1193 should synthesize a non-empty waveform");
  expect(samples.size() >= 44U,
         "Gate 12-A: waveform should contain enough samples for a full cycle");

  for (int i = 0; i < 22; ++i) {
    expect(samples[static_cast<std::size_t>(i)] > 0,
           "Gate 12-A: first half-cycle for 1000 Hz tone should be positive");
  }
  for (int i = 22; i < 44; ++i) {
    expect(samples[static_cast<std::size_t>(i)] < 0,
           "Gate 12-A: second half-cycle for 1000 Hz tone should be negative");
  }
}

void test_gate_12b_priority_preemption_in_sound_driver() {
  comic2::SoundDriver driver;
  comic2::SoundStream low_priority_stream{{0x0180u, 0x0002u, 0x0000u, 0x0000u},
                                          2U};
  driver.play(low_priority_stream, 2U);
  expect(driver.is_playing(),
         "Gate 12-B: starting a sound should activate the sound driver");
  expect(driver.current_priority() == 2U,
         "Gate 12-B: current priority should match the active sound");

  comic2::SoundStream rejected_stream{{0x0200u, 0x0003u, 0x0000u, 0x0000u}, 1U};
  driver.play(rejected_stream, 1U);
  expect(driver.is_playing(),
         "Gate 12-B: lower-priority sounds should be rejected");
  expect(driver.current_priority() == 2U,
         "Gate 12-B: lower-priority sound should not override the active one");

  comic2::SoundStream preempt_stream{{0x0400u, 0x0002u, 0x0000u, 0x0000u}, 3U};
  driver.play(preempt_stream, 3U);
  expect(driver.is_playing(),
         "Gate 12-B: higher-priority sounds should preempt the active one");
  expect(driver.current_priority() == 3U,
         "Gate 12-B: active priority should update when preempted");
  expect(driver.current_divisor() != 0U,
         "Gate 12-B: preempting sound should drive a non-zero PIT divisor");
}

void test_gate_12b2_sound_driver_tick_and_synthesizer() {
  comic2::SoundDriver driver;
  expect(driver.tick() == 0U,
         "Gate 12-B2: idle sound driver tick should yield a zero divisor");

  comic2::SoundStream stream{{0x0180u, 0x0002u, 0x0000u, 0x0000u}, 2U};
  driver.play(stream, 2U);
  expect(driver.tick() == 0x0180u,
         "Gate 12-B2: tick should report the active PIT divisor");

  comic2::AudioSynthesizer synthesizer(44100);
  const auto from_stream = synthesizer.synthesize(stream, 128);
  expect(
      from_stream.size() == 128U,
      "Gate 12-B2: stream synthesis should honor the requested sample count");
  const auto from_divisor = synthesizer.synthesize(1193U, 4U);
  expect(!from_divisor.empty(),
         "Gate 12-B2: divisor synthesis should produce samples");
}

void test_gate_12c_null_audio_backend_stays_headless() {
  comic2::NullAudioBackend backend;
  expect(backend.initialize(), "Gate 12-C: null backend should initialize "
                               "successfully in headless mode");
  expect(!backend.is_available(),
         "Gate 12-C: null backend must remain unavailable by design");

  for (int i = 0; i < 1000; ++i) {
    backend.enqueue_event(comic2::AudioEvent::Pickup);
    backend.update();
  }
  backend.shutdown();

  expect(backend.events_enqueued() == 1000U,
         "Gate 12-C: null backend should count each queued event across 1000 "
         "ticks");
  expect(!backend.is_available(),
         "Gate 12-C: null backend should stay unavailable after shutdown");
}

void test_gate_13b_entity_movement_parity() {
  comic2::RoomTileGrid grid;
  grid.tile_w = 16;
  grid.tile_h = 8;
  for (int r = 0; r < 8; ++r) {
    grid.row_pointers.push_back(static_cast<std::uint16_t>(r * 16));
  }
  grid.tile_data.assign(16 * 8, 0xFF);
  for (int x = 0; x < 16; ++x) {
    grid.tile_data[5 * 16 + x] = 0x01; // solid floor on row 5 (y = 80..95)
  }
  grid.tile_data[3 * 16 + 8] =
      0x01; // solid wall block at col 8, row 3 (x = 128..143, y = 48..63)

  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.room_grid = grid;
  state.tile_hazard_bounds = {1, 1};
  state.camera_x = 0;
  state.camera_y = 0;
  state.player.x = 200;
  state.player.y = 48;
  state.player.is_airborne = false;
  state.player.is_physics_active = true;

  // 1. Slime Hopper Parabolic Arc & Floor Landing Snap:
  // Entity resting on floor (row 5 starts at y = 80, so foot = 64 + 16 = 80).
  state.runtime_slots.resize(1);
  state.runtime_slots[0] = comic2::RuntimeEntitySlot32{
      .hitbox_w = 16,
      .hitbox_h = 16,
      .type_flags = 2,
      .behavior_state = comic2::kBehaviorJump,
      .anim_span = 0,
      .anim_period = 0,
      .unknown_0c = 0,
      .unknown_0e = 0,
      .x = 32,
      .y = 64,
      .param_a = 2,
      .param_b = 0,
      .dir_toggle = 1,
      .anim_tick = 0,
      .unknown_1c = 0,
      .mapped_object_ptr = 1,
  };

  // Initial contact on ground triggers jump impulse.
  comic2::update_entity_behaviors(state);
  expect(state.runtime_slots[0].y == 64 && state.runtime_slots[0].param_b == -6,
         "Gate 13-B: Slime ground collision triggers jump impulse");

  const struct ExpectedStep {
    std::int16_t x;
    std::int16_t y;
    std::int16_t param_b;
  } expected_trajectory[] = {
      {36, 59, -5}, {38, 55, -4}, {40, 52, -3}, {42, 50, -2},
      {44, 49, -1}, {46, 49, 0},  {48, 50, 1},  {50, 52, 2},
      {52, 55, 3},  {54, 59, 4},  {56, 64, -6},
  };

  for (std::size_t i = 0; i < 11; ++i) {
    comic2::update_entity_behaviors(state);
    const auto &slot = state.runtime_slots[0];
    expect(slot.x == expected_trajectory[i].x,
           "Gate 13-B: Slime hop x coordinate mismatch");
    expect(slot.y == expected_trajectory[i].y,
           "Gate 13-B: Slime hop y coordinate mismatch");
    expect(slot.param_b == expected_trajectory[i].param_b,
           "Gate 13-B: Slime hop vertical velocity mismatch");
  }

  // 2. Wall Collision and Direction Reflection:
  // Entity approaching wall block at x = 128..143 (row 3, col 8).
  state.runtime_slots[0] = comic2::RuntimeEntitySlot32{
      .hitbox_w = 16,
      .hitbox_h = 16,
      .type_flags = 1,
      .behavior_state = comic2::kBehaviorChase,
      .anim_span = 0,
      .anim_period = 0,
      .unknown_0c = 0,
      .unknown_0e = 0,
      .x = 108,
      .y = 48,
      .param_a = 4,
      .param_b = 0,
      .dir_toggle = 1, // moving right toward wall
      .anim_tick = 0,
      .unknown_1c = 0,
      .mapped_object_ptr = 1,
  };

  bool wall_bounced = false;
  for (int step = 0; step < 10; ++step) {
    comic2::update_entity_behaviors(state);
    if (state.runtime_slots[0].dir_toggle == 2) {
      wall_bounced = true;
      break;
    }
  }
  expect(wall_bounced, "Gate 13-B: entity should bounce on wall collision");
  expect(state.runtime_slots[0].dir_toggle == 2,
         "Gate 13-B: wall bounce should invert dir_toggle to left");
  expect(state.runtime_slots[0].param_a < 0,
         "Gate 13-B: wall bounce should negate horizontal velocity");

  const auto x_before_retreat = state.runtime_slots[0].x;
  comic2::update_entity_behaviors(state);
  expect(state.runtime_slots[0].x < x_before_retreat,
         "Gate 13-B: entity should retreat leftward away from wall");

  // 3. Flyer / Bat Vertical Oscillation & Viewport/Tile Boundary Reflection:
  state.runtime_slots[0] = comic2::RuntimeEntitySlot32{
      .hitbox_w = 16,
      .hitbox_h = 16,
      .type_flags = 4,
      .behavior_state = comic2::kBehaviorBounce,
      .anim_span = 0,
      .anim_period = 0,
      .unknown_0c = 0,
      .unknown_0e = 0,
      .x = 60,
      .y = 48,
      .param_a = 0,
      .param_b = 4, // descending
      .dir_toggle = 1,
      .anim_tick = 0,
      .unknown_1c = 0,
      .mapped_object_ptr = 1,
  };

  comic2::update_entity_behaviors(state);
  expect(state.runtime_slots[0].x == 60 && state.runtime_slots[0].y == 52,
         "Gate 13-B: flyer descends at step 1");
  comic2::update_entity_behaviors(state);
  expect(state.runtime_slots[0].x == 60 && state.runtime_slots[0].y == 56,
         "Gate 13-B: flyer descends at step 2");
  comic2::update_entity_behaviors(state);
  expect(state.runtime_slots[0].x == 60 && state.runtime_slots[0].y == 60,
         "Gate 13-B: flyer descends at step 3");
  comic2::update_entity_behaviors(state);
  expect(state.runtime_slots[0].x == 60 && state.runtime_slots[0].y == 64 &&
             state.runtime_slots[0].param_b == -4,
         "Gate 13-B: flyer reflects on floor collision and negates vy to -4");
  comic2::update_entity_behaviors(state);
  expect(state.runtime_slots[0].x == 60 && state.runtime_slots[0].y == 60 &&
             state.runtime_slots[0].param_b == -4,
         "Gate 13-B: flyer ascends upward after floor reflection");

  // 4. Dispatcher Pipeline Stage Integration:
  comic2::GameDispatcher dispatcher;
  comic2::install_default_stage_hooks(dispatcher);
  state.player.x = 200;
  state.player.y = 48;
  state.player.is_physics_active = true;
  state.player.is_airborne = false;
  const auto tick = dispatcher.run_tick(state);
  expect(tick.stage == comic2::DispatchStage::GroundedPhysics,
         "Gate 13-B: dispatcher should run grounded physics stage");
  expect(state.runtime_slots[0].y == 56,
         "Gate 13-B: dispatcher grounded physics stage should tick entity "
         "behaviors");
}

void test_gate_13c_trigger_activation() {
  // 1. Transition zone contact triggers room transition with exact target
  // metadata
  comic2::GameDispatcher dispatcher;
  comic2::install_default_stage_hooks(dispatcher);

  comic2::RuntimeState state;
  state.current_level = 0;
  state.current_room = 0;
  state.level_data = comic2::get_level_runtime_data(0);
  state.room_resource_bytes = make_gate11f_room_resource_bytes();
  expect(state.level_data != nullptr,
         "Gate 13-C: Level 0 data must be available");

  // In Level 0 Room 0, transition zone 0 is at (x=0, y_min=0, y_max=16),
  // leading to Room 1 at spawn zone 0 (x=1008, y_max=16).
  state.player.x = 0;
  state.player.y = 10;
  state.player.is_physics_active = true;
  state.player.is_airborne = false;
  state.input.left_pressed = true;

  // Run grounded physics tick (which updates room transitions from player
  // bounds)
  const auto tick = dispatcher.run_tick(state);
  expect(tick.stage == comic2::DispatchStage::GroundedPhysics,
         "Gate 13-C: initial tick should run grounded physics");
  expect(state.flags.level_transition_pending,
         "Gate 13-C: entering transition zone should stage pending level "
         "transition");
  expect(state.pending_room_transition.has_value(),
         "Gate 13-C: pending room transition metadata must be staged");
  expect(state.pending_room_transition->target_room == 1,
         "Gate 13-C: transition zone must target room 1");
  expect(state.pending_room_transition->target_level.has_value() &&
             *state.pending_room_transition->target_level == 0,
         "Gate 13-C: transition zone must target level 0");
  expect(
      state.pending_room_transition->target_player_x == 1008,
      "Gate 13-C: arrival player x must match target room spawn zone (1008)");
  expect(state.pending_room_transition->target_player_y.has_value() &&
             *state.pending_room_transition->target_player_y == 16,
         "Gate 13-C: arrival player y must match target room spawn zone (16)");

  // Run transition tick: LevelTransition stage consumes pending transition
  state.input.left_pressed = false;
  const auto trans_tick = dispatcher.run_tick(state);
  expect(trans_tick.stage == comic2::DispatchStage::LevelTransition,
         "Gate 13-C: transition tick should route to LevelTransition stage");
  expect(!state.flags.level_transition_pending,
         "Gate 13-C: level transition hook should clear pending flag");
  expect(state.current_room == 1,
         "Gate 13-C: level transition should commit target room 1");
  expect(state.player.x == 1008,
         "Gate 13-C: player x should update to target spawn x (1008)");
  expect(state.player.y == 16,
         "Gate 13-C: player y should update to target spawn y (16)");
  expect(state.transition_state.active,
         "Gate 13-C: transition state must become active");

  // 2. Tile trigger range evaluation parity
  const auto *lvl0 = state.level_data;
  // Level 0 touch triggers: range [26..28] -> [57, 80, 139]
  const auto touch26 = comic2::evaluate_tile_trigger(lvl0->touch_trigger, 26);
  expect(touch26.has_value() && *touch26 == 57,
         "Gate 13-C: touch trigger for tile 26 must evaluate to 57");
  const auto touch27 = comic2::evaluate_tile_trigger(lvl0->touch_trigger, 27);
  expect(touch27.has_value() && *touch27 == 80,
         "Gate 13-C: touch trigger for tile 27 must evaluate to 80");
  const auto touch28 = comic2::evaluate_tile_trigger(lvl0->touch_trigger, 28);
  expect(touch28.has_value() && *touch28 == 139,
         "Gate 13-C: touch trigger for tile 28 must evaluate to 139");
  const auto touch_out = comic2::evaluate_tile_trigger(lvl0->touch_trigger, 25);
  expect(
      !touch_out.has_value(),
      "Gate 13-C: out-of-range tile 25 must return nullopt for touch trigger");

  // Level 0 projectile triggers: range [25..25] -> [139]
  const auto proj25 =
      comic2::evaluate_tile_trigger(lvl0->projectile_trigger, 25);
  expect(proj25.has_value() && *proj25 == 139,
         "Gate 13-C: projectile trigger for tile 25 must evaluate to 139");
  const auto proj_out =
      comic2::evaluate_tile_trigger(lvl0->projectile_trigger, 26);
  expect(!proj_out.has_value(), "Gate 13-C: out-of-range tile 26 must return "
                                "nullopt for projectile trigger");

  // Level 0 mode 2 triggers: range [140..140] -> [24]
  const auto mode140 = comic2::evaluate_tile_trigger(lvl0->mode2_trigger, 140);
  expect(mode140.has_value() && *mode140 == 24,
         "Gate 13-C: mode 2 trigger for tile 140 must evaluate to 24");
}

void test_gate_13a_script_decode_parity() {
  // 1. Fixture test: 64-byte encoded header prefix of FR004.3
  const std::uint8_t kFr004_3HeaderFixture[64] = {
      0x35, 0x25, 0x43, 0x24, 0x9d, 0x27, 0xd7, 0x26, 0xe9, 0x21, 0x5b,
      0x23, 0x15, 0x2d, 0xd7, 0x2d, 0x33, 0x36, 0x61, 0x60, 0x64, 0x77,
      0x05, 0x66, 0x64, 0x75, 0x71, 0x64, 0x6c, 0x6b, 0x09, 0x05, 0x05,
      0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
      0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
      0x05, 0x05, 0x05, 0x71, 0x6d, 0x60, 0x05, 0x67, 0x64};

  const auto fixture_decoded =
      comic2::decode_level_script_payload(kFr004_3HeaderFixture);
  expect(
      fixture_decoded.has_value(),
      "Gate 13-A: decode_level_script_payload must succeed on header fixture");
  expect(fixture_decoded->size() == comic2::kLevelScriptBlockSize,
         "Gate 13-A: decoded fixture must pad to 0x0C00 bytes");

  // Validate all 8 message offsets from decoded fixture table
  const std::uint16_t expected_offsets[8] = {0x0010, 0x0166, 0x02b8, 0x03f2,
                                             0x04cc, 0x067e, 0x0830, 0x08f2};
  for (std::size_t i = 0; i < 8; ++i) {
    const auto off = comic2::level_script_offset(*fixture_decoded, i);
    expect(off.has_value() && *off == expected_offsets[i],
           "Gate 13-A: message offset table mismatch");
  }

  // Offset 0x0010 contains Message 0 header: width=22 (0x16), height=19 (0x13)
  expect((*fixture_decoded)[0x0010] == 22,
         "Gate 13-A: Message 0 width must be 22");
  expect((*fixture_decoded)[0x0011] == 19,
         "Gate 13-A: Message 0 height must be 19");

  // Followed by "DEAR CAPTAIN,"
  const std::string_view msg0_prefix(
      reinterpret_cast<const char *>(fixture_decoded->data() + 0x0012), 13);
  expect(msg0_prefix == "DEAR CAPTAIN,",
         "Gate 13-A: Message 0 text start must be 'DEAR CAPTAIN,'");

  // 2. Real asset verification when reference/original is present
  const auto original_root = find_reference_original_assets_root();
  if (original_root.empty()) {
    return;
  }

  const auto script_path = original_root / "FR004.3";
  if (!std::filesystem::exists(script_path)) {
    return;
  }

  std::ifstream file(script_path, std::ios::binary);
  expect(file.is_open(), "Gate 13-A: failed to open FR004.3");
  std::vector<std::uint8_t> file_bytes((std::istreambuf_iterator<char>(file)),
                                       std::istreambuf_iterator<char>());
  expect(file_bytes.size() == 2437U,
         "Gate 13-A: FR004.3 reference file size must be 2437 bytes");

  const auto full_decoded = comic2::decode_level_script_payload(file_bytes);
  expect(full_decoded.has_value(),
         "Gate 13-A: full FR004.3 payload decode must succeed");
  expect(full_decoded->size() == comic2::kLevelScriptBlockSize,
         "Gate 13-A: full decoded script must equal 0x0C00 block size");

  // Verify byte-level XOR-0x25 inversion for all file bytes
  for (std::size_t i = 0; i < file_bytes.size(); ++i) {
    expect(((*full_decoded)[i] ^ comic2::kLevelScriptXorKey) == file_bytes[i],
           "Gate 13-A: byte-by-byte XOR-0x25 inversion mismatch");
  }

  // Verify unread padded bytes are decoded to zero
  for (std::size_t i = file_bytes.size(); i < full_decoded->size(); ++i) {
    expect((*full_decoded)[i] == 0,
           "Gate 13-A: trailing padded bytes must decode to 0");
  }

  // Verify all 8 decoded message texts from full FR004.3
  const struct ExpectedMessage {
    std::size_t index = 0;
    std::uint16_t offset = 0;
    std::uint8_t width = 0;
    std::uint8_t height = 0;
    std::string_view prefix = {};
  } expected_messages[] = {
      {0, 0x0010, 22, 19, "DEAR CAPTAIN,"},
      {1, 0x0166, 26, 16,
       "GREETINGS CAPTAIN, I AM GLAD YOU MADE IT OUT OF THE TEMPLE"},
      {2, 0x02b8, 26, 15,
       "CAPTAIN, I AM DEEPLY    SORRY, BUT I BELIEVE THESKREJGIB H"},
      {3, 0x03f2, 26, 11, "WE WILL USE THIS BASE   TO DO OUR OWN RESEARCH."},
      {4, 0x04cc, 26, 20, "DEAR CAPTAIN,"},
      {5, 0x067e, 26, 20,
       "THANKS AS WELL FOR YOUR SPACE CRUISER, ITS SALE WILL HELP"},
      {6, 0x0830, 26, 10, "WE'VE INSTALLED THIS OLDTIME DEVICE"},
      {7, 0x08f2, 26, 8,
       "WHO CAN SAY WHAT KIND OFTECHNICAL ADVANCES OUR  RACE WILL"},
  };

  for (const auto &em : expected_messages) {
    const auto off = comic2::level_script_offset(*full_decoded, em.index);
    expect(off.has_value() && *off == em.offset,
           "Gate 13-A: live message offset mismatch");
    expect((*full_decoded)[em.offset] == em.width,
           "Gate 13-A: live message width mismatch");
    expect((*full_decoded)[em.offset + 1] == em.height,
           "Gate 13-A: live message height mismatch");
    const std::string_view actual_prefix(
        reinterpret_cast<const char *>(full_decoded->data() + em.offset + 2),
        em.prefix.size());
    expect(actual_prefix == em.prefix,
           "Gate 13-A: live message prefix mismatch");
  }
}

void test_gate_14a_boots_trajectory_and_firepower() {
  // Gate 14-A: High-jump gravity boots trajectory parity and firepower
  // progression (comic2_data.asm DS:0x08ae, DS:0x08ac, DS:0x08a8).

  // 1. Jump Trajectory Verification (Unequipped vs High-Jump Gravity Boots)
  // Base DOS motion: 5 jump ticks capacity, jump impulse -7, gravity +5.
  // Equipped boots: boosts jump capacity to 6 ticks, jump impulse -7.
  const comic2::PlayerMotionConfig dos_motion =
      comic2::make_original_dos_motion_config();
  const comic2::TileCollisionConfig collision{.solid_tile_threshold = 2};

  // Construct a room grid with solid floor at row 10 (y=160)
  comic2::RoomTileGrid arena_grid;
  arena_grid.tile_w = 20;
  arena_grid.tile_h = 12;
  arena_grid.row_pointers.resize(12);
  arena_grid.tile_data.assign(240, 0x00);
  for (std::size_t r = 0; r < 12; ++r) {
    arena_grid.row_pointers[r] = static_cast<std::uint16_t>(r * 20);
  }
  for (std::size_t c = 0; c < 20; ++c) {
    arena_grid.tile_data[10 * 20 + c] = 0x02; // solid floor tile
  }

  // 1A. Baseline jump (boots unequipped):
  comic2::RuntimeState baseline_state = comic2::make_default_runtime_state();
  baseline_state.room_grid = arena_grid;
  baseline_state.player.x = 64;
  baseline_state.player.y = 128; // resting on floor row 10 (128 + 32 = 160)
  baseline_state.player.boots_equipped = false;
  baseline_state.player.is_airborne = false;
  baseline_state.input.jump_pressed = false;

  // Replenish on ground: jump_counter_max is 5
  comic2::apply_grounded_physics_tick(baseline_state, dos_motion, collision);
  expect(baseline_state.player.jump_counter == 5,
         "Gate 14-A: unequipped jump counter must replenish to 5 ticks");
  expect(baseline_state.player.jump_counter_max == 5,
         "Gate 14-A: unequipped jump_counter_max must equal 5");

  // Initiate jump with jump_pressed held
  baseline_state.input.jump_pressed = true;
  comic2::apply_grounded_physics_tick(baseline_state, dos_motion, collision);
  expect(baseline_state.player.jump_counter == 4,
         "Gate 14-A: baseline jump tick 1 consumes 1 tick (4 remaining)");

  // Advance airborne ticks while jump is held until peak height is reached
  std::int16_t baseline_peak_y = baseline_state.player.y;
  int baseline_rise_ticks = 1;
  while (baseline_state.player.y_vel < 0) {
    comic2::apply_airborne_physics_tick(baseline_state, dos_motion, collision);
    if (baseline_state.player.y < baseline_peak_y) {
      baseline_peak_y = baseline_state.player.y;
      ++baseline_rise_ticks;
    }
  }

  // 1B. Boots equipped jump:
  comic2::RuntimeState boots_state = comic2::make_default_runtime_state();
  boots_state.room_grid = arena_grid;
  boots_state.player.x = 64;
  boots_state.player.y = 128; // resting on floor row 10
  boots_state.player.boots_equipped = true;
  boots_state.player.is_airborne = false;
  boots_state.input.jump_pressed = false;

  // Replenish on ground with boots: jump_counter_max boosts to 6
  comic2::apply_grounded_physics_tick(boots_state, dos_motion, collision);
  expect(boots_state.player.jump_counter == 6,
         "Gate 14-A: boots equipped jump counter must replenish to 6 ticks");
  expect(boots_state.player.jump_counter_max == 6,
         "Gate 14-A: boots equipped jump_counter_max must equal 6");

  // Initiate jump with jump_pressed held
  boots_state.input.jump_pressed = true;
  comic2::apply_grounded_physics_tick(boots_state, dos_motion, collision);
  expect(boots_state.player.jump_counter == 5,
         "Gate 14-A: boots jump tick 1 consumes 1 tick (5 remaining)");

  std::int16_t boots_peak_y = boots_state.player.y;
  int boots_rise_ticks = 1;
  while (boots_state.player.y_vel < 0) {
    comic2::apply_airborne_physics_tick(boots_state, dos_motion, collision);
    if (boots_state.player.y < boots_peak_y) {
      boots_peak_y = boots_state.player.y;
      ++boots_rise_ticks;
    }
  }

  // Continue falling until landing on ground
  boots_state.input.jump_pressed = false;
  while (boots_state.player.is_airborne) {
    comic2::apply_airborne_physics_tick(boots_state, dos_motion, collision);
  }
  expect(!boots_state.player.is_airborne,
         "Gate 14-A: boots jump must land on ground");
  expect(boots_state.player.jump_counter == 6,
         "Gate 14-A: landing on ground with boots must replenish jump counter "
         "to 6");

  // Verify boots trajectory superiority:
  // Boots jump must achieve higher apex (smaller y coordinate in screen space)
  expect(boots_peak_y < baseline_peak_y,
         "Gate 14-A: boots jump must achieve strictly higher peak height than "
         "baseline jump");
  // Boots rise duration must exceed baseline rise duration by exactly 1 tick
  expect(boots_rise_ticks == baseline_rise_ticks + 1,
         "Gate 14-A: boots jump rise duration must exceed baseline rise "
         "duration by 1 tick");

  // 1C. Variable jump height (short hop) with boots equipped:
  comic2::RuntimeState shorthop_state = comic2::make_default_runtime_state();
  shorthop_state.room_grid = arena_grid;
  shorthop_state.player.x = 64;
  shorthop_state.player.y = 128;
  shorthop_state.player.boots_equipped = true;
  shorthop_state.input.jump_pressed = false;
  comic2::apply_grounded_physics_tick(shorthop_state, dos_motion, collision);

  // Jump for 2 ticks then release
  shorthop_state.input.jump_pressed = true;
  comic2::apply_grounded_physics_tick(shorthop_state, dos_motion, collision);
  comic2::apply_airborne_physics_tick(shorthop_state, dos_motion, collision);
  expect(shorthop_state.player.jump_counter == 4,
         "Gate 14-A: shorthop should consume 2 ticks with 4 remaining");

  // Release jump button early
  shorthop_state.input.jump_pressed = false;
  comic2::apply_airborne_physics_tick(shorthop_state, dos_motion, collision);
  expect(shorthop_state.player.jump_counter == 0,
         "Gate 14-A: releasing jump early must immediately truncate jump "
         "counter to 0");

  std::int16_t shorthop_peak_y = shorthop_state.player.y;
  while (shorthop_state.player.y_vel < 0) {
    comic2::apply_airborne_physics_tick(shorthop_state, dos_motion, collision);
    if (shorthop_state.player.y < shorthop_peak_y) {
      shorthop_peak_y = shorthop_state.player.y;
    }
  }
  expect(shorthop_peak_y > boots_peak_y,
         "Gate 14-A: shorthop must reach lower peak height than full boots "
         "jump");

  // 2. Firepower Progression (Levels 1-5) and Rate Limiter Cooldown (DS:0x08a8,
  // DS:0x08ac)
  comic2::RuntimeState fire_state = comic2::make_default_runtime_state();
  fire_state.player.x = 50;
  fire_state.player.y = 50;
  fire_state.player.firepower = 1;
  fire_state.player.weapon_cooldown = 0;
  const comic2::ProjectileBounds wide_bounds{
      .min_x = 0, .max_x = 1000, .min_y = -1000, .max_y = 1000};

  // 2A. Level 1: single projectile limit
  fire_state.input.fire_pressed = true;
  const bool p1_fired = comic2::try_player_fire_projectile(fire_state);
  expect(p1_fired, "Gate 14-A: Level 1 initial projectile fire must succeed");
  expect(fire_state.player.weapon_cooldown ==
             comic2::kDefaultWeaponCooldownTicks,
         "Gate 14-A: firing must engage 20-tick weapon cooldown");
  expect(fire_state.projectiles.size() == 1,
         "Gate 14-A: exactly 1 projectile should be spawned");
  expect(fire_state.projectiles[0].active,
         "Gate 14-A: spawned projectile must be active");

  // 2B. Cooldown blocking: immediate shot is rejected
  expect(!comic2::try_player_fire_projectile(fire_state),
         "Gate 14-A: shot during active cooldown must be rejected");

  // Release fire button and elapse cooldown
  fire_state.input.fire_pressed = false;
  for (std::uint8_t cd = comic2::kDefaultWeaponCooldownTicks; cd > 0; --cd) {
    expect(fire_state.player.weapon_cooldown == cd,
           "Gate 14-A: cooldown must decrement each advance");
    comic2::advance_runtime_projectiles(fire_state, wide_bounds, 0, -1000, 1000,
                                        2000);
  }
  expect(fire_state.player.weapon_cooldown == 0,
         "Gate 14-A: cooldown must expire to 0");

  // Level 1 active projectile concurrency limit: projectile 0 is still active
  expect(!comic2::try_player_fire_projectile(fire_state),
         "Gate 14-A: second shot blocked when active projectiles equal Level 1 "
         "firepower");

  // 2C. Level 2 upgrade allows 2 concurrent projectiles
  fire_state.player.firepower = 2;
  const bool p2_fired = comic2::try_player_fire_projectile(fire_state);
  expect(p2_fired,
         "Gate 14-A: upgrading to Level 2 firepower allows second concurrent "
         "shot");
  expect(fire_state.projectiles.size() == 2,
         "Gate 14-A: 2 projectiles now active in flight");

  // 2D. Level 5 upgrade allows up to 5 concurrent projectiles
  fire_state.player.firepower = 5;
  for (int p = 3; p <= 5; ++p) {
    fire_state.player.weapon_cooldown = 0;
    const bool p_fired = comic2::try_player_fire_projectile(fire_state);
    expect(p_fired,
           "Gate 14-A: Level 5 firepower allows concurrent shots up to 5");
  }
  expect(fire_state.projectiles.size() == 5,
         "Gate 14-A: 5 projectiles simultaneously active in flight");

  // Sixth shot blocked at concurrency cap
  fire_state.player.weapon_cooldown = 0;
  expect(!comic2::try_player_fire_projectile(fire_state),
         "Gate 14-A: sixth shot blocked at Level 5 firepower concurrency cap");
}

void test_gate_14b_cavern_lantern_and_palette_darkness() {
  // Gate 14-B: Cavern lantern illumination and palette darkness mechanics
  // (comic2_data.asm DS:0x08b0, comic2_standalone.asm vid_load_alt_palette
  // 1000:7999, vid_set_ega_palette 1000:010e).

  // 1. Palette Table Darkness Mask Verification
  {
    auto state = comic2::make_default_runtime_state();
    state.is_dark_room = true;
    state.player.lantern_equipped = false;

    comic2::update_room_lighting_palette(state);
    expect(state.palette.loaded,
           "Gate 14-B: palette must be marked loaded after lighting update");
    expect(
        comic2::is_black_palette_mask(state.palette),
        "Gate 14-B: dark room without lantern must apply black palette mask");

    // Verify all 16 palette entries are mapped to black
    for (std::size_t i = 0; i < state.palette.entries.size(); ++i) {
      expect(state.palette.entries[i][0] == 0U &&
                 state.palette.entries[i][1] == 0U &&
                 state.palette.entries[i][2] == 0U,
             "Gate 14-B: palette entry must have 0 RGB in dark room");
    }

    // Equip lantern: palette must swap back to full visible 16-color EGA
    // palette
    state.player.lantern_equipped = true;
    comic2::update_room_lighting_palette(state);
    expect(!comic2::is_black_palette_mask(state.palette),
           "Gate 14-B: dark room with lantern equipped must not be black mask");
    expect(
        state.palette.entries == comic2::make_default_ega_palette(),
        "Gate 14-B: dark room with lantern must restore standard EGA palette");

    // In lit room (e.g. outdoors), palette is visible regardless of lantern
    state.is_dark_room = false;
    state.player.lantern_equipped = false;
    comic2::update_room_lighting_palette(state);
    expect(!comic2::is_black_palette_mask(state.palette),
           "Gate 14-B: lit room without lantern must maintain visible palette");
  }

  // 2. Playfield Viewport Planar Frame Rendering Verification
  {
    auto dark_state = comic2::make_default_runtime_state();
    constexpr std::uint8_t kBorderColor = 0x05U; // sentinel border color
    dark_state.bezel_frame =
        make_gate11_bezel_with_sentinel(kBorderColor, 0x00U);
    dark_state.player.score = 500U;
    dark_state.player.gems = 2U;
    dark_state.player.lives = 3U;
    dark_state.player.shield_power = 5U;
    dark_state.is_dark_room = true;
    dark_state.player.lantern_equipped = false;

    // Build a room grid with non-zero tile indices and non-zero tile graphics
    dark_state.room_grid.tile_w = 20;
    dark_state.room_grid.tile_h = 12;
    dark_state.room_grid.row_pointers.resize(12);
    dark_state.room_grid.tile_data.assign(20 * 12, 0x01);
    for (std::uint16_t row = 0; row < 12; ++row) {
      dark_state.room_grid.row_pointers[row] =
          static_cast<std::uint16_t>(row * 20);
    }

    comic2::Ega4PlaneImage tile{};
    tile.width_bytes = 2U;
    tile.height_rows = 16U;
    tile.row_span_bytes = 32U;
    for (auto &plane : tile.planes) {
      plane.assign(32U, 0xFFU); // white tile (color index 0x0F)
    }
    dark_state.level_tileset = {tile};

    comic2::MemoryFramePresenter presenter;
    comic2::render_bootstrap_frame(presenter, dark_state);
    expect(presenter.has_frame(), "Gate 14-B: frame must be presented");
    const auto &dark_frame = presenter.last_frame();

    // Verify all pixels inside the playfield viewport (x=8..247, y=8..183) are
    // 0x00
    bool all_playfield_black = true;
    for (std::size_t y = 8U; y < 184U && all_playfield_black; ++y) {
      for (std::size_t x = 8U; x < 248U && all_playfield_black; ++x) {
        if (read_gate11_color(dark_frame, x, y) != 0x00U) {
          all_playfield_black = false;
        }
      }
    }
    expect(
        all_playfield_black,
        "Gate 14-B: playfield viewport must be entirely black without lantern");

    // Verify bezel border and HUD panel pixels outside playfield remain intact
    expect(read_gate11_color(dark_frame, 3U, 3U) == kBorderColor,
           "Gate 14-B: bezel border must remain intact during darkness "
           "masking");

    bool hud_has_pixels = false;
    for (std::size_t y = 90U; y < 176U && !hud_has_pixels; ++y) {
      for (std::size_t x = 258U; x < 288U && !hud_has_pixels; ++x) {
        if (read_gate11_color(dark_frame, x, y) != 0x00U) {
          hud_has_pixels = true;
        }
      }
    }
    expect(hud_has_pixels,
           "Gate 14-B: HUD panel must remain visible during cavern darkness");

    // Verify playfield remains black during active room transition effects
    auto transition_dark_state = dark_state;
    transition_dark_state.transition_state.active = true;
    transition_dark_state.transition_state.palette_tint = 3;
    transition_dark_state.transition_state.palette_shift = 1;
    transition_dark_state.transition_state.frame_index = 4;
    transition_dark_state.transition_state.effect_type = 0;
    comic2::render_bootstrap_frame(presenter, transition_dark_state);
    expect(presenter.has_frame(),
           "Gate 14-B: transition frame must be presented");
    const auto &trans_frame = presenter.last_frame();
    bool transition_playfield_black = true;
    for (std::size_t y = 8U; y < 184U && transition_playfield_black; ++y) {
      for (std::size_t x = 8U; x < 248U && transition_playfield_black; ++x) {
        if (read_gate11_color(trans_frame, x, y) != 0x00U) {
          transition_playfield_black = false;
        }
      }
    }
    expect(transition_playfield_black,
           "Gate 14-B: playfield viewport must remain entirely black during "
           "transitions without lantern");

    // 3. Equip Cavern Lantern: Visible Field Renders
    dark_state.player.lantern_equipped = true;
    comic2::render_bootstrap_frame(presenter, dark_state);
    expect(presenter.has_frame(),
           "Gate 14-B: illuminated frame must be presented");
    const auto &lit_frame = presenter.last_frame();

    std::size_t visible_pixel_count = 0;
    for (std::size_t y = 8U; y < 184U; ++y) {
      for (std::size_t x = 8U; x < 248U; ++x) {
        if (read_gate11_color(lit_frame, x, y) != 0x00U) {
          ++visible_pixel_count;
        }
      }
    }
    expect(visible_pixel_count > 0,
           "Gate 14-B: playfield must render visible pixels with lantern "
           "equipped");

    // Compare with an identical baseline room with is_dark_room = false
    auto baseline_state = dark_state;
    baseline_state.is_dark_room = false;
    comic2::MemoryFramePresenter baseline_presenter;
    comic2::render_bootstrap_frame(baseline_presenter, baseline_state);
    const auto &baseline_frame = baseline_presenter.last_frame();

    bool playfield_matches_baseline = true;
    for (std::size_t y = 8U; y < 184U && playfield_matches_baseline; ++y) {
      for (std::size_t x = 8U; x < 248U && playfield_matches_baseline; ++x) {
        if (read_gate11_color(lit_frame, x, y) !=
            read_gate11_color(baseline_frame, x, y)) {
          playfield_matches_baseline = false;
        }
      }
    }
    expect(playfield_matches_baseline,
           "Gate 14-B: lantern illumination must match standard lit room "
           "playfield");

    // 4. Dynamic Toggle and Level Transition
    // Unequip lantern -> becomes dark again
    dark_state.player.lantern_equipped = false;
    comic2::render_bootstrap_frame(presenter, dark_state);
    expect(read_gate11_color(presenter.last_frame(), 100U, 100U) == 0x00U,
           "Gate 14-B: unequipping lantern must re-apply darkness mask");

    // Re-equip -> restores visibility
    dark_state.player.lantern_equipped = true;
    comic2::render_bootstrap_frame(presenter, dark_state);
    expect(read_gate11_color(presenter.last_frame(), 100U, 100U) != 0x00U,
           "Gate 14-B: re-equipping lantern must restore visibility "
           "immediately");

    // Level detection: level 1 cavern detection
    expect(comic2::is_cavern_room_dark(1, 0),
           "Gate 14-B: Level 1 rooms must be recognized as cavern dark rooms");
    expect(!comic2::is_cavern_room_dark(0, 0),
           "Gate 14-B: Level 0 rooms must be recognized as outdoor lit rooms");
  }
}

void test_gate_14c_tile_modification() {
  // Gate 14-C: Drainage Corkscrew (DS:0x08b2), Door Keys (DS:0x08b6), and
  // Teleportation Wand (DS:0x08b4) runtime grid tile transformations.

  // 1. Drainage Corkscrew: Impassable Metal Grate -> Passable Passage
  {
    auto state = comic2::make_default_runtime_state();
    state.room_grid.tile_w = 4;
    state.room_grid.tile_h = 3;
    state.room_grid.row_pointers = {0, 4, 8};
    state.room_grid.tile_data.assign(12, 0);

    const comic2::TileTransformationConfig config;
    const comic2::TileCollisionConfig collision{.solid_tile_threshold = 1};

    // Place an impassable metal grate at tile (1, 1) -> (16, 16)
    comic2::set_tile_at_coords(state.room_grid, 1, 1, config.grate_tile);
    expect(
        !comic2::is_tile_passable_at_pixels(state.room_grid, 16, 16, collision),
        "Gate 14-C: grate tile must be impassable prior to transformation");

    // Position player directly adjacent to grate tile facing it
    state.player.x = 0;
    state.player.y = 16;
    state.player.facing_right = true;

    // Contact without corkscrew: tile remains impassable grate
    state.player.corkscrew_equipped = false;
    const bool grate_unlocked_without =
        comic2::process_player_tile_interactions(state, config);
    expect(!grate_unlocked_without,
           "Gate 14-C: grate must not break without corkscrew equipped");
    auto tile_after_no_corkscrew =
        comic2::get_tile_at_pixels(state.room_grid, 16, 16);
    expect(tile_after_no_corkscrew.has_value() &&
               *tile_after_no_corkscrew == config.grate_tile,
           "Gate 14-C: grate tile ID must remain intact without corkscrew");
    expect(
        !comic2::is_tile_passable_at_pixels(state.room_grid, 16, 16, collision),
        "Gate 14-C: grate tile must stay impassable without corkscrew");

    // Equip Corkscrew [DS:0x08b2] and contact grate: breaks grate to passable
    // tile
    state.player.corkscrew_equipped = true;
    const bool grate_broken =
        comic2::process_player_tile_interactions(state, config);
    expect(grate_broken,
           "Gate 14-C: corkscrew equipped must break grate on contact");
    auto tile_after_corkscrew =
        comic2::get_tile_at_pixels(state.room_grid, 16, 16);
    expect(tile_after_corkscrew.has_value() &&
               *tile_after_corkscrew == config.passable_tile,
           "Gate 14-C: grate tile must transform into passable tile in runtime "
           "grid");
    expect(
        comic2::is_tile_passable_at_pixels(state.room_grid, 16, 16, collision),
        "Gate 14-C: transformed grate tile must now be passable passage");
  }

  // 2. Door Keys: Impassable Locked Grate / Door -> Passable Passage
  {
    auto state = comic2::make_default_runtime_state();
    state.room_grid.tile_w = 4;
    state.room_grid.tile_h = 3;
    state.room_grid.row_pointers = {0, 4, 8};
    state.room_grid.tile_data.assign(12, 0);

    const comic2::TileTransformationConfig config;
    const comic2::TileCollisionConfig collision{.solid_tile_threshold = 1};

    // Place a locked door tile at tile (2, 1) -> (32, 16)
    comic2::set_tile_at_coords(state.room_grid, 2, 1, config.locked_door_tile);
    expect(
        !comic2::is_tile_passable_at_pixels(state.room_grid, 32, 16, collision),
        "Gate 14-C: locked door tile must be impassable prior to unlock");

    // Player adjacent to locked door
    state.player.x = 16;
    state.player.y = 16;
    state.player.facing_right = true;

    // Contact with 0 keys: fails, door stays locked and impassable
    state.player.key_count = 0;
    const bool door_unlocked_zero =
        comic2::process_player_tile_interactions(state, config);
    expect(!door_unlocked_zero,
           "Gate 14-C: locked door must not unlock with 0 keys");
    expect(state.player.key_count == 0,
           "Gate 14-C: key count must remain 0 when unlock fails");
    expect(
        !comic2::is_tile_passable_at_pixels(state.room_grid, 32, 16, collision),
        "Gate 14-C: locked door must remain impassable with 0 keys");

    // Acquire key [DS:0x08b6] = 1: contact unlocks door, decrements key count,
    // transforms tile
    state.player.key_count = 1;
    const bool door_unlocked =
        comic2::process_player_tile_interactions(state, config);
    expect(door_unlocked, "Gate 14-C: touching locked door with key must "
                          "unlock and transform tile");
    expect(state.player.key_count == 0,
           "Gate 14-C: key count must decrement upon unlocking door");
    auto door_tile = comic2::get_tile_at_pixels(state.room_grid, 32, 16);
    expect(door_tile.has_value() && *door_tile == config.passable_tile,
           "Gate 14-C: door tile must transform to passable passage in runtime "
           "grid");
    expect(
        comic2::is_tile_passable_at_pixels(state.room_grid, 32, 16, collision),
        "Gate 14-C: unlocked door tile must now be passable passage");

    // Verify DoorOpen audio event was dispatched
    expect(!state.audio.pending_events.empty() &&
               state.audio.pending_events.back() ==
                   comic2::AudioEvent::DoorOpen,
           "Gate 14-C: door unlock must queue AudioEvent::DoorOpen");
  }

  // 3. Teleportation Wand: Return to Room Entrance & Warp Beacon Tile
  // Transformation
  {
    auto state = comic2::make_default_runtime_state();
    state.room_grid.tile_w = 4;
    state.room_grid.tile_h = 3;
    state.room_grid.row_pointers = {0, 4, 8};
    state.room_grid.tile_data.assign(12, 0);

    const comic2::TileTransformationConfig config;

    state.room_entrance_x = 64;
    state.room_entrance_y = 96;

    // Player at distant coordinates in motion
    state.player.x = 220;
    state.player.y = 140;
    state.player.x_vel = 8;
    state.player.y_vel = -6;
    state.player.is_airborne = true;

    // Wand unequipped [DS:0x08b4] -> cannot teleport
    state.player.wand_equipped = false;
    expect(!comic2::try_use_teleport_wand(state, config),
           "Gate 14-C: teleport wand use must fail when wand is unequipped");
    expect(
        state.player.x == 220 && state.player.y == 140,
        "Gate 14-C: player coordinates must not change when wand unequipped");

    // Equip wand [DS:0x08b4] -> teleports back to room entrance
    state.player.wand_equipped = true;
    state.audio.pending_events.clear();
    const bool wand_success = comic2::try_use_teleport_wand(state, config);
    expect(wand_success,
           "Gate 14-C: teleport wand use must succeed when equipped");
    expect(state.player.x == 64 && state.player.y == 96,
           "Gate 14-C: wand must teleport Comic back to room entrance");
    expect(state.player.x_vel == 0 && state.player.y_vel == 0,
           "Gate 14-C: wand teleport must reset player velocities to 0");
    expect(!state.player.is_airborne,
           "Gate 14-C: wand teleport must clear airborne state");
    expect(!state.audio.pending_events.empty() &&
               state.audio.pending_events.back() ==
                   comic2::AudioEvent::Teleport,
           "Gate 14-C: wand teleport must queue AudioEvent::Teleport");

    // Warp Beacon tile transformation in runtime grid
    comic2::set_tile_at_coords(state.room_grid, 2, 2, config.warp_beacon_tile);
    const bool beacon_activated =
        comic2::try_activate_warp_beacon(state, 32, 32, config);
    expect(beacon_activated,
           "Gate 14-C: wand must activate warp beacon tile on contact");
    auto beacon_tile = comic2::get_tile_at_pixels(state.room_grid, 32, 32);
    expect(beacon_tile.has_value() &&
               *beacon_tile == config.active_warp_portal_tile,
           "Gate 14-C: warp beacon tile must transform into active portal in "
           "runtime grid");
    expect(state.warp_beacon.has_value() && state.warp_beacon->first == 32 &&
               state.warp_beacon->second == 32,
           "Gate 14-C: warp beacon coordinates must be registered in state");

    // Teleporting with active warp beacon teleports to beacon
    state.player.x = 180;
    state.player.y = 120;
    expect(comic2::try_use_teleport_wand(state, config),
           "Gate 14-C: teleporting with active warp beacon must succeed");
    expect(
        state.player.x == 32 && state.player.y == 32,
        "Gate 14-C: player must be teleported to active warp beacon location");
  }

  // 4. Progression State Synchronization
  {
    auto state = comic2::make_default_runtime_state();
    state.player.boots_equipped = true;
    state.player.lantern_equipped = true;
    state.player.corkscrew_equipped = true;
    state.player.wand_equipped = true;
    state.player.key_count = 3;

    comic2::update_progression_state(state);
    expect(state.progression.boots_acquired,
           "Gate 14-C: boots_acquired must sync with boots_equipped");
    expect(state.progression.lantern_acquired,
           "Gate 14-C: lantern_acquired must sync with lantern_equipped");
    expect(state.progression.corkscrew_acquired,
           "Gate 14-C: corkscrew_acquired must sync with corkscrew_equipped");
    expect(state.progression.wand_acquired,
           "Gate 14-C: wand_acquired must sync with wand_equipped");
    expect(state.progression.keys_collected == 3,
           "Gate 14-C: keys_collected must sync with key_count");
  }
}

std::filesystem::path find_gate_reference_original_dir() {
  const std::vector<std::filesystem::path> candidates = {
      "reference/original",
      "../reference/original",
      "../../reference/original",
  };
  const auto it = std::find_if(candidates.begin(), candidates.end(),
                               [](const std::filesystem::path &p) {
                                 return std::filesystem::exists(p / "GAME.000");
                               });
  if (it != candidates.end()) {
    return *it;
  }
  return {};
}

void test_gate_15a_savegame_roundtrip() {
  const auto dir = find_gate_reference_original_dir();
  if (dir.empty()) {
    return;
  }

  // 1. Validate authentic GAME.000 byte-for-byte roundtrip
  const auto game0_path = dir / "GAME.000";
  const auto game0_bytes = comic2::load_file_bytes(game0_path);
  expect(game0_bytes.has_value(), "Gate 15-A: GAME.000 file must exist");
  expect(game0_bytes->size() == comic2::kDosSavegameSize,
         "Gate 15-A: GAME.000 binary size must be exactly 3,127 bytes");

  comic2::RuntimeState state = comic2::make_default_runtime_state();
  comic2::savegame_read_snapshot(state, *game0_bytes);

  expect(state.mapped_objects.size() == 250,
         "Gate 15-A: GAME.000 must deserialize 250 mapped object descriptors");
  expect(state.player.x == 568 && state.player.y == 80,
         "Gate 15-A: GAME.000 player spawn coordinates must equal (568, 80)");
  expect(state.player.facing_right,
         "Gate 15-A: GAME.000 initial facing direction must be right");
  expect(state.current_level == 0 && state.current_room == 0,
         "Gate 15-A: GAME.000 initial level/room must be 0/0");

  std::vector<std::uint8_t> roundtrip_bytes;
  comic2::savegame_write_snapshot(state, roundtrip_bytes);
  expect(roundtrip_bytes.size() == comic2::kDosSavegameSize,
         "Gate 15-A: serialized savegame must be exactly 3,127 bytes");
  expect(roundtrip_bytes == *game0_bytes,
         "Gate 15-A: saved GAME.000 binary must match original DOS file "
         "byte-for-byte");

  // 2. Validate mutated in-game state serialization roundtrip
  state.player.x = 420;
  state.player.y = 112;
  state.player.firepower = 4;
  state.player.boots_equipped = true;
  state.player.lantern_equipped = true;
  state.player.wand_equipped = true;
  state.player.key_count = 5;
  state.player.score = 9876;
  state.player.gems = 42;
  state.current_level = 2;
  state.current_room = 5;

  std::vector<std::uint8_t> mutated_bytes;
  comic2::savegame_write_snapshot(state, mutated_bytes);
  expect(mutated_bytes.size() == comic2::kDosSavegameSize,
         "Gate 15-A: mutated savegame snapshot must be 3,127 bytes");

  comic2::RuntimeState loaded_mutated = comic2::make_default_runtime_state();
  comic2::savegame_read_snapshot(loaded_mutated, mutated_bytes);
  expect(loaded_mutated.player.x == 420 && loaded_mutated.player.y == 112,
         "Gate 15-A: restored coordinates must match mutated values");
  expect(loaded_mutated.player.firepower == 4,
         "Gate 15-A: restored firepower must match mutated level");
  expect(loaded_mutated.player.boots_equipped &&
             loaded_mutated.player.lantern_equipped &&
             loaded_mutated.player.wand_equipped,
         "Gate 15-A: restored equipment flags must remain active");
  expect(loaded_mutated.player.key_count == 5,
         "Gate 15-A: restored keys count must match mutated count");
  expect(loaded_mutated.player.score == 9876,
         "Gate 15-A: restored score must match mutated score");
  expect(loaded_mutated.player.gems == 42,
         "Gate 15-A: restored gems must match mutated gems");
  expect(loaded_mutated.current_level == 2 && loaded_mutated.current_room == 5,
         "Gate 15-A: restored level/room must match mutated coordinates");
}

void test_gate_15b_config_persistence() {
  const auto dir = find_gate_reference_original_dir();
  if (dir.empty()) {
    return;
  }

  // 1. Validate authentic FR.CFG byte-for-byte roundtrip
  const auto frcfg_path = dir / "FR.CFG";
  const auto frcfg_bytes = comic2::load_file_bytes(frcfg_path);
  expect(frcfg_bytes.has_value(), "Gate 15-B: FR.CFG file must exist");
  expect(frcfg_bytes->size() == comic2::kDosConfigSize,
         "Gate 15-B: FR.CFG binary size must be exactly 102 bytes");

  comic2::RuntimeState cfg_state = comic2::make_default_runtime_state();
  comic2::cfg_load_options_or_defaults(cfg_state, *frcfg_bytes);

  std::vector<std::uint8_t> saved_cfg_bytes;
  comic2::cfg_save_options(cfg_state, saved_cfg_bytes);
  expect(saved_cfg_bytes.size() == comic2::kDosConfigSize,
         "Gate 15-B: saved FR.CFG must be exactly 102 bytes");
  expect(saved_cfg_bytes == *frcfg_bytes,
         "Gate 15-B: saved FR.CFG binary must match original DOS file "
         "byte-for-byte");

  // 2. Validate custom player profiles and audio options persistence
  cfg_state.dos_config.player_count = 2;
  cfg_state.dos_config.player_names[0] = "COMIC";
  cfg_state.dos_config.player_names[1] = "FRACTURE";
  cfg_state.ui.music_volume = 7;
  cfg_state.ui.sfx_volume = 8;
  cfg_state.ui.display_scale = 2;

  std::vector<std::uint8_t> custom_cfg_bytes;
  comic2::cfg_save_options(cfg_state, custom_cfg_bytes);
  expect(custom_cfg_bytes.size() == comic2::kDosConfigSize,
         "Gate 15-B: custom config must serialize to 102 bytes");

  comic2::RuntimeState loaded_cfg = comic2::make_default_runtime_state();
  comic2::cfg_load_options_or_defaults(loaded_cfg, custom_cfg_bytes);
  expect(loaded_cfg.dos_config.player_count == 2,
         "Gate 15-B: player profile count must roundtrip");
  expect(loaded_cfg.dos_config.player_names[0] == "COMIC",
         "Gate 15-B: player 0 name must roundtrip");
  expect(loaded_cfg.dos_config.player_names[1] == "FRACTURE",
         "Gate 15-B: player 1 name must roundtrip");
  expect(loaded_cfg.ui.music_volume == 7 && loaded_cfg.ui.sfx_volume == 8,
         "Gate 15-B: custom music/sfx volume levels must roundtrip");
  expect(loaded_cfg.ui.display_scale == 2,
         "Gate 15-B: custom display scale must roundtrip");

  // 3. Validate KEYS.DEF custom scancode persistence and defaults
  const auto keys_path = dir / "KEYS.DEF";
  const auto keys_bytes = comic2::load_file_bytes(keys_path);
  if (keys_bytes.has_value()) {
    expect(keys_bytes->size() == comic2::kDosKeyBindingsSize,
           "Gate 15-B: KEYS.DEF binary size must be exactly 6 bytes");

    comic2::DosKeyBindings keys{};
    comic2::keys_load_or_defaults(keys, *keys_bytes);

    std::vector<std::uint8_t> saved_keys;
    comic2::keys_save(keys, saved_keys);
    expect(saved_keys.size() == comic2::kDosKeyBindingsSize,
           "Gate 15-B: saved KEYS.DEF must be exactly 6 bytes");
    expect(
        saved_keys == *keys_bytes,
        "Gate 15-B: saved KEYS.DEF must match original DOS file byte-for-byte");
  }

  comic2::DosKeyBindings default_keys{};
  comic2::keys_load_or_defaults(default_keys, {});
  expect(
      default_keys.left == 0x4B && default_keys.right == 0x4D &&
          default_keys.up == 0x48 && default_keys.down == 0x50 &&
          default_keys.jump == 0x39 && default_keys.fire == 0x52,
      "Gate 15-B: empty payload must restore authentic DOS default scancodes");
}

void test_gate_15c_end_to_end_game_flow() {
  using comic2::GameLifecycleState;
  const auto dir = find_gate_reference_original_dir();

  comic2::RuntimeState state = comic2::make_default_runtime_state();
  if (!dir.empty()) {
    comic2::load_initial_bootstrap_resources(state, dir);
  }

  comic2::GameDispatcher dispatcher = comic2::make_default_game_dispatcher();
  comic2::EgaPlanarSurface surface(320, 200);

  // 1. Title State: verify initial state and rendering
  expect(comic2::get_game_lifecycle_state(state) == GameLifecycleState::Title,
         "Gate 15-C: initial lifecycle state must be Title");
  comic2::render_game_lifecycle_frame(surface, state);

  // Advance from Title to Intro via user input
  comic2::InputState input{.jump_pressed = true};
  expect(comic2::update_game_lifecycle(state, input, &dispatcher),
         "Gate 15-C: Title input must succeed");
  expect(comic2::get_game_lifecycle_state(state) == GameLifecycleState::Intro,
         "Gate 15-C: state must transition to Intro");

  // 2. Intro State: play through intro sequence
  comic2::render_game_lifecycle_frame(surface, state);
  state.ui.cinematic_frame = 4;
  input = comic2::InputState{};
  expect(comic2::update_game_lifecycle(state, input, &dispatcher),
         "Gate 15-C: Intro sequence advance must succeed");
  expect(comic2::get_game_lifecycle_state(state) == GameLifecycleState::Menu,
         "Gate 15-C: state must transition from Intro to Menu");
  expect(state.ui.menu_state == comic2::MenuState::GameSelect,
         "Gate 15-C: Menu state must be GameSelect");

  // 3. Menu State: navigate options and select game
  state.ui.selected_option_index = 0;
  comic2::render_game_lifecycle_frame(surface, state);

  // Confirm selection with fire input to start gameplay
  input = comic2::InputState{.fire_pressed = true};
  expect(comic2::update_game_lifecycle(state, input, &dispatcher),
         "Gate 15-C: Menu confirmation must succeed");
  expect(comic2::get_game_lifecycle_state(state) ==
             GameLifecycleState::Gameplay,
         "Gate 15-C: state must transition from Menu to Gameplay");
  expect(state.ui.menu_state == comic2::MenuState::None,
         "Gate 15-C: Menu state must be None in Gameplay");

  // 4. Gameplay State: run active gameplay loop
  state.player.hp = 12;
  state.player.lives = 3;
  state.player.score = 500;
  state.player.gems = 5;
  const auto initial_x = state.player.x;

  for (int tick = 0; tick < 10; ++tick) {
    input = comic2::InputState{.right_pressed = true};
    expect(comic2::update_game_lifecycle(state, input, &dispatcher),
           "Gate 15-C: Gameplay tick must succeed");
    comic2::render_game_lifecycle_frame(surface, state);
  }
  expect(state.player.x >= initial_x,
         "Gate 15-C: player must advance right during gameplay");

  // 5. Save State: trigger savegame snapshot persistence
  state.flags.room_event_triggered = true;
  expect(
      comic2::update_game_lifecycle(state, comic2::InputState{}, &dispatcher),
      "Gate 15-C: transition to Save must succeed");
  expect(comic2::get_game_lifecycle_state(state) == GameLifecycleState::Save,
         "Gate 15-C: state must transition from Gameplay to Save");
  expect(state.saved_snapshot_bytes.size() == comic2::kDosSavegameSize,
         "Gate 15-C: saved snapshot must be exact authentic 3,127-byte DOS "
         "binary");

  // Verify roundtrip fidelity of the snapshot taken in Save state
  comic2::RuntimeState restored_state = comic2::make_default_runtime_state();
  comic2::savegame_read_snapshot(restored_state, state.saved_snapshot_bytes);
  expect(restored_state.player.lives == state.player.lives,
         "Gate 15-C: save snapshot must preserve player lives");
  expect(restored_state.player.gems == state.player.gems,
         "Gate 15-C: save snapshot must preserve player gems");

  // 6. Victory State: advance from Save/Gameplay to Victory
  state.progression.flags |= 0x80U;
  input = comic2::InputState{.fire_pressed = true};
  expect(comic2::update_game_lifecycle(state, input, &dispatcher),
         "Gate 15-C: transition to Victory must succeed");
  expect(comic2::get_game_lifecycle_state(state) == GameLifecycleState::Victory,
         "Gate 15-C: state must transition to Victory");
  expect((state.progression.flags & 0x80U) != 0,
         "Gate 15-C: victory progression flag must remain set");

  // Render victory scene
  comic2::render_game_lifecycle_frame(surface, state);

  // Return to Menu from Victory
  input = comic2::InputState{.fire_pressed = true};
  expect(comic2::update_game_lifecycle(state, input, &dispatcher),
         "Gate 15-C: transition from Victory to Menu must succeed");
  expect(comic2::get_game_lifecycle_state(state) == GameLifecycleState::Menu,
         "Gate 15-C: state must return smoothly to Menu");
}

} // namespace

void run_integration_gate_tests() {
  test_gate_b_scripted_trace_matches_oracle();
  test_gate_b_oracle_stage_sequence_rotation();

  test_gate_c_state_vectors_match_snapshots();
  test_gate_c_hazard_flag_snapshot_progression();

  test_gate_86a_resolver_mapping_is_deterministic();
  test_gate_86d_room_load_persists_resource_bytes_for_reentry();
  test_gate_d_room_fixture_alpha_hash_matches_oracle();
  test_gate_d_room_fixture_beta_hash_matches_oracle();

  test_gate_phase_91_combat_and_pickup_flow();
  test_gate_e_hazard_death_routes_to_special_state();
  test_gate_e_room_transition_boundary_sequence();
  test_gate_e_left_boundary_clamps_at_room_zero();
  test_gate_e_projectile_collision_outcome();
  test_gate_phase_93_transition_flow_freezes_player_and_tracks_camera();
  test_gate_phase_10d_event_trigger_flow();
  test_gate_phase_92_audio_events_trigger_from_gameplay();

  // Gate 11-E: Viewport compositing and HUD counter rendering.
  test_gate_11e_font_glyph_digit_is_nonzero();
  test_gate_11e_blit_string_writes_pixels_on_surface();
  test_gate_11e_bezel_border_not_overwritten_by_tiles();
  test_gate_11e_hud_counters_appear_in_bezel_panel_region();
  test_gate_11e_no_bezel_tiles_reach_full_frame_width();
  test_gate_11e_no_bezel_tiles_reach_full_frame_height();

  // Gate 11-F: deterministic replay stability across room transitions.
  test_gate_11f_scripted_replay_is_deterministic_across_room_transition();
  test_gate_11f_room_transition_reloads_geometry_and_entity_slots();
  test_gate_11f_level0_room0_initializes_from_real_assets_if_present();

  // Gate 12-A/B/C: PIT synthesis and headless audio behavior.
  test_gate_12a_frequency_parity_for_pit_1193_divisor();
  test_gate_12b_priority_preemption_in_sound_driver();
  test_gate_12b2_sound_driver_tick_and_synthesizer();
  test_gate_12c_null_audio_backend_stays_headless();

  // Gate 13-A/B/C: Level script decode parity, entity movement parity, and
  // trigger activation.
  test_gate_13a_script_decode_parity();
  test_gate_13b_entity_movement_parity();
  test_gate_13c_trigger_activation();

  // Gate 14-A: High-jump boots trajectory parity and firepower progression.
  test_gate_14a_boots_trajectory_and_firepower();

  // Gate 14-B: Cavern lantern illumination and palette darkness mechanics.
  test_gate_14b_cavern_lantern_and_palette_darkness();

  // Gate 14-C: Tile modification (corkscrew, keys, and teleportation wand).
  test_gate_14c_tile_modification();

  // Gate 15-A: Authentic DOS savegame binary roundtrip.
  test_gate_15a_savegame_roundtrip();

  // Gate 15-B: Authentic DOS configuration and key definition persistence.
  test_gate_15b_config_persistence();

  // Gate 15-C: End-to-End Game Flow Lifecycle.
  test_gate_15c_end_to_end_game_flow();
}
