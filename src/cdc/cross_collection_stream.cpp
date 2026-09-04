/**
 * @file cross_collection_stream.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "cdc/cross_collection_stream.h"
#include "cdc/cdc_error.h"
#include "utils/logger.h"

#include <algorithm>
#include <stdexcept>

namespace themis {
namespace cdc {

// ============================================================
// AggregatedEvent
// ============================================================

nlohmann::json AggregatedEvent::toJson() const {
    nlohmann::json j = event.toJson();
    j["collection"] = collection;
    return j;
}

// ============================================================
// Collection registry
// ============================================================

void CrossCollectionStream::addCollection(const std::string& name,
                                          Changefeed* feed) {
    if (name.empty()) {
        throw std::invalid_argument(
            "CrossCollectionStream::addCollection: name must not be empty");
    }
    if (!feed) {
        throw std::invalid_argument(
            "CrossCollectionStream::addCollection: feed must not be null");
    }
    std::lock_guard<std::mutex> lock(mutex_);
    feeds_[name] = feed;
    THEMIS_DEBUG("CrossCollectionStream: registered collection '{}'", name);
}

void CrossCollectionStream::removeCollection(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    feeds_.erase(name);
}

bool CrossCollectionStream::hasCollection(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return feeds_.count(name) > 0;
}

size_t CrossCollectionStream::collectionCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<int>(feeds_.size());
}

std::vector<std::string> CrossCollectionStream::listCollections() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> names = {};

    names.reserve(feeds_.size());
    for (const auto& kv : feeds_) {
        names.push_back(kv.first);
    }
    return names;
}

// ============================================================
// Aggregated query helpers
// ============================================================

namespace {

// Comparator: sort by (timestamp_ms ASC, collection ASC, sequence ASC).
// This gives a deterministic global ordering even when clocks are coarse.
bool aggregatedEventLess(const AggregatedEvent& a, const AggregatedEvent& b) {
    if (a.event.timestamp_ms != b.event.timestamp_ms) {
        return a.event.timestamp_ms < b.event.timestamp_ms;
    }
    if (a.collection != b.collection) {
        return a.collection < b.collection;
    }
    return a.event.sequence < b.event.sequence;
}

} // anonymous namespace

// ============================================================
// listEvents
// ============================================================

std::vector<AggregatedEvent> CrossCollectionStream::listEvents(
    const StreamOptions& options) const {

    // Take a snapshot of the feed map under the lock so we don't hold it
    // while querying individual changefeeds (which have their own locks).
    std::unordered_map<std::string, Changefeed*> feeds_snapshot;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        feeds_snapshot = feeds_;
    }

    const size_t effective_limit =
        (options.limit == 0) ? 100 : options.limit;

    std::vector<AggregatedEvent> all_events;

    for (const auto& [name, feed] : feeds_snapshot) {
        // Apply collection filter when specified
        if (!options.collections.empty() &&
            options.collections.count(name) == 0) {
            continue;
        }

        // Determine the resume cursor for this collection
        uint64_t from_seq = 0;
        auto cursor_it = options.from_sequence.find(name);
        if (cursor_it != options.from_sequence.end()) {
            from_seq = cursor_it->second;
        }

        // Fetch events from this feed.  We request up to `effective_limit`
        // events per collection; the final truncation to `effective_limit`
        // happens after merging all collections, so the merged vector may
        // temporarily be larger than the final result.
        Changefeed::ListOptions lo;
        lo.from_sequence = from_seq;
        lo.limit         = effective_limit; // Each feed contributes at most limit events
        if (options.key_prefix.has_value()) {
            lo.key_prefix = options.key_prefix;
        }
        lo.event_types = options.event_types;

        try {
            auto events = feed->listEvents(lo);
            for (auto& ev : events) {
                all_events.push_back({name, std::move(ev)});
            }
        } catch (const std::exception& ex) {
            THEMIS_WARN("CrossCollectionStream::listEvents: error fetching "
                        "events from collection '{}': {}", name, ex.what());
            // Continue with other collections; partial results are better than none.
        }
    }

    // Sort the merged event list
    std::sort(all_events.begin(), all_events.end(), aggregatedEventLess);

    // Truncate to the requested limit
    if (static_cast<int>(all_events.size()) > effective_limit) {
        all_events.resize(effective_limit);
    }

    return all_events;
}

// ============================================================
// listEventsFor
// ============================================================

std::vector<AggregatedEvent> CrossCollectionStream::listEventsFor(
    const std::set<std::string>& collection_names,
    size_t limit) const {
    StreamOptions opts;
    opts.collections = collection_names;
    opts.limit       = limit;
    return listEvents(opts);
}

// ============================================================
// getHighWatermark
// ============================================================

uint64_t CrossCollectionStream::getHighWatermark(
    const std::set<std::string>& collection_names) const {

    std::unordered_map<std::string, Changefeed*> feeds_snapshot;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        feeds_snapshot = feeds_;
    }

    uint64_t watermark = 0;
    for (const auto& [name, feed] : feeds_snapshot) {
        if (!collection_names.empty() && collection_names.count(name) == 0) {
            continue;
        }
        try {
            uint64_t seq = feed->getLatestSequence();
            if (seq > watermark) {
                watermark = seq;
            }
        } catch (const std::exception& ex) {
            THEMIS_WARN("CrossCollectionStream::getHighWatermark: error "
                        "querying collection '{}': {}", name, ex.what());
        }
    }
    return watermark;
}

} // namespace cdc
} // namespace themis
