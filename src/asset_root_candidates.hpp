#pragma once

#include <algorithm>
#include <filesystem>
#include <vector>

namespace comic2::detail {

inline std::filesystem::path
canonicalize_asset_path(const std::filesystem::path &path) {
  std::error_code ec;
  const auto canonical = std::filesystem::weakly_canonical(path, ec);
  return ec ? path.lexically_normal() : canonical;
}

inline bool is_related_asset_root(const std::filesystem::path &lhs,
                                  const std::filesystem::path &rhs) {
  if (lhs.empty() || rhs.empty()) {
    return false;
  }

  const auto canonical_left = canonicalize_asset_path(lhs);
  const auto canonical_right = canonicalize_asset_path(rhs);
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

inline std::vector<std::filesystem::path>
build_asset_root_candidates(const std::filesystem::path &root) {
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

    const auto key = canonicalize_asset_path(candidate);
    const bool already_present =
        std::any_of(candidates.begin(), candidates.end(),
                    [&](const auto &existing) { return existing == key; });
    if (!already_present) {
      candidates.push_back(key);
    }
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

} // namespace comic2::detail
