#include <array>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "comic2/audio.hpp"
#include "comic2/bootstrap.hpp"
#include "comic2/default_handlers.hpp"
#include "comic2/dispatcher.hpp"
#include "comic2/entity_runtime.hpp"
#include "comic2/projectiles.hpp"
#include "comic2/renderer.hpp"
#include "comic2/renderer_validation.hpp"
#include "comic2/room_loader.hpp"

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

// Gate 11-E: Frame Compositing — bezel, viewport clipping, and HUD counters.

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
  // an all-white tile; pixels at x ≥ 240 (e.g. column 300) must be written.
  auto state = comic2::make_default_runtime_state();
  // No bezel — state.bezel_frame stays std::nullopt.

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

  expect(presenter.has_frame(),
         "Gate 11-E no-bezel: frame must be presented");
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
  state.player.score = 1000U; // non-zero → overlay guard passes
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
}
