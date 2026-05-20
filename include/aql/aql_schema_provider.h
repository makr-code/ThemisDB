/*
 * ThemisDB | File: aql_schema_provider.h | Version: 0.0.15
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

#include <string>
#include <vector>

namespace themis {
namespace aql {

/// Lightweight metadata for a single field/property in a collection.
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
struct CollectionMetadata {
    std::string                    name;            ///< Collection/table name
    std::string                    type;            ///< "document", "relational", "graph_node", etc.
    std::vector<CollectionFieldInfo> fields;         ///< Known fields (may be incomplete)
    std::size_t                    estimated_count = 0; ///< Approximate document count (0 = unknown)
};

/// Format a vector of CollectionMetadata into a human-readable / LLM-friendly
/// schema context string.
///
/// The returned string is suitable for passing to
/// @c AQLQueryBuilder::getCompletionSuggestions() or
/// @c AQLQueryBuilder::getLLMSuggestion() as the @p schema_context argument,
/// or it can be displayed to end users as schema documentation.
///
/// Returns an empty string when @p schema is empty.
std::string formatSchemaContext(const std::vector<CollectionMetadata>& schema);

} // namespace aql
} // namespace themis
