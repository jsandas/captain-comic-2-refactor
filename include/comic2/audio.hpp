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
  DoorOpen,
  Teleport,
  JumpTruncate,
  EnemyHit,
  EnemyDefeat,
  BossDamage,
  ExtraLife,
  MenuTheme,
  VictoryFanfare,
  ModalPrompt,
  IntroFlash,
  IntroOverlay,
  IntroFlashB,
  ItemActivate,
};

constexpr std::size_t kAuthenticSoundStreamCount = 56U;

constexpr std::uint16_t kSoundOffsetTitleTheme = 0x00DBu;
constexpr std::uint16_t kSoundOffsetLevelWave = 0x93CCu;
constexpr std::uint16_t kSoundOffsetRoomTransition = 0x965Eu;
constexpr std::uint16_t kSoundOffsetLadderClimb = 0x9676u;
constexpr std::uint16_t kSoundOffsetTransientBreak = 0x968Eu;
constexpr std::uint16_t kSoundOffsetPlayerDeath = 0x969Au;
constexpr std::uint16_t kSoundOffsetTimedOverlay = 0x96AEu;
constexpr std::uint16_t kSoundOffsetVictoryFanfare = 0x96B6u;
constexpr std::uint16_t kSoundOffsetJumpStart = 0x96DEu;
constexpr std::uint16_t kSoundOffsetJumpTruncate = 0x96F6u;
constexpr std::uint16_t kSoundOffsetRevealWipe = 0x973Cu;
constexpr std::uint16_t kSoundOffsetDoorEnter = 0x9764u;
constexpr std::uint16_t kSoundOffsetDoorExit = 0x97CCu;
constexpr std::uint16_t kSoundOffsetTeleportWave = 0x9834u;
constexpr std::uint16_t kSoundOffsetStackedSpriteRise = 0x9894u;
constexpr std::uint16_t kSoundOffsetLowToneStep = 0x98D0u;
constexpr std::uint16_t kSoundOffsetItemPickup = 0x98E0u;
constexpr std::uint16_t kSoundOffsetInventoryFull = 0x98F4u;
constexpr std::uint16_t kSoundOffsetItemActivate = 0x98FCu;
constexpr std::uint16_t kSoundOffsetExtraLife = 0x9904u;
constexpr std::uint16_t kSoundOffsetSpecialRoomItem = 0x99A4u;
constexpr std::uint16_t kSoundOffsetSpecialEvent = 0x99ECu;
constexpr std::uint16_t kSoundOffsetEnemyDefeat = 0x9A25u;
constexpr std::uint16_t kSoundOffsetEnemyHit = 0x9A31u;
constexpr std::uint16_t kSoundOffsetBossDamage = 0x9A3Du;
constexpr std::uint16_t kSoundOffsetPlayerHurt = 0x9A4Du;
constexpr std::uint16_t kSoundOffsetPlayerShoot = 0x9A80u;
constexpr std::uint16_t kSoundOffsetMainMenuTheme = 0x9D3Fu;
constexpr std::uint16_t kSoundOffsetModalPromptBeep = 0xAAC0u;
constexpr std::uint16_t kSoundOffsetIntroFlashA = 0xAC3Au;
constexpr std::uint16_t kSoundOffsetIntroOverlay = 0xAC7Au;
constexpr std::uint16_t kSoundOffsetIntroFlashB = 0xAC92u;

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
  bool pending_initial_tick_ = false;
};

class IAudioBackend {
public:
  virtual ~IAudioBackend() = default;

  virtual bool initialize() = 0;
  virtual void enqueue_event(AudioEvent event) = 0;
  virtual void stop() {}
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
std::size_t get_authentic_sound_table_size();
SoundStream get_authentic_sound_stream_by_index(std::size_t index);
std::uint16_t get_authentic_sound_offset_by_index(std::size_t index);
SoundStream make_stream_from_offset(std::uint16_t stream_offset);
std::uint16_t get_sound_offset_for_event(AudioEvent event);
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
