#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "comic2/game_state.hpp"
#include "comic2/persistence.hpp"
#include "comic2/resource_loader.hpp"

namespace {

void expect(bool condition, const char *message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

std::filesystem::path find_reference_original_dir() {
  const std::vector<std::filesystem::path> candidates = {
      "reference/original",
      "../reference/original",
      "../../reference/original",
  };
  for (const auto &p : candidates) {
    if (std::filesystem::exists(p / "GAME.000")) {
      return p;
    }
  }
  return {};
}

void test_reference_original_game000_roundtrip() {
  const auto dir = find_reference_original_dir();
  if (dir.empty()) {
    return;
  }

  const auto path = dir / "GAME.000";
  const auto bytes = comic2::load_file_bytes(path);
  expect(bytes.has_value(), "GAME.000 must exist in reference/original");
  expect(bytes->size() == comic2::kDosSavegameSize,
         "GAME.000 must be exactly 3,127 bytes");

  comic2::RuntimeState state = comic2::make_default_runtime_state();
  comic2::savegame_read_snapshot(state, *bytes);

  expect(state.player.x == 568, "GAME.000 spawn X should equal 568");
  expect(state.player.y == 80, "GAME.000 spawn Y should equal 80");
  expect(state.player.facing_right == true,
         "GAME.000 facing right should be true");
  expect(state.player.firepower == 0, "GAME.000 firepower should equal 0");
  expect(state.player.boots_equipped == false,
         "GAME.000 boots should be unequipped");
  expect(state.player.score == 0, "GAME.000 score should equal 0");
  expect(state.player.gems == 0, "GAME.000 gems should equal 0");
  expect(state.current_level == 0, "GAME.000 level should equal 0");
  expect(state.mapped_objects.size() == 250,
         "GAME.000 must have 250 mapped objects");

  // Roundtrip save
  std::vector<std::uint8_t> saved_bytes;
  comic2::savegame_write_snapshot(state, saved_bytes);

  expect(saved_bytes.size() == comic2::kDosSavegameSize,
         "Saved GAME.000 must be exactly 3,127 bytes");
  expect(saved_bytes == *bytes,
         "Saved GAME.000 must match original DOS file byte-for-byte");
}

void test_reference_original_game_slots_roundtrip() {
  const auto dir = find_reference_original_dir();
  if (dir.empty()) {
    return;
  }

  for (const std::string filename : {"GAME.001", "GAME.002", "GAME.003"}) {
    const auto path = dir / filename;
    const auto bytes = comic2::load_file_bytes(path);
    if (!bytes.has_value()) {
      continue;
    }
    expect(bytes->size() == comic2::kDosSavegameSize,
           "Game slot file must be exactly 3,127 bytes");

    comic2::RuntimeState state = comic2::make_default_runtime_state();
    comic2::savegame_read_snapshot(state, *bytes);

    if (filename == "GAME.003") {
      expect(!state.player.facing_right,
             "GAME.003 facing right should be false");
      expect(state.player.firepower == 1, "GAME.003 firepower should be 1");
      expect(state.player.boots_equipped == true,
             "GAME.003 boots should be equipped");
      expect(state.progression.boots_acquired == true,
             "GAME.003 boots progression acquired");
      expect(state.current_room == 2, "GAME.003 room should be 2");
      expect(state.player.score == 1280, "GAME.003 score should equal 1280");
    }

    std::vector<std::uint8_t> saved_bytes;
    comic2::savegame_write_snapshot(state, saved_bytes);

    expect(saved_bytes.size() == comic2::kDosSavegameSize,
           "Saved slot must be exactly 3,127 bytes");
    expect(saved_bytes == *bytes,
           "Saved slot must match original DOS file byte-for-byte");
  }
}

void test_savegame_mutation_roundtrip() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.player.x = 320;
  state.player.y = 128;
  state.player.x_vel = 12;
  state.player.y_vel = -8;
  state.player.facing_right = false;
  state.player.animation_state = 3;
  state.player.firepower = 3;
  state.player.boots_equipped = true;
  state.player.lantern_equipped = true;
  state.player.wand_equipped = true;
  state.player.key_count = 4;
  state.player.score = 5432;
  state.player.gems = 18;
  state.player.lives = 5;
  state.current_level = 1;
  state.current_room = 3;

  std::vector<std::uint8_t> bytes;
  comic2::savegame_write_snapshot(state, bytes);
  expect(bytes.size() == comic2::kDosSavegameSize,
         "savegame_write_snapshot should emit 3,127 bytes");

  comic2::RuntimeState loaded = comic2::make_default_runtime_state();
  comic2::savegame_read_snapshot(loaded, bytes);

  expect(loaded.player.x == 320, "mutated x should roundtrip");
  expect(loaded.player.y == 128, "mutated y should roundtrip");
  expect(loaded.player.x_vel == 12, "mutated x_vel should roundtrip");
  expect(loaded.player.y_vel == -8, "mutated y_vel should roundtrip");
  expect(loaded.player.facing_right == false,
         "mutated facing direction should roundtrip");
  expect(loaded.player.animation_state == 3,
         "mutated animation state should roundtrip");
  expect(loaded.player.firepower == 3, "mutated firepower should roundtrip");
  expect(loaded.player.boots_equipped == true,
         "mutated boots should roundtrip");
  expect(loaded.progression.boots_acquired == true,
         "mutated boots progression should roundtrip");
  expect(loaded.player.lantern_equipped == true,
         "mutated lantern should roundtrip");
  expect(loaded.progression.lantern_acquired == true,
         "mutated lantern progression should roundtrip");
  expect(loaded.player.wand_equipped == true, "mutated wand should roundtrip");
  expect(loaded.progression.wand_acquired == true,
         "mutated wand progression should roundtrip");
  expect(loaded.player.key_count == 4, "mutated key count should roundtrip");
  expect(loaded.progression.keys_collected == 4,
         "mutated keys progression should roundtrip");
  expect(loaded.player.score == 5432, "mutated score should roundtrip");
  expect(loaded.player.gems == 18, "mutated gems should roundtrip");
  expect(loaded.player.lives == 5, "mutated lives should roundtrip");
  expect(loaded.current_level == 1, "mutated level should roundtrip");
  expect(loaded.current_room == 3, "mutated room should roundtrip");
}

void test_reference_original_frcfg_roundtrip() {
  const auto dir = find_reference_original_dir();
  if (dir.empty()) {
    return;
  }

  const auto path = dir / "FR.CFG";
  const auto bytes = comic2::load_file_bytes(path);
  expect(bytes.has_value(), "FR.CFG must exist in reference/original");
  expect(bytes->size() == comic2::kDosConfigSize,
         "FR.CFG must be exactly 102 bytes");

  comic2::RuntimeState state = comic2::make_default_runtime_state();
  comic2::cfg_load_options_or_defaults(state, *bytes);

  std::vector<std::uint8_t> saved_bytes;
  comic2::cfg_save_options(state, saved_bytes);

  expect(saved_bytes.size() == comic2::kDosConfigSize,
         "Saved FR.CFG must be exactly 102 bytes");
  expect(saved_bytes == *bytes,
         "Saved FR.CFG must match original DOS file byte-for-byte");
}

void test_config_player_profiles_roundtrip() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.dos_config.player_count = 3;
  state.dos_config.player_names[0] = "COMIC";
  state.dos_config.player_names[1] = "CAPTAIN";
  state.dos_config.player_names[2] = "HERO";

  std::vector<std::uint8_t> bytes;
  comic2::cfg_save_options(state, bytes);
  expect(bytes.size() == comic2::kDosConfigSize,
         "config should serialize to 102 bytes");

  comic2::RuntimeState loaded = comic2::make_default_runtime_state();
  comic2::cfg_load_options_or_defaults(loaded, bytes);

  expect(loaded.dos_config.player_count == 3, "player_count should roundtrip");
  expect(loaded.dos_config.player_names[0] == "COMIC",
         "player 0 name should roundtrip");
  expect(loaded.dos_config.player_names[1] == "CAPTAIN",
         "player 1 name should roundtrip");
  expect(loaded.dos_config.player_names[2] == "HERO",
         "player 2 name should roundtrip");
}

void test_reference_original_keys_def_roundtrip() {
  const auto dir = find_reference_original_dir();
  if (dir.empty()) {
    return;
  }

  const auto path = dir / "KEYS.DEF";
  const auto bytes = comic2::load_file_bytes(path);
  expect(bytes.has_value(), "KEYS.DEF must exist in reference/original");
  expect(bytes->size() == comic2::kDosKeyBindingsSize,
         "KEYS.DEF must be exactly 6 bytes");

  comic2::DosKeyBindings keys{};
  comic2::keys_load_or_defaults(keys, *bytes);

  expect(keys.left == 0x1D, "KEYS.DEF Left scancode should be 0x1D");
  expect(keys.right == 0x38, "KEYS.DEF Right scancode should be 0x38");

  std::vector<std::uint8_t> saved_bytes;
  comic2::keys_save(keys, saved_bytes);

  expect(saved_bytes.size() == comic2::kDosKeyBindingsSize,
         "Saved KEYS.DEF must be exactly 6 bytes");
  expect(saved_bytes == *bytes,
         "Saved KEYS.DEF must match original DOS file byte-for-byte");
}

void test_keys_def_defaults_and_customization() {
  comic2::DosKeyBindings keys{};
  comic2::keys_load_or_defaults(keys, {});

  expect(keys.left == 0x4B, "default Left should be 0x4B (Left arrow)");
  expect(keys.right == 0x4D, "default Right should be 0x4D (Right arrow)");
  expect(keys.up == 0x48, "default Up should be 0x48 (Up arrow)");
  expect(keys.down == 0x50, "default Down should be 0x50 (Down arrow)");
  expect(keys.jump == 0x39, "default Jump should be 0x39 (Space)");
  expect(keys.fire == 0x52, "default Fire should be 0x52 (Ins/R)");

  keys.left = 0x1E;  // 'A'
  keys.right = 0x20; // 'D'
  keys.up = 0x11;    // 'W'
  keys.down = 0x1F;  // 'S'
  keys.jump = 0x39;  // Space
  keys.fire = 0x2A;  // Left Shift

  std::vector<std::uint8_t> bytes;
  comic2::keys_save(keys, bytes);
  expect(bytes.size() == 6, "keys_save should produce 6 bytes");

  comic2::DosKeyBindings loaded_keys{};
  comic2::keys_load_or_defaults(loaded_keys, bytes);

  expect(loaded_keys == keys, "custom keys should roundtrip exactly");
}

void test_disk_file_io_helpers() {
  const auto temp_dir = std::filesystem::temp_directory_path();
  const auto save_path = temp_dir / "test_comic2_game.000";
  const auto cfg_path = temp_dir / "test_comic2_fr.cfg";
  const auto keys_path = temp_dir / "test_comic2_keys.def";

  // 1. Savegame file I/O
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.player.x = 450;
  state.player.y = 120;
  state.player.score = 7777;

  expect(comic2::savegame_write_snapshot_file(state, save_path),
         "savegame_write_snapshot_file should succeed");
  expect(std::filesystem::file_size(save_path) == comic2::kDosSavegameSize,
         "savegame file on disk must be 3,127 bytes");

  comic2::RuntimeState loaded_state = comic2::make_default_runtime_state();
  expect(comic2::savegame_read_snapshot_file(loaded_state, save_path),
         "savegame_read_snapshot_file should succeed");
  expect(loaded_state.player.x == 450, "file-loaded X should match");
  expect(loaded_state.player.score == 7777, "file-loaded score should match");

  // 2. Config file I/O
  state.dos_config.player_count = 1;
  state.dos_config.player_names[0] = "CHAMPION";
  expect(comic2::cfg_save_options_file(state, cfg_path),
         "cfg_save_options_file should succeed");
  expect(std::filesystem::file_size(cfg_path) == comic2::kDosConfigSize,
         "config file on disk must be 102 bytes");

  comic2::RuntimeState loaded_cfg_state = comic2::make_default_runtime_state();
  expect(comic2::cfg_load_options_file(loaded_cfg_state, cfg_path),
         "cfg_load_options_file should succeed");
  expect(loaded_cfg_state.dos_config.player_names[0] == "CHAMPION",
         "file-loaded player name should match");

  // 3. Keys file I/O
  comic2::DosKeyBindings keys{};
  keys.left = 0x1E;
  keys.right = 0x20;
  expect(comic2::keys_save_file(keys, keys_path),
         "keys_save_file should succeed");
  expect(std::filesystem::file_size(keys_path) == comic2::kDosKeyBindingsSize,
         "keys file on disk must be 6 bytes");

  comic2::DosKeyBindings loaded_keys{};
  expect(comic2::keys_load_file(loaded_keys, keys_path),
         "keys_load_file should succeed");
  expect(loaded_keys.left == 0x1E, "file-loaded left key should match");
  expect(loaded_keys.right == 0x20, "file-loaded right key should match");

  // Cleanup
  std::filesystem::remove(save_path);
  std::filesystem::remove(cfg_path);
  std::filesystem::remove(keys_path);
}

void test_config_roundtrip_preserves_settings() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.ui.music_volume = 0x05;
  state.ui.sfx_volume = 0x07;
  state.ui.display_scale = 0x02;

  std::vector<std::uint8_t> bytes;
  comic2::cfg_save_options(state, bytes);
  expect(bytes.size() >= 3, "config save should write at least three bytes");

  comic2::RuntimeState loaded = comic2::make_default_runtime_state();
  comic2::cfg_load_options_or_defaults(loaded, bytes);
  expect(loaded.ui.music_volume == 0x05, "music volume should roundtrip");
  expect(loaded.ui.sfx_volume == 0x07, "sfx volume should roundtrip");
  expect(loaded.ui.display_scale == 0x02, "display scale should roundtrip");
}

void test_snapshot_roundtrip_preserves_state() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.player.score = 0x1234;
  state.player.gems = 0x07;
  state.player.lives = 0x03;
  state.player.hp = 0x09;
  state.current_level = 2;
  state.current_room = 4;
  state.ui.menu_state = comic2::MenuState::Options;

  std::vector<std::uint8_t> bytes;
  comic2::savegame_write_snapshot(state, bytes);
  expect(!bytes.empty(), "snapshot writer should emit data");

  comic2::RuntimeState loaded = comic2::make_default_runtime_state();
  comic2::savegame_read_snapshot(loaded, bytes);
  expect(loaded.current_level == 2, "level should roundtrip");
  expect(loaded.current_room == 4, "room should roundtrip");
  expect(loaded.player.score == 0x1234, "score should roundtrip");
  expect(loaded.player.gems == 0x07, "gems should roundtrip");
  expect(loaded.player.lives == 0x03, "lives should roundtrip");
  expect(loaded.ui.menu_state == comic2::MenuState::Options,
         "menu state should roundtrip");
}

void test_config_loader_stops_at_truncated_payload() {
  const std::vector<std::uint8_t> bytes{0x01, 0x05, 0x07};

  comic2::RuntimeState loaded = comic2::make_default_runtime_state();
  comic2::cfg_load_options_or_defaults(loaded, bytes);
  expect(loaded.ui.music_volume == 0x05, "music volume should load from bytes");
  expect(loaded.ui.sfx_volume == 0x07, "sfx volume should load from bytes");
  expect(loaded.ui.display_scale == 0x01,
         "display scale should remain default for truncated payload");
}

} // namespace

void run_persistence_tests() {
  test_reference_original_game000_roundtrip();
  test_reference_original_game_slots_roundtrip();
  test_savegame_mutation_roundtrip();
  test_reference_original_frcfg_roundtrip();
  test_config_player_profiles_roundtrip();
  test_reference_original_keys_def_roundtrip();
  test_keys_def_defaults_and_customization();
  test_disk_file_io_helpers();
  test_config_roundtrip_preserves_settings();
  test_snapshot_roundtrip_preserves_state();
  test_config_loader_stops_at_truncated_payload();
}
