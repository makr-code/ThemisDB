/*
 * ThemisDB | File: continuous_query_engine.cpp | Version: 0.0.1 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 310
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=166 | delta=163 | status=divergent
 * External Severity (v3): C=15, H=139, M=12
 * PR: none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "query/continuous_query_engine_impl.h"
#include "utils/error_registry.h"

#include <algorithm>
#include <chrono>
#include <thread>

namespace themis {
namespace query {

// ──────────────────────────────────────────────────────────────────────────────
// ResultQueue
// ──────────────────────────────────────────────────────────────────────────────

ResultQueue::ResultQueue(size_t capacity) : capacity_(capacity) {}

void ResultQueue::push(CQResult item) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.size() >= capacity_) {
        // Drop oldest to prevent unbounded growth
        queue_.pop_front();
    }
    queue_.push_back(std::move(item));
    cv_.notify_one();
}

std::optional<CQResult> ResultQueue::pop(std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!cv_.wait_for(lock, timeout, [this] {
            return !queue_.empty() || cancelled_.load(std::memory_order_acquire);
        })) {
        return std::nullopt;  // timeout
    }
    if (cancelled_.load(std::memory_order_acquire) && queue_.empty()) {
        return std::nullopt;
    }
    auto item = std::move(queue_.front());
    queue_.pop_front();
    return item;
}

void ResultQueue::cancel() noexcept {
    cancelled_.store(true, std::memory_order_release);
    cv_.notify_all();
}

size_t ResultQueue::depth() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

bool ResultQueue::isCancelled() const noexcept {
    return cancelled_.load(std::memory_order_acquire);
}

// ──────────────────────────────────────────────────────────────────────────────
// CQResultStreamImpl
// ──────────────────────────────────────────────────────────────────────────────

CQResultStreamImpl::CQResultStreamImpl(std::shared_ptr<ResultQueue> queue)
    : queue_(std::move(queue)) {}

bool CQResultStreamImpl::hasMore() const noexcept {
    return !queue_->isCancelled();
}

std::optional<CQResult> CQResultStreamImpl::next(
    std::chrono::milliseconds timeout) {
    return queue_->pop(timeout);
}

void CQResultStreamImpl::cancel() noexcept {
    queue_->cancel();
}

size_t CQResultStreamImpl::queueDepth() const noexcept {
    return queue_->depth();
}

// ──────────────────────────────────────────────────────────────────────────────
// ContinuousQueryEngineImpl — lifecycle
// ──────────────────────────────────────────────────────────────────────────────

ContinuousQueryEngineImpl::ContinuousQueryEngineImpl(
    std::chrono::milliseconds tick_interval)
    : tick_interval_(tick_interval) {
    startLoop();
}

ContinuousQueryEngineImpl::~ContinuousQueryEngineImpl() {
    stopLoop();
    // Cancel all subscriber queues
    std::lock_guard<std::mutex> lock(registry_mutex_);
    for (auto& [name, entry] : registry_) {
        for (auto& q : entry.subscribers) {
            q->cancel();
        }
    }
}

void ContinuousQueryEngineImpl::startLoop() {
    running_.store(true, std::memory_order_release);
    loop_thread_ = std::thread([this] {
        while (running_.load(std::memory_order_acquire)) {
            {
                std::unique_lock<std::mutex> lock(loop_mutex_);
                loop_cv_.wait_for(lock, tick_interval_);
            }
            if (running_.load(std::memory_order_acquire)) {
                tickOnce();
            }
        }
    });
}

void ContinuousQueryEngineImpl::stopLoop() {
    running_.store(false, std::memory_order_release);
    loop_cv_.notify_all();
    if (loop_thread_.joinable()) {
        loop_thread_.join();
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// ContinuousQueryEngineImpl — registerQuery / dropQuery / subscribe
// ──────────────────────────────────────────────────────────────────────────────

Result<ContinuousQueryEngine::ContinuousQueryHandle>
ContinuousQueryEngineImpl::registerQuery(ContinuousQuerySpec spec) {
    // Compile plan (performs validation)
    auto plan_result = planner_.compile(spec);
    if (!plan_result) {
        return tl::unexpected(plan_result.error());
    }

    std::lock_guard<std::mutex> lock(registry_mutex_);
    if (registry_.count(spec.name)) {
        return Err<ContinuousQueryHandle>(
            errors::ErrorCode::ERR_QUERY_INVALID,
            "continuous query already registered: " + spec.name);
    }

    QueryRegistryEntry entry;
    entry.spec     = spec;
    entry.plan     = std::move(*plan_result);
    entry.synopsis = std::make_unique<SynopsisStore>(
        spec.max_window_tuples, spec.max_window_bytes);
    entry.watermark = std::make_unique<CQWatermark>(spec.allowed_lateness_ms);

    // Populate info
    entry.info.name               = spec.name;
    entry.info.source_collection  = spec.source_collection;
    entry.info.window             = spec.window;
    entry.info.result_mode        = spec.result_mode;
    entry.info.registered_at      = std::chrono::system_clock::now();

    const auto name = spec.name;
    registry_.emplace(name, std::move(entry));
    return name;
}

Result<void> ContinuousQueryEngineImpl::dropQuery(const std::string& name) {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    auto it = registry_.find(name);
    if (it == registry_.end()) {
        return ErrVoid(errors::ErrorCode::ERR_QUERY_INVALID,
                       "continuous query not found: " + name);
    }
    // Cancel all subscriber queues
    for (auto& q : it->second.subscribers) {
        q->cancel();
    }
    registry_.erase(it);
    return OkVoid();
}

Result<ContinuousQueryEngine::ResultStreamPtr>
ContinuousQueryEngineImpl::subscribe(const std::string& name,
                                      ResultMode /*mode*/) {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    auto it = registry_.find(name);
    if (it == registry_.end()) {
        return Err<ResultStreamPtr>(errors::ErrorCode::ERR_QUERY_INVALID,
                                    "continuous query not found: " + name);
    }
    auto queue  = std::make_shared<ResultQueue>();
    auto stream = std::make_shared<CQResultStreamImpl>(queue);
    it->second.subscribers.push_back(std::move(queue));
    return stream;
}

std::vector<ContinuousQueryInfo>
ContinuousQueryEngineImpl::listQueries() const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    std::vector<ContinuousQueryInfo> result;
    result.reserve(registry_.size());
    for (const auto& [name, entry] : registry_) {
        auto info = entry.info;
        // Update live depth across all subscriber queues
        size_t total_depth = 0;
        for (const auto& q : entry.subscribers) {
            total_depth += q->depth();
        }
        info.result_queue_depth = total_depth;
        result.push_back(info);
    }
    return result;
}

void ContinuousQueryEngineImpl::injectTuple(const std::string& collection,
                                             const std::string& tuple,
                                             int64_t            event_ts) {
    std::lock_guard<std::mutex> lock(inject_mutex_);
    inject_queue_.push_back({collection, tuple, event_ts});
}

// ──────────────────────────────────────────────────────────────────────────────
// ContinuousQueryEngineImpl — evaluation tick
// ──────────────────────────────────────────────────────────────────────────────

void ContinuousQueryEngineImpl::tickOnce() {
    // Drain the injection queue into the relevant synopsis stores
    {
        std::lock_guard<std::mutex> inj_lock(inject_mutex_);
        if (!inject_queue_.empty()) {
            std::lock_guard<std::mutex> reg_lock(registry_mutex_);
            for (auto& incoming : inject_queue_) {
                for (auto& [name, entry] : registry_) {
                    if (entry.spec.source_collection == incoming.collection) {
                        entry.watermark->observe(incoming.event_ts_us);
                        SynopsisTuple st{incoming.event_ts_us, incoming.payload};
                        const bool ok = entry.synopsis->insert(std::move(st));
                        if (ok) {
                            // Emit addition to all subscribers (DELTA / CHANGES)
                            if (entry.spec.result_mode == ResultMode::DELTA ||
                                entry.spec.result_mode == ResultMode::CHANGES) {
                                CQResult r{incoming.payload, false};
                                for (auto& q : entry.subscribers) {
                                    q->push(r);
                                }
                            }
                            entry.info.tuples_processed++;
                        }
                    }
                }
            }
            inject_queue_.clear();
        }
    }

    // Run the evaluation plan for each registered query
    {
        std::lock_guard<std::mutex> lock(registry_mutex_);
        for (auto& [name, entry] : registry_) {
            // Build a transient ContinuousQueryState view
            ContinuousQueryState state;
            state.spec     = entry.spec;
            state.info     = entry.info;
            state.synopsis = std::unique_ptr<SynopsisStore>(entry.synopsis.get());
            state.watermark = std::unique_ptr<CQWatermark>(entry.watermark.get());

            std::vector<CQResult> tick_results;
            entry.plan.evaluate(state, tick_results);

            // Restore ownership (state held non-owning views)
            (void)state.synopsis.release();
            (void)state.watermark.release();

            entry.info = state.info;

            // Push results to all subscriber queues
            for (const auto& r : tick_results) {
                for (auto& q : entry.subscribers) {
                    q->push(r);
                }
            }

            // For SNAPSHOT mode push current synopsis every tick
            if (entry.spec.result_mode == ResultMode::SNAPSHOT) {
                auto snap = entry.synopsis->snapshot();
                for (const auto& t : snap) {
                    CQResult r{t.payload, false};
                    for (auto& q : entry.subscribers) {
                        q->push(r);
                    }
                }
            }
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Factory function
// ──────────────────────────────────────────────────────────────────────────────

std::unique_ptr<ContinuousQueryEngine> makeContinuousQueryEngine(
    std::chrono::milliseconds tick_interval) {
    return std::make_unique<ContinuousQueryEngineImpl>(tick_interval);
}

}  // namespace query
}  // namespace themis
