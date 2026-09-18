/**
 * @file crdt_importer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class CRDTTableState {
public:
    struct CRDTRecord {
        std::string id;            ///< Globally unique record identifier
        json value;                ///< Record payload
        uint64_t lamport_clock{0}; ///< Logical timestamp
        std::string replica_id;    ///< Originating importer instance
        uint64_t wall_clock_ns{0}; ///< Physical timestamp (nanoseconds since epoch)

        /**
         * @brief Merge.
         * @param[in] left Input parameter.
         * @param[in] right Input parameter.
         * @return Return value.
         */
        static CRDTRecord merge(const CRDTRecord& left, const CRDTRecord& right);

        /**
         * @brief To Json.
         * @return Return value.
         */
        json toJson() const;
        /**
         * @brief From Json.
         * @param[in] j Input parameter.
         * @return Return value.
         */
        static CRDTRecord fromJson(const json& j);
    };

    /**
     * @brief Import With CRDT.
     * @param[in] table_name Name of the table.
     * @param[in] records Input parameter.
     * @param[in] replica_id Identifier of the replica.
     * @return Return value.
     */
    size_t importWithCRDT(
        const std::string& table_name,
        const std::vector<json>& records,
        const std::string& replica_id
    );

    /**
     * @brief Lookup.
     * @param[in] table_name Name of the table.
     * @param[in] record_id Identifier of the record.
     * @return Pointer to the result.
     */
    const CRDTRecord* lookup(const std::string& table_name,
                             const std::string& record_id) const;

    /**
     * @brief Tick Clock.
     * @return Return value.
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
