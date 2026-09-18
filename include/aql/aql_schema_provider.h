/**
 * @file aql_schema_provider.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct CollectionFieldInfo {
    std::string name;           ///< Field name (e.g., "age")
    std::string type;           ///< Data type: "string", "integer", "double", "boolean", "vector", etc.
    bool        indexed  = false; ///< True when a secondary index exists on this field
    bool        nullable = true;  ///< True when the field may be absent or null
};

struct CollectionMetadata {
    std::string                    name;            ///< Collection/table name
    std::string                    type;            ///< "document", "relational", "graph_node", etc.
    std::vector<CollectionFieldInfo> fields;         ///< Known fields (may be incomplete)
    std::size_t                    estimated_count = 0; ///< Approximate document count (0 = unknown)
};

/**
 * @brief Format Schema Context.
 * @param[in] schema Input parameter.
 * @return Return value.
 */
std::string formatSchemaContext(const std::vector<CollectionMetadata>& schema);

} // namespace aql
} // namespace themis
