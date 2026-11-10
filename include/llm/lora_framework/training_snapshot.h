/**
 * @file training_snapshot.h
 * @brief MVCC snapshot capture, retention pinning and Merkle fingerprinting
 *        of LoRA/AdaLoRA training data (training-data snapshot governance).
 *
 * The fingerprint binds an adapter to the exact records it was trained on
 * without persisting the records themselves: only hashes, counts, the
 * selector specification and the HLC snapshot timestamp are stored in the
 * adapter metadata (`themis.training.snapshot.*`).
 *
 * ## Consistency guarantee
 * Snapshot-provenanced datasets are read exclusively from the MVCCStore via
 * MVCCStore::getVersionAtTimestamp() at one HLC timestamp `T` captured at
 * job start.  Every record of a job is therefore the version visible at `T`
 * (newest version with commit ts ≤ T); later writes are invisible.  AQL
 * collection scans (LoRADatasetLoader::load) read live data and cannot be
 * pinned to a timestamp, so they are NOT used for provenanced jobs.
 *
 * Residual window: MVCCStore::put() assigns its HLC timestamp before the
 * RocksDB write lands, so a write in flight at capture time may receive a
 * timestamp ≤ T but become visible only after the capture read.  This is
 * detected (never silently accepted): the orchestrator re-reads the snapshot
 * after training and before signing and fails the job if the Merkle root
 * changed (TrainingSnapshotReader::verify()).
 *
 * ## Key layout
 * Dataset records of collection `c` live under MVCC base keys
 * `lora_ds:<c>:<record_key>`.  Values are UTF-8 JSON documents with string
 * `input`/`output` and optional `context` fields (see LoRADatasetLoader).
 * An empty value is an MVCC tombstone and means "record absent".
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include "storage/hlc.h"

#include <nlohmann/json.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace themis {
class MVCCStore;
}

namespace themis {
namespace llm {
namespace lora {

/// @brief Raw SHA-256 digest.
using Sha256Digest = std::array<uint8_t, 32>;

/// @brief SHA-256 of @p bytes (OpenSSL EVP).  @throws std::runtime_error on EVP failure.
[[nodiscard]] Sha256Digest sha256Digest(std::string_view bytes);

/// @brief Lowercase hex encoding of a digest.
[[nodiscard]] std::string digestToHex(const Sha256Digest& d);

/// @brief Lowercase hex SHA-256 of @p bytes.
[[nodiscard]] std::string sha256Hex(std::string_view bytes);

/// @brief Metadata key names (single source of truth for C++).
namespace snapshot_keys {
inline constexpr const char* kCollection      = "themis.training.snapshot.collection";
inline constexpr const char* kHlcTs           = "themis.training.snapshot.hlc_ts";
inline constexpr const char* kQuery           = "themis.training.snapshot.query";
inline constexpr const char* kQuerySha256     = "themis.training.snapshot.query_sha256";
inline constexpr const char* kRecordCount     = "themis.training.snapshot.record_count";
inline constexpr const char* kMerkleRoot      = "themis.training.snapshot.merkle_root";
inline constexpr const char* kSplitSeed       = "themis.training.snapshot.split_seed";
inline constexpr const char* kHoldoutFraction = "themis.training.snapshot.holdout_fraction";
inline constexpr const char* kPinned          = "themis.training.snapshot.pinned";
}  // namespace snapshot_keys

/**
 * @brief One canonical Merkle leaf input: identifies a record version by hash.
 */
struct SnapshotRecordRef {
    std::string  collection;     ///< Dataset collection name.
    std::string  key;            ///< Record key (without the `lora_ds:<c>:` prefix).
    uint64_t     version_ts = 0; ///< Raw HLC value of the version visible at T.
    Sha256Digest value_sha256{}; ///< SHA-256 of the stored value bytes.
};

/**
 * @brief Deterministic binary Merkle tree over snapshot records.
 *
 * - Leaf  = SHA256(0x00 ‖ u32be(len(collection)) ‖ collection ‖
 *                  u32be(len(key)) ‖ key ‖ u64be(version_ts) ‖ value_sha256)
 * - Node  = SHA256(0x01 ‖ left ‖ right)
 * - Leaves are sorted by key (bytewise) first, so the root is independent of
 *   input order.  An odd node at the end of a level is promoted unchanged
 *   (no duplication, avoiding the CVE-2012-2459 ambiguity).
 * - The root of an empty set is SHA256("").
 */
class SnapshotMerkleTree {
public:
    /// @brief Hash of one leaf (see class docs).
    [[nodiscard]] static Sha256Digest leafHash(const SnapshotRecordRef& rec);

    /**
     * @brief Compute the Merkle root (lowercase hex).
     * @param records Records in any order.
     * @param error   Receives a message on failure (duplicate keys).
     * @return Hex root, or std::nullopt if two records share a key.
     */
    [[nodiscard]] static std::optional<std::string> root(std::vector<SnapshotRecordRef> records,
                                                         std::string* error = nullptr);
};

/**
 * @brief Selector specification of a snapshot dataset.
 *
 * Selection rule: all base keys under keyPrefix() that are visible (not a
 * tombstone) at the snapshot timestamp, in ascending bytewise key order,
 * truncated to the first @c limit keys.
 */
struct SnapshotSelector {
    std::string collection;   ///< Must satisfy LoRADatasetLoader::isValidCollectionName.
    std::size_t limit = 0;    ///< Maximum records, in [1, LoRADatasetLoader::kMaxSamples].

    /// @brief MVCC key prefix `lora_ds:<collection>:`.
    [[nodiscard]] std::string keyPrefix() const;
    /// @brief MVCC key prefix for @p collection.
    [[nodiscard]] static std::string keyPrefixFor(std::string_view collection);
    /// @brief Validate collection and limit.  Fills @p error on failure.
    [[nodiscard]] bool validate(std::string* error = nullptr) const;
    /**
     * @brief Canonical selector JSON (sorted keys, no whitespace), e.g.
     * `{"collection":"c","key_prefix":"lora_ds:c:","limit":100,"order":"key_asc","source":"mvcc","v":1}`.
     */
    [[nodiscard]] std::string canonicalJson() const;
    /// @brief SHA-256 (hex) of canonicalJson().
    [[nodiscard]] std::string sha256() const;
    /// @brief Parse canonical selector JSON produced by canonicalJson().
    [[nodiscard]] static std::optional<SnapshotSelector> fromCanonicalJson(std::string_view json,
                                                                          std::string* error = nullptr);
};

/**
 * @brief Hash-only fingerprint of a training snapshot (no data).
 */
struct TrainingSnapshotFingerprint {
    std::string collection;         ///< Dataset collection.
    uint64_t    hlc_ts = 0;         ///< Raw HLC snapshot timestamp T.
    std::string query;              ///< Canonical selector JSON.
    std::string query_sha256;       ///< SHA-256 of #query.
    uint64_t    record_count = 0;   ///< Number of records in the Merkle tree.
    std::string merkle_root;        ///< Hex Merkle root over the records.
    uint64_t    split_seed = 0;     ///< Train/holdout split seed.
    double      holdout_fraction = 0.0; ///< Holdout fraction.
    bool        pinned = false;     ///< Whether a GC retention pin was held at capture.

    /// @brief Serialise to `themis.training.snapshot.*` string metadata.
    [[nodiscard]] std::map<std::string, std::string> toMetadata() const;

    /**
     * @brief Parse from metadata (extra keys are ignored).
     * @return std::nullopt with @p error set if a key is missing/malformed or
     *         query_sha256 does not match the query.
     */
    [[nodiscard]] static std::optional<TrainingSnapshotFingerprint> fromMetadata(
        const std::map<std::string, std::string>& metadata, std::string* error = nullptr);

    /// @brief True iff both fingerprints identify identical training data
    ///        (collection, hlc_ts, query_sha256, record_count, merkle_root).
    [[nodiscard]] bool sameData(const TrainingSnapshotFingerprint& other) const;
};

/// @brief Error categories of snapshot operations.
enum class SnapshotErrorCode {
    None,
    InvalidSelector,
    NoStore,
    PinFailed,
    InvalidRecord,
    SnapshotUnavailable,   ///< Data needed for the snapshot has been pruned.
    FingerprintMismatch,   ///< Data at T no longer matches the fingerprint.
    Internal
};

/// @brief Result of a snapshot read.
struct TrainingSnapshotRead {
    bool ok = false;
    SnapshotErrorCode code = SnapshotErrorCode::None;
    std::string error;
    TrainingSnapshotFingerprint fingerprint;  ///< Valid iff ok.
    nlohmann::json rows = nlohmann::json::array(); ///< Parsed documents in key order (iff ok).
    std::optional<std::string> lease_id;      ///< Pin lease held for this snapshot, if any.
};

/**
 * @brief Captures, re-reads and verifies MVCC training snapshots.
 *
 * Thread-safe (stateless apart from the shared store).
 */
class TrainingSnapshotReader {
public:
    /**
     * @param store MVCC store holding `lora_ds:*` records.  Its attached
     *              MVCCSnapshotPinRegistry (if any) is used for leases.
     */
    explicit TrainingSnapshotReader(std::shared_ptr<MVCCStore> store);

    /**
     * @brief Capture a new snapshot at a fresh HLC timestamp T and read it.
     *
     * Order: T = fresh HLC tick → acquire pin at T (if a registry is
     * attached; failure aborts) → read all records at T.  Because the pin
     * protects the versions visible at T before any GC can run, the snapshot
     * remains reproducible until the lease is released.
     *
     * @param owner Owner tag for the pin (e.g. "lora:<adapter_id>").
     * @return ok=false on invalid selector, pin failure or invalid record.
     *         On failure no lease is left behind.
     */
    [[nodiscard]] TrainingSnapshotRead capture(const SnapshotSelector& selector,
                                               uint64_t split_seed,
                                               double holdout_fraction,
                                               std::string_view owner);

    /**
     * @brief Read the snapshot at an explicit timestamp (no pinning).
     * @param pinned Value recorded in the returned fingerprint.
     */
    [[nodiscard]] TrainingSnapshotRead readAt(const SnapshotSelector& selector,
                                              HLCTimestamp ts,
                                              uint64_t split_seed,
                                              double holdout_fraction,
                                              bool pinned) const;

    /**
     * @brief Re-read the data described by @p expected and compare.
     *
     * If @p acquire_pin is true and a registry is attached, a lease at
     * expected.hlc_ts is acquired first (and released again on failure) so
     * that the verified data cannot be pruned before it is used.
     *
     * @return ok=true with rows on exact match.  Otherwise
     *         SnapshotUnavailable (record count lower than recorded, i.e. a
     *         referenced version was pruned or never existed),
     *         FingerprintMismatch (any other difference), or another error.
     */
    [[nodiscard]] TrainingSnapshotRead verify(const TrainingSnapshotFingerprint& expected,
                                              bool acquire_pin,
                                              std::string_view owner);

    /// @brief Release a lease obtained by capture()/verify().
    bool releasePin(std::string_view lease_id);

private:
    std::shared_ptr<MVCCStore> store_;
};

}  // namespace lora
}  // namespace llm
}  // namespace themis
