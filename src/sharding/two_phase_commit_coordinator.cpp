/**
 * @file two_phase_commit_coordinator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.34
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// Two-Phase Commit (2PC) Coordinator – cross-shard transaction driver
//
// Canonical decision authority for direct 2PC and CrossShardTransactionCoordinator.
// WALLoggingHelper centralizes durable writes; the normalized existing
// commit_timestamp_ns metadata preserves CrossShard MVCC timestamps on replay.
// Older CrossShard TransactionWAL histories remain a separate, blocked recovery
// domain: this consolidation does not merge formats or invent migration decisions.
// The facade retains its public lifecycle and non-2PC protocols; other
// coordinators, participant WALs, and replication recovery remain separate.
// → Architecture reference: docs/architecture/transaction_coordinators.md

#include "sharding/two_phase_commit_coordinator.h"
#include "sharding/detail/two_phase_commit_contract.h"
#include "sharding/shard_rpc_client_adapter.h"
#include "sharding/metrics_registry.h"
#include "sharding/prometheus_metrics.h"
#include "sharding/wal_logging_helper.h"
#include "transaction/two_phase_commit_wal_recovery.h"
#include "utils/logger.h"
#include <algorithm>
#include <chrono>
#include <stdexcept>

namespace themis::sharding {

namespace {
bool isTestEndpoint(const std::string& endpoint) {
    return endpoint.rfind("inproc://", 0) == 0 ||
           endpoint.rfind("loopback://", 0) == 0;
}

bool validEndpoint(const std::string& endpoint) {
    if (endpoint.empty() || endpoint.find_first_of(" \t\r\n") != std::string::npos) {
        return false;
    }
    if (isTestEndpoint(endpoint)) {
        return endpoint.find("://") + 3 < endpoint.size();
    }
    const auto colon = endpoint.rfind(':');
    if (colon == std::string::npos || colon == 0 || colon + 1 == endpoint.size()) {
        return false;
    }
    const auto port = endpoint.substr(colon + 1);
    if (!std::all_of(port.begin(), port.end(), [](unsigned char ch) {
            return ch >= '0' && ch <= '9';
        }) || port.size() > 5) {
        return false;
    }
    const auto value = std::stoul(port);
    return value > 0 && value <= 65535;
}

void validateCoordinatorEntry(
    const themis::transaction::TwoPhaseCommitWALRecovery::RecoveredTransactions& history,
    const WALEntry& entry) {
    if (entry.type == WALEntryType::CHECKPOINT) {
        return;
    }
    if (entry.transaction_id.empty() || !entry.data.is_object()) {
        throw std::runtime_error("invalid canonical coordinator WAL transaction metadata");
    }
    const auto previous = history.find(entry.transaction_id);
    const bool has_decision = previous != history.end() && previous->second.has_decision;
    const bool terminal = entry.type == WALEntryType::COMMIT_TX ||
                          entry.type == WALEntryType::ABORT_TX;
    const bool commit = entry.type == WALEntryType::COMMIT_TX;
    if (has_decision &&
        ((terminal && previous->second.decision_commit != commit) ||
         entry.type == WALEntryType::PREPARE_TX ||
         entry.type == WALEntryType::BEGIN_TX)) {
        throw std::runtime_error("canonical coordinator WAL contradicts an immutable decision");
    }
    if (terminal && entry.data.contains("phase")) {
        const auto& phase = entry.data.at("phase");
        if (!phase.is_string() || (phase != "decision" && phase != "complete")) {
            throw std::runtime_error("invalid canonical coordinator WAL phase");
        }
        if (phase == "complete" && !has_decision) {
            throw std::runtime_error("canonical coordinator completion has no durable decision");
        }
    }
    if (terminal && entry.data.contains("decision") &&
        entry.data.at("decision") != (commit ? "commit" : "abort")) {
        throw std::runtime_error("canonical coordinator WAL decision does not match record type");
    }
    if (terminal && entry.data.contains("commit_timestamp_ns")) {
        const auto& timestamp = entry.data.at("commit_timestamp_ns");
        if (!commit || !timestamp.is_number_integer() || timestamp.get<int64_t>() <= 0 ||
            (has_decision && previous->second.commit_timestamp_ns &&
             *previous->second.commit_timestamp_ns != timestamp.get<int64_t>())) {
            throw std::runtime_error("invalid or contradictory canonical commit timestamp");
        }
    }
}

class TimestampedEndpointParticipant final : public ShardRPCServer::RequestHandler {
public:
    TimestampedEndpointParticipant(
        std::unique_ptr<ShardRPCServer::RequestHandler> delegate,
        const ShardRPCClient::Config& rpc,
        const ShardRPCClient::InProcessResponseHandler& handler,
        std::function<std::optional<int64_t>(const std::string&)> timestamp,
        bool require_timestamp)
        : delegate_(std::move(delegate)), client_(rpc), timestamp_(std::move(timestamp)),
          require_timestamp_(require_timestamp) {
        if (isTestEndpoint(rpc.endpoint) && handler) {
            client_.setInProcessResponseHandler(handler);
        }
    }

    bool onPrepare(const std::string& id, const std::string& coordinator,
                   const std::string& payload) override {
        return delegate_->onPrepare(id, coordinator, payload);
    }
    bool onCommit(const std::string& id) override {
        const auto timestamp = timestamp_(id);
        if (timestamp) {
            return *timestamp > 0 && client_.commit(id, *timestamp);
        }
        return !require_timestamp_ && delegate_->onCommit(id);
    }
    bool onAbort(const std::string& id) override { return delegate_->onAbort(id); }
    ShardRPCServer::HealthInfo onHealthCheck() override { return delegate_->onHealthCheck(); }

private:
    std::unique_ptr<ShardRPCServer::RequestHandler> delegate_;
    ShardRPCClient client_;
    std::function<std::optional<int64_t>(const std::string&)> timestamp_;
    bool require_timestamp_;
};

class PhaseTimeoutParticipantAdapter final : public ShardRPCServer::RequestHandler {
public:
    PhaseTimeoutParticipantAdapter(
        const ShardRPCClient::Config& base_config,
        std::chrono::milliseconds prepare_timeout,
        std::chrono::milliseconds commit_timeout,
        std::chrono::milliseconds abort_timeout,
        const ShardRPCClient::InProcessResponseHandler& handler)
        : prepare_(makeAdapter(withTimeout(base_config, prepare_timeout), handler))
        , commit_(makeAdapter(withTimeout(base_config, commit_timeout), handler))
        , abort_(makeAdapter(withTimeout(base_config, abort_timeout), handler))
    {}

    bool onPrepare(
        const std::string& transaction_id,
        const std::string& coordinator_id,
        const std::string& transaction_data) override {
        return prepare_->onPrepare(transaction_id, coordinator_id, transaction_data);
    }

    bool onCommit(const std::string& transaction_id) override {
        return commit_->onCommit(transaction_id);
    }

    bool onAbort(const std::string& transaction_id) override {
        return abort_->onAbort(transaction_id);
    }

    ShardRPCServer::HealthInfo onHealthCheck() override {
        return prepare_->onHealthCheck();
    }

private:
    static ShardRPCClient::Config withTimeout(
        ShardRPCClient::Config config,
        std::chrono::milliseconds timeout) {
        config.timeout_ms = static_cast<int>(timeout.count());
        return config;
    }

    static std::unique_ptr<ShardRPCClientAdapter> makeAdapter(
        const ShardRPCClient::Config& config,
        const ShardRPCClient::InProcessResponseHandler& handler) {
        if (handler &&
            (config.endpoint.rfind("inproc://", 0) == 0 ||
             config.endpoint.rfind("loopback://", 0) == 0)) {
            return std::make_unique<ShardRPCClientAdapter>(config, handler);
        }
        return std::make_unique<ShardRPCClientAdapter>(config);
    }

    std::unique_ptr<ShardRPCClientAdapter> prepare_;
    std::unique_ptr<ShardRPCClientAdapter> commit_;
    std::unique_ptr<ShardRPCClientAdapter> abort_;
};

}  // namespace

// ============================================================================
// Sprint 8 Phase 1: Use-After-Move Safety (GAP B-1/B-2)
// ============================================================================
// Coordinator adapters are moved to owned_adapters_ map for lifetime management,
// but raw pointers are stored in participants_ for O(1) lookup during 2PC operations.
// This pattern ensures coordinator state survives moves through pipeline stages.
// Pattern: Move unique_ptr to storage, access via raw pointer; safe reference semantics.

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

TwoPhaseCommitCoordinator::TwoPhaseCommitCoordinator(
    const std::string& coordinator_id
)
    : TwoPhaseCommitCoordinator(coordinator_id, Config{})
{}

TwoPhaseCommitCoordinator::TwoPhaseCommitCoordinator(
    const std::string& coordinator_id,
    const Config&      config
)
    : coordinator_id_(coordinator_id)
    , config_(config)
{
    if (!config_.wal_directory.empty()) {
        WALManagerConfig wal_cfg;
        wal_cfg.wal_directory = config_.wal_directory;
        wal_cfg.sync_on_write = config_.sync_wal_writes;
        wal_cfg.recovery_callback = [this](const WALEntry& entry) {
            switch (entry.type) {
            case WALEntryType::BEGIN_TX:
            case WALEntryType::PREPARE_TX:
            case WALEntryType::COMMIT_TX:
            case WALEntryType::ABORT_TX:
            case WALEntryType::CHECKPOINT:
                break;
            default:
                return false;
            }
            validateCoordinatorEntry(recovered_wal_transactions_, entry);
            themis::transaction::TwoPhaseCommitWALRecovery::applyEntry(
                recovered_wal_transactions_, entry);
            return true;
        };
        wal_ = std::make_unique<WALManager>(wal_cfg);
        for (const auto& [id, replay] : recovered_wal_transactions_) {
            CoordinatorTxnRecord rec;
            rec.transaction_id = id;
            rec.participant_shards = replay.participants;
            rec.decision_commit = replay.has_decision && replay.decision_commit;
            rec.commit_timestamp_ns = replay.commit_timestamp_ns;
            rec.state = (replay.participants_invalid || replay.participants.empty())
                ? CoordinatorTxnState::FAILED
                : replay.completed ? CoordinatorTxnState::COMPLETED
                : (replay.has_decision
                    ? (replay.decision_commit ? CoordinatorTxnState::COMMIT_DECIDED
                                              : CoordinatorTxnState::ABORT_DECIDED)
                    : CoordinatorTxnState::ACTIVE);
            transactions_.emplace(id, std::move(rec));
        }
        recovery_status_ = std::any_of(
            transactions_.begin(), transactions_.end(), [](const auto& item) {
                return item.second.state != CoordinatorTxnState::COMPLETED;
            }) ? "blocked" : "complete";
        THEMIS_INFO("2PC coordinator [{}] WAL initialised at {}",
                    coordinator_id_, config_.wal_directory);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Participant management
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Register Participant.
 * @param[in] shard_id Identifier of the shard.
 * @param[in,out] participant Input/output parameter.
 * @throws std::invalid_argument if an error occurs.
 * @details Calls: lock(), THEMIS_DEBUG().
 */
void TwoPhaseCommitCoordinator::registerParticipant(
    const std::string&              shard_id,
    ShardRPCServer::RequestHandler* participant
) {
    if (shard_id.empty() || !participant) {
        throw std::invalid_argument("shard ID and participant must not be empty");
    }
    std::lock_guard<std::timed_mutex> lock(mutex_);
    const auto existing = participants_.find(shard_id);
    if (existing != participants_.end() && existing->second == participant) {
        return;
    }
    if (!active_drivers_.empty() || recovery_status_ == "running") {
        throw std::logic_error("cannot replace participants while protocol RPCs are active");
    }
    if (existing != participants_.end() &&
        hasUnresolvedParticipantLocked(shard_id)) {
        throw std::logic_error(
            "cannot replace participant while a transaction for shard " +
            shard_id + " is unresolved");
    }
    owned_adapters_.erase(shard_id);
    participant_endpoints_.erase(shard_id);
    participants_[shard_id] = participant;
    THEMIS_DEBUG("2PC coordinator [{}] registered participant shard {}",
                 coordinator_id_, shard_id);
}

/**
 * @brief Unregister Participant.
 * @param[in] shard_id Identifier of the shard.
 * @return True when the operation succeeds.
 * @details Calls: lock(), erase().
 */
bool TwoPhaseCommitCoordinator::unregisterParticipant(const std::string& shard_id) {
    std::lock_guard<std::timed_mutex> lock(mutex_);
    if (!active_drivers_.empty() || recovery_status_ == "running") {
        return false;
    }
    if (hasUnresolvedParticipantLocked(shard_id)) {
        THEMIS_WARN("2PC coordinator [{}] cannot unregister shard {} while a transaction is unresolved",
                    coordinator_id_, shard_id);
        return false;
    }
    owned_adapters_.erase(shard_id); // also remove any owned adapter
    participant_endpoints_.erase(shard_id);
    return participants_.erase(shard_id) > 0;
}

/**
 * @brief Register Participant By Endpoint.
 * @param[in] shard_id Identifier of the shard.
 * @param[in] rpc_config Input parameter.
 * @details Calls: lock(), get(), std::move(), THEMIS_DEBUG().
 */
void TwoPhaseCommitCoordinator::registerParticipantByEndpoint(
    const std::string&            shard_id,
    const ShardRPCClient::Config& rpc_config
) {
    if (shard_id.empty() || !validEndpoint(rpc_config.endpoint) ||
        (isTestEndpoint(rpc_config.endpoint) && !config_.in_process_response_handler)) {
        throw std::invalid_argument("invalid shard endpoint or missing explicit test handler");
    }
    std::lock_guard<std::timed_mutex> lock(mutex_);
    if (!active_drivers_.empty() || recovery_status_ == "running") {
        throw std::logic_error("cannot replace participants while protocol RPCs are active");
    }
    const auto existing_endpoint = participant_endpoints_.find(shard_id);
    if (hasUnresolvedParticipantLocked(shard_id)) {
        if (existing_endpoint != participant_endpoints_.end() &&
            existing_endpoint->second == rpc_config.endpoint) {
            return;
        }
        throw std::logic_error(
            "cannot replace participant endpoint while a transaction for shard " +
            shard_id + " is unresolved");
    }
    auto adapter = makeEndpointParticipant(rpc_config);
    participants_[shard_id] = adapter.get();  // Sprint 8: Store raw pointer for O(1) lookup
    // Sprint 8 Phase 1 (GAP B-1/B-2): Unique_ptr moved to owned_adapters_ for lifetime management.
    // Raw pointer stored in participants_ for transaction phase access. Pattern: Move for storage,
    // access via pointer; never access moved unique_ptr.
    owned_adapters_[shard_id] = std::move(adapter);
    participant_endpoints_[shard_id] = rpc_config.endpoint;
    THEMIS_DEBUG("2PC coordinator [{}] registered remote participant shard {} at {}",
                 coordinator_id_, shard_id, rpc_config.endpoint);
}

bool TwoPhaseCommitCoordinator::hasUnresolvedParticipantLocked(
    const std::string& shard_id) const {
    return std::any_of(
        transactions_.begin(), transactions_.end(),
        [&shard_id](const auto& item) {
            const auto& record = item.second;
            return record.state != CoordinatorTxnState::COMPLETED &&
                std::find(record.participant_shards.begin(),
                          record.participant_shards.end(),
                          shard_id) != record.participant_shards.end();
        });
}

size_t TwoPhaseCommitCoordinator::participantCount() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::timed_mutex> lock(mutex_);
    return participants_.size();
}

std::unique_ptr<ShardRPCServer::RequestHandler>
TwoPhaseCommitCoordinator::makeEndpointParticipant(const ShardRPCClient::Config& rpc) {
    const auto commit_timeout = config_.commit_timeout.value_or(config_.prepare_timeout);
    const auto abort_timeout = config_.abort_timeout.value_or(config_.prepare_timeout);
    const bool phase_timeouts_differ =
        commit_timeout != config_.prepare_timeout ||
        abort_timeout != config_.prepare_timeout;
    std::unique_ptr<ShardRPCServer::RequestHandler> adapter;
    if (phase_timeouts_differ) {
        adapter = std::make_unique<PhaseTimeoutParticipantAdapter>(
            rpc, config_.prepare_timeout, commit_timeout, abort_timeout,
            config_.in_process_response_handler);
    } else if (isTestEndpoint(rpc.endpoint)) {
        adapter = std::make_unique<ShardRPCClientAdapter>(
            rpc, config_.in_process_response_handler);
    } else {
        adapter = std::make_unique<ShardRPCClientAdapter>(rpc);
    }
    auto timestamp_rpc = rpc;
    timestamp_rpc.timeout_ms = static_cast<int>(commit_timeout.count());
    return std::make_unique<TimestampedEndpointParticipant>(
        std::move(adapter), timestamp_rpc, config_.in_process_response_handler,
        [this](const std::string& id) -> std::optional<int64_t> {
            std::lock_guard<std::timed_mutex> lock(mutex_);
            const auto found = transactions_.find(id);
            return found == transactions_.end() ? std::nullopt
                                                : found->second.commit_timestamp_ns;
        }, static_cast<bool>(config_.commit_timestamp_provider));
}

// ─────────────────────────────────────────────────────────────────────────────
// Core 2PC protocol
// ─────────────────────────────────────────────────────────────────────────────

CoordinatorTxnOutcome TwoPhaseCommitCoordinator::commit(
    const std::string& transaction_id,
    const std::map<std::string, nlohmann::json>& ops_per_shard
) {
    return execute(transaction_id, ops_per_shard, false, false);
}

bool TwoPhaseCommitCoordinator::prepare(
    const std::string& transaction_id,
    const std::map<std::string, nlohmann::json>& ops_per_shard
) {
    const auto outcome = execute(transaction_id, ops_per_shard, true, false);
    return outcome.committed();
}

CoordinatorTxnOutcome TwoPhaseCommitCoordinator::abort(
    const std::string& transaction_id,
    const std::vector<std::string>& participant_shards
) {
    std::map<std::string, nlohmann::json> operations;
    for (const auto& shard : participant_shards) {
        if (shard.empty() || !operations.emplace(shard, nlohmann::json::array()).second) {
            return {CoordinatorTxnResult::FAILURE, transaction_id,
                    "empty or duplicate participant ID"};
        }
    }
    return execute(transaction_id, operations, false, true);
}

bool TwoPhaseCommitCoordinator::isReady() const {
    std::lock_guard<std::timed_mutex> lock(mutex_);
    return wal_ && !recovery_failed_ &&
        (recovery_status_ == "complete" || recovery_status_ == "not_run");
}

std::optional<bool> TwoPhaseCommitCoordinator::getTransactionDecision(
    const std::string& transaction_id) const {
    std::lock_guard<std::timed_mutex> lock(mutex_);
    const auto it = transactions_.find(transaction_id);
    if (it == transactions_.end() ||
        (it->second.state != CoordinatorTxnState::COMMIT_DECIDED &&
         it->second.state != CoordinatorTxnState::ABORT_DECIDED &&
         it->second.state != CoordinatorTxnState::COMPLETED)) {
        return std::nullopt;
    }
    return it->second.decision_commit;
}

std::optional<CoordinatorTxnRecord> TwoPhaseCommitCoordinator::getTransactionRecord(
    const std::string& transaction_id) const {
    std::lock_guard<std::timed_mutex> lock(mutex_);
    const auto found = transactions_.find(transaction_id);
    return found == transactions_.end() ? std::nullopt
                                       : std::optional<CoordinatorTxnRecord>(found->second);
}

CoordinatorTxnOutcome TwoPhaseCommitCoordinator::execute(
    const std::string&                           transaction_id,
    const std::map<std::string, nlohmann::json>& ops_per_shard,
    bool prepare_only, bool abort_only
) {
    const auto t0 = std::chrono::steady_clock::now();

    CoordinatorTxnOutcome outcome;
    outcome.transaction_id = transaction_id;

    if (!wal_ || transaction_id.empty() || ops_per_shard.empty()) {
        outcome.result = CoordinatorTxnResult::FAILURE;
        outcome.reason = !wal_ ? "durable coordinator WAL is required"
                              : "transaction ID and participant set must not be empty";
        THEMIS_WARN("2PC coordinator [{}] txn {} – no shards, aborting",
                    coordinator_id_, transaction_id);
        return outcome;
    }

    bool resume_prepared = false;
    {
        std::lock_guard<std::timed_mutex> lock(mutex_);
        if (recovery_failed_ || recovery_status_ == "running" ||
            recovery_status_ == "blocked" ||
            (wal_ && recovery_status_ != "complete")) {
            outcome.result = CoordinatorTxnResult::FAILURE;
            outcome.reason = recovery_failed_
                ? "coordinator WAL recovery has failed"
                : (recovery_status_ == "running"
                    ? "coordinator WAL recovery is in progress"
                    : (recovery_status_ == "blocked"
                        ? "coordinator WAL recovery has unresolved transactions"
                        : "coordinator WAL recovery has not completed"));
            return outcome;
        }
        const auto it = transactions_.find(transaction_id);
        if (it != transactions_.end()) {
            if (it->second.state == CoordinatorTxnState::COMPLETED) {
                outcome.result = it->second.decision_commit
                    ? CoordinatorTxnResult::COMMITTED
                    : CoordinatorTxnResult::ABORTED;
                return outcome;
            }
            if (it->second.state != CoordinatorTxnState::PREPARED) {
                outcome.reason = "transaction already in progress; recovery required";
                return outcome;
            }
            std::vector<std::string> requested;
            for (const auto& [shard, _] : ops_per_shard) {
                requested.push_back(shard);
            }
            if (requested != it->second.participant_shards) {
                outcome.reason = "participant set differs from prepared transaction";
                return outcome;
            }
            if (!abort_only) {
                for (const auto& [shard, ops] : ops_per_shard) {
                    if (nlohmann::json::parse(it->second.shard_payloads.at(shard))
                            .at("operations") != ops) {
                        outcome.reason = "operations differ from the prepared transaction";
                        return outcome;
                    }
                }
            }
            resume_prepared = true;
        }
        if (!active_drivers_.insert(transaction_id).second) {
            outcome.reason = "transaction already has an active protocol driver";
            return outcome;
        }
    }
    const auto driver_guard = std::unique_ptr<void, std::function<void(void*)>>(
        reinterpret_cast<void*>(1), [this, &transaction_id](void*) {
            std::lock_guard<std::timed_mutex> lock(mutex_);
            active_drivers_.erase(transaction_id);
        });

    if (!abort_only && !resume_prepared && !config_.fencing_token_provider) {
        outcome.result = CoordinatorTxnResult::FAILURE;
        outcome.reason = "fencing token provider not configured";
        ++total_errors_;
        return outcome;
    }

    std::map<std::string, EpochToken> fencing_tokens;
    if (!abort_only && !resume_prepared) {
    for (const auto& [shard_id, _] : ops_per_shard) {
        try {
            auto token = config_.fencing_token_provider(shard_id);
            if (!token || token->epoch == 0 || token->issuer != coordinator_id_ ||
                token->shard_id != shard_id) {
                outcome.result = CoordinatorTxnResult::FAILURE;
                outcome.reason = "invalid fencing token for shard: " + shard_id;
                ++total_errors_;
                return outcome;
            }
            fencing_tokens.emplace(shard_id, std::move(*token));
        } catch (const std::exception& e) {
            outcome.result = CoordinatorTxnResult::FAILURE;
            outcome.reason = "fencing token provider failed for shard " + shard_id +
                             ": " + e.what();
            ++total_errors_;
            return outcome;
        }
    }
    }

    // ── Build the record ────────────────────────────────────────────────────
    if (!resume_prepared) {
    CoordinatorTxnRecord rec;
    rec.transaction_id = transaction_id;
    rec.state          = CoordinatorTxnState::ACTIVE;
    rec.started_at     = std::chrono::steady_clock::now();
    nlohmann::json participant_endpoints = nlohmann::json::object();

    {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::timed_mutex> lock(mutex_);

        if (recovery_failed_ || recovery_status_ == "running" ||
            recovery_status_ == "blocked" ||
            (wal_ && recovery_status_ != "complete")) {
            outcome.result = CoordinatorTxnResult::FAILURE;
            outcome.reason = recovery_failed_
                ? "coordinator WAL recovery has failed"
                : (recovery_status_ == "running"
                    ? "coordinator WAL recovery is in progress"
                    : (recovery_status_ == "blocked"
                        ? "coordinator WAL recovery has unresolved transactions"
                        : "coordinator WAL recovery has not completed"));
            return outcome;
        }

        // Detect duplicate transaction IDs
        if (transactions_.count(transaction_id)) {
            auto& existing = transactions_.at(transaction_id);
            if (existing.state == CoordinatorTxnState::COMPLETED) {
                THEMIS_DEBUG("2PC coordinator [{}] duplicate commit for completed txn {} – "
                             "returning its recorded decision",
                             coordinator_id_, transaction_id);
                outcome.result = existing.decision_commit
                    ? CoordinatorTxnResult::COMMITTED
                    : CoordinatorTxnResult::ABORTED;
                return outcome;
            }
            outcome.result = CoordinatorTxnResult::FAILURE;
            outcome.reason = "transaction already in progress";
            return outcome;
        }

        // Validate all shards are registered
        for (const auto& [shard_id, _] : ops_per_shard) {
            if (participants_.find(shard_id) == participants_.end()) {
                outcome.result = CoordinatorTxnResult::FAILURE;
                outcome.reason = "unknown shard: " + shard_id;
                THEMIS_ERROR("2PC coordinator [{}] txn {} – unknown shard {}",
                             coordinator_id_, transaction_id, shard_id);
                return outcome;
            }
        }

        // Build per-shard payloads
        for (const auto& [shard_id, ops] : ops_per_shard) {
            if (shard_id.empty() || !ops.is_array()) {
                outcome.reason = "shard IDs must not be empty and operations must be arrays";
                return outcome;
            }
            rec.participant_shards.push_back(shard_id);
            if (!abort_only) {
                rec.shard_payloads[shard_id] = buildPayload(ops, fencing_tokens.at(shard_id));
            }
            const auto endpoint_it = participant_endpoints_.find(shard_id);
            if (endpoint_it != participant_endpoints_.end()) {
                participant_endpoints[shard_id] = endpoint_it->second;
                rec.participant_endpoints[shard_id] = endpoint_it->second;
            }
        }

        transactions_[transaction_id] = rec;
    }

    // Persist BEGIN_TX
    if (!logToWAL(WALEntryType::BEGIN_TX, transaction_id, {
        {"transaction_id", transaction_id},
        {"coordinator_id", coordinator_id_},
        {"shards",         [&]() {
            std::vector<std::string> v = {};

            for (auto& [s, _] : ops_per_shard) {
              v.push_back(s);
            }
            return v;
        }()},
        {"participant_endpoints", participant_endpoints}
    }, /*force_sync=*/true)) {
        std::lock_guard<std::timed_mutex> lock(mutex_);
        transactions_.at(transaction_id).state = CoordinatorTxnState::FAILED;
        recovery_failed_ = true;
        recovery_status_ = "failed";
        total_errors_.fetch_add(1, std::memory_order_relaxed);
        outcome.result = CoordinatorTxnResult::FAILURE;
        outcome.reason = "failed to durably persist transaction BEGIN; no participants were prepared";
        return outcome;
    }

    total_transactions_.fetch_add(1, std::memory_order_relaxed);
    }

    if (prepare_only && resume_prepared) {
        outcome.result = CoordinatorTxnResult::COMMITTED;
        return outcome;
    }

    bool prepared_here = false;
    if (prepare_only) {
        std::unique_lock<std::timed_mutex> lock(mutex_);
        auto& stored = transactions_.at(transaction_id);
        stored.state = CoordinatorTxnState::PREPARING;
        prepared_here = runPhase1(stored, lock);
        if (prepared_here) {
            stored.state = CoordinatorTxnState::PREPARED;
            outcome.result = CoordinatorTxnResult::COMMITTED;
            return outcome;
        }
    }

    // Both sharding coordinator APIs use this shared phase-ordering and
    // decision/failure contract. Participant transport and WAL encoding remain
    // coordinator-specific.
    const auto t1 = std::chrono::steady_clock::now();
    bool all_prepared = false;
    bool decision_recorded = false;
    auto prepare_finished = t1;
    auto phase2_started = t1;
    const auto execution = detail::executeTwoPhaseCommit(
        [this, &transaction_id, &all_prepared, &prepare_finished,
         resume_prepared, prepare_only, abort_only]() {
            std::unique_lock<std::timed_mutex> lock(mutex_);
            auto& stored = transactions_.at(transaction_id);
            stored.state = CoordinatorTxnState::PREPARING;
            all_prepared = !abort_only && !prepare_only &&
                (resume_prepared || runPhase1(stored, lock));
            prepare_finished = std::chrono::steady_clock::now();
            return all_prepared;
        },
        [this, &transaction_id, &phase2_started, &decision_recorded](bool do_commit) {
            nlohmann::json decision_data = {
                {"transaction_id", transaction_id},
                {"coordinator_id", coordinator_id_},
                {"phase", "decision"},
                {"decision", do_commit ? "commit" : "abort"}
            };
            if (do_commit && config_.commit_timestamp_provider) {
                const auto timestamp = config_.commit_timestamp_provider(transaction_id);
                if (!timestamp || *timestamp <= 0) {
                    return false;
                }
                decision_data["commit_timestamp_ns"] = *timestamp;
                std::lock_guard<std::timed_mutex> lock(mutex_);
                transactions_.at(transaction_id).commit_timestamp_ns = timestamp;
            }
            const bool persisted = logToWAL(
                do_commit ? WALEntryType::COMMIT_TX : WALEntryType::ABORT_TX,
                transaction_id, decision_data, /*force_sync=*/true);
            phase2_started = std::chrono::steady_clock::now();
            if (!persisted) {
                THEMIS_ERROR("2PC coordinator [{}] could not persist decision for txn {}",
                            coordinator_id_, transaction_id);
            }
            decision_recorded = persisted;
            if (persisted) {
                std::lock_guard<std::timed_mutex> lock(mutex_);
                auto& stored = transactions_.at(transaction_id);
                stored.state = do_commit ? CoordinatorTxnState::COMMIT_DECIDED
                                         : CoordinatorTxnState::ABORT_DECIDED;
                stored.decision_commit = do_commit;
            }
            return persisted;
        },
        [this, &transaction_id](bool do_commit) {
            std::unique_lock<std::timed_mutex> lock(mutex_);
            auto& stored = transactions_.at(transaction_id);
            return runPhase2(stored, do_commit, lock);
        });
    const bool phase2_completed =
        execution != detail::TwoPhaseCommitExecutionResult::EXECUTION_FAILED;

    const double prepare_ms = std::chrono::duration<double, std::milli>(
        prepare_finished - t1).count();
    const double phase2_ms = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - phase2_started).count();
    if (auto m = ShardingMetricsRegistry::instance().getMetrics()) {
        m->record2PCPreparePhase(coordinator_id_, prepare_ms, all_prepared);
    }

    if (execution == detail::TwoPhaseCommitExecutionResult::EXECUTION_FAILED) {
        std::lock_guard<std::timed_mutex> lock(mutex_);
        auto it = transactions_.find(transaction_id);
        if (it != transactions_.end() && it->second.state != CoordinatorTxnState::COMPLETED) {
            // A successful decision-record callback with incomplete phase two
            // remains in-doubt so getRecoverableTransactions() exposes the
            // selected decision. Crash durability depends on WAL configuration.
            it->second.state = decision_recorded
                ? (all_prepared
                    ? CoordinatorTxnState::COMMIT_DECIDED
                    : CoordinatorTxnState::ABORT_DECIDED)
                : CoordinatorTxnState::FAILED;
        }
        recovery_status_ = "blocked";
        outcome.result = CoordinatorTxnResult::FAILURE;
        outcome.reason = "2PC decision persistence or phase-two acknowledgement failed";
        total_errors_.fetch_add(1, std::memory_order_relaxed);
        return outcome;
    }

    if (!logToWAL(all_prepared ? WALEntryType::COMMIT_TX : WALEntryType::ABORT_TX,
                 transaction_id, {
                     {"transaction_id", transaction_id},
                     {"coordinator_id", coordinator_id_},
                     {"phase", "complete"}
                 }, /*force_sync=*/true)) {
        {
            std::lock_guard<std::timed_mutex> lock(mutex_);
            recovery_status_ = "blocked";
        }
        outcome.result = CoordinatorTxnResult::FAILURE;
        outcome.reason = "failed to persist 2PC completion; transaction remains recoverable";
        total_errors_.fetch_add(1, std::memory_order_relaxed);
        return outcome;
    }

    {
        std::lock_guard<std::timed_mutex> lock(mutex_);
        transactions_.at(transaction_id).state = CoordinatorTxnState::COMPLETED;
    }

    if (all_prepared && phase2_completed) {
        total_commits_.fetch_add(1, std::memory_order_relaxed);
        outcome.result = CoordinatorTxnResult::COMMITTED;
        THEMIS_INFO("2PC coordinator [{}] txn {} COMMITTED", coordinator_id_, transaction_id);
    } else if (!all_prepared && phase2_completed) {
        total_aborts_.fetch_add(1, std::memory_order_relaxed);
        outcome.result = CoordinatorTxnResult::ABORTED;
        outcome.reason = "one or more participants voted ABORT";
        THEMIS_INFO("2PC coordinator [{}] txn {} ABORTED", coordinator_id_, transaction_id);
    } else {
        total_errors_.fetch_add(1, std::memory_order_relaxed);
        outcome.result = CoordinatorTxnResult::FAILURE;
        outcome.reason = "one or more participants did not acknowledge phase 2";
    }

    [[maybe_unused]] const double total_ms = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - t0).count();

    if (auto m = ShardingMetricsRegistry::instance().getMetrics()) {
        const bool completed_commit = all_prepared && phase2_completed;
        m->record2PCCommitPhase(coordinator_id_, phase2_ms, completed_commit);
        m->record2PCTransaction(coordinator_id_, completed_commit);
        if (!completed_commit) {
            m->record2PCAbort(coordinator_id_, outcome.reason);
        }
        // available for future histogram
    }

    return outcome;
}

TwoPhaseCommitCoordinator::AdapterPrepareResult
TwoPhaseCommitCoordinator::prepareForAdapter(
    const std::string& transaction_id,
    const std::map<std::string, nlohmann::json>& ops_per_shard) {
    if (!wal_ || transaction_id.empty() || ops_per_shard.empty() ||
        !config_.fencing_token_provider) {
        return AdapterPrepareResult::FAILED;
    }

    {
        std::lock_guard<std::timed_mutex> lock(mutex_);
        const auto existing = transactions_.find(transaction_id);
        if (existing != transactions_.end()) {
            if (existing->second.state == CoordinatorTxnState::PREPARED) {
                return AdapterPrepareResult::PREPARED;
            }
            if (existing->second.state == CoordinatorTxnState::COMPLETED) {
                return existing->second.decision_commit
                    ? AdapterPrepareResult::FAILED
                    : AdapterPrepareResult::ABORTED;
            }
            return AdapterPrepareResult::FAILED;
        }
        if (!wal_ || recovery_failed_ || recovery_status_ == "running" ||
            recovery_status_ == "blocked" ||
            recovery_status_ != "complete") {
            return AdapterPrepareResult::FAILED;
        }
    }

    std::map<std::string, EpochToken> fencing_tokens;
    for (const auto& [shard_id, _] : ops_per_shard) {
        try {
            auto token = config_.fencing_token_provider(shard_id);
            if (!token || token->epoch == 0 || token->issuer != coordinator_id_ ||
                token->shard_id != shard_id) {
                return AdapterPrepareResult::FAILED;
            }
            fencing_tokens.emplace(shard_id, std::move(*token));
        } catch (...) {
            return AdapterPrepareResult::FAILED;
        }
    }

    CoordinatorTxnRecord rec;
    rec.transaction_id = transaction_id;
    rec.state = CoordinatorTxnState::ACTIVE;
    rec.started_at = std::chrono::steady_clock::now();
    nlohmann::json participant_endpoints = nlohmann::json::object();
    {
        std::lock_guard<std::timed_mutex> lock(mutex_);
        if (!wal_ || recovery_failed_ || recovery_status_ == "running" ||
            recovery_status_ == "blocked" ||
            recovery_status_ != "complete" ||
            transactions_.contains(transaction_id)) {
            return AdapterPrepareResult::FAILED;
        }
        for (const auto& [shard_id, ops] : ops_per_shard) {
            if (participants_.find(shard_id) == participants_.end()) {
                return AdapterPrepareResult::FAILED;
            }
            rec.participant_shards.push_back(shard_id);
            rec.shard_payloads[shard_id] = buildPayload(ops, fencing_tokens.at(shard_id));
            const auto endpoint_it = participant_endpoints_.find(shard_id);
            if (endpoint_it != participant_endpoints_.end()) {
                rec.participant_endpoints[shard_id] = endpoint_it->second;
                participant_endpoints[shard_id] = endpoint_it->second;
            }
        }
        transactions_.emplace(transaction_id, rec);
    }

    if (!logToWAL(WALEntryType::BEGIN_TX, transaction_id, {
            {"transaction_id", transaction_id},
            {"coordinator_id", coordinator_id_},
            {"shards", rec.participant_shards},
            {"participant_endpoints", participant_endpoints}
        }, /*force_sync=*/true)) {
        std::lock_guard<std::timed_mutex> lock(mutex_);
        transactions_.at(transaction_id).state = CoordinatorTxnState::FAILED;
        ++total_errors_;
        return AdapterPrepareResult::FAILED;
    }
    ++total_transactions_;

    bool prepared = false;
    {
        std::unique_lock<std::timed_mutex> lock(mutex_);
        auto& stored = transactions_.at(transaction_id);
        stored.state = CoordinatorTxnState::PREPARING;
        prepared = runPhase1(stored, lock);
        stored.state = prepared
            ? CoordinatorTxnState::PREPARED
            : CoordinatorTxnState::ABORT_DECIDED;
        stored.decision_commit = false;
    }
    if (prepared) {
        return AdapterPrepareResult::PREPARED;
    }

    const bool decision_recorded = logToWAL(
        WALEntryType::ABORT_TX, transaction_id,
        {{"transaction_id", transaction_id},
         {"coordinator_id", coordinator_id_},
         {"phase", "decision"},
         {"decision", "abort"},
         {"reason", "prepare_vote_or_rpc_failure"}},
        /*force_sync=*/true);
    if (!decision_recorded) {
        std::lock_guard<std::timed_mutex> lock(mutex_);
        transactions_.at(transaction_id).state = CoordinatorTxnState::FAILED;
        ++total_errors_;
        return AdapterPrepareResult::FAILED;
    }

    bool acknowledged = false;
    {
        std::unique_lock<std::timed_mutex> lock(mutex_);
        acknowledged = runPhase2(transactions_.at(transaction_id), false, lock);
    }
    const bool completed = acknowledged &&
        logToWAL(WALEntryType::ABORT_TX, transaction_id,
                 {{"transaction_id", transaction_id},
                  {"coordinator_id", coordinator_id_},
                  {"phase", "complete"}});
    {
        std::lock_guard<std::timed_mutex> lock(mutex_);
        transactions_.at(transaction_id).state = completed
            ? CoordinatorTxnState::COMPLETED
            : CoordinatorTxnState::ABORT_DECIDED;
    }
    if (completed) {
        ++total_aborts_;
        return AdapterPrepareResult::ABORTED;
    }
    ++total_errors_;
    return AdapterPrepareResult::FAILED;
}

CoordinatorTxnOutcome TwoPhaseCommitCoordinator::commitPreparedForAdapter(
    const std::string& transaction_id) {
    CoordinatorTxnOutcome outcome;
    outcome.transaction_id = transaction_id;
    std::optional<int64_t> commit_timestamp;
    if (config_.commit_timestamp_provider) {
        try {
            commit_timestamp = config_.commit_timestamp_provider(transaction_id);
        } catch (...) {
            outcome.reason = "failed to allocate canonical commit timestamp";
            return outcome;
        }
        if (!commit_timestamp || *commit_timestamp <= 0) {
            outcome.reason = "canonical commit timestamp is unavailable";
            return outcome;
        }
    }
    {
        std::lock_guard<std::timed_mutex> lock(mutex_);
        if (!wal_ || recovery_failed_ || recovery_status_ == "running" ||
            recovery_status_ == "blocked" ||
            recovery_status_ != "complete") {
            outcome.reason = "coordinator WAL recovery is not available";
            return outcome;
        }
        const auto it = transactions_.find(transaction_id);
        if (it == transactions_.end() ||
            it->second.state != CoordinatorTxnState::PREPARED) {
            outcome.reason = "transaction is not prepared";
            return outcome;
        }
    }

    nlohmann::json decision_data = {
        {"transaction_id", transaction_id},
        {"coordinator_id", coordinator_id_},
        {"phase", "decision"},
        {"decision", "commit"}
    };
    if (commit_timestamp) {
        decision_data["commit_timestamp_ns"] = *commit_timestamp;
    }
    if (!logToWAL(WALEntryType::COMMIT_TX, transaction_id, decision_data,
                  /*force_sync=*/true)) {
        std::lock_guard<std::timed_mutex> lock(mutex_);
        transactions_.at(transaction_id).state = CoordinatorTxnState::FAILED;
        outcome.reason = "failed to durably persist COMMIT decision";
        ++total_errors_;
        return outcome;
    }

    {
        std::lock_guard<std::timed_mutex> lock(mutex_);
        auto& record = transactions_.at(transaction_id);
        record.state = CoordinatorTxnState::COMMIT_DECIDED;
        record.decision_commit = true;
        record.commit_timestamp_ns = commit_timestamp;
    }

    bool acknowledged = false;
    {
        std::unique_lock<std::timed_mutex> lock(mutex_);
        acknowledged = runPhase2(transactions_.at(transaction_id), true, lock);
    }
    if (!acknowledged ||
        !logToWAL(WALEntryType::COMMIT_TX, transaction_id,
                  {{"transaction_id", transaction_id},
                   {"coordinator_id", coordinator_id_},
                   {"phase", "complete"}})) {
        outcome.reason = "COMMIT decision remains in-doubt";
        ++total_errors_;
        return outcome;
    }
    {
        std::lock_guard<std::timed_mutex> lock(mutex_);
        transactions_.at(transaction_id).state = CoordinatorTxnState::COMPLETED;
    }
    ++total_commits_;
    outcome.result = CoordinatorTxnResult::COMMITTED;
    return outcome;
}

CoordinatorTxnOutcome TwoPhaseCommitCoordinator::abortForAdapter(
    const std::string& transaction_id,
    const std::vector<std::string>& participant_shards) {
    CoordinatorTxnOutcome outcome;
    outcome.transaction_id = transaction_id;
    if (transaction_id.empty()) {
        outcome.reason = "transaction id is empty";
        return outcome;
    }

    bool has_record = false;
    {
        std::lock_guard<std::timed_mutex> lock(mutex_);
        if (!wal_ || recovery_failed_ || recovery_status_ == "running" ||
            recovery_status_ == "blocked" ||
            recovery_status_ != "complete") {
            outcome.reason = "coordinator WAL recovery is not available";
            return outcome;
        }
        auto it = transactions_.find(transaction_id);
        if (it != transactions_.end()) {
            if (it->second.state == CoordinatorTxnState::COMPLETED) {
                outcome.result = it->second.decision_commit
                    ? CoordinatorTxnResult::FAILURE
                    : CoordinatorTxnResult::ABORTED;
                return outcome;
            }
            if (it->second.state == CoordinatorTxnState::COMMIT_DECIDED ||
                it->second.decision_commit) {
                outcome.reason = "COMMIT decision is already authoritative";
                return outcome;
            }
            has_record = true;
        } else if (!participant_shards.empty()) {
            CoordinatorTxnRecord rec;
            rec.transaction_id = transaction_id;
            rec.state = CoordinatorTxnState::ACTIVE;
            rec.started_at = std::chrono::steady_clock::now();
            for (const auto& shard_id : participant_shards) {
                if (participants_.find(shard_id) == participants_.end()) {
                    outcome.reason = "unknown shard: " + shard_id;
                    return outcome;
                }
                rec.participant_shards.push_back(shard_id);
                const auto endpoint_it = participant_endpoints_.find(shard_id);
                if (endpoint_it != participant_endpoints_.end()) {
                    rec.participant_endpoints[shard_id] = endpoint_it->second;
                }
            }
            transactions_.emplace(transaction_id, std::move(rec));
            has_record = true;
        }
    }

    if (!has_record) {
        outcome.result = CoordinatorTxnResult::ABORTED;
        return outcome;
    }

    const auto rec_it = [&]() {
        std::lock_guard<std::timed_mutex> lock(mutex_);
        return transactions_.at(transaction_id);
    }();
    nlohmann::json endpoints = nlohmann::json::object();
    for (const auto& [shard_id, endpoint] : rec_it.participant_endpoints) {
        endpoints[shard_id] = endpoint;
    }
    if ((rec_it.state == CoordinatorTxnState::ACTIVE ||
         rec_it.state == CoordinatorTxnState::FAILED) &&
        !logToWAL(WALEntryType::BEGIN_TX, transaction_id,
                  {{"transaction_id", transaction_id},
                   {"coordinator_id", coordinator_id_},
                   {"shards", rec_it.participant_shards},
                   {"participant_endpoints", endpoints}},
                  /*force_sync=*/true)) {
        outcome.reason = "failed to durably persist transaction BEGIN";
        return outcome;
    }
    if (!logToWAL(WALEntryType::ABORT_TX, transaction_id,
                  {{"transaction_id", transaction_id},
                   {"coordinator_id", coordinator_id_},
                   {"phase", "decision"},
                   {"decision", "abort"}},
                  /*force_sync=*/true)) {
        outcome.reason = "failed to durably persist ABORT decision";
        return outcome;
    }
    {
        std::lock_guard<std::timed_mutex> lock(mutex_);
        auto& stored = transactions_.at(transaction_id);
        stored.state = CoordinatorTxnState::ABORT_DECIDED;
        stored.decision_commit = false;
    }

    bool acknowledged = false;
    {
        std::unique_lock<std::timed_mutex> lock(mutex_);
        acknowledged = runPhase2(transactions_.at(transaction_id), false, lock);
    }
    if (!acknowledged ||
        !logToWAL(WALEntryType::ABORT_TX, transaction_id,
                  {{"transaction_id", transaction_id},
                   {"coordinator_id", coordinator_id_},
                   {"phase", "complete"}})) {
        outcome.reason = "ABORT decision remains in-doubt";
        ++total_errors_;
        return outcome;
    }
    {
        std::lock_guard<std::timed_mutex> lock(mutex_);
        transactions_.at(transaction_id).state = CoordinatorTxnState::COMPLETED;
    }
    ++total_aborts_;
    outcome.result = CoordinatorTxnResult::ABORTED;
    return outcome;
}

// ─────────────────────────────────────────────────────────────────────────────
// Recovery
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Recover In Doubt Transactions.
 * @return Return value.
 * @details Calls: THEMIS_INFO(), readRange(), getOldestLSN(), themis::transaction::TwoPhaseCommitWALRecovery::reconstruct(), std::chrono::steady_clock::now(), emplace(), std::move(), lock().
 */
size_t TwoPhaseCommitCoordinator::recoverInDoubtTransactions() {
    if (!wal_) {
      return 0;
    }

    {
        std::lock_guard<std::timed_mutex> lock(mutex_);
        if (recovery_status_ == "running") {
            throw std::runtime_error("coordinator WAL recovery is already in progress");
        }
        if (!active_drivers_.empty()) {
            throw std::runtime_error(
                "coordinator WAL recovery cannot run while transactions are active");
        }
        recovery_status_ = "running";
        recovery_failed_ = false;
    }

    THEMIS_INFO("2PC coordinator [{}] recovering from WAL…", coordinator_id_);

    size_t resolved = 0;

    try {
        themis::transaction::TwoPhaseCommitWALRecovery::RecoveredTransactions recovered_wal;
        {
            std::lock_guard<std::mutex> recovery_lock(recovery_history_mutex_);
            if (wal_write_failed_) {
                throw std::runtime_error(
                    "ambiguous coordinator WAL write; restart and validated WAL rescan required");
            }
            recovered_wal = recovered_wal_transactions_;
        }

        std::map<std::string, CoordinatorTxnRecord> recovered = {};

        for (const auto& [txn_id, replay_txn] : recovered_wal) {
            CoordinatorTxnRecord rec;
            rec.transaction_id     = txn_id;
            rec.started_at         = std::chrono::steady_clock::now();
            rec.participant_shards = replay_txn.participants;
            rec.participant_endpoints = replay_txn.participant_endpoints;
            rec.decision_commit = replay_txn.has_decision && replay_txn.decision_commit;
            rec.commit_timestamp_ns = replay_txn.commit_timestamp_ns;

            if (replay_txn.participants_invalid || replay_txn.participants.empty()) {
                rec.state = CoordinatorTxnState::FAILED;
            } else if (replay_txn.completed) {
                rec.state = CoordinatorTxnState::COMPLETED;
            } else if (replay_txn.has_decision) {
                rec.state = replay_txn.decision_commit
                    ? CoordinatorTxnState::COMMIT_DECIDED
                    : CoordinatorTxnState::ABORT_DECIDED;
            } else {
                rec.state = CoordinatorTxnState::ACTIVE;
            }

            recovered.emplace(txn_id, std::move(rec));
        }

        for (const auto& [txn_id, rec] : recovered) {
            if (rec.state == CoordinatorTxnState::COMPLETED) {
                continue;
            }
            for (const auto& [shard_id, endpoint] : rec.participant_endpoints) {
                bool needs_registration = false;
                {
                    std::lock_guard<std::timed_mutex> lock(mutex_);
                    const auto registered = participants_.find(shard_id);
                    if (registered == participants_.end()) {
                        needs_registration = true;
                    } else {
                        const auto registered_endpoint = participant_endpoints_.find(shard_id);
                        if (registered_endpoint != participant_endpoints_.end() &&
                            registered_endpoint->second != endpoint) {
                            throw std::runtime_error(
                                "recovered transaction " + txn_id +
                                " conflicts with registered endpoint for participant " + shard_id);
                        }
                    }
                }
                if (needs_registration) {
                    ShardRPCClient::Config rpc_config;
                    rpc_config.endpoint = endpoint;
                    rpc_config.timeout_ms = static_cast<int>(config_.prepare_timeout.count());
                    registerParticipantByEndpoint(shard_id, rpc_config);
                }
            }
        }

        const auto invalid_participant_record = std::find_if(
            recovered.begin(), recovered.end(), [](const auto& item) {
                return item.second.state == CoordinatorTxnState::FAILED;
            });
        if (invalid_participant_record != recovered.end()) {
            {
                std::lock_guard<std::timed_mutex> lock(mutex_);
                for (auto& [txn_id, rec] : recovered) {
                    if (rec.state != CoordinatorTxnState::COMPLETED) {
                        transactions_[txn_id] = std::move(rec);
                    }
                }
            }
            throw std::runtime_error(
                "recovered transaction " + invalid_participant_record->first +
                " has missing or malformed participant identities");
        }

        if (!wal_->flushDurable()) {
            THEMIS_ERROR("2PC coordinator [{}] cannot replay WAL decisions until "
                         "the durability check succeeds", coordinator_id_);
            {
                std::lock_guard<std::timed_mutex> failed_recovery_lock(mutex_);
                for (auto& [txn_id, rec] : recovered) {
                    if (rec.state != CoordinatorTxnState::COMPLETED) {
                        rec.state = CoordinatorTxnState::FAILED;
                        transactions_[txn_id] = std::move(rec);
                    }
                }
            }
            throw std::runtime_error(
                "coordinator WAL durability check failed during recovery");
        }

        // Re-drive transactions that have a Phase 2 decision but are not COMPLETED
        // 2PC-1: use unique_lock so runPhase2 can release it around each RPC.
        std::unique_lock<std::timed_mutex> lock(mutex_);
        for (auto& [txn_id, rec] : recovered) {
            if (rec.state == CoordinatorTxnState::COMPLETED) {
                transactions_[txn_id] = rec;
                continue;
            }

            const bool has_decision = rec.state == CoordinatorTxnState::COMMIT_DECIDED ||
                                      rec.state == CoordinatorTxnState::ABORT_DECIDED;
            bool do_commit = rec.state == CoordinatorTxnState::COMMIT_DECIDED;
            if (!has_decision) {
                // No decision logged → abort conservatively and broadcast ABORT
                // to release participants from PREPARED state (2PC-2 fix).
                THEMIS_WARN("2PC coordinator [{}] in-doubt txn {} has no decision "
                            "– aborting conservatively and broadcasting ABORT",
                            coordinator_id_, txn_id);
                rec.state = CoordinatorTxnState::ABORT_DECIDED;
                do_commit = false;
                const bool abort_decision_recorded = logToWAL(
                    WALEntryType::ABORT_TX, txn_id, {
                        {"transaction_id", txn_id},
                        {"coordinator_id", coordinator_id_},
                        {"phase", "decision"},
                        {"decision", "abort"},
                        {"recovery", true},
                        {"reason", "no_decision_conservative_abort"}
                    }, /*force_sync=*/true);
                if (!abort_decision_recorded) {
                    rec.state = CoordinatorTxnState::FAILED;
                    transactions_[txn_id] = rec;
                    THEMIS_ERROR("2PC coordinator [{}] could not persist conservative ABORT "
                                 "decision for txn {}; phase two was not sent",
                                 coordinator_id_, txn_id);
                    throw std::runtime_error(
                        "could not persist conservative ABORT decision for txn " + txn_id);
                }
            } else {
                THEMIS_WARN("2PC coordinator [{}] re-driving in-doubt txn {} with decision {}",
                            coordinator_id_, txn_id, do_commit ? "COMMIT" : "ABORT");
            }

            transactions_[txn_id] = rec;
            const bool all_acknowledged = runPhase2(rec, do_commit, lock);
            nlohmann::json completion_data = {
                {"transaction_id", txn_id},
                {"coordinator_id", coordinator_id_},
                {"phase", "complete"},
                {"recovery", true}
            };
            if (!has_decision) {
                completion_data["reason"] = "no_decision_conservative_abort";
            }
            const bool completion_recorded = all_acknowledged &&
                logToWAL(do_commit ? WALEntryType::COMMIT_TX : WALEntryType::ABORT_TX,
                         txn_id, completion_data, /*force_sync=*/true);

            rec.state = completion_recorded
                ? CoordinatorTxnState::COMPLETED
                : (do_commit
                    ? CoordinatorTxnState::COMMIT_DECIDED
                    : CoordinatorTxnState::ABORT_DECIDED);

            // runPhase2 may return without reacquiring the coordinator lock
            // after a lock timeout. Do not mark the transaction completed;
            // retain the in-memory recovery view only if the lock is available.
            if (!lock.owns_lock() && !lock.try_lock_for(config_.prepare_timeout)) {
                THEMIS_ERROR("2PC coordinator [{}] could not reacquire its state lock "
                             "after recovery RPCs for txn {}",
                             coordinator_id_, txn_id);
                throw std::runtime_error(
                    "could not reacquire coordinator state lock after recovery RPCs");
            }
            transactions_[txn_id] = rec;

            if (completion_recorded) {
                ++resolved;
            } else {
                THEMIS_WARN("2PC coordinator [{}] txn {} remains in-doubt after recovery "
                            "(decision={}, acknowledgements={}, completion_recorded={})",
                            coordinator_id_, txn_id,
                            do_commit ? "COMMIT" : "ABORT",
                            all_acknowledged, completion_recorded);
            }
        }

        const bool unresolved = std::any_of(
            transactions_.begin(), transactions_.end(), [](const auto& item) {
                return item.second.state != CoordinatorTxnState::COMPLETED;
            });
        recovery_status_ = unresolved ? "blocked" : "complete";
    } catch (const std::exception& e) {
        {
            std::lock_guard<std::timed_mutex> lock(mutex_);
            recovery_failed_ = true;
            recovery_status_ = "failed";
        }
        THEMIS_ERROR("2PC coordinator [{}] WAL recovery failed: {}", coordinator_id_, e.what());
        throw;
    } catch (...) {
        {
            std::lock_guard<std::timed_mutex> lock(mutex_);
            recovery_failed_ = true;
            recovery_status_ = "failed";
        }
        THEMIS_ERROR("2PC coordinator [{}] WAL recovery failed with an unknown exception",
                     coordinator_id_);
        throw;
    }

    THEMIS_INFO("2PC coordinator [{}] recovery complete – {} in-doubt txns resolved",
                coordinator_id_, resolved);
    return resolved;
}

std::string TwoPhaseCommitCoordinator::recoveryCoordinatorName() const {
    return "TwoPhaseCommitCoordinator";
}

std::string TwoPhaseCommitCoordinator::recoveryBackendName() const {
    return wal_ ? "WAL" : "disabled";
}

std::vector<themis::transaction::RecoverableTwoPhaseTransaction>
TwoPhaseCommitCoordinator::getRecoverableTransactions() const {
    using themis::transaction::RecoverableTwoPhaseState;
    using themis::transaction::RecoverableTwoPhaseTransaction;

    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::timed_mutex> lock(mutex_);

    std::vector<RecoverableTwoPhaseTransaction> result = {};

    for (const auto& [txn_id, rec] : transactions_) {
        if (rec.state == CoordinatorTxnState::COMPLETED) {
            continue;
        }

        RecoverableTwoPhaseTransaction rt;
        rt.transaction_id = txn_id;

        switch (rec.state) {
        case CoordinatorTxnState::ACTIVE:
            rt.state = RecoverableTwoPhaseState::ACTIVE;
            break;
        case CoordinatorTxnState::PREPARING:
            rt.state = RecoverableTwoPhaseState::PREPARING;
            break;
        case CoordinatorTxnState::PREPARED:
            rt.state = RecoverableTwoPhaseState::PREPARED;
            break;
        case CoordinatorTxnState::COMMIT_DECIDED:
            rt.state             = RecoverableTwoPhaseState::COMMITTING;
            rt.decision_recorded = true;
            rt.decision_commit   = true;
            break;
        case CoordinatorTxnState::ABORT_DECIDED:
            rt.state             = RecoverableTwoPhaseState::ABORTING;
            rt.decision_recorded = true;
            rt.decision_commit   = false;
            break;
        case CoordinatorTxnState::FAILED:
            rt.state = RecoverableTwoPhaseState::FAILED;
            break;
        default:
            rt.state = RecoverableTwoPhaseState::UNKNOWN;
            break;
        }

        result.push_back(std::move(rt));
    }

    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// Introspection
// ─────────────────────────────────────────────────────────────────────────────

std::optional<CoordinatorTxnState>
TwoPhaseCommitCoordinator::getTransactionState(const std::string& transaction_id) const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::timed_mutex> lock(mutex_);
    auto it = transactions_.find(transaction_id);
    if (it == transactions_.end()) {
      return std::nullopt;
    }
    return it->second.state;
}

nlohmann::json TwoPhaseCommitCoordinator::getStatistics() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::timed_mutex> lock(mutex_);

    size_t active    = 0;
    size_t completed = 0;
    for (const auto& [id, rec] : transactions_) {
        if (rec.state == CoordinatorTxnState::COMPLETED) {
          ++completed;
        }
        else                                              ++active;
    }

    const auto uptime = std::chrono::steady_clock::now() - start_time_;
    std::string wal_recovery_status = "disabled";
    if (wal_) {
        switch (wal_->getRecoveryResult().status) {
        case themis::WALStorage::RecoveryStatus::InProgress:
            wal_recovery_status = "in_progress";
            break;
        case themis::WALStorage::RecoveryStatus::Complete:
            wal_recovery_status = "complete";
            break;
        case themis::WALStorage::RecoveryStatus::TruncatedTail:
            wal_recovery_status = "truncated_tail";
            break;
        case themis::WALStorage::RecoveryStatus::Corrupted:
            wal_recovery_status = "corrupted";
            break;
        case themis::WALStorage::RecoveryStatus::TimedOut:
            wal_recovery_status = "timed_out";
            break;
        case themis::WALStorage::RecoveryStatus::Failed:
            wal_recovery_status = "failed";
            break;
        }
    }
    return {
        {"coordinator_id",      coordinator_id_},
        {"uptime_seconds",      static_cast<uint64_t>(
                                    std::chrono::duration_cast<std::chrono::seconds>(uptime).count())},
        {"registered_shards",participants_.size()},
        {"total_transactions",  total_transactions_.load()},
        {"total_commits",       total_commits_.load()},
        {"total_aborts",        total_aborts_.load()},
        {"total_errors",        total_errors_.load()},
        {"active_transactions", active},
        {"completed_transactions", completed},
        {"recovery_status", recovery_status_},
        {"recovery_failed", recovery_failed_},
        {"wal_recovery_status", wal_recovery_status}
    };
}

/**
 * @brief ───────────────────────────────────────────────────────────────────────────── Internal helpers (called with mutex_ held) ─────────────────────────────────────────────────────────────────────────────
 * @param[in,out] rec Input/output parameter.
 * @param[in,out] lock Input/output parameter.
 * @return True when the operation succeeds.
 * @details Calls: find(), end(), THEMIS_ERROR(), unlock(), onPrepare(), what(), try_lock_for(), THEMIS_INFO().
 */

bool TwoPhaseCommitCoordinator::runPhase1(CoordinatorTxnRecord& rec,
                                          std::unique_lock<std::timed_mutex>& lock) {
    // 2PC-1: mutex_ must be held by the caller on entry (asserted by contract);
    // we release it around each blocking RPC call and re-acquire before
    // touching shared state, so that concurrent coordinator operations are
    // not serialised behind network I/O.
    bool all_committed = true;

    for (const auto& [shard_id, payload] : rec.shard_payloads) {
        auto pit = participants_.find(shard_id);  // safe: lock held
        if (pit == participants_.end()) {
            THEMIS_ERROR("2PC coordinator [{}] Phase 1 – participant {} not found for txn {}",
                         coordinator_id_, shard_id, rec.transaction_id);
            rec.votes[shard_id] = false;
            all_committed       = false;
            continue;
        }

        // Capture the raw pointer while the lock is held; the object's lifetime
        // is tied to owned_adapters_ (or external caller), which outlives this call.
        auto* participant = pit->second;

        bool vote = false;
        lock.unlock();  // release mutex_ before blocking RPC
        try {
            vote = participant->onPrepare(
                rec.transaction_id,
                coordinator_id_,
                payload
            );
        } catch (const std::exception& e) {
            THEMIS_ERROR("2PC coordinator [{}] Phase 1 – shard {} threw on PREPARE for txn {}: {}",
                         coordinator_id_, shard_id, rec.transaction_id, e.what());
            vote = false;
        } catch (...) {
            vote = false;
        }
        if (!lock.try_lock_for(config_.prepare_timeout)) {
            THEMIS_ERROR("2PC coordinator [{}] Phase 1 – timed out re-acquiring mutex for txn {}; aborting",
                         coordinator_id_, rec.transaction_id);
            lock.lock();
            return false;
        }
        if (!vote) {
            THEMIS_INFO("2PC coordinator [{}] shard {} voted ABORT for txn {}",
                        coordinator_id_, shard_id, rec.transaction_id);
            all_committed = false;
        }
        rec.votes[shard_id] = vote;
    }

    return all_committed;
}

/**
 * @brief Run Phase2.
 * @param[in,out] rec Input/output parameter.
 * @param[in] do_commit Input parameter.
 * @param[in,out] lock Input/output parameter.
 * @details Calls: empty(), reserve(), size(), push_back(), find(), end(), THEMIS_WARN(), unlock().
 */
bool TwoPhaseCommitCoordinator::runPhase2(CoordinatorTxnRecord& rec, bool do_commit,
                                          std::unique_lock<std::timed_mutex>& lock) {
    // 2PC-1: same pattern as runPhase1 — release lock around each blocking RPC.
    const auto shard_ids =
        rec.participant_shards.empty()
            ? [&rec]() {
                  std::vector<std::string> ids = {};

                  ids.reserve(rec.shard_payloads.size());
                  for (const auto& [shard_id, _] : rec.shard_payloads) {
                      ids.push_back(shard_id);
                  }
                  return ids;
              }()
            : rec.participant_shards;

    bool all_acknowledged = !shard_ids.empty();
    for (const auto& shard_id : shard_ids) {
        auto pit = participants_.find(shard_id);  // safe: lock held
        if (pit == participants_.end() && config_.participant_endpoint_resolver) {
            std::unique_ptr<ShardRPCServer::RequestHandler> adapter;
            lock.unlock();
            try {
                const auto rpc = config_.participant_endpoint_resolver(shard_id);
                if (rpc && validEndpoint(rpc->endpoint) &&
                    (!isTestEndpoint(rpc->endpoint) ||
                     config_.in_process_response_handler)) {
                    adapter = makeEndpointParticipant(*rpc);
                }
            } catch (const std::exception& e) {
                THEMIS_WARN("Cannot resolve participant {}: {}", shard_id, e.what());
            }
            lock.lock();
            if (adapter) {
                participants_[shard_id] = adapter.get();
                owned_adapters_[shard_id] = std::move(adapter);
                pit = participants_.find(shard_id);
            }
        }
        if (pit == participants_.end()) {
            THEMIS_WARN("2PC coordinator [{}] Phase 2 – participant {} not found for txn {} "
                        "(shard may have been deregistered)",
                        coordinator_id_, shard_id, rec.transaction_id);
            all_acknowledged = false;
            continue;
        }

        auto* participant = pit->second;  // capture while lock is held

            lock.unlock();  // release mutex_ before blocking RPC
        try {
            bool acknowledged = false;
            if (do_commit) {
                acknowledged = participant->onCommit(rec.transaction_id);
            } else {
                acknowledged = participant->onAbort(rec.transaction_id);
            }
            if (!lock.try_lock_for(config_.prepare_timeout)) {
                THEMIS_ERROR("2PC coordinator [{}] Phase 2 – timed out re-acquiring mutex for txn {} after {}; returning without phase-two completion",
                             coordinator_id_, rec.transaction_id,
                             do_commit ? "COMMIT" : "ABORT");
                lock.lock();
                return false;
            }
            if (acknowledged) {
                rec.phase2_acked.push_back(shard_id);
            } else {
                all_acknowledged = false;
                THEMIS_ERROR("2PC coordinator [{}] Phase 2 – shard {} rejected {} for txn {}",
                             coordinator_id_, shard_id,
                             do_commit ? "COMMIT" : "ABORT", rec.transaction_id);
            }
        } catch (const std::exception& e) {
            if (!lock.owns_lock() && !lock.try_lock_for(config_.prepare_timeout)) {
                THEMIS_ERROR("2PC coordinator [{}] Phase 2 – timed out re-acquiring mutex for txn {} on error path",
                             coordinator_id_, rec.transaction_id);
                lock.lock();
                return false;
            }
            // Log and continue – idempotency allows retrying later
            THEMIS_ERROR("2PC coordinator [{}] Phase 2 – shard {} threw on {} for txn {}: {}",
                         coordinator_id_, shard_id,
                         do_commit ? "COMMIT" : "ABORT",
                         rec.transaction_id, e.what());
            all_acknowledged = false;
        } catch (...) {
            if (!lock.owns_lock()) {
                lock.lock();
            }
            all_acknowledged = false;
        }
    }
    return all_acknowledged;
}

/**
 * @brief Build Payload.
 * @param[in] ops Input parameter.
 * @return Return value.
 * @details Calls: dump().
 */
std::string TwoPhaseCommitCoordinator::buildPayload(
    const nlohmann::json& ops,
    const EpochToken& token) {
    nlohmann::json j;
    j["operations"] = ops;
    j["fencing_token"] = {
        {"epoch", token.epoch},
        {"issuer", token.issuer},
        {"shard_id", token.shard_id},
        {"issued_at_ms", std::chrono::duration_cast<std::chrono::milliseconds>(
            token.issued_at.time_since_epoch()).count()}
    };
    return j.dump();
}

/**
 * @brief Persist one coordinator WAL entry and report whether it was written.
 * @param[in] type WAL entry kind.
 * @param[in] txn_id Transaction identifier associated with the entry.
 * @param[in] data Additional WAL payload.
 * @param[in] force_sync Whether this protocol record must be synchronously flushed.
 * @return True only when WAL exists and the append/required flush succeeds.
 */
bool TwoPhaseCommitCoordinator::logToWAL(
    WALEntryType          type,
    const std::string&    txn_id,
    const nlohmann::json& data,
    bool                  force_sync
) {
    if (!wal_) {
        return false;
    }

    std::lock_guard<std::mutex> recovery_lock(recovery_history_mutex_);
    if (wal_write_failed_) {
        return false;
    }
    WALEntry entry;
    entry.type = type;
    entry.transaction_id = txn_id;
    entry.data = data;
    validateCoordinatorEntry(recovered_wal_transactions_, entry);
    const auto persisted = WALLoggingHelper::appendEntryWithResult(
        wal_.get(), type, txn_id, data,
        force_sync || config_.sync_wal_writes,
        "coordinator", coordinator_id_
    );
    if (!persisted) {
        wal_write_failed_ = true;
        return false;
    }

    themis::transaction::TwoPhaseCommitWALRecovery::applyEntry(
        recovered_wal_transactions_, entry);
    return true;
}

} // namespace themis::sharding
