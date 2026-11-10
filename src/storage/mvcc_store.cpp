/**
 * @file mvcc_store.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "storage/mvcc_store.h"
#include "storage/mvcc_snapshot_pins.h"
#include <algorithm>
#include <stdexcept>

namespace themis {

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

MVCCStore::MVCCStore(
    std::shared_ptr<RocksDBWrapper> db,
    std::shared_ptr<HybridLogicalClock> clock
)
    : db_(std::move(db))
    , clock_(clock ? std::move(clock) : std::make_shared<HybridLogicalClock>())
{
    // uncaught_exception scanner alert (line 31): throws std::invalid_argument when
    // db is null; callers must supply a non-null database instance — intentional API
    // contract enforcement — false positive.
    if (!db_) {
        throw std::invalid_argument("MVCCStore: db cannot be null");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Key encoding helpers
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Encode Versioned Key.
 * @param[in] base_key Input parameter.
 * @param[in] ts Input parameter.
 * @return Return value.
 * @details Calls: reserve(), size(), append(), data(), push_back(), encodeToString().
 */
std::string MVCCStore::encodeVersionedKey(std::string_view base_key, HLCTimestamp ts) {
    std::string key = {};
    key.reserve(base_key.size() + 1 + 8);
    key.append(base_key.data(),base_key.size());
    key.push_back('\x00');
    key.append(ts.encodeToString());
    return key;
}

/**
 * @brief Encode Version Prefix.
 * @param[in] base_key Input parameter.
 * @return Return value.
 * @details Calls: reserve(), size(), append(), data(), push_back().
 */
std::string MVCCStore::encodeVersionPrefix(std::string_view base_key) {
    std::string prefix = {};
    prefix.reserve(base_key.size() + 1);
    prefix.append(base_key.data(),base_key.size());
    prefix.push_back('\x00');
    return prefix;
}

/**
 * @brief Decode Timestamp.
 * @param[in] versioned_key Input parameter.
 * @return Return value.
 * @details Calls: size(), data(), HLCTimestamp::decodeFromBytes().
 */
HLCTimestamp MVCCStore::decodeTimestamp(std::string_view versioned_key) {
    // The timestamp is always the last 8 bytes of the versioned key.
    // The separator '\x00' is at position (size - 9); the timestamp follows it.
    // We use a fixed offset from the end (not rfind) because the 8-byte
    // big-endian timestamp may itself contain '\x00' bytes.
    if (versioned_key.size() < 9) {
        return HLCTimestamp{};  // not a valid versioned key
    }
    const auto* ts_bytes = reinterpret_cast<const uint8_t*>(
        versioned_key.data() + versioned_key.size() - 8
    );
    return HLCTimestamp::decodeFromBytes(ts_bytes);
}

// ─────────────────────────────────────────────────────────────────────────────
// Write
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Put.
 * @param[in] key Input parameter.
 * @param[in] value Input parameter.
 * @return Return value.
 * @details Calls: now(), putWithTimestamp().
 */
HLCTimestamp MVCCStore::put(std::string_view key, const std::vector<uint8_t>& value) {
    HLCTimestamp ts = clock_->now();
    putWithTimestamp(key, value, ts);
    return ts;
}

/**
 * @brief Put With Timestamp.
 * @param[in] key Input parameter.
 * @param[in] value Input parameter.
 * @param[in] ts Input parameter.
 * @details Calls: update(), encodeVersionedKey(), put(), lk(), find(), std::string(), end().
 */
void MVCCStore::putWithTimestamp(
    std::string_view key,
    const std::vector<uint8_t>& value,
    HLCTimestamp ts
) {
    // Advance the local clock to be strictly greater than the received timestamp.
    clock_->update(ts);
    std::string vkey = encodeVersionedKey(key, ts);
    db_->put(vkey, value);

    // F-010: Update the latest-version cache so getLatest() can use a direct
    // point-read.  Only update if ts is >= the currently cached timestamp so
    // that concurrent writes don't regress the cache to an older version.
    {
        std::unique_lock<std::shared_mutex> lk(latest_mu_);
        auto it = latest_ts_map_.find(std::string(key));
        // data_race scanner alert (×2): both the map lookup and the map write
        // are performed inside this unique_lock scope on latest_mu_; all other
        // accessors of latest_ts_map_ likewise acquire latest_mu_ before access.
        // The scanner false-positively flags the code inside the lock body.
        if (it == latest_ts_map_.end() || ts.value >= it->second.value) {
            latest_ts_map_[std::string(key)] = ts;
        }
    }
}

/**
 * @brief Put In Txn.
 * @param[in,out] txn Input/output parameter.
 * @param[in] key Input parameter.
 * @param[in] value Input parameter.
 * @return Return value.
 * @details Calls: now(), encodeVersionedKey(), put().
 */
HLCTimestamp MVCCStore::putInTxn(
    RocksDBWrapper::TransactionWrapper& txn,
    std::string_view key,
    const std::vector<uint8_t>& value
) {
    HLCTimestamp ts = clock_->now();
    std::string vkey = encodeVersionedKey(key, ts);
    txn.put(vkey, value);
    return ts;
}

/**
 * @brief Del In Txn.
 * @param[in,out] txn Input/output parameter.
 * @param[in] key Input parameter.
 * @return Return value.
 * @details Calls: now(), encodeVersionedKey(), put().
 */
HLCTimestamp MVCCStore::delInTxn(
    RocksDBWrapper::TransactionWrapper& txn,
    std::string_view key
) {
    HLCTimestamp ts = clock_->now();
    // Write a tombstone: a versioned key with empty value signals deletion.
    std::string vkey = encodeVersionedKey(key, ts);
    txn.put(vkey, std::vector<uint8_t>{});
    return ts;
}

// ─────────────────────────────────────────────────────────────────────────────
// Read
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Get Latest.
 * @param[in] key Input parameter.
 * @return Return value.
 * @details Calls: lk(), find(), std::string(), end(), encodeVersionedKey(), unlock(), get(), empty().
 */
std::optional<std::vector<uint8_t>> MVCCStore::getLatest(std::string_view key) {
    // F-010: fast path — if we have a cached latest timestamp for this key,
    // perform a direct db_->get() point-read instead of creating an iterator.
    // This is valid for keys written via put()/putWithTimestamp(); keys written
    // exclusively through transactions fall through to the iterator path.
    {
        std::shared_lock<std::shared_mutex> lk(latest_mu_);
        auto it = latest_ts_map_.find(std::string(key));
        if (it != latest_ts_map_.end()) {
            std::string vkey = encodeVersionedKey(key, it->second);
            lk.unlock();  // Release before RocksDB I/O; concurrent writers may proceed.
            auto val = db_->get(vkey);
            if (val) {
                // Empty value signals a tombstone (deleted key).
                if (val->empty()) {
                  return std::nullopt;
                }
                return val;
            }
            // Key not found in RocksDB (e.g. compacted away) — fall through
            // to the slow iterator path which will also return nullopt cleanly.
        }
    }
    // Slow path: iterator-based seek for time-travel reads or cache misses.
    return getAtTimestamp(key, HLCTimestamp(UINT64_MAX));
}

/**
 * @brief Get At Timestamp.
 * @param[in] key Input parameter.
 * @param[in] ts Input parameter.
 * @return Return value.
 * @details Calls: encodeVersionPrefix(), std::string(), data(), size(), push_back(), encodeVersionedKey(), HLCTimestamp(), newSafeIterator().
 */
std::optional<std::vector<uint8_t>> MVCCStore::getAtTimestamp(
    std::string_view key,
    HLCTimestamp ts
) {
    std::string prefix = encodeVersionPrefix(key);

    // Build the exclusive seek target that is strictly greater than every
    // versioned key for `key` with timestamp <= ts.
    //
    // For ts < UINT64_MAX:
    //   seek_key = encodeVersionedKey(key, ts + 1)   [first key AFTER ts]
    //
    // For ts == UINT64_MAX:
    //   seek_key = base_key + '\x01'   [which sorts after all base_key + '\x00' + ... entries]
    //
    // We then step backward one entry; if it still has `prefix`, that is
    // the most-recent version committed at or before ts.
    std::string seek_key = {};
    if (ts.value == UINT64_MAX) {
        // Append '\x01' (> '\x00') to step past all versions of this base key.
        seek_key = std::string(key.data(),key.size());
        seek_key.push_back('\x01');
    } else {
        seek_key = encodeVersionedKey(key, HLCTimestamp(ts.value + 1));
    }

    auto iter_result = db_->newSafeIterator();
    if (!iter_result.has_value()) {
        return std::nullopt;
    }
    auto& it = iter_result.value();
    if (!it) {
        return std::nullopt;
    }

    it.Seek(seek_key);

    // Step back to find the entry just before seek_key.
    if (!it.Valid()) {
        // seek_key is past the end of the DB – step back from the last entry.
        it.SeekToLast();
    } else {
        it.Prev();
    }

    if (!it.Valid()) {
        return std::nullopt;
    }

    std::string_view found_key = it.key();

    // Verify the found key belongs to the same base key (shares the prefix).
    if (found_key.size() <prefix.size() ||
        found_key.substr(0,prefix.size()) != std::string_view(prefix)) {
        return std::nullopt;
    }

    // Decode the timestamp and verify it is within the requested range.
    HLCTimestamp found_ts = decodeTimestamp(found_key);
    if (found_ts > ts) {
        return std::nullopt;
    }

    std::string_view raw_val = it.value();
    // null_dereference scanner alerts (lines 225/226, 244/245): raw_val is a
    // std::string_view into a RocksDB iterator value; it.value() is non-null when the
    // iterator is valid (validated above); reinterpret_cast to const uint8_t* on a
    // valid string_view data() pointer is safe — false positives.
    return std::vector<uint8_t>(
        reinterpret_cast<const uint8_t*>(raw_val.data()),
        reinterpret_cast<const uint8_t*>(raw_val.data()) + raw_val.size() 
    );
}

// ─────────────────────────────────────────────────────────────────────────────
// Version scan
// ─────────────────────────────────────────────────────────────────────────────

void MVCCStore::scanVersions(
    std::string_view key,
    std::function<bool(const VersionEntry&)> callback
) {
    std::string prefix = encodeVersionPrefix(key);

    db_->scanPrefix(prefix, [&](std::string_view vkey, std::string_view raw_val) -> bool {
        VersionEntry entry;
        entry.timestamp = decodeTimestamp(vkey);
        entry.value.assign(
            reinterpret_cast<const uint8_t*>(raw_val.data()),
            reinterpret_cast<const uint8_t*>(raw_val.data()) + raw_val.size() 
        );
        return callback(entry);
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// Garbage collection
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Gc Versions Before.
 * @param[in] key Input parameter.
 * @param[in] min_ts Input parameter.
 * @param[in] opts Input parameter.
 * @return Return value.
 * @details Calls: encodeVersionPrefix(), scanPrefix(), emplace_back(), empty(), size(), decodeTimestamp(), std::min(), del().
 */
uint64_t MVCCStore::gcVersionsBefore(
    std::string_view key,
    HLCTimestamp min_ts,
    GCOptions opts
) {
    // Snapshot-pin interlock: hold the registry's GC guard while selecting
    // and deleting so that a concurrently acquired pin cannot be bypassed.
    auto pins = snapshotPinRegistry();
    std::shared_lock<std::shared_mutex> pin_guard;
    std::vector<HLCTimestamp> pinned;
    if (pins) {
        pin_guard = pins->gcGuard();
        pinned = pins->pinnedTimestamps();
    }

    std::string prefix = encodeVersionPrefix(key);

    // Collect all versioned keys for this base key (ascending timestamp
    // order thanks to the big-endian encoding).
    std::vector<std::string> all_keys;
    std::vector<HLCTimestamp> all_ts;
    db_->scanPrefix(prefix, [&](std::string_view vkey, std::string_view) -> bool {
        all_keys.emplace_back(vkey);
        all_ts.push_back(decodeTimestamp(vkey));
        return true;
    });

    if (all_keys.empty()) {
        return 0;
    }

    const auto victims = selectGcVictims(all_ts, min_ts, opts.min_versions_to_keep, pinned);
    uint64_t deleted = 0;
    for (size_t idx : victims) {
        db_->del(all_keys[idx]);
        ++deleted;
    }
    return deleted;
}

std::vector<size_t> MVCCStore::selectGcVictims(
    const std::vector<HLCTimestamp>& ascending_ts,
    HLCTimestamp min_ts,
    uint32_t min_keep,
    const std::vector<HLCTimestamp>& pinned
) {
    const size_t total = ascending_ts.size();
    const size_t keep = min_keep > 0 ? min_keep : 1;
    const size_t max_index = total > keep ? total - keep : 0;  // exclusive

    // Mark the version visible at each pinned timestamp P: the newest
    // version with ts <= P.  Tombstones are protected too, otherwise an older
    // live version would become visible at P after GC.
    std::vector<bool> is_protected(total, false);
    for (const auto& p : pinned) {
        auto it = std::upper_bound(ascending_ts.begin(), ascending_ts.end(), p);
        if (it == ascending_ts.begin()) {
            continue;  // key did not exist at P
        }
        is_protected[static_cast<size_t>(std::distance(ascending_ts.begin(), it)) - 1] = true;
    }

    std::vector<size_t> victims;
    for (size_t i = 0; i < max_index; ++i) {
        if (!(ascending_ts[i] < min_ts)) {
            break;  // ascending: no later version is below the watermark
        }
        if (!is_protected[i]) {
            victims.push_back(i);
        }
    }
    return victims;
}

uint64_t MVCCStore::deleteVersions(std::string_view key,
                                   const std::vector<HLCTimestamp>& timestamps) {
    if (timestamps.empty()) {
        return 0;
    }
    // Re-check pins against the current version chain.  The caller is
    // expected to hold the GC guard; re-checking keeps this method safe even
    // if it does not.
    std::vector<HLCTimestamp> pinned;
    if (auto pins = snapshotPinRegistry()) {
        pinned = pins->pinnedTimestamps();
    }
    std::vector<HLCTimestamp> chain;
    db_->scanPrefix(encodeVersionPrefix(key), [&](std::string_view vkey, std::string_view) -> bool {
        chain.push_back(decodeTimestamp(vkey));
        return true;
    });
    std::vector<bool> is_protected(chain.size(), false);
    for (const auto& p : pinned) {
        auto it = std::upper_bound(chain.begin(), chain.end(), p);
        if (it != chain.begin()) {
            is_protected[static_cast<size_t>(std::distance(chain.begin(), it)) - 1] = true;
        }
    }

    uint64_t deleted = 0;
    for (const auto& ts : timestamps) {
        auto it = std::lower_bound(chain.begin(), chain.end(), ts);
        if (it == chain.end() || *it != ts) {
            continue;  // already gone
        }
        const size_t idx = static_cast<size_t>(std::distance(chain.begin(), it));
        if (is_protected[idx] || idx + 1 == chain.size()) {
            continue;  // pinned, or the latest version (never deleted)
        }
        if (db_->del(encodeVersionedKey(key, ts))) {
            ++deleted;
        }
    }
    return deleted;
}

/**
 * @brief Gc All Before.
 * @param[in] min_ts Input parameter.
 * @param[in] opts Input parameter.
 * @return Return value.
 * @details Calls: scanBaseKeys(), emplace_back(), gcVersionsBefore().
 */
uint64_t MVCCStore::gcAllBefore(HLCTimestamp min_ts, GCOptions opts) {
    std::vector<std::string> base_keys;
    scanBaseKeys([&](std::string_view bk) -> bool {
        base_keys.emplace_back(bk);
        return true;
    });

    uint64_t total_deleted = 0;
    for (const auto& bk : base_keys) {
        total_deleted += gcVersionsBefore(bk, min_ts, opts);
    }
    return total_deleted;
}

void MVCCStore::scanBaseKeys(std::function<bool(std::string_view base_key)> callback) {
    // Collect all versioned keys (those using the versioned key format:
    // <base_key> '\x00' <8-byte-ts>).
    // A key is treated as versioned if it is at least 9 bytes long AND
    // the byte at position (size - 9) is '\x00' (the version separator).
    // Note: the separator position is always at size-9 (fixed width suffix),
    // NOT the last '\x00' found by rfind, because the 8-byte timestamp can
    // itself contain '\x00' bytes.
    std::vector<std::string> base_keys;

    db_->scanAll([&](std::string_view vkey, std::string_view) -> bool {
        if (vkey.size() >= 9 &&
            static_cast<unsigned char>(vkey[vkey.size() - 9]) == '\x00') {
            base_keys.emplace_back(vkey.data(), vkey.size() - 9);
        }
        return true;
    });

    // Deduplicate (the scan is ordered, but interleaved non-versioned keys may
    // cause duplicate base keys in the list).
    std::sort(base_keys.begin(), base_keys.end());
    base_keys.erase(std::unique(base_keys.begin(), base_keys.end()), base_keys.end());

    for (const auto& bk : base_keys) {
        if (!callback(bk)) {
            break;
        }
    }
}

void MVCCStore::scanBaseKeysWithPrefix(
    std::string_view prefix,
    std::function<bool(std::string_view base_key)> callback
) {
    std::vector<std::string> base_keys;
    db_->scanPrefix(prefix, [&](std::string_view vkey, std::string_view) -> bool {
        if (vkey.size() >= prefix.size() + 9 &&
            static_cast<unsigned char>(vkey[vkey.size() - 9]) == '\x00') {
            std::string_view bk(vkey.data(), vkey.size() - 9);
            if (base_keys.empty() || base_keys.back() != bk) {
                base_keys.emplace_back(bk);
            }
        }
        return true;
    });
    std::sort(base_keys.begin(), base_keys.end());
    base_keys.erase(std::unique(base_keys.begin(), base_keys.end()), base_keys.end());
    for (const auto& bk : base_keys) {
        if (!callback(bk)) {
            break;
        }
    }
}

std::optional<MVCCStore::VersionEntry> MVCCStore::getVersionAtTimestamp(
    std::string_view key,
    HLCTimestamp ts
) {
    std::optional<VersionEntry> visible;
    scanVersions(key, [&](const VersionEntry& e) -> bool {
        if (e.timestamp > ts) {
            return false;  // ascending order: nothing later can be visible
        }
        visible = e;
        return true;
    });
    return visible;
}

void MVCCStore::setSnapshotPinRegistry(std::shared_ptr<MVCCSnapshotPinRegistry> pins) {
    std::lock_guard<std::mutex> lk(pins_mu_);
    pins_ = std::move(pins);
}

std::shared_ptr<MVCCSnapshotPinRegistry> MVCCStore::snapshotPinRegistry() const {
    std::lock_guard<std::mutex> lk(pins_mu_);
    return pins_;
}

// ─────────────────────────────────────────────────────────────────────────────
// Clock access
// ─────────────────────────────────────────────────────────────────────────────

HLCTimestamp MVCCStore::currentTimestamp() const {
    return clock_->peek();
}

/**
 * @brief Update Clock.
 * @param[in] received Input parameter.
 * @return Return value.
 * @details Calls: update().
 */
HLCTimestamp MVCCStore::updateClock(HLCTimestamp received) {
    return clock_->update(received);
}

} // namespace themis
