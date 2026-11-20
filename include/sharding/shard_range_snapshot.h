#pragma once

#include "sharding/shard_transfer_store.h"

#include <utility>

namespace themis::sharding {

/**
 * @brief An immutable, process-local logical range export.
 * The storage adapter must retain the result for retries under its assigned ID.
 * If it is lost, the control plane must issue a fresh migration ID rather than
 * opening a new storage snapshot for the old assignment.
 */
struct ShardRangeSnapshot {
    ShardTransferManifest manifest;
    std::function<std::pair<std::string, std::string>(uint64_t)> read_chunk;
};

/**
 * @brief Materialize a bounded canonical export from an already pinned storage snapshot.
 * @param assignment ID, shards, ascending half-open token range, epoch and storage
 *        watermark. Integrity fields are computed, not taken from the input.
 * @param scan Synchronously visit only storage-owned live logical records in
 *        the range at that watermark, once each, in strictly increasing bytewise
 *        key order. Tokens must be computed using the authoritative routing domain.
 *        The visitor copies each record; views need only last for that call.
 * @return Immutable manifest and concurrent-safe indexed reader. Out-of-range
 *         reads throw std::out_of_range. Empty scans produce an explicit empty manifest.
 * @throws std::invalid_argument for invalid metadata, an out-of-range token or
 *         invalid key order; std::length_error above 64 MiB of encoded records.
 *         Storage and digest errors propagate; no partial export is returned.
 * @note Internal storage API. Each record is encoded as unsigned big-endian
 *       32-bit key size, 32-bit value size, key bytes, value bytes. Chunk
 *       boundaries may split records. This does not quiesce writes or publish data.
 */
[[nodiscard]] ShardRangeSnapshot materializeShardRangeSnapshot(
    ShardTransferManifest assignment,
    const std::function<void(const std::function<void(
        uint64_t token, std::string_view key, std::string_view value)>&)>& scan);

} // namespace themis::sharding
