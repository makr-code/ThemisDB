/**
 * @file two_phase_commit_wal_recovery.h
 * @brief WAL recovery logic for the two-phase commit protocol.
 *
 * Declares the WAL reader and recovery state machine that resurrect
 * in-doubt 2PC transactions after a coordinator restart.
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// Shared WAL reconstruction logic for 2PC recovery.
//
// TwoPhaseCommitCoordinator and GlobalTransactionManager both persist their
// transaction state to WAL using the normalized JSON schema:
//
//   BEGIN_TX   → { "transaction_id": "…", "coordinator_id": "…",
//                  "shards"|"regions": […] }
//   COMMIT_TX  → { "phase": "decision", "decision": "commit" }   (durable decision)
//   ABORT_TX   → { "phase": "decision", "decision": "abort" }    (durable decision)
//   COMMIT_TX  → { "phase": "complete" }                         (Phase 2 done)
//   ABORT_TX   → { "phase": "complete" }                         (Phase 2 done)
//
// TwoPhaseCommitWALRecovery::reconstruct() reads these entries and produces a
// map of transaction_id → RecoveredTwoPhaseCommitTransaction so that the
// caller only needs to iterate the result and re-drive non-completed transactions.

#pragma once

#include "sharding/wal_manager.h"
#include <map>
#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace themis::transaction {

// ─────────────────────────────────────────────────────────────────────────────
// Canonical WAL replay result for one 2PC transaction
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Canonical WAL replay result for one 2PC transaction.
 *
 * Produced by TwoPhaseCommitWALRecovery::reconstruct().  All fields are
 * populated from WAL entries and represent the last durable state observed.
 */
struct RecoveredTwoPhaseCommitTransaction {
    std::string              transaction_id;       ///< Transaction identifier.
    bool                     completed    = false; ///< True when a "complete" entry was seen.
    bool                     has_decision = false; ///< True when a durable COMMIT/ABORT decision was recorded.
    bool                     decision_commit = false; ///< Valid when has_decision is true.

    /// Participant identifiers extracted from the BEGIN_TX entry.
    /// May be empty if the coordinator did not write shard/region metadata.
    std::vector<std::string> participants;

    /// Coordinator identifier extracted from the BEGIN_TX entry (may be empty).
    std::string coordinator_id;

    /// Commit timestamp in nanoseconds (if present in the decision entry).
    std::optional<int64_t>   commit_timestamp_ns;
};

// ─────────────────────────────────────────────────────────────────────────────
// TwoPhaseCommitWALRecovery
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Shared reconstruction logic for 2PC recovery from WALManager entries.
 *
 * This helper decodes the normalized WAL schema written by
 * TwoPhaseCommitCoordinator and GlobalTransactionManager, so each coordinator
 * does not have to duplicate the replay logic.
 *
 * It also accepts the simpler per-entry format written by
 * DistributedTransactionManager (PREPARE_TX with no JSON payload) so that a
 * mixed WAL stream from heterogeneous coordinators remains recoverable.
 */
class TwoPhaseCommitWALRecovery {
public:
    /**
     * @brief Rebuild recoverable transaction state from ordered WAL entries.
     *
     * Replays @p entries in order and returns a map keyed by transaction ID.
     * Entries whose transaction_id is empty are silently ignored.
     *
     * Recognised entry types:
     * - BEGIN_TX   — creates a new transaction record; extracts shards/regions.
     * - COMMIT_TX  — records durable COMMIT decision or marks completion.
     * - ABORT_TX   — records durable ABORT decision or marks completion.
     * - PREPARE_TX — marks in-doubt state (DistributedTransactionManager style).
     *
     * @param entries WAL entries in append order.
     * @return        Map from transaction_id to recovered state.
     */
    [[nodiscard]] static std::map<std::string, RecoveredTwoPhaseCommitTransaction>
    reconstruct(const std::vector<themis::sharding::WALEntry>& entries) {
        std::map<std::string, RecoveredTwoPhaseCommitTransaction> result;

        for (const auto& entry : entries) {
            const std::string& txn_id = entry.transaction_id;
            if (txn_id.empty()) {
                continue;
            }

            auto& rec = result[txn_id];
            rec.transaction_id = txn_id;

            switch (entry.type) {
            case themis::sharding::WALEntryType::BEGIN_TX:
                applyBegin(rec, entry.data);
                break;

            case themis::sharding::WALEntryType::PREPARE_TX:
                // DistributedTransactionManager style: PREPARE_TX marks in-doubt.
                // No decision yet — has_decision stays false.
                rec.has_decision  = false;
                rec.completed     = false;
                break;

            case themis::sharding::WALEntryType::COMMIT_TX:
                applyDecisionOrComplete(rec, entry.data, /*is_commit=*/true);
                break;

            case themis::sharding::WALEntryType::ABORT_TX:
                applyDecisionOrComplete(rec, entry.data, /*is_commit=*/false);
                break;

            default:
                break;
            }
        }

        return result;
    }

private:
    /**
     * @brief Apply a BEGIN_TX entry: extract coordinator_id and participants.
     *
     * @param rec   Transaction record to populate.
     * @param data  JSON payload from the WAL entry.
     */
    static void applyBegin(
        RecoveredTwoPhaseCommitTransaction& rec,
        const nlohmann::json&               data
    ) {
        if (data.contains("coordinator_id") && data["coordinator_id"].is_string()) {
            rec.coordinator_id = data["coordinator_id"].get<std::string>();
        }

        // Accept both "shards" (TwoPhaseCommitCoordinator) and
        // "regions" (GlobalTransactionManager) as participant lists.
        mergeParticipants(rec, data, "shards");
        mergeParticipants(rec, data, "regions");
        mergeParticipants(rec, data, "participants");
    }

    /**
     * @brief Apply a COMMIT_TX or ABORT_TX entry.
     *
     * If the entry has "phase": "complete" it marks the transaction as done.
     * Otherwise it records a durable decision.
     *
     * @param rec       Transaction record to update.
     * @param data      JSON payload from the WAL entry.
     * @param is_commit True for COMMIT_TX; false for ABORT_TX.
     */
    static void applyDecisionOrComplete(
        RecoveredTwoPhaseCommitTransaction& rec,
        const nlohmann::json&               data,
        bool                                is_commit
    ) {
        // A "phase": "complete" entry supersedes decision entries.
        if (data.contains("phase") && data["phase"].is_string() &&
            data["phase"].get<std::string>() == "complete") {
            rec.completed = true;
            return;
        }

        // Durable decision (TwoPhaseCommitCoordinator writes "phase":"decision";
        // GlobalTransactionManager writes the COMMIT_TX/ABORT_TX entry without
        // a "complete" field as the decision itself).
        rec.has_decision    = true;
        rec.decision_commit = is_commit;

        // Capture optional commit timestamp (GlobalTransactionManager TrueTime).
        if (is_commit) {
            if (data.contains("commit_timestamp_ns") &&
                data["commit_timestamp_ns"].is_number()) {
                rec.commit_timestamp_ns =
                    data["commit_timestamp_ns"].get<int64_t>();
            } else if (data.contains("commit_time") &&
                       data["commit_time"].is_number()) {
                rec.commit_timestamp_ns =
                    data["commit_time"].get<int64_t>();
            }
        }
    }

    /**
     * @brief Append participant identifiers from a named JSON array field.
     *
     * @param rec        Transaction record to populate.
     * @param data       JSON object potentially containing the list.
     * @param field_name Key to look up in @p data.
     */
    static void mergeParticipants(
        RecoveredTwoPhaseCommitTransaction& rec,
        const nlohmann::json&               data,
        const char*                         field_name
    ) {
        if (!data.contains(field_name)) {
            return;
        }

        const auto& arr = data[field_name];
        if (!arr.is_array()) {
            return;
        }

        for (const auto& item : arr) {
            if (item.is_string()) {
                rec.participants.push_back(item.get<std::string>());
            } else if (item.is_object() && item.contains("node_id") &&
                       item["node_id"].is_string()) {
                // Accept structured participant objects from DTM-style BEGIN entries.
                rec.participants.push_back(item["node_id"].get<std::string>());
            }
        }
    }
};

} // namespace themis::transaction
