#pragma once

#include "query/window_spec.h"
#include <chrono>
#include <cstdint>
#include <string>

namespace themis {
namespace query {

/** @brief How results are delivered to a subscriber. */
enum class ResultMode {
    DELTA,     ///< Only the tuples added/removed since the last tick
    SNAPSHOT,  ///< Full current window contents every tick
    CHANGES    ///< Annotated stream: each tuple tagged +added / -removed
};

/**
 * @brief Runtime information about a registered continuous query.
 *
 * Returned by ContinuousQueryEngine::listQueries() and the
 * `SHOW CONTINUOUS QUERIES` AQL DDL statement.
 */
struct ContinuousQueryInfo {
    std::string name;
    std::string source_collection;
    WindowSpec  window;
    ResultMode  result_mode{ResultMode::DELTA};

    std::chrono::system_clock::time_point registered_at;
    std::chrono::system_clock::time_point last_tick_at;

    uint64_t tuples_processed{0};
    size_t   result_queue_depth{0};
};

}  // namespace query
}  // namespace themis
