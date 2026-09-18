/**
 * @file ethics_profile_registry.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct EthicsProfileMeta {
    std::string school_id;          ///< Unique identifier, matches YAML school_id
    std::string name;               ///< Human-readable display name
    std::string taxonomy_class;     ///< Primary taxonomy class (see ethics_taxonomy.yaml)
    std::vector<std::string> tags;  ///< Domain / topic tags used for routing
    std::vector<std::string> applicable_domains;  ///< Dilemma domains this school covers
    std::string yaml_path;          ///< Absolute path to the YAML file
    std::string description_snippet;

    EthicsProfileMeta() = default;
};

struct EthicsIndexQuery {
    std::string taxonomy_class;     ///< Filter by taxonomy class; empty = no filter
    std::vector<std::string> tags;  ///< All listed tags must be present; empty = no filter
    std::vector<std::string> domains; ///< At least one domain must match; empty = no filter
    size_t max_results{0};          ///< 0 = unlimited
};

class IEthicsProfileRegistry {
public:
    /**
     * @brief IEthics Profile Registry.
     * @return Return value.
     */
    virtual ~IEthicsProfileRegistry() = default;

    /**
     * @brief Query Index.
     * @param[in] query Input parameter.
     * @return Return value.
     */
    virtual std::vector<EthicsProfileMeta> queryIndex(
        const EthicsIndexQuery& query) const = 0;

    virtual std::variant<PhilosophyProfile, Status> getProfile(
        const std::string& school_id) = 0;

    virtual std::variant<size_t, Status> rebuildIndex(
        const std::string& directory) = 0;

    /**
     * @brief Index Size.
     * @return Return value.
     */
    virtual size_t indexSize() const = 0;

    /**
     * @brief Has Profile.
     * @param[in] school_id Identifier of the school.
     * @return True when the operation succeeds.
     */
    virtual bool hasProfile(const std::string& school_id) const = 0;
};

} // namespace ethics
} // namespace plugins
} // namespace themis
