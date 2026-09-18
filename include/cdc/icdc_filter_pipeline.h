/**
 * @file icdc_filter_pipeline.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

enum class FilterResult {
    Pass, ///< Forward the event to the next stage / subscriber
    Drop, ///< Discard the event; do not deliver it
};

// ── IEventFilter ──────────────────────────────────────────────────────────────

class IEventFilter {
public:
    /**
     * @brief IEvent Filter.
     * @return Return value.
     */
    virtual ~IEventFilter() = default;

    [[nodiscard]] virtual FilterResult evaluate(const Changefeed::ChangeEvent& event) const noexcept = 0;

    [[nodiscard]] virtual std::string name() const = 0;
};

// ── PredicateFilter ───────────────────────────────────────────────────────────

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

class ICDCFilterPipeline {
public:
    /**
     * @brief ICDCFilter Pipeline.
     * @return Return value.
     */
    virtual ~ICDCFilterPipeline() = default;

    [[nodiscard]] virtual bool addFilter(std::unique_ptr<IEventFilter> filter) = 0;

    [[nodiscard]] virtual bool removeFilter(const std::string& name) = 0;

    [[nodiscard]] virtual bool hasFilter(const std::string& name) const = 0;

    [[nodiscard]] virtual std::size_t size() const = 0;

    [[nodiscard]] virtual bool empty() const = 0;

    [[nodiscard]] virtual FilterResult apply(const Changefeed::ChangeEvent& event) const = 0;

    [[nodiscard]] virtual std::vector<Changefeed::ChangeEvent> applyBatch(
        const std::vector<Changefeed::ChangeEvent>& events) const = 0;

    [[nodiscard]] virtual std::vector<std::string> filterNames() const = 0;

    [[nodiscard]] virtual std::size_t totalPassed() const = 0;

    [[nodiscard]] virtual std::size_t totalDropped() const = 0;

    /**
     * @brief Reset Counters.
     */
    virtual void resetCounters() = 0;
};

// ── InMemoryFilterPipeline ────────────────────────────────────────────────────

class InMemoryFilterPipeline : public ICDCFilterPipeline {
public:
    InMemoryFilterPipeline() = default;

    bool addFilter(std::unique_ptr<IEventFilter> filter) override {
        if (!filter) {
          return false;
        }
        /**
         * @brief Lk.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
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
        /**
         * @brief Lk.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
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
        /**
         * @brief Lk.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::unique_lock<std::mutex> lk(mutex_);
        for (const auto& entry : stages_) {
            if (entry->name() == name) {
              return true;
            }
        }
        return false;
    }

    std::size_t size() const override {
        /**
         * @brief Lk.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::unique_lock<std::mutex> lk(mutex_);
        return stages_.size();
    }

    bool empty() const override {
        /**
         * @brief Lk.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::unique_lock<std::mutex> lk(mutex_);
        return stages_.empty();
    }

    FilterResult apply(const Changefeed::ChangeEvent& event) const override {
        std::vector<IEventFilter*> snapshot;
        {
            /**
             * @brief Lk.
             * @param[in] mutex_ Input parameter.
             * @return Return value.
             */
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
        /**
         * @brief Lk.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
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
