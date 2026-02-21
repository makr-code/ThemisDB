/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            aql_runner.h                                       ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-02-21 17:07:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     45                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <utility>
#include <nlohmann/json.hpp>
#include "query/aql_parser.h"
#include "query/aql_translator.h"
#include "query_engine.h"
#include "utils/expected.h"

namespace themis {

// High-level convenience dispatcher for AQL execution.
// Translates AQL to internal query forms and invokes the proper QueryEngine method.
// Returns Result<nlohmann::json> for unified error handling.
// GAP-002: Migrated from std::pair<Status, json> to Result<json>
Result<nlohmann::json> executeAql(const std::string& aql, QueryEngine& engine);

// ── Query plan visualisation (EXPLAIN / EXPLAIN ANALYZE) ─────────────────────
//
// All three functions parse and translate the AQL query but stop before execution;
// they return the optimised query plan instead.  Non-conjunctive query forms
// (graph traversal, vector+geo, OR queries, …) fall back to a single SeqScan node
// that describes the query type.

/// Returns the execution plan as a JSON object (EXPLAIN format).
/// Set @p analyze=true to include an `actual_time_ms` / `actual_rows` skeleton
/// (populated with sentinel values −1 / 0 since no execution is performed here).
Result<nlohmann::json> explainAql(const std::string& aql, QueryEngine& engine,
                                  bool analyze = false);

/// Returns the execution plan as indented text (PostgreSQL-style EXPLAIN output).
Result<std::string> explainAqlText(const std::string& aql, QueryEngine& engine,
                                   bool analyze = false);

/// Returns the execution plan as a Graphviz DOT digraph string.
/// The output can be piped to `dot -Tpng -o plan.png` for a visual diagram.
Result<std::string> explainAqlDot(const std::string& aql, QueryEngine& engine);

} // namespace themis
