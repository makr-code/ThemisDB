/**
 * @file query_profiler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
