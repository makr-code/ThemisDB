/**
 * @file two_phase_commit_wal_recovery.h
 * @brief Shared WAL replay helpers for 2PC coordinator recovery.
 */

#pragma once

#include "sharding/wal_manager.h"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace themis::transaction {

/**
 * @brief Canonical WAL replay result for one 2PC transaction.
 */
struct RecoveredTwoPhaseCommitTransaction {
    std::string         transaction_id;          ///< Transaction identifier from WAL.
    nlohmann::json      participants = nlohmann::json::array(); ///< Normalized participant array.
    std::optional<bool> decision;               ///< COMMIT=true, ABORT=false, nullopt=no durable decision.
    bool                prepared = false;       ///< True when a PREPARE marker exists.
    bool                completed = false;      ///< True when a terminal COMPLETE marker exists.
    int64_t             commit_timestamp_ns = 0; ///< Durable commit timestamp when available.
    nlohmann::json      last_metadata = nlohmann::json::object(); ///< Last payload seen for txn.

    /**
     * @brief Return all known participant/shard IDs from normalized metadata.
     * @return Ordered list of non-empty participant identifiers.
     */
    [[nodiscard]] std::vector<std::string> participantIds() const {
        std::vector<std::string> ids;
        if (!participants.is_array()) {
            return ids;
        }

        ids.reserve(participants.size());
        for (const auto& participant : participants) {
            if (participant.is_string()) {
                const auto shard_id = participant.get<std::string>();
                if (!shard_id.empty()) {
                    ids.push_back(shard_id);
                }
                continue;
            }

            if (participant.is_object()) {
                const auto shard_id = participant.value("shard_id", std::string{});
                if (!shard_id.empty()) {
                    ids.push_back(shard_id);
                }
            }
        }
        return ids;
    }
};

/**
 * @brief Shared reconstruction logic for 2PC recovery from WALManager entries.
 *
 * This helper intentionally accepts both the current normalized schema and the
 * legacy DistributedTransactionCoordinator payloads so that older WAL segments
 * remain recoverable after migration.
 */
class TwoPhaseCommitWALRecovery {
public:
    /**
     * @brief Rebuild recoverable transaction state from ordered WAL entries.
     * @param entries WAL entries in append order.
     * @return Map keyed by transaction ID.
     */
    [[nodiscard]] static std::map<std::string, RecoveredTwoPhaseCommitTransaction>
    reconstruct(const std::vector<themis::sharding::WALEntry>& entries) {
        std::map<std::string, RecoveredTwoPhaseCommitTransaction> recovered;

        for (const auto& entry : entries) {
            if (entry.transaction_id.empty()) {
                continue;
            }

            auto& txn = recovered[entry.transaction_id];
            txn.transaction_id = entry.transaction_id;
            txn.last_metadata = entry.data;

            switch (entry.type) {
                case themis::sharding::WALEntryType::BEGIN_TX:
                    mergeParticipants(txn, entry.data);
                    break;
                case themis::sharding::WALEntryType::PREPARE_TX:
                    txn.prepared = true;
                    mergeParticipants(txn, entry.data);
                    break;
                case themis::sharding::WALEntryType::COMMIT_TX:
                    mergeParticipants(txn, entry.data);
                    txn.commit_timestamp_ns = extractCommitTimestamp(entry.data, txn.commit_timestamp_ns);
                    if (isCompletionEntry(entry)) {
                        txn.completed = true;
                        txn.decision = true;
                    } else {
                        txn.decision = true;
                    }
                    break;
                case themis::sharding::WALEntryType::ABORT_TX:
                    mergeParticipants(txn, entry.data);
                    if (isCompletionEntry(entry)) {
                        txn.completed = true;
                        txn.decision = false;
                    } else {
                        txn.decision = false;
                    }
                    break;
                default:
                    break;
            }
        }

        return recovered;
    }

private:
    static constexpr int kLegacyDistributedTxnCommitted = 4;
    static constexpr int kLegacyDistributedTxnAborted = 6;

    static void mergeParticipants(
        RecoveredTwoPhaseCommitTransaction& txn,
        const nlohmann::json& data
    ) {
        nlohmann::json normalized = nlohmann::json::array();

        if (data.contains("participants")) {
            normalized = normalizeParticipants(data["participants"]);
        } else if (data.contains("shards")) {
            normalized = normalizeParticipants(data["shards"]);
        }

        if (!normalized.empty()) {
            txn.participants = std::move(normalized);
        }
    }

    [[nodiscard]] static nlohmann::json normalizeParticipants(
        const nlohmann::json& raw_participants
    ) {
        nlohmann::json normalized = nlohmann::json::array();
        if (!raw_participants.is_array()) {
            return normalized;
        }

        for (const auto& participant : raw_participants) {
            if (participant.is_string()) {
                const auto shard_id = participant.get<std::string>();
                if (!shard_id.empty()) {
                    normalized.push_back({{"shard_id", shard_id}});
                }
                continue;
            }

            if (!participant.is_object()) {
                continue;
            }

            nlohmann::json normalized_participant = participant;
            if (!normalized_participant.contains("shard_id")) {
                const auto shard_id = normalized_participant.value("node_id", std::string{});
                if (!shard_id.empty()) {
                    normalized_participant["shard_id"] = shard_id;
                }
            }

            if (!normalized_participant.value("shard_id", std::string{}).empty()) {
                normalized.push_back(std::move(normalized_participant));
            }
        }

        return normalized;
    }

    [[nodiscard]] static int64_t extractCommitTimestamp(
        const nlohmann::json& data,
        int64_t fallback
    ) {
        if (data.contains("commit_timestamp_ns")) {
            return data["commit_timestamp_ns"].get<int64_t>();
        }
        if (data.contains("commit_time")) {
            return data["commit_time"].get<int64_t>();
        }
        return fallback;
    }

    [[nodiscard]] static bool isCompletionEntry(
        const themis::sharding::WALEntry& entry
    ) {
        const auto phase = entry.data.value("phase", std::string{});
        if (phase == "complete") {
            return true;
        }

        if (phase == "decision" || phase == "prepared") {
            return false;
        }

        if (!entry.data.contains("state")) {
            return false;
        }

        if (!entry.data["state"].is_number_integer()) {
            return false;
        }

        const auto state = entry.data["state"].get<int>();
        if (entry.type == themis::sharding::WALEntryType::COMMIT_TX) {
            return state == kLegacyDistributedTxnCommitted;
        }
        if (entry.type == themis::sharding::WALEntryType::ABORT_TX) {
            return state == kLegacyDistributedTxnAborted;
        }
        return false;
    }
};

} // namespace themis::transaction
