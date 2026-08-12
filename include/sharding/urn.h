/**
 * @file urn.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <string_view>
#include <optional>
#include <cstdint>
#include <regex>

namespace themis::sharding {

/**
 * @brief Canonical shard-routable resource identifier.
 *
 * Format: urn:themis:{model}:{namespace}:{collection}:{uuid}
 *
 * Examples:
 * - urn:themis:relational:customers:users:550e8400-e29b-41d4-a716-446655440000
 * - urn:themis:graph:social:nodes:7c9e6679-7425-40de-944b-e07fc1f90ae7
 * - urn:themis:vector:embeddings:documents:f47ac10b-58cc-4372-a567-0e02b2c3d479
 * - urn:themis:timeseries:metrics:cpu_usage:3d6e3e3e-4c5d-4f5e-9e7f-8a9b0c1d2e3f
 *
 * The identifier provides location transparency and keeps client-visible
 * keys stable across resharding operations.
 */
struct URN {
    std::string model;        // relational, graph, vector, timeseries, document
    std::string namespace_;   // customer_a, tenant_123, global
    std::string collection;   // users, nodes, documents, edges
    std::string uuid;         // RFC 4122 UUID v4 (e.g., 550e8400-e29b-41d4-a716-446655440000)
    
    /**
     * @brief Parse a textual URN into components.
     * @param urn_str URN text in the format
     *        urn:themis:{model}:{namespace}:{collection}:{uuid}.
     * @return Parsed URN when the full format and value constraints are valid,
     *         std::nullopt otherwise.
     */
    static std::optional<URN> parse(std::string_view urn_str);
    
    /**
     * @brief Serialize this URN to canonical text form.
     * @return Canonical URN string.
     */
    std::string toString() const;
    
    /**
     * @brief Compute routing hash for shard placement.
     * @return 64-bit hash value over the UUID component.
     * @note Uses xxHash when available and falls back to std::hash.
     */
    uint64_t hash() const;
    
    /**
     * @brief Validate UUID syntax.
     * @return true when uuid matches RFC-4122 textual layout.
     */
    bool isValidUUID() const;
    
    /**
     * @brief Return backward-compatible key form.
     * @return Resource identifier in collection:uuid format.
     */
    std::string getResourceId() const { return collection + ":" + uuid; }
    
    /**
     * @brief Validate model namespace.
     * @return true when model is in the supported model set.
     */
    bool isValidModel() const;
    
    /** @brief Structural equality across all URN components. */
    bool operator==(const URN& other) const {
        return model == other.model &&
               namespace_ == other.namespace_ &&
               collection == other.collection &&
               uuid == other.uuid;
    }
    
    /** @brief Structural inequality across all URN components. */
    bool operator!=(const URN& other) const {
        return !(*this == other);
    }
};

} // namespace themis::sharding
