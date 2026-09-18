/**
 * @file request_coalescer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 82/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

class RequestCoalescer {
public:
    struct Result {
        bool        success{false};
        std::string data;    ///< JSON payload returned by fn()
        uint64_t    version{0};
        std::string error;   ///< Exception message when success == false
    };

    template<typename Fn>
    /**
     * @brief Do.
     * @param[in] key Input parameter.
     * @param[in] fn Input parameter.
     * @return Return value.
     * @details Calls: lk(), find(), end(), get_future(), share(), emplace(), get(), fn().
     */
    std::shared_ptr<Result> Do(const std::string& key, Fn&& fn) {
        std::shared_future<std::shared_ptr<Result>> fut;
        std::shared_ptr<std::promise<std::shared_ptr<Result>>> prom;
        bool is_owner = false;

        {
            std::lock_guard<std::mutex> lk(mu_);
            auto it = inflight_.find(key);
            if (it != inflight_.end()) {
                // Join existing flight — do NOT hold the lock while waiting.
                fut = it->second;
            } else {
                // We are the owner: insert a shared_future backed by a promise.
                prom = std::make_shared<std::promise<std::shared_ptr<Result>>>();
                fut = prom->get_future().share();
                inflight_.emplace(key, fut);
                is_owner = true;
            }
        }

        if (!is_owner) {
            // Block until the owner fulfils the promise, then return shared result.
            return fut.get();
        }

        // We are the owner: execute fn() outside the mutex.
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

    size_t inflight_count() const {
        /**
         * @brief Lk.
         * @param[in] mu_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lk(mu_);
        return inflight_.size();
    }

private:
    mutable std::mutex mu_;
    std::unordered_map<std::string,
                       std::shared_future<std::shared_ptr<Result>>> inflight_;
};

}} // namespace themis::cache
