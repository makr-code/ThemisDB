/**
 * @file continuous_query_engine_impl.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

static constexpr size_t kDefaultResultQueueCapacity = 65536;

/**
 * @brief Bounded queue used to fan out query results to subscribed consumers.
 *
 * The queue provides a single-producer/single-consumer style handoff for result
 * batches emitted by `ContinuousQueryEngineImpl`. It serializes access with a
 * mutex and wakes blocked readers via a condition variable. When the queue is at
 * capacity the oldest result is dropped to keep the consumer side responsive under
 * bursty workloads.
 *
 * @note The queue is intentionally bounded: callers must treat dropped items as a
 * normal backpressure mechanism rather than as an error condition.
 */
class ResultQueue {
public:
    explicit ResultQueue(size_t capacity = kDefaultResultQueueCapacity);

    /**
     * @brief Push.
     * @param[in] item Input parameter.
     */
    void push(CQResult item);

    /**
     * @brief Pop.
     * @param[in] timeout Input parameter.
     * @return Return value.
     */
    std::optional<CQResult> pop(std::chrono::milliseconds timeout);

    /**
     * @brief Cancel.
     * @note Exception safety: noexcept.
     */
    void cancel() noexcept;

    /**
     * @brief Returns the number of queued results currently waiting to be read.
     * @return Number of buffered result items in the queue.
     * @note The value is sampled while holding the queue mutex, so it reflects a
     * consistent snapshot of the queue depth for a single call.
     */
    [[nodiscard]] size_t depth() const noexcept;

    /**
     * @brief Reports whether the queue has been cancelled for shutdown.
     * @return `true` if cancellation has been requested; otherwise `false`.
     */
    [[nodiscard]] bool isCancelled() const noexcept;

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
    /**
     * @brief CQResult Stream Impl.
     * @param[in] queue Input parameter.
     * @return Return value.
     */
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
    /**
     * @brief Start Loop.
     */
    void startLoop();
    /**
     * @brief Stop Loop.
     */
    void stopLoop();
    /**
     * @brief Tick Once.
     */
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
    std::atomic<bool>         loop_stop_complete_{false};
    std::mutex                loop_mutex_;
    std::condition_variable   loop_cv_;
};

}  // namespace query
}  // namespace themis
