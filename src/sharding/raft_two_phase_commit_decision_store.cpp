/**
 * @file raft_two_phase_commit_decision_store.cpp
 * @brief Durable Raft decision authority for the canonical 2PC coordinator.
 */
#include "sharding/raft_two_phase_commit_decision_store.h"

#include "sharding/raft_consensus.h"
#include "sharding/raft_log.h"

#include <nlohmann/json.hpp>

#include <cstddef>
#include <limits>
#include <map>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace themis::sharding {
namespace {

constexpr std::string_view kDecisionPrefix = "themisdb.2pc.decision.v1:";
constexpr std::string_view kDecisionSchema = "themisdb.2pc.decision.v1";
constexpr std::string_view kDecisionSnapshotSchema =
    "themisdb.2pc.decision-state.v1";
constexpr std::size_t kMaxIdentityBytes = 1024;
constexpr std::size_t kMaxDecisionSnapshotBytes = 256 * 1024 * 1024;

using DecisionKey = std::pair<std::string, std::string>;
using DecisionSnapshot = std::map<DecisionKey, TwoPhaseCommitDecision>;

std::mutex g_operation_mutex_registry_mutex;
std::unordered_map<themisdb::sharding::RaftConsensus*, std::weak_ptr<std::mutex>>
    g_operation_mutex_registry;

std::shared_ptr<std::mutex> operationMutexFor(
    themisdb::sharding::RaftConsensus* raft) {
    std::lock_guard<std::mutex> lock(g_operation_mutex_registry_mutex);
    auto& weak_mutex = g_operation_mutex_registry[raft];
    auto operation_mutex = weak_mutex.lock();
    if (!operation_mutex) {
        operation_mutex = std::make_shared<std::mutex>();
        weak_mutex = operation_mutex;
    }
    return operation_mutex;
}

bool validIdentity(std::string_view value) noexcept {
    return !value.empty() && value.size() <= kMaxIdentityBytes;
}

void validateDecision(const TwoPhaseCommitDecision& decision) {
    if (decision.commit_timestamp_ns &&
        (!decision.commit || *decision.commit_timestamp_ns <= 0)) {
        throw std::invalid_argument(
            "a 2PC COMMIT timestamp must be positive and cannot accompany ABORT");
    }
}

struct DecisionHistory {
    std::optional<TwoPhaseCommitDecision> committed;
    std::optional<TwoPhaseCommitDecision> retained_uncommitted;
    bool conflict = false;
};

void mergeDecision(
    std::optional<TwoPhaseCommitDecision>& destination,
    const TwoPhaseCommitDecision& decision,
    bool& conflict) {
    if (destination &&
        (destination->commit != decision.commit ||
         destination->commit_timestamp_ns != decision.commit_timestamp_ns)) {
        conflict = true;
        return;
    }
    destination = decision;
}

struct DecisionRecord {
    std::string coordinator_id;
    std::string transaction_id;
    TwoPhaseCommitDecision decision;
};

std::optional<DecisionRecord> parseDecision(
    const themisdb::sharding::LogEntry& entry,
    const std::string& expected_group) {
    if (!entry.command.starts_with(kDecisionPrefix)) {
        return std::nullopt;
    }

    try {
        const auto payload = nlohmann::json::parse(
            entry.command.substr(kDecisionPrefix.size()));
        if (!payload.is_object() ||
            payload.at("schema").get<std::string>() != kDecisionSchema) {
            throw std::runtime_error("unsupported Raft 2PC decision schema");
        }

        const auto group_id = payload.at("raft_group_id").get<std::string>();
        const auto coordinator_id = payload.at("coordinator_id").get<std::string>();
        const auto transaction_id = payload.at("transaction_id").get<std::string>();
        if (!validIdentity(group_id) || !validIdentity(coordinator_id) ||
            !validIdentity(transaction_id)) {
            throw std::runtime_error("invalid identity in Raft 2PC decision");
        }
        if (group_id != expected_group) {
            return std::nullopt;
        }

        const auto decision_name = payload.at("decision").get<std::string>();
        if (decision_name != "commit" && decision_name != "abort") {
            throw std::runtime_error("invalid Raft 2PC decision value");
        }

        TwoPhaseCommitDecision decision;
        decision.commit = decision_name == "commit";
        const auto& timestamp = payload.at("commit_timestamp_ns");
        if (!timestamp.is_null()) {
            if (!timestamp.is_number_integer()) {
                throw std::runtime_error("invalid Raft 2PC commit timestamp");
            }
            decision.commit_timestamp_ns = timestamp.get<int64_t>();
        }
        validateDecision(decision);
        return DecisionRecord{
            coordinator_id,
            transaction_id,
            decision
        };
    } catch (const std::exception& error) {
        throw std::runtime_error(
            "corrupt Raft 2PC decision at log index " +
            std::to_string(entry.index) + ": " + error.what());
    }
}

void mergeSnapshotDecision(
    DecisionSnapshot& snapshot,
    const DecisionKey& key,
    const TwoPhaseCommitDecision& decision) {
    const auto [it, inserted] = snapshot.emplace(key, decision);
    if (!inserted &&
        (it->second.commit != decision.commit ||
         it->second.commit_timestamp_ns != decision.commit_timestamp_ns)) {
        throw std::runtime_error("conflicting decisions in Raft 2PC snapshot history");
    }
}

std::vector<uint8_t> encodeDecisionSnapshot(
    const std::string& group_id,
    uint64_t snapshot_index,
    uint64_t snapshot_term,
    const DecisionSnapshot& decisions) {
    nlohmann::json encoded_decisions = nlohmann::json::array();
    for (const auto& [key, decision] : decisions) {
        encoded_decisions.push_back({
            {"coordinator_id", key.first},
            {"transaction_id", key.second},
            {"decision", decision.commit ? "commit" : "abort"},
            {"commit_timestamp_ns", decision.commit_timestamp_ns
                ? nlohmann::json(*decision.commit_timestamp_ns)
                : nlohmann::json(nullptr)}
        });
    }

    const nlohmann::json payload = {
        {"schema", std::string(kDecisionSnapshotSchema)},
        {"raft_group_id", group_id},
        {"snapshot_index", snapshot_index},
        {"snapshot_term", snapshot_term},
        {"decisions", std::move(encoded_decisions)}
    };
    const auto text = payload.dump();
    if (text.size() > kMaxDecisionSnapshotBytes) {
        throw std::runtime_error("Raft 2PC decision snapshot exceeds the size limit");
    }
    return std::vector<uint8_t>(text.begin(), text.end());
}

DecisionSnapshot decodeDecisionSnapshot(
    const std::vector<uint8_t>& snapshot_data,
    const std::string& expected_group,
    uint64_t expected_index,
    uint64_t expected_term) {
    if (snapshot_data.empty() ||
        snapshot_data.size() > kMaxDecisionSnapshotBytes) {
        throw std::runtime_error("Raft 2PC decision snapshot has an invalid size");
    }

    try {
        const auto payload = nlohmann::json::parse(
            snapshot_data.begin(), snapshot_data.end());
        if (!payload.is_object() ||
            payload.at("schema").get<std::string>() != kDecisionSnapshotSchema ||
            payload.at("raft_group_id").get<std::string>() != expected_group ||
            payload.at("snapshot_index").get<uint64_t>() != expected_index ||
            payload.at("snapshot_term").get<uint64_t>() != expected_term ||
            !payload.at("decisions").is_array()) {
            throw std::runtime_error("snapshot identity or schema does not match");
        }

        DecisionSnapshot decisions;
        for (const auto& item : payload.at("decisions")) {
            if (!item.is_object()) {
                throw std::runtime_error("snapshot decision entry is not an object");
            }
            auto coordinator_id = item.at("coordinator_id").get<std::string>();
            auto transaction_id = item.at("transaction_id").get<std::string>();
            if (!validIdentity(coordinator_id) || !validIdentity(transaction_id)) {
                throw std::runtime_error("snapshot decision has an invalid identity");
            }

            const auto decision_name = item.at("decision").get<std::string>();
            if (decision_name != "commit" && decision_name != "abort") {
                throw std::runtime_error("snapshot decision value is invalid");
            }
            TwoPhaseCommitDecision decision;
            decision.commit = decision_name == "commit";
            const auto& timestamp = item.at("commit_timestamp_ns");
            if (!timestamp.is_null()) {
                if (!timestamp.is_number_integer()) {
                    throw std::runtime_error("snapshot commit timestamp is invalid");
                }
                decision.commit_timestamp_ns = timestamp.get<int64_t>();
            }
            validateDecision(decision);
            if (!decisions.emplace(
                    DecisionKey{std::move(coordinator_id),
                                std::move(transaction_id)},
                    decision).second) {
                throw std::runtime_error("duplicate transaction in decision snapshot");
            }
        }
        return decisions;
    } catch (const std::exception& error) {
        throw std::runtime_error(
            "corrupt Raft 2PC decision snapshot: " + std::string(error.what()));
    }
}

std::string encodeDecision(
    const std::string& group_id,
    const std::string& coordinator_id,
    const std::string& transaction_id,
    const TwoPhaseCommitDecision& decision) {
    const nlohmann::json payload = {
        {"schema", std::string(kDecisionSchema)},
        {"raft_group_id", group_id},
        {"coordinator_id", coordinator_id},
        {"transaction_id", transaction_id},
        {"decision", decision.commit ? "commit" : "abort"},
        {"commit_timestamp_ns", decision.commit_timestamp_ns
            ? nlohmann::json(*decision.commit_timestamp_ns)
            : nlohmann::json(nullptr)}
    };
    return std::string(kDecisionPrefix) + payload.dump();
}

} // namespace

struct RaftTwoPhaseCommitDecisionStore::State {
    State(std::shared_ptr<themisdb::sharding::RaftConsensus> consensus,
          std::string group_id)
        : raft(std::move(consensus)),
          raft_group_id(std::move(group_id)),
          operation_mutex(operationMutexFor(raft.get())) {}

    void requireAuthority() const {
        const auto& log = raft->getRaftState().getLog();
        if (!log.hasDurableJournal() || !log.isPersistenceHealthy()) {
            throw std::runtime_error(
                "2PC decision authority requires a healthy durable Raft journal");
        }
        if (log.getSnapshotIndex() != snapshot_index ||
            log.getSnapshotTerm() != snapshot_term) {
            throw std::runtime_error(
                "2PC decision authority snapshot state does not match the compacted Raft log");
        }
        if (!raft->isLeader() || !raft->hasQuorum()) {
            throw std::runtime_error(
                "2PC decision authority requires the current Raft leader and quorum");
        }
    }

    DecisionHistory scan(
        const std::string& coordinator_id,
        const std::string& transaction_id) const {
        const auto& log = raft->getRaftState().getLog();
        const auto committed_index = log.getCommitIndex();
        const auto last_index = log.getLastLogIndex();
        if (committed_index > last_index) {
            throw std::runtime_error("Raft commit index exceeds the local log tail");
        }

        DecisionHistory history;
        const auto snapshot_decision = snapshot_decisions.find(
            DecisionKey{coordinator_id, transaction_id});
        if (snapshot_decision != snapshot_decisions.end()) {
            history.committed = snapshot_decision->second;
        }
        if (snapshot_index > committed_index) {
            throw std::runtime_error("Raft snapshot index exceeds the commit index");
        }
        if (snapshot_index == std::numeric_limits<uint64_t>::max()) {
            return history;
        }
        uint64_t index = snapshot_index + 1;
        while (index <= last_index) {
            const auto entry = log.getEntry(index);
            if (!entry) {
                throw std::runtime_error(
                    "Raft decision log has a missing entry at index " +
                    std::to_string(index));
            }
            const auto decision = parseDecision(
                *entry, raft_group_id);
            if (decision &&
                decision->coordinator_id == coordinator_id &&
                decision->transaction_id == transaction_id) {
                if (index <= committed_index) {
                    mergeDecision(history.committed, decision->decision, history.conflict);
                } else {
                    mergeDecision(history.retained_uncommitted, decision->decision,
                                  history.conflict);
                }
            }
            if (index == last_index) {
                break;
            }
            ++index;
        }
        if (history.committed && history.retained_uncommitted &&
            (history.committed->commit != history.retained_uncommitted->commit ||
             history.committed->commit_timestamp_ns !=
                 history.retained_uncommitted->commit_timestamp_ns)) {
            history.conflict = true;
        }
        return history;
    }

    std::optional<TwoPhaseCommitDecision> lookup(
        const std::string& coordinator_id,
        const std::string& transaction_id) const {
        if (!validIdentity(coordinator_id) || !validIdentity(transaction_id)) {
            throw std::invalid_argument("2PC decision lookup identities must be non-empty and bounded");
        }

        std::lock_guard<std::mutex> lock(*operation_mutex);
        requireAuthority();
        const auto term = raft->getCurrentTerm();
        const auto history = scan(coordinator_id, transaction_id);
        requireAuthority();
        if (raft->getCurrentTerm() != term) {
            throw std::runtime_error(
                "Raft term changed while reading 2PC decision history");
        }
        if (history.conflict) {
            throw std::runtime_error("conflicting Raft 2PC decision history");
        }
        return history.committed;
    }

    bool write(
        const std::string& coordinator_id,
        const std::string& transaction_id,
        const TwoPhaseCommitDecision& decision) {
        if (!validIdentity(coordinator_id) || !validIdentity(transaction_id)) {
            throw std::invalid_argument("2PC decision identities must be non-empty and bounded");
        }
        validateDecision(decision);

        std::lock_guard<std::mutex> lock(*operation_mutex);
        requireAuthority();
        auto history = scan(coordinator_id, transaction_id);
        if (history.conflict) {
            return false;
        }
        if (history.committed) {
            return history.committed->commit == decision.commit &&
                   history.committed->commit_timestamp_ns == decision.commit_timestamp_ns;
        }
        if (history.retained_uncommitted) {
            return false;
        }

        const auto command = encodeDecision(
            raft_group_id, coordinator_id, transaction_id, decision);
        const auto index = raft->proposeWithIndex(command);
        if (!index) {
            return false;
        }

        requireAuthority();
        history = scan(coordinator_id, transaction_id);
        if (history.conflict || !history.committed ||
            history.committed->commit != decision.commit ||
            history.committed->commit_timestamp_ns != decision.commit_timestamp_ns) {
            return false;
        }
        const auto& log = raft->getRaftState().getLog();
        const auto entry = log.getEntry(*index);
        return log.getCommitIndex() >= *index &&
               entry && entry->command == command;
    }

    std::vector<uint8_t> captureSnapshotStateLocked(
        uint64_t requested_index,
        uint64_t requested_term) const {
        requireAuthority();
        const auto term = raft->getCurrentTerm();

        const auto& log = raft->getRaftState().getLog();
        const auto current_index = log.getSnapshotIndex();
        if (requested_index == 0 || requested_index < current_index ||
            requested_index > log.getCommitIndex()) {
            throw std::runtime_error(
                "2PC snapshot boundary must be committed and cannot regress");
        }
        if (requested_index == current_index) {
            if (requested_term != log.getSnapshotTerm()) {
                throw std::runtime_error("2PC snapshot term conflicts with the current anchor");
            }
        } else {
            const auto boundary = log.getEntry(requested_index);
            if (!boundary || boundary->term != requested_term) {
                throw std::runtime_error("2PC snapshot boundary does not match the Raft log");
            }
        }

        auto decisions = snapshot_decisions;
        if (current_index != std::numeric_limits<uint64_t>::max()) {
            uint64_t index = current_index + 1;
            while (index <= requested_index) {
                const auto entry = log.getEntry(index);
                if (!entry) {
                    throw std::runtime_error(
                        "Raft decision log has a missing entry at index " +
                        std::to_string(index));
                }
                if (const auto decision = parseDecision(*entry, raft_group_id)) {
                    mergeSnapshotDecision(
                        decisions,
                        DecisionKey{decision->coordinator_id,
                                    decision->transaction_id},
                        decision->decision);
                }
                if (index == requested_index) {
                    break;
                }
                ++index;
            }
        }
        requireAuthority();
        if (raft->getCurrentTerm() != term) {
            throw std::runtime_error(
                "Raft term changed while capturing 2PC decision snapshot state");
        }
        return encodeDecisionSnapshot(
            raft_group_id, requested_index, requested_term, decisions);
    }

    std::vector<uint8_t> captureSnapshotState(
        uint64_t requested_index,
        uint64_t requested_term) const {
        std::lock_guard<std::mutex> lock(*operation_mutex);
        return captureSnapshotStateLocked(requested_index, requested_term);
    }

    void restoreSnapshotStateLocked(
        uint64_t requested_index,
        uint64_t requested_term,
        const std::vector<uint8_t>& data) {
        if (requested_index == 0) {
            throw std::runtime_error("2PC decision snapshot index must be positive");
        }
        auto decisions = decodeDecisionSnapshot(
            data, raft_group_id, requested_index, requested_term);

        const auto& log = raft->getRaftState().getLog();
        if (!log.hasDurableJournal() || !log.isPersistenceHealthy() ||
            log.getSnapshotIndex() != requested_index ||
            log.getSnapshotTerm() != requested_term) {
            throw std::runtime_error(
                "2PC decision snapshot does not match a healthy installed Raft snapshot");
        }
        if (log.getCommitIndex() < requested_index) {
            throw std::runtime_error(
                "2PC decision snapshot boundary exceeds the committed Raft prefix");
        }
        if (requested_index < snapshot_index) {
            throw std::runtime_error("2PC decision snapshot restore cannot regress");
        }
        for (const auto& [key, decision] : snapshot_decisions) {
            const auto restored = decisions.find(key);
            if (restored == decisions.end() ||
                restored->second.commit != decision.commit ||
                restored->second.commit_timestamp_ns != decision.commit_timestamp_ns) {
                throw std::runtime_error(
                    "restored 2PC snapshot omits or conflicts with prior decision state");
            }
        }

        const auto last_index = log.getLastLogIndex();
        DecisionSnapshot retained_decisions;
        if (requested_index < last_index) {
            uint64_t index = requested_index + 1;
            while (index <= last_index) {
                const auto entry = log.getEntry(index);
                if (!entry) {
                    throw std::runtime_error(
                        "Raft decision log has a missing entry at index " +
                        std::to_string(index));
                }
                if (const auto decision = parseDecision(*entry, raft_group_id)) {
                    const DecisionKey key{
                        decision->coordinator_id, decision->transaction_id};
                    mergeSnapshotDecision(
                        retained_decisions, key, decision->decision);
                    const auto existing = decisions.find(key);
                    if (existing != decisions.end() &&
                        (existing->second.commit != decision->decision.commit ||
                         existing->second.commit_timestamp_ns !=
                             decision->decision.commit_timestamp_ns)) {
                        throw std::runtime_error(
                            "restored 2PC decision conflicts with retained Raft history");
                    }
                }
                if (index == last_index) {
                    break;
                }
                ++index;
            }
        }

        snapshot_decisions = std::move(decisions);
        snapshot_index = requested_index;
        snapshot_term = requested_term;
    }

    void restoreSnapshotState(
        uint64_t requested_index,
        uint64_t requested_term,
        const std::vector<uint8_t>& data) {
        std::lock_guard<std::mutex> lock(*operation_mutex);
        restoreSnapshotStateLocked(requested_index, requested_term, data);
    }

    bool createAndInstallSnapshot(
        themisdb::sharding::RaftSnapshotManager& snapshot_manager,
        uint64_t requested_index,
        uint64_t requested_term,
        const std::vector<uint8_t>& application_state) {
        std::lock_guard<std::mutex> lock(*operation_mutex);
        const auto decision_state =
            captureSnapshotStateLocked(requested_index, requested_term);
        themisdb::sharding::RaftSnapshotExtensions extensions;
        extensions.emplace(
            std::string(RaftTwoPhaseCommitDecisionStore::kSnapshotExtensionName),
            decision_state);
        if (!snapshot_manager.createAndInstall(
                raft->getRaftState(), requested_index, requested_term,
                application_state, extensions)) {
            return false;
        }
        restoreSnapshotStateLocked(
            requested_index, requested_term, decision_state);
        return true;
    }

    void restoreInstalledSnapshot(
        const themisdb::sharding::RaftSnapshotManager& snapshot_manager) {
        const auto& log = raft->getRaftState().getLog();
        const auto installed_index = log.getSnapshotIndex();
        if (installed_index == 0) {
            return;
        }

        const auto snapshot = snapshot_manager.loadSnapshot(installed_index);
        if (!snapshot ||
            snapshot->snapshot_index != installed_index ||
            snapshot->snapshot_term != log.getSnapshotTerm()) {
            throw std::runtime_error(
                "installed Raft snapshot is missing or does not match its journal anchor");
        }

        const auto extension = snapshot->extensions.find(
            std::string(RaftTwoPhaseCommitDecisionStore::kSnapshotExtensionName));
        if (extension == snapshot->extensions.end()) {
            throw std::runtime_error(
                "installed Raft snapshot is missing the 2PC decision-state extension");
        }

        restoreSnapshotState(
            installed_index, snapshot->snapshot_term, extension->second);
    }

    std::shared_ptr<themisdb::sharding::RaftConsensus> raft;
    std::string raft_group_id;
    std::shared_ptr<std::mutex> operation_mutex;
    DecisionSnapshot snapshot_decisions;
    uint64_t snapshot_index = 0;
    uint64_t snapshot_term = 0;
};

RaftTwoPhaseCommitDecisionStore::RaftTwoPhaseCommitDecisionStore(
    std::shared_ptr<themisdb::sharding::RaftConsensus> raft,
    std::string raft_group_id) {
    if (!raft || !validIdentity(raft_group_id)) {
        throw std::invalid_argument(
            "Raft 2PC decision store requires a Raft instance and bounded group ID");
    }
    state_ = std::make_shared<State>(std::move(raft), std::move(raft_group_id));
}

RaftTwoPhaseCommitDecisionStore::RaftTwoPhaseCommitDecisionStore(
    std::shared_ptr<themisdb::sharding::RaftConsensus> raft,
    std::string raft_group_id,
    const themisdb::sharding::RaftSnapshotManager& snapshot_manager)
    : RaftTwoPhaseCommitDecisionStore(
          std::move(raft), std::move(raft_group_id)) {
    state_->restoreInstalledSnapshot(snapshot_manager);
}

TwoPhaseCommitDecisionWriter RaftTwoPhaseCommitDecisionStore::writer() const {
    const auto state = state_;
    return [state](
        const std::string& coordinator_id,
        const std::string& transaction_id,
        const TwoPhaseCommitDecision& decision) {
        return state->write(coordinator_id, transaction_id, decision);
    };
}

TwoPhaseCommitDecisionLookup RaftTwoPhaseCommitDecisionStore::lookup() const {
    const auto state = state_;
    return [state](
        const std::string& coordinator_id,
        const std::string& transaction_id) {
        return state->lookup(coordinator_id, transaction_id);
    };
}

std::vector<uint8_t> RaftTwoPhaseCommitDecisionStore::captureSnapshotState(
    uint64_t snapshot_index,
    uint64_t snapshot_term) const {
    return state_->captureSnapshotState(snapshot_index, snapshot_term);
}

bool RaftTwoPhaseCommitDecisionStore::createAndInstallSnapshot(
    themisdb::sharding::RaftSnapshotManager& snapshot_manager,
    uint64_t snapshot_index,
    uint64_t snapshot_term,
    const std::vector<uint8_t>& application_state) const {
    return state_->createAndInstallSnapshot(
        snapshot_manager, snapshot_index, snapshot_term, application_state);
}

void RaftTwoPhaseCommitDecisionStore::restoreSnapshotState(
    uint64_t snapshot_index,
    uint64_t snapshot_term,
    const std::vector<uint8_t>& snapshot_data) {
    state_->restoreSnapshotState(snapshot_index, snapshot_term, snapshot_data);
}

} // namespace themis::sharding
