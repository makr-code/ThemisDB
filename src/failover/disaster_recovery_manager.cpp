/**
 * @file disaster_recovery_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.11
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "failover/disaster_recovery_manager.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <numeric>
#include <sstream>
#include <thread>

#include "spdlog/spdlog.h"

namespace themis {
namespace failover {
namespace {

std::string encodeHex(const std::string& value) {
    std::ostringstream encoded;
    encoded << std::hex << std::setfill('0');
    for (const unsigned char character : value) {
        encoded << std::setw(2) << static_cast<unsigned int>(character);
    }
    return encoded.str();
}

bool decodeHex(const std::string& value, std::string& decoded) {
    if (value.size() % 2 != 0) {
        return false;
    }

    decoded.clear();
    decoded.reserve(value.size() / 2);
    for (std::size_t i = 0; i < value.size(); i += 2) {
        unsigned int byte = 0;
        std::istringstream parser(value.substr(i, 2));
        parser >> std::hex >> byte;
        if (!parser || !parser.eof() || byte > 0xff) {
            return false;
        }
        decoded.push_back(static_cast<char>(byte));
    }
    return true;
}

uint64_t metadataChecksum(const std::string& record) noexcept {
    constexpr uint64_t kFnvOffsetBasis = 14695981039346656037ull;
    constexpr uint64_t kFnvPrime = 1099511628211ull;
    uint64_t checksum = kFnvOffsetBasis;
    for (const unsigned char byte : record) {
        checksum ^= byte;
        checksum *= kFnvPrime;
    }
    return checksum;
}

template <std::size_t N>
bool splitMetadataRecord(const std::string& line, std::array<std::string, N>& fields) {
    std::istringstream input(line);
    for (std::size_t i = 0; i < fields.size(); ++i) {
        if (!std::getline(input, fields[i], '|')) {
            return false;
        }
    }
    return input.peek() == std::char_traits<char>::eof();
}

std::string encodeStepResults(const std::vector<DisasterRecoveryStepResult>& results) {
    std::ostringstream serialized;
    for (const auto& result : results) {
        serialized << static_cast<unsigned int>(result.step) << ','
                   << (result.success ? 1 : 0) << ','
                   << static_cast<unsigned int>(result.error_code) << ','
                   << encodeHex(result.message) << ';';
    }
    return encodeHex(serialized.str());
}

bool decodeStepResults(const std::string& encoded,
                       std::vector<DisasterRecoveryStepResult>& results) {
    std::string serialized;
    if (!decodeHex(encoded, serialized) || serialized.empty()) {
        return false;
    }

    std::istringstream records(serialized);
    std::string record;
    while (std::getline(records, record, ';')) {
        std::array<std::string, 4> fields;
        std::istringstream input(record);
        for (std::size_t i = 0; i < fields.size(); ++i) {
            if (!std::getline(input, fields[i], ',')) {
                return false;
            }
        }
        if (input.peek() != std::char_traits<char>::eof()) {
            return false;
        }

        try {
            std::size_t consumed = 0;
            const auto step = std::stoul(fields[0], &consumed);
            if (consumed != fields[0].size() ||
                step > static_cast<unsigned int>(DisasterRecoveryStep::VERIFICATION)) {
                return false;
            }
            const auto success = std::stoul(fields[1], &consumed);
            if (consumed != fields[1].size() || success > 1) {
                return false;
            }
            const auto error_code = std::stoul(fields[2], &consumed);
            if (consumed != fields[2].size() ||
                error_code > static_cast<unsigned int>(
                    DisasterRecoveryErrorCode::METADATA_PERSISTENCE_FAILED)) {
                return false;
            }
            std::string message;
            if (!decodeHex(fields[3], message)) {
                return false;
            }
            results.push_back({static_cast<DisasterRecoveryStep>(step),
                               success != 0,
                               std::move(message),
                               static_cast<DisasterRecoveryErrorCode>(error_code)});
        } catch (const std::exception&) {
            return false;
        }
    }
    return !results.empty();
}

std::string planFingerprint(const DisasterRecoveryPlan& plan) {
    std::string fingerprint;
    const auto append = [&fingerprint](const std::string& field) {
        fingerprint += std::to_string(field.size());
        fingerprint.push_back(':');
        fingerprint += field;
    };

    append(plan.primary_site);
    append(plan.recovery_site);
    append(plan.snapshot_id);
    append(plan.snapshot_checksum);
    append(std::to_string(plan.required_lsn));
    append(std::to_string(plan.required_term));
    append(plan.dry_run ? "1" : "0");
    append(plan.shift_traffic ? "1" : "0");
    append(std::to_string(plan.critical_nodes.size()));
    for (const auto& node : plan.critical_nodes) {
        append(node);
    }
    return encodeHex(fingerprint);
}

}  // namespace

DisasterRecoveryManager::DisasterRecoveryManager(
    DisasterRecoveryConfig config,
    std::shared_ptr<themisdb::replication::ReplicationManager> replication_mgr,
    std::shared_ptr<sharding::EpochFencingManager> fencing_mgr)
    : config_(std::move(config)),
      replication_mgr_(std::move(replication_mgr)),
      fencing_mgr_(std::move(fencing_mgr)) {
    loadCompletedPlans();
}

/**
 * @brief Execute Plan.
 * @param[in] plan Input parameter.
 * @return Return value.
 * @details Calls: idem_lock(), find(), end(), spdlog::info(), exec_lock(), owns_lock(), spdlog::error(), std::chrono::steady_clock::now().
 */
DisasterRecoveryResult DisasterRecoveryManager::executePlan(const DisasterRecoveryPlan& plan) {
    const auto fingerprint = planFingerprint(plan);
    // FO-IMPL-007: Idempotent plan execution — return cached result for repeated plan_id
    if (metadata_load_error_.empty() || plan.dry_run) {
        std::lock_guard<std::mutex> idem_lock(idempotency_mutex_);
        auto it = completed_plans_.find(plan.plan_id);
        const bool may_return_cached_result =
            metadata_load_error_.empty() || plan.dry_run ||
            (it != completed_plans_.end() && !it->second.success);
        if (it != completed_plans_.end() && may_return_cached_result) {
            const auto prior_fingerprint = completed_plan_fingerprints_.find(plan.plan_id);
            if (prior_fingerprint == completed_plan_fingerprints_.end() ||
                prior_fingerprint->second != fingerprint) {
                DisasterRecoveryResult conflict;
                conflict.final_state = DisasterRecoveryState::FAILED;
                conflict.error_code = DisasterRecoveryErrorCode::INVALID_PLAN;
                conflict.error_message = "plan_id was already used for a different recovery plan";
                return conflict;
            }
            spdlog::info("DisasterRecoveryManager::executePlan: returning cached result "
                         "for plan_id='{}' (idempotent)", plan.plan_id);
            return it->second;
        }
    }

    // Reject concurrent invocations: concurrent calls race on state_ and fencing_mgr_.
    std::unique_lock<std::mutex> exec_lock(execution_mutex_, std::try_to_lock);
    if (!exec_lock.owns_lock()) {
        DisasterRecoveryResult result;
        result.success       = false;
        result.final_state   = DisasterRecoveryState::FAILED;
        result.error_message = "concurrent execution rejected";
        spdlog::error("DisasterRecoveryManager::executePlan: concurrent execution rejected "
                      "(plan_id='{}')", plan.plan_id);
        return result;
    }

    const auto started_at = std::chrono::steady_clock::now();

    DisasterRecoveryResult result;
    std::string validation_error = {};
    if (!validatePlan(plan, validation_error)) {
        transitionState(DisasterRecoveryState::FAILED);
        result.success = false;
        result.final_state = DisasterRecoveryState::FAILED;
        result.error_message = validation_error;
        result.error_code = DisasterRecoveryErrorCode::INVALID_PLAN;
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - started_at);
        updateStatistics(result);

        if (!plan.plan_id.empty()) {
            std::lock_guard<std::mutex> idem_lock(idempotency_mutex_);
            completed_plans_[plan.plan_id] = result;
            completed_plan_fingerprints_[plan.plan_id] = fingerprint;
        }
        return result;
    }

    transitionState(DisasterRecoveryState::PRECHECKS);

    uint64_t fenced_epoch = 0;
    std::string step_error = {};
    DisasterRecoveryErrorCode step_error_code = DisasterRecoveryErrorCode::NONE;

    if (!runStep(DisasterRecoveryStep::PRECHECKS, DisasterRecoveryState::PRECHECKS,
                 plan, result, step_error, fenced_epoch, step_error_code)) {
        result.success = false;
        result.final_state = DisasterRecoveryState::FAILED;
        result.error_message = step_error;
        result.error_code = step_error_code;
    } else if (!runStep(DisasterRecoveryStep::SNAPSHOT_VALIDATION,
                        DisasterRecoveryState::SNAPSHOT_VALIDATION,
                        plan, result, step_error, fenced_epoch, step_error_code)) {
        result.success = false;
        result.final_state = DisasterRecoveryState::FAILED;
        result.error_message = step_error;
        result.error_code = step_error_code;
    } else if (!runStep(DisasterRecoveryStep::EPOCH_FENCING,
                        DisasterRecoveryState::EPOCH_FENCING,
                        plan, result, step_error, fenced_epoch, step_error_code)) {
        result.success = false;
        result.final_state = DisasterRecoveryState::FAILED;
        result.error_message = step_error;
        result.error_code = step_error_code;
    } else if (!runStep(DisasterRecoveryStep::RESTORE,
                        DisasterRecoveryState::RESTORE,
                        plan, result, step_error, fenced_epoch, step_error_code)) {
        result.success = false;
        result.final_state = DisasterRecoveryState::FAILED;
        result.error_message = step_error;
        result.error_code = step_error_code;
    } else if (!runStep(DisasterRecoveryStep::REPLICA_CATCHUP,
                        DisasterRecoveryState::REPLICA_CATCHUP,
                        plan, result, step_error, fenced_epoch, step_error_code)) {
        result.success = false;
        result.final_state = DisasterRecoveryState::FAILED;
        result.error_message = step_error;
        result.error_code = step_error_code;
    } else if (!runStep(DisasterRecoveryStep::TRAFFIC_SHIFT,
                        DisasterRecoveryState::TRAFFIC_SHIFT,
                        plan, result, step_error, fenced_epoch, step_error_code)) {
        result.success = false;
        result.final_state = DisasterRecoveryState::FAILED;
        result.error_message = step_error;
        result.error_code = step_error_code;
    } else if (!runStep(DisasterRecoveryStep::VERIFICATION,
                        DisasterRecoveryState::VERIFICATION,
                        plan, result, step_error, fenced_epoch, step_error_code)) {
        result.success = false;
        result.final_state = DisasterRecoveryState::FAILED;
        result.error_message = step_error;
        result.error_code = step_error_code;
    } else {
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - started_at);
        result.fenced_epoch = fenced_epoch;
        std::string persistence_error;
        if (!plan.dry_run && !persistCompletedPlan(plan, result, persistence_error)) {
            result.success = false;
            result.final_state = DisasterRecoveryState::FAILED;
            result.error_code = DisasterRecoveryErrorCode::METADATA_PERSISTENCE_FAILED;
            result.error_message = persistence_error;
        } else {
            result.success = true;
            result.final_state = DisasterRecoveryState::COMPLETED;
            transitionState(DisasterRecoveryState::COMPLETED);
        }
    }

    if (!result.success) {
        transitionState(result.final_state);
        spdlog::error("Disaster recovery plan '{}' failed: {}", plan.plan_id, result.error_message);
    } else {
        spdlog::info("Disaster recovery plan '{}' completed", plan.plan_id);
    }

    result.fenced_epoch = fenced_epoch;
    result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - started_at);

    updateStatistics(result);

    // FO-IMPL-007: Cache result for idempotency (only for plans with a valid plan_id)
    if (!plan.plan_id.empty()) {
        std::lock_guard<std::mutex> idem_lock(idempotency_mutex_);
        completed_plans_[plan.plan_id] = result;
        completed_plan_fingerprints_[plan.plan_id] = fingerprint;
    }

    return result;
}

bool DisasterRecoveryManager::validatePlan(const DisasterRecoveryPlan& plan, std::string& error) const {
    if (plan.plan_id.empty()) {
        error = "plan_id must not be empty";
        return false;
    }
    if (plan.primary_site.empty()) {
        error = "primary_site must not be empty";
        return false;
    }
    if (plan.recovery_site.empty()) {
        error = "recovery_site must not be empty";
        return false;
    }
    if (!plan.dry_run && plan.snapshot_id.empty()) {
        error = "snapshot_id must not be empty for non-dry-run recovery";
        return false;
    }
    if (!plan.dry_run && plan.snapshot_checksum.empty()) {
        error = "snapshot_checksum must be provided for non-dry-run recovery";
        return false;
    }
    if (!plan.dry_run && (plan.required_lsn == 0 || plan.required_term == 0)) {
        error = "required_lsn and required_term must be non-zero for non-dry-run recovery";
        return false;
    }
    return true;
}

/**
 * @brief Set Step Hook.
 * @param[in] step Input parameter.
 * @param[in] hook Input parameter.
 * @details Calls: std::move().
 */
void DisasterRecoveryManager::setStepHook(DisasterRecoveryStep step, StepHook hook) {
    if (step == DisasterRecoveryStep::PRECHECKS ||
        step == DisasterRecoveryStep::EPOCH_FENCING) {
        return;
    }
    if (hook) {
        hooks_[step] = std::move(hook);
    } else {
        hooks_.erase(step);
    }
}

void DisasterRecoveryManager::setVerificationHook(DisasterRecoveryStep step,
                                                   VerificationHook hook) {
    if (step == DisasterRecoveryStep::PRECHECKS ||
        step == DisasterRecoveryStep::EPOCH_FENCING) {
        return;
    }
    if (hook) {
        verification_hooks_[step] = std::move(hook);
    } else {
        verification_hooks_.erase(step);
    }
}

/**
 * @brief Clear Step Hooks.
 * @details Calls: clear().
 */
void DisasterRecoveryManager::clearStepHooks() {
    hooks_.clear();
    verification_hooks_.clear();
}

DisasterRecoveryState DisasterRecoveryManager::getState() const noexcept {
    return state_.load();
}

DisasterRecoveryManager::Statistics DisasterRecoveryManager::getStatistics() const {
    /**
     * @brief Lock.
     * @param[in] stats_mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

/**
 * @brief Run Step.
 * @param[in] step Input parameter.
 * @param[in] state Input parameter.
 * @param[in] plan Input parameter.
 * @param[in,out] result Input/output parameter.
 * @param[in,out] error Input/output parameter.
 * @param[in,out] fenced_epoch Input/output parameter.
 * @return True when the operation succeeds.
 * @details Calls: transitionState(), find(), end(), second(), runPrechecks(), validateSnapshot(), applyEpochFencing(), runRestore().
 */
bool DisasterRecoveryManager::runStep(DisasterRecoveryStep step,
                                      DisasterRecoveryState state,
                                      const DisasterRecoveryPlan& plan,
                                      DisasterRecoveryResult& result,
                                      std::string& error,
                                      uint64_t& fenced_epoch,
                                      DisasterRecoveryErrorCode& error_code) {
    transitionState(state);

    std::string detail = {};
    bool ok = false;

    switch (step) {
        case DisasterRecoveryStep::PRECHECKS:
            ok = runPrechecks(plan, detail, error_code);
            break;
        case DisasterRecoveryStep::EPOCH_FENCING:
            ok = applyEpochFencing(plan, detail, fenced_epoch, error_code);
            break;
        case DisasterRecoveryStep::SNAPSHOT_VALIDATION:
        case DisasterRecoveryStep::RESTORE:
        case DisasterRecoveryStep::REPLICA_CATCHUP:
        case DisasterRecoveryStep::TRAFFIC_SHIFT:
        case DisasterRecoveryStep::VERIFICATION:
            if (plan.dry_run) {
                detail = "dry-run step skipped";
                ok = true;
            } else if (step == DisasterRecoveryStep::TRAFFIC_SHIFT && !plan.shift_traffic) {
                detail = "traffic shift disabled by plan";
                ok = true;
            } else {
                ok = runAdapterStep(step, plan, detail, error_code);
            }
            break;
    }

    result.step_results.push_back({step, ok, detail, error_code});
    if (!ok) {
        error = detail;
        return false;
    }
    return true;
}

bool DisasterRecoveryManager::runAdapterStep(
    DisasterRecoveryStep step,
    const DisasterRecoveryPlan& plan,
    std::string& detail,
    DisasterRecoveryErrorCode& error_code) {
    const auto adapter = hooks_.find(step);
    if (adapter == hooks_.end()) {
        detail = "required operation adapter is not configured";
        error_code = DisasterRecoveryErrorCode::NOT_CONFIGURED;
        return false;
    }
    const auto verifier = verification_hooks_.find(step);
    if (verifier == verification_hooks_.end()) {
        detail = "independent verification adapter is not configured";
        error_code = DisasterRecoveryErrorCode::NOT_CONFIGURED;
        return false;
    }

    try {
        if (!adapter->second(plan, detail)) {
            if (detail.empty()) {
                detail = "operation adapter did not acknowledge the request";
            }
            error_code = DisasterRecoveryErrorCode::ADAPTER_FAILURE;
            return false;
        }
    } catch (const std::exception& exception) {
        detail = std::string("operation adapter threw: ") + exception.what();
        error_code = DisasterRecoveryErrorCode::ADAPTER_FAILURE;
        return false;
    } catch (...) {
        detail = "operation adapter threw an unknown exception";
        error_code = DisasterRecoveryErrorCode::ADAPTER_FAILURE;
        return false;
    }

    std::optional<DisasterRecoveryVerificationProof> proof;
    try {
        proof = verifier->second(plan, detail);
    } catch (const std::exception& exception) {
        detail = std::string("verification adapter threw: ") + exception.what();
        error_code = DisasterRecoveryErrorCode::VERIFICATION_FAILED;
        return false;
    } catch (...) {
        detail = "verification adapter threw an unknown exception";
        error_code = DisasterRecoveryErrorCode::VERIFICATION_FAILED;
        return false;
    }

    if (!proof) {
        if (detail.empty()) {
            detail = "independent verification did not provide proof";
        }
        error_code = DisasterRecoveryErrorCode::VERIFICATION_FAILED;
        return false;
    }

    switch (step) {
        case DisasterRecoveryStep::SNAPSHOT_VALIDATION:
        case DisasterRecoveryStep::RESTORE:
        case DisasterRecoveryStep::VERIFICATION:
            if (proof->checksum.empty() || proof->checksum != plan.snapshot_checksum) {
                detail = "checksum verification failed for snapshot '" + plan.snapshot_id + "'";
                error_code = DisasterRecoveryErrorCode::VERIFICATION_FAILED;
                return false;
            }
            break;
        case DisasterRecoveryStep::REPLICA_CATCHUP:
            if (proof->term != plan.required_term || proof->lsn < plan.required_lsn) {
                detail = "replication position is stale: observed term " +
                         std::to_string(proof->term) + ", LSN " +
                         std::to_string(proof->lsn) + "; required term " +
                         std::to_string(plan.required_term) + ", LSN at least " +
                         std::to_string(plan.required_lsn);
                error_code = DisasterRecoveryErrorCode::STALE_REPLICATION_POSITION;
                return false;
            }
            break;
        case DisasterRecoveryStep::TRAFFIC_SHIFT:
            if (proof->observed_site != plan.recovery_site) {
                detail = "traffic verification did not observe recovery site '" +
                         plan.recovery_site + "'";
                error_code = DisasterRecoveryErrorCode::VERIFICATION_FAILED;
                return false;
            }
            break;
        case DisasterRecoveryStep::PRECHECKS:
        case DisasterRecoveryStep::EPOCH_FENCING:
            detail = "internal DR step cannot use an external adapter";
            error_code = DisasterRecoveryErrorCode::ADAPTER_FAILURE;
            return false;
    }

    detail = "adapter acknowledged and independent verification passed";
    error_code = DisasterRecoveryErrorCode::NONE;
    return true;
}

bool DisasterRecoveryManager::persistCompletedPlan(
    const DisasterRecoveryPlan& plan,
    const DisasterRecoveryResult& result,
    std::string& detail) {
    if (config_.metadata_log_path.empty()) {
        detail = "DR metadata persistence adapter is not configured";
        return false;
    }

    const std::filesystem::path metadata_path(config_.metadata_log_path);
    std::error_code filesystem_error;
    if (!metadata_path.parent_path().empty()) {
        std::filesystem::create_directories(metadata_path.parent_path(), filesystem_error);
        if (filesystem_error) {
            detail = "cannot create DR metadata directory: " + filesystem_error.message();
            return false;
        }
    }

    const std::string payload = encodeHex(plan.plan_id) + "|" +
                                planFingerprint(plan) + "|" +
                                std::to_string(result.fenced_epoch) + "|" +
                                std::to_string(result.duration.count()) + "|" +
                                encodeStepResults(result.step_results);
    std::ofstream output(metadata_path, std::ios::out | std::ios::app);
    if (!output) {
        detail = "cannot open DR metadata log for append";
        return false;
    }

    output << payload << '|' << std::hex << metadataChecksum(payload) << '\n';
    output.flush();
    if (!output.good()) {
        detail = "failed to persist completed DR metadata";
        return false;
    }
    output.close();
    if (output.fail()) {
        detail = "failed to close completed DR metadata log";
        return false;
    }
    return true;
}

void DisasterRecoveryManager::loadCompletedPlans() {
    if (config_.metadata_log_path.empty()) {
        return;
    }

    std::error_code filesystem_error;
    const bool metadata_exists = std::filesystem::exists(
        config_.metadata_log_path, filesystem_error);
    if (filesystem_error) {
        metadata_load_error_ = "cannot inspect DR metadata log: " +
                               filesystem_error.message();
        return;
    }
    if (!metadata_exists) {
        return;
    }

    std::ifstream input(config_.metadata_log_path);
    if (!input.is_open()) {
        metadata_load_error_ = "cannot open existing DR metadata log for replay";
        return;
    }

    std::string line;
    while (std::getline(input, line)) {
        std::array<std::string, 6> fields;
        if (!splitMetadataRecord(line, fields)) {
            metadata_load_error_ = "DR metadata log contains a malformed record";
            break;
        }

        const std::string payload = fields[0] + "|" + fields[1] + "|" + fields[2] +
                                    "|" + fields[3] + "|" + fields[4];
        uint64_t stored_checksum = 0;
        uint64_t fenced_epoch = 0;
        uint64_t duration_ms = 0;
        try {
            std::size_t checksum_chars = 0;
            stored_checksum = std::stoull(fields[5], &checksum_chars, 16);
            if (checksum_chars != fields[5].size()) {
                metadata_load_error_ = "DR metadata log contains an invalid checksum";
                break;
            }
            std::size_t epoch_chars = 0;
            fenced_epoch = std::stoull(fields[2], &epoch_chars);
            if (epoch_chars != fields[2].size()) {
                metadata_load_error_ = "DR metadata log contains an invalid fencing epoch";
                break;
            }
            std::size_t duration_chars = 0;
            duration_ms = std::stoull(fields[3], &duration_chars);
            if (duration_chars != fields[3].size() ||
                duration_ms > static_cast<uint64_t>(
                    std::numeric_limits<std::chrono::milliseconds::rep>::max())) {
                metadata_load_error_ = "DR metadata log contains an invalid duration";
                break;
            }
        } catch (const std::exception&) {
            metadata_load_error_ = "DR metadata log contains an invalid numeric field";
            break;
        }

        if (stored_checksum != metadataChecksum(payload)) {
            metadata_load_error_ = "DR metadata log checksum validation failed";
            break;
        }

        std::string plan_id;
        std::string fingerprint;
        std::vector<DisasterRecoveryStepResult> step_results;
        if (!decodeHex(fields[0], plan_id) ||
            !decodeHex(fields[1], fingerprint) ||
            !decodeStepResults(fields[4], step_results) ||
            plan_id.empty() || fingerprint.empty()) {
            metadata_load_error_ = "DR metadata log contains an invalid completion record";
            break;
        }

        DisasterRecoveryResult recovered;
        recovered.success = true;
        recovered.final_state = DisasterRecoveryState::COMPLETED;
        recovered.fenced_epoch = fenced_epoch;
        recovered.duration = std::chrono::milliseconds(duration_ms);
        recovered.step_results = std::move(step_results);
        completed_plans_[plan_id] = std::move(recovered);
        completed_plan_fingerprints_[plan_id] = fields[1];
    }

    if (input.bad() && metadata_load_error_.empty()) {
        metadata_load_error_ = "failed while reading DR metadata log";
    }
}

/**
 * @brief Run Prechecks.
 * @param[in] plan Input parameter.
 * @param[in,out] detail Input/output parameter.
 * @return True when the operation succeeds.
 * @details Calls: hasQuorum(), getClusterHealth(), std::any_of(), begin(), end().
 */
bool DisasterRecoveryManager::runPrechecks(const DisasterRecoveryPlan& plan,
                                           std::string& detail,
                                           DisasterRecoveryErrorCode& error_code) {
    if (plan.dry_run) {
        detail = "dry-run prechecks passed";
        return true;
    }

    if (!metadata_load_error_.empty()) {
        detail = metadata_load_error_;
        error_code = DisasterRecoveryErrorCode::METADATA_PERSISTENCE_FAILED;
        return false;
    }

    if (!config_.require_quorum) {
        detail = "quorum enforcement cannot be disabled for non-dry-run recovery";
        error_code = DisasterRecoveryErrorCode::QUORUM_UNAVAILABLE;
        return false;
    }
    if (!config_.enforce_epoch_fencing) {
        detail = "epoch fencing cannot be disabled for non-dry-run recovery";
        error_code = DisasterRecoveryErrorCode::FENCING_UNAVAILABLE;
        return false;
    }
    if (config_.metadata_log_path.empty()) {
        detail = "DR metadata persistence adapter is not configured";
        error_code = DisasterRecoveryErrorCode::NOT_CONFIGURED;
        return false;
    }

    const std::filesystem::path metadata_path(config_.metadata_log_path);
    std::error_code filesystem_error;
    if (!metadata_path.parent_path().empty()) {
        std::filesystem::create_directories(metadata_path.parent_path(), filesystem_error);
        if (filesystem_error) {
            detail = "cannot create DR metadata directory: " + filesystem_error.message();
            error_code = DisasterRecoveryErrorCode::METADATA_PERSISTENCE_FAILED;
            return false;
        }
    }
    std::ofstream metadata_check(metadata_path, std::ios::out | std::ios::app);
    if (!metadata_check) {
        detail = "cannot open DR metadata log for append";
        error_code = DisasterRecoveryErrorCode::METADATA_PERSISTENCE_FAILED;
        return false;
    }
    metadata_check.flush();
    metadata_check.close();
    if (metadata_check.fail()) {
        detail = "failed to verify DR metadata log";
        error_code = DisasterRecoveryErrorCode::METADATA_PERSISTENCE_FAILED;
        return false;
    }
#ifdef THEMIS_TEST_BUILD
    if (test_quorum_override_) {
        if (!*test_quorum_override_) {
            detail = "cluster has no quorum";
            error_code = DisasterRecoveryErrorCode::QUORUM_UNAVAILABLE;
            return false;
        }
        detail = "test quorum precheck passed";
        return true;
    }
#endif
    if (!replication_mgr_) {
        detail = "replication manager required for quorum precheck";
        error_code = DisasterRecoveryErrorCode::NOT_CONFIGURED;
        return false;
    }

    if (config_.require_quorum && !replication_mgr_->hasQuorum()) {
        detail = "cluster has no quorum";
        error_code = DisasterRecoveryErrorCode::QUORUM_UNAVAILABLE;
        return false;
    }

    const auto health = replication_mgr_->getClusterHealth();
    const bool any_healthy = std::any_of(health.begin(), health.end(),
        [](const auto& pair) { return pair.second; });
    if (!any_healthy) {
        detail = "no healthy nodes visible in cluster health";
        error_code = DisasterRecoveryErrorCode::QUORUM_UNAVAILABLE;
        return false;
    }

    detail = "prechecks passed";
    return true;
}

/**
 * @brief Apply Epoch Fencing.
 * @param[in] plan Input parameter.
 * @param[in,out] detail Input/output parameter.
 * @param[in,out] fenced_epoch Input/output parameter.
 * @return True when the operation succeeds.
 * @details Calls: bumpEpoch(), std::to_string().
 */
bool DisasterRecoveryManager::applyEpochFencing(const DisasterRecoveryPlan& plan,
                                                std::string& detail,
                                                uint64_t& fenced_epoch,
                                                DisasterRecoveryErrorCode& error_code) {
    if (plan.dry_run) {
        detail = "dry-run epoch fencing skipped";
        return true;
    }

    if (!config_.enforce_epoch_fencing) {
        detail = "epoch fencing disabled by config";
        error_code = DisasterRecoveryErrorCode::FENCING_UNAVAILABLE;
        return false;
    }

    if (!fencing_mgr_) {
        detail = "epoch fencing manager required";
        error_code = DisasterRecoveryErrorCode::NOT_CONFIGURED;
        return false;
    }

    const auto token = fencing_mgr_->bumpEpoch("disaster-recovery: " + plan.plan_id);
    fenced_epoch = token.epoch;
    if (fenced_epoch == 0) {
        detail = "fencing returned invalid epoch";
        error_code = DisasterRecoveryErrorCode::FENCING_UNAVAILABLE;
        return false;
    }

    detail = "epoch fenced at " + std::to_string(fenced_epoch);
    return true;
}

void DisasterRecoveryManager::transitionState(DisasterRecoveryState next) noexcept {
    state_.store(next);
}

/**
 * @brief Update Statistics.
 * @param[in] result Input parameter.
 * @details Calls: lock(), push_back(), std::accumulate(), begin(), end(), std::chrono::milliseconds(), size().
 */
void DisasterRecoveryManager::updateStatistics(const DisasterRecoveryResult& result) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.total_runs++;

    if (result.final_state == DisasterRecoveryState::ABORTED) {
        stats_.aborted_runs++;
    } else if (result.success) {
        stats_.successful_runs++;
    } else {
        stats_.failed_runs++;
    }

    durations_.push_back(result.duration);
    const auto total_ms = std::accumulate(
        durations_.begin(), durations_.end(), std::chrono::milliseconds(0));
    stats_.average_duration = total_ms / static_cast<int64_t>(durations_.size());
}

}  // namespace failover
}  // namespace themis
