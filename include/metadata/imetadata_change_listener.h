/**
 * @file imetadata_change_listener.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 88/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB — Metadata Change Listener Interface
 *
 * Observer interface for schema and metadata change events.  Allows
 * components (cache invalidators, external catalog exporters, audit hooks,
 * …) to react to table creation, modification, and removal without polling.
 *
 * Implementations ship in this header-only file:
 *   - IMetadataChangeListener    — abstract observer interface
 *   - RecordingMetadataChangeListener — in-memory recording implementation
 *
 * Design constraints:
 *  - onMetadataChanged() must return promptly; heavy work should be
 *    offloaded to a background thread inside the implementation.
 *  - Implementations must be thread-safe (dispatch may arrive from any
 *    background or I/O thread).
 *
 * Copyright (c) 2026 ThemisDB Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <chrono>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <nlohmann/json.hpp>

namespace themis {
namespace metadata {

using json = nlohmann::json;

// ── MetadataChangeType ────────────────────────────────────────────────────────

/**
 * @brief Classifies the kind of metadata change that occurred.
 */
enum class MetadataChangeType {
    TABLE_CREATED,      ///< A new table/collection was discovered or created
    TABLE_MODIFIED,     ///< An existing table schema was updated
    TABLE_DROPPED,      ///< A table was removed from the catalog
    CONSTRAINT_ADDED,   ///< A new schema constraint was defined
    CONSTRAINT_DROPPED, ///< A schema constraint was removed
    STATISTICS_UPDATED, ///< Statistics for a table were refreshed
};

// ── MetadataChangeEvent ───────────────────────────────────────────────────────

/**
 * @brief Describes a single metadata change event passed to listeners.
 */
struct MetadataChangeEvent {
    MetadataChangeType                    change_type; ///< What happened
    std::string                           table_name;  ///< Affected table/collection
    std::optional<std::string>            actor;       ///< Who triggered the change (nullopt = system)
    std::optional<std::string>            detail;      ///< Human-readable description
    std::chrono::system_clock::time_point timestamp;   ///< When the change occurred

    json toJSON() const {
        json j;
        j["change_type"] = static_cast<int>(change_type);
        j["table_name"]  = table_name;
        if (actor) {
          j["actor"]  = *actor;
        }
        if (detail) {
          j["detail"] = *detail;
        }
        j["timestamp_ms"] = std::chrono::duration_cast<std::chrono::milliseconds>(
            timestamp.time_since_epoch()).count();
        return j;
    }
};

// ── IMetadataChangeListener ───────────────────────────────────────────────────

/**
 * @brief Abstract observer interface for metadata change notifications.
 *
 * Implementations MUST be thread-safe, as onMetadataChanged() may be
 * invoked from any background thread.
 */
class IMetadataChangeListener {
public:
    virtual ~IMetadataChangeListener() = default;

    /**
     * @brief Called when a metadata change event occurs.
     *
     * @param event  Details of the change.
     *
     * @note Implementations should return quickly to avoid blocking the
     *       dispatcher.  Heavy processing should be queued asynchronously.
     */
    virtual void onMetadataChanged(const MetadataChangeEvent& event) = 0;
};

// ── RecordingMetadataChangeListener ──────────────────────────────────────────

/**
 * @brief Thread-safe in-memory listener that records all received events.
 *
 * Primarily intended for unit tests and integration tests.
 * An optional callback is invoked synchronously (under the mutex) after
 * each event is recorded.
 *
 * Example:
 * @code
 *   RecordingMetadataChangeListener rec;
 *   // ... wire rec into SchemaManager / CatalogExporter ...
 *   EXPECT_EQ(rec.eventCount(), 1);
 *   EXPECT_EQ(rec.lastEvent()->change_type, MetadataChangeType::TABLE_CREATED);
 * @endcode
 */
class RecordingMetadataChangeListener : public IMetadataChangeListener {
public:
    using EventCallback = std::function<void(const MetadataChangeEvent&)>;

    explicit RecordingMetadataChangeListener(EventCallback cb = {})
        : callback_(std::move(cb)) {}

    // ── IMetadataChangeListener ───────────────────────────────────────────────

    void onMetadataChanged(const MetadataChangeEvent& event) override {
        std::unique_lock<std::mutex> lk(mutex_);
        events_.push_back(event);
        auto cb = callback_;          // copy under lock
        lk.unlock();
        if (cb) {
          cb(event);
        }
    }

    // ── Inspection helpers ────────────────────────────────────────────────────

    /**
     * @brief Return a snapshot of all recorded events, oldest first.
     */
    std::vector<MetadataChangeEvent> events() const {
        std::unique_lock<std::mutex> lk(mutex_);
        return events_;
    }

    /**
     * @brief Return the number of events recorded so far.
     */
    std::size_t eventCount() const {
        std::unique_lock<std::mutex> lk(mutex_);
        return events_.size();
    }

    /**
     * @brief Return the most recently recorded event, or nullopt if none.
     */
    std::optional<MetadataChangeEvent> lastEvent() const {
        std::unique_lock<std::mutex> lk(mutex_);
        if (events_.empty()) {
          return std::nullopt;
        }
        return events_.back();
    }

    /**
     * @brief Clear all recorded events.
     */
    void clear() {
        std::unique_lock<std::mutex> lk(mutex_);
        events_.clear();
    }

    /**
     * @brief Replace the event callback.
     *
     * Thread-safe; the new callback takes effect for the next event.
     */
    void setCallback(EventCallback cb) {
        std::unique_lock<std::mutex> lk(mutex_);
        callback_ = std::move(cb);
    }

private:
    mutable std::mutex               mutex_;
    std::vector<MetadataChangeEvent> events_;
    EventCallback                    callback_;
};

} // namespace metadata
} // namespace themis

