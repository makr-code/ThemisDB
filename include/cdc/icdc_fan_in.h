/**
 * @file icdc_fan_in.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB — CDC Multi-Source Fan-In Interface
 *
 * Provides a unified, globally-ordered (best-effort across sources) view of
 * CDC change events from multiple named Changefeed instances.
 *
 * Design constraints (from FUTURE_ENHANCEMENTS.md):
 *  - Events from the same source collection arrive in order.
 *  - Cross-collection ordering is best-effort (wall-clock timestamp + sequence).
 *  - FanInEvent wraps a ChangeEvent with the originating CollectionId.
 *  - IFanInMergePolicy is pluggable for custom conflict-resolution strategies.
 *
 * Copyright (c) 2025 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <algorithm>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "cdc/changefeed.h"

namespace themis {
namespace cdc {

// ── CollectionId ─────────────────────────────────────────────────────────────

using CollectionId = std::string;

// ── FanInEvent ────────────────────────────────────────────────────────────────

struct FanInEvent {
    CollectionId           collection; ///< Name of the originating collection
    Changefeed::ChangeEvent event;     ///< The underlying change event

    nlohmann::json toJson() const {
        nlohmann::json j = event.toJson();
        j["collection"] = collection;
        return j;
    }
};

// ── IFanInMergePolicy ─────────────────────────────────────────────────────────

class IFanInMergePolicy {
public:
    /**
     * @brief IFan In Merge Policy.
     * @return Return value.
     */
    virtual ~IFanInMergePolicy() = default;

    /**
     * @brief Merge.
     * @param[in,out] events Input/output parameter.
     */
    virtual void merge(std::vector<FanInEvent>& events) const = 0;
};

// ── TimestampMergePolicy ──────────────────────────────────────────────────────

class TimestampMergePolicy : public IFanInMergePolicy {
public:
    void merge(std::vector<FanInEvent>& events) const override {
        std::stable_sort(events.begin(), events.end(),
            [](const FanInEvent& a, const FanInEvent& b) {
                if (a.event.timestamp_ms != b.event.timestamp_ms) {
                    return a.event.timestamp_ms < b.event.timestamp_ms;
                }
                if (a.collection != b.collection) {
                    return a.collection < b.collection;
                }
                return a.event.sequence < b.event.sequence;
            });
    }
};

// ── ICDCFanIn ─────────────────────────────────────────────────────────────────

class ICDCFanIn {
public:
    /**
     * @brief ICDCFan In.
     * @return Return value.
     */
    virtual ~ICDCFanIn() = default;

    [[nodiscard]] virtual bool addSource(const CollectionId& id, Changefeed* feed) = 0;

    [[nodiscard]] virtual bool removeSource(const CollectionId& id) = 0;

    [[nodiscard]] virtual std::vector<FanInEvent> listEvents(
        uint64_t                         from_sequence  = 0,
        std::size_t                      limit          = 0,
        const std::vector<CollectionId>& collections    = {}) const = 0;

    /**
     * @brief Set Merge Policy.
     * @param[in] policy Input parameter.
     */
    virtual void setMergePolicy(std::unique_ptr<IFanInMergePolicy> policy) = 0;

    [[nodiscard]] virtual std::vector<CollectionId> sourceIds() const = 0;
};

// ── InMemoryFanIn ─────────────────────────────────────────────────────────────

class InMemoryFanIn : public ICDCFanIn {
public:
    InMemoryFanIn()
        : policy_(std::make_unique<TimestampMergePolicy>()) {}

    // ── ICDCFanIn ────────────────────────────────────────────────────────────

    bool addSource(const CollectionId& id, Changefeed* feed) override {
        /**
         * @brief Lk.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::unique_lock<std::mutex> lk(mutex_);
        if (sources_.count(id)) {
          return false;
        }
        sources_[id] = feed;
        return true;
    }

    bool removeSource(const CollectionId& id) override {
        /**
         * @brief Lk.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::unique_lock<std::mutex> lk(mutex_);
        return sources_.erase(id) > 0;
    }

    std::vector<FanInEvent> listEvents(
        uint64_t                         from_sequence = 0,
        std::size_t                      limit = 0,
        const std::vector<CollectionId>& collections = {}) const override
    {
        // Take a snapshot of sources under the lock, then query without holding it.
        std::unordered_map<CollectionId, Changefeed*> snapshot;
        {
            /**
             * @brief Lk.
             * @param[in] mutex_ Input parameter.
             * @return Return value.
             */
            std::unique_lock<std::mutex> lk(mutex_);
            snapshot = sources_;
        }

        std::vector<FanInEvent> merged = {};

        for (const auto& [id, feed] : snapshot) {
            if (!feed) {
              continue;
            }
            // Restrict to requested collections if specified.
            if (!collections.empty()) {
                bool found = false;
                for (const auto& c : collections) {
                    if (c == id) { found = true; break; }
                }
                if (!found) {
                  continue;
                }
            }

            Changefeed::ListOptions opts;
            opts.from_sequence = from_sequence;
            opts.limit         = limit
                                 ? limit
                                 : std::numeric_limits<std::size_t>::max();

            auto events = feed->listEvents(opts);
            for (auto& ev : events) {
                merged.push_back(FanInEvent{id, std::move(ev)});
            }
        }

        {
            /**
             * @brief Lk.
             * @param[in] mutex_ Input parameter.
             * @return Return value.
             */
            std::unique_lock<std::mutex> lk(mutex_);
            if (policy_) {
              policy_->merge(merged);
            }
        }

        if (limit && merged.size() > limit) {
            merged.resize(limit);
        }
        return merged;
    }

    void setMergePolicy(std::unique_ptr<IFanInMergePolicy> policy) override {
        /**
         * @brief Lk.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::unique_lock<std::mutex> lk(mutex_);
        policy_ = std::move(policy);
    }

    std::vector<CollectionId> sourceIds() const override {
        /**
         * @brief Lk.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::unique_lock<std::mutex> lk(mutex_);
        std::vector<CollectionId> ids = {};

        ids.reserve(sources_.size());
        for (const auto& [id, _] : sources_) {
          ids.push_back(id);
        }
        return ids;
    }

private:
    mutable std::mutex                              mutex_;
    std::unordered_map<CollectionId, Changefeed*>  sources_;
    std::unique_ptr<IFanInMergePolicy>              policy_;
};

} // namespace cdc
} // namespace themis
