#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace comic2 {

struct RuntimeState;

enum class AudioEvent {
  StartupChime,
  Jump,
  Hazard,
  Shoot,
  Hit,
  Pickup,
  Death,
  LevelStart,
};

enum class SoundCommand {
  SetGateOn = 0,
  Play = 1,
  SetGateOff = 2,
  Stop = 3,
  Query = 4,
};

struct SoundStream {
  std::vector<std::uint16_t> words;
  std::uint8_t priority = 0;
};

struct SoundPlaybackState {
  bool is_playing = false;
  bool is_enabled = true;
  bool gate_enabled = true;
  std::uint8_t priority = 0;
  std::uint16_t stream_offset = 0;
  std::uint16_t stream_segment = 0;
  std::uint16_t tick_remaining = 0;
  std::size_t stream_index = 0;
  std::vector<std::uint16_t> stream_words;
};

struct SoundPlaybackResult {
  bool playing = false;
  std::uint8_t priority = 0;
};

struct AudioDispatchState {
  std::vector<AudioEvent> pending_events;
  SoundPlaybackState sound;
  bool mute = false;
  bool audio_enabled = true;
};

class AudioSynthesizer {
public:
  explicit AudioSynthesizer(int sample_rate = 44100);

  std::vector<std::int16_t> synthesize(const SoundStream &stream,
                                       int sample_count) const;
  std::vector<std::int16_t> synthesize(std::uint16_t divisor,
                                       std::uint16_t duration_ticks) const;

private:
  int sample_rate_;
};

class SoundDriver {
public:
  void play(const SoundStream &stream, std::uint8_t priority);
  void stop();
  std::uint16_t tick();
  bool is_playing() const;
  std::uint8_t current_priority() const;
  std::uint16_t current_divisor() const;

private:
  void load_next_pair();

  SoundStream stream_;
  std::uint8_t priority_ = 0;
  std::uint16_t current_divisor_ = 0;
  std::uint16_t tick_remaining_ = 0;
  std::size_t stream_index_ = 0;
  bool playing_ = false;
};

class IAudioBackend {
public:
  virtual ~IAudioBackend() = default;

  virtual bool initialize() = 0;
  virtual void enqueue_event(AudioEvent event) = 0;
  virtual void update() = 0;
  virtual void shutdown() = 0;

  virtual bool is_available() const = 0;
  virtual std::size_t events_enqueued() const = 0;
};

class NullAudioBackend final : public IAudioBackend {
public:
  bool initialize() override;
  void enqueue_event(AudioEvent event) override;
  void update() override;
  void shutdown() override;

  bool is_available() const override;
  std::size_t events_enqueued() const override;

private:
  std::size_t events_enqueued_ = 0;
};

SoundPlaybackResult handle_sound_command(SoundPlaybackState &state,
                                         SoundCommand command,
                                         std::uint16_t stream_offset,
                                         std::uint16_t stream_segment,
                                         std::uint8_t priority);
SoundStream make_stream_for_event(AudioEvent event);
std::vector<std::int16_t> synthesize_pit_samples(std::uint16_t divisor,
                                                 std::uint16_t duration_ticks,
                                                 int sample_rate);
std::vector<std::int16_t> synthesize_sound_samples(const SoundStream &stream,
                                                   int sample_rate,
                                                   int sample_count);
void advance_sound_stream(SoundPlaybackState &state, int sample_rate);
void queue_audio_event(RuntimeState &state, AudioEvent event);
void flush_audio_events(RuntimeState &state, IAudioBackend *backend);

std::unique_ptr<IAudioBackend> make_default_audio_backend();

} // namespace comic2
