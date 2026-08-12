/**
 * @file crdt_importer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace themis {
namespace importers {
namespace crdt {

using json = nlohmann::json;

/**
 * @brief Conflict-Free Replicated Data Type state for distributed imports.
 *
 * Multiple importers can write concurrently to the same ThemisDB instance
 * without holding locks.  Conflicts are resolved deterministically using a
 * Last-Write-Wins (LWW) strategy ordered by (wall_clock_ns, lamport_clock,
 * replica_id lexicographic order).
 *
 * References:
 *   - Shapiro et al. (2016) "A Conflict-free Replicated Data Type for Databases"
 *   - Preguiça et al. (2018) "CRDTs: Consistency without concurrency control"
 */
class CRDTTableState {
public:
    struct CRDTRecord {
        std::string id;            ///< Globally unique record identifier
        json value;                ///< Record payload
        uint64_t lamport_clock{0}; ///< Logical timestamp
        std::string replica_id;    ///< Originating importer instance
        uint64_t wall_clock_ns{0}; ///< Physical timestamp (nanoseconds since epoch)

        /**
         * @brief Deterministic LWW merge: returns the record that should win.
         *
         * Ordering: wall_clock_ns DESC → lamport_clock DESC → replica_id ASC.
         */
        static CRDTRecord merge(const CRDTRecord& left, const CRDTRecord& right);

        /** @brief Serialise to JSON. */
        json toJson() const;
        /** @brief Deserialise from JSON. */
        static CRDTRecord fromJson(const json& j);
    };

    /**
     * @brief Import a batch of records with CRDT semantics.
     *
     * Each record is merged against any existing state for the same id using
     * CRDTRecord::merge().  Records with a higher logical clock win.
     *
     * @param table_name  Destination table.
     * @param records     Batch of JSON record payloads. Each must contain an
     *                    "id" field (string).
     * @param replica_id  Unique identifier of the calling importer instance.
     * @return Number of records written (merged or new).
     */
    size_t importWithCRDT(
        const std::string& table_name,
        const std::vector<json>& records,
        const std::string& replica_id
    );

    /**
     * @brief Retrieve the current winning state for a record.
     * @return The merged CRDTRecord, or std::nullopt if not found.
     */
    const CRDTRecord* lookup(const std::string& table_name,
                             const std::string& record_id) const;

    /**
     * @brief Increment and return the local Lamport clock.
     * Used internally; also callable by callers who need a monotonic counter.
     */
    uint64_t tickClock();

private:
    uint64_t lamport_clock_{0};
    // table_name → record_id → winning record
    std::map<std::string, std::map<std::string, CRDTRecord>> state_;
};

} // namespace crdt
} // namespace importers
} // namespace themis
