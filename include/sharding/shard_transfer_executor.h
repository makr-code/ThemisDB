#pragma once

#include "sharding/auto_rebalancer.h"
#include "sharding/shard_rpc_client.h"
#include "sharding/shard_transfer_store.h"

#include <stdexcept>

namespace themis::sharding {

/** @brief Storage-owned immutable logical export and its assigned target. */
struct ShardRangeExport {
    ShardTransferManifest manifest;
    std::shared_ptr<ShardRPCClient> target;
    std::function<std::pair<std::string, std::string>(uint64_t)> read_chunk;
};

/**
 * @brief Bind the rebalance executor to verified gRPC staging, not cutover.
 * @param open_export Trusted storage/control-plane adapter. It must check the
 *        approved operator request and resolve a current assignment and target,
 *        then open the immutable snapshot at the manifest watermark. The result
 *        must remain readable for the call and reproducible across retry, or
 *        require a fresh assignment after loss of the source snapshot.
 * @return An executor reporting at most TRANSFER_VERIFIED. It never writes
 *         topology or makes staging visible to normal database reads.
 * @throws std::invalid_argument if the export adapter is empty.
 * @note The executor may run concurrently and the adapter must bound its I/O.
 */
[[nodiscard]] inline AutoRebalancer::MigrationExecutor makeShardTransferExecutor(
    std::function<ShardRangeExport(const AutoRebalancer::MigrationRequest&)> open_export) {
    if (!open_export) {
        throw std::invalid_argument("Shard transfer requires an immutable export provider");
    }
    return [open_export = std::move(open_export)](const AutoRebalancer::MigrationRequest& request)
        -> AutoRebalancer::MigrationResult {
        using Outcome = AutoRebalancer::MigrationOutcome;
        auto outcome = Outcome::REJECTED;
        try {
            const auto snapshot = open_export(request);
            const auto& manifest = snapshot.manifest;
            const auto& move = request.operation;
            if (!snapshot.target || manifest.source != move.source_shard_id ||
                manifest.target != move.target_shard_id || manifest.range_start != move.token_range_start ||
                manifest.range_end != move.token_range_end) {
                return {outcome, "Export does not match the approved move"};
            }
            outcome = Outcome::ADMISSION_UNCERTAIN;
            if (snapshot.target->transferShard(manifest, snapshot.read_chunk, [&] {
                    outcome = Outcome::ADMITTED;
                })) {
                return {Outcome::TRANSFER_VERIFIED, "Ownership cutover is not configured"};
            }
            return {outcome, "Target did not verify persisted transfer contents"};
        } catch (const std::exception& e) {
            return {outcome, e.what()};
        } catch (...) {
            return {outcome, "Shard transfer failed"};
        }
    };
}

} // namespace themis::sharding
