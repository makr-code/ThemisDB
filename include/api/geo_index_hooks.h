/**
 * @file geo_index_hooks.h
 * @brief Integration hooks for geospatial index operations.
 *
 * @details Provides callback interfaces for geospatial query optimization, index
 * creation/deletion, and spatial predicate evaluation hooks.
 *
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Status: Production Ready
 */


#pragma once

#include "storage/rocksdb_wrapper.h"
#include <string>
#include <vector>
#include <memory>

namespace themis {
namespace index {
class SpatialIndexManager;
}

namespace api {

class GeoIndexHooks {
public:
    /**
     * @brief On Entity Put.
     * @param[in,out] db Input/output parameter.
     * @param[in,out] spatial_mgr Input/output parameter.
     * @param[in] table Input parameter.
     * @param[in] pk Input parameter.
     * @param[in] blob Input parameter.
     */
    static void onEntityPut(
        RocksDBWrapper& db,
        index::SpatialIndexManager* spatial_mgr,
        const std::string& table,
        const std::string& pk,
        const std::vector<uint8_t>& blob
    );

    /**
     * @brief On Entity Put Atomic.
     * @param[in,out] batch Input/output parameter.
     * @param[in,out] spatial_mgr Input/output parameter.
     * @param[in] table Input parameter.
     * @param[in] pk Input parameter.
     * @param[in] blob Input parameter.
     * @return True when the operation succeeds.
     */
    static bool onEntityPutAtomic(
        RocksDBWrapper::WriteBatchWrapper& batch,
        index::SpatialIndexManager* spatial_mgr,
        const std::string& table,
        const std::string& pk,
        const std::vector<uint8_t>& blob
    );

    /**
     * @brief On Entity Delete Atomic.
     * @param[in,out] batch Input/output parameter.
     * @param[in,out] spatial_mgr Input/output parameter.
     * @param[in] table Input parameter.
     * @param[in] pk Input parameter.
     * @param[in] old_blob Input parameter.
     * @return True when the operation succeeds.
     */
    static bool onEntityDeleteAtomic(
        RocksDBWrapper::WriteBatchWrapper& batch,
        index::SpatialIndexManager* spatial_mgr,
        const std::string& table,
        const std::string& pk,
        const std::vector<uint8_t>& old_blob
    );

    /**
     * @brief On Entity Delete.
     * @param[in,out] db Input/output parameter.
     * @param[in,out] spatial_mgr Input/output parameter.
     * @param[in] table Input parameter.
     * @param[in] pk Input parameter.
     * @param[in] old_blob Input parameter.
     */
    static void onEntityDelete(
        RocksDBWrapper& db,
        index::SpatialIndexManager* spatial_mgr,
        const std::string& table,
        const std::string& pk,
        const std::vector<uint8_t>& old_blob
    );
};

} // namespace api
} // namespace themis
