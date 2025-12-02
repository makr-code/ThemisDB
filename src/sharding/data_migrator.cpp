#include "sharding/data_migrator.h"
#include "sharding/mtls_client.h"
#include <stdexcept>
#include <thread>
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

DataMigrator::DataMigrator(const DataMigratorConfig& config)
    : config_(config) {
    
    if (config_.source_endpoint.empty() || config_.target_endpoint.empty()) {
        throw std::invalid_argument("Source and target endpoints must not be empty");
    }
    
    if (config_.batch_size == 0) {
        throw std::invalid_argument("Batch size must be greater than 0");
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
    
    try {
        // Estimate total records (would query source shard in real implementation)
        progress.total_records = 10000; // Placeholder
        
        uint32_t offset = 0;
        uint32_t records_in_batch = config_.batch_size;
        
        while (records_in_batch == config_.batch_size) {
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
            
            // Write batch to target
            if (!writeBatch(target_shard_id, batch)) {
                result.errors.push_back("Failed to write batch at offset " + 
                                      std::to_string(offset));
                progress.errors++;
                
                if (progress.errors > config_.max_retries) {
                    result.error_message = "Too many errors during migration";
                    return result;
                }
                continue;
            }
            
            // Update progress
            progress.records_migrated += records_in_batch;
            progress.bytes_transferred += batch.dump().size();
            progress.progress_percent = 
                (static_cast<double>(progress.records_migrated) / progress.total_records) * 100.0;
            
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

} // namespace sharding
} // namespace themis
