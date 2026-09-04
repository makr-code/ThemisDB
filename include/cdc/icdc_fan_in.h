/**
 * @file icdc_fan_in.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/// Strong typedef for a named collection identifier within a fan-in.
using CollectionId = std::string;

// ── FanInEvent ────────────────────────────────────────────────────────────────

/**
 * @brief A tagged ChangeEvent that carries its originating CollectionId.
 *
 * All fields of the underlying ChangeEvent are accessible via the `event`
 * member.  The `collection` field identifies which registered source produced
 * the event.
 */
struct FanInEvent {
    CollectionId           collection; ///< Name of the originating collection
    Changefeed::ChangeEvent event;     ///< The underlying change event

    /**
     * @brief Serialise to JSON, adding a top-level "collection" field.
     */
    nlohmann::json toJson() const {
        nlohmann::json j = event.toJson();
        j["collection"] = collection;
        return j;
    }
};

// ── IFanInMergePolicy ─────────────────────────────────────────────────────────

/**
 * @brief Pluggable merge / ordering policy for fan-in event streams.
 *
 * Implementations decide how to order or deduplicate FanInEvents from
 * multiple source collections when they are merged into a single stream.
 *
 * Thread-safety: implementations must be thread-safe.
 */
class IFanInMergePolicy {
public:
    virtual ~IFanInMergePolicy() = default;

    /**
     * @brief Sort (or otherwise reorder) a batch of fan-in events in-place.
     *
     * Called once per listEvents() invocation after events from all sources
     * have been gathered.  The default (wall-clock + lexicographic collection
     * tie-break) is provided by TimestampMergePolicy below.
     *
     * @param events  Mutable reference to the event batch to be reordered.
     */
    virtual void merge(std::vector<FanInEvent>& events) const = 0;
};

// ── TimestampMergePolicy ──────────────────────────────────────────────────────

/**
 * @brief Default merge policy: order by (timestamp_ms ASC, collection ASC,
 *        sequence ASC).
 *
 * This provides a globally deterministic total order when event clocks are
 * reasonably synchronised.
 */
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

/**
 * @brief Abstract multi-source CDC fan-in interface.
 *
 * Combines events from multiple registered Changefeed sources into a single
 * merged event stream.
 *
 * Thread-safety: all methods must be thread-safe in every implementation.
 */
class ICDCFanIn {
public:
    virtual ~ICDCFanIn() = default;

    /**
     * @brief Register a named CDC source.
     *
     * @param id    Unique collection identifier.
     * @param feed  Non-owning pointer to the Changefeed to subscribe from.
     * @return true if the source was added; false if id is already registered.
     */
    [[nodiscard]] virtual bool addSource(const CollectionId& id, Changefeed* feed) = 0;

    /**
     * @brief Deregister a named CDC source.
     *
     * @return true if the source was found and removed; false otherwise.
     */
    [[nodiscard]] virtual bool removeSource(const CollectionId& id) = 0;

    /**
     * @brief List all events from all (or a subset of) registered sources,
     *        merged according to the active IFanInMergePolicy.
     *
     * @param from_sequence  Only return events with sequence > from_sequence
     *                       (0 = return all events).
     * @param limit          Maximum number of events to return (0 = unlimited).
     * @param collections    If non-empty, restrict to these collection IDs.
     * @return Merged, ordered event batch.
     */
    [[nodiscard]] virtual std::vector<FanInEvent> listEvents(
        uint64_t                         from_sequence  = 0,
        std::size_t                      limit          = 0,
        const std::vector<CollectionId>& collections    = {}) const = 0;

    /**
     * @brief Set a custom merge policy.
     *
     * Replaces the previous policy.  Ownership is transferred to the fan-in.
     *
     * @param policy  Non-null pointer to the merge policy.
     */
    virtual void setMergePolicy(std::unique_ptr<IFanInMergePolicy> policy) = 0;

    /**
     * @brief Return the list of registered collection IDs.
     */
    [[nodiscard]] virtual std::vector<CollectionId> sourceIds() const = 0;
};

// ── InMemoryFanIn ─────────────────────────────────────────────────────────────

/**
 * @brief Thread-safe in-memory implementation of ICDCFanIn.
 *
 * Suitable for unit tests and standalone use (no RocksDB dependency).
 * Queries each registered Changefeed via its listEvents() API and merges
 * the results using the configured IFanInMergePolicy.
 */
class InMemoryFanIn : public ICDCFanIn {
public:
    InMemoryFanIn()
        : policy_(std::make_unique<TimestampMergePolicy>()) {}

    // ── ICDCFanIn ────────────────────────────────────────────────────────────

    bool addSource(const CollectionId& id, Changefeed* feed) override {
        std::unique_lock<std::mutex> lk(mutex_);
        if (sources_.count(id)) {
          return false;
        }
        sources_[id] = feed;
        return true;
    }

    bool removeSource(const CollectionId& id) override {
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
            std::unique_lock<std::mutex> lk(mutex_);
            snapshot = sources_;
        }

        std::vector<FanInEvent> merged;
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
        std::unique_lock<std::mutex> lk(mutex_);
        policy_ = std::move(policy);
    }

    std::vector<CollectionId> sourceIds() const override {
        std::unique_lock<std::mutex> lk(mutex_);
        std::vector<CollectionId> ids;
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
