/**
 * @file shard_artifact_fragment_store.h
 * @brief Durable, principal-scoped storage for immutable tensor artifact fragments.
 */
#pragma once

#include "sharding/shard_rpc_client.h"
#include "storage/rocksdb_wrapper.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace themis::sharding {

/**
 * @brief Persists immutable fragments addressed by authenticated principal, shard, and artifact.
 *
 * The key is versioned and independent of TensorNetworkStorageEngine field keys:
 * `(principal_id, shard_id, artifact_id)` maps to a dedicated RocksDB keyspace.
 * The principal is supplied by the host from its authorization policy; the RPC
 * server derives it from an explicit verified-mTLS identity binding.
 */
class ShardArtifactFragmentStore final {
public:
    /// @brief Default maximum payload size (64 MiB).
    static constexpr uint64_t kDefaultMaxFragmentBytes = 64ULL * 1024ULL * 1024ULL;

    /**
     * @brief Outcome of an immutable fragment write.
     */
    enum class PutResult {
        Stored,       ///< New payload and metadata were committed atomically.
        Unchanged,    ///< Identical content was synchronously committed again.
        Conflict,     ///< The key already names different immutable content.
        InvalidInput, ///< An identifier or payload violates configured bounds.
        StorageError  ///< The database transaction or existing record was invalid.
    };

    /**
     * @brief Creates a fragment store over an already-open RocksDB instance.
     * @param storage Shared owner of an already-open database. The store retains ownership;
     *        callers must not close the database while the store is in use.
     * @param max_fragment_bytes Maximum payload accepted and returned; in [1, 1 GiB].
     * @throws std::invalid_argument if the database is null/closed or the limit is invalid.
     * @note Concurrent calls are supported through independent RocksDB transactions.
     */
    explicit ShardArtifactFragmentStore(
        std::shared_ptr<RocksDBWrapper> storage,
        uint64_t max_fragment_bytes = kDefaultMaxFragmentBytes);

    /**
     * @brief Stores a fragment once under its authorization and manifest identity.
     * @param principal_id Non-empty storage authorization principal; at most 1024 bytes.
     * @param shard_id Exact shard placement identity; at most 1024 bytes.
     * @param artifact_id Opaque manifest artifact ID; at most 1024 bytes.
     * @param payload Complete fragment bytes, no larger than the configured maximum.
     *        Empty fragments are valid and use the SHA-256 digest of empty input.
     * @return Stored or Unchanged on success; Conflict, InvalidInput, or StorageError otherwise.
     * @throws std::runtime_error if SHA-256 computation fails.
     * @note Metadata and payload use one synchronous WAL transaction, independent of
     *       database default write options. Identical replays are committed again to
     *       establish durability; different bytes never overwrite an existing fragment.
     */
    [[nodiscard]] PutResult put(
        std::string_view principal_id,
        std::string_view shard_id,
        std::string_view artifact_id,
        const std::vector<uint8_t>& payload);

    /**
     * @brief Reads and verifies one complete fragment before returning it to the RPC server.
     * @param principal_id Authorization principal resolved from the verified client identity.
     * @param shard_id Exact shard placement identity.
     * @param artifact_id Opaque manifest artifact ID.
     * @param max_bytes Per-request ceiling; must be positive and no greater than the store limit.
     * @return Complete payload with its lowercase SHA-256, or empty if absent, unauthorized,
     *         or larger than the request ceiling.
     * @throws std::invalid_argument for invalid identifiers or byte limits.
     * @throws std::runtime_error for an unavailable database or corrupt/inconsistent stored data.
     * @note The size metadata is checked before the payload key is read. Returned bytes are
     *       hashed again; manifest verification remains the fetcher's responsibility.
     */
    [[nodiscard]] std::optional<TensorFragmentPayload> read(
        std::string_view principal_id,
        std::string_view shard_id,
        std::string_view artifact_id,
        uint64_t max_bytes) const;

private:
    [[nodiscard]] static bool validIdentifier(std::string_view value) noexcept;
    [[nodiscard]] static std::string keyPrefix(
        std::string_view principal_id,
        std::string_view shard_id,
        std::string_view artifact_id);

    std::shared_ptr<RocksDBWrapper> storage_;
    uint64_t max_fragment_bytes_;
};

} // namespace themis::sharding
