/**
 * @file ethics_profile_registry.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "ethics_profile_registry.h"
#include "utils/logger.h"

#include <algorithm>
#include <filesystem>
#include <set>

#ifdef HAVE_YAML_CPP
#include <yaml-cpp/yaml.h>
#endif

namespace themis {
namespace plugins {
namespace ethics {

/// Maximum number of characters kept from a profile's `description` field
/// in the lightweight metadata index.  150 chars is enough for semantic
/// routing (Stage-2 term-overlap) while keeping index RAM at ~500 B/profile.
static constexpr size_t kDescriptionSnippetMaxLength = 150;

namespace {
#ifdef HAVE_YAML_CPP
/// Helper: read a sequence of strings from a YAML node (scalar or sequence).
std::vector<std::string> yamlStringSeq(const YAML::Node& node) {
    std::vector<std::string> result = {};

    if (!node) {
      return result;
    }
    if (node.IsScalar()) {
        auto v = node.as<std::string>("");
        if (!v.empty()) {
          result.push_back(v);
        }
    } else if (node.IsSequence()) {
        for (const auto& item : node) {
            if (item.IsScalar()) {
                auto v = item.as<std::string>("");
                if (!v.empty()) {
                  result.push_back(v);
                }
            }
        }
    }
    return result;
}
#endif
} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// Query index
// ─────────────────────────────────────────────────────────────────────────────

std::vector<EthicsProfileMeta> EthicsProfileRegistry::queryIndex(
    const EthicsIndexQuery& query) const
{
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<EthicsProfileMeta> results = {};

    results.reserve(index_.size());

    for (const auto& [id, meta] : index_) {
        // Filter by taxonomy_class
        if (!query.taxonomy_class.empty() &&
            meta.taxonomy_class != query.taxonomy_class) {
            continue;
        }

        // Filter: all requested tags must be present
        if (!query.tags.empty()) {
            // COMPLEXITY FIX: Convert to set for O(1) lookups instead of O(n²) (HIGH: repeated_search)
            std::set<std::string> meta_tags(meta.tags.begin(), meta.tags.end());
            bool all_found = true;
            for (const auto& t : query.tags) {
                if (meta_tags.find(t) == meta_tags.end()) {
                    all_found = false;
                    break;
                }
            }
            if (!all_found) {
              continue;
            }
        }

        // Filter: at least one requested domain must match
        if (!query.domains.empty()) {
            // COMPLEXITY FIX: Convert to set for O(1) lookups instead of O(n²) (HIGH: repeated_search)
            std::set<std::string> meta_domains(meta.applicable_domains.begin(), meta.applicable_domains.end());
            bool any_found = false;
            for (const auto& d : query.domains) {
                if (meta_domains.find(d) != meta_domains.end()) {
                    any_found = true;
                    break;
                }
            }
            if (!any_found) {
              continue;
            }
        }

        results.push_back(meta);
        if (query.max_results > 0 && results.size() >= query.max_results) {
          break;
        }
    }

    return results;
}

// ─────────────────────────────────────────────────────────────────────────────
// Constructor + simple accessors
// ─────────────────────────────────────────────────────────────────────────────

EthicsProfileRegistry::EthicsProfileRegistry(size_t lru_capacity)
    : lru_capacity_(lru_capacity)
{
}

size_t EthicsProfileRegistry::indexSize() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return index_.size();
}

bool EthicsProfileRegistry::hasProfile(const std::string& school_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return index_.find(school_id) != index_.end();
}

std::variant<PhilosophyProfile, Status> EthicsProfileRegistry::getProfile(
    const std::string& school_id)
{
    std::lock_guard<std::mutex> lock(mutex_);

    // LRU hit
    if (const PhilosophyProfile* cached = lruGet(school_id)) {
        return *cached;
    }

    // Unknown school
    auto it = index_.find(school_id);
    if (it == index_.end()) {
        return Status::Error("Unknown ethics school: " + school_id);
    }

    // Cold load via PhilosophyLoader
    Status s = loader_.loadFromFile(it->second.yaml_path);
    if (!s.isOK()) {
        return Status::Error("Failed to load profile '" + school_id +
                             "': " + s.message);
    }

    auto profile_var = loader_.getProfile(school_id);
    if (std::holds_alternative<Status>(profile_var)) {
        return std::get<Status>(profile_var);
    }

    const PhilosophyProfile& profile = std::get<PhilosophyProfile>(profile_var);
    lruPut(school_id, profile);
    return profile;
}

std::variant<size_t, Status> EthicsProfileRegistry::rebuildIndex(
    const std::string& directory)
{
    namespace fs = std::filesystem;

    if (!fs::exists(directory)) {
        return Status::Error("Directory does not exist: " + directory);
    }
    if (!fs::is_directory(directory)) {
        return Status::Error("Path is not a directory: " + directory);
    }

    std::map<std::string, EthicsProfileMeta> new_index;

    // COMPLEXITY FIX: Store iterator to avoid temporary reference invalidation (HIGH: range_temporary)
    auto dir_iter = fs::recursive_directory_iterator(directory);
    for (const auto& entry : dir_iter) {
        if (!entry.is_regular_file()) {
          continue;
        }
        const auto ext = entry.path().extension().string();
        if (ext != ".yaml" && ext != ".yml") {
          continue;
        }

        EthicsProfileMeta meta = scanHeader(entry.path().string());
        if (!meta.school_id.empty()) {
            new_index[meta.school_id] = std::move(meta);
        }
    }

    std::lock_guard<std::mutex> lock(mutex_);
    index_ = std::move(new_index);
    // Flush LRU cache after index rebuild
    lru_list_.clear();
    lru_map_.clear();
    loader_.clear();

    return index_.size();
}

// ─────────────────────────────────────────────────────────────────────────────
// LRU cache helpers
// ─────────────────────────────────────────────────────────────────────────────

void EthicsProfileRegistry::lruPut(const std::string& id,
                                    const PhilosophyProfile& profile)
{
    // Remove existing entry (if any) to avoid duplicates
    auto it = lru_map_.find(id);
    if (it != lru_map_.end()) {
        lru_list_.erase(it->second);
        lru_map_.erase(it);
    }

    // Evict LRU if at capacity
    if (lru_list_.size() >= lru_capacity_) {
        lruEvict();
    }

    lru_list_.push_front({id, profile});
    lru_map_[id] = lru_list_.begin();
}

const PhilosophyProfile* EthicsProfileRegistry::lruGet(const std::string& id)
{
    auto it = lru_map_.find(id);
    if (it == lru_map_.end()) {
      return nullptr;
    }

    // Move to front (most-recently used)
    lru_list_.splice(lru_list_.begin(), lru_list_, it->second);
    return &(lru_list_.front().second);
}

void EthicsProfileRegistry::lruEvict()
{
    if (lru_list_.empty()) {
      return;
    }
    const std::string& lru_id = lru_list_.back().first;
    lru_map_.erase(lru_id);
    lru_list_.pop_back();
}

// ─────────────────────────────────────────────────────────────────────────────
// Header-only YAML scan
// ─────────────────────────────────────────────────────────────────────────────

EthicsProfileMeta EthicsProfileRegistry::scanHeader(const std::string& filepath)
{
    EthicsProfileMeta meta;
    namespace fs = std::filesystem;

    // Derive school_id from filename as fallback
    meta.school_id = fs::path(filepath).stem().string();
    meta.yaml_path = filepath;

#ifdef HAVE_YAML_CPP
    try {
        YAML::Node root = YAML::LoadFile(filepath);

        // school_id: override filename-derived fallback when present
        if (root["school_id"] && root["school_id"].IsScalar()) {
            std::string school_id_value = root["school_id"].as<std::string>("");
            if (!school_id_value.empty()) {
              meta.school_id = school_id_value;
            }
        }
        // Alternate key used by some profiles (e.g. nietzsche.yaml)
        if (meta.school_id.empty() && root["school"] && root["school"].IsScalar()) {
            meta.school_id = root["school"].as<std::string>("");
        }

        if (root["name"] && root["name"].IsScalar())
            meta.name = root["name"].as<std::string>("");

        if (root["taxonomy_class"] && root["taxonomy_class"].IsScalar())
            meta.taxonomy_class = root["taxonomy_class"].as<std::string>("");

        meta.tags             = yamlStringSeq(root["tags"]);
        meta.applicable_domains = yamlStringSeq(root["applicable_domains"]);

        // Grab a short description snippet
        if (root["description"] && root["description"].IsScalar()) {
            std::string desc = root["description"].as<std::string>("");
            meta.description_snippet = desc.substr(0, std::min(desc.size(), kDescriptionSnippetMaxLength));
        }
    } catch (const std::exception& ex) {
        // Non-fatal: return what we have (school_id from filename)
        THEMIS_WARN("EthicsProfileRegistry: failed to scan header of '{}': {}",
                 filepath, ex.what());
    }
#endif

    return meta;
}

} // namespace ethics
} // namespace plugins
} // namespace themis
