/**
 * @file continuous_query_engine_impl.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "query/continuous_query_engine.h"
#include "query/continuous_query_planner.h"
#include "query/synopsis_store.h"
#include "query/cq_watermark.h"

#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <atomic>

namespace themis {
namespace query {

// ──────────────────────────────────────────────────────────────────────────────
// Bounded result queue consumed by CQResultStreamImpl
// ──────────────────────────────────────────────────────────────────────────────

/** Maximum items buffered per subscriber before oldest entries are dropped. */
static constexpr size_t kDefaultResultQueueCapacity = 65536;

class ResultQueue {
public:
    explicit ResultQueue(size_t capacity = kDefaultResultQueueCapacity);

    /** Push item; drops oldest if capacity is exceeded. */
    void push(CQResult item);

    /**
     * @brief Pop next item, blocking up to `timeout`.
     * @return item, or empty on timeout or after cancel().
     */
    std::optional<CQResult> pop(std::chrono::milliseconds timeout);

    /** Signal all blocked pop() callers to return immediately. */
    void cancel() noexcept;

    [[nodiscard]] size_t depth() const noexcept;
    [[nodiscard]] bool   isCancelled() const noexcept;

private:
    mutable std::mutex       mutex_;
    std::condition_variable  cv_;
    std::deque<CQResult>     queue_;
    size_t                   capacity_{kDefaultResultQueueCapacity};
    std::atomic<bool>        cancelled_{false};
};

// ──────────────────────────────────────────────────────────────────────────────
// CQResultStreamImpl — concrete CQResultStream
// ──────────────────────────────────────────────────────────────────────────────

class CQResultStreamImpl : public CQResultStream {
public:
    ~CQResultStreamImpl() override = default;
    explicit CQResultStreamImpl(std::shared_ptr<ResultQueue> queue);

    bool hasMore() const noexcept override;
    std::optional<CQResult> next(std::chrono::milliseconds timeout) override;
    void   cancel() noexcept override;
    size_t queueDepth() const noexcept override;

private:
    std::shared_ptr<ResultQueue> queue_;
};

// ──────────────────────────────────────────────────────────────────────────────
// Registry entry for one registered query
// ──────────────────────────────────────────────────────────────────────────────

struct QueryRegistryEntry {
    ContinuousQuerySpec                          spec;
    ContinuousQueryInfo                          info;
    ContinuousPlan                               plan;
    std::unique_ptr<SynopsisStore>               synopsis;
    std::unique_ptr<CQWatermark>                 watermark;
    std::vector<std::shared_ptr<ResultQueue>>    subscribers;
};

// ──────────────────────────────────────────────────────────────────────────────
// ContinuousQueryEngineImpl
// ──────────────────────────────────────────────────────────────────────────────

/**
 * @brief Concrete implementation of ContinuousQueryEngine.
 *
 * A single background thread drives the evaluation loop, waking every
 * `tick_interval_ms` milliseconds and evaluating each registered query.
 */
class ContinuousQueryEngineImpl : public ContinuousQueryEngine {
public:
    explicit ContinuousQueryEngineImpl(
        std::chrono::milliseconds tick_interval = std::chrono::milliseconds{100});
    ~ContinuousQueryEngineImpl() override;

    // ContinuousQueryEngine interface
    Result<ContinuousQueryHandle> registerQuery(ContinuousQuerySpec spec) override;
    Result<void>                  dropQuery(const std::string& name) override;
    Result<ResultStreamPtr>       subscribe(const std::string& name,
                                            ResultMode mode) override;
    std::vector<ContinuousQueryInfo> listQueries() const override;
    void injectTuple(const std::string& collection,
                     const std::string& tuple,
                     int64_t            event_ts) override;

private:
    void startLoop();
    void stopLoop();
    void tickOnce();

    ContinuousQueryPlanner planner_;
    std::chrono::milliseconds tick_interval_;

    mutable std::mutex                                         registry_mutex_;
    std::unordered_map<std::string, QueryRegistryEntry>        registry_;

    // Incoming tuple injection queue
    struct IncomingTuple {
        std::string collection;
        std::string payload;
        int64_t     event_ts_us = 0;
    };
    std::mutex                    inject_mutex_;
    std::deque<IncomingTuple>     inject_queue_;

    std::thread               loop_thread_;
    std::atomic<bool>         running_{false};
    std::mutex                loop_mutex_;
    std::condition_variable   loop_cv_;
};

}  // namespace query
}  // namespace themis
