/**
 * @file raft_two_phase_commit_decision_store.cpp
 * @brief Durable Raft decision authority for the canonical 2PC coordinator.
 */
#include "sharding/raft_two_phase_commit_decision_store.h"

#include "sharding/raft_consensus.h"

#include <nlohmann/json.hpp>

#include <cstddef>
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
constexpr std::size_t kMaxIdentityBytes = 1024;

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

std::optional<TwoPhaseCommitDecision> parseDecision(
    const themisdb::sharding::LogEntry& entry,
    const std::string& expected_group,
    const std::string& expected_coordinator,
    const std::string& expected_transaction) {
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
        if (group_id != expected_group ||
            coordinator_id != expected_coordinator ||
            transaction_id != expected_transaction) {
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
        return decision;
    } catch (const std::exception& error) {
        throw std::runtime_error(
            "corrupt Raft 2PC decision at log index " +
            std::to_string(entry.index) + ": " + error.what());
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
        if (log.getSnapshotIndex() != 0) {
            throw std::runtime_error(
                "2PC decision authority cannot read compacted history without a decision snapshot");
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
        uint64_t index = 1;
        while (index <= last_index) {
            const auto entry = log.getEntry(index);
            if (!entry) {
                throw std::runtime_error(
                    "Raft decision log has a missing entry at index " +
                    std::to_string(index));
            }
            const auto decision = parseDecision(
                *entry, raft_group_id, coordinator_id, transaction_id);
            if (decision) {
                if (index <= committed_index) {
                    mergeDecision(history.committed, *decision, history.conflict);
                } else {
                    mergeDecision(history.retained_uncommitted, *decision, history.conflict);
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

    std::shared_ptr<themisdb::sharding::RaftConsensus> raft;
    std::string raft_group_id;
    std::shared_ptr<std::mutex> operation_mutex;
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

} // namespace themis::sharding
