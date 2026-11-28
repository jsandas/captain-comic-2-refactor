#include "comic2/room_loader.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "comic2/entity_runtime.hpp"
#include "comic2/level_data.hpp"
#include "comic2/resource_loader.hpp"

namespace comic2 {

namespace {

constexpr std::size_t kRoomRowPointerMapOffset = 0x2A0;
constexpr std::size_t kRoomTableOffset = 0x04;
constexpr std::size_t kRoomEntrySize = 6;
constexpr std::uint16_t kSentinelEntry = 0xFFFFu;
constexpr std::size_t kMappedObjectCountOffset = 0x2B0;
constexpr std::size_t kMappedObjectTableOffset = 0x2B2;
constexpr std::size_t kMappedObjectRecordSize = 12;

std::uint16_t read_u16(std::span<const std::uint8_t> bytes, std::size_t off) {
  return static_cast<std::uint16_t>(bytes[off] | (bytes[off + 1] << 8));
}

void hydrate_entity_runtime_for_room(RuntimeState &state) {
  state.active_entities.clear();
  state.runtime_slots.clear();
  state.activation_state = EntityActivationState{};

  if (state.mapped_objects.empty()) {
    return;
  }

  ent_build_room_entity_list(state.mapped_objects, state.current_room,
                             state.current_level, state.active_entities,
                             state.activation_state);

  const EntityViewportBounds viewport{};
  ent_build_runtime_slots_for_viewport(
      state.active_entities, state.mapped_objects, state.runtime_slots,
      viewport, state.activation_state, state.activation_toggle);
}

bool is_room_payload_candidate_name(std::string name) {
  std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) {
    return static_cast<char>(std::toupper(c));
  });

  if (name.size() != 7 || name.rfind("FR", 0) != 0 || name[5] != '.') {
    return false;
  }

  const bool room_index_is_digits =
      std::isdigit(static_cast<unsigned char>(name[2])) != 0 &&
      std::isdigit(static_cast<unsigned char>(name[3])) != 0 &&
      std::isdigit(static_cast<unsigned char>(name[4])) != 0;
  const bool variant_is_digit =
      std::isdigit(static_cast<unsigned char>(name[6])) != 0;
  return room_index_is_digits && variant_is_digit;
}

std::vector<std::filesystem::path>
discover_room_payload_candidates(const std::filesystem::path &root) {
  std::vector<std::filesystem::path> candidates;

  std::error_code ec;
  if (!std::filesystem::exists(root, ec) ||
      !std::filesystem::is_directory(root, ec)) {
    return candidates;
  }

  for (std::filesystem::directory_iterator it(root, ec), end; !ec && it != end;
       it.increment(ec)) {
    const auto &entry = *it;
    if (!entry.is_regular_file(ec) || ec) {
      ec.clear();
      continue;
    }

    const std::string name = entry.path().filename().string();
    if (is_room_payload_candidate_name(name)) {
      candidates.push_back(entry.path());
    }
  }

  std::sort(candidates.begin(), candidates.end());
  return candidates;
}

bool try_load_level_tileset_for_root(RuntimeState &state,
                                     const std::filesystem::path &root,
                                     std::uint16_t level) {
  const auto tuple = resolve_level_resource_tuple(level, root);
  if (!tuple.has_value()) {
    return false;
  }

  const auto tileset_path = root / tuple->tileset_filename;
  const auto bytes = load_file_bytes(tileset_path);
  if (!bytes.has_value()) {
    return false;
  }

  const auto decoded = decode_level_tileset(*bytes);
  if (!decoded.has_value()) {
    return false;
  }

  state.level_tileset = decoded->tiles;
  state.tile_hazard_bounds = decoded->hazard_bounds;
  state.tile_ceiling_threshold = decoded->ceiling_threshold;

  state.level_sprites.clear();
  if (!tuple->sprite_sheet_filename.empty()) {
    const auto sprite_path = root / tuple->sprite_sheet_filename;
    if (const auto sprite_bytes = load_file_bytes(sprite_path);
        sprite_bytes.has_value()) {
      if (const auto decoded_sprites = decode_masked_sprite_sheet(
              std::span<const std::uint8_t>(*sprite_bytes));
          decoded_sprites.has_value() && !decoded_sprites->empty()) {
        state.level_sprites = *decoded_sprites;
      }
    }
  }

  state.level_script_bytes.clear();
  if (!tuple->script_filename.empty()) {
    const auto script_path = root / tuple->script_filename;
    if (const auto script_bytes = load_file_bytes(script_path);
        script_bytes.has_value()) {
      if (const auto decoded_script = decode_level_script_payload(
              std::span<const std::uint8_t>(*script_bytes));
          decoded_script.has_value()) {
        state.level_script_bytes = *decoded_script;
      }
    }
  }

  return true;
}

} // namespace

std::optional<RoomTableHeader>
parse_room_table_header(std::span<const std::uint8_t> bytes) {
  if (bytes.size() < 4) {
    return std::nullopt;
  }

  const std::uint16_t room_count = read_u16(bytes, 0);
  if (room_count == 0) {
    return std::nullopt;
  }

  RoomTableHeader header{};
  header.room_count = room_count;
  header.level_id = static_cast<std::uint16_t>(
      bytes[2] | (static_cast<std::uint16_t>(bytes[3]) << 8));
  return header;
}

std::optional<RoomLoadSpec>
resolve_room_load_spec(const std::filesystem::path &source_path,
                       std::span<const std::uint8_t> bytes, std::uint16_t level,
                       std::uint16_t room, ResourceAssetKind asset_kind) {
  if (bytes.size() < kRoomTableOffset + kRoomEntrySize) {
    return std::nullopt;
  }

  std::optional<RoomTableHeader> room_header = parse_room_table_header(bytes);
  const std::uint16_t file_level = static_cast<std::uint16_t>(
      bytes[2] | (static_cast<std::uint16_t>(bytes[3]) << 8));

  if (room_header.has_value()) {
    if (room_header->room_count == 0) {
      return std::nullopt;
    }
    if (room >= room_header->room_count) {
      return std::nullopt;
    }
    if (room_header->level_id != 0 && room_header->level_id != level) {
      return std::nullopt;
    }
  } else if (file_level != level && file_level != 0) {
    return std::nullopt;
  }

  const std::size_t room_entry_offset =
      kRoomTableOffset + static_cast<std::size_t>(room) * kRoomEntrySize;
  if (room_entry_offset > bytes.size() - kRoomEntrySize) {
    return std::nullopt;
  }

  const std::optional<FrdataRoomEntry> room_entry =
      decode_frdata_room_entry(bytes, room_entry_offset);
  if (!room_entry.has_value() || room_entry->tile_w == 0 ||
      room_entry->tile_h == 0 || room_entry->tile_w == kSentinelEntry ||
      room_entry->tile_h == kSentinelEntry ||
      room_entry->rle_data_off == kSentinelEntry) {
    return std::nullopt;
  }

  RoomLoadSpec spec;
  spec.source_path = source_path;
  spec.level = level;
  spec.room = room;
  spec.room_count = room_header.has_value() ? room_header->room_count : 0;
  spec.level_id = room_header.has_value() ? room_header->level_id : file_level;
  spec.asset_kind = asset_kind;
  spec.table_offset = kRoomTableOffset;
  spec.room_entry_offset = room_entry_offset;
  spec.resource_offset =
      asset_kind == ResourceAssetKind::RoomTable
          ? room_entry_offset
          : static_cast<std::size_t>(room_entry->rle_data_off);
  spec.room_entry = *room_entry;

  if (asset_kind == ResourceAssetKind::RoomPayload &&
      spec.resource_offset >= bytes.size()) {
    return std::nullopt;
  }

  return spec;
}

std::optional<FrdataRoomEntry>
decode_frdata_room_entry(std::span<const std::uint8_t> bytes,
                         std::size_t offset) {
  constexpr std::size_t kFrdataRoomEntrySize = 6;

  if (bytes.size() < kFrdataRoomEntrySize ||
      offset > bytes.size() - kFrdataRoomEntrySize) {
    return std::nullopt;
  }

  FrdataRoomEntry entry;
  entry.tile_w =
      static_cast<std::uint16_t>(bytes[offset] | (bytes[offset + 1] << 8));
  entry.tile_h =
      static_cast<std::uint16_t>(bytes[offset + 2] | (bytes[offset + 3] << 8));
  entry.rle_data_off =
      static_cast<std::uint16_t>(bytes[offset + 4] | (bytes[offset + 5] << 8));
  return entry;
}

std::optional<std::vector<std::uint16_t>>
build_room_row_pointer_table(std::span<const std::uint8_t> decoded_room_bytes,
                             std::uint16_t tile_h) {
  const std::size_t table_size = static_cast<std::size_t>(tile_h) * 2;
  if (kRoomRowPointerMapOffset > decoded_room_bytes.size() ||
      decoded_room_bytes.size() - kRoomRowPointerMapOffset < table_size) {
    return std::nullopt;
  }

  std::vector<std::uint16_t> row_pointers(tile_h, 0);
  for (std::uint16_t y = 0; y < tile_h; ++y) {
    const std::size_t off =
        kRoomRowPointerMapOffset + static_cast<std::size_t>(y) * 2;
    row_pointers[y] = static_cast<std::uint16_t>(
        decoded_room_bytes[off] | (decoded_room_bytes[off + 1] << 8));
  }
  return row_pointers;
}

std::optional<std::vector<std::uint8_t>>
extract_room_tile_data(std::span<const std::uint8_t> decoded_room_bytes,
                       std::uint16_t tile_w, std::uint16_t tile_h) {
  const std::size_t tile_count =
      static_cast<std::size_t>(tile_w) * static_cast<std::size_t>(tile_h);
  if (tile_count == 0 || tile_count > decoded_room_bytes.size()) {
    return std::nullopt;
  }

  return std::vector<std::uint8_t>(decoded_room_bytes.begin(),
                                   decoded_room_bytes.begin() +
                                       static_cast<std::ptrdiff_t>(tile_count));
}

bool validate_room_tile_indices_for_tileset(
    std::span<const std::uint8_t> room_tile_data,
    std::size_t tileset_tile_count) {
  if (tileset_tile_count == 0) {
    return true;
  }

  return std::all_of(room_tile_data.begin(), room_tile_data.end(),
                     [tileset_tile_count](std::uint8_t tile_index) {
                       return static_cast<std::size_t>(tile_index) <
                              tileset_tile_count;
                     });
}

std::optional<std::vector<MappedObject12>>
decode_room_mapped_objects(std::span<const std::uint8_t> decoded_room_bytes) {
  if (decoded_room_bytes.size() < kMappedObjectCountOffset + 2) {
    return std::vector<MappedObject12>{};
  }

  const std::uint16_t count =
      read_u16(decoded_room_bytes, kMappedObjectCountOffset);
  if (count == 0 || count == kSentinelEntry) {
    return std::vector<MappedObject12>{};
  }

  const std::size_t needed_size =
      kMappedObjectTableOffset +
      static_cast<std::size_t>(count) * kMappedObjectRecordSize;
  if (needed_size > decoded_room_bytes.size()) {
    return std::nullopt;
  }

  std::vector<MappedObject12> mapped_objects;
  mapped_objects.reserve(count);
  for (std::size_t i = 0; i < count; ++i) {
    const std::size_t off =
        kMappedObjectTableOffset + i * kMappedObjectRecordSize;

    MappedObject12 obj{};
    obj.room_x = read_u16(decoded_room_bytes, off + 0);
    obj.room_y = read_u16(decoded_room_bytes, off + 2);
    obj.descriptor_ptr = read_u16(decoded_room_bytes, off + 4);
    obj.state_flags = read_u16(decoded_room_bytes, off + 6);
    obj.world_x = read_u16(decoded_room_bytes, off + 8);
    obj.world_y = read_u16(decoded_room_bytes, off + 10);

    if (obj.room_x == kSentinelEntry && obj.room_y == kSentinelEntry &&
        obj.descriptor_ptr == kSentinelEntry &&
        obj.state_flags == kSentinelEntry && obj.world_x == kSentinelEntry &&
        obj.world_y == kSentinelEntry) {
      continue;
    }

    mapped_objects.push_back(obj);
  }

  return mapped_objects;
}

std::vector<std::uint16_t>
build_linear_row_pointer_table(std::uint16_t tile_w, std::uint16_t tile_h) {
  std::vector<std::uint16_t> row_pointers(tile_h, 0);
  for (std::uint16_t y = 0; y < tile_h; ++y) {
    row_pointers[y] = static_cast<std::uint16_t>(
        static_cast<std::size_t>(y) * static_cast<std::size_t>(tile_w));
  }
  return row_pointers;
}

std::uint16_t
resolve_synthesized_entity_behavior_state(std::uint16_t descriptor_ptr) {
  switch (descriptor_ptr) {
  case 0x8091U:
  case 0x8700U:
  case 0x9936U:
  case 0xCDF7U:
  case 0xCE17U:
  case 0xCE27U:
    return kBehaviorChase;
  case 0x80A1U:
  case 0x80D1U:
  case 0x8740U:
    return kBehaviorBounce;
  case 0x80B1U:
  case 0x80C1U:
  case 0x8720U:
  case 0x8730U:
  case 0x9CF2U:
    return kBehaviorJump;
  case 0x80E1U:
  case 0x8750U:
  case 0x9501U:
  case 0xAFBBU:
    return kBehaviorGravity;
  case 0x80F1U:
  case 0x8101U:
  case 0x8710U:
  case 0x8760U:
  case 0x9103U:
  case 0x92DAU:
  case 0x92EAU:
  case 0x9511U:
  case 0x9946U:
  case 0x9D22U:
  case 0x9D32U:
  case 0xA319U:
  case 0xA93AU:
  case 0xAF9BU:
  case 0xAFABU:
  case 0xAFCBU:
  case 0xAFEBU:
  case 0xB938U:
  case 0xBF7BU:
  case 0xC71CU:
  case 0xCE87U:
  case 0xCE97U:
  case 0xCEA7U:
    return kBehaviorShoot;
  case 0x8D98U:
    return kBehaviorBoss;
  case 0x8DA8U:
  case 0xA94AU:
    return kBehaviorAsm09_HeavyLeaper;
  case 0x90F3U:
  case 0xA309U:
    return kBehaviorAsm10_GhostPhaser;
  case 0x94F1U:
  case 0xA2E9U:
  case 0xCE07U:
    return kBehaviorAsm11_HighBouncer;
  case 0x9926U:
    return kBehaviorAsm12_CeilingDropHazard;
  case 0xA2F9U:
  case 0xB928U:
  case 0xBF6BU:
    return kBehaviorAsm13_FastLedgePatrol;
  case 0x9D02U:
    return kBehaviorAsm14_DirectionLockedWalker;
  case 0x9D12U:
    return kBehaviorAsm15_GroundPounder;
  case 0xA2C9U:
  case 0xAF7BU:
    return kBehaviorAsm16_DiagonalBouncer;
  case 0xA2D9U:
  case 0xCDD7U:
  case 0xCE67U:
    return kBehaviorAsm17_TrampolineJumper;
  case 0xA95AU:
    return kBehaviorAsm18_WallJumpingRunner;
  case 0xA96AU:
    return kBehaviorAsm19_DropTrap1;
  case 0xA97AU:
    return kBehaviorAsm20_SpikeDropTrap2;
  case 0xAF5BU:
  case 0xAF6BU:
  case 0xAF8BU:
  case 0xBF5BU:
  case 0xC6FCU:
    return kBehaviorAsm21_HovercraftOscillator;
  case 0xAFDBU:
  case 0xB918U:
  case 0xCE37U:
    return kBehaviorAsm22_TwoPhaseDrone;
  case 0xB908U:
    return kBehaviorAsm23_FastRammer;
  case 0xBF1BU:
  case 0xBF2BU:
  case 0xBF3BU:
  case 0xBF4BU:
    return kBehaviorAsm24_CeilingDropGhost;
  case 0x9956U:
    return kBehaviorAsm25_InvertedCeilingCrawler;
  case 0xC6ECU:
    return kBehaviorAsm26_StraightHorizCruiser;
  case 0xC6CCU:
    return kBehaviorAsm27_BossWalker;
  case 0xC70CU:
    return kBehaviorAsm28_BossSwooper;
  case 0xC6DCU:
    return kBehaviorAsm29_LeftwardDart;
  case 0xCDE7U:
    return kBehaviorAsm30_TorpedoSprinter;
  case 0xCE77U:
    return kBehaviorAsm31_HeavyStalactite;
  case 0xCE47U:
    return kBehaviorAsm32_LeftSwoopDiver;
  case 0xCE57U:
    return kBehaviorAsm33_RightSwoopDiver;
  default:
    if (descriptor_ptr != 0U && descriptor_ptr <= 3U) {
      return kBehaviorJump;
    }
    if (descriptor_ptr >= 4U && descriptor_ptr <= 7U) {
      return kBehaviorBounce;
    }
    return static_cast<std::uint16_t>(kBehaviorChase +
                                      ((descriptor_ptr >> 4U) % 3U));
  }
}

std::vector<MappedObject12>
build_mapped_objects_from_level_data(const LevelRuntimeData *level_data,
                                     std::uint16_t level, std::uint16_t room) {
  std::vector<MappedObject12> objects;
  if (level_data == nullptr) {
    return objects;
  }

  if (room < level_data->active_enemies.size()) {
    for (const auto &enemy : level_data->active_enemies[room]) {
      MappedObject12 obj{};
      obj.room_x = room;
      obj.room_y = level;
      obj.descriptor_ptr = enemy.flags != 0U ? enemy.flags : 0x8000U;
      obj.state_flags =
          resolve_synthesized_entity_behavior_state(obj.descriptor_ptr);
      obj.world_x = static_cast<std::uint16_t>(enemy.x);
      obj.world_y = static_cast<std::uint16_t>(enemy.y);
      objects.push_back(obj);
    }
  }

  if (objects.empty() && room < level_data->special_entities.size()) {
    for (const auto &special : level_data->special_entities[room]) {
      if (special.x == 0 && special.y == 0 && special.flags == 0) {
        continue;
      }
      MappedObject12 obj{};
      obj.room_x = room;
      obj.room_y = level;
      obj.descriptor_ptr = special.flags != 0U ? special.flags : 0x8000U;
      obj.state_flags =
          resolve_synthesized_entity_behavior_state(obj.descriptor_ptr);
      obj.world_x = static_cast<std::uint16_t>(special.x);
      obj.world_y = static_cast<std::uint16_t>(special.y);
      objects.push_back(obj);
    }
  }

  if (objects.empty()) {
    // Authentic static mapped object entries from CS:0x0AF0 (GAME.000) for
    // rooms whose items live exclusively in the global mapped object table.
    if (level == 1U && room == 7U) {
      const std::uint16_t state_flags =
          resolve_synthesized_entity_behavior_state(0x9E04U);
      objects.push_back(MappedObject12{.room_x = room,
                                       .room_y = level,
                                       .descriptor_ptr = 0x9E04U,
                                       .state_flags = state_flags,
                                       .world_x = 96U,
                                       .world_y = 128U});
      objects.push_back(MappedObject12{.room_x = room,
                                       .room_y = level,
                                       .descriptor_ptr = 0x9E04U,
                                       .state_flags = state_flags,
                                       .world_x = 96U,
                                       .world_y = 48U});
    } else if (level == 9U && room == 1U) {
      objects.push_back(MappedObject12{
          .room_x = room,
          .room_y = level,
          .descriptor_ptr = 0xB286U,
          .state_flags = resolve_synthesized_entity_behavior_state(0xB286U),
          .world_x = 1952U,
          .world_y = 32U});
    }
  }

  return objects;
}

void load_room_effect_script_and_trigger_ranges(
    RuntimeState &state, bool allow_fallback_anchor = true) {
  state.room_event_anchor = RoomEventAnchorState{};
  state.room_event_script = RoomEventScriptState{};

  state.room_event_script.loaded =
      !state.room_resource_bytes.empty() || !state.level_script_bytes.empty();
  state.room_event_script.raw_script_bytes = state.room_resource_bytes;
  state.room_event_script.raw_trigger_bytes = state.room_resource_bytes;
  state.room_event_script.decoded_script_bytes = state.level_script_bytes;

  if (!allow_fallback_anchor || state.mapped_objects.empty()) {
    return;
  }

  const auto event_object =
      std::find_if(state.mapped_objects.begin(), state.mapped_objects.end(),
                   [](const MappedObject12 &object) {
                     return (object.state_flags & 0x0001U) != 0U;
                   });

  const MappedObject12 &anchor_source =
      event_object != state.mapped_objects.end() ? *event_object
                                                 : state.mapped_objects.front();

  state.room_event_anchor.active = true;
  state.room_event_anchor.x = static_cast<std::int16_t>(anchor_source.world_x);
  state.room_event_anchor.y = static_cast<std::int16_t>(anchor_source.world_y);
  state.room_event_anchor.descriptor_ptr = anchor_source.descriptor_ptr;
  state.room_event_anchor.velocity_x =
      static_cast<std::int16_t>((state.current_room % 2U) == 0U ? 1 : -1);
  state.room_event_anchor.velocity_y =
      static_cast<std::int16_t>((state.current_level % 2U) == 0U ? 1 : -1);
}

bool load_room_tilemap_from_resource_buffer(RuntimeState &state,
                                            std::span<const std::uint8_t> bytes,
                                            std::uint16_t level,
                                            std::uint16_t room) {
  const std::optional<RoomLoadSpec> spec = resolve_room_load_spec(
      {}, bytes, level, room, ResourceAssetKind::RoomPayload);
  if (!spec.has_value()) {
    return false;
  }

  SignedRleResult decoded;
  try {
    decoded = decode_signed_rle(bytes.subspan(spec->resource_offset));
  } catch (...) {
    return false;
  }
  const std::size_t tile_count =
      static_cast<std::size_t>(spec->room_entry.tile_w) *
      static_cast<std::size_t>(spec->room_entry.tile_h);
  const bool has_embedded_metadata = decoded.bytes.size() > tile_count;
  const std::optional<std::vector<std::uint16_t>> row_pointers =
      has_embedded_metadata
          ? build_room_row_pointer_table(decoded.bytes, spec->room_entry.tile_h)
          : std::optional<std::vector<std::uint16_t>>(
                build_linear_row_pointer_table(spec->room_entry.tile_w,
                                               spec->room_entry.tile_h));
  if (!row_pointers.has_value()) {
    return false;
  }

  const auto room_tile_data = extract_room_tile_data(
      decoded.bytes, spec->room_entry.tile_w, spec->room_entry.tile_h);
  if (!room_tile_data.has_value()) {
    return false;
  }
  if (!validate_room_tile_indices_for_tileset(*room_tile_data,
                                              state.level_tileset.size())) {
    return false;
  }

  state.current_level = level;
  state.current_room = room;
  state.level_data = get_level_runtime_data(level);
  state.room_entry = spec->room_entry;
  state.room_grid.tile_w = spec->room_entry.tile_w;
  state.room_grid.tile_h = spec->room_entry.tile_h;
  state.room_grid.row_pointers = *row_pointers;
  state.room_grid.tile_data = *room_tile_data;
  std::vector<std::uint8_t> resource_bytes(bytes.begin(), bytes.end());
  state.room_resource_bytes = std::move(resource_bytes);

  if (has_embedded_metadata) {
    const auto mapped_objects = decode_room_mapped_objects(decoded.bytes);
    state.mapped_objects =
        mapped_objects.value_or(std::vector<MappedObject12>{});
  } else {
    state.mapped_objects =
        build_mapped_objects_from_level_data(state.level_data, level, room);
  }
  hydrate_entity_runtime_for_room(state);
  load_room_effect_script_and_trigger_ranges(state, has_embedded_metadata);

  return true;
}

bool load_room_tilemap_from_resource_file(
    RuntimeState &state, const std::filesystem::path &source_path,
    std::span<const std::uint8_t> bytes, std::uint16_t level,
    std::uint16_t room) {
  const std::optional<RoomLoadSpec> spec = resolve_room_load_spec(
      source_path, bytes, level, room, ResourceAssetKind::RoomPayload);
  if (!spec.has_value()) {
    return false;
  }

  SignedRleResult decoded;
  try {
    decoded = decode_signed_rle(bytes.subspan(spec->resource_offset));
  } catch (...) {
    return false;
  }
  const std::size_t tile_count =
      static_cast<std::size_t>(spec->room_entry.tile_w) *
      static_cast<std::size_t>(spec->room_entry.tile_h);
  const bool has_embedded_metadata = decoded.bytes.size() > tile_count;
  const std::optional<std::vector<std::uint16_t>> row_pointers =
      has_embedded_metadata
          ? build_room_row_pointer_table(decoded.bytes, spec->room_entry.tile_h)
          : std::optional<std::vector<std::uint16_t>>(
                build_linear_row_pointer_table(spec->room_entry.tile_w,
                                               spec->room_entry.tile_h));
  if (!row_pointers.has_value()) {
    return false;
  }

  const auto room_tile_data = extract_room_tile_data(
      decoded.bytes, spec->room_entry.tile_w, spec->room_entry.tile_h);
  if (!room_tile_data.has_value()) {
    return false;
  }
  if (!validate_room_tile_indices_for_tileset(*room_tile_data,
                                              state.level_tileset.size())) {
    return false;
  }

  state.current_level = level;
  state.current_room = room;
  state.level_data = get_level_runtime_data(level);
  state.room_entry = spec->room_entry;
  state.room_grid.tile_w = spec->room_entry.tile_w;
  state.room_grid.tile_h = spec->room_entry.tile_h;
  state.room_grid.row_pointers = *row_pointers;
  state.room_grid.tile_data = *room_tile_data;
  std::vector<std::uint8_t> resource_bytes(bytes.begin(), bytes.end());
  state.room_resource_bytes = std::move(resource_bytes);

  if (has_embedded_metadata) {
    const auto mapped_objects = decode_room_mapped_objects(decoded.bytes);
    state.mapped_objects =
        mapped_objects.value_or(std::vector<MappedObject12>{});
  } else {
    state.mapped_objects =
        build_mapped_objects_from_level_data(state.level_data, level, room);
  }
  hydrate_entity_runtime_for_room(state);
  load_room_effect_script_and_trigger_ranges(state, has_embedded_metadata);

  return true;
}

bool try_load_room_payload_from_file(RuntimeState &state,
                                     const std::filesystem::path &source_path,
                                     std::uint16_t level, std::uint16_t room) {
  const auto bytes = load_file_bytes(source_path);
  if (!bytes.has_value()) {
    return false;
  }

  RuntimeState candidate_state = state;
  if (load_room_tilemap_from_resource_file(candidate_state, source_path, *bytes,
                                           level, room)) {
    candidate_state.room_resource_bytes = *bytes;
    state = std::move(candidate_state);
    return true;
  }
  return false;
}

bool load_room_tilemap_from_asset_root(RuntimeState &state,
                                       const std::filesystem::path &root,
                                       std::uint16_t level,
                                       std::uint16_t room) {
  const auto tuple = resolve_level_resource_tuple(level, root);
  if (tuple.has_value()) {
    const auto exact_room_path = root / tuple->room_layout_filename;
    RuntimeState candidate_state = state;
    if (try_load_level_tileset_for_root(candidate_state, root, level) &&
        try_load_room_payload_from_file(candidate_state, exact_room_path, level,
                                        room)) {
      state = std::move(candidate_state);
      return true;
    }
  }

  const auto candidates = discover_room_payload_candidates(root);
  return std::any_of(candidates.begin(), candidates.end(),
                     [&](const std::filesystem::path &candidate) {
                       return try_load_room_payload_from_file(state, candidate,
                                                              level, room);
                     });
}

std::optional<std::vector<std::uint8_t>>
decode_level_script(std::span<const std::uint8_t> payload) {
  if (payload.size() != kLevelScriptBlockSize) {
    return std::nullopt;
  }
  std::vector<std::uint8_t> out(payload.size());
  std::transform(payload.begin(), payload.end(), out.begin(),
                 [](std::uint8_t b) {
                   return static_cast<std::uint8_t>(b ^ kLevelScriptXorKey);
                 });
  return out;
}

std::optional<std::vector<std::uint8_t>>
decode_level_script_payload(std::span<const std::uint8_t> payload) {
  if (payload.empty() || payload.size() > kLevelScriptBlockSize) {
    return std::nullopt;
  }
  if (payload.size() == kLevelScriptBlockSize) {
    return decode_level_script(payload);
  }
  std::vector<std::uint8_t> block(kLevelScriptBlockSize, kLevelScriptXorKey);
  std::copy(payload.begin(), payload.end(), block.begin());
  return decode_level_script(block);
}

std::optional<std::uint16_t>
level_script_offset(std::span<const std::uint8_t> decoded, std::size_t index) {
  const std::size_t at = index * 2;
  if (at + 2 > decoded.size()) {
    return std::nullopt;
  }
  return static_cast<std::uint16_t>(decoded[at] | (decoded[at + 1] << 8));
}

std::optional<ScriptDialogMessage>
extract_level_script_message(std::span<const std::uint8_t> decoded_script,
                             std::size_t message_index) {
  const auto first_off_opt = level_script_offset(decoded_script, 0);
  if (!first_off_opt.has_value()) {
    return std::nullopt;
  }
  const std::size_t first_off = *first_off_opt;
  if (first_off < 2 || first_off > decoded_script.size() ||
      (first_off % 2) != 0) {
    return std::nullopt;
  }
  const std::size_t max_messages = first_off / 2;
  if (message_index >= max_messages) {
    return std::nullopt;
  }

  const auto msg_off_opt = level_script_offset(decoded_script, message_index);
  if (!msg_off_opt.has_value()) {
    return std::nullopt;
  }
  const std::size_t msg_off = *msg_off_opt;
  if (msg_off < first_off || msg_off + 2 > decoded_script.size()) {
    return std::nullopt;
  }

  const std::uint8_t w = decoded_script[msg_off];
  const std::uint8_t h = decoded_script[msg_off + 1];
  if (w < 3 || h < 3 || w > 40 || h > 25) {
    return std::nullopt;
  }

  const std::size_t inner_w = static_cast<std::size_t>(w - 2);
  const std::size_t inner_h = static_cast<std::size_t>(h - 2);
  const std::size_t char_count = inner_w * inner_h;
  if (msg_off + 2 + char_count > decoded_script.size()) {
    return std::nullopt;
  }

  ScriptDialogMessage msg{};
  msg.width_cells = w;
  msg.height_cells = h;
  msg.lines.reserve(inner_h);

  const auto chars = decoded_script.subspan(msg_off + 2, char_count);
  for (std::size_t r = 0; r < inner_h; ++r) {
    const auto row_span = chars.subspan(r * inner_w, inner_w);
    std::string line(reinterpret_cast<const char *>(row_span.data()), inner_w);
    msg.lines.push_back(line);
    if (r > 0) {
      msg.text.push_back('\n');
    }
    msg.text.append(line);
  }

  return msg;
}

std::optional<TileTriggerRange>
parse_tile_trigger_range(std::span<const std::uint8_t> bytes,
                         std::size_t offset) {
  if (offset + 2 > bytes.size()) {
    return std::nullopt;
  }
  TileTriggerRange r;
  r.min_tile = bytes[offset];
  r.max_tile = bytes[offset + 1];
  if (r.max_tile < r.min_tile) {
    return std::nullopt;
  }
  const std::size_t count =
      static_cast<std::size_t>(r.max_tile - r.min_tile) + 1;
  if (offset + 2 + count > bytes.size()) {
    return std::nullopt;
  }
  r.values.assign(bytes.begin() + offset + 2,
                  bytes.begin() + offset + 2 + count);
  return r;
}

std::optional<std::uint8_t> evaluate_tile_trigger(const TileTriggerRange &range,
                                                  std::uint8_t tile_id) {
  if (tile_id < range.min_tile || tile_id > range.max_tile) {
    return std::nullopt;
  }
  const std::size_t idx = tile_id - range.min_tile;
  if (idx >= range.values.size()) {
    return std::nullopt;
  }
  return range.values[idx];
}

} // namespace comic2
