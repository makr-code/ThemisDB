/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            query_profiler.cpp                                 ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-04-15 04:18:48                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     101                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • d8c296b8a5  2026-04-11  feat(query): port v2.0.0 rewrite/profiler/approx-aggregat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "query/query_profiler.h"

#include <limits>

namespace themis {
namespace query {

// ─────────────────────────────────────────────────────────────────────────────
// QueryProfile helpers
// ─────────────────────────────────────────────────────────────────────────────

const OperatorProfile* QueryProfile::slowestOperator() const {
    if (operators.empty()) return nullptr;
    const OperatorProfile* slowest = &operators[0];
    for (const auto& op : operators) {
        if (op.duration_ns > slowest->duration_ns) {
            slowest = &op;
        }
    }
    return slowest;
}

// ─────────────────────────────────────────────────────────────────────────────
// QueryProfiler
// ─────────────────────────────────────────────────────────────────────────────

void QueryProfiler::beginQuery(const std::string& query_text) {
    reset();
    profile_.query_text = query_text;
    query_start_ = std::chrono::steady_clock::now();
}

void QueryProfiler::endQuery(size_t result_rows, bool cache_hit) {
    const auto now = std::chrono::steady_clock::now();
    profile_.total_duration_ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(now - query_start_).count();
    profile_.result_rows = result_rows;
    profile_.cache_hit   = cache_hit;

    // Compute peak memory across operators.
    profile_.peak_memory_bytes = 0;
    for (const auto& op : profile_.operators) {
        if (op.memory_bytes > profile_.peak_memory_bytes) {
            profile_.peak_memory_bytes = op.memory_bytes;
        }
    }
}

void QueryProfiler::beginOperator(const std::string& operator_name) {
    current_operator_ = operator_name;
    op_start_         = std::chrono::steady_clock::now();
}

void QueryProfiler::endOperator(size_t rows_in, size_t rows_out,
                                 size_t memory_bytes, size_t io_reads) {
    const auto now = std::chrono::steady_clock::now();
    OperatorProfile op;
    op.operator_name = current_operator_;
    op.duration_ns   =
        std::chrono::duration_cast<std::chrono::nanoseconds>(now - op_start_).count();
    op.rows_in      = rows_in;
    op.rows_out     = rows_out;
    op.memory_bytes = memory_bytes;
    op.io_reads     = io_reads;
    profile_.operators.push_back(std::move(op));
    current_operator_.clear();
}

QueryProfile QueryProfiler::getProfile() const {
    return profile_;
}

void QueryProfiler::reset() {
    profile_ = QueryProfile{};
    current_operator_.clear();
}

} // namespace query
} // namespace themis
