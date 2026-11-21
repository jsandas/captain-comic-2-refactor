#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <stdexcept>
#include <vector>

#ifdef _WIN32
#include <process.h>
#endif

#include "comic2/audio.hpp"
#include "comic2/bootstrap.hpp"
#include "comic2/default_handlers.hpp"
#include "comic2/renderer.hpp"

namespace {

void set_test_env(const char *name, const char *value, int overwrite) {
#ifdef _WIN32
  (void)overwrite;
  _putenv_s(name, value);
#else
  setenv(name, value, overwrite);
#endif
}

void expect(bool condition, const char *message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

class RecordingAudioBackend final : public comic2::IAudioBackend {
public:
  bool initialize() override {
    ++initialize_calls;
    initialized = true;
    return true;
  }

  void enqueue_event(comic2::AudioEvent event) override {
    events.push_back(event);
  }

  void update() override { ++update_calls; }

  void shutdown() override {
    ++shutdown_calls;
    initialized = false;
  }

  bool is_available() const override { return initialized; }

  std::size_t events_enqueued() const override { return events.size(); }

  bool initialized = false;
  int initialize_calls = 0;
  int update_calls = 0;
  int shutdown_calls = 0;
  std::vector<comic2::AudioEvent> events;
};

class FailingAudioBackend final : public comic2::IAudioBackend {
public:
  bool initialize() override {
    ++initialize_calls;
    return false;
  }

  void enqueue_event(comic2::AudioEvent) override { ++enqueue_calls; }

  void update() override { ++update_calls; }

  void shutdown() override { ++shutdown_calls; }

  bool is_available() const override { return false; }

  std::size_t events_enqueued() const override { return 0; }

  int initialize_calls = 0;
  int enqueue_calls = 0;
  int update_calls = 0;
  int shutdown_calls = 0;
};

void test_run_render_loop_audio_lifecycle_and_startup_event() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  comic2::GameDispatcher dispatcher = comic2::make_default_game_dispatcher();
  comic2::MemoryFramePresenter presenter;
  RecordingAudioBackend audio;

  const auto summary = comic2::run_render_loop(
      state, dispatcher, presenter, 2, std::chrono::milliseconds(0), &audio);

  expect(summary.frames_rendered == 2,
         "render loop should process requested frames with audio attached");
  expect(audio.initialize_calls == 1,
         "audio backend should be initialized once per loop invocation");
  expect(audio.shutdown_calls == 1,
         "audio backend should be shut down once per loop invocation");
  expect(audio.update_calls == 2,
         "audio backend should receive one non-blocking update per frame");
  expect(!audio.events.empty(),
         "audio backend should receive at least a startup event");
  expect(audio.events.front() == comic2::AudioEvent::StartupChime,
         "first enqueued audio event should be the startup chime");
}

void test_run_render_loop_emits_jump_event_on_takeoff() {
  set_test_env("COMIC2_FORCE_ENV_INPUT", "1", 1);
  set_test_env("COMIC2_INPUT_JUMP", "1", 1);
  set_test_env("COMIC2_INPUT_LEFT", "0", 1);
  set_test_env("COMIC2_INPUT_RIGHT", "0", 1);
  set_test_env("COMIC2_INPUT_DOWN", "0", 1);

  struct EnvCleanup {
    ~EnvCleanup() {
      set_test_env("COMIC2_FORCE_ENV_INPUT", "0", 1);
      set_test_env("COMIC2_INPUT_JUMP", "0", 1);
      set_test_env("COMIC2_INPUT_LEFT", "0", 1);
      set_test_env("COMIC2_INPUT_RIGHT", "0", 1);
      set_test_env("COMIC2_INPUT_DOWN", "0", 1);
    }
  } env_cleanup;
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.player.is_physics_active = false;
  state.player.is_airborne = false;
  state.player.jump_counter = 1;

  comic2::GameDispatcher dispatcher = comic2::make_default_game_dispatcher();
  comic2::MemoryFramePresenter presenter;
  RecordingAudioBackend audio;

  const auto summary = comic2::run_render_loop(
      state, dispatcher, presenter, 1, std::chrono::milliseconds(0), &audio);

  expect(summary.frames_rendered == 1,
         "render loop should process one frame for jump trigger test");

  const bool found_jump = std::any_of(
      audio.events.begin(), audio.events.end(),
      [](const auto event) { return event == comic2::AudioEvent::Jump; });

  expect(
      found_jump,
      "audio backend should receive jump event when player becomes airborne");
}

void test_run_render_loop_skips_audio_when_initialization_fails() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  comic2::GameDispatcher dispatcher = comic2::make_default_game_dispatcher();
  comic2::MemoryFramePresenter presenter;
  FailingAudioBackend audio;

  const auto summary = comic2::run_render_loop(
      state, dispatcher, presenter, 1, std::chrono::milliseconds(0), &audio);

  expect(summary.frames_rendered == 1,
         "render loop should still process frames when audio initialization "
         "fails");
  expect(audio.initialize_calls == 1,
         "audio backend should still receive one initialization attempt");
  expect(audio.enqueue_calls == 0,
         "audio events should not be enqueued when initialization fails");
  expect(audio.update_calls == 0,
         "audio backend update should not run when initialization fails");
  expect(audio.shutdown_calls == 0,
         "audio backend shutdown should not run when initialization fails");
}

void test_sound_command_parser_and_synthesizer_work() {
  comic2::SoundPlaybackState state{};
  const auto play_result = comic2::handle_sound_command(
      state, comic2::SoundCommand::Play, 0x00DB, 0x0000, 0x02);

  expect(play_result.playing,
         "sound command parser should mark playback active for play requests");
  expect(play_result.priority == 0x02,
         "sound command parser should preserve requested priority");
  expect(state.is_playing,
         "sound playback state should activate after a play command");

  const auto samples = comic2::synthesize_sound_samples(
      comic2::make_stream_for_event(comic2::AudioEvent::Jump), 22050, 8);

  expect(!samples.empty(),
         "synthesizer should produce audio samples for jump events");
  expect(samples.front() != 0 || samples.back() != 0,
         "synthesizer should output non-zero waveform data");

  comic2::advance_sound_stream(state, 22050);
  expect(state.tick_remaining > 0,
         "sound playback should advance into its first note duration");
}

void test_null_audio_backend_accepts_events() {
  comic2::NullAudioBackend null_audio;
  expect(null_audio.initialize(),
         "null audio backend initialize should always succeed");
  expect(!null_audio.is_available(),
         "null audio backend should remain unavailable by design");

  null_audio.enqueue_event(comic2::AudioEvent::StartupChime);
  null_audio.enqueue_event(comic2::AudioEvent::Hazard);
  null_audio.update();
  null_audio.shutdown();

  expect(null_audio.events_enqueued() == 2,
         "null backend should still track enqueued events for diagnostics");
}

void test_authentic_56_sound_table_and_continuous_18_2hz_stepping() {
  expect(comic2::get_authentic_sound_table_size() ==
             comic2::kAuthenticSoundStreamCount,
         "sound table must contain all 56 authentic sound streams");

  // Verify every entry in the 56-stream table has a valid offset, non-zero
  // initial divisor, and (0, 0) terminator.
  for (std::size_t i = 0; i < comic2::kAuthenticSoundStreamCount; ++i) {
    const auto offset = comic2::get_authentic_sound_offset_by_index(i);
    const auto stream = comic2::get_authentic_sound_stream_by_index(i);
    expect(offset != 0U, "authentic sound entry offset must be non-zero");
    expect(stream.words.size() >= 4U && (stream.words.size() % 2U) == 0U,
           "authentic sound stream must contain divisor/duration pairs");
    expect(stream.words[0] != 0U && stream.words[1] != 0U,
           "authentic sound stream must start with a valid note");
    expect(stream.words[stream.words.size() - 2] == 0U &&
               stream.words[stream.words.size() - 1] == 0U,
           "authentic sound stream must end with (0, 0) terminator");

    const auto by_offset = comic2::make_stream_from_offset(offset);
    expect(by_offset.words == stream.words,
           "make_stream_from_offset must return exact stream words");
  }

  // Verify specific canonical streams from comic2_data.asm:
  // - DS:0x00DB (Title/Intro theme, 68 pairs = 136 words)
  // - DS:0x9D3F (Main menu melody at data_0:0x2174F, 72 pairs = 144 words)
  // - DS:0x969A (Player death at data_0:0x210AA, 5 pairs = 10 words, prio 10)
  // - DS:0x98E0 (Item pickup at data_0:0x212F0, 5 pairs = 10 words, prio 3)
  const auto title_stream =
      comic2::make_stream_from_offset(comic2::kSoundOffsetTitleTheme);
  expect(title_stream.words.size() == 136U &&
             title_stream.words[0] == 0x1FBEu &&
             title_stream.words[1] == 0x0003u,
         "DS:0x00DB title stream must match comic2_data.asm bytes");

  const auto menu_stream =
      comic2::make_stream_from_offset(comic2::kSoundOffsetMainMenuTheme);
  expect(menu_stream.words.size() == 144U && menu_stream.words[0] == 0x1C48u &&
             menu_stream.words[1] == 0x0002u,
         "DS:0x9D3F main menu stream must match comic2_data.asm bytes");

  // Verify exact duration-tick stepping through SoundDriver::tick() for
  // multi-note stream DS:0x98E0 (ItemPickup: 4 notes of duration 2 ticks each)
  const auto pickup_stream =
      comic2::make_stream_from_offset(comic2::kSoundOffsetItemPickup);
  expect(pickup_stream.words.size() == 10U && pickup_stream.priority == 3U,
         "DS:0x98E0 item pickup stream must have 4 notes + terminator");

  comic2::SoundDriver driver;
  driver.play(pickup_stream, pickup_stream.priority);
  expect(driver.is_playing(), "driver should be playing pickup stream");

  const std::uint16_t expected_divisors[8] = {
      0x0FDAu, 0x0FDAu, 0x0C90u, 0x0C90u, 0x0A91u, 0x0A91u, 0x0800u, 0x0800u,
  };
  for (int t = 0; t < 8; ++t) {
    const auto div = driver.tick();
    expect(div == expected_divisors[t],
           "SoundDriver::tick must hold each divisor for its exact tick count");
  }
  expect(driver.tick() == 0U && !driver.is_playing(),
         "SoundDriver::tick must stop cleanly upon reaching (0, 0) terminator");

  // Verify priority preemption with authentic streams:
  // Start MenuTheme (priority 1), reject Shoot (priority 1 allows
  // same-or-higher replacement, JumpStart priority 0 is rejected), and preempt
  // with PlayerDeath (priority 10).
  driver.play(menu_stream, menu_stream.priority);
  expect(driver.current_priority() == 1U && driver.current_divisor() == 0x1C48u,
         "MenuTheme should start at divisor 0x1C48 with priority 1");
  const auto jump_stream =
      comic2::make_stream_from_offset(comic2::kSoundOffsetJumpStart);
  driver.play(jump_stream, jump_stream.priority);
  expect(driver.current_priority() == 1U && driver.current_divisor() == 0x1C48u,
         "priority-0 JumpStart must not preempt priority-1 MenuTheme");

  const auto death_stream =
      comic2::make_stream_from_offset(comic2::kSoundOffsetPlayerDeath);
  driver.play(death_stream, death_stream.priority);
  expect(driver.current_priority() == 10U &&
             driver.current_divisor() == 0x23DAu,
         "priority-10 PlayerDeath must preempt MenuTheme");
}

} // namespace

void run_audio_backend_tests() {
  test_run_render_loop_audio_lifecycle_and_startup_event();
  test_run_render_loop_emits_jump_event_on_takeoff();
  test_run_render_loop_skips_audio_when_initialization_fails();
  test_sound_command_parser_and_synthesizer_work();
  test_null_audio_backend_accepts_events();
  test_authentic_56_sound_table_and_continuous_18_2hz_stepping();
}
