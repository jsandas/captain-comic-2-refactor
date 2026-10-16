#include "comic2/resource_loader.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "comic2/bootstrap.hpp"
#include "comic2/room_loader.hpp"

namespace comic2 {
namespace {

constexpr std::size_t kMaxDecodedBytes = 0x10000;
constexpr std::uint16_t kFrpakMinPakId = 1;
constexpr std::uint16_t kFrpakMaxPakId = 999;
constexpr std::size_t kTilesetTileBytes = 128;

const std::array<LevelResourceTuple, 15> &level_resource_catalog() {
  static const std::array<LevelResourceTuple, 15> kTuples = {
      LevelResourceTuple{.level_id = 0,
                         .tileset_filename = "FR000.0",
                         .room_layout_filename = "FR000.1",
                         .sprite_sheet_filename = "FR000.2",
                         .script_filename = "FR004.3"},
      LevelResourceTuple{.level_id = 1,
                         .tileset_filename = "FR001.0",
                         .room_layout_filename = "FR001.1",
                         .sprite_sheet_filename = "FR001.2",
                         .script_filename = "FR001.3"},
      LevelResourceTuple{.level_id = 2,
                         .tileset_filename = "FR002.0",
                         .room_layout_filename = "FR002.1",
                         .sprite_sheet_filename = "FR002.2",
                         .script_filename = ""},
      LevelResourceTuple{.level_id = 3,
                         .tileset_filename = "FR003.0",
                         .room_layout_filename = "FR003.1",
                         .sprite_sheet_filename = "FR003.2",
                         .script_filename = "FR003.3"},
      LevelResourceTuple{.level_id = 4,
                         .tileset_filename = "FR001.0",
                         .room_layout_filename = "FR004.1",
                         .sprite_sheet_filename = "FR004.2",
                         .script_filename = "FR004.3"},
      LevelResourceTuple{.level_id = 5,
                         .tileset_filename = "FR005.0",
                         .room_layout_filename = "FR005.1",
                         .sprite_sheet_filename = "FR005.2",
                         .script_filename = "FR005.3"},
      LevelResourceTuple{.level_id = 6,
                         .tileset_filename = "FR006.0",
                         .room_layout_filename = "FR006.1",
                         .sprite_sheet_filename = "FR006.2",
                         .script_filename = "FR006.3"},
      LevelResourceTuple{.level_id = 7,
                         .tileset_filename = "FR007.0",
                         .room_layout_filename = "FR007.1",
                         .sprite_sheet_filename = "FR007.2",
                         .script_filename = ""},
      LevelResourceTuple{.level_id = 8,
                         .tileset_filename = "FR008.0",
                         .room_layout_filename = "FR008.1",
                         .sprite_sheet_filename = "FR008.2",
                         .script_filename = ""},
      LevelResourceTuple{.level_id = 9,
                         .tileset_filename = "FR009.0",
                         .room_layout_filename = "FR009.1",
                         .sprite_sheet_filename = "FR009.2",
                         .script_filename = ""},
      LevelResourceTuple{.level_id = 10,
                         .tileset_filename = "FR010.0",
                         .room_layout_filename = "FR010.1",
                         .sprite_sheet_filename = "FR010.2",
                         .script_filename = "FR006.3"},
      LevelResourceTuple{.level_id = 11,
                         .tileset_filename = "FR011.0",
                         .room_layout_filename = "FR011.1",
                         .sprite_sheet_filename = "FR011.2",
                         .script_filename = "FR006.3"},
      LevelResourceTuple{.level_id = 12,
                         .tileset_filename = "FR012.0",
                         .room_layout_filename = "FR012.1",
                         .sprite_sheet_filename = "FR012.2",
                         .script_filename = ""},
      LevelResourceTuple{.level_id = 13,
                         .tileset_filename = "FR013.0",
                         .room_layout_filename = "FR013.1",
                         .sprite_sheet_filename = "FR013.2",
                         .script_filename = "FR013.3"},
      LevelResourceTuple{.level_id = 14,
                         .tileset_filename = "FR014.0",
                         .room_layout_filename = "FR014.1",
                         .sprite_sheet_filename = "FR014.2",
                         .script_filename = "FR013.3"},
  };
  return kTuples;
}

std::uint16_t read_u16(std::span<const std::uint8_t> bytes, std::size_t off) {
  if (off + 1 >= bytes.size()) {
    throw std::runtime_error("read_u16 out of range");
  }
  return static_cast<std::uint16_t>(
      bytes[off] | (static_cast<std::uint16_t>(bytes[off + 1]) << 8));
}

bool path_exists(const std::filesystem::path &path) {
  return std::filesystem::exists(path) &&
         std::filesystem::is_regular_file(path);
}

std::optional<std::uint16_t>
parse_frpak_id_from_path(const std::filesystem::path &path) {
  const std::string ext = path.extension().string();
  if (ext.size() != 4 || ext[0] != '.') {
    return std::nullopt;
  }

  if (ext[1] < '0' || ext[1] > '9' || ext[2] < '0' || ext[2] > '9' ||
      ext[3] < '0' || ext[3] > '9') {
    return std::nullopt;
  }

  const auto pak_id = static_cast<std::uint16_t>(
      (ext[1] - '0') * 100 + (ext[2] - '0') * 10 + (ext[3] - '0'));
  if (pak_id < kFrpakMinPakId || pak_id > kFrpakMaxPakId) {
    return std::nullopt;
  }

  return pak_id;
}

const FrpakCatalogFile *find_frpak_catalog_file(const FrpakCatalog &catalog,
                                                std::uint16_t pak_id) {
  const auto it = std::find_if(
      catalog.files.begin(), catalog.files.end(),
      [pak_id](const FrpakCatalogFile &file) { return file.pak_id == pak_id; });
  return it != catalog.files.end() ? &(*it) : nullptr;
}

} // namespace

std::optional<std::vector<std::uint8_t>>
load_file_bytes(const std::filesystem::path &path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    return std::nullopt;
  }

  input.seekg(0, std::ios::end);
  const auto end_pos = input.tellg();
  if (end_pos < 0) {
    return std::nullopt;
  }
  const auto size = static_cast<std::size_t>(end_pos);
  input.seekg(0, std::ios::beg);
  if (!input) {
    return std::nullopt;
  }

  std::vector<std::uint8_t> bytes(size);
  if (size > 0) {
    input.read(reinterpret_cast<char *>(bytes.data()),
               static_cast<std::streamsize>(size));
    if (!input) {
      return std::nullopt;
    }
  }
  return bytes;
}

std::optional<FrpakCatalogFile>
build_frpak_catalog_file(const std::filesystem::path &source_path,
                         std::span<const std::uint8_t> bytes,
                         std::uint16_t pak_id) {
  if (bytes.size() < sizeof(EgaRle4PlaneHeader)) {
    return std::nullopt;
  }

  const std::uint16_t row_span = read_u16(bytes, 0);
  if (row_span == 0) {
    return std::nullopt;
  }
  constexpr std::uint16_t kMaxRowSpan = 0x7FFF;
  if (row_span > kMaxRowSpan) {
    return std::nullopt;
  }

  FrpakCatalogFile file;
  file.pak_id = pak_id;
  file.source_path = source_path;
  file.file_size = bytes.size();
  file.records.push_back(FrpakCatalogRecord{
      .pak_id = pak_id,
      .record_id = 0,
      .data_offset = 0,
      .data_size = bytes.size(),
      .row_span_bytes = row_span,
  });
  return file;
}

std::optional<FrpakCatalogRecord>
find_frpak_catalog_record(const FrpakCatalog &catalog, std::uint16_t pak_id,
                          std::uint16_t record_id) {
  const FrpakCatalogFile *file = find_frpak_catalog_file(catalog, pak_id);
  if (file == nullptr) {
    return std::nullopt;
  }

  const auto it = std::find_if(file->records.begin(), file->records.end(),
                               [record_id](const FrpakCatalogRecord &record) {
                                 return record.record_id == record_id;
                               });
  return it != file->records.end() ? std::optional<FrpakCatalogRecord>(*it)
                                   : std::nullopt;
}

bool validate_frpak_catalog_record_bounds(const FrpakCatalogRecord &record,
                                          std::size_t file_size) {
  if (record.data_offset > file_size) {
    return false;
  }
  return record.data_size <= (file_size - record.data_offset);
}

bool is_related_asset_root(const std::filesystem::path &lhs,
                           const std::filesystem::path &rhs) {
  if (lhs.empty() || rhs.empty()) {
    return false;
  }

  std::error_code ec_a;
  std::error_code ec_b;
  const auto left = std::filesystem::weakly_canonical(lhs, ec_a);
  const auto right = std::filesystem::weakly_canonical(rhs, ec_b);

  const auto canonical_left = ec_a ? lhs.lexically_normal() : left;
  const auto canonical_right = ec_b ? rhs.lexically_normal() : right;

  if (canonical_left == canonical_right) {
    return true;
  }

  std::filesystem::path cursor = canonical_left;
  for (;;) {
    if (cursor == canonical_right) {
      return true;
    }
    const auto parent = cursor.parent_path();
    if (parent == cursor || parent.empty()) {
      break;
    }
    cursor = parent;
  }

  cursor = canonical_right;
  for (;;) {
    if (cursor == canonical_left) {
      return true;
    }
    const auto parent = cursor.parent_path();
    if (parent == cursor || parent.empty()) {
      break;
    }
    cursor = parent;
  }

  return false;
}

std::vector<std::filesystem::path>
build_asset_root_candidates_for_level(const std::filesystem::path &root) {
  std::vector<std::filesystem::path> seeds;
  if (!root.empty()) {
    seeds.push_back(root);
  }

  const auto current = std::filesystem::current_path();
  if (!current.empty() && current != root &&
      is_related_asset_root(current, root)) {
    seeds.push_back(current);
  }

  std::vector<std::filesystem::path> candidates;
  auto add_candidate = [&](const std::filesystem::path &candidate) {
    if (candidate.empty()) {
      return;
    }

    std::error_code ec;
    const auto canonical = std::filesystem::weakly_canonical(candidate, ec);
    const auto key = ec ? candidate.lexically_normal() : canonical;
    if (std::any_of(candidates.begin(), candidates.end(),
                    [&](const auto &existing) { return existing == key; })) {
      return;
    }
    candidates.push_back(key);
  };

  for (const auto &seed : seeds) {
    std::filesystem::path cursor = seed;
    for (;;) {
      add_candidate(cursor);
      add_candidate(cursor / "reference" / "original");
      add_candidate(cursor / "original");

      const auto parent = cursor.parent_path();
      if (parent == cursor || parent.empty()) {
        break;
      }
      cursor = parent;
    }
  }

  return candidates;
}

std::optional<LevelResourceTuple>
resolve_level_resource_tuple(std::uint16_t level_id,
                             const std::filesystem::path &root) {
  if (level_id > 14) {
    return std::nullopt;
  }

  const auto tuple = level_resource_catalog()[level_id];
  const auto roots = build_asset_root_candidates_for_level(root);
  for (const auto &candidate_root : roots) {
    const auto has_required_files = [&](const std::string &filename) {
      if (filename.empty()) {
        return true;
      }
      return path_exists(candidate_root / filename);
    };

    if (!has_required_files(tuple.tileset_filename) ||
        !has_required_files(tuple.room_layout_filename) ||
        !has_required_files(tuple.sprite_sheet_filename)) {
      continue;
    }

    if (tuple.has_script() && !has_required_files(tuple.script_filename)) {
      continue;
    }

    return tuple;
  }

  return std::nullopt;
}

std::optional<LevelTilesetDecode>
decode_level_tileset(std::span<const std::uint8_t> payload) {
  if (payload.size() < 6) {
    return std::nullopt;
  }

  LevelTilesetDecode decoded{};
  decoded.hazard_bounds[0] = read_u16(payload, 2);
  decoded.hazard_bounds[1] = read_u16(payload, 4);

  try {
    const auto rle = decode_signed_rle(payload.subspan(6));
    if (rle.bytes.size() % kTilesetTileBytes != 0) {
      return std::nullopt;
    }

    const std::size_t tile_count = rle.bytes.size() / kTilesetTileBytes;
    decoded.tiles.reserve(tile_count);
    for (std::size_t tile_index = 0; tile_index < tile_count; ++tile_index) {
      const std::size_t tile_offset = tile_index * kTilesetTileBytes;
      Ega4PlaneImage tile{};
      tile.row_span_bytes = 32;
      tile.width_bytes = 2;
      tile.height_rows = 16;
      for (std::size_t plane = 0; plane < 4; ++plane) {
        const std::size_t plane_offset = plane * 32U;
        const auto begin = rle.bytes.begin() + static_cast<std::ptrdiff_t>(
                                                   tile_offset + plane_offset);
        const auto end = begin + static_cast<std::ptrdiff_t>(32);
        tile.planes[plane].assign(begin, end);
      }
      decoded.tiles.push_back(tile);
    }
    return decoded;
  } catch (const std::exception &) {
    return std::nullopt;
  }
}

std::optional<std::vector<MaskedSpriteRecord>>
decode_masked_sprite_sheet(std::span<const std::uint8_t> payload) {
  if (payload.empty()) {
    return std::vector<MaskedSpriteRecord>{};
  }

  std::vector<MaskedSpriteRecord> sprites;
  std::size_t offset = 0;

  while (offset + 6 <= payload.size()) {
    const auto width = read_u16(payload, offset);
    const auto height = read_u16(payload, offset + 2);
    const auto image_data_off = read_u16(payload, offset + 4);

    if (width == 0 || height == 0) {
      break;
    }

    MaskedSpriteRecord sprite{};
    sprite.width_pixels = width;
    sprite.height_rows = height;
    sprite.image_data_off = image_data_off;
    const std::size_t mask_byte_count = sprite.mask_byte_count();
    const std::size_t record_header_size = 6U;
    const std::size_t image_offset =
        record_header_size + static_cast<std::size_t>(image_data_off);
    if (image_offset < record_header_size + mask_byte_count) {
      return std::nullopt;
    }

    const std::size_t record_size = record_header_size +
                                    static_cast<std::size_t>(image_data_off) +
                                    (4U * mask_byte_count);
    if (offset + record_size > payload.size()) {
      return std::nullopt;
    }

    const auto mask_begin = payload.begin() + static_cast<std::ptrdiff_t>(
                                                  offset + record_header_size);
    const auto mask_end =
        mask_begin + static_cast<std::ptrdiff_t>(mask_byte_count);
    sprite.mask_bytes.assign(mask_begin, mask_end);

    const auto image_begin =
        payload.begin() + static_cast<std::ptrdiff_t>(offset + image_offset);
    for (std::size_t plane = 0; plane < sprite.planes.size(); ++plane) {
      const auto plane_begin =
          image_begin + static_cast<std::ptrdiff_t>(plane * mask_byte_count);
      const auto plane_end =
          plane_begin + static_cast<std::ptrdiff_t>(mask_byte_count);
      sprite.planes[plane].assign(plane_begin, plane_end);
    }

    sprites.push_back(sprite);
    offset += record_size;
  }

  return sprites;
}

std::optional<Ega4PlaneImage>
decode_frpak_catalog_record(const RuntimeState &state,
                            const FrpakCatalogRecord &record) {
  const FrpakCatalogFile *file =
      find_frpak_catalog_file(state.frpak_catalog, record.pak_id);
  if (file == nullptr) {
    return std::nullopt;
  }
  if (!validate_frpak_catalog_record_bounds(record, file->file_size)) {
    return std::nullopt;
  }
  if (file->blob_offset > state.sprite_resource_bytes.size()) {
    return std::nullopt;
  }

  const std::size_t blob_bytes_remaining =
      state.sprite_resource_bytes.size() - file->blob_offset;
  if (record.data_offset > blob_bytes_remaining) {
    return std::nullopt;
  }

  const std::size_t global_offset = file->blob_offset + record.data_offset;
  if (record.data_size > state.sprite_resource_bytes.size() - global_offset) {
    return std::nullopt;
  }

  try {
    return decode_ega_4plane_rle(
        std::span<const std::uint8_t>(state.sprite_resource_bytes)
            .subspan(global_offset, record.data_size),
        true);
  } catch (const std::exception &) {
    return std::nullopt;
  }
}

std::optional<Ega4PlaneImage> decode_frpak_record(RuntimeState &state,
                                                  std::uint16_t pak_id,
                                                  std::uint16_t record_id) {
  const auto cache_it = std::find_if(
      state.frpak_decode_cache.begin(), state.frpak_decode_cache.end(),
      [pak_id, record_id](const FrpakDecodedRecordCacheEntry &entry) {
        return entry.pak_id == pak_id && entry.record_id == record_id;
      });
  if (cache_it != state.frpak_decode_cache.end()) {
    return cache_it->image;
  }

  const auto record =
      find_frpak_catalog_record(state.frpak_catalog, pak_id, record_id);
  if (!record.has_value()) {
    return std::nullopt;
  }

  const auto decoded = decode_frpak_catalog_record(state, *record);
  if (!decoded.has_value()) {
    return std::nullopt;
  }

  state.frpak_decode_cache.push_back(FrpakDecodedRecordCacheEntry{
      .pak_id = pak_id,
      .record_id = record_id,
      .image = *decoded,
  });
  return decoded;
}

void clear_frpak_decode_cache(RuntimeState &state) {
  state.frpak_decode_cache.clear();
}

SignedRleResult decode_signed_rle(std::span<const std::uint8_t> encoded) {
  SignedRleResult out;
  std::size_t i = 0;
  while (i < encoded.size()) {
    const auto control = static_cast<std::int8_t>(encoded[i++]);
    if (control == 0) {
      out.consumed = i;
      return out;
    }

    if (control < 0) {
      if (i >= encoded.size()) {
        throw std::runtime_error("signed-rle truncated run value");
      }
      const auto value = encoded[i++];
      const auto count = static_cast<std::size_t>(-control);
      if (out.bytes.size() > kMaxDecodedBytes - count) {
        throw std::runtime_error("signed-rle decoded output too large");
      }
      out.bytes.insert(out.bytes.end(), count, value);
      continue;
    }

    const auto count = static_cast<std::size_t>(control);
    if (i + count > encoded.size()) {
      throw std::runtime_error("signed-rle truncated literal");
    }
    if (out.bytes.size() > kMaxDecodedBytes - count) {
      throw std::runtime_error("signed-rle decoded output too large");
    }
    out.bytes.insert(out.bytes.end(),
                     encoded.begin() + static_cast<std::ptrdiff_t>(i),
                     encoded.begin() + static_cast<std::ptrdiff_t>(i + count));
    i += count;
  }

  throw std::runtime_error("signed-rle missing terminator");
}

Ega4PlaneImage decode_ega_4plane_rle(std::span<const std::uint8_t> encoded,
                                     bool require_full_consumption) {
  if (encoded.size() < sizeof(EgaRle4PlaneHeader)) {
    throw std::runtime_error("ega-rle data too small");
  }

  Ega4PlaneImage image;
  image.row_span_bytes = read_u16(encoded, 0);
  if (image.row_span_bytes == 0) {
    throw std::runtime_error("ega-rle row span cannot be zero");
  }
  // row_span_bytes is u16 (max 65535), so it can never exceed kMaxDecodedBytes
  // (65536). Check for unreasonable values with a u16-safe threshold.
  constexpr std::uint16_t kMaxRowSpan =
      0x7FFF; // 32768 - reasonable upper bound
  if (image.row_span_bytes > kMaxRowSpan) {
    throw std::runtime_error("ega-rle row span too large");
  }

  std::size_t i = sizeof(EgaRle4PlaneHeader);
  for (std::size_t plane = 0; plane < image.planes.size(); ++plane) {
    auto &out = image.planes[plane];
    out.reserve(image.row_span_bytes);
    while (out.size() < image.row_span_bytes) {
      if (i >= encoded.size()) {
        throw std::runtime_error("ega-rle truncated packet stream");
      }

      const std::uint8_t packet = encoded[i++];
      if ((packet & 0x80U) == 0) {
        const auto count = static_cast<std::size_t>(packet);
        if (i + count > encoded.size()) {
          throw std::runtime_error("ega-rle truncated literal packet");
        }
        if (out.size() > kMaxDecodedBytes - count) {
          throw std::runtime_error("ega-rle decoded output too large");
        }
        out.insert(out.end(), encoded.begin() + static_cast<std::ptrdiff_t>(i),
                   encoded.begin() + static_cast<std::ptrdiff_t>(i + count));
        i += count;
      } else {
        const auto count = static_cast<std::size_t>(packet & 0x7FU);
        if (i >= encoded.size()) {
          throw std::runtime_error("ega-rle truncated run packet");
        }
        const auto value = encoded[i++];
        if (out.size() > kMaxDecodedBytes - count) {
          throw std::runtime_error("ega-rle decoded output too large");
        }
        out.insert(out.end(), count, value);
      }

      if (out.size() > image.row_span_bytes) {
        throw std::runtime_error("ega-rle plane overrun");
      }
    }
  }

  if (require_full_consumption && i != encoded.size()) {
    throw std::runtime_error("ega-rle trailing bytes after 4-plane decode");
  }

  return image;
}

BootstrapLoadSummary
load_initial_bootstrap_resources(RuntimeState &state,
                                 const std::filesystem::path &root) {
  BootstrapLoadSummary summary{};

  const std::array metadata_candidates = {
      root / "FRDATA.0",
      root / "FRDATA.1",
  };
  const std::array sprite_candidates = {
      root / "FRPAK.001", root / "FRPAK.002", root / "FRPAK.003",
      root / "FRPAK.004", root / "FRPAK.005", root / "FRPAK.006",
      root / "FRPAK.007",
  };

  for (const auto &candidate : metadata_candidates) {
    ++summary.metadata_files_tried;
    if (!path_exists(candidate)) {
      continue;
    }

    try {
      const auto bytes = load_file_bytes(candidate);
      if (!bytes.has_value()) {
        continue;
      }

      state.level_metadata_bytes = *bytes;
      state.room_resource_bytes = *bytes;
      // The FRDATA payload feeds both the metadata snapshot and the room
      // resource bytes in this bootstrap path.
      if (load_room_tilemap_from_resource_file(state, candidate, *bytes, 0,
                                               0)) {
        summary.room_grid_loaded = true;
        break;
      }
    } catch (const std::exception &) {
      // Keep the bootstrap non-fatal and fall through to the next candidate.
    }
  }

  if (!summary.room_grid_loaded) {
    summary.room_grid_loaded =
        load_room_tilemap_from_asset_root(state, root, 0, 0);
  }

  for (const auto &candidate : sprite_candidates) {
    ++summary.sprite_files_tried;
    if (!path_exists(candidate)) {
      continue;
    }

    try {
      const auto bytes = load_file_bytes(candidate);
      if (!bytes.has_value()) {
        continue;
      }

      const auto pak_id = parse_frpak_id_from_path(candidate);
      if (!pak_id.has_value()) {
        continue;
      }

      const auto catalog_file =
          build_frpak_catalog_file(candidate, *bytes, *pak_id);
      if (!catalog_file.has_value()) {
        continue;
      }

      const bool all_records_valid = std::all_of(
          catalog_file->records.begin(), catalog_file->records.end(),
          [&](const FrpakCatalogRecord &record) {
            return validate_frpak_catalog_record_bounds(
                record, catalog_file->file_size);
          });
      if (!all_records_valid) {
        continue;
      }

      FrpakCatalogFile indexed_file = *catalog_file;
      indexed_file.blob_offset = state.sprite_resource_bytes.size();
      state.frpak_catalog.files.push_back(indexed_file);
      state.sprite_resource_bytes.insert(state.sprite_resource_bytes.end(),
                                         bytes->begin(), bytes->end());
      clear_frpak_decode_cache(state);
    } catch (const std::exception &) {
      // Missing or unreadable FRPAK payloads should not stop bootstrap.
    }
  }

  if (!state.bezel_frame.has_value()) {
    const auto bezel_record =
        find_frpak_catalog_record(state.frpak_catalog, 1U, 0U);
    if (bezel_record.has_value()) {
      const auto decoded = decode_frpak_catalog_record(state, *bezel_record);
      if (decoded.has_value()) {
        Ega4PlaneImage image = *decoded;
        if (image.row_span_bytes > 0U && image.row_span_bytes % 40U == 0U) {
          image.width_bytes = 40U;
          image.height_rows =
              static_cast<std::uint16_t>(image.row_span_bytes / 40U);
        }
        if (image.width_bytes == 40U && image.height_rows == 200U) {
          state.bezel_frame = image;
        }
      }
    }
  }

  return summary;
}

} // namespace comic2
