/**
 * @file cross_collection_stream.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "cdc/changefeed.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace cdc {

/**
 * @brief A single change event enriched with its source collection name.
 */
struct AggregatedEvent {
    std::string collection;      ///< Name of the collection that emitted this event
    Changefeed::ChangeEvent event; ///< The underlying change event

    /**
     * @brief Serialize to JSON, adding a top-level "collection" field.
     */
    nlohmann::json toJson() const;
};

/**
 * @brief Cross-collection change aggregation stream.
 *
 * Manages a set of named Changefeed references and provides a unified view of
 * their change events.  Events from all registered collections are merged and
 * sorted by (timestamp_ms, sequence) so that the caller receives a
 * globally-ordered stream regardless of which collection each event originated
 * from.
 *
 * ### Usage
 * ```cpp
 * CrossCollectionStream stream;
 * stream.addCollection("orders",    orders_feed.get());
 * stream.addCollection("inventory", inventory_feed.get());
 *
 * CrossCollectionStream::StreamOptions opts;
 * opts.limit = 200;
 * opts.event_types = { Changefeed::ChangeEventType::EVENT_PUT };
 *
 * auto events = stream.listEvents(opts);
 * ```
 *
 * ### Thread-safety
 * All public methods are thread-safe.  `addCollection` / `removeCollection`
 * take an exclusive lock; `listEvents` and the other const query helpers take
 * a snapshot of the feed map under a lock and then query individual feeds
 * without holding the lock.
 *
 * ### Ownership
 * The `CrossCollectionStream` does **not** own the `Changefeed` pointers.
 * Callers must ensure the referenced feeds outlive the stream.
 */
class CrossCollectionStream {
public:
    /**
     * @brief Options controlling a `listEvents` call.
     */
    struct StreamOptions {
        /**
         * Per-collection resume cursors.  For each entry the stream will only
         * return events with sequence > from_sequence[collection].  Collections
         * absent from the map default to 0 (return all events).
         */
        std::unordered_map<std::string, uint64_t> from_sequence;

        /** Maximum total events to return across all collections. */
        size_t limit{100};

        /**
         * If set, only events whose key starts with this prefix are included.
         * Applied after collection filtering.
         */
        std::optional<std::string> key_prefix;

        /**
         * Restrict results to these event types.  Empty set means no filter
         * (all event types are returned).
         */
        std::set<Changefeed::ChangeEventType> event_types;

        /**
         * Restrict results to these collection names.  Empty set means no
         * filter (all registered collections are queried).
         */
        std::set<std::string> collections;

        static StreamOptions defaults() { return {}; }
    };

    CrossCollectionStream() = default;
    ~CrossCollectionStream() = default;

    // Non-copyable; streams own internal state tied to live Changefeed pointers.
    CrossCollectionStream(const CrossCollectionStream&) = delete;
    CrossCollectionStream& operator=(const CrossCollectionStream&) = delete;

    CrossCollectionStream(CrossCollectionStream&&) = delete;
    CrossCollectionStream& operator=(CrossCollectionStream&&) = delete;

    // ----------------------------------------------------------------
    // Collection registry
    // ----------------------------------------------------------------

    /**
     * @brief Register a named collection feed.
     *
     * If a feed with the same name is already registered it is replaced.
     *
     * @param name  Collection name used in AggregatedEvent::collection.
     * @param feed  Non-null pointer to the Changefeed (not owned).
     * @throws std::invalid_argument if @p name is empty or @p feed is null.
     */
    void addCollection(const std::string& name, Changefeed* feed);

    /**
     * @brief Unregister a collection.  No-op if @p name is not registered.
     */
    void removeCollection(const std::string& name);

    /**
     * @brief Return true if a collection with @p name is registered.
     */
    bool hasCollection(const std::string& name) const;

    /**
     * @brief Return the number of registered collections.
     */
    size_t collectionCount() const;

    /**
     * @brief Return the names of all registered collections (unordered).
     */
    std::vector<std::string> listCollections() const;

    // ----------------------------------------------------------------
    // Aggregated query
    // ----------------------------------------------------------------

    /**
     * @brief Fetch and merge change events from all registered collections.
     *
     * Queries each collection (subject to `options.collections` filter),
     * applies `options.key_prefix` and `options.event_types` filters, merges
     * the results, sorts them by (timestamp_ms ASC, collection ASC, sequence
     * ASC) and returns the first `options.limit` entries.
     *
     * @param options  Query options (cursors, filters, limit).
     * @return Merged, ordered list of aggregated change events.
     */
    std::vector<AggregatedEvent> listEvents(
        const StreamOptions& options = StreamOptions::defaults()) const;

    /**
     * @brief Convenience overload: list events across a collection subset.
     *
     * Equivalent to creating a StreamOptions with `collections = collection_names`
     * and calling `listEvents(options)`.
     *
     * @param collection_names  Collections to include; empty = all registered.
     * @param limit             Max events to return.
     * @return Merged, ordered list of aggregated change events.
     */
    std::vector<AggregatedEvent> listEventsFor(
        const std::set<std::string>& collection_names,
        size_t limit = 100) const;

    /**
     * @brief Return the highest sequence number seen across all (or the
     *        specified) collections.
     *
     * Returns 0 if no collections are registered or all feeds are empty.
     *
     * @param collection_names  Collections to inspect; empty = all registered.
     */
    uint64_t getHighWatermark(
        const std::set<std::string>& collection_names = {}) const;

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, Changefeed*> feeds_;
};

} // namespace cdc
} // namespace themis
