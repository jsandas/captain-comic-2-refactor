#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <vector>

#include "comic2/game_state.hpp"
#include "comic2/resource_formats.hpp"
#include "comic2/types.hpp"

namespace comic2 {

enum class ResourceAssetKind {
  RoomTable,
  RoomPayload,
};

struct BootstrapLoadSummary {
  std::size_t metadata_files_tried = 0;
  std::size_t sprite_files_tried = 0;
  bool room_grid_loaded = false;
};

std::optional<std::vector<std::uint8_t>>
load_file_bytes(const std::filesystem::path &path);

std::optional<FrpakCatalogFile>
build_frpak_catalog_file(const std::filesystem::path &source_path,
                         std::span<const std::uint8_t> bytes,
                         std::uint16_t pak_id);

std::optional<FrpakCatalogRecord>
find_frpak_catalog_record(const FrpakCatalog &catalog, std::uint16_t pak_id,
                          std::uint16_t record_id);

std::optional<LevelResourceTuple>
resolve_level_resource_tuple(std::uint16_t level_id,
                             const std::filesystem::path &root);

std::optional<LevelTilesetDecode>
decode_level_tileset(std::span<const std::uint8_t> payload);

std::optional<std::vector<MaskedSpriteRecord>>
decode_masked_sprite_sheet(std::span<const std::uint8_t> payload);

bool validate_frpak_catalog_record_bounds(const FrpakCatalogRecord &record,
                                          std::size_t file_size);

std::optional<Ega4PlaneImage>
decode_frpak_catalog_record(const RuntimeState &state,
                            const FrpakCatalogRecord &record);

std::optional<Ega4PlaneImage> decode_frpak_record(RuntimeState &state,
                                                  std::uint16_t pak_id,
                                                  std::uint16_t record_id);

void clear_frpak_decode_cache(RuntimeState &state);

BootstrapLoadSummary
load_initial_bootstrap_resources(RuntimeState &state,
                                 const std::filesystem::path &root);

} // namespace comic2
