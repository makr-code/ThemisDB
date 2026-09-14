#pragma once

#include <memory>
#include <rocksdb/db.h>
#include <string>
#include <vector>

namespace themis::auth::detail {

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
