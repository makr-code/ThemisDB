#pragma once

#include <memory>
#include <rocksdb/db.h>
#include <string>
#include <vector>

namespace themis::storage::detail {

template <typename DBType = rocksdb::DB>
/**
 * @brief Open Db Compat.
 * @param[in] options Input parameter.
 * @param[in] db_path Path to the db.
 * @param[in,out] out_db Input/output parameter.
 * @return Return value.
 * @details Calls: constexpr(), requires(), DBType::Open(), ok(), release().
 */
rocksdb::Status openDbCompat(
    const rocksdb::Options& options,
    const std::string& db_path,
    DBType** out_db) {
    if constexpr (requires(std::unique_ptr<DBType>* db_uptr) {
                      DBType::Open(options, db_path, db_uptr);
                  }) {
        std::unique_ptr<DBType> db_uptr;
        rocksdb::Status status = DBType::Open(options, db_path, &db_uptr);
        if (status.ok()) {
            *out_db = db_uptr.release();
        }
        return status;
    } else {
        return DBType::Open(options, db_path, out_db);
    }
}

template <typename DBType = rocksdb::DB>
/**
 * @brief Open Db With Column Families Compat.
 * @param[in] options Input parameter.
 * @param[in] db_path Path to the db.
 * @param[in] cf_descs Input parameter.
 * @param[in,out] cf_handles Input/output parameter.
 * @param[in,out] out_db Input/output parameter.
 * @return Return value.
 * @details Calls: constexpr(), requires(), DBType::Open(), ok(), release().
 */
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

template <typename DBType = rocksdb::DB>
rocksdb::Status openDbForReadOnlyCompat(
    const rocksdb::Options& options,
    const std::string& db_path,
    DBType** out_db,
    bool error_if_wal_file_exists = false) {
    if constexpr (requires(std::unique_ptr<DBType>* db_uptr) {
                      DBType::OpenForReadOnly(options, db_path, db_uptr, error_if_wal_file_exists);
                  }) {
        std::unique_ptr<DBType> db_uptr;
        rocksdb::Status status = DBType::OpenForReadOnly(options, db_path, &db_uptr, error_if_wal_file_exists);
        if (status.ok()) {
            *out_db = db_uptr.release();
        }
        return status;
    } else {
        return DBType::OpenForReadOnly(options, db_path, out_db, error_if_wal_file_exists);
    }
}

} // namespace themis::storage::detail
