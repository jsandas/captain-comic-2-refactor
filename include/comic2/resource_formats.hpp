#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace comic2 {

inline constexpr std::size_t kEgaPlaneCount = 4;

#pragma pack(push, 1)

struct EgaRle4PlaneHeader {
  std::uint16_t row_span_bytes;
};

struct OpaqueSpriteRecordHeader {
  std::uint16_t width_pixels;
  std::uint16_t height_rows;
};

struct MaskedSpriteRecordHeader {
  std::uint16_t width_pixels;
  std::uint16_t height_rows;
  std::uint16_t image_data_off;
};

#pragma pack(pop)

struct Ega4PlaneImage {
  std::uint16_t row_span_bytes =
      0; // total decoded bytes per plane (from file header)
  std::uint16_t width_bytes =
      0; // row stride in bytes (must be set by caller for blitting)
  std::uint16_t height_rows =
      0; // image height in rows (must be set by caller for blitting)
  std::array<std::vector<std::uint8_t>, kEgaPlaneCount> planes;

  bool operator==(const Ega4PlaneImage &) const = default;
};

struct MaskedSpriteRecord {
  std::uint16_t width_pixels = 0;
  std::uint16_t height_rows = 0;
  std::uint16_t image_data_off = 0;
  std::vector<std::uint8_t> mask_bytes;
  std::array<std::vector<std::uint8_t>, kEgaPlaneCount> planes;

  std::size_t mask_byte_count() const {
    return static_cast<std::size_t>(width_pixels / 8U) *
           static_cast<std::size_t>(height_rows);
  }

  bool operator==(const MaskedSpriteRecord &) const = default;
};

struct LevelResourceTuple {
  std::uint16_t level_id = 0;
  std::string tileset_filename;
  std::string room_layout_filename;
  std::string sprite_sheet_filename;
  std::string script_filename;

  bool has_script() const { return !script_filename.empty(); }

  bool operator==(const LevelResourceTuple &) const = default;
};

struct LevelTilesetDecode {
  std::array<std::uint16_t, 2> hazard_bounds{0, 0};
  std::vector<Ega4PlaneImage> tiles;

  bool operator==(const LevelTilesetDecode &) const = default;
};

struct SignedRleResult {
  std::vector<std::uint8_t> bytes;
  std::size_t consumed = 0;
};

struct FrpakCatalogRecord {
  std::uint16_t pak_id = 0;
  std::uint16_t record_id = 0;
  std::size_t data_offset = 0;
  std::size_t data_size = 0;
  std::uint16_t row_span_bytes = 0;

  bool operator==(const FrpakCatalogRecord &) const = default;
};

struct FrpakCatalogFile {
  std::uint16_t pak_id = 0;
  std::filesystem::path source_path;
  std::size_t file_size = 0;
  std::size_t blob_offset = 0;
  std::vector<FrpakCatalogRecord> records;

  bool operator==(const FrpakCatalogFile &) const = default;
};

struct FrpakCatalog {
  std::vector<FrpakCatalogFile> files;

  bool operator==(const FrpakCatalog &) const = default;
};

struct FrpakDecodedRecordCacheEntry {
  std::uint16_t pak_id = 0;
  std::uint16_t record_id = 0;
  Ega4PlaneImage image;

  bool operator==(const FrpakDecodedRecordCacheEntry &) const = default;
};

SignedRleResult decode_signed_rle(std::span<const std::uint8_t> encoded);
Ega4PlaneImage decode_ega_4plane_rle(std::span<const std::uint8_t> encoded,
                                     bool require_full_consumption = true);

} // namespace comic2
