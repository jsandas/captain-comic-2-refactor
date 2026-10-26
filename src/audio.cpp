#include "comic2/audio.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <vector>

#include "comic2/game_state.hpp"

#ifdef COMIC2_USE_SDL2
#if __has_include(<SDL2/SDL.h>)
#include <SDL2/SDL.h>
#elif __has_include(<SDL.h>)
#include <SDL.h>
#else
#error "SDL2 headers not found"
#endif
#endif

namespace comic2 {

namespace {

SoundStream make_stream_from_offset(std::uint16_t stream_offset) {
  SoundStream stream;
  stream.priority = 1;
  switch (stream_offset) {
  case 0x00DB:
    stream.words = {0x0300u, 0x0004u, 0x0280u, 0x0003u, 0x0000u, 0x0000u};
    break;
  case 0x965E:
    stream.words = {0x0200u, 0x0006u, 0x0100u, 0x0004u, 0x0000u, 0x0000u};
    break;
  case 0x9676:
    stream.words = {0x0180u, 0x0002u, 0x0200u, 0x0002u, 0x0000u, 0x0000u};
    break;
  case 0x96B6:
    stream.words = {0x0250u, 0x0004u, 0x01F0u, 0x0003u, 0x0000u, 0x0000u};
    break;
  default:
    stream.words = {0x01C0u, 0x0003u, 0x0000u, 0x0000u};
    break;
  }
  return stream;
}

bool read_env_bool(const char *name) {
  const char *value = std::getenv(name);
  return value != nullptr && (*value == '1' || *value == 't' || *value == 'T' ||
                              *value == 'y' || *value == 'Y');
}

[[maybe_unused]] SoundStream make_event_stream_impl(AudioEvent event) {
  switch (event) {
  case AudioEvent::StartupChime:
    return SoundStream{{0x0300u, 0x0004u, 0x0280u, 0x0003u, 0x0000u, 0x0000u},
                       1};
  case AudioEvent::Jump:
    return SoundStream{{0x0180u, 0x0002u, 0x0200u, 0x0002u, 0x0000u, 0x0000u},
                       1};
  case AudioEvent::Hazard:
    return SoundStream{{0x0200u, 0x0006u, 0x0100u, 0x0004u, 0x0000u, 0x0000u},
                       1};
  case AudioEvent::Shoot:
    return SoundStream{{0x01E0u, 0x0002u, 0x00C0u, 0x0001u, 0x0000u, 0x0000u},
                       1};
  case AudioEvent::Hit:
    return SoundStream{{0x0200u, 0x0003u, 0x0100u, 0x0002u, 0x0000u, 0x0000u},
                       1};
  case AudioEvent::Pickup:
    return SoundStream{{0x0400u, 0x0002u, 0x0500u, 0x0002u, 0x0000u, 0x0000u},
                       1};
  case AudioEvent::Death:
    return SoundStream{{0x00C0u, 0x0008u, 0x0080u, 0x0004u, 0x0000u, 0x0000u},
                       1};
  case AudioEvent::LevelStart:
    return SoundStream{{0x0250u, 0x0004u, 0x01F0u, 0x0003u, 0x0000u, 0x0000u},
                       1};
  }
  return SoundStream{{0x01C0u, 0x0003u, 0x0000u, 0x0000u}, 1};
}

#ifdef COMIC2_USE_SDL2
class Sdl2ToneAudioBackend final : public IAudioBackend {
public:
  bool initialize() override {
    if (available_) {
      return true;
    }
    if (read_env_bool("COMIC2_DISABLE_AUDIO")) {
      return false;
    }

    if (SDL_WasInit(SDL_INIT_AUDIO) == 0) {
      if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        return false;
      }
      owns_sdl_audio_subsystem_ = true;
    }

    SDL_AudioSpec desired{};
    desired.freq = 44100;
    desired.format = AUDIO_S16SYS;
    desired.channels = 1;
    desired.samples = 1024;
    desired.callback = nullptr;

    device_id_ = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained_spec_, 0);
    if (device_id_ == 0) {
      return false;
    }

    SDL_PauseAudioDevice(device_id_, 0);
    available_ = true;
    return true;
  }

  void enqueue_event(AudioEvent event) override {
    ++events_enqueued_;
    if (!available_ || device_id_ == 0) {
      return;
    }

    const SoundStream stream = make_event_stream_impl(event);
    sound_driver_.play(stream, stream.priority);
    const std::uint16_t divisor = sound_driver_.tick();
    const int sample_rate =
        obtained_spec_.freq > 0 ? obtained_spec_.freq : 44100;
    const int sample_count = std::max(1, sample_rate / 20);
    auto pcm = synthesize_pit_samples(divisor, 1U, sample_rate);
    if (pcm.empty()) {
      pcm = synthesize_sound_samples(stream, sample_rate, sample_count);
    }
    if (pcm.empty()) {
      return;
    }

    SDL_QueueAudio(device_id_, pcm.data(),
                   static_cast<Uint32>(pcm.size() * sizeof(std::int16_t)));
  }

  void update() override {
    if (!available_ || device_id_ == 0) {
      return;
    }

    // Keep queued data bounded to avoid unbounded audio backlog.
    const Uint32 queued = SDL_GetQueuedAudioSize(device_id_);
    const Uint32 max_queued_bytes =
        static_cast<Uint32>(obtained_spec_.freq * sizeof(std::int16_t));
    if (queued > max_queued_bytes) {
      SDL_ClearQueuedAudio(device_id_);
    }
  }

  void shutdown() override {
    if (device_id_ != 0) {
      SDL_CloseAudioDevice(device_id_);
      device_id_ = 0;
    }
    available_ = false;

    if (owns_sdl_audio_subsystem_) {
      SDL_QuitSubSystem(SDL_INIT_AUDIO);
      owns_sdl_audio_subsystem_ = false;
    }
  }

  bool is_available() const override { return available_; }
  std::size_t events_enqueued() const override { return events_enqueued_; }

  ~Sdl2ToneAudioBackend() override { shutdown(); }

private:
  bool available_ = false;
  bool owns_sdl_audio_subsystem_ = false;
  std::size_t events_enqueued_ = 0;
  SoundDriver sound_driver_;
  SDL_AudioDeviceID device_id_ = 0;
  SDL_AudioSpec obtained_spec_{};
};
#endif

} // namespace

AudioSynthesizer::AudioSynthesizer(int sample_rate)
    : sample_rate_(sample_rate > 0 ? sample_rate : 44100) {}

std::vector<std::int16_t> AudioSynthesizer::synthesize(
    const SoundStream &stream, int sample_count) const {
  if (sample_count <= 0 || stream.words.empty()) {
    return {};
  }

  const std::uint16_t divisor = stream.words.front();
  const std::uint16_t duration_ticks =
      stream.words.size() > 1 ? stream.words[1] : 1U;
  auto samples = synthesize_pit_samples(divisor, duration_ticks, sample_rate_);
  if (samples.empty()) {
    return {};
  }

  std::vector<std::int16_t> output(static_cast<std::size_t>(sample_count), 0);
  const int copy_count =
      std::min<int>(sample_count, static_cast<int>(samples.size()));
  std::copy_n(samples.begin(), copy_count, output.begin());
  return output;
}

std::vector<std::int16_t> AudioSynthesizer::synthesize(
    std::uint16_t divisor, std::uint16_t duration_ticks) const {
  return synthesize_pit_samples(divisor, duration_ticks, sample_rate_);
}

void SoundDriver::play(const SoundStream &stream, std::uint8_t priority) {
  if (playing_ && priority < priority_) {
    return;
  }

  stream_ = stream;
  priority_ = std::max<std::uint8_t>(priority, stream.priority);

  current_divisor_ = 0;
  tick_remaining_ = 0;
  stream_index_ = 0;
  playing_ = !stream_.words.empty();
  if (playing_) {
    load_next_pair();
  }
}

void SoundDriver::stop() {
  stream_ = SoundStream{};
  priority_ = 0;
  current_divisor_ = 0;
  tick_remaining_ = 0;
  stream_index_ = 0;
  playing_ = false;
}

std::uint16_t SoundDriver::tick() {
  if (!playing_) {
    current_divisor_ = 0;
    return 0;
  }

  if (tick_remaining_ == 0) {
    load_next_pair();
  }

  if (tick_remaining_ > 0) {
    --tick_remaining_;
  }

  return current_divisor_;
}

bool SoundDriver::is_playing() const { return playing_; }

std::uint8_t SoundDriver::current_priority() const { return priority_; }

std::uint16_t SoundDriver::current_divisor() const { return current_divisor_; }

void SoundDriver::load_next_pair() {
  if (!playing_ || stream_.words.empty()) {
    current_divisor_ = 0;
    playing_ = false;
    return;
  }

  if (stream_index_ >= stream_.words.size()) {
    stop();
    return;
  }

  const std::uint16_t divisor = stream_.words[stream_index_];
  const std::size_t duration_index = stream_index_ + 1U;
  const std::uint16_t duration_ticks =
      duration_index < stream_.words.size() ? stream_.words[duration_index] : 0U;
  stream_index_ = duration_index + 1U;

  if (divisor == 0U) {
    stop();
    return;
  }

  current_divisor_ = divisor;
  tick_remaining_ = duration_ticks == 0U ? 1U : duration_ticks;
}

std::vector<std::int16_t> synthesize_pit_samples(std::uint16_t divisor,
                                                 std::uint16_t duration_ticks,
                                                 int sample_rate) {
  if (sample_rate <= 0 || divisor == 0U) {
    return {};
  }

  const int frequency = static_cast<int>(1193180 / divisor);
  if (frequency <= 0) {
    return {};
  }

  const double cycle_samples = static_cast<double>(sample_rate) / frequency;
  const int cycle_length =
      std::max(1, static_cast<int>(std::lround(cycle_samples)));
  const int half_cycle =
      std::max(1, static_cast<int>(std::lround(cycle_samples / 2.0)));
  const int total_sample_count =
      std::max(1, static_cast<int>(std::lround(
                     static_cast<double>(frequency) *
                     std::max<std::uint16_t>(1U, duration_ticks) / 18.2)));

  std::vector<std::int16_t> samples(static_cast<std::size_t>(total_sample_count));
  constexpr int kAmplitude = 12000;

  for (int i = 0; i < total_sample_count; ++i) {
    const int phase = i % cycle_length;
    const bool high = phase < half_cycle;
    samples[static_cast<std::size_t>(i)] =
        high ? static_cast<std::int16_t>(kAmplitude)
             : static_cast<std::int16_t>(-kAmplitude);
  }

  return samples;
}

SoundPlaybackResult handle_sound_command(SoundPlaybackState &state,
                                         SoundCommand command,
                                         std::uint16_t stream_offset,
                                         std::uint16_t stream_segment,
                                         std::uint8_t priority) {
  SoundPlaybackResult result{.playing = state.is_playing,
                             .priority = state.priority};
  switch (command) {
  case SoundCommand::SetGateOn:
    state.gate_enabled = true;
    state.is_enabled = true;
    break;
  case SoundCommand::Play: {
    const bool replace = !state.is_playing || priority >= state.priority;
    if (replace) {
      state.is_playing = true;
      state.is_enabled = true;
      state.priority = priority;
      state.stream_offset = stream_offset;
      state.stream_segment = stream_segment;
      state.tick_remaining = 0;
      state.stream_index = 0;
      state.stream_words = make_stream_from_offset(stream_offset).words;
    }
    break;
  }
  case SoundCommand::SetGateOff:
    state.gate_enabled = false;
    state.is_enabled = false;
    break;
  case SoundCommand::Stop:
    state.is_playing = false;
    state.tick_remaining = 0;
    state.stream_index = 0;
    break;
  case SoundCommand::Query:
    break;
  }

  result.playing = state.is_playing && state.is_enabled;
  result.priority = state.priority;
  return result;
}

SoundStream make_stream_for_event(AudioEvent event) {
  return make_event_stream_impl(event);
}

std::vector<std::int16_t> synthesize_sound_samples(const SoundStream &stream,
                                                   int sample_rate,
                                                   int sample_count) {
  if (sample_count <= 0 || stream.words.empty()) {
    return {};
  }

  const std::uint16_t divisor = stream.words.front();
  const std::uint16_t duration_ticks =
      stream.words.size() > 1 ? stream.words[1] : 1U;
  const auto samples = synthesize_pit_samples(divisor, duration_ticks, sample_rate);
  if (samples.empty()) {
    return {};
  }

  std::vector<std::int16_t> output(static_cast<std::size_t>(sample_count), 0);
  const int copy_count =
      std::min<int>(sample_count, static_cast<int>(samples.size()));
  std::copy_n(samples.begin(), copy_count, output.begin());
  return output;
}

void advance_sound_stream(SoundPlaybackState &state, int sample_rate) {
  (void)sample_rate;
  if (!state.is_playing || !state.is_enabled) {
    return;
  }

  if (state.tick_remaining == 0) {
    if (state.stream_index + 1 >= state.stream_words.size()) {
      state.is_playing = false;
      state.tick_remaining = 0;
      return;
    }

    const std::uint16_t divisor = state.stream_words[state.stream_index];
    const std::uint16_t duration = state.stream_words[state.stream_index + 1];
    state.stream_index += 2;
    if (divisor == 0) {
      state.is_playing = false;
      state.tick_remaining = 0;
      return;
    }
    state.tick_remaining = duration == 0 ? 1 : duration;
    return;
  }

  if (state.tick_remaining > 0) {
    --state.tick_remaining;
  }
}

void queue_audio_event(RuntimeState &state, AudioEvent event) {
  if (state.audio.mute || !state.audio.audio_enabled) {
    return;
  }
  state.audio.pending_events.push_back(event);
}

void flush_audio_events(RuntimeState &state, IAudioBackend *backend) {
  if (backend == nullptr) {
    state.audio.pending_events.clear();
    return;
  }

  for (const auto event : state.audio.pending_events) {
    backend->enqueue_event(event);
  }
  state.audio.pending_events.clear();
}

bool NullAudioBackend::initialize() { return true; }

void NullAudioBackend::enqueue_event(AudioEvent event) {
  (void)event;
  ++events_enqueued_;
}

void NullAudioBackend::update() {}

void NullAudioBackend::shutdown() {}

bool NullAudioBackend::is_available() const { return false; }

std::size_t NullAudioBackend::events_enqueued() const {
  return events_enqueued_;
}

std::unique_ptr<IAudioBackend> make_default_audio_backend() {
#ifdef COMIC2_USE_SDL2
  auto sdl_backend = std::make_unique<Sdl2ToneAudioBackend>();
  if (sdl_backend->initialize()) {
    return sdl_backend;
  }
#endif
  auto null_backend = std::make_unique<NullAudioBackend>();
  null_backend->initialize();
  return null_backend;
}

} // namespace comic2
