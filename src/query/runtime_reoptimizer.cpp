/**
 * @file runtime_reoptimizer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "query/runtime_reoptimizer.h"
#include "utils/hash_util.h"
#include <spdlog/spdlog.h>
#include <iomanip>
#include <sstream>

namespace themis {

// ---------------------------------------------------------------------------
// FNV-1a 64-bit hash (public domain)
// ---------------------------------------------------------------------------
static std::string fnv1a_hex(const std::string& text) {
    return themis::hash::fnv1a64_hex(text);
}

// ---------------------------------------------------------------------------
// ExecutionGuard
// ---------------------------------------------------------------------------

RuntimeReoptimizer::ExecutionGuard::ExecutionGuard(RuntimeReoptimizer& owner,
                                                    ExecutionContext ctx)
    : owner_(&owner), ctx_(std::move(ctx)) {}

RuntimeReoptimizer::ExecutionGuard::ExecutionGuard(ExecutionGuard&& other) noexcept
    : owner_(other.owner_),
      ctx_(std::move(other.ctx_)),
      actual_rows_(other.actual_rows_),
      finished_(other.finished_) {
    other.owner_    = nullptr;
    other.finished_ = true;
}

RuntimeReoptimizer::ExecutionGuard&
RuntimeReoptimizer::ExecutionGuard::operator=(ExecutionGuard&& other) noexcept {
    if (this != &other) {
        if (!finished_ && owner_) {
            finish(actual_rows_);
        }
        owner_       = other.owner_;
        ctx_         = std::move(other.ctx_);
        actual_rows_ = other.actual_rows_;
        finished_    = other.finished_;
        other.owner_    = nullptr;
        other.finished_ = true;
    }
    return *this;
}

RuntimeReoptimizer::ExecutionGuard::~ExecutionGuard() {
    if (!finished_ && owner_) {
        finish(actual_rows_);
    }
}

void RuntimeReoptimizer::ExecutionGuard::finish([[maybe_unused]] size_t actual_rows) {
    if (finished_ || !owner_) {
        return;
    }
    finished_    = true;
    actual_rows_ = actual_rows;

    auto elapsed_ms = std::chrono::duration<double, std::milli>(
                          std::chrono::steady_clock::now() - ctx_.start_time)
                          .count();

    owner_->recordExecution(ctx_.query_hash, ctx_.estimated_rows,
                             actual_rows, elapsed_ms);
}

// ---------------------------------------------------------------------------
// RuntimeReoptimizer
// ---------------------------------------------------------------------------

RuntimeReoptimizer::RuntimeReoptimizer()
    : stats_(std::make_shared<AdaptiveQueryStats>()),
      selector_(std::make_shared<AdaptivePlanSelector>()) {}

std::string RuntimeReoptimizer::computeQueryHash(const std::string& aql_text) {
    return fnv1a_hex(aql_text);
}

RuntimeReoptimizer::ExecutionContext
RuntimeReoptimizer::beginExecution(const std::string& query_hash,
                                    size_t estimated_rows) const {
    ExecutionContext ctx;
    ctx.query_hash     = query_hash;
    ctx.estimated_rows = estimated_rows;
    ctx.start_time     = std::chrono::steady_clock::now();
    return ctx;
}

RuntimeReoptimizer::ExecutionGuard
RuntimeReoptimizer::beginExecutionGuard(const std::string& query_hash,
                                         size_t estimated_rows) {
    // When no optimizer estimate is provided, fall back to the historical
    // average of actual rows so that adjustment factors can still be computed.
    if (estimated_rows == 0) {
        estimated_rows = stats_->getAverageActualRows(query_hash);
    }
    return ExecutionGuard(*this, beginExecution(query_hash, estimated_rows));
}

bool RuntimeReoptimizer::shouldReoptimize([[maybe_unused]] const std::string& query_hash,
                                           size_t rows_so_far,
                                           size_t estimated_total,
                                           double progress,
                                           double threshold) const {
    if (!enabled_) {
        return false;
    }
    return selector_->shouldSwitchPlan(rows_so_far, estimated_total, progress,
                                       threshold);
}

void RuntimeReoptimizer::recordExecution(const std::string& query_hash,
                                          size_t estimated_rows,
                                          size_t actual_rows,
                                          double execution_time_ms) {
    AdaptiveQueryStats::QueryExecution exec;
    exec.query_hash        = query_hash;
    exec.estimated_rows    = estimated_rows;
    exec.actual_rows       = actual_rows;
    exec.execution_time_ms = execution_time_ms;
    exec.selectivity       = estimated_rows > 0
                                 ? static_cast<double>(actual_rows) / estimated_rows
                                 : 1.0;
    exec.timestamp         = std::chrono::system_clock::now();

    stats_->recordExecution(exec);

    spdlog::debug(
        "RuntimeReoptimizer: recorded hash={} est={} actual={} time_ms={:.2f}",
        query_hash, estimated_rows, actual_rows, execution_time_ms);
}

double RuntimeReoptimizer::getAdjustmentFactor(
    const std::string& query_hash) const {
    if (!enabled_) {
        return 1.0;
    }
    return stats_->getAdaptiveAdjustmentFactor(query_hash);
}

bool RuntimeReoptimizer::hasMisestimation(const std::string& query_hash,
                                           double threshold) const {
    return stats_->hasCardinalityMisestimation(query_hash, threshold);
}

void RuntimeReoptimizer::pruneOldStats(std::chrono::hours retention) {
    stats_->pruneOldStats(retention);
}

void RuntimeReoptimizer::enable([[maybe_unused]] bool enabled) {
    enabled_ = enabled;
    spdlog::info("RuntimeReoptimizer: re-optimization {}",
                 enabled ? "enabled" : "disabled");
}

bool RuntimeReoptimizer::isEnabled() const {
    return enabled_;
}

size_t RuntimeReoptimizer::totalExecutions() const {
    return stats_->getTotalQueries();
}

const AdaptiveQueryStats& RuntimeReoptimizer::stats() const {
    return *stats_;
}

} // namespace themis
