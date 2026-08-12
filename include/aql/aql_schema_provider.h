/**
 * @file aql_schema_provider.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

#include <string>
#include <vector>

namespace themis {
namespace aql {

/// Lightweight metadata for a single field/property in a collection.
/// 
/// Captures essential field metadata for schema-aware validation and
/// natural language query translation without requiring live database connections.
struct CollectionFieldInfo {
    std::string name;           ///< Field name (e.g., "age")
    std::string type;           ///< Data type: "string", "integer", "double", "boolean", "vector", etc.
    bool        indexed  = false; ///< True when a secondary index exists on this field
    bool        nullable = true;  ///< True when the field may be absent or null
};

/// Lightweight, self-contained metadata snapshot for one collection.
///
/// This struct intentionally avoids any dependency on SchemaManager or
/// RocksDB so that callers (tests, tools, REST handlers) can construct it
/// without a live database connection.  Use @c fromTableSchema() from
/// `metadata/aql_schema_bridge.h` to populate it from a live SchemaManager.
/// 
/// Typical usage:
/// @code
/// std::vector<CollectionMetadata> schema = {
///   {.name="users", .type="document", .fields={...}, .estimated_count=1000},
///   {.name="posts", .type="document", .fields={...}, .estimated_count=5000}
/// };
/// std::string context = formatSchemaContext(schema);
/// @endcode
struct CollectionMetadata {
    std::string                    name;            ///< Collection/table name
    std::string                    type;            ///< "document", "relational", "graph_node", etc.
    std::vector<CollectionFieldInfo> fields;         ///< Known fields (may be incomplete)
    std::size_t                    estimated_count = 0; ///< Approximate document count (0 = unknown)
};

/// @brief Format a vector of CollectionMetadata into a human-readable / LLM-friendly schema context string.
///
/// The returned string is suitable for passing to
/// @c AQLQueryBuilder::getCompletionSuggestions() or
/// @c AQLQueryBuilder::getLLMSuggestion() as the @p schema_context argument,
/// or it can be displayed to end users as schema documentation.
///
/// @param schema  Vector of collection metadata to format
/// @return Formatted schema context string (empty if @p schema is empty)
/// 
/// Example output:
/// @code
/// Collection: users (document)
///   - _key: string (indexed)
///   - name: string (nullable)
///   - age: integer (nullable)
///   - tags: vector (nullable)
/// 
/// Collection: posts (document)
///   - _key: string (indexed)
///   - user_id: string
///   - content: string (nullable)
/// @endcode
std::string formatSchemaContext(const std::vector<CollectionMetadata>& schema);

} // namespace aql
} // namespace themis
