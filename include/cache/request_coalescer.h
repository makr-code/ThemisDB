/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            request_coalescer.h                                ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:33:38                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     159                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 040083b025  2026-04-12  feat: StreamingIngestManager, TsStreamCursor, LZ4 compres... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include "utils/logger.h"

namespace themis { namespace cache {

/**
 * @brief Singleflight request coalescer — eliminates thundering-herd on cache misses.
 *
 * When multiple threads call `Do(key, fn)` concurrently with the same key, only
 * the *first* call actually executes `fn()`.  All subsequent callers for the same
 * key block and receive the same `shared_ptr<Result>` once the first call
 * completes.  This reduces redundant backend fetches to exactly one per key
 * during a cache-miss storm.
 *
 * ### Behaviour
 * - `fn` is called exactly once per in-flight key regardless of the number of
 *   concurrent callers.
 * - The `Result` is shared (by value-copy inside the struct) across all waiters.
 * - If `fn` throws, every waiter receives a `Result{success=false}` and the
 *   exception message is stored in `Result::error`.
 * - After the flight completes the key is removed so subsequent independent
 *   requests start a new flight.
 *
 * ### Thread safety
 * All methods are fully thread-safe.  The hot-path mutex is held only while
 * inserting or looking up the inflight entry, not during the execution of `fn`.
 *
 * ### Performance target
 * - Under contention: ≥ N–1 threads saved from redundant backend calls
 * - Lock hold-time: O(1), unordered_map lookup + shared_future construction only
 */
class RequestCoalescer {
public:
    struct Result {
        bool        success{false};
        std::string data;    ///< JSON payload returned by fn()
        uint64_t    version{0};
        std::string error;   ///< Exception message when success == false
    };

    /**
     * @brief Execute `fn` for `key`, or join an already-running flight.
     *
     * @tparam F  Callable with signature `Result()`.
     * @param  key  Logical cache key.
     * @param  fn   Factory function that fetches / computes the value.
     * @return Shared pointer to the result produced by the first caller.
     *         All concurrent callers for the same key receive the same pointer.
     */
    template<typename Fn>
    std::shared_ptr<Result> Do(const std::string& key, Fn&& fn) {
        std::shared_future<std::shared_ptr<Result>> fut;
        bool is_owner = false;

        {
            std::lock_guard<std::mutex> lk(mu_);
            auto it = inflight_.find(key);
            if (it != inflight_.end()) {
                // Join existing flight — do NOT hold the lock while waiting.
                fut = it->second;
            } else {
                // We are the owner: insert a shared_future backed by a promise.
                auto prom = std::make_shared<std::promise<std::shared_ptr<Result>>>();
                fut = prom->get_future().share();
                inflight_.emplace(key, fut);
                is_owner = true;
                // Store the promise so we can fulfil it after releasing the lock.
                owner_promise_ = std::move(prom);
            }
        }

        if (!is_owner) {
            // Block until the owner fulfils the promise, then return shared result.
            return fut.get();
        }

        // We are the owner: execute fn() outside the mutex.
        auto prom = std::move(owner_promise_);
        std::shared_ptr<Result> res;
        try {
            res = std::make_shared<Result>(fn());
        } catch (const std::exception& ex) {
            THEMIS_WARN("RequestCoalescer: fn() threw for key '{}': {}", key, ex.what());
            res = std::make_shared<Result>();
            res->success = false;
            res->error   = ex.what();
        } catch (...) {
            THEMIS_WARN("RequestCoalescer: fn() threw unknown exception for key '{}'", key);
            res = std::make_shared<Result>();
            res->success = false;
            res->error   = "unknown exception";
        }

        // Fulfil the promise — unblocks all waiters.
        prom->set_value(res);

        // Remove flight entry so future calls for the same key start fresh.
        {
            std::lock_guard<std::mutex> lk(mu_);
            inflight_.erase(key);
        }

        return res;
    }

    /**
     * @brief Number of currently in-flight fetches.
     *
     * Useful for testing and monitoring.  The value is approximate in
     * multi-threaded contexts.
     */
    size_t inflight_count() const {
        std::lock_guard<std::mutex> lk(mu_);
        return inflight_.size();
    }

private:
    mutable std::mutex mu_;
    std::unordered_map<std::string,
                       std::shared_future<std::shared_ptr<Result>>> inflight_;

    // Temporary storage for the owner's promise between lock release and fn() call.
    // Only written/read by the owner thread before fn() is called; no sharing needed.
    std::shared_ptr<std::promise<std::shared_ptr<Result>>> owner_promise_;
};

}} // namespace themis::cache
