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
