#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace themis::sharding {

/// Immutable assignment and snapshot integrity contract for a single transfer ID.
struct ShardTransferManifest {
    std::string id, source, target, watermark, sha256;
    uint64_t range_start{}, range_end{}, epoch{}, chunks{}, bytes{};
    bool operator==(const ShardTransferManifest&) const = default;
};

/// Durable, isolated destination staging. Verification never publishes data or changes ownership.
///
/// Calls are serialized for each store. The assignment predicate must be thread-safe,
/// must not re-enter this store, and must consult current authoritative assignment state.
/// The caller supplies the authenticated source shard identity as `principal`.
/// Malformed inputs throw std::invalid_argument; authorization, state and integrity
/// contract violations throw std::logic_error; storage or persisted corruption errors
/// throw std::runtime_error.
class ShardTransferStore {
public:
    static constexpr std::size_t kMaxChunkBytes = 1024 * 1024;

    /// Open a dedicated staging DB bound to `local_shard`. An empty predicate is invalid.
    explicit ShardTransferStore(
        const std::string& path, std::string local_shard,
        std::function<bool(const ShardTransferManifest&)> current_assignment);
    ~ShardTransferStore();

    /// Persist an authorized manifest. Identical assignments are idempotent.
    /// IDs, shard names and watermark are nonempty UTF-8 strings of at most 1 KiB
    /// without NUL. The range is ascending and digests are lowercase hex SHA-256.
    /// A transfer may declare at most 65,536 chunks and 1 GiB of payload.
    void assign(const ShardTransferManifest&, const std::string& principal);

    /// Start or resume an exact assigned manifest and return the durable next chunk index.
    uint64_t prepare(const ShardTransferManifest&, const std::string& principal);

    /// Persist the next nonempty chunk (or acknowledge an identical replay), returning
    /// the durable next index. A zero-byte transfer has no chunks.
    uint64_t append(const std::string& id, const std::string& principal, uint64_t index,
                    const std::string& bytes, const std::string& sha256);

    /// Verify the exact count, byte length and digest of the staged stream in index order.
    void verify(const std::string& id, const std::string& principal);

    /**
     * @brief Read verified staging in canonical stream order without publishing it.
     * @param manifest Exact assignment expected by the local storage consumer.
     * @param consume Synchronous per-chunk visitor; views are valid only during the call.
     *        Must not re-enter this store or write visible data. If it throws or
     *        authorization changes during consumption, discard any partial output.
     * @throws std::logic_error if the assignment is stale, differs or is not verified.
     * @throws std::runtime_error if persisted data fails integrity or storage checks.
     * @throws std::invalid_argument if the manifest or visitor is invalid.
     * @note Internal storage API, not an RPC or an ownership authorization. The
     *       store serializes validation and visiting against abort and append.
     */
    void readVerified(const ShardTransferManifest& manifest,
                      const std::function<void(std::string_view)>& consume);

    /// Remove staged chunks atomically and retain a tombstone; the ID cannot be reused.
    void abort(const std::string& id, const std::string& principal);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace themis::sharding
