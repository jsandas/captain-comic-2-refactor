#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

#include "comic2/bootstrap.hpp"
#include "comic2/default_handlers.hpp"
#include "comic2/game_state.hpp"
#include "comic2/renderer.hpp"
#include "comic2/renderer_validation.hpp"
#include "comic2/resource_loader.hpp"

namespace {

void set_test_env(const char *name, const char *value, int overwrite) {
#ifdef _WIN32
  // Windows doesn't have setenv; use _putenv_s instead.
  (void)overwrite;
  _putenv_s(name, value);
#else
  if (value == nullptr || *value == '\0') {
    unsetenv(name);
  } else {
    setenv(name, value, overwrite);
  }
#endif
}

void check(bool condition, const char *message) {
  if (!condition) {
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

std::optional<std::filesystem::path> find_reference_assets_root() {
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
          if (name.rfind("FRDATA.", 0) == 0 && entry.is_regular_file()) {
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

  return std::nullopt;
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

  std::vector<std::uint8_t> encoded;
  std::size_t offset = 0;
  while (offset < decoded_room_bytes.size()) {
    const std::size_t chunk =
        std::min<std::size_t>(127U, decoded_room_bytes.size() - offset);
    encoded.push_back(static_cast<std::uint8_t>(chunk));
    encoded.insert(encoded.end(),
                   decoded_room_bytes.begin() +
                       static_cast<std::ptrdiff_t>(offset),
                   decoded_room_bytes.begin() +
                       static_cast<std::ptrdiff_t>(offset + chunk));
    offset += chunk;
  }
  encoded.push_back(0x00);

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

struct RecordingPresenter : comic2::IFramePresenter {
  int last_width = 0;
  int last_height = 0;
  int present_calls = 0;

  void present(const comic2::EgaPlanarSurface &frame) override {
    ++present_calls;
    last_width = frame.width_pixels();
    last_height = frame.height_rows();
  }
};

std::uint8_t read_color_index(const comic2::EgaPlanarSurface &frame,
                              std::size_t x, std::size_t y) {
  const std::size_t x_byte = x / 8;
  const std::uint8_t bit = static_cast<std::uint8_t>(7 - (x % 8));
  std::uint8_t index = 0;
  for (std::size_t plane = 0; plane < comic2::EgaPlanarSurface::kPlaneCount;
       ++plane) {
    const auto value = frame.get_plane_byte(plane, x_byte, y);
    if (value & static_cast<std::uint8_t>(1U << bit)) {
      index = static_cast<std::uint8_t>(index | (1U << plane));
    }
  }
  return index;
}

comic2::Ega4PlaneImage make_asset_fixture_image() {
  comic2::Ega4PlaneImage image;
  image.row_span_bytes = 0x1F40;
  image.width_bytes = 40;
  image.height_rows = 200;
  for (auto &plane : image.planes) {
    plane.assign(8000, 0x00);
  }

  // Tile 0 (x bytes 0..1): color index 1 (plane 0 set)
  for (std::size_t row = 0; row < 16; ++row) {
    image.planes[0][row * 40] = 0xFF;
    image.planes[0][row * 40 + 1] = 0xFF;
  }

  // Tile 1 (x bytes 2..3): color index 2 (plane 1 set)
  for (std::size_t row = 0; row < 16; ++row) {
    image.planes[1][row * 40 + 2] = 0xFF;
    image.planes[1][row * 40 + 3] = 0xFF;
  }

  return image;
}

void test_bootstrap_entry_runs_without_crashing() {
  const auto empty_root =
      std::filesystem::temp_directory_path() / "comic2_bootstrap_empty";
  std::filesystem::remove_all(empty_root);
  std::filesystem::create_directories(empty_root);

  set_test_env("COMIC2_BOOTSTRAP_TICKS", "1", 1);
  set_test_env("COMIC2_FORCE_ENV_INPUT", "1", 1);
  set_test_env("COMIC2_INPUT_QUIT", "0", 1);
  set_test_env("COMIC2_INPUT_LEFT", "0", 1);
  set_test_env("COMIC2_INPUT_RIGHT", "0", 1);
  set_test_env("COMIC2_INPUT_JUMP", "0", 1);
  set_test_env("COMIC2_INPUT_DOWN", "0", 1);

  const int exit_code = comic2::run_bootstrap_entry(empty_root);

  check(exit_code == 0, "bootstrap entry should continue even when bootstrap "
                        "resources are missing");

  std::filesystem::remove_all(empty_root);
}

void test_bootstrap_tick_wires_input_dispatch_and_render() {
  set_test_env("COMIC2_FORCE_ENV_INPUT", "1", 1);
  set_test_env("COMIC2_INPUT_QUIT", "0", 1);
  set_test_env("COMIC2_INPUT_LEFT", "1", 1);
  set_test_env("COMIC2_INPUT_RIGHT", "1", 1);
  set_test_env("COMIC2_INPUT_JUMP", "0", 1);
  set_test_env("COMIC2_INPUT_DOWN", "0", 1);

  auto state = comic2::make_default_runtime_state();
  state.player.is_airborne = false;
  state.player.is_physics_active = true;

  auto dispatcher = comic2::make_default_game_dispatcher();
  RecordingPresenter presenter;

  const auto summary = comic2::run_bootstrap_tick(state, dispatcher, presenter);

  check(summary.input_captured, "bootstrap tick should capture input flags");
  check(summary.stage == comic2::DispatchStage::GroundedPhysics,
        "bootstrap tick should enter grounded physics stage when grounded");
  check(summary.frame_presented, "bootstrap tick should render a frame");
  check(presenter.present_calls == 1,
        "bootstrap tick should invoke the presenter once");
  check(state.input.left_pressed, "left input should be captured");
  check(state.input.right_pressed, "right input should be captured");
  check(!state.input.jump_pressed,
        "jump input should remain false when env says 0");
}

void test_bootstrap_frame_interval_defaults_to_original_tick_rate() {
  set_test_env("COMIC2_BOOTSTRAP_FRAME_MS", "", 1);

  const auto interval = comic2::read_bootstrap_frame_interval();

  check(interval == std::chrono::milliseconds(110),
        "bootstrap frame interval should default to the original 110 ms tick "
        "pacing");
}

void test_bootstrap_frame_interval_can_be_overridden() {
  set_test_env("COMIC2_BOOTSTRAP_FRAME_MS", "33", 1);

  const auto interval = comic2::read_bootstrap_frame_interval();

  check(interval == std::chrono::milliseconds(33),
        "bootstrap frame interval should accept an override for calibration");
  set_test_env("COMIC2_BOOTSTRAP_FRAME_MS", "", 1);
}

void test_scene_bootstrap_discovers_reference_assets_from_repo_subdirectory() {
  const auto repo_root =
      std::filesystem::temp_directory_path() / "comic2_repo_root_resolution";
  const auto subdir = repo_root / "nested" / "launch";
  const auto asset_root = repo_root / "reference" / "original";

  std::filesystem::remove_all(repo_root);
  std::filesystem::create_directories(subdir);
  std::filesystem::create_directories(asset_root);

  const auto room_bytes = make_test_room_resource_bytes(0, 0, 4, 3, 0x7AU);
  std::ofstream(asset_root / "FRDATA.0", std::ios::binary)
      .write(reinterpret_cast<const char *>(room_bytes.data()),
             static_cast<std::streamsize>(room_bytes.size()));

  const auto previous_cwd = std::filesystem::current_path();
  std::filesystem::current_path(subdir);

  try {
    comic2::RuntimeState state = comic2::make_default_runtime_state();
    const auto summary = comic2::initialize_runtime_scene(state, subdir);
    const auto canonical_asset_root =
        std::filesystem::weakly_canonical(asset_root);
    const auto canonical_summary_root =
        std::filesystem::weakly_canonical(summary.assets_root_used);

    check(summary.room_grid_loaded,
          "bootstrap should load the repo-root original asset directory from a "
          "nested launch path");
    check(
        canonical_summary_root == canonical_asset_root,
        "bootstrap should resolve the canonical reference/original directory");
    check(std::filesystem::weakly_canonical(state.assets_root) ==
              canonical_asset_root,
          "runtime state should preserve the canonical asset root");
  } catch (...) {
    std::filesystem::current_path(previous_cwd);
    std::filesystem::remove_all(repo_root);
    throw;
  }

  std::filesystem::current_path(previous_cwd);
  std::filesystem::remove_all(repo_root);
}

void test_integrated_loop_exits_cleanly_when_quit_is_requested() {
  const auto empty_root =
      std::filesystem::temp_directory_path() / "comic2_integrated_quit";
  std::filesystem::remove_all(empty_root);
  std::filesystem::create_directories(empty_root);

  set_test_env("COMIC2_FORCE_ENV_INPUT", "1", 1);
  set_test_env("COMIC2_INPUT_QUIT", "1", 1);
  set_test_env("COMIC2_INPUT_LEFT", "0", 1);
  set_test_env("COMIC2_INPUT_RIGHT", "0", 1);
  set_test_env("COMIC2_INPUT_JUMP", "0", 1);
  set_test_env("COMIC2_INPUT_DOWN", "0", 1);

  RecordingPresenter presenter;
  const auto summary = comic2::run_integrated_bootstrap_loop(
      empty_root, presenter, 5, std::chrono::milliseconds(0));

  check(
      !summary.bootstrap.room_grid_loaded,
      "integrated loop should stay in placeholder mode when assets are absent");
  check(summary.loop.quit_requested,
        "integrated loop should report a quit request");
  check(summary.loop.frames_rendered == 0,
        "integrated loop should stop before rendering frames when quit is "
        "requested");

  std::filesystem::remove_all(empty_root);
}

void test_dispatcher_moves_player_when_left_input_is_active() {
  set_test_env("COMIC2_FORCE_ENV_INPUT", "1", 1);
  set_test_env("COMIC2_INPUT_QUIT", "0", 1);
  set_test_env("COMIC2_INPUT_LEFT", "1", 1);
  set_test_env("COMIC2_INPUT_RIGHT", "0", 1);
  set_test_env("COMIC2_INPUT_JUMP", "0", 1);
  set_test_env("COMIC2_INPUT_DOWN", "0", 1);

  auto state = comic2::make_default_runtime_state();
  state.player.is_airborne = false;
  state.player.is_physics_active = true;

  auto dispatcher = comic2::make_default_game_dispatcher();
  comic2::poll_bootstrap_input(state);
  const auto result = dispatcher.run_tick(state);

  check(result.stage == comic2::DispatchStage::GroundedPhysics,
        "dispatcher should run the grounded physics stage for the player");
  check(state.player.x < 64,
        "left input should move the player left through the dispatcher path");
  check(state.player.x_vel < 0,
        "left input should set a negative horizontal velocity");
}

void test_render_loop_renders_multiple_frames() {
  set_test_env("COMIC2_FORCE_ENV_INPUT", "1", 1);
  set_test_env("COMIC2_INPUT_QUIT", "0", 1);
  set_test_env("COMIC2_INPUT_LEFT", "0", 1);
  set_test_env("COMIC2_INPUT_RIGHT", "0", 1);
  set_test_env("COMIC2_INPUT_JUMP", "0", 1);
  set_test_env("COMIC2_INPUT_DOWN", "0", 1);

  auto state = comic2::make_default_runtime_state();
  auto dispatcher = comic2::make_default_game_dispatcher();
  RecordingPresenter presenter;

  const auto summary = comic2::run_render_loop(state, dispatcher, presenter, 3,
                                               std::chrono::milliseconds(0));

  check(summary.frames_rendered == 3,
        "render loop should render the requested frame count");
  check(summary.ticks_processed == 3,
        "render loop should process one dispatcher tick per frame");
  check(presenter.present_calls == 3,
        "render loop should invoke the presenter for each frame");
}

void test_render_loop_updates_state_while_presenting_frames() {
  set_test_env("COMIC2_FORCE_ENV_INPUT", "1", 1);
  set_test_env("COMIC2_INPUT_QUIT", "0", 1);
  set_test_env("COMIC2_INPUT_LEFT", "0", 1);
  set_test_env("COMIC2_INPUT_RIGHT", "1", 1);
  set_test_env("COMIC2_INPUT_JUMP", "0", 1);
  set_test_env("COMIC2_INPUT_DOWN", "0", 1);

  auto state = comic2::make_default_runtime_state();
  state.player.is_airborne = false;
  state.player.is_physics_active = true;
  const std::int16_t initial_x = state.player.x;

  auto dispatcher = comic2::make_default_game_dispatcher();
  RecordingPresenter presenter;

  const auto summary = comic2::run_render_loop(state, dispatcher, presenter, 4,
                                               std::chrono::milliseconds(0));

  check(summary.frames_rendered == 4,
        "render loop should render continuously until frame budget is reached");
  check(summary.ticks_processed == 4,
        "render loop should process one game tick per presented frame");
  check(presenter.present_calls == 4,
        "render loop should present each frame while running continuously");
  check(state.player.x > initial_x,
        "runtime state should update while frames are being presented");
}

void test_render_bootstrap_frame_uses_room_tile_data() {
  auto state = comic2::make_default_runtime_state();
  state.player.x = 300;
  state.player.y = 180;
  state.room_grid.tile_w = 1;
  state.room_grid.tile_h = 1;
  state.room_grid.row_pointers = {0};
  state.room_grid.tile_data = {0x0A};

  comic2::MemoryFramePresenter presenter;
  comic2::render_bootstrap_frame(presenter, state);

  check(presenter.has_frame(), "render should present a frame");
  const auto &frame_a = presenter.last_frame();
  const auto color_a = read_color_index(frame_a, 8, 8);

  state.room_grid.tile_data = {0x02};
  comic2::render_bootstrap_frame(presenter, state);
  const auto &frame_b = presenter.last_frame();
  const auto color_b = read_color_index(frame_b, 8, 8);

  check(color_a != color_b,
        "rendered tile color should change when room tile data changes");

  const auto top_left_edge = read_color_index(frame_b, 0, 0);
  const auto center_fill = read_color_index(frame_b, 8, 8);
  const auto bottom_right_edge = read_color_index(frame_b, 15, 15);

  check(top_left_edge == 0x04,
        "tile border should use the accent color derived from tile data");
  check(center_fill == 0x02,
        "tile interior should use the base color derived from tile data");
  check(bottom_right_edge == 0x04,
        "tile border should remain accented at the far edge");
}

void test_render_bootstrap_frame_uses_level_tileset_bytes_when_available() {
  auto state = comic2::make_default_runtime_state();
  state.player.x = 300;
  state.player.y = 180;
  state.room_grid.tile_w = 1;
  state.room_grid.tile_h = 1;
  state.room_grid.row_pointers = {0};
  state.room_grid.tile_data = {0x00};

  comic2::Ega4PlaneImage tile;
  tile.width_bytes = 2;
  tile.height_rows = 16;
  tile.row_span_bytes = 32;
  for (std::size_t plane = 0; plane < tile.planes.size(); ++plane) {
    tile.planes[plane].assign(32, 0x00);
  }
  for (std::size_t row = 0; row < 16; ++row) {
    tile.planes[0][row * 2] = 0xFF;
    tile.planes[0][row * 2 + 1] = 0xFF;
  }
  state.level_tileset = {tile};

  comic2::MemoryFramePresenter presenter;
  comic2::render_bootstrap_frame(presenter, state);

  const auto &frame = presenter.last_frame();
  const auto color = read_color_index(frame, 8, 8);

  check(color == 0x01,
        "bootstrap should render the level tileset tile bytes instead of the "
        "diagnostic per-tile color fallback");
}

void set_bezel_pixel(comic2::Ega4PlaneImage &image, std::size_t x,
                     std::size_t y, std::uint8_t color_index) {
  const auto x_byte = x / 8U;
  const auto bit = static_cast<std::uint8_t>(7U - (x % 8U));
  const auto mask = static_cast<std::uint8_t>(1U << bit);
  const auto row_offset = y * image.width_bytes;

  for (std::size_t plane = 0; plane < image.planes.size(); ++plane) {
    auto &plane_data = image.planes[plane];
    auto value = plane_data[row_offset + x_byte];
    if ((color_index >> plane) & 0x1U) {
      value |= mask;
    } else {
      value &= static_cast<std::uint8_t>(~mask);
    }
    plane_data[row_offset + x_byte] = value;
  }
}

void test_render_bootstrap_frame_uses_bezel_canvas_when_available() {
  auto state = comic2::make_default_runtime_state();
  state.bezel_frame = comic2::Ega4PlaneImage{};
  auto &bezel = *state.bezel_frame;
  bezel.width_bytes = 40U;
  bezel.height_rows = 200U;
  bezel.row_span_bytes = 0x1F40U;
  for (auto &plane : bezel.planes) {
    plane.assign(8000U, 0x00U);
  }
  set_bezel_pixel(bezel, 10U, 20U, 0x05U);

  comic2::MemoryFramePresenter presenter;
  comic2::render_bootstrap_frame(presenter, state);

  check(presenter.has_frame(),
        "bezel-backed renders should still present a frame");
  const auto &frame = presenter.last_frame();
  const auto color = read_color_index(frame, 10U, 20U);

  check(color == 0x05U,
        "render should start from the decoded bezel canvas when available");
}

void test_render_bootstrap_frame_falls_back_without_bezel_canvas() {
  auto state = comic2::make_default_runtime_state();
  state.bezel_frame.reset();
  state.room_grid.tile_w = 0;
  state.room_grid.tile_h = 0;

  comic2::MemoryFramePresenter presenter;
  comic2::render_bootstrap_frame(presenter, state);

  check(presenter.has_frame(), "fallback render should still present a frame");
  const auto &frame = presenter.last_frame();
  const auto color = read_color_index(frame, 0U, 0U);

  check(color == 0x0FU,
        "render should keep the procedural fallback background when bezel data "
        "is unavailable");
}

void test_render_bootstrap_frame_renders_scrolled_right_edge_tiles() {
  auto state = comic2::make_default_runtime_state();
  state.player.x = 300;
  state.player.y = 180;
  state.room_grid.tile_w = 30;
  state.room_grid.tile_h = 1;
  state.room_grid.row_pointers = {0};
  state.room_grid.tile_data.assign(30, 0x00);
  state.camera_x = 16;

  // Tile index 20 should appear at the right edge when camera_x is 16.
  state.room_grid.tile_data[20] = 0x0A;

  comic2::MemoryFramePresenter presenter;
  comic2::render_bootstrap_frame(presenter, state);

  check(presenter.has_frame(), "render should present a scrolled frame");
  const auto &frame = presenter.last_frame();
  const auto right_edge_center = read_color_index(frame, 312, 8);

  check(right_edge_center == 0x0A, "camera-aligned tile iteration should "
                                   "render the right-edge tile when scrolled");
}

void test_render_bootstrap_frame_renders_active_runtime_entities() {
  auto state_without_entities = comic2::make_default_runtime_state();
  state_without_entities.player.x = 128;
  state_without_entities.player.y = 128;
  state_without_entities.room_grid.tile_w = 0;
  state_without_entities.room_grid.tile_h = 0;
  state_without_entities.runtime_slots.clear();

  auto state_with_entities = state_without_entities;
  state_with_entities.runtime_slots.resize(1);
  state_with_entities.runtime_slots[0].mapped_object_ptr = 1;
  state_with_entities.runtime_slots[0].behavior_state = 0x0004;
  state_with_entities.runtime_slots[0].x = 8;
  state_with_entities.runtime_slots[0].y = 8;

  comic2::MemoryFramePresenter without_entities;
  comic2::render_bootstrap_frame(without_entities, state_without_entities);

  comic2::MemoryFramePresenter with_entities;
  comic2::render_bootstrap_frame(with_entities, state_with_entities);

  const auto &frame_without = without_entities.last_frame();
  const auto &frame_with = with_entities.last_frame();
  const auto color_without = read_color_index(frame_without, 8, 8);
  const auto color_with = read_color_index(frame_with, 8, 8);

  check(color_with != color_without,
        "active runtime entities should change the rendered frame at their "
        "sprite position");
}

void test_render_bootstrap_frame_distinguishes_pickups_from_enemies() {
  auto state = comic2::make_default_runtime_state();
  state.player.x = 64;
  state.player.y = 64;
  state.room_grid.tile_w = 0;
  state.room_grid.tile_h = 0;

  auto background_state = state;
  background_state.runtime_slots.clear();

  auto enemy_state = state;
  enemy_state.runtime_slots.clear();
  enemy_state.runtime_slots.resize(1);
  enemy_state.runtime_slots[0].mapped_object_ptr = 1;
  enemy_state.runtime_slots[0].behavior_state = 0x0001;
  enemy_state.runtime_slots[0].x = 0;
  enemy_state.runtime_slots[0].y = 0;

  auto gem_state = state;
  gem_state.runtime_slots.clear();
  gem_state.runtime_slots.resize(1);
  gem_state.runtime_slots[0].mapped_object_ptr = 1;
  gem_state.runtime_slots[0].behavior_state = 0x0004;
  gem_state.runtime_slots[0].x = 0;
  gem_state.runtime_slots[0].y = 0;

  auto powerup_state = state;
  powerup_state.runtime_slots.clear();
  powerup_state.runtime_slots.resize(1);
  powerup_state.runtime_slots[0].mapped_object_ptr = 1;
  powerup_state.runtime_slots[0].behavior_state = 0x0005;
  powerup_state.runtime_slots[0].x = 0;
  powerup_state.runtime_slots[0].y = 0;

  comic2::MemoryFramePresenter background_presenter;
  comic2::render_bootstrap_frame(background_presenter, background_state);

  comic2::MemoryFramePresenter enemy_presenter;
  comic2::render_bootstrap_frame(enemy_presenter, enemy_state);

  comic2::MemoryFramePresenter gem_presenter;
  comic2::render_bootstrap_frame(gem_presenter, gem_state);

  comic2::MemoryFramePresenter powerup_presenter;
  comic2::render_bootstrap_frame(powerup_presenter, powerup_state);

  const auto background_color =
      read_color_index(background_presenter.last_frame(), 8, 4);
  const auto enemy_sample =
      read_color_index(enemy_presenter.last_frame(), 8, 4);
  const auto gem_sample = read_color_index(gem_presenter.last_frame(), 8, 4);
  const auto powerup_sample =
      read_color_index(powerup_presenter.last_frame(), 7, 4);

  check(enemy_sample != background_color,
        "enemy placeholder sprites should change a sample pixel in the sprite");
  check(gem_sample != background_color,
        "gem pickups should render a visible compact sprite");
  check(powerup_sample != gem_sample,
        "powerups should render with a distinct pickup pattern from gems");
}

void test_render_bootstrap_frame_renders_active_projectiles() {
  auto state_without_projectiles = comic2::make_default_runtime_state();
  state_without_projectiles.player.x = 64;
  state_without_projectiles.player.y = 64;
  state_without_projectiles.room_grid.tile_w = 0;
  state_without_projectiles.room_grid.tile_h = 0;

  auto state_with_active_projectile = state_without_projectiles;
  state_with_active_projectile.projectiles.push_back(
      comic2::ProjectileState{.x = 8, .y = 8, .active = true});

  auto state_with_inactive_projectile = state_without_projectiles;
  state_with_inactive_projectile.projectiles.push_back(
      comic2::ProjectileState{.x = 8, .y = 8, .active = false});

  comic2::MemoryFramePresenter without_projectiles;
  comic2::render_bootstrap_frame(without_projectiles,
                                 state_without_projectiles);

  comic2::MemoryFramePresenter with_active_projectile;
  comic2::render_bootstrap_frame(with_active_projectile,
                                 state_with_active_projectile);

  comic2::MemoryFramePresenter with_inactive_projectile;
  comic2::render_bootstrap_frame(with_inactive_projectile,
                                 state_with_inactive_projectile);

  const auto &frame_without = without_projectiles.last_frame();
  const auto &frame_with_active = with_active_projectile.last_frame();
  const auto &frame_with_inactive = with_inactive_projectile.last_frame();

  const auto color_without = read_color_index(frame_without, 8, 8);
  const auto color_with_active = read_color_index(frame_with_active, 8, 8);
  const auto color_with_inactive = read_color_index(frame_with_inactive, 8, 8);

  check(color_with_active != color_without,
        "active projectiles should change the rendered frame at their sprite "
        "position");
  check(color_with_inactive == color_without,
        "inactive projectiles should not alter the rendered frame");
}

void test_render_bootstrap_frame_hides_player_on_invuln_blink_frames() {
  auto state = comic2::make_default_runtime_state();
  state.player.x = 0;
  state.player.y = 0;
  state.player.invuln_ticks = 0;

  comic2::MemoryFramePresenter visible_presenter;
  comic2::render_bootstrap_frame(visible_presenter, state);
  const auto visible_color =
      read_color_index(visible_presenter.last_frame(), 0, 0);

  state.player.invuln_ticks = 3;
  comic2::MemoryFramePresenter hidden_presenter;
  comic2::render_bootstrap_frame(hidden_presenter, state);
  const auto hidden_color =
      read_color_index(hidden_presenter.last_frame(), 0, 0);

  check(
      visible_color != hidden_color,
      "render should skip the player marker on hidden invulnerability frames");
}

void test_render_bootstrap_frame_renders_timed_overlay() {
  auto state = comic2::make_default_runtime_state();
  state.player.x = 12;
  state.player.y = 12;
  state.player.overlay_active = true;
  state.player.overlay_ticks = 6;
  state.player.overlay_sprite_frame = 2;

  comic2::MemoryFramePresenter presenter;
  comic2::render_bootstrap_frame(presenter, state);

  const auto overlay_color =
      read_color_index(presenter.last_frame(), 12 + 4, 12 + 4);
  check(overlay_color != 0x00,
        "timed overlay should render a visible sprite over the player");
}

void test_render_bootstrap_frame_renders_room_event_anchor_sprite() {
  auto state_without_anchor = comic2::make_default_runtime_state();
  state_without_anchor.player.x = 200;
  state_without_anchor.player.y = 150;

  auto state_with_anchor = state_without_anchor;
  state_with_anchor.room_event_anchor.active = true;
  state_with_anchor.room_event_anchor.x = 16;
  state_with_anchor.room_event_anchor.y = 16;

  comic2::MemoryFramePresenter without_anchor;
  comic2::render_bootstrap_frame(without_anchor, state_without_anchor);

  comic2::MemoryFramePresenter with_anchor;
  comic2::render_bootstrap_frame(with_anchor, state_with_anchor);

  const auto color_without =
      read_color_index(without_anchor.last_frame(), 16, 16);
  const auto color_with = read_color_index(with_anchor.last_frame(), 16, 16);

  check(color_with != color_without,
        "room-event anchors should render a visible placeholder sprite");
}

void test_render_bootstrap_frame_asset_backed_hash_regression() {
  auto state = comic2::make_default_runtime_state();
  state.player.x = 16;
  state.player.y = 16;
  state.player.hp = 1;

  state.room_grid.tile_w = 2;
  state.room_grid.tile_h = 1;
  state.room_grid.row_pointers = {0};
  state.room_grid.tile_data = {0x00, 0x01};

  comic2::FrpakCatalogFile file;
  file.pak_id = 1;
  file.file_size = 1;
  file.blob_offset = 0;
  file.records.push_back(comic2::FrpakCatalogRecord{
      .pak_id = 1,
      .record_id = 0,
      .data_offset = 0,
      .data_size = 1,
      .row_span_bytes = 0x1F40,
  });
  state.frpak_catalog.files.push_back(file);
  state.frpak_decode_cache.push_back(comic2::FrpakDecodedRecordCacheEntry{
      .pak_id = 1,
      .record_id = 0,
      .image = make_asset_fixture_image(),
  });

  comic2::MemoryFramePresenter presenter;
  comic2::render_bootstrap_frame(presenter, state);

  check(presenter.has_frame(),
        "asset-backed bootstrap render should present a frame");
  const auto hash =
      comic2::validation::hash_surface_planes(presenter.last_frame());
  constexpr std::uint64_t kExpectedHash = 0x1f693466a2ef8a45ULL;
  check(hash == kExpectedHash,
        "asset-backed bootstrap frame hash regression mismatch");
}

void test_render_bootstrap_frame_falls_back_when_asset_decode_fails() {
  auto state = comic2::make_default_runtime_state();
  state.player.x = 300;
  state.player.y = 180;
  state.room_grid.tile_w = 1;
  state.room_grid.tile_h = 1;
  state.room_grid.row_pointers = {0};
  state.room_grid.tile_data = {0x02};

  comic2::FrpakCatalogFile file;
  file.pak_id = 1;
  file.file_size = 128;
  file.blob_offset = 5000;
  file.records.push_back(comic2::FrpakCatalogRecord{
      .pak_id = 1,
      .record_id = 0,
      .data_offset = 0,
      .data_size = 128,
      .row_span_bytes = 0x1F40,
  });
  state.frpak_catalog.files.push_back(file);

  comic2::MemoryFramePresenter presenter;
  comic2::render_bootstrap_frame(presenter, state);

  check(presenter.has_frame(), "fallback render should still present a frame");
  const auto &frame = presenter.last_frame();
  check(read_color_index(frame, 8, 8) == 0x02,
        "render should fall back to procedural tile rendering when asset "
        "decode fails");
}

void test_bootstrap_loader_reads_reference_room_data() {
  const std::optional<std::filesystem::path> reference_root =
      find_reference_assets_root();
  if (!reference_root.has_value()) {
    if (require_original_assets()) {
      throw std::runtime_error(
          "COMIC2_REQUIRE_ORIGINAL_ASSETS is set, but reference/original "
          "assets containing FRDATA.* were not found");
    }
    std::cerr << "Skipping FRDATA bootstrap load test: reference/original "
                 "assets are not present.\n";
    return;
  }

  auto state = comic2::make_default_runtime_state();

  const auto summary =
      comic2::load_initial_bootstrap_resources(state, reference_root.value());

  check(summary.room_grid_loaded, "bootstrap loader should load the reference "
                                  "room grid from FRDATA assets");
  check(state.room_grid.tile_w > 0 && state.room_grid.tile_h > 0,
        "bootstrap loader should populate the room grid dimensions");
}

void test_scene_bootstrap_discovers_reference_assets_from_repo_root() {
  const std::optional<std::filesystem::path> reference_root =
      find_reference_assets_root();
  if (!reference_root.has_value()) {
    if (require_original_assets()) {
      throw std::runtime_error(
          "COMIC2_REQUIRE_ORIGINAL_ASSETS is set, but reference/original "
          "assets containing FRDATA.* were not found");
    }
    std::cerr << "Skipping scene bootstrap discovery test: reference/original "
                 "assets are not present.\n";
    return;
  }

  const std::filesystem::path repo_root =
      reference_root->parent_path().parent_path();
  auto state = comic2::make_default_runtime_state();
  state.player.x = 10000;
  state.player.y = 10000;

  const auto summary = comic2::initialize_runtime_scene(state, repo_root);

  check(summary.room_grid_loaded,
        "scene bootstrap should load room data from discovered asset roots");
  check(!summary.using_placeholder,
        "scene bootstrap should not use placeholder mode when assets exist");
  check(!summary.assets_root_used.empty(),
        "scene bootstrap should report the selected assets root");
  check(state.assets_root == summary.assets_root_used,
        "scene bootstrap should persist selected assets root into runtime "
        "state");
  check(state.room_grid.tile_w > 0 && state.room_grid.tile_h > 0,
        "scene bootstrap should populate room grid dimensions");
  check(state.player.x >= 0 && state.player.x <= 319,
        "scene bootstrap should clamp player x into visible bounds");
  check(state.player.y >= 0 && state.player.y <= 199,
        "scene bootstrap should clamp player y into visible bounds");
}

void test_scene_bootstrap_does_not_commit_failed_candidate_state() {
  const auto scratch_root =
      std::filesystem::temp_directory_path() / "comic2_scene_bootstrap_scratch";
  std::filesystem::remove_all(scratch_root);
  std::filesystem::create_directories(scratch_root);

  const std::vector<std::uint8_t> failing_sprite_bytes = {0xDE, 0xAD, 0xBE,
                                                          0xEF};
  {
    std::ofstream output(scratch_root / "FRPAK.001", std::ios::binary);
    output.write(reinterpret_cast<const char *>(failing_sprite_bytes.data()),
                 static_cast<std::streamsize>(failing_sprite_bytes.size()));
  }

  auto state = comic2::make_default_runtime_state();
  const auto summary = comic2::initialize_runtime_scene(state, scratch_root);

  check(!summary.room_grid_loaded,
        "scene bootstrap should remain in placeholder mode when no candidate "
        "succeeds");
  check(summary.using_placeholder,
        "scene bootstrap should explicitly mark placeholder fallback mode");
  check(
      state.sprite_resource_bytes.empty(),
      "failed candidate sprite bytes should not be committed to runtime state");
  check(state.level_metadata_bytes.empty(),
        "failed candidate metadata bytes should not be committed to runtime "
        "state");
  check(state.room_resource_bytes.empty(),
        "failed candidate room bytes should not be committed to runtime state");

  std::filesystem::remove_all(scratch_root);
}

void test_scene_bootstrap_falls_back_with_missing_assets() {
  const auto empty_root =
      std::filesystem::temp_directory_path() / "comic2_scene_bootstrap_empty";
  std::filesystem::remove_all(empty_root);
  std::filesystem::create_directories(empty_root);

  auto state = comic2::make_default_runtime_state();
  const auto summary = comic2::initialize_runtime_scene(state, empty_root);

  check(!summary.room_grid_loaded,
        "scene bootstrap should not report room load success without assets");
  check(summary.using_placeholder,
        "scene bootstrap should explicitly mark placeholder fallback mode");
  check(state.player.is_physics_active,
        "scene bootstrap fallback should keep physics stage active");

  std::filesystem::remove_all(empty_root);
}

} // namespace

void run_bootstrap_wiring_tests() {
  test_bootstrap_entry_runs_without_crashing();
  test_bootstrap_tick_wires_input_dispatch_and_render();
  test_bootstrap_frame_interval_defaults_to_original_tick_rate();
  test_bootstrap_frame_interval_can_be_overridden();
  test_scene_bootstrap_discovers_reference_assets_from_repo_subdirectory();
  test_integrated_loop_exits_cleanly_when_quit_is_requested();
  test_dispatcher_moves_player_when_left_input_is_active();
  test_render_loop_renders_multiple_frames();
  test_render_loop_updates_state_while_presenting_frames();
  test_render_bootstrap_frame_uses_room_tile_data();
  test_render_bootstrap_frame_uses_level_tileset_bytes_when_available();
  test_render_bootstrap_frame_renders_scrolled_right_edge_tiles();
  test_render_bootstrap_frame_uses_bezel_canvas_when_available();
  test_render_bootstrap_frame_falls_back_without_bezel_canvas();
  test_render_bootstrap_frame_renders_active_runtime_entities();
  test_render_bootstrap_frame_distinguishes_pickups_from_enemies();
  test_render_bootstrap_frame_renders_active_projectiles();
  test_render_bootstrap_frame_hides_player_on_invuln_blink_frames();
  test_render_bootstrap_frame_renders_timed_overlay();
  test_render_bootstrap_frame_renders_room_event_anchor_sprite();
  test_render_bootstrap_frame_asset_backed_hash_regression();
  test_render_bootstrap_frame_falls_back_when_asset_decode_fails();
  test_bootstrap_loader_reads_reference_room_data();
  test_scene_bootstrap_discovers_reference_assets_from_repo_root();
  test_scene_bootstrap_does_not_commit_failed_candidate_state();
  test_scene_bootstrap_falls_back_with_missing_assets();
}
