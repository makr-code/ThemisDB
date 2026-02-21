/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            aql_runner.h                                       ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:37:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     45                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 37da19d1c  2026-02-10  Refactor code structure for improved readability and main... ║
    • ce7781536  2026-02-05  GAP-002: Migrate QueryEngine error handling to Result<T> ... ║
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

} // namespace themis
