/**
 * @file auto_rebalancer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=5, H=4, M=7, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "sharding/auto_rebalancer.h"
#include "sharding/shard_topology.h"
#include "sharding/prometheus_metrics.h"
#include "sharding/data_migrator.h"
#include "sharding/predictive_detector.h"
#include "utils/audit_logger.h"
#include "utils/logger.h"
#include "utils/tracing.h"
#include "utils/thread_join_utils.h"
#include <sstream>
#include <iomanip>
#include <set>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/sha.h>
#include <openssl/rsa.h>
#include <fstream>
#include <chrono>

namespace themis {
namespace sharding {

// ─────────────────────────────────────────────────────────────────────────────
// HotShardSplitPolicy
// ─────────────────────────────────────────────────────────────────────────────

/** @brief Construct split policy with default thresholds. */
HotShardSplitPolicy::HotShardSplitPolicy(std::shared_ptr<ShardLoadDetector> detector)
    : detector_(std::move(detector)), config_(Config{}) {}

/** @brief Construct split policy with explicit thresholds. */
HotShardSplitPolicy::HotShardSplitPolicy(
    std::shared_ptr<ShardLoadDetector> detector,
    const Config& config
) : detector_(std::move(detector)), config_(config) {}

/** @brief Attach optional non-owning ML predictive detector. */
void HotShardSplitPolicy::setPredictiveDetector(
    themisdb::sharding::PredictiveFailureDetector* pd
) {
    // Non-owning assignment: the caller is responsible for ensuring that 'pd'
    // remains valid for at least as long as this HotShardSplitPolicy object.
    predictive_detector_ = pd;
}

/** @brief Evaluate reactive/statistical/ML split triggers and return proposals. */
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
                if (!proposal.reason.empty()) proposal.reason += ", ";
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

/** @brief Construct auto rebalancer with default config. */
AutoRebalancer::AutoRebalancer(
    std::shared_ptr<ShardTopology> topology,
    std::shared_ptr<ShardLoadDetector> load_detector,
    std::shared_ptr<PrometheusMetrics> metrics,
    std::shared_ptr<DataMigrator> migrator
) : AutoRebalancer(topology, load_detector, metrics, migrator, Config{}) {}

/** @brief Construct auto rebalancer with explicit config. */
AutoRebalancer::AutoRebalancer(
    std::shared_ptr<ShardTopology> topology,
    std::shared_ptr<ShardLoadDetector> load_detector,
    std::shared_ptr<PrometheusMetrics> metrics,
    std::shared_ptr<DataMigrator> migrator,
    const Config& config
) : topology_(topology),
    load_detector_(load_detector),
    metrics_(metrics),
    migrator_(migrator),
    config_(config),
    last_check_time_(std::chrono::system_clock::time_point::min()) {
    
    THEMIS_INFO("AutoRebalancer initialized with check_interval={}s, max_concurrent={}",
               config_.check_interval.count() / 1000, config_.max_concurrent_operations);
}

/** @brief Stop monitor thread and active operations on destruction. */
AutoRebalancer::~AutoRebalancer() {
    stop();
}

/** @brief Start periodic monitor loop thread. */
void AutoRebalancer::start() {
    if (running_.exchange(true)) {
        THEMIS_WARN("AutoRebalancer already running");
        return;
    }
    
    THEMIS_INFO("Starting AutoRebalancer monitoring loop");
    
    monitor_thread_ = std::thread([this]() {
        monitorLoop();
    });
    
    if (metrics_) {
        metrics_->setGauge("themis_auto_rebalancer_running", 1.0);
    }
}

/** @brief Stop monitor loop thread and update running gauge. */
void AutoRebalancer::stop() {
    if (!running_.exchange(false)) {
        return;
    }
    
    THEMIS_INFO("Stopping AutoRebalancer");
    
    cv_.notify_all();
    
    if (monitor_thread_.joinable()) {
        const bool joined = themis::utils::joinThreadWithin(monitor_thread_);
        if (!joined) {
            THEMIS_WARN("AutoRebalancer: monitor thread join timed out");
        }
    }
    
    if (metrics_) {
        metrics_->setGauge("themis_auto_rebalancer_running", 0.0);
    }
    
    THEMIS_INFO("AutoRebalancer stopped");
}

/** @brief Monitor loop: detect imbalance, enforce safety limits, dispatch operations. */
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
            
            // Cleanup completed operations
            cleanupCompletedOperations();
            
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
                
                if (imbalance.is_imbalanced) {
                    THEMIS_WARN("Load imbalance detected: {}", imbalance.reason);
                    span.setAttribute("imbalance_reason", imbalance.reason);
                    span.setAttribute("recommendations", static_cast<int64_t>(imbalance.recommendations.size()));
                    
                    // Check safety limits
                    if (!isWithinSafetyLimits(imbalance)) {
                        THEMIS_ERROR("Rebalance exceeds safety limits, skipping");
                        span.recordError("Safety limits exceeded");
                    } else {
                        // Execute rebalance operations
                        for (const auto& rec : imbalance.recommendations) {
                            // Check max concurrent operations
                            {
                                std::lock_guard<std::mutex> lock(mutex_);
                                if (active_operations_.size() >= config_.max_concurrent_operations) {
                                    THEMIS_WARN("Max concurrent operations reached, queuing remaining");
                                    break;
                                }
                            }
                            
                            if (config_.require_manual_approval) {
                                // Queue for approval
                                std::string op_id = generateOperationId();
                                std::lock_guard<std::mutex> lock(mutex_);
                                pending_approvals_[op_id] = rec;
                                
                                THEMIS_INFO("Rebalance operation queued for approval: {}", op_id);
                                
                                if (metrics_) {
                                    metrics_->incrementCounter("themis_rebalance_pending_approvals_total");
                                }
                            } else if (config_.auto_trigger_enabled) {
                                // Execute automatically
                                executeRebalance(rec);
                            }
                        }
                        
                        // Record trigger
                        load_detector_->recordRebalanceTriggered();
                    }
                }

                // Evaluate hot-shard splits (reactive + predictive)
                evaluateAndExecuteSplits();
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
            return !running_.load();
        });
    }
    
    THEMIS_INFO("AutoRebalancer monitor loop stopped");
}

/** @brief Execute one rebalance recommendation by creating and starting operation object. */
bool AutoRebalancer::executeRebalance(const LoadImbalanceResult::RebalanceRecommendation& recommendation) {
    auto span = Tracer::startSpan("AutoRebalancer.executeRebalance");
    span.setAttribute("source_shard", recommendation.source_shard);
    span.setAttribute("target_shard", recommendation.target_shard);
    
    std::string op_id = generateOperationId();
    
    THEMIS_INFO("Executing rebalance operation: {} (source={}, target={})",
               op_id, recommendation.source_shard, recommendation.target_shard);
    
    // Create rebalance operation config
    RebalanceOperationConfig op_config;
    op_config.source_shard_id = recommendation.source_shard;
    op_config.target_shard_id = recommendation.target_shard;
    op_config.token_range_start = recommendation.token_range_start;
    op_config.token_range_end = recommendation.token_range_end;
    op_config.operator_cert_path = config_.operator_cert_path;
    op_config.ca_cert_path = config_.ca_cert_path;
    op_config.batch_size = config_.batch_size;
    op_config.verify_data = config_.verify_data;
    op_config.enable_rollback = config_.enable_rollback;
    
    // Create operation
    auto operation = std::make_unique<RebalanceOperation>(op_config);
    
    // Sign operation (fail closed when signing is unavailable)
    std::string signature = signOperation(op_id);
    if (signature.empty()) {
        THEMIS_ERROR("Failed to sign rebalance operation: {}", op_id);
        span.recordError("Operation signing failed");
        return false;
    }
    
    // Start operation
    bool started = operation->start(signature);
    
    if (!started) {
        THEMIS_ERROR("Failed to start rebalance operation: {}", op_id);
        span.recordError("Operation start failed");
        return false;
    }
    
    // Track operation
    {
        std::lock_guard<std::mutex> lock(mutex_);
        active_operations_[op_id] = std::move(operation);
        
        OperationStatus status;
        status.operation_id = op_id;
        status.state = RebalanceState::IN_PROGRESS;
        status.start_time = std::chrono::system_clock::now();
        operation_history_.push_back(status);
    }
    
    triggered_operations_++;
    
    if (metrics_) {
        metrics_->incrementCounter("themis_rebalance_operations_triggered_total");
        metrics_->setGauge("themis_rebalance_active_operations", 
                          static_cast<double>(active_operations_.size()));
    }
    
    THEMIS_INFO("Rebalance operation started: {}", op_id);
    span.setAttribute("operation_id", op_id);
    
    return true;
}

/** @brief Generate operation id from current wall-clock milliseconds. */
std::string AutoRebalancer::generateOperationId() const {
    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();
    
    std::ostringstream oss;
    oss << "rebalance_" << std::hex << now_ms;
    return oss.str();
}

/** @brief Sign rebalance operation id using configured private key material. */
std::string AutoRebalancer::signOperation(const std::string& operation_id) const {
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

    std::ostringstream msg_oss;
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
    
    // Allocate buffer and perform signing
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
    std::vector<unsigned char> b64_buf(b64_len);
    
    int encoded_len = EVP_EncodeBlock(b64_buf.data(), signature.data(), 
                                       static_cast<int>(signature.size()));
    
    std::string sig_b64;
    if (encoded_len > 0) {
        sig_b64 = std::string(reinterpret_cast<char*>(b64_buf.data()),
                              static_cast<size_t>(encoded_len));
    } else {
        THEMIS_ERROR("AutoRebalancer: Base64 encoding failed");
        return {};
    }
    
    // Return formatted signature: SIGNATURE:{sig_b64}:{timestamp}
    std::ostringstream result;
    result << "SIGNATURE:" << sig_b64 << ":" << timestamp;
    
    THEMIS_INFO("AutoRebalancer: Successfully signed operation {} (sig_len={})", 
                operation_id, sig_len);
    
    return result.str();
}

/** @brief Check cooldown, concurrency and daily-rate limits for new operations. */
bool AutoRebalancer::canTriggerRebalance() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if load detector is in cooldown
    if (load_detector_->isInCooldown()) {
        return false;
    }
    
    // Check max concurrent operations
    if (active_operations_.size() >= config_.max_concurrent_operations) {
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

/** @brief Validate imbalance recommendation batch against safety constraints. */
bool AutoRebalancer::isWithinSafetyLimits(const LoadImbalanceResult& imbalance) const {
    // Check if total data movement is within limits
    // Simplified - in production, calculate actual data size
    
    if (imbalance.recommendations.empty()) {
        return false;
    }
    
    // For now, allow if we have reasonable recommendations
    return imbalance.recommendations.size() <= config_.max_concurrent_operations * 2;
}

/** @brief Remove completed operations from active map and update history/counters. */
void AutoRebalancer::cleanupCompletedOperations() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> completed_ids;
    
    for (auto& [op_id, operation] : active_operations_) {
        RebalanceState state = operation->getState();
        
        if (state == RebalanceState::COMPLETED || 
            state == RebalanceState::FAILED || 
            state == RebalanceState::ROLLED_BACK) {
            
            completed_ids.push_back(op_id);
            
            // Update statistics
            if (state == RebalanceState::COMPLETED) {
                completed_operations_++;
            } else {
                failed_operations_++;
            }
            
            // Update history
            for (auto& status : operation_history_) {
                if (status.operation_id == op_id) {
                    status.state = state;
                    status.end_time = std::chrono::system_clock::now();
                    status.progress = operation->getProgress();
                }
            }
        }
    }
    
    // Remove completed operations
    for (const auto& op_id : completed_ids) {
        THEMIS_INFO("Cleaning up completed rebalance operation: {}", op_id);
        active_operations_.erase(op_id);
    }
    
    if (metrics_) {
        metrics_->setGauge("themis_rebalance_active_operations", 
                          static_cast<double>(active_operations_.size()));
        metrics_->setGauge("themis_rebalance_completed_operations_total",
                          static_cast<double>(completed_operations_.load()));
        metrics_->setGauge("themis_rebalance_failed_operations_total",
                          static_cast<double>(failed_operations_.load()));
    }
}

/** @brief Trigger immediate monitor wake-up for manual rebalance check. */
bool AutoRebalancer::triggerCheck() {
    if (!running_.load()) {
        THEMIS_WARN("AutoRebalancer not running, cannot trigger check");
        return false;
    }
    
    THEMIS_INFO("Manual rebalance check triggered");
    cv_.notify_one();
    return true;
}

/** @brief Approve pending recommendation and execute associated operation. */
bool AutoRebalancer::approveOperation(const std::string& operation_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = pending_approvals_.find(operation_id);
    if (it == pending_approvals_.end()) {
        THEMIS_WARN("Operation not found in pending approvals: {}", operation_id);
        return false;
    }
    
    auto recommendation = it->second;
    pending_approvals_.erase(it);
    
    THEMIS_INFO("Rebalance operation approved: {}", operation_id);
    
    // Execute approved operation
    return executeRebalance(recommendation);
}

/** @brief Cancel active operation by invoking rollback path. */
bool AutoRebalancer::cancelOperation(const std::string& operation_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = active_operations_.find(operation_id);
    if (it == active_operations_.end()) {
        THEMIS_WARN("Operation not found: {}", operation_id);
        return false;
    }
    
    // Trigger rollback
    bool rolled_back = it->second->rollback();
    
    if (rolled_back) {
        THEMIS_INFO("Rebalance operation cancelled and rolled back: {}", operation_id);
        
        if (metrics_) {
            metrics_->incrementCounter("themis_rebalance_operations_cancelled_total");
        }
    } else {
        THEMIS_ERROR("Failed to rollback operation: {}", operation_id);
    }
    
    return rolled_back;
}

/** @brief Return merged historical+active status view for all operations. */
std::vector<AutoRebalancer::OperationStatus> AutoRebalancer::getOperationStatuses() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<OperationStatus> statuses = operation_history_;
    
    // Add current active operations
    for (const auto& [op_id, operation] : active_operations_) {
        bool found = false;
        for (auto& status : statuses) {
            if (status.operation_id == op_id) {
                status.state = operation->getState();
                status.progress = operation->getProgress();
                found = true;
                break;
            }
        }
        
        if (!found) {
            OperationStatus status;
            status.operation_id = op_id;
            status.state = operation->getState();
            status.progress = operation->getProgress();
            status.start_time = std::chrono::system_clock::now();
            statuses.push_back(status);
        }
    }
    
    return statuses;
}

/** @brief Return runtime statistics for monitor checks and operation outcomes. */
nlohmann::json AutoRebalancer::getStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    nlohmann::json stats;
    stats["running"] = running_.load();
    stats["total_checks"] = total_checks_.load();
    stats["triggered_operations"] = triggered_operations_.load();
    stats["completed_operations"] = completed_operations_.load();
    stats["failed_operations"] = failed_operations_.load();
    stats["split_proposals_total"] = split_proposals_total_.load();
    stats["active_operations"] = active_operations_.size();
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

/** @brief Install or replace hot-shard split policy. */
void AutoRebalancer::setSplitPolicy(std::shared_ptr<HotShardSplitPolicy> policy) {
    std::lock_guard<std::mutex> lock(mutex_);
    split_policy_ = std::move(policy);
}

/** @brief Install or replace audit logger used for split compliance events. */
void AutoRebalancer::setAuditLogger(
    std::shared_ptr<themis::utils::AuditLogger> audit_logger
) {
    std::lock_guard<std::mutex> lock(mutex_);
    audit_logger_ = std::move(audit_logger);
}

// ─────────────────────────────────────────────────────────────────────────────
// Hot-shard split evaluation and execution
// ─────────────────────────────────────────────────────────────────────────────

/** @brief Evaluate split proposals and execute those that pass safety checks. */
void AutoRebalancer::evaluateAndExecuteSplits() {
    std::shared_ptr<HotShardSplitPolicy> policy;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        policy = split_policy_;
    }

    if (!policy) {
        return;
    }

    const auto proposals = policy->evaluate();
    if (proposals.empty()) {
        THEMIS_DEBUG("HotShardSplitPolicy: no split proposals");
        return;
    }

    THEMIS_INFO("HotShardSplitPolicy: {} split proposal(s) generated", proposals.size());
    split_proposals_total_ += proposals.size();

    if (metrics_) {
        metrics_->setGauge("themis_split_proposals_total",
                           static_cast<double>(split_proposals_total_.load()));
    }

    for (const auto& proposal : proposals) {
        if (!canTriggerRebalance()) {
            THEMIS_WARN("HotShardSplitPolicy: safety limits reached; deferring split for {}",
                        proposal.hot_shard_id);
            break;
        }

        executeSplitProposal(proposal);
    }
}

/** @brief Execute one split proposal by mapping it to rebalance recommendation. */
bool AutoRebalancer::executeSplitProposal(const HotShardSplitPolicy::SplitProposal& proposal) {
    auto span = Tracer::startSpan("AutoRebalancer.executeSplitProposal");
    span.setAttribute("hot_shard", proposal.hot_shard_id);
    span.setAttribute("is_predictive", proposal.is_predictive);

    THEMIS_INFO("Executing shard split for {} – {} (current={:.1f}%, predicted={:.1f}%)",
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
    std::string target_shard;

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
    uint64_t shard_token_start = 0;
    uint64_t shard_token_end   = UINT64_MAX;
    if (topology_) {
        auto shard_info = topology_->getShard(proposal.hot_shard_id);
        if (shard_info) {
            shard_token_start = shard_info->token_start;
            shard_token_end   = shard_info->token_end;
        }
    }
    const uint64_t midpoint = shard_token_start + (shard_token_end - shard_token_start) / 2;
    rec.token_range_start = midpoint;
    rec.token_range_end   = shard_token_end;
    rec.expected_load_reduction_percent = 50.0;
    rec.justification = "Hot-shard split – " + proposal.reason;

    const bool ok = executeRebalance(rec);

    if (metrics_) {
        if (ok) {
            metrics_->incrementCounter("themis_shard_splits_total");
        } else {
            metrics_->incrementCounter("themis_shard_split_failures_total");
        }
    }

    span.setAttribute("success", ok);
    return ok;
}

/** @brief Install custom operation-signing callback override. */
void AutoRebalancer::setSignOperationFn(SignOperationFn fn) {
    std::lock_guard<std::mutex> lock(sign_fn_mutex_);
    sign_fn_ = std::move(fn);
}

/**
 * @brief Detect and handle automatic rebalancing for topology changes (node join/leave).
 *
 * Monitors the cluster topology for changes (nodes joining or leaving) and
 * automatically triggers rebalancing to redistribute shards and maintain balance.
 * Completion must occur at >=80% throughput with no data loss.
 */
void AutoRebalancer::handleTopologyChange() {
    if (!topology_) {
        return;
    }

    auto all_shards = topology_->getAllShards();
    std::vector<std::string> current_topology;
    for (const auto& shard : all_shards) {
        current_topology.push_back(shard.shard_id);
    }

    if (last_known_topology_.empty()) {
        last_known_topology_ = current_topology;
        return;
    }

    const std::set<std::string> previous_set(last_known_topology_.begin(), last_known_topology_.end());
    const std::set<std::string> current_set(current_topology.begin(), current_topology.end());
    if (previous_set == current_set) {
        return;
    }

    bool is_join = false;
    bool is_leave = false;
    for (const auto& shard_id : current_set) {
        if (previous_set.find(shard_id) == previous_set.end()) {
            is_join = true;
            break;
        }
    }
    for (const auto& shard_id : previous_set) {
        if (current_set.find(shard_id) == current_set.end()) {
            is_leave = true;
            break;
        }
    }

    THEMIS_WARN("Topology change detected: {} (was {}, now {} nodes)",
               is_join && is_leave ? "REPLACEMENT" : (is_join ? "JOIN" : (is_leave ? "LEAVE" : "UNKNOWN")),
               last_known_topology_.size(), current_topology.size());

    if (!config_.auto_trigger_enabled) {
        last_known_topology_ = current_topology;
        return;
    }

    if (!canTriggerRebalance()) {
        THEMIS_DEBUG("Cannot trigger topology rebalance (safety limits or cooldown)");
        return;
    }

    std::vector<std::string> old_topology = last_known_topology_;
    std::vector<std::string> target_topology = current_topology;
    std::vector<LoadImbalanceResult::RebalanceRecommendation> recommendations;

    if (is_join) {
        std::string source = old_topology.empty() ? target_topology.front() : old_topology.front();
        std::string target = target_topology.empty() ? source : target_topology.back();
        if (!source.empty() && !target.empty() && source != target) {
            LoadImbalanceResult::RebalanceRecommendation rec;
            rec.source_shard = source;
            rec.target_shard = target;
            rec.token_range_start = 0;
            rec.token_range_end = UINT64_MAX;
            rec.justification = "Topology change: node join";
            recommendations.push_back(rec);
        }
    }
    if (is_leave) {
        for (const auto& leaving : old_topology) {
            if (std::find(target_topology.begin(), target_topology.end(), leaving) == target_topology.end()) {
                std::string target = target_topology.empty() ? leaving : target_topology.front();
                if (!target.empty()) {
                    LoadImbalanceResult::RebalanceRecommendation rec;
                    rec.source_shard = leaving;
                    rec.target_shard = target;
                    rec.token_range_start = 0;
                    rec.token_range_end = UINT64_MAX;
                    rec.justification = "Topology change: node leave";
                    recommendations.push_back(rec);
                }
            }
        }
    }

    if (recommendations.empty()) {
        if (!old_topology.empty() && !target_topology.empty()) {
            LoadImbalanceResult::RebalanceRecommendation rec;
            rec.source_shard = old_topology.front();
            rec.target_shard = target_topology.front();
            rec.token_range_start = 0;
            rec.token_range_end = UINT64_MAX;
            rec.justification = "Topology change rebalancing";
            recommendations.push_back(rec);
        }
    }

    for (const auto& rec : recommendations) {
        executeRebalance(rec);
    }

    if (!recommendations.empty()) {
        load_detector_->recordRebalanceTriggered();
    }

    topology_change_count_++;
    last_known_topology_ = current_topology;

    THEMIS_INFO("Generated topology rebalance plan for {} nodes with {} recommendation(s)",
               current_topology.size(), recommendations.size());

    if (metrics_) {
        metrics_->incrementCounter("themis_topology_changes_total");
        metrics_->setGauge("themis_cluster_nodes",
                           static_cast<double>(current_topology.size()));
    }
}

} // namespace sharding
} // namespace themis
