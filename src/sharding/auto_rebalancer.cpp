/**
 * @file auto_rebalancer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "sharding/auto_rebalancer.h"
#include "sharding/shard_topology.h"
#include "sharding/prometheus_metrics.h"
#include "sharding/predictive_detector.h"
#include "utils/audit_logger.h"
#include "utils/logger.h"
#include "utils/tracing.h"
#include <sstream>
#include <iomanip>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/sha.h>
#include <openssl/rsa.h>
#include <fstream>
#include <chrono>
#include <stdexcept>

namespace themis {
namespace sharding {

// ─────────────────────────────────────────────────────────────────────────────
// HotShardSplitPolicy
// ─────────────────────────────────────────────────────────────────────────────

HotShardSplitPolicy::HotShardSplitPolicy(std::shared_ptr<ShardLoadDetector> detector)
    : detector_(std::move(detector)), config_(Config{}) {}

HotShardSplitPolicy::HotShardSplitPolicy(
    std::shared_ptr<ShardLoadDetector> detector,
    const Config& config
) : detector_(std::move(detector)), config_(config) {}

/**
 * @brief Set Predictive Detector.
 * @param[in,out] pd Input/output parameter.
 * @details Implements setPredictiveDetector without additional internal calls.
 */
void HotShardSplitPolicy::setPredictiveDetector(
    themisdb::sharding::PredictiveFailureDetector* pd
) {
    // Non-owning assignment: the caller is responsible for ensuring that 'pd'
    // remains valid for at least as long as this HotShardSplitPolicy object.
    predictive_detector_ = pd;
}

std::vector<HotShardSplitPolicy::SplitProposal> HotShardSplitPolicy::evaluate() const {
    if (!detector_) {
        return {};
    }

    std::vector<SplitProposal> proposals;

    const auto all_loads = detector_->getAllShardLoads();

    for (const auto& [shard_id, load] : all_loads) {
        SplitProposal proposal;
        proposal.hot_shard_id = shard_id;

        // ── Reactive check: current CPU or storage already over threshold ──
        const bool cpu_hot     = (load.cpu_usage_percent / 100.0) >= config_.cpu_split_threshold;
        const bool storage_hot = (load.storage_usage_percent / 100.0) >= config_.storage_split_threshold;

        if (cpu_hot || storage_hot) {
            if (cpu_hot) {
                proposal.reason = "CPU " + std::to_string(static_cast<int>(load.cpu_usage_percent)) + "%";
            }
            if (storage_hot) {
                if (!proposal.reason.empty()) {
                  proposal.reason += ", ";
                }
                proposal.reason += "storage " + std::to_string(static_cast<int>(load.storage_usage_percent)) + "%";
            }
            proposal.reason = "Reactive split: " + proposal.reason +
                              " exceeds " +
                              std::to_string(static_cast<int>(config_.cpu_split_threshold * 100)) + "% threshold";
            proposal.current_load_percent   = std::max(load.cpu_usage_percent, load.storage_usage_percent);
            proposal.predicted_load_percent = proposal.current_load_percent;
            proposal.is_predictive          = false;
            proposals.push_back(proposal);
            continue;  // No need to check predictive path for already-hot shard
        }

        // ── Predictive check: forecast exceeds threshold ──
        if (config_.enable_predictive_splitting) {
            auto forecast = detector_->forecastLoad(shard_id, config_.forecast_horizon);
            if (forecast && forecast->predicted_composite_load >= config_.predictive_load_threshold) {
                proposal.reason = "Predictive split: forecast composite load " +
                                  std::to_string(static_cast<int>(forecast->predicted_composite_load)) +
                                  "/100 in " + std::to_string(config_.forecast_horizon.count()) +
                                  " min (threshold " +
                                  std::to_string(static_cast<int>(config_.predictive_load_threshold)) + ")";
                // Report the last observed composite load as current_load_percent so
                // the caller can distinguish current vs. forecasted load in logging.
                proposal.current_load_percent   = forecast->predicted_composite_load
                                                      - forecast->confidence_interval;
                proposal.predicted_load_percent = forecast->predicted_composite_load;
                proposal.is_predictive          = true;
                proposals.push_back(proposal);
                continue;  // ML-based check not needed if statistical path already fired
            }
        }

        // ── ML-based predictive check via PredictiveFailureDetector ──────────
        // When a PredictiveFailureDetector is attached, consult its ML-model
        // predictions for the shard.  A high failure probability indicates the
        // shard is under significant stress (I/O errors, latency spikes, etc.)
        // and should be split pre-emptively to reduce load before saturation or
        // hardware failure occurs.
        if (config_.enable_ml_predictive_splitting && predictive_detector_) {
            try {
                auto pred = predictive_detector_->predictShard(shard_id);
                if (pred.failure_probability >= config_.failure_probability_threshold) {
                    proposal.reason = "ML-based predictive split: PredictiveFailureDetector "
                                      "failure probability " +
                                      std::to_string(static_cast<int>(pred.failure_probability * 100)) +
                                      "% >= threshold " +
                                      std::to_string(static_cast<int>(
                                          config_.failure_probability_threshold * 100)) + "%";
                    proposal.current_load_percent   = load.cpu_usage_percent;
                    proposal.predicted_load_percent = static_cast<double>(pred.failure_probability) * 100.0;
                    proposal.is_predictive          = true;
                    proposals.push_back(proposal);
                }
            } catch (const std::exception& ex) {
                THEMIS_WARN("HotShardSplitPolicy: PredictiveFailureDetector threw for {}: {}",
                            shard_id, ex.what());
            }
        }
    }

    return proposals;
}

// ─────────────────────────────────────────────────────────────────────────────
// AutoRebalancer
// ─────────────────────────────────────────────────────────────────────────────

AutoRebalancer::AutoRebalancer(
    std::shared_ptr<ShardTopology> topology,
    std::shared_ptr<ShardLoadDetector> load_detector,
    std::shared_ptr<PrometheusMetrics> metrics,
    MigrationExecutor migration_executor
) : AutoRebalancer(std::move(topology), std::move(load_detector), std::move(metrics),
                   std::move(migration_executor), Config{}) {}

AutoRebalancer::AutoRebalancer(
    std::shared_ptr<ShardTopology> topology,
    std::shared_ptr<ShardLoadDetector> load_detector,
    std::shared_ptr<PrometheusMetrics> metrics,
    MigrationExecutor migration_executor,
    const Config& config
) : topology_(std::move(topology)),
    load_detector_(std::move(load_detector)),
    metrics_(std::move(metrics)),
    migration_executor_(std::move(migration_executor)),
    config_(config),
    last_check_time_(std::chrono::system_clock::time_point::min()) {
    if (!migration_executor_) {
        throw std::invalid_argument("AutoRebalancer requires a migration executor");
    }

    THEMIS_INFO("AutoRebalancer initialized with check_interval={}s, max_concurrent={}",
               config_.check_interval.count() / 1000, config_.max_concurrent_operations);
}

AutoRebalancer::~AutoRebalancer() {
    stop();
}

/**
 * @brief Start.
 * @details Calls: exchange(), THEMIS_WARN(), THEMIS_INFO(), std::thread(), monitorLoop(), setGauge().
 */
void AutoRebalancer::start() {
    std::lock_guard<std::mutex> lifecycle_lock(monitor_lifecycle_mutex_);
    if (running_.exchange(true)) {
        THEMIS_WARN("AutoRebalancer already running");
        return;
    }
    
    THEMIS_INFO("Starting AutoRebalancer monitoring loop");
    
    try {
        monitor_thread_ = std::thread([this]() { monitorLoop(); });
    } catch (...) {
        running_ = false;
        throw;
    }
    
    if (metrics_) {
        metrics_->setGauge("themis_auto_rebalancer_running", 1.0);
    }
}

/**
 * @brief Stop.
 */
void AutoRebalancer::stop() {
    std::lock_guard<std::mutex> lifecycle_lock(monitor_lifecycle_mutex_);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_.exchange(false)) {
            return;
        }
        check_requested_ = false;
    }
    
    THEMIS_INFO("Stopping AutoRebalancer");
    
    cv_.notify_all();
    
    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    }
    
    if (metrics_) {
        metrics_->setGauge("themis_auto_rebalancer_running", 0.0);
    }
    
    THEMIS_INFO("AutoRebalancer stopped");
}

/**
 * @brief Monitor Loop.
 */
void AutoRebalancer::monitorLoop() {
    THEMIS_INFO("AutoRebalancer monitor loop started");
    
    while (running_.load()) {
        auto span = Tracer::startSpan("AutoRebalancer.monitorTick");
        
        try {
            total_checks_++;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                last_check_time_ = std::chrono::system_clock::now();
            }
            
            // Check for topology changes (node join/leave) and handle automatic rebalancing
            handleTopologyChange();
            
            // Check if we can trigger new rebalances
            if (!canTriggerRebalance()) {
                THEMIS_DEBUG("Cannot trigger rebalance (safety limits or cooldown)");
                span.setAttribute("can_trigger", false);
            } else {
                // Detect imbalance
                auto imbalance = load_detector_->detectImbalance();
                
                span.setAttribute("imbalance_detected", imbalance.is_imbalanced);
                bool submitted = false;
                if (imbalance.is_imbalanced) {
                    THEMIS_WARN("Load imbalance detected: {}", imbalance.reason);
                    span.setAttribute("imbalance_reason", imbalance.reason);
                    span.setAttribute("recommendations", static_cast<int64_t>(imbalance.recommendations.size()));
                    
                    // Check safety limits
                    if (!isWithinSafetyLimits(imbalance)) {
                        THEMIS_ERROR("Rebalance exceeds safety limits, skipping");
                        span.recordError("Safety limits exceeded");
                    } else {
                        for (const auto& rec : imbalance.recommendations) {
                            submitted |= submitRebalance({rec, false});
                        }
                    }
                }
                // Evaluate splits before applying the cooldown for this tick.
                submitted |= evaluateAndExecuteSplits();
                if (submitted) {
                    load_detector_->recordRebalanceTriggered();
                }
                span.setAttribute("rebalance_submitted", submitted);
            }
            
        } catch (const std::exception& e) {
            THEMIS_ERROR("Error in AutoRebalancer monitor loop: {}", e.what());
            span.recordError(e.what());
            
            if (metrics_) {
                metrics_->incrementCounter("themis_auto_rebalancer_errors_total");
            }
        }
        
        // Wait for next interval or shutdown
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait_for(lock, config_.check_interval, [this] {
            return !running_.load() || check_requested_;
        });
        check_requested_ = false;
    }
    
    THEMIS_INFO("AutoRebalancer monitor loop stopped");
}

bool AutoRebalancer::submitRebalance(PlannedOperation plan) {
    const auto op_id = generateOperationId();
    if (config_.require_manual_approval) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_ || !hasExecutionCapacityLocked()) {
            return false;
        }
        const auto& rec = plan.recommendation;
        for (const auto& entry : pending_approvals_) {
            const auto& pending = entry.second.recommendation;
            if (pending.source_shard == rec.source_shard && pending.target_shard == rec.target_shard &&
                pending.token_range_start == rec.token_range_start &&
                pending.token_range_end == rec.token_range_end) {
                return false;
            }
        }
        pending_approvals_.emplace(op_id, std::move(plan));
        if (metrics_) {
            metrics_->incrementCounter("themis_rebalance_pending_approvals_total");
        }
        return true;
    }
    return config_.auto_trigger_enabled && executeRebalance(op_id, std::move(plan));
}

bool AutoRebalancer::executeRebalance(const std::string& op_id,
                                     std::optional<PlannedOperation> automatic_plan) {
    auto span = Tracer::startSpan("AutoRebalancer.executeRebalance");
    span.setAttribute("operation_id", op_id);

    size_t status_index;
    PlannedOperation plan;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!hasExecutionCapacityLocked() || (automatic_plan && !running_)) {
            return false;
        }
        const auto pending = pending_approvals_.find(op_id);
        if (!automatic_plan && pending == pending_approvals_.end()) {
            return false;
        }
        plan = automatic_plan ? std::move(*automatic_plan) : pending->second;
        OperationStatus status;
        status.operation_id = op_id;
        status.state = RebalanceState::IN_PROGRESS;
        status.start_time = std::chrono::system_clock::now();
        status.progress.start_time = status.start_time;
        status_index = operation_history_.size();
        operation_history_.push_back(std::move(status));
        if (!automatic_plan) {
            pending_approvals_.erase(pending);
        }
        ++active_operations_;
    }

    const auto& rec = plan.recommendation;
    std::optional<MigrationOutcome> outcome;
    std::string error;
    try {
        MigrationRequest request{op_id,
            {rec.source_shard, rec.target_shard, rec.token_range_start, rec.token_range_end,
             config_.operator_cert_path, config_.ca_cert_path, config_.batch_size,
             config_.verify_data, config_.enable_rollback}, {}};
        if (rec.source_shard == rec.target_shard) {
            throw std::invalid_argument("Source and target shard must differ");
        }
        RebalanceOperation admission(request.operation);
        request.operator_signature = signOperation(op_id);
        if (request.operator_signature.empty() || !admission.start(request.operator_signature)) {
            error = "Rebalance operator admission failed";
        } else {
            outcome = MigrationOutcome::OWNERSHIP_UNCERTAIN;
            ++triggered_operations_;
            auto result = migration_executor_(request);
            outcome = result.outcome;
            error = std::move(result.error_message);
        }
    } catch (const std::exception& e) {
        error = e.what();
    } catch (...) {
        error = "Unknown exception during rebalance execution";
    }

    const bool committed = outcome == MigrationOutcome::OWNERSHIP_COMMITTED;
    if (!committed && error.empty()) {
        error = outcome == MigrationOutcome::OWNERSHIP_UNCERTAIN
            ? "Ownership outcome uncertain; reconcile with the authoritative backend"
            : "Authoritative ownership commit not confirmed";
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& status = operation_history_[status_index];
        status.state = committed ? RebalanceState::COMPLETED : RebalanceState::FAILED;
        status.end_time = std::chrono::system_clock::now();
        status.migration_outcome = outcome;
        status.error_message = std::move(error);
        if (committed) {
            status.progress.progress_percent = 100.0;
            ++completed_operations_;
        } else {
            ++failed_operations_;
            span.recordError(status.error_message);
        }
        --active_operations_;
        if (metrics_) {
            if (outcome) {
                metrics_->incrementCounter("themis_rebalance_operations_triggered_total");
            }
            metrics_->setGauge("themis_rebalance_active_operations", static_cast<double>(active_operations_));
            metrics_->setGauge("themis_rebalance_completed_operations_total",
                              static_cast<double>(completed_operations_.load()));
            metrics_->setGauge("themis_rebalance_failed_operations_total",
                              static_cast<double>(failed_operations_.load()));
            if (plan.is_split) {
                metrics_->incrementCounter(committed ? "themis_shard_splits_total" :
                                                      "themis_shard_split_failures_total");
            }
        }
    }
    return outcome.has_value();
}

std::string AutoRebalancer::generateOperationId() const {
    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();
    
    std::ostringstream oss = {};
    oss << "rebalance_" << std::hex << now_ms << '_' << next_operation_sequence_++;
    return oss.str();
}

std::string AutoRebalancer::signOperation(const std::string& operation_id) const {
    SignOperationFn sign;
    {
        std::lock_guard<std::mutex> lock(sign_fn_mutex_);
        sign = sign_fn_;
    }
    if (sign) {
        return sign(operation_id);
    }
    // RSA-SHA256 signing using operator certificate
    // Enforce fail-closed signing (stub #310): when fail_closed_signing is enabled,
    // throw exception instead of returning empty/unsigned token on failure.
    
    // Load private key from operator certificate
    if (config_.operator_key_path.empty()) {
        THEMIS_ERROR("AutoRebalancer: No operator key configured for operation {}", operation_id);
        if (config_.fail_closed_signing) {
            throw std::runtime_error(
                "Rebalance operation signing failed (fail_closed_signing=true, missing key path): " + operation_id);
        }
        return {};
    }

    // Open and read private key file
    FILE* key_file = fopen(config_.operator_key_path.c_str(), "r");
    if (!key_file) {
        THEMIS_ERROR("AutoRebalancer: Cannot open operator key file: {}", config_.operator_key_path);
        if (config_.fail_closed_signing) {
            throw std::runtime_error(
                "Rebalance operation signing failed (fail_closed_signing=true, cannot open key): " + operation_id);
        }
        return {};
    }

    EVP_PKEY* pkey = PEM_read_PrivateKey(key_file, nullptr, nullptr, nullptr);
    fclose(key_file);

    if (!pkey) {
        THEMIS_ERROR("AutoRebalancer: Failed to parse operator private key");
        if (config_.fail_closed_signing) {
            throw std::runtime_error(
                "Rebalance operation signing failed (fail_closed_signing=true, cannot parse key): " + operation_id);
        }
        return {};
    }

    // Create canonical message to sign: "REBALANCE:{operation_id}:{timestamp}"
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()
    ).count();

    std::ostringstream msg_oss = {};
    msg_oss << "REBALANCE:" << operation_id << ":" << timestamp;
    std::string message = msg_oss.str();

    // Compute SHA-256 hash of the message
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(message.c_str()),
           message.size(), hash);

    // Sign the hash using RSA-SHA256 via EVP API
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pkey, nullptr);
    if (!ctx) {
        EVP_PKEY_free(pkey);
        THEMIS_ERROR("AutoRebalancer: Failed to create signing context");
        return {};
    }

    if (EVP_PKEY_sign_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        THEMIS_ERROR("AutoRebalancer: Failed to initialize signing");
        return {};
    }

    // Set padding mode to PKCS#1 v1.5
    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PADDING) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        THEMIS_ERROR("AutoRebalancer: Failed to set RSA padding");
        return {};
    }

    if (EVP_PKEY_CTX_set_signature_md(ctx, EVP_sha256()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        THEMIS_ERROR("AutoRebalancer: Failed to set signature digest");
        return {};
    }

    // Determine signature buffer size
    size_t sig_len = 0;
    if (EVP_PKEY_sign(ctx, nullptr, &sig_len, hash, SHA256_DIGEST_LENGTH) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        THEMIS_ERROR("AutoRebalancer: Failed to determine signature size");
        return {};
    }
    
    /**
     * @brief Allocate buffer and perform signing
     * @param[in] sig_len Input parameter.
     * @return Return value.
     */
    std::vector<unsigned char> signature(sig_len);
    if (EVP_PKEY_sign(ctx, signature.data(), &sig_len, hash, SHA256_DIGEST_LENGTH) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        THEMIS_ERROR("AutoRebalancer: Signing operation failed");
        return {};
    }
    signature.resize(sig_len);
    
    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    
    // Encode signature as Base64 using OpenSSL
    // Calculate required buffer size: ((input_len + 2) / 3) * 4 + 1 for null terminator
    size_t b64_len = ((signature.size() + 2) / 3) * 4 + 1;
    /**
     * @brief B64 buf.
     * @param[in] b64_len Input parameter.
     * @return Return value.
     */
    std::vector<unsigned char> b64_buf(b64_len);
    
    int encoded_len = EVP_EncodeBlock(b64_buf.data(), signature.data(), 
                                       signature.size());
    
    std::string sig_b64 = {};
    if (encoded_len > 0) {
        sig_b64 = std::string(reinterpret_cast<char*>(b64_buf.data()),
                              static_cast<size_t>(encoded_len));
    } else {
        THEMIS_ERROR("AutoRebalancer: Base64 encoding failed");
        return {};
    }
    
    // Return formatted signature: SIGNATURE:{sig_b64}:{timestamp}
    std::ostringstream result = {};
    result << "SIGNATURE:" << sig_b64 << ":" << timestamp;
    
    THEMIS_INFO("AutoRebalancer: Successfully signed operation {} (sig_len={})", 
                operation_id, sig_len);
    
    return result.str();
}

bool AutoRebalancer::canTriggerRebalance() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if load detector is in cooldown
    if (load_detector_->isInCooldown()) {
        return false;
    }
    return hasExecutionCapacityLocked();
}

bool AutoRebalancer::hasExecutionCapacityLocked() const {
    // Check max concurrent operations
    if (active_operations_ >= config_.max_concurrent_operations) {
        return false;
    }
    
    // Check daily limit
    auto now = std::chrono::system_clock::now();
    auto day_start = now - std::chrono::hours(24);
    
    size_t operations_today = 0;
    for (const auto& status : operation_history_) {
        if (status.start_time > day_start) {
            operations_today++;
        }
    }
    
    if (operations_today >= config_.max_operations_per_day) {
        THEMIS_WARN("Daily operation limit reached ({}/{})", 
                   operations_today, config_.max_operations_per_day);
        return false;
    }
    
    return true;
}

bool AutoRebalancer::isWithinSafetyLimits(const LoadImbalanceResult& imbalance) const {
    // Check if total data movement is within limits
    // Simplified - in production, calculate actual data size
    
    if (imbalance.recommendations.empty()) {
        return false;
    }
    
    // For now, allow if we have reasonable recommendations
    return imbalance.recommendations.size() <= config_.max_concurrent_operations * 2;
}

/**
 * @brief Trigger Check.
 * @return True when the operation succeeds.
 * @details Calls: load(), THEMIS_WARN(), THEMIS_INFO(), notify_one().
 */
bool AutoRebalancer::triggerCheck() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_) {
            return false;
        }
        check_requested_ = true;
    }
    
    THEMIS_INFO("Manual rebalance check triggered");
    cv_.notify_one();
    return true;
}

/**
 * @brief Approve Operation.
 * @param[in] operation_id Identifier of the operation.
 * @return True when the operation succeeds.
 */
bool AutoRebalancer::approveOperation(const std::string& operation_id) {
    return executeRebalance(operation_id, std::nullopt);
}

/**
 * @brief Cancel Operation.
 * @param[in] operation_id Identifier of the operation.
 * @return True when the operation succeeds.
 */
bool AutoRebalancer::cancelOperation(const std::string& operation_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (pending_approvals_.erase(operation_id) == 0) {
        return false;
    }
    if (metrics_) {
        metrics_->incrementCounter("themis_rebalance_operations_cancelled_total");
    }
    return true;
}

std::vector<AutoRebalancer::OperationStatus> AutoRebalancer::getOperationStatuses() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<OperationStatus> statuses = operation_history_;
    
    for (const auto& pending : pending_approvals_) {
        OperationStatus status;
        status.operation_id = pending.first;
        statuses.push_back(std::move(status));
    }
    
    return statuses;
}

nlohmann::json AutoRebalancer::getStatistics() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    
    nlohmann::json stats;
    stats["running"] = running_.load();
    stats["total_checks"] = total_checks_.load();
    stats["triggered_operations"] = triggered_operations_.load();
    stats["completed_operations"] = completed_operations_.load();
    stats["failed_operations"] = failed_operations_.load();
    stats["split_proposals_total"] = split_proposals_total_.load();
    stats["active_operations"] = active_operations_;
    stats["pending_approvals"] = pending_approvals_.size();
    
    if (last_check_time_ != std::chrono::system_clock::time_point::min()) {
        auto now = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            now - last_check_time_
        );
        stats["seconds_since_last_check"] = elapsed.count();
    }
    
    return stats;
}

/**
 * @brief Set Split Policy.
 * @param[in] policy Input parameter.
 * @details Calls: lock(), std::move().
 */
void AutoRebalancer::setSplitPolicy(std::shared_ptr<HotShardSplitPolicy> policy) {
    std::lock_guard<std::mutex> lock(mutex_);
    split_policy_ = std::move(policy);
}

/**
 * @brief Set Audit Logger.
 * @param[in] audit_logger Input parameter.
 * @details Calls: lock(), std::move().
 */
void AutoRebalancer::setAuditLogger(
    std::shared_ptr<themis::utils::AuditLogger> audit_logger
) {
    std::lock_guard<std::mutex> lock(mutex_);
    audit_logger_ = std::move(audit_logger);
}

/**
 * @brief ───────────────────────────────────────────────────────────────────────────── Hot-shard split evaluation and execution ─────────────────────────────────────────────────────────────────────────────
 * @details Calls: lock(), evaluate(), empty(), THEMIS_DEBUG(), THEMIS_INFO(), size(), setGauge(), load().
 */

bool AutoRebalancer::evaluateAndExecuteSplits() {
    std::shared_ptr<HotShardSplitPolicy> policy;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        policy = split_policy_;
    }

    if (!policy) {
        return false;
    }

    const auto proposals = policy->evaluate();
    if (proposals.empty()) {
        THEMIS_DEBUG("HotShardSplitPolicy: no split proposals");
        return false;
    }

    THEMIS_INFO("HotShardSplitPolicy: {} split proposal(s) generated",proposals.size());
    split_proposals_total_ += proposals.size();

    if (metrics_) {
        metrics_->setGauge("themis_split_proposals_total",
                           static_cast<double>(split_proposals_total_.load()));
    }

    bool submitted = false;
    for (const auto& proposal : proposals) {
        if (!canTriggerRebalance()) {
            THEMIS_WARN("HotShardSplitPolicy: safety limits reached; deferring split for {}",
                        proposal.hot_shard_id);
            break;
        }

        submitted |= executeSplitProposal(proposal);
    }
    return submitted;
}

/**
 * @brief Execute Split Proposal.
 * @param[in] proposal Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: Tracer::startSpan(), setAttribute(), THEMIS_INFO(), lock(), logSecurityEvent(), THEMIS_WARN(), what(), detectImbalance().
 */
bool AutoRebalancer::executeSplitProposal(const HotShardSplitPolicy::SplitProposal& proposal) {
    auto span = Tracer::startSpan("AutoRebalancer.executeSplitProposal");
    span.setAttribute("hot_shard", proposal.hot_shard_id);
    span.setAttribute("is_predictive", proposal.is_predictive);

    THEMIS_INFO("Evaluating shard split for {} – {} (current={:.1f}%, predicted={:.1f}%)",
                proposal.hot_shard_id, proposal.reason,
                proposal.current_load_percent, proposal.predicted_load_percent);

    // Emit SHARD_SPLIT audit event for compliance trail
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (audit_logger_) {
            nlohmann::json details;
            details["hot_shard_id"]           = proposal.hot_shard_id;
            details["reason"]                 = proposal.reason;
            details["current_load_percent"]   = proposal.current_load_percent;
            details["predicted_load_percent"] = proposal.predicted_load_percent;
            details["is_predictive"]          = proposal.is_predictive;

            try {
                audit_logger_->logSecurityEvent(
                    themis::utils::SecurityEventType::SHARD_SPLIT,
                    "auto_rebalancer",
                    proposal.hot_shard_id,
                    details
                );
            } catch (const std::exception& ex) {
                THEMIS_WARN("AutoRebalancer: audit log failed for split {}: {}",
                            proposal.hot_shard_id, ex.what());
            }
        }
    }

    // Translate the split proposal into a standard rebalance recommendation.
    // Splitting a hot shard means moving ~50 % of its token range to a new
    // (currently empty or lightly loaded) shard.  The load detector's cold
    // shard list is consulted; if no cold shard is available the split is
    // deferred until one is.
    const auto imbalance = load_detector_->detectImbalance();
    std::string target_shard = {};

    if (!imbalance.cold_shards.empty()) {
        target_shard = imbalance.cold_shards.front();
    } else {
        THEMIS_WARN("AutoRebalancer: no cold shard available for split of {}; deferring",
                    proposal.hot_shard_id);
        span.setAttribute("deferred", true);
        return false;
    }

    LoadImbalanceResult::RebalanceRecommendation rec;
    rec.source_shard = proposal.hot_shard_id;
    rec.target_shard = target_shard;

    // Derive the split token range from the hot shard's actual topology entry.
    // We migrate the upper half of whatever range the hot shard owns so that
    // the migration is always within the shard's authoritative range.
    const auto shard_info = topology_ ? topology_->getShard(proposal.hot_shard_id) : std::nullopt;
    if (!shard_info || shard_info->token_start >= shard_info->token_end ||
        target_shard == proposal.hot_shard_id) {
        THEMIS_WARN("AutoRebalancer: no valid topology range or target for split of {}; deferring",
                    proposal.hot_shard_id);
        return false;
    }
    const auto shard_token_start = shard_info->token_start;
    const auto shard_token_end = shard_info->token_end;
    const uint64_t midpoint = shard_token_start + (shard_token_end - shard_token_start) / 2;
    rec.token_range_start = midpoint;
    rec.token_range_end   = shard_token_end;
    rec.expected_load_reduction_percent = 50.0;
    rec.justification = "Hot-shard split – " + proposal.reason;

    const bool submitted = submitRebalance({std::move(rec), true});
    span.setAttribute("submitted", submitted);
    return submitted;
}

/**
 * @brief Set Sign Operation Fn.
 * @param[in] fn Input parameter.
 * @details Calls: lock(), std::move().
 */
void AutoRebalancer::setSignOperationFn(SignOperationFn fn) {
    std::lock_guard<std::mutex> lock(sign_fn_mutex_);
    sign_fn_ = std::move(fn);
}

/**
 * @brief Handle Topology Change.
 * @details Calls: getAllShards(), push_back(), size(), THEMIS_WARN(), canTriggerRebalance(), THEMIS_DEBUG(), THEMIS_INFO(), incrementCounter().
 */
void AutoRebalancer::handleTopologyChange() {
    if (!topology_) {
        return; // No topology to monitor
    }
    
    // Get current shard IDs from topology
    auto all_shards = topology_->getAllShards();
    std::vector<std::string> current_topology = {};

    for (const auto& shard : all_shards) {
        current_topology.push_back(shard.shard_id);
    }
    
    // Check if topology has changed since last check
    if (current_topology == last_known_topology_) {
        return; // No topology change
    }
    
    // Topology has changed - detect join or leave
    bool is_join = current_topology.size() > last_known_topology_.size();
    bool is_leave = current_topology.size() <last_known_topology_.size();
    
    THEMIS_WARN("Topology change detected: {} (was {}, now {} nodes)",
               is_join ? "JOIN" : (is_leave ? "LEAVE" : "UNKNOWN"),
               last_known_topology_.size(),current_topology.size());
    
    if (!config_.auto_trigger_enabled) {
        last_known_topology_ = current_topology;
        return; // Auto-trigger disabled, skip rebalancing
    }
    
    // Check if we can trigger rebalance
    if (!canTriggerRebalance()) {
        THEMIS_DEBUG("Cannot trigger topology rebalance (safety limits or cooldown)");
        return;
    }
    
    // Topology observation is not a migration plan; the load/split planners
    // can submit moves on this tick through the normal approval/dispatch path.
    
    topology_change_count_++;
    last_known_topology_ = current_topology;
    
    if (metrics_) {
        metrics_->incrementCounter("themis_topology_changes_total");
        metrics_->setGauge("themis_cluster_nodes", 
                           static_cast<double>(current_topology.size()));
    }
}

} // namespace sharding
} // namespace themis
