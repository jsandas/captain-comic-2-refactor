#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "comic2/game_state.hpp"
#include "comic2/types.hpp"

namespace comic2 {

constexpr std::size_t kDosSavegameSize = 3127;
constexpr std::size_t kDosMappedObjectsCount = 250;
constexpr std::size_t kDosMappedObjectsBytes = 3000;
constexpr std::size_t kDosSavegameTailBytes = 127;
constexpr std::size_t kDosConfigSize = 102;
constexpr std::size_t kDosKeyBindingsSize = 6;

constexpr std::uint16_t kDosSavegameMagicDefault = 0x0150;
constexpr std::uint16_t kDosSavegameMagicSaved = 0x0110;

// Configuration options serialization (FR.CFG - 102 bytes)
void cfg_load_options_or_defaults(RuntimeState &state,
                                  const std::vector<std::uint8_t> &bytes);
void cfg_save_options(const RuntimeState &state,
                      std::vector<std::uint8_t> &bytes);
bool cfg_load_options_file(RuntimeState &state,
                           const std::filesystem::path &path);
bool cfg_save_options_file(const RuntimeState &state,
                           const std::filesystem::path &path);

// Savegame snapshot serialization (GAME.000 .. GAME.003 - 3,127 bytes)
void savegame_write_snapshot(const RuntimeState &state,
                             std::vector<std::uint8_t> &bytes);
void savegame_read_snapshot(RuntimeState &state,
                            const std::vector<std::uint8_t> &bytes);
bool savegame_write_snapshot_file(const RuntimeState &state,
                                  const std::filesystem::path &path);
bool savegame_read_snapshot_file(RuntimeState &state,
                                 const std::filesystem::path &path);

// Key definitions serialization (KEYS.DEF - 6 bytes)
void keys_load_or_defaults(DosKeyBindings &keys,
                           const std::vector<std::uint8_t> &bytes);
void keys_save(const DosKeyBindings &keys,
               std::vector<std::uint8_t> &bytes);
bool keys_load_file(DosKeyBindings &keys,
                    const std::filesystem::path &path);
bool keys_save_file(const DosKeyBindings &keys,
                    const std::filesystem::path &path);

// File I/O helper
bool save_file_bytes(const std::filesystem::path &path,
                     const std::vector<std::uint8_t> &bytes);

} // namespace comic2
