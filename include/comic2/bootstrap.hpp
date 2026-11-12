#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>

#include "comic2/audio.hpp"
#include "comic2/dispatcher.hpp"
#include "comic2/game_state.hpp"
#include "comic2/renderer.hpp"

namespace comic2 {

struct BootstrapTickSummary {
  DispatchStage stage = DispatchStage::InputHandling;
  bool input_captured = false;
  bool frame_presented = false;
};

struct FrameLoopSummary {
  int frames_rendered = 0;
  int ticks_processed = 0;
  DispatchStage last_stage = DispatchStage::InputHandling;
  bool quit_requested = false;
  std::uint64_t total_frame_time_us = 0;
  std::uint64_t average_frame_time_us = 0;
  std::uint64_t max_frame_time_us = 0;
  std::size_t estimated_state_bytes = 0;
};

struct SceneBootstrapSummary {
  std::size_t metadata_files_tried = 0;
  std::size_t sprite_files_tried = 0;
  bool room_grid_loaded = false;
  bool using_placeholder = true;
  std::filesystem::path assets_root_used;
};

struct IntegratedLoopSummary {
  SceneBootstrapSummary bootstrap;
  FrameLoopSummary loop;
};

bool read_bootstrap_bool_env(const char *name);
int read_bootstrap_tick_budget(int default_ticks = 2);
std::chrono::milliseconds
read_bootstrap_frame_interval(std::chrono::milliseconds default_interval =
                                  std::chrono::milliseconds(110));
std::size_t select_player_sprite_frame(const RuntimeState &state);
bool should_render_player_sprite(const RuntimeState &state);
bool should_render_timed_overlay(const RuntimeState &state);
SceneBootstrapSummary
initialize_runtime_scene(RuntimeState &state,
                         const std::filesystem::path &root);
int run_bootstrap_entry(const std::filesystem::path &root);
IntegratedLoopSummary run_integrated_bootstrap_loop(
    const std::filesystem::path &root, IFramePresenter &presenter,
    int frame_budget = 60,
    std::chrono::milliseconds frame_interval = std::chrono::milliseconds(0),
    IAudioBackend *audio_backend = nullptr);
FrameLoopSummary run_render_loop(
    RuntimeState &state, GameDispatcher &dispatcher, IFramePresenter &presenter,
    int frame_budget = 60,
    std::chrono::milliseconds frame_interval = std::chrono::milliseconds(0),
    IAudioBackend *audio_backend = nullptr);

bool poll_bootstrap_input(RuntimeState &state);
void render_bootstrap_surface(EgaPlanarSurface &frame, RuntimeState &state);
void render_bootstrap_frame(IFramePresenter &presenter, RuntimeState &state);
BootstrapTickSummary run_bootstrap_tick(RuntimeState &state,
                                        GameDispatcher &dispatcher,
                                        IFramePresenter &presenter);

} // namespace comic2
