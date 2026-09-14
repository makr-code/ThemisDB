#pragma once

#include <memory>
#include <rocksdb/db.h>
#include <string>
#include <vector>

namespace themis::auth::detail {

/**
 * @brief Open a RocksDB database with column families in a version-compatible way.
 *
 * Dispatches to the unique_ptr-based `DBType::Open` overload when available
 * (newer RocksDB releases), or falls back to the raw-pointer overload for older
 * versions.  On success the caller **owns** the raw pointer stored in `*out_db`
 * and must delete it (or wrap it in a `std::unique_ptr`) when done.
 *
 * @tparam DBType  Database type (default: `rocksdb::DB`).
 * @param options     DBOptions passed to `DBType::Open`.
 * @param db_path     Path to the database directory.
 * @param cf_descs    Column-family descriptors (name + options) to open.
 * @param cf_handles  [out] Populated with one handle per entry in `cf_descs`.
 *                    Caller owns the resulting handles and must release them
 *                    before closing the database.
 * @param out_db      [out] Receives the opened database pointer.  Non-null on
 *                    success; unchanged on failure.  Caller owns the object.
 * @return `rocksdb::Status::OK()` on success; an error status otherwise.
 */
template <typename DBType = rocksdb::DB>
rocksdb::Status openDbWithColumnFamiliesCompat(
    const rocksdb::DBOptions& options,
    const std::string& db_path,
    const std::vector<rocksdb::ColumnFamilyDescriptor>& cf_descs,
    std::vector<rocksdb::ColumnFamilyHandle*>* cf_handles,
    DBType** out_db) {
    if constexpr (requires(std::unique_ptr<DBType>* db_uptr) {
                      DBType::Open(options, db_path, cf_descs, cf_handles, db_uptr);
                  }) {
        std::unique_ptr<DBType> db_uptr;
        rocksdb::Status status = DBType::Open(options, db_path, cf_descs, cf_handles, &db_uptr);
        if (status.ok()) {
            *out_db = db_uptr.release();
        }
        return status;
    } else {
        return DBType::Open(options, db_path, cf_descs, cf_handles, out_db);
    }
}

} // namespace themis::auth::detail
