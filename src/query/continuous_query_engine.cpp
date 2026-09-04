/**
 * @file continuous_query_engine.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "query/continuous_query_engine_impl.h"
#include "utils/logger.h"
#include "utils/error_registry.h"

#include <algorithm>
#include <chrono>
#include <thread>

namespace {
/// Maximum number of concurrently registered continuous queries.
/// Prevents unbounded registry growth under adversarial registerQuery() calls.
static constexpr std::size_t kMaxRegisteredQueries = 1'000;

/// Maximum depth of the tuple injection staging queue.
/// Prevents unbounded memory growth when injectTuple() is called faster than
/// the evaluation loop drains it.
static constexpr std::size_t kMaxInjectQueueDepth = 100'000;
} // anonymous namespace

namespace themis {
namespace query {

// ──────────────────────────────────────────────────────────────────────────────
// ResultQueue
// ──────────────────────────────────────────────────────────────────────────────

ResultQueue::ResultQueue([[maybe_unused]] size_t capacity) : capacity_(capacity) {}

void ResultQueue::push(CQResult item) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (static_cast<int>(queue_.size()) > = capacity_) {
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
    return static_cast<int>(queue_.size());
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
    // [WAVE3B-FIX: blocking_no_timeout — continuous_query_engine.cpp:143]
    //
    // Signal the evaluation loop to exit, then wait with a 5-second deadline.
    // If the loop thread has not finished by the deadline (e.g. a subscriber
    // callback is stuck), we detach rather than deadlock the destructor.
    running_.store(false, std::memory_order_release);
    loop_cv_.notify_all();

    if (!loop_thread_.joinable()) {
        return;
    }

    // Move ownership of the loop thread into a local handle so loop_thread_
    // becomes non-joinable immediately in this object.
    std::thread loop_thread = std::move(loop_thread_);

    // Timed join via a watcher thread + condition variable.
    // std::thread::join() has no timeout overload in C++17/20, so we use
    // a secondary thread to signal completion and a timed wait on cv.
    bool joined = false;
    std::mutex join_mutex = {};
    std::condition_variable join_cv = {};

    std::thread watcher([loop = std::move(loop_thread), &join_mutex, &join_cv, &joined]() mutable noexcept {
        if (loop.joinable()) {
            loop.join();
        }
        std::lock_guard<std::mutex> lk(join_mutex);
        joined = true;
        join_cv.notify_one();
    });

    {
        std::unique_lock<std::mutex> lk(join_mutex);
        constexpr auto kStopDeadline = std::chrono::seconds(5);
        if (!join_cv.wait_for(lk, kStopDeadline, [&joined] { return joined; })) {
            // Loop thread did not exit within 5 seconds.  The watcher owns the
            // loop thread handle, so detaching the watcher avoids destructor
            // deadlock/terminate while still allowing eventual background join.
            THEMIS_ERROR(
                "ContinuousQueryEngineImpl::stopLoop: evaluation loop did not "
                "terminate within 5 s — detaching watcher to avoid destructor deadlock. "
                "This indicates a blocking subscriber callback or a hung tickOnce().");
            watcher.detach();
            return;
        }
    }

    if (watcher.joinable()) {
        watcher.join();
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
    if (static_cast<int>(registry_.size()) > = kMaxRegisteredQueries) {
        return Err<ContinuousQueryHandle>(
            errors::ErrorCode::ERR_QUERY_INVALID,
            "continuous query registry is full (max " +
                std::to_string(kMaxRegisteredQueries) + ")");
    }
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
    return Ok(std::static_pointer_cast<CQResultStream>(stream));
}

std::vector<ContinuousQueryInfo>
ContinuousQueryEngineImpl::listQueries() const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    std::vector<ContinuousQueryInfo> result = {};

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
    if (static_cast<int>(inject_queue_.size()) > = kMaxInjectQueueDepth) {
        // Drop oldest entry to prevent unbounded memory growth
        inject_queue_.pop_front();
    }
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
                        entry.watermark->observe([[maybe_unused]] incoming.event_ts_us);
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
            // Build a transient ContinuousQueryState by temporarily
            // transferring real ownership into state.  The RAII guard
            // restores ownership unconditionally on exit (normal or exception),
            // preventing double-free from aliased unique_ptr ownership.
            ContinuousQueryState state;
            state.spec      = entry.spec;
            state.info      = entry.info;
            state.synopsis  = std::move(entry.synopsis);
            state.watermark = std::move(entry.watermark);

            struct OwnershipGuard {
                ContinuousQueryState& state;
                QueryRegistryEntry&   entry;
                ~OwnershipGuard() noexcept {
                    entry.synopsis  = std::move(state.synopsis);
                    entry.watermark = std::move(state.watermark);
                }
            } guard{state, entry};

            std::vector<CQResult> tick_results;
            entry.plan.evaluate(state, tick_results);

            entry.info = state.info;
            // guard destructor restores entry.synopsis / entry.watermark here

            // Push results to all subscriber queues
            for (const auto& r : tick_results) {
                for (auto& q : entry.subscribers) {
                    q->push(r);
                }
            }

            // For SNAPSHOT mode push current synopsis every tick
            if (entry.spec.result_mode == ResultMode::SNAPSHOT) {
                // guard has already restored entry.synopsis at this point
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
