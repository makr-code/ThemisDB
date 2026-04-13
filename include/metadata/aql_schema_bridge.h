/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            aql_schema_bridge.h                                ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-04-13 20:23:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     79                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 2e58fd3cd9  2026-02-23  feat(aql): schema-aware query generation using live colle... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

#include "aql/aql_schema_provider.h"
#include "metadata/schema_manager.h"

namespace themis {
namespace aql {

/// Convert a live @c SchemaManager::TableSchema to a lightweight
/// @c CollectionMetadata snapshot.
///
/// This helper is the bridge between the metadata module (which owns live
/// schema data) and the aql module (which consumes a portable snapshot for
/// query generation and validation).
///
/// @param ts  Source table schema from SchemaManager.  An empty properties
///            list is valid and results in a CollectionMetadata with no fields.
/// @return    CollectionMetadata populated with name, type, estimated_count,
///            and one @c CollectionFieldInfo per property in @p ts.
///
/// Usage example:
/// @code
///   SchemaManager schema_mgr(db, idx_mgr);
///   auto tables = schema_mgr.getAllTables();
///
///   std::vector<aql::CollectionMetadata> meta;
///   meta.reserve(tables.size());
///   for (const auto& t : tables) {
///       meta.push_back(aql::fromTableSchema(t));
///   }
///   builder.setSchema(meta);
/// @endcode
inline CollectionMetadata fromTableSchema(const SchemaManager::TableSchema& ts) {
    CollectionMetadata meta;
    meta.name            = ts.name;
    meta.type            = ts.type;
    meta.estimated_count = ts.estimated_row_count;

    meta.fields.reserve(ts.properties.size());
    for (const auto& p : ts.properties) {
        CollectionFieldInfo f;
        f.name     = p.name;
        f.type     = p.type;
        f.indexed  = p.indexed;
        f.nullable = p.nullable;
        meta.fields.push_back(std::move(f));
    }

    return meta;
}

} // namespace aql
} // namespace themis
