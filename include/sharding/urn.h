/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            urn.h                                              ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:26:54                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     125                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <string_view>
#include <optional>
#include <cstdint>
#include <regex>

namespace themis::sharding {

/**
 * URN Structure: urn:themis:{model}:{namespace}:{collection}:{uuid}
 * 
 * Examples:
 *   urn:themis:relational:customers:users:550e8400-e29b-41d4-a716-446655440000
 *   urn:themis:graph:social:nodes:7c9e6679-7425-40de-944b-e07fc1f90ae7
 *   urn:themis:vector:embeddings:documents:f47ac10b-58cc-4372-a567-0e02b2c3d479
 *   urn:themis:timeseries:metrics:cpu_usage:3d6e3e3e-4c5d-4f5e-9e7f-8a9b0c1d2e3f
 * 
 * This URN format provides:
 * - Location transparency: Clients don't know which shard holds the data
 * - Dynamic resharding: Shards can be moved without client changes
 * - Multi-tenancy: Namespaces isolate tenants
 * - Cross-model queries: URN-based routing across all data models
 * 
 * @sources
 * - Concept: VCC-URN (Virtual Content Container - Uniform Resource Name)
 * - Origin: ThemisDB Original Design
 * - Purpose: Unified addressing scheme for multi-model database with sharding support
 * - Inspiration: 
 *   - URN Standard: RFC 8141 (Uniform Resource Names)
 *   - Azure Cosmos DB: Hierarchical partition keys
 *   - Cassandra: Partition key + clustering key
 * - Innovation: Combines URN standard with multi-model awareness and content-based routing
 * - Implementation: ThemisDB Core Team
 * - First Introduced: ThemisDB v1.0.0
 */
struct URN {
    std::string model;        // relational, graph, vector, timeseries, document
    std::string namespace_;   // customer_a, tenant_123, global
    std::string collection;   // users, nodes, documents, edges
    std::string uuid;         // RFC 4122 UUID v4 (e.g., 550e8400-e29b-41d4-a716-446655440000)
    
    /**
     * Parse URN string into components
     * @param urn_str URN string in format: urn:themis:{model}:{namespace}:{collection}:{uuid}
     * @return Parsed URN if valid, nullopt otherwise
     */
    static std::optional<URN> parse(std::string_view urn_str);
    
    /**
     * Serialize URN to string
     * @return URN string representation
     */
    std::string toString() const;
    
    /**
     * Hash URN for consistent hashing (uses UUID for distribution)
     * Uses xxHash for fast, well-distributed hashing
     * @return 64-bit hash value
     */
    uint64_t hash() const;
    
    /**
     * Validate UUID format (RFC 4122)
     * Expected format: 8-4-4-4-12 hex digits with hyphens
     * Example: 550e8400-e29b-41d4-a716-446655440000
     * @return true if UUID is valid RFC 4122 format
     */
    bool isValidUUID() const;
    
    /**
     * Get full resource identifier (collection:uuid)
     * This matches the existing ThemisDB key format for backward compatibility
     * @return Resource ID string
     */
    std::string getResourceId() const { return collection + ":" + uuid; }
    
    /**
     * Validate model type
     * @return true if model is one of: relational, graph, vector, timeseries, document
     */
    bool isValidModel() const;
    
    /**
     * Equality operator
     */
    bool operator==(const URN& other) const {
        return model == other.model &&
               namespace_ == other.namespace_ &&
               collection == other.collection &&
               uuid == other.uuid;
    }
    
    /**
     * Inequality operator
     */
    bool operator!=(const URN& other) const {
        return !(*this == other);
    }
};

} // namespace themis::sharding
