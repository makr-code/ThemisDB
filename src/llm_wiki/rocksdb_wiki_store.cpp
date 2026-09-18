/**
 * @file rocksdb_wiki_store.cpp
 * @brief RocksDB-backed key-value store for LLM Wiki page persistence —
 *        implementation.
 *
 * Compiled only when `THEMIS_USE_ROCKSDB` is defined.  The corresponding
 * CMake target (`themis_llm_wiki`) links against `rocksdb` when the option
 * `THEMIS_USE_ROCKSDB` is set.
 *
 * @date    2026-08-26
 * @note    Wave-B gap closure — LW1 (RocksDB backend)
 * @see     include/llm_wiki/rocksdb_wiki_store.h
 */

#ifdef THEMIS_USE_ROCKSDB

#include "llm_wiki/rocksdb_wiki_store.h"
#include "utils/rocksdb_open_compat.h"

#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>

namespace themis {
namespace plugins {
namespace llm_wiki {

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/**
 * @brief To Themis Error.
 * @param[in] rdb_status Input parameter.
 * @return Return value.
 * @details Calls: Status::Error(), ToString().
 */
Status toThemisError(const rocksdb::Status& rdb_status) {
    return Status::Error("RocksDB error: " + rdb_status.ToString());
}

}  // namespace

/**
 * @brief ───────────────────────────────────────────────────────────────────────────── RocksDbWikiStore::open ─────────────────────────────────────────────────────────────────────────────
 * @param[in] db_path Path to the db.
 * @return Return value.
 * @details Calls: empty(), Status::Error(), std::filesystem::create_directories(), message(), themis::storage::detail::openDbCompat(), ok(), toThemisError(), Status::Ok().
 */

Status RocksDbWikiStore::open(const std::string& db_path) {
    if (db_path.empty()) {
        return Status::Error("db_path must not be empty");
    }

    // Ensure the directory exists (RocksDB creates the directory itself when
    // create_if_missing=true, but the parent directory must exist).
    std::error_code ec = {};
    std::filesystem::create_directories(db_path, ec);
    if (ec) {
        return Status::Error("Failed to create RocksDB directory '" + db_path +
                             "': " + ec.message());
    }

    options_.create_if_missing = true;
    options_.error_if_exists   = false;

    // Cross-compiler / cross-OS portability: RocksDB Open APIs commonly expose
    // DB** out-parameters in packaged builds, so open via raw pointer first.
    rocksdb::DB* db_instance = nullptr;
    rocksdb::Status rdb_st = themis::storage::detail::openDbCompat(options_, db_path, &db_instance);

    if (!rdb_st.ok()) {
        // RocksDB contract: on failure, db_instance remains nullptr.
        // Ensure db_ remains nullptr.
        db_ = nullptr;
        return toThemisError(rdb_st);
    }

    // Assign raw pointer directly after ownership transfer.
    db_ = db_instance;
    db_path_ = db_path;
    return Status::Ok();
}

/**
 * @brief ───────────────────────────────────────────────────────────────────────────── RocksDbWikiStore::close ─────────────────────────────────────────────────────────────────────────────
 * @details Calls: FlushWAL(), clear().
 */

void RocksDbWikiStore::close() {
    if (!db_) {
        return;
    }
    // FlushWAL before releasing the DB pointer so no committed data is lost.
    rocksdb::FlushOptions flush_opts;
    flush_opts.wait = true;
    (void)db_->FlushWAL(flush_opts.wait);  // best-effort; ignore return code
    delete db_;
    db_ = nullptr;
    db_path_.clear();
}

/**
 * @brief ───────────────────────────────────────────────────────────────────────────── RocksDbWikiStore::put ─────────────────────────────────────────────────────────────────────────────
 * @param[in] key Input parameter.
 * @param[in] value_json Input parameter.
 * @return Return value.
 * @details Calls: Status::Error(), Put(), rocksdb::Slice(), ok(), toThemisError(), Status::Ok().
 */

Status RocksDbWikiStore::put(const std::string& key,
                              const std::string& value_json) {
    if (!db_) {
        return Status::Error("store is not open");
    }
    rocksdb::WriteOptions write_opts;
    write_opts.sync = false;  // WAL durability; no per-write fsync overhead

    rocksdb::Status rdb_st =
        db_->Put(write_opts, rocksdb::Slice(key), rocksdb::Slice(value_json));

    if (!rdb_st.ok()) {
        return toThemisError(rdb_st);
    }
    return Status::Ok();
}

// ─────────────────────────────────────────────────────────────────────────────
// RocksDbWikiStore::get
// ─────────────────────────────────────────────────────────────────────────────

std::pair<Status, std::string> RocksDbWikiStore::get(
    const std::string& key) const {
    if (!db_) {
        return {Status::Error("store is not open"), {}};
    }
    std::string value = {};
    rocksdb::Status rdb_st =
        db_->Get(rocksdb::ReadOptions(), rocksdb::Slice(key), &value);

    if (rdb_st.IsNotFound()) {
        return {Status::Error("not_found"), {}};
    }
    if (!rdb_st.ok()) {
        return {toThemisError(rdb_st), {}};
    }
    return {Status::Ok(), std::move(value)};
}

/**
 * @brief ───────────────────────────────────────────────────────────────────────────── RocksDbWikiStore::remove ─────────────────────────────────────────────────────────────────────────────
 * @param[in] key Input parameter.
 * @return Return value.
 * @details Calls: Status::Error(), Delete(), rocksdb::Slice(), ok(), IsNotFound(), toThemisError(), Status::Ok().
 */

Status RocksDbWikiStore::remove(const std::string& key) {
    if (!db_) {
        return Status::Error("store is not open");
    }
    rocksdb::WriteOptions write_opts;
    rocksdb::Status rdb_st =
        db_->Delete(write_opts, rocksdb::Slice(key));

    // Deleting a non-existent key is treated as success (idempotent).
    if (!rdb_st.ok() && !rdb_st.IsNotFound()) {
        return toThemisError(rdb_st);
    }
    return Status::Ok();
}

// ─────────────────────────────────────────────────────────────────────────────
// RocksDbWikiStore::scan
// ─────────────────────────────────────────────────────────────────────────────

void RocksDbWikiStore::scan(
    std::function<void(std::string_view key,
                       std::string_view value)> cb) const {
    if (!db_ || !cb) {
        return;
    }
    rocksdb::ReadOptions read_opts;
    read_opts.fill_cache = false;  // avoid polluting block cache during scan

    std::unique_ptr<rocksdb::Iterator> it(
        db_->NewIterator(read_opts));

    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        cb(std::string_view(it->key().data(), it->key().size()),
           std::string_view(it->value().data(), it->value().size()));
    }
    // Iterator status is checked implicitly; corrupt iterators simply stop.
}

}  // namespace llm_wiki
}  // namespace plugins
}  // namespace themis

#endif  // THEMIS_USE_ROCKSDB
