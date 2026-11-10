/**
 * @file mvcc_snapshot_pins.h
 * @brief Durable snapshot leases that prevent MVCC GC from pruning the
 *        versions required to reproduce a referenced HLC snapshot.
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include "storage/hlc.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace themis {

class RocksDBWrapper;

/**
 * @brief Registry of MVCC snapshot pins ("leases").
 *
 * A pin at HLC timestamp @c P guarantees that, for every base key, the
 * version that is *visible* at @c P (the newest version with
 * `timestamp <= P`, including tombstones) is never deleted by MVCC garbage
 * collection while the pin is held.  This is exactly the set of versions
 * needed to re-read the store as of @c P, so a pinned snapshot stays
 * reproducible regardless of the GC watermark.
 *
 * ## Persistence
 * When constructed with a RocksDB instance, every pin is persisted under
 * `__mvcc_snapshot_pin__:<lease_id>` (value: 8-byte big-endian HLC
 * timestamp followed by the UTF-8 owner string) and reloaded on
 * construction, so pins survive process restarts.  Lease ids are lowercase
 * hex strings and therefore never contain the `\0` byte that MVCCStore uses
 * as its version separator, so pin records are never mistaken for versioned
 * keys.  With a null database the registry is in-memory only.
 *
 * ## GC interlock
 * GC implementations (MVCCStore::gcVersionsBefore, MVCCChainPruner) hold
 * gcGuard() (shared) while they compute and delete victims of one key;
 * acquire() takes the guard exclusively while publishing a new pin.  This
 * closes the race in which a GC pass that has already computed its victim
 * list deletes a version that a concurrently acquired pin needs.
 *
 * @note A pin cannot resurrect versions that were already pruned before the
 *       pin was acquired.  Callers that pin a *past* timestamp must verify
 *       the data afterwards (see themis::llm::lora::TrainingSnapshotReader).
 *
 * Thread-safe.
 */
class MVCCSnapshotPinRegistry {
public:
    /** @brief Storage key prefix for persisted pins. */
    static constexpr std::string_view kKeyPrefix = "__mvcc_snapshot_pin__:";

    /** @brief A single snapshot lease. */
    struct Pin {
        std::string  lease_id;   ///< Opaque lowercase-hex identifier.
        HLCTimestamp ts;         ///< Pinned snapshot timestamp.
        std::string  owner;      ///< Free-form owner tag (e.g. "lora:adapter-x").
    };

    /**
     * @brief Construct a registry and load persisted pins.
     * @param db Optional RocksDB instance used for persistence.  Null keeps
     *           pins in memory only (lost on restart).
     * @throws std::runtime_error if a persisted pin record is malformed.
     */
    explicit MVCCSnapshotPinRegistry(std::shared_ptr<RocksDBWrapper> db = nullptr);

    MVCCSnapshotPinRegistry(const MVCCSnapshotPinRegistry&) = delete;
    MVCCSnapshotPinRegistry& operator=(const MVCCSnapshotPinRegistry&) = delete;

    /**
     * @brief Acquire a new pin at @p ts.
     * @param ts    Snapshot timestamp to pin.
     * @param owner Owner tag stored with the pin (diagnostics / audit).
     * @return The created pin, or std::nullopt if persisting it failed.
     */
    std::optional<Pin> acquire(HLCTimestamp ts, std::string_view owner);

    /**
     * @brief Release the pin identified by @p lease_id.
     * @return true if the pin existed and was removed (including from disk).
     */
    bool release(std::string_view lease_id);

    /** @brief Look up a pin by lease id. */
    std::optional<Pin> find(std::string_view lease_id) const;

    /** @brief Snapshot of all current pins (unordered). */
    std::vector<Pin> list() const;

    /** @brief true if at least one pin exists exactly at @p ts. */
    bool isPinned(HLCTimestamp ts) const;

    /** @brief Sorted, de-duplicated list of all pinned timestamps. */
    std::vector<HLCTimestamp> pinnedTimestamps() const;

    /**
     * @brief Shared lock that GC passes must hold while selecting and
     *        deleting the victims of one key.
     */
    std::shared_lock<std::shared_mutex> gcGuard() const;

private:
    static std::string generateLeaseId();
    bool persist(const Pin& pin);

    std::shared_ptr<RocksDBWrapper> db_;
    mutable std::shared_mutex gc_mu_;   ///< GC interlock (see class docs).
    mutable std::mutex mu_;             ///< Protects pins_.
    std::unordered_map<std::string, Pin> pins_;
};

} // namespace themis
