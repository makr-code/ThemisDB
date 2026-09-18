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

struct RecoveredTwoPhaseCommitTransaction {
    std::string              transaction_id;       ///< Transaction identifier.
    bool                     completed    = false; ///< True when a "complete" entry was seen.
    bool                     has_decision = false; ///< True when a durable COMMIT/ABORT decision was recorded.
    bool                     decision_commit = false; ///< Valid when has_decision is true.

    std::vector<std::string> participants;

    std::string coordinator_id;

    std::optional<int64_t>   commit_timestamp_ns;
};

// ─────────────────────────────────────────────────────────────────────────────
// TwoPhaseCommitWALRecovery
// ─────────────────────────────────────────────────────────────────────────────

class TwoPhaseCommitWALRecovery {
public:
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
     * @brief Apply Begin.
     * @param[in,out] rec Input/output parameter.
     * @param[in] data Input parameter.
     * @details Calls: contains(), is_string(), mergeParticipants().
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
     * @brief Apply Decision Or Complete.
     * @param[in,out] rec Input/output parameter.
     * @param[in] data Input parameter.
     * @param[in] is_commit Input parameter.
     * @details Calls: contains(), is_string(), is_number().
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
     * @brief Merge Participants.
     * @param[in,out] rec Input/output parameter.
     * @param[in] data Input parameter.
     * @param[in] field_name Name of the field.
     * @details Calls: contains(), is_array(), is_string(), push_back(), is_object().
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
