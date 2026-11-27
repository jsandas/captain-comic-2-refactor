#include <chrono>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>

#include "comic2/audio.hpp"
#include "comic2/bootstrap.hpp"
#include "comic2/default_handlers.hpp"
#include "comic2/oracle.hpp"
#include "comic2/renderer.hpp"
#include "comic2/resource_loader.hpp"

namespace {

bool is_smoke_test_arg(int argc, char **argv) {
  for (int i = 1; i < argc; ++i) {
    if (std::string(argv[i]) == "--smoke-test") {
      return true;
    }
  }
  return false;
}

bool is_replay_arg(int argc, char **argv, std::string &path) {
  for (int i = 1; i < argc; ++i) {
    if (std::string(argv[i]) == "--replay" && i + 1 < argc) {
      path = argv[i + 1];
      return true;
    }
  }
  return false;
}

} // namespace

int main(int argc, char **argv) {
  try {
    const std::filesystem::path root = (argc > 1)
                                           ? std::filesystem::path(argv[1])
                                           : std::filesystem::current_path();

#ifdef COMIC2_USE_SDL2
    comic2::Sdl2FramePresenter presenter(640, 400);
#else
    comic2::MemoryFramePresenter presenter;
#endif
    auto audio_backend = comic2::make_default_audio_backend();

    if (is_smoke_test_arg(argc, argv)) {
      comic2::RuntimeState state = comic2::make_default_runtime_state();
      comic2::GameDispatcher dispatcher =
          comic2::make_default_game_dispatcher();
      std::vector<comic2::InputState> inputs(4);
      inputs[0].right_pressed = true;
      inputs[1].right_pressed = true;
      inputs[2].jump_pressed = true;
      inputs[3].down_pressed = true;
      const auto snapshots =
          comic2::capture_runtime_snapshots(state, dispatcher, inputs);
      std::cout << "smoke-test snapshots=" << snapshots.size() << "\n";
      return 0;
    }

    std::string replay_path;
    if (is_replay_arg(argc, argv, replay_path)) {
      std::ifstream replay_stream(replay_path);
      if (!replay_stream) {
        std::cerr << "oracle replay file not found: " << replay_path << "\n";
        return 2;
      }
      std::ostringstream replay_buffer;
      replay_buffer << replay_stream.rdbuf();
      const auto parsed = comic2::parse_replay_log(replay_buffer.str());
      std::cout << "oracle replay parsed_snapshots=" << parsed.size() << "\n";
      return 0;
    }

    // Run until user quits (frame budget is effectively unbounded)
    const auto frame_interval = comic2::read_bootstrap_frame_interval();
    const auto loop_summary = comic2::run_integrated_bootstrap_loop(
        root, presenter, std::numeric_limits<int>::max(), frame_interval,
        audio_backend.get());

    std::cout << "render loop complete: frames_rendered="
              << loop_summary.loop.frames_rendered
              << " ticks_processed=" << loop_summary.loop.ticks_processed
              << " last_stage="
              << comic2::to_string(loop_summary.loop.last_stage)
              << " quit_requested=" << std::boolalpha
              << loop_summary.loop.quit_requested << std::noboolalpha
              << " avg_frame_us=" << loop_summary.loop.average_frame_time_us
              << " max_frame_us=" << loop_summary.loop.max_frame_time_us
              << " est_state_bytes=" << loop_summary.loop.estimated_state_bytes
              << "\n";
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << "error: " << ex.what() << "\n";
    return 1;
  }
}
