#include "comic2/persistence.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <vector>

#include "comic2/resource_loader.hpp"

namespace comic2 {
namespace {

constexpr std::array<std::uint16_t, 10> kDefaultDosConfigRecords = {
    0x021B, 0x6965, 0x023B, 0x3FAD, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x37BD,
};

// Default 127-byte savegame tail from original reference GAME.000
// (DS:0x210..0x28E)
const std::array<std::uint8_t, 127> kDefaultDosSavegameTail = {
    0x38, 0x02, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xb8, 0x0c, 0x4e, 0xa6, 0xb8,
    0x0c, 0x4e, 0xa6, 0xa0, 0x0f, 0x0a, 0xac, 0x4c, 0x0f, 0xb0, 0xac, 0x34,
    0x0f, 0x64, 0xab, 0x34, 0x0f, 0x64, 0xab, 0x4c, 0x0f, 0xb0, 0xac, 0x24,
    0x10, 0x56, 0xad, 0x4c, 0x12, 0xe0, 0xb1, 0x94, 0x12, 0x2c, 0xb3, 0xe4,
    0x13, 0x78, 0xb4, 0xc8, 0x14, 0x6a, 0xb6, 0xa4, 0x14, 0x1e, 0xb5, 0x4c,
    0x15, 0xb6, 0xb7, 0x4c, 0x15, 0x02, 0xb9,
};

void write_u16_at(std::vector<std::uint8_t> &bytes, std::size_t offset,
                  std::uint16_t value) {
  if (offset + 2 <= bytes.size()) {
    bytes[offset] = static_cast<std::uint8_t>(value & 0xFF);
    bytes[offset + 1] = static_cast<std::uint8_t>((value >> 8) & 0xFF);
  }
}

void write_i16_at(std::vector<std::uint8_t> &bytes, std::size_t offset,
                  std::int16_t value) {
  write_u16_at(bytes, offset, static_cast<std::uint16_t>(value));
}

std::uint16_t read_u16_at(const std::vector<std::uint8_t> &bytes,
                          std::size_t offset) {
  if (offset + 2 > bytes.size()) {
    return 0;
  }
  return static_cast<std::uint16_t>(bytes[offset]) |
         (static_cast<std::uint16_t>(bytes[offset + 1]) << 8);
}

std::int16_t read_i16_at(const std::vector<std::uint8_t> &bytes,
                         std::size_t offset) {
  return static_cast<std::int16_t>(read_u16_at(bytes, offset));
}

std::uint16_t read_u16_stream(const std::vector<std::uint8_t> &bytes,
                              std::size_t &offset) {
  const auto val = read_u16_at(bytes, offset);
  offset += 2;
  return val;
}

} // namespace

bool save_file_bytes(const std::filesystem::path &path,
                     const std::vector<std::uint8_t> &bytes) {
  std::ofstream out(path, std::ios::binary);
  if (!out) {
    return false;
  }
  if (!bytes.empty()) {
    out.write(reinterpret_cast<const char *>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
  }
  return out.good();
}

void keys_load_or_defaults(DosKeyBindings &keys,
                           const std::vector<std::uint8_t> &bytes) {
  if (bytes.size() >= kDosKeyBindingsSize) {
    keys.left = bytes[0];
    keys.right = bytes[1];
    keys.up = bytes[2];
    keys.down = bytes[3];
    keys.jump = bytes[4];
    keys.fire = bytes[5];
  } else {
    keys.left = 0x4B;  // Left arrow
    keys.right = 0x4D; // Right arrow
    keys.up = 0x48;    // Up arrow
    keys.down = 0x50;  // Down arrow
    keys.jump = 0x39;  // Space bar
    keys.fire = 0x52;  // 'R' / Ins / trigger
  }
}

void keys_save(const DosKeyBindings &keys, std::vector<std::uint8_t> &bytes) {
  bytes = {keys.left, keys.right, keys.up, keys.down, keys.jump, keys.fire};
}

bool keys_load_file(DosKeyBindings &keys, const std::filesystem::path &path) {
  const auto loaded = load_file_bytes(path);
  if (!loaded.has_value()) {
    keys_load_or_defaults(keys, {});
    return false;
  }
  keys_load_or_defaults(keys, *loaded);
  return true;
}

bool keys_save_file(const DosKeyBindings &keys,
                    const std::filesystem::path &path) {
  std::vector<std::uint8_t> bytes;
  keys_save(keys, bytes);
  return save_file_bytes(path, bytes);
}

void cfg_load_options_or_defaults(RuntimeState &state,
                                  const std::vector<std::uint8_t> &bytes) {
  if (bytes.empty()) {
    state.ui.music_volume = 0x04;
    state.ui.sfx_volume = 0x04;
    state.ui.display_scale = 0x01;
    state.dos_config = DosConfig{};
    state.dos_config.records = kDefaultDosConfigRecords;
    return;
  }

  if (bytes.size() >= kDosConfigSize) {
    std::size_t offset = 0;
    state.dos_config.player_count = read_u16_stream(bytes, offset);

    for (std::size_t slot = 0; slot < 5; ++slot) {
      std::string name;
      name.reserve(16);
      for (std::size_t i = 0; i < 16 && offset < bytes.size(); ++i) {
        const std::uint8_t ch = bytes[offset++];
        if (ch != 0) {
          name.push_back(static_cast<char>(ch));
        }
      }
      while (!name.empty() && name.back() == ' ') {
        name.pop_back();
      }
      state.dos_config.player_names[slot] = name;
    }

    for (std::size_t i = 0; i < 10; ++i) {
      state.dos_config.records[i] = read_u16_stream(bytes, offset);
    }

    const auto vol_word = state.dos_config.records[4];
    if (vol_word != 0) {
      state.ui.music_volume = static_cast<std::uint8_t>(vol_word & 0xFF);
      state.ui.sfx_volume = static_cast<std::uint8_t>((vol_word >> 8) & 0xFF);
    } else {
      state.ui.music_volume = 0x04;
      state.ui.sfx_volume = 0x04;
    }

    const auto scale_word = state.dos_config.records[5];
    if (scale_word != 0) {
      state.ui.display_scale = static_cast<std::uint8_t>(scale_word & 0xFF);
    } else {
      state.ui.display_scale = 0x01;
    }
    return;
  }

  // Legacy/provisional format check
  std::size_t offset = 0;
  const std::uint8_t version = bytes[offset++];
  (void)version;

  if (offset < bytes.size()) {
    state.ui.music_volume = bytes[offset++];
  }
  if (offset < bytes.size()) {
    state.ui.sfx_volume = bytes[offset++];
  }
  if (offset < bytes.size()) {
    state.ui.display_scale = bytes[offset++];
  }
}

void cfg_save_options(const RuntimeState &state,
                      std::vector<std::uint8_t> &bytes) {
  bytes.assign(kDosConfigSize, 0);

  write_u16_at(bytes, 0, state.dos_config.player_count);

  for (std::size_t slot = 0; slot < 5; ++slot) {
    const std::size_t slot_offset = 2 + slot * 16;
    const auto &name = state.dos_config.player_names[slot];
    const std::size_t copy_len = std::min<std::size_t>(name.size(), 15);
    for (std::size_t i = 0; i < copy_len; ++i) {
      bytes[slot_offset + i] = static_cast<std::uint8_t>(name[i]);
    }
  }

  const bool records_all_zero = std::all_of(
      state.dos_config.records.begin(), state.dos_config.records.end(),
      [](std::uint16_t v) { return v == 0; });

  const auto &source_records =
      records_all_zero ? kDefaultDosConfigRecords : state.dos_config.records;

  for (std::size_t i = 0; i < 10; ++i) {
    write_u16_at(bytes, 82 + i * 2, source_records[i]);
  }

  // Preserve customized audio volume and display scale settings in records 4 &
  // 5
  if (state.ui.music_volume != 0x04 || state.ui.sfx_volume != 0x04) {
    const std::uint16_t vol_word =
        static_cast<std::uint16_t>(state.ui.music_volume) |
        (static_cast<std::uint16_t>(state.ui.sfx_volume) << 8);
    write_u16_at(bytes, 82 + 4 * 2, vol_word);
  }
  if (state.ui.display_scale != 0x01) {
    write_u16_at(bytes, 82 + 5 * 2,
                 static_cast<std::uint16_t>(state.ui.display_scale));
  }
}

bool cfg_load_options_file(RuntimeState &state,
                           const std::filesystem::path &path) {
  const auto loaded = load_file_bytes(path);
  if (!loaded.has_value()) {
    cfg_load_options_or_defaults(state, {});
    return false;
  }
  cfg_load_options_or_defaults(state, *loaded);
  return true;
}

bool cfg_save_options_file(const RuntimeState &state,
                           const std::filesystem::path &path) {
  std::vector<std::uint8_t> bytes;
  cfg_save_options(state, bytes);
  return save_file_bytes(path, bytes);
}

void savegame_read_snapshot(RuntimeState &state,
                            const std::vector<std::uint8_t> &bytes) {
  if (bytes.empty()) {
    return;
  }

  if (bytes.size() >= kDosSavegameSize) {
    // 1. First 3000 bytes: 250 mapped object records
    state.savegame_mapped_objects_raw.assign(
        bytes.begin(), bytes.begin() + kDosMappedObjectsBytes);
    state.mapped_objects.resize(kDosMappedObjectsCount);
    std::memcpy(state.mapped_objects.data(), bytes.data(),
                kDosMappedObjectsBytes);

    // 2. Trailing 127 bytes: player/session state
    state.savegame_tail_bytes.assign(bytes.begin() + kDosMappedObjectsBytes,
                                     bytes.begin() + kDosMappedObjectsBytes +
                                         kDosSavegameTailBytes);

    const std::size_t tail = kDosMappedObjectsBytes;
    state.player.x = read_i16_at(bytes, tail + 0);
    state.player.y = read_i16_at(bytes, tail + 2);
    state.player.x_vel = read_i16_at(bytes, tail + 4);
    state.player.y_vel = read_i16_at(bytes, tail + 6);

    const auto facing_byte = bytes[tail + 8];
    state.player.facing_right = (facing_byte != 2);
    state.player.animation_state = bytes[tail + 9];

    state.player.firepower = bytes[tail + 12];
    const bool boots = (bytes[tail + 13] != 0);
    state.player.boots_equipped = boots;
    state.progression.boots_acquired = boots;

    const bool lantern = (bytes[tail + 14] != 0);
    state.player.lantern_equipped = lantern;
    state.progression.lantern_acquired = lantern;

    const bool wand = (bytes[tail + 15] != 0);
    state.player.wand_equipped = wand;
    state.progression.wand_acquired = wand;

    const auto room_byte = bytes[tail + 16];
    state.current_room = (room_byte == 0xFF ? 0 : room_byte);

    state.player.key_count = bytes[tail + 17];
    state.progression.keys_collected = bytes[tail + 17];

    state.player.score = read_u16_at(bytes, tail + 18);
    state.player.gems =
        static_cast<std::uint8_t>(read_u16_at(bytes, tail + 20) & 0xFF);
    state.current_level = bytes[tail + 22];

    if (bytes[tail + 11] != 0) {
      state.ui.menu_state = static_cast<MenuState>(bytes[tail + 11] & 0x07);
      const auto lives = static_cast<std::uint8_t>(bytes[tail + 11] >> 3);
      if (lives > 0) {
        state.player.lives = lives;
      }
    }
    return;
  }

  // Legacy snapshot parsing fallback
  std::size_t offset = 0;
  const auto version = bytes[offset++];
  (void)version;

  state.current_level = read_u16_stream(bytes, offset);
  state.current_room = read_u16_stream(bytes, offset);

  if (offset + 2 > bytes.size()) {
    return;
  }
  const auto score_lo = bytes[offset++];
  const auto score_hi = bytes[offset++];
  state.player.score = static_cast<std::uint16_t>(
      score_lo | (static_cast<std::uint16_t>(score_hi) << 8));

  if (offset < bytes.size()) {
    state.player.gems = bytes[offset++];
  } else {
    return;
  }
  if (offset < bytes.size()) {
    state.player.lives = bytes[offset++];
  } else {
    return;
  }
  if (offset < bytes.size()) {
    state.player.hp = bytes[offset++];
  } else {
    return;
  }
  if (offset < bytes.size()) {
    state.ui.menu_state = static_cast<MenuState>(bytes[offset++]);
  }
}

void savegame_write_snapshot(const RuntimeState &state,
                             std::vector<std::uint8_t> &bytes) {
  bytes.assign(kDosSavegameSize, 0);

  // 1. Populate 3,000 bytes of mapped objects
  if (state.savegame_mapped_objects_raw.size() == kDosMappedObjectsBytes) {
    std::memcpy(bytes.data(), state.savegame_mapped_objects_raw.data(),
                kDosMappedObjectsBytes);
    if (!state.mapped_objects.empty()) {
      const std::size_t count = std::min<std::size_t>(
          state.mapped_objects.size(), kDosMappedObjectsCount);
      std::memcpy(bytes.data(), state.mapped_objects.data(),
                  count * sizeof(MappedObject12));
    }
  } else if (!state.mapped_objects.empty()) {
    const std::size_t count = std::min<std::size_t>(state.mapped_objects.size(),
                                                    kDosMappedObjectsCount);
    std::memcpy(bytes.data(), state.mapped_objects.data(),
                count * sizeof(MappedObject12));
  }

  // 2. Populate 127 bytes of tail state
  const std::size_t tail = kDosMappedObjectsBytes;
  if (state.savegame_tail_bytes.size() == kDosSavegameTailBytes) {
    std::memcpy(bytes.data() + tail, state.savegame_tail_bytes.data(),
                kDosSavegameTailBytes);
  } else {
    std::memcpy(bytes.data() + tail, kDefaultDosSavegameTail.data(),
                kDosSavegameTailBytes);
  }

  // 3. Overlay active runtime state fields
  write_i16_at(bytes, tail + 0, state.player.x);
  write_i16_at(bytes, tail + 2, state.player.y);
  write_i16_at(bytes, tail + 4, state.player.x_vel);
  write_i16_at(bytes, tail + 6, state.player.y_vel);
  bytes[tail + 8] = state.player.facing_right ? 1 : 2;
  bytes[tail + 9] = state.player.animation_state;

  if (state.savegame_tail_bytes.empty()) {
    bytes[tail + 10] = state.player.hp;
    bytes[tail + 11] = static_cast<std::uint8_t>(
        (static_cast<std::uint8_t>(state.ui.menu_state) & 0x07) |
        ((state.player.lives & 0x1F) << 3));
  }

  bytes[tail + 12] = state.player.firepower;
  bytes[tail + 13] = state.player.boots_equipped ? 1 : 0;
  bytes[tail + 14] = state.player.lantern_equipped ? 1 : 0;
  bytes[tail + 15] = state.player.wand_equipped ? 1 : 0;

  if (state.current_room == 0 && (!state.savegame_tail_bytes.empty()
                                      ? state.savegame_tail_bytes[16] == 0xFF
                                      : true)) {
    bytes[tail + 16] = 0xFF;
  } else {
    bytes[tail + 16] = static_cast<std::uint8_t>(state.current_room & 0xFF);
  }
  bytes[tail + 17] = state.player.key_count;

  write_u16_at(bytes, tail + 18, state.player.score);

  const auto orig_gems =
      state.savegame_tail_bytes.size() == kDosSavegameTailBytes
          ? read_u16_at(state.savegame_tail_bytes, 20)
          : 0;
  if ((orig_gems & 0xFF) == state.player.gems) {
    write_u16_at(bytes, tail + 20, orig_gems);
  } else {
    write_u16_at(bytes, tail + 20, state.player.gems);
  }

  bytes[tail + 22] = static_cast<std::uint8_t>(state.current_level & 0xFF);

  const auto sig = read_u16_at(bytes, tail + 23);
  if (sig != kDosSavegameMagicDefault && sig != kDosSavegameMagicSaved) {
    write_u16_at(bytes, tail + 23, kDosSavegameMagicDefault);
  }
}

bool savegame_read_snapshot_file(RuntimeState &state,
                                 const std::filesystem::path &path) {
  const auto loaded = load_file_bytes(path);
  if (!loaded.has_value()) {
    return false;
  }
  savegame_read_snapshot(state, *loaded);
  return true;
}

bool savegame_write_snapshot_file(const RuntimeState &state,
                                  const std::filesystem::path &path) {
  std::vector<std::uint8_t> bytes;
  savegame_write_snapshot(state, bytes);
  return save_file_bytes(path, bytes);
}

} // namespace comic2
