/**
 * @file ethics_profile_registry.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "ethics_ai/ethics_ai_types.h"
#include <string>
#include <vector>
#include <variant>

namespace themis {
namespace plugins {
namespace ethics {

/**
 * @brief Lightweight metadata record kept in-RAM for every known profile.
 *
 * The full `PhilosophyProfile` is loaded on demand; only this struct is
 * always resident, keeping RAM usage to ~500 B per profile (100 KB for 200
 * profiles).
 */
struct EthicsProfileMeta {
    std::string school_id;          ///< Unique identifier, matches YAML school_id
    std::string name;               ///< Human-readable display name
    std::string taxonomy_class;     ///< Primary taxonomy class (see ethics_taxonomy.yaml)
    std::vector<std::string> tags;  ///< Domain / topic tags used for routing
    std::vector<std::string> applicable_domains;  ///< Dilemma domains this school covers
    std::string yaml_path;          ///< Absolute path to the YAML file
    /// Optional short description (first ~150 chars of description field)
    std::string description_snippet;

    EthicsProfileMeta() = default;
};

/**
 * @brief Query structure for the metadata index.
 *
 * All specified fields are combined with AND semantics: only profiles
 * matching *all* non-empty criteria are returned.
 */
struct EthicsIndexQuery {
    std::string taxonomy_class;     ///< Filter by taxonomy class; empty = no filter
    std::vector<std::string> tags;  ///< All listed tags must be present; empty = no filter
    std::vector<std::string> domains; ///< At least one domain must match; empty = no filter
    size_t max_results{0};          ///< 0 = unlimited
};

/**
 * @brief Interface for the scalable ethics profile registry.
 *
 * Provides O(1) metadata lookup and on-demand loading of full profiles.
 * Implementations must be thread-safe.
 *
 * ## Scaling guarantees
 * - `queryIndex()` ≤ 2 ms for ≤ 1 000 profiles.
 * - `getProfile()` ≤ 100 ms cold, ≤ 1 ms warm (LRU cache).
 * - `rebuildIndex()` ≤ 500 ms for ≤ 200 profiles (filesystem scan).
 */
class IEthicsProfileRegistry {
public:
    virtual ~IEthicsProfileRegistry() = default;

    /**
     * @brief Query the lightweight metadata index.
     *
     * Always returns results from RAM without loading full YAML profiles.
     *
     * @param query  Filter criteria; all empty → returns all known profiles.
     * @return       Matching metadata records (may be empty, never an error).
     */
    virtual std::vector<EthicsProfileMeta> queryIndex(
        const EthicsIndexQuery& query) const = 0;

    /**
     * @brief Retrieve (and cache) the full profile for @p school_id.
     *
     * Returns `Status::Error` when the school_id is unknown or the YAML
     * file cannot be parsed.
     */
    virtual std::variant<PhilosophyProfile, Status> getProfile(
        const std::string& school_id) = 0;

    /**
     * @brief Rebuild the metadata index by scanning @p directory.
     *
     * Performs a header-only YAML scan (reads `school_id`, `name`,
     * `taxonomy_class`, `tags`, `applicable_domains`) for every `.yaml`
     * file in the directory tree.  Does not load full profile content.
     * After a successful rebuild the LRU cache is flushed.
     *
     * @return Number of profiles indexed, or `Status::Error` on failure.
     */
    virtual std::variant<size_t, Status> rebuildIndex(
        const std::string& directory) = 0;

    /**
     * @brief Number of profiles currently in the index.
     */
    virtual size_t indexSize() const = 0;

    /**
     * @brief Check whether @p school_id is known (without loading the profile).
     */
    virtual bool hasProfile(const std::string& school_id) const = 0;
};

} // namespace ethics
} // namespace plugins
} // namespace themis
