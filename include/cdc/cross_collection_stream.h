/**
 * @file cross_collection_stream.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct AggregatedEvent {
    std::string collection;      ///< Name of the collection that emitted this event
    Changefeed::ChangeEvent event; ///< The underlying change event

    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
};

class CrossCollectionStream {
public:
    struct StreamOptions {
        std::unordered_map<std::string, uint64_t> from_sequence;

        size_t limit{100};

        std::optional<std::string> key_prefix;

        std::set<Changefeed::ChangeEventType> event_types;

        std::set<std::string> collections;

        /**
         * @brief Defaults.
         * @return Return value.
         * @details Implements defaults without additional internal calls.
         */
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
     * @brief Add Collection.
     * @param[in] name Input parameter.
     * @param[in,out] feed Input/output parameter.
     */
    void addCollection(const std::string& name, Changefeed* feed);

    /**
     * @brief Remove Collection.
     * @param[in] name Input parameter.
     */
    void removeCollection(const std::string& name);

    /**
     * @brief Has Collection.
     * @param[in] name Input parameter.
     * @return True when the operation succeeds.
     */
    bool hasCollection(const std::string& name) const;

    /**
     * @brief Collection Count.
     * @return Return value.
     */
    size_t collectionCount() const;

    /**
     * @brief List Collections.
     * @return Return value.
     */
    std::vector<std::string> listCollections() const;

    // ----------------------------------------------------------------
    // Aggregated query
    // ----------------------------------------------------------------

    std::vector<AggregatedEvent> listEvents(
        const StreamOptions& options = StreamOptions::defaults()) const;

    std::vector<AggregatedEvent> listEventsFor(
        const std::set<std::string>& collection_names,
        size_t limit = 100) const;

    uint64_t getHighWatermark(
        const std::set<std::string>& collection_names = {}) const;

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, Changefeed*> feeds_;
};

} // namespace cdc
} // namespace themis
