#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <vector>

#include "comic2/game_state.hpp"
#include "comic2/resource_loader.hpp"
#include "comic2/types.hpp"

namespace comic2 {

struct RoomTableHeader {
  std::uint16_t room_count = 0;
  std::uint16_t level_id = 0;

  bool operator==(const RoomTableHeader &) const = default;
};

std::optional<RoomTableHeader>
parse_room_table_header(std::span<const std::uint8_t> bytes);

struct RoomLoadSpec {
  std::filesystem::path source_path;
  std::uint16_t level = 0;
  std::uint16_t room = 0;
  std::uint16_t room_count = 0;
  std::uint16_t level_id = 0;
  ResourceAssetKind asset_kind = ResourceAssetKind::RoomPayload;
  std::size_t table_offset = 0x04;
  std::size_t room_entry_offset = 0;
  std::size_t resource_offset = 0;
  FrdataRoomEntry room_entry{};

  bool operator==(const RoomLoadSpec &) const = default;
};

std::optional<RoomLoadSpec> resolve_room_load_spec(
    const std::filesystem::path &source_path,
    std::span<const std::uint8_t> bytes, std::uint16_t level,
    std::uint16_t room,
    ResourceAssetKind asset_kind = ResourceAssetKind::RoomPayload);

std::optional<FrdataRoomEntry>
decode_frdata_room_entry(std::span<const std::uint8_t> bytes,
                         std::size_t offset);

std::optional<std::vector<std::uint16_t>>
build_room_row_pointer_table(std::span<const std::uint8_t> decoded_room_bytes,
                             std::uint16_t tile_h);

std::optional<std::vector<MappedObject12>>
decode_room_mapped_objects(std::span<const std::uint8_t> decoded_room_bytes);

// FR###.3 payload (0xC00 bytes) is XOR-0x25 encoded (io_read_xor25_block_9ec0).
inline constexpr std::size_t kLevelScriptBlockSize = 0x0C00;
inline constexpr std::uint8_t kLevelScriptXorKey = 0x25;

std::vector<std::uint8_t>
decode_level_script(std::span<const std::uint8_t> payload);

// Script block starts with u16 offsets (relative to block start), indexed
// by message id (see ent_process_interaction_script_and_queue_message).
std::optional<std::uint16_t>
level_script_offset(std::span<const std::uint8_t> decoded, std::size_t index);

bool load_room_tilemap_from_resource_buffer(RuntimeState &state,
                                            std::span<const std::uint8_t> bytes,
                                            std::uint16_t level,
                                            std::uint16_t room);

bool load_room_tilemap_from_resource_file(
    RuntimeState &state, const std::filesystem::path &source_path,
    std::span<const std::uint8_t> bytes, std::uint16_t level,
    std::uint16_t room);

bool load_room_tilemap_from_asset_root(RuntimeState &state,
                                       const std::filesystem::path &root,
                                       std::uint16_t level, std::uint16_t room);

} // namespace comic2
