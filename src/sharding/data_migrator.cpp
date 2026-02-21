/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            data_migrator.cpp                                  ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-02-21 12:09:10                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     537                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "sharding/data_migrator.h"
#include "sharding/mtls_client.h"
#include "sharding/prometheus_metrics.h"
#include <stdexcept>
#include <thread>
#include <filesystem>
#include <fstream>
#include <openssl/sha.h>
#include <iomanip>
#include <sstream>
#include <iostream>

namespace themis {
namespace sharding {

// Create mTLS client from DataMigratorConfig
static std::unique_ptr<themis::sharding::MTLSClient> createMTLSClient(const DataMigratorConfig& config) {
    themis::sharding::MTLSClient::Config mtls_config;
    mtls_config.cert_path = config.cert_path;
    mtls_config.key_path = config.key_path;
    mtls_config.ca_cert_path = config.ca_cert_path;
    mtls_config.tls_version = "TLSv1.3";
    mtls_config.verify_peer = true;
    mtls_config.verify_hostname = true;
    mtls_config.connect_timeout_ms = 5000;
    mtls_config.request_timeout_ms = 30000;
    mtls_config.max_retries = config.max_retries;
    mtls_config.retry_delay_ms = config.retry_delay_ms;
    
    return std::make_unique<themis::sharding::MTLSClient>(mtls_config);
}

DataMigrator::DataMigrator(
    const DataMigratorConfig& config,
    std::shared_ptr<PrometheusMetrics> metrics)
    : config_(config),
      metrics_(metrics) {
    
    if (config_.source_endpoint.empty() || config_.target_endpoint.empty()) {
        throw std::invalid_argument("Source and target endpoints must not be empty");
    }
    
    if (config_.batch_size == 0) {
        throw std::invalid_argument("Batch size must be greater than 0");
    }
    
    // Load idempotency state if enabled
    if (config_.enable_idempotency) {
        loadIdempotencyState();
    }
}

MigrationResult DataMigrator::migrate(
    const std::string& source_shard_id,
    const std::string& target_shard_id,
    uint64_t token_range_start,
    uint64_t token_range_end,
    ProgressCallback progress_callback
) {
    MigrationResult result;
    MigrationProgress progress;
    
    // Generate deterministic migration ID
    std::string migration_id = generateMigrationId(
        source_shard_id, target_shard_id,
        token_range_start, token_range_end
    );
    result.migration_id = migration_id;
    progress.migration_id = migration_id;
    
    // Check if already completed (idempotency)
    if (config_.enable_idempotency && isMigrationCompleted(migration_id)) {
        result.success = true;
        result.was_already_completed = true;
        return result;
    }
    
    auto start_time = std::chrono::steady_clock::now();
    std::string operation_id = source_shard_id + "_to_" + target_shard_id;
    
    try {
        // Estimate total records (would query source shard in real implementation)
        progress.total_records = 10000; // Placeholder
        
        uint32_t offset = 0;
        uint32_t batch_index = 0;
        uint32_t records_in_batch = config_.batch_size;
        
        while (records_in_batch == config_.batch_size) {
            // Generate deterministic batch ID
            std::string batch_id = generateBatchId(migration_id, batch_index);
            
            // Skip already completed batches (idempotency)
            if (config_.enable_idempotency && isBatchCompleted(batch_id)) {
                offset += config_.batch_size;
                batch_index++;
                continue;
            }
            
            // Fetch batch from source
            auto batch = fetchBatch(source_shard_id, token_range_start, 
                                  token_range_end, offset, config_.batch_size);
            
            records_in_batch = batch.is_array() ? static_cast<uint32_t>(batch.size()) : 0;
            
            if (records_in_batch == 0) {
                break;
            }
            
            // Calculate hash if verification enabled
            std::string batch_hash;
            if (config_.verify_integrity) {
                batch_hash = calculateHash(batch);
            }
            
            // Write batch to target (atomic operation)
            if (!writeBatch(target_shard_id, batch)) {
                result.errors.push_back("Failed to write batch at offset " + 
                                      std::to_string(offset));
                progress.errors++;
                
                if (progress.errors > config_.max_retries) {
                    result.error_message = "Too many errors during migration";
                    return result;
                }
                // Do not mark batch as completed on failure - allows retry
                offset += config_.batch_size;
                batch_index++;
                continue;
            }
            
            // Mark batch as completed (idempotency tracking)
            if (config_.enable_idempotency) {
                markBatchCompleted(batch_id);
            }
            
            // Update progress
            progress.records_migrated += records_in_batch;
            progress.bytes_transferred += batch.dump().size();
            progress.progress_percent = 
                (static_cast<double>(progress.records_migrated) / progress.total_records) * 100.0;
            
            // Record metrics
            if (metrics_) {
                metrics_->recordMigrationProgress(
                    operation_id,
                    progress.records_migrated,
                    progress.bytes_transferred,
                    progress.progress_percent
                );
            }
            
            if (progress_callback) {
                progress_callback(progress);
            }
            
            offset += records_in_batch;
            result.records_migrated = progress.records_migrated;
            result.bytes_transferred = progress.bytes_transferred;
        }
        
        // Verify integrity if enabled
        if (config_.verify_integrity) {
            if (!verifyIntegrity(source_shard_id, target_shard_id, 
                               token_range_start, token_range_end)) {
                result.error_message = "Integrity verification failed";
                return result;
            }
        }
        
        result.success = true;
        
        // Mark migration as completed (idempotency)
        if (config_.enable_idempotency) {
            markMigrationCompleted(migration_id);
        }
        
        // Record final metrics
        auto end_time = std::chrono::steady_clock::now();
        auto duration_sec = std::chrono::duration_cast<std::chrono::seconds>(
            end_time - start_time).count();
        
        if (metrics_) {
            metrics_->recordMigrationDuration(operation_id, static_cast<double>(duration_sec));
        }
        
    } catch (const std::exception& e) {
        result.error_message = std::string("Migration failed: ") + e.what();
        result.success = false;
    }
    
    return result;
}

bool DataMigrator::verifyIntegrity(
    const std::string& source_shard_id,
    const std::string& target_shard_id,
    uint64_t token_range_start,
    uint64_t token_range_end
) {
    try {
        // Fetch data from both shards
        auto source_data = fetchBatch(source_shard_id, token_range_start, 
                                     token_range_end, 0, 10000);
        auto target_data = fetchBatch(target_shard_id, token_range_start, 
                                     token_range_end, 0, 10000);
        
        // Calculate hashes
        std::string source_hash = calculateHash(source_data);
        std::string target_hash = calculateHash(target_data);
        
        return source_hash == target_hash;
        
    } catch (const std::exception&) {
        return false;
    }
}

nlohmann::json DataMigrator::fetchBatch(
    const std::string& source_shard_id,
    uint64_t token_range_start,
    uint64_t token_range_end,
    uint32_t offset,
    uint32_t limit
) {
    // Create mTLS client for secure shard-to-shard communication
    auto mtls_client = createMTLSClient(config_);
    
    if (!mtls_client->isReady()) {
        std::cerr << "DataMigrator: mTLS client not ready (missing certificates?)" << std::endl;
        return nlohmann::json::array();
    }
    
    // Build the query endpoint for fetching data in the specified token range
    std::ostringstream path_oss;
    path_oss << "/api/v1/data/migrate/fetch"
             << "?token_range_start=" << token_range_start
             << "&token_range_end=" << token_range_end
             << "&offset=" << offset
             << "&limit=" << limit
             << "&shard_id=" << source_shard_id;
    
    // Execute the GET request via mTLS
    auto response = mtls_client->get(config_.source_endpoint, path_oss.str());
    
    if (!response.success) {
        std::cerr << "DataMigrator::fetchBatch: Failed to fetch from source shard " 
                  << source_shard_id << " - " << response.error << std::endl;
        return nlohmann::json::array();
    }
    
    // Validate response structure
    if (!response.body.is_object()) {
        std::cerr << "DataMigrator::fetchBatch: Invalid response format (expected object)" << std::endl;
        return nlohmann::json::array();
    }
    
    // Extract the records array from the response
    if (response.body.contains("records") && response.body["records"].is_array()) {
        return response.body["records"];
    }
    
    // Legacy format: response body itself might be the array
    if (response.body.is_array()) {
        return response.body;
    }
    
    std::cerr << "DataMigrator::fetchBatch: Response did not contain 'records' array" << std::endl;
    return nlohmann::json::array();
}

bool DataMigrator::writeBatch(
    const std::string& target_shard_id,
    const nlohmann::json& batch
) {
    if (!batch.is_array() || batch.empty()) {
        return false;
    }
    
    // Create mTLS client for secure shard-to-shard communication
    auto mtls_client = createMTLSClient(config_);
    
    if (!mtls_client->isReady()) {
        std::cerr << "DataMigrator: mTLS client not ready (missing certificates?)" << std::endl;
        return false;
    }
    
    // Build the request body for the batch write operation
    nlohmann::json request_body;
    request_body["shard_id"] = target_shard_id;
    request_body["records"] = batch;
    request_body["operation"] = "migration_write";
    
    // Retry the write operation with exponential backoff
    return retryOperation([&]() {
        // Build the POST endpoint for migration writes
        std::string path = "/api/v1/data/migrate/write";
        
        // Execute the POST request via mTLS
        auto response = mtls_client->post(config_.target_endpoint, path, request_body);
        
        if (!response.success) {
            std::cerr << "DataMigrator::writeBatch: Failed to write to target shard " 
                      << target_shard_id << " - " << response.error 
                      << " (HTTP " << response.status_code << ")" << std::endl;
            return false;
        }
        
        // Check response for success confirmation
        if (response.body.is_object() && response.body.contains("success")) {
            return response.body["success"].get<bool>();
        }
        
        // HTTP 2xx status codes indicate success
        return response.status_code >= 200 && response.status_code < 300;
    });
}

std::string DataMigrator::calculateHash(const nlohmann::json& data) {
    // Calculate SHA-256 hash of JSON data
    std::string data_str = data.dump();
    
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data_str.c_str()), 
           data_str.size(), hash);
    
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') 
           << static_cast<int>(hash[i]);
    }
    
    return ss.str();
}

template<typename Func>
bool DataMigrator::retryOperation(Func func) {
    for (uint32_t attempt = 0; attempt < config_.max_retries; ++attempt) {
        try {
            if (func()) {
                return true;
            }
        } catch (const std::exception&) {
            // Continue to retry
        }
        
        if (attempt < config_.max_retries - 1) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(config_.retry_delay_ms * (attempt + 1))
            );
        }
    }
    
    return false;
}


// ============================================================================
// Idempotency Helper Methods
// ============================================================================

std::string DataMigrator::generateMigrationId(
    const std::string& source_shard_id,
    const std::string& target_shard_id,
    uint64_t token_range_start,
    uint64_t token_range_end
) {
    // Create deterministic string for hashing
    std::ostringstream oss;
    oss << source_shard_id << ":"
        << target_shard_id << ":"
        << token_range_start << ":"
        << token_range_end;
    
    // Calculate SHA256 hash
    std::string input = oss.str();
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), 
           input.size(), hash);
    
    // Convert to hex string
    std::ostringstream hex_oss;
    hex_oss << std::hex << std::setfill('0');
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        hex_oss << std::setw(2) << static_cast<unsigned>(hash[i]);
    }
    
    return "migration_" + hex_oss.str();
}

std::string DataMigrator::generateBatchId(
    const std::string& migration_id,
    uint32_t batch_index
) {
    return migration_id + "_batch_" + std::to_string(batch_index);
}

bool DataMigrator::isMigrationCompleted(const std::string& migration_id) {
    std::lock_guard<std::mutex> lock(idempotency_mutex_);
    return completed_migrations_.find(migration_id) != completed_migrations_.end();
}

void DataMigrator::markMigrationCompleted(const std::string& migration_id) {
    std::lock_guard<std::mutex> lock(idempotency_mutex_);
    completed_migrations_.insert(migration_id);
    saveIdempotencyState();
}

bool DataMigrator::isBatchCompleted(const std::string& batch_id) {
    std::lock_guard<std::mutex> lock(idempotency_mutex_);
    return completed_batches_.find(batch_id) != completed_batches_.end();
}

void DataMigrator::markBatchCompleted(const std::string& batch_id) {
    std::lock_guard<std::mutex> lock(idempotency_mutex_);
    completed_batches_.insert(batch_id);
    // Persist after every N batches to avoid too frequent I/O (thread-safe with atomic)
    if (batch_counter_.fetch_add(1, std::memory_order_relaxed) % 10 == 0) {
        saveIdempotencyState();
    }
}

void DataMigrator::loadIdempotencyState() {
    std::lock_guard<std::mutex> lock(idempotency_mutex_);
    
    try {
        namespace fs = std::filesystem;
        fs::path state_dir(config_.idempotency_store_path);
        
        if (!fs::exists(state_dir)) {
            fs::create_directories(state_dir);
            return;
        }
        
        // Load completed migrations
        fs::path migrations_file = state_dir / "completed_migrations.json";
        if (fs::exists(migrations_file)) {
            std::ifstream ifs(migrations_file);
            nlohmann::json j;
            ifs >> j;
            
            if (j.is_array()) {
                for (const auto& item : j) {
                    if (item.is_string()) {
                        completed_migrations_.insert(item.get<std::string>());
                    }
                }
            }
        }
        
        // Load completed batches
        fs::path batches_file = state_dir / "completed_batches.json";
        if (fs::exists(batches_file)) {
            std::ifstream ifs(batches_file);
            nlohmann::json j;
            ifs >> j;
            
            if (j.is_array()) {
                for (const auto& item : j) {
                    if (item.is_string()) {
                        completed_batches_.insert(item.get<std::string>());
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "DataMigrator: Failed to load idempotency state: " 
                  << e.what() << std::endl;
    }
}

void DataMigrator::saveIdempotencyState() {
    // Note: mutex should already be locked by caller
    
    try {
        namespace fs = std::filesystem;
        fs::path state_dir(config_.idempotency_store_path);
        
        if (!fs::exists(state_dir)) {
            fs::create_directories(state_dir);
        }
        
        // Save completed migrations
        nlohmann::json migrations_json = nlohmann::json::array();
        for (const auto& migration_id : completed_migrations_) {
            migrations_json.push_back(migration_id);
        }
        
        fs::path migrations_file = state_dir / "completed_migrations.json";
        std::ofstream ofs_migrations(migrations_file);
        ofs_migrations << migrations_json.dump(2);
        
        // Save completed batches
        nlohmann::json batches_json = nlohmann::json::array();
        for (const auto& batch_id : completed_batches_) {
            batches_json.push_back(batch_id);
        }
        
        fs::path batches_file = state_dir / "completed_batches.json";
        std::ofstream ofs_batches(batches_file);
        ofs_batches << batches_json.dump(2);
        
    } catch (const std::exception& e) {
        std::cerr << "DataMigrator: Failed to save idempotency state: " 
                  << e.what() << std::endl;
    }
}

}  // namespace sharding
}  // namespace themis

