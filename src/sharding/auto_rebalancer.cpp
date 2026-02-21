/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            auto_rebalancer.cpp                                ║
  Version:         0.0.21                                             ║
  Last Modified:   2026-02-21 19:20:15                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     595                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "sharding/auto_rebalancer.h"
#include "sharding/shard_topology.h"
#include "sharding/prometheus_metrics.h"
#include "sharding/data_migrator.h"
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

namespace themis {
namespace sharding {

AutoRebalancer::AutoRebalancer(
    std::shared_ptr<ShardTopology> topology,
    std::shared_ptr<ShardLoadDetector> load_detector,
    std::shared_ptr<PrometheusMetrics> metrics,
    std::shared_ptr<DataMigrator> migrator
) : AutoRebalancer(topology, load_detector, metrics, migrator, Config{}) {}

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

AutoRebalancer::~AutoRebalancer() {
    stop();
}

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

void AutoRebalancer::stop() {
    if (!running_.exchange(false)) {
        return;
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

void AutoRebalancer::monitorLoop() {
    THEMIS_INFO("AutoRebalancer monitor loop started");
    
    while (running_.load()) {
        auto span = Tracer::startSpan("AutoRebalancer.monitorTick");
        
        try {
            total_checks_++;
            last_check_time_ = std::chrono::system_clock::now();
            
            // Cleanup completed operations
            cleanupCompletedOperations();
            
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
    
    // Sign operation
    std::string signature = signOperation(op_id);
    
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

std::string AutoRebalancer::generateOperationId() const {
    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();
    
    std::ostringstream oss;
    oss << "rebalance_" << std::hex << now_ms;
    return oss.str();
}

std::string AutoRebalancer::signOperation(const std::string& operation_id) const {
    // RSA-SHA256 signing using operator certificate
    
    // Load private key from operator certificate
    if (config_.operator_key_path.empty()) {
        THEMIS_WARN("AutoRebalancer: No operator key configured, using placeholder signature");
        std::ostringstream oss;
        oss << "UNSIGNED:" << operation_id << ":no_key_configured";
        return oss.str();
    }
    
    // Open and read private key file
    FILE* key_file = fopen(config_.operator_key_path.c_str(), "r");
    if (!key_file) {
        THEMIS_ERROR("AutoRebalancer: Cannot open operator key file: {}", config_.operator_key_path);
        std::ostringstream oss;
        oss << "UNSIGNED:" << operation_id << ":key_file_error";
        return oss.str();
    }
    
    EVP_PKEY* pkey = PEM_read_PrivateKey(key_file, nullptr, nullptr, nullptr);
    fclose(key_file);
    
    if (!pkey) {
        THEMIS_ERROR("AutoRebalancer: Failed to parse operator private key");
        std::ostringstream oss;
        oss << "UNSIGNED:" << operation_id << ":key_parse_error";
        return oss.str();
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
        return "UNSIGNED:" + operation_id + ":context_error";
    }
    
    if (EVP_PKEY_sign_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        THEMIS_ERROR("AutoRebalancer: Failed to initialize signing");
        return "UNSIGNED:" + operation_id + ":init_error";
    }
    
    // Set padding mode to PKCS#1 v1.5
    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PADDING) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        THEMIS_ERROR("AutoRebalancer: Failed to set RSA padding");
        return "UNSIGNED:" + operation_id + ":padding_error";
    }
    
    if (EVP_PKEY_CTX_set_signature_md(ctx, EVP_sha256()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        THEMIS_ERROR("AutoRebalancer: Failed to set signature digest");
        return "UNSIGNED:" + operation_id + ":digest_error";
    }
    
    // Determine signature buffer size
    size_t sig_len = 0;
    if (EVP_PKEY_sign(ctx, nullptr, &sig_len, hash, SHA256_DIGEST_LENGTH) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        THEMIS_ERROR("AutoRebalancer: Failed to determine signature size");
        return "UNSIGNED:" + operation_id + ":sigsize_error";
    }
    
    // Allocate buffer and perform signing
    std::vector<unsigned char> signature(sig_len);
    if (EVP_PKEY_sign(ctx, signature.data(), &sig_len, hash, SHA256_DIGEST_LENGTH) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        THEMIS_ERROR("AutoRebalancer: Signing operation failed");
        return "UNSIGNED:" + operation_id + ":sign_error";
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
        return "UNSIGNED:" + operation_id + ":encoding_error";
    }
    
    // Return formatted signature: SIGNATURE:{sig_b64}:{timestamp}
    std::ostringstream result;
    result << "SIGNATURE:" << sig_b64 << ":" << timestamp;
    
    THEMIS_INFO("AutoRebalancer: Successfully signed operation {} (sig_len={})", 
                operation_id, sig_len);
    
    return result.str();
}

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

bool AutoRebalancer::isWithinSafetyLimits(const LoadImbalanceResult& imbalance) const {
    // Check if total data movement is within limits
    // Simplified - in production, calculate actual data size
    
    if (imbalance.recommendations.empty()) {
        return false;
    }
    
    // For now, allow if we have reasonable recommendations
    return imbalance.recommendations.size() <= config_.max_concurrent_operations * 2;
}

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

bool AutoRebalancer::triggerCheck() {
    if (!running_.load()) {
        THEMIS_WARN("AutoRebalancer not running, cannot trigger check");
        return false;
    }
    
    THEMIS_INFO("Manual rebalance check triggered");
    cv_.notify_one();
    return true;
}

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

nlohmann::json AutoRebalancer::getStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    nlohmann::json stats;
    stats["running"] = running_.load();
    stats["total_checks"] = total_checks_.load();
    stats["triggered_operations"] = triggered_operations_.load();
    stats["completed_operations"] = completed_operations_.load();
    stats["failed_operations"] = failed_operations_.load();
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

} // namespace sharding
} // namespace themis
