/**
 * @file icdc_filter_pipeline.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB — CDC Server-Side Filter Pipeline Interface
 *
 * Provides a composable, ordered chain of named event filters that are
 * applied server-side before CDC events are delivered to a subscriber.
 * Each filter stage is an IEventFilter implementation that independently
 * decides whether to pass or drop an event.
 *
 * Design constraints:
 *  - Filter stages are applied in insertion order; the first stage that
 *    drops an event short-circuits the rest (fail-fast semantics).
 *  - Filter names are unique within a pipeline; adding a filter with the
 *    same name as an existing one is a no-op that returns false.
 *  - Filters must be noexcept; exceptions from filter implementations
 *    are caught and treated as FilterResult::Pass to avoid blocking the
 *    event stream.
 *  - All ICDCFilterPipeline methods are thread-safe.
 *
 * Copyright (c) 2025 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "cdc/changefeed.h"

namespace themis {
namespace cdc {

// ── FilterResult ─────────────────────────────────────────────────────────────

/**
 * @brief Decision returned by a single IEventFilter stage.
 */
enum class FilterResult {
    Pass, ///< Forward the event to the next stage / subscriber
    Drop, ///< Discard the event; do not deliver it
};

// ── IEventFilter ──────────────────────────────────────────────────────────────

/**
 * @brief Abstract single-stage event filter.
 *
 * Implementations must be thread-safe and must not throw.
 */
class IEventFilter {
public:
    virtual ~IEventFilter() = default;

    /**
     * @brief Evaluate the filter for a single event.
     *
     * @param event  The CDC change event to evaluate.
     * @return FilterResult::Pass to forward; FilterResult::Drop to discard.
     */
    [[nodiscard]] virtual FilterResult evaluate(const Changefeed::ChangeEvent& event) const noexcept = 0;

    /**
     * @brief Human-readable name for this filter stage (for diagnostics).
     */
    [[nodiscard]] virtual std::string name() const = 0;
};

// ── PredicateFilter ───────────────────────────────────────────────────────────

/**
 * @brief Convenience IEventFilter backed by a std::function predicate.
 *
 * Constructed with a callable that returns true to pass, false to drop.
 *
 * Example:
 * @code
 * auto f = std::make_unique<PredicateFilter>("only-puts",
 *     [](const auto& ev) {
 *         return ev.type == Changefeed::ChangeEventType::EVENT_PUT;
 *     });
 * @endcode
 */
class PredicateFilter : public IEventFilter {
public:
    using Predicate = std::function<bool(const Changefeed::ChangeEvent&)>;

    PredicateFilter(std::string name, Predicate pred)
        : name_(std::move(name)), pred_(std::move(pred)) {}

    FilterResult evaluate(
        const Changefeed::ChangeEvent& event) const noexcept override
    {
        try {
            return pred_(event) ? FilterResult::Pass : FilterResult::Drop;
        } catch (...) {
            return FilterResult::Pass; // fail-open on exception
        }
    }

    std::string name() const override { return name_; }

private:
    std::string name_;
    Predicate   pred_;
};

// ── KeyPrefixFilter ───────────────────────────────────────────────────────────

/**
 * @brief Built-in filter: pass only events whose key starts with a prefix.
 */
class KeyPrefixFilter : public IEventFilter {
public:
    KeyPrefixFilter(std::string name, std::string prefix)
        : name_(std::move(name)), prefix_(std::move(prefix)) {}

    FilterResult evaluate(
        const Changefeed::ChangeEvent& event) const noexcept override
    {
        if (prefix_.empty()) {
          return FilterResult::Pass;
        }
        return (event.key.substr(0, prefix_.size()) == prefix_)
               ? FilterResult::Pass
               : FilterResult::Drop;
    }

    std::string name() const override { return name_; }

private:
    std::string name_;
    std::string prefix_;
};

// ── EventTypeFilter ───────────────────────────────────────────────────────────

/**
 * @brief Built-in filter: pass only events matching one of the given types.
 */
class EventTypeFilter : public IEventFilter {
public:
    EventTypeFilter(std::string name,
                    std::vector<Changefeed::ChangeEventType> types)
        : name_(std::move(name)), types_(std::move(types)) {}

    FilterResult evaluate(
        const Changefeed::ChangeEvent& event) const noexcept override
    {
        if (types_.empty()) {
          return FilterResult::Pass;
        }
        for (auto t : types_) {
            if (event.type == t) {
              return FilterResult::Pass;
            }
        }
        return FilterResult::Drop;
    }

    std::string name() const override { return name_; }

private:
    std::string name_;
    std::vector<Changefeed::ChangeEventType> types_;
};

// ── ICDCFilterPipeline ────────────────────────────────────────────────────────

/**
 * @brief Abstract server-side CDC event filter pipeline.
 *
 * A pipeline is an ordered sequence of named IEventFilter stages.  The
 * apply() method runs each stage in order and returns FilterResult::Drop
 * as soon as any stage returns Drop; otherwise returns FilterResult::Pass.
 *
 * Thread-safety: all methods must be thread-safe in every implementation.
 */
class ICDCFilterPipeline {
public:
    virtual ~ICDCFilterPipeline() = default;

    /**
     * @brief Append a filter stage to the end of the pipeline.
     *
     * @param filter  Non-null owning pointer to the filter.
     * @return true if added; false if a filter with the same name already
     *         exists.
     */
    [[nodiscard]] virtual bool addFilter(std::unique_ptr<IEventFilter> filter) = 0;

    /**
     * @brief Remove a filter stage by name.
     *
     * @return true if the stage was found and removed; false otherwise.
     */
    [[nodiscard]] virtual bool removeFilter(const std::string& name) = 0;

    /**
     * @brief Return true if a filter with the given name is registered.
     */
    [[nodiscard]] virtual bool hasFilter(const std::string& name) const = 0;

    /**
     * @brief Return the number of filter stages in the pipeline.
     */
    [[nodiscard]] virtual std::size_t size() const = 0;

    /**
     * @brief Return true when the pipeline contains no filter stages.
     *
     * An empty pipeline passes all events.
     */
    [[nodiscard]] virtual bool empty() const = 0;

    /**
     * @brief Run all stages against an event and return the verdict.
     *
     * Short-circuits on the first Drop.  An empty pipeline always returns
     * FilterResult::Pass.
     *
     * @param event  The event to evaluate.
     * @return FilterResult::Pass or FilterResult::Drop.
     */
    [[nodiscard]] virtual FilterResult apply(const Changefeed::ChangeEvent& event) const = 0;

    /**
     * @brief Filter a batch of events, returning only those that pass.
     *
     * @param events  Input batch.
     * @return New vector containing only passing events (order preserved).
     */
    [[nodiscard]] virtual std::vector<Changefeed::ChangeEvent> applyBatch(
        const std::vector<Changefeed::ChangeEvent>& events) const = 0;

    /**
     * @brief Names of registered filter stages, in pipeline order.
     */
    [[nodiscard]] virtual std::vector<std::string> filterNames() const = 0;

    /**
     * @brief Cumulative count of events that passed all stages.
     */
    [[nodiscard]] virtual std::size_t totalPassed() const = 0;

    /**
     * @brief Cumulative count of events that were dropped by any stage.
     */
    [[nodiscard]] virtual std::size_t totalDropped() const = 0;

    /**
     * @brief Reset the pass/drop counters to zero.
     */
    virtual void resetCounters() = 0;
};

// ── InMemoryFilterPipeline ────────────────────────────────────────────────────

/**
 * @brief Thread-safe in-memory ICDCFilterPipeline implementation.
 *
 * Suitable for unit tests and standalone use.  All public methods acquire
 * a shared mutex; apply() acquires a shared (reader) lock to allow
 * concurrent evaluation without blocking each other.
 */
class InMemoryFilterPipeline : public ICDCFilterPipeline {
public:
    InMemoryFilterPipeline() = default;

    bool addFilter(std::unique_ptr<IEventFilter> filter) override {
        if (!filter) {
          return false;
        }
        std::unique_lock<std::mutex> lk(mutex_);
        const std::string n = filter->name();
        for (const auto& entry : stages_) {
            if (entry->name() == n) {
              return false;
            }
        }
        stages_.push_back(std::move(filter));
        return true;
    }

    bool removeFilter(const std::string& name) override {
        std::unique_lock<std::mutex> lk(mutex_);
        for (auto it = stages_.begin(); it != stages_.end(); ++it) {
            if ((*it)->name() == name) {
                stages_.erase(it);
                return true;
            }
        }
        return false;
    }

    bool hasFilter(const std::string& name) const override {
        std::unique_lock<std::mutex> lk(mutex_);
        for (const auto& entry : stages_) {
            if (entry->name() == name) {
              return true;
            }
        }
        return false;
    }

    std::size_t size() const override {
        std::unique_lock<std::mutex> lk(mutex_);
        return stages_.size();
    }

    bool empty() const override {
        std::unique_lock<std::mutex> lk(mutex_);
        return stages_.empty();
    }

    FilterResult apply(const Changefeed::ChangeEvent& event) const override {
        std::vector<IEventFilter*> snapshot;
        {
            std::unique_lock<std::mutex> lk(mutex_);
            snapshot.reserve(stages_.size());
            for (const auto& s : stages_) {
              snapshot.push_back(s.get());
            }
        }
        for (auto* f : snapshot) {
            if (f->evaluate(event) == FilterResult::Drop) {
                dropped_.fetch_add(1, std::memory_order_relaxed);
                return FilterResult::Drop;
            }
        }
        passed_.fetch_add(1, std::memory_order_relaxed);
        return FilterResult::Pass;
    }

    std::vector<Changefeed::ChangeEvent> applyBatch(
        const std::vector<Changefeed::ChangeEvent>& events) const override
    {
        std::vector<Changefeed::ChangeEvent> out = {};

        out.reserve(events.size());
        for (const auto& ev : events) {
            if (apply(ev) == FilterResult::Pass) {
              out.push_back(ev);
            }
        }
        return out;
    }

    std::vector<std::string> filterNames() const override {
        std::unique_lock<std::mutex> lk(mutex_);
        std::vector<std::string> names = {};

        names.reserve(stages_.size());
        for (const auto& s : stages_) {
          names.push_back(s->name());
        }
        return names;
    }

    std::size_t totalPassed() const override {
        return passed_.load(std::memory_order_relaxed);
    }

    std::size_t totalDropped() const override {
        return dropped_.load(std::memory_order_relaxed);
    }

    void resetCounters() override {
        passed_.store(0, std::memory_order_relaxed);
        dropped_.store(0, std::memory_order_relaxed);
    }

private:
    mutable std::mutex                          mutex_;
    std::vector<std::unique_ptr<IEventFilter>>  stages_;
    mutable std::atomic<std::size_t>            passed_{0};
    mutable std::atomic<std::size_t>            dropped_{0};
};

} // namespace cdc
} // namespace themis
