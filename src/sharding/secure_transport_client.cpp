/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            secure_transport_client.cpp                        ║
  Version:         0.0.21                                             ║
  Last Modified:   2026-02-21 19:20:15                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   88.0/100                                       ║
    • Total Lines:     228                                            ║
    • Open Issues:     TODOs: 2, Stubs: 1                             ║
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

#include "sharding/secure_transport_client.h"
#include "utils/cursor.h"
#include <spdlog/spdlog.h>
#include <thread>
#include <chrono>
#include <algorithm>

namespace themis::sharding {

SecureTransportClient::SecureTransportClient(const Config& config)
    : config_(config) {
    
    // Create mTLS client if certificates provided
    if (!config_.cert_path.empty()) {
        MTLSClient::Config mtls_config;
        mtls_config.cert_path = config_.cert_path;
        mtls_config.key_path = config_.key_path;
        mtls_config.ca_cert_path = config_.ca_cert_path;
        mtls_config.request_timeout_ms = config_.request_timeout_ms;
        mtls_config.max_retries = 0;  // We handle retries in this class
        
        mtls_client_ = std::make_shared<MTLSClient>(mtls_config);
        spdlog::debug("SecureTransportClient: mTLS client initialized");
    } else {
        spdlog::warn("SecureTransportClient: No certificates provided, mTLS disabled");
    }
}

SecureTransportClient::~SecureTransportClient() = default;

bool SecureTransportClient::isReady() const {
    return mtls_client_ && mtls_client_->isReady();
}

std::shared_ptr<MTLSClient> SecureTransportClient::getMTLSClient() const {
    return mtls_client_;
}

bool SecureTransportClient::compressData(const std::string& data, std::string& compressed) {
    if (config_.compression == Config::CompressionType::None) {
        return false;
    }
    
    // Only compress if data exceeds threshold
    if (data.size() < config_.compression_threshold) {
        return false;
    }
    
    try {
        if (config_.compression == Config::CompressionType::Zstd) {
            auto compressed_bytes = utils::zstd_compress(data, config_.compression_level);
            if (!compressed_bytes.empty() && compressed_bytes.size() < data.size()) {
                compressed = std::string(compressed_bytes.begin(), compressed_bytes.end());
                spdlog::debug("SecureTransportClient: Compressed {} -> {} bytes (ratio: {:.2f}x)",
                             data.size(), compressed.size(),
                             static_cast<double>(data.size()) / compressed.size());
                return true;
            }
        }
        // TODO: Add LZ4 support in the future
    } catch (const std::exception& e) {
        spdlog::warn("SecureTransportClient: Compression failed: {}", e.what());
    }
    
    return false;
}

SecureTransportClient::TransferResult SecureTransportClient::transfer(
    const std::string& endpoint,
    const std::string& path,
    const Payload& payload
) {
    return transferWithRetry(endpoint, path, payload, 0);
}

SecureTransportClient::TransferResult SecureTransportClient::transferWithRetry(
    const std::string& endpoint,
    const std::string& path,
    const Payload& payload,
    int retry_count
) {
    TransferResult result;
    
    if (!mtls_client_ || !mtls_client_->isReady()) {
        result.error = "mTLS client not ready";
        spdlog::error("SecureTransportClient: {}", result.error);
        return result;
    }
    
    try {
        // Prepare payload
        size_t original_size = payload.data.size();
        std::string transfer_data;
        bool compressed = false;
        
        // Try compression
        std::string compressed_data;
        if (compressData(payload.data, compressed_data)) {
            transfer_data = compressed_data;
            compressed = true;
            result.bytes_compressed = compressed_data.size();
            result.compression_ratio = static_cast<double>(original_size) / compressed_data.size();
        } else {
            transfer_data = payload.data;
            result.bytes_compressed = original_size;
            result.compression_ratio = 1.0;
        }
        
        // Build request JSON
        nlohmann::json request;
        
        // Add metadata
        if (!payload.metadata.is_null()) {
            request["metadata"] = payload.metadata;
        }
        
        // Add integrity checks
        if (!payload.checksum.empty()) {
            request["checksum"] = payload.checksum;
        }
        if (!payload.signature.empty()) {
            request["signature"] = payload.signature;
        }
        
        // Add compression info
        if (compressed) {
            request["compression"] = "zstd";
            request["original_size"] = original_size;
        } else {
            request["compression"] = "none";
        }
        
        // Encode binary data as base64 string (not JSON binary type which doesn't serialize properly)
        std::string data_base64 = utils::Cursor::base64Encode(transfer_data);
        request["data"] = data_base64;
        request["content_type"] = payload.content_type;
        
        // TODO: MTLSClient doesn't currently support custom headers (like Authorization: Bearer)
        // For cross-shard communication, the receiving endpoint needs to either:
        // 1. Bypass JWT validation when mTLS certificate is valid (service-to-service auth)
        // 2. MTLSClient needs to be extended to accept custom headers
        // 3. Use a separate endpoint that doesn't require JWT (only mTLS)
        // Currently: authorization_token in payload is not used
        
        // Send via mTLS POST
        spdlog::debug("SecureTransportClient: Sending {} bytes (compressed: {}) to {}{}",
                     original_size, compressed, endpoint, path);
        
        auto response = mtls_client_->post(endpoint, path, request);
        
        if (response.success) {
            result.success = true;
            result.status_code = response.status_code;
            result.bytes_sent = original_size;
            result.retry_count = retry_count;
            
            spdlog::info("SecureTransportClient: Transfer successful ({} bytes, {} retries)",
                        original_size, retry_count);
            return result;
        } else {
            // Transfer failed, check if we should retry
            result.status_code = response.status_code;
            result.error = response.error;
            
            if (retry_count < static_cast<int>(config_.max_retries)) {
                // Calculate exponential backoff delay
                uint32_t delay = config_.retry_delay_ms * (1 << retry_count);
                delay = std::min(delay, config_.max_retry_delay_ms);
                
                spdlog::warn("SecureTransportClient: Transfer failed ({}), retrying in {}ms (attempt {}/{})",
                            response.error, delay, retry_count + 1, config_.max_retries);
                
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                return transferWithRetry(endpoint, path, payload, retry_count + 1);
            } else {
                spdlog::error("SecureTransportClient: Transfer failed after {} retries: {}",
                             config_.max_retries, response.error);
                result.retry_count = retry_count;
                return result;
            }
        }
        
    } catch (const std::exception& e) {
        result.error = std::string("Transfer exception: ") + e.what();
        spdlog::error("SecureTransportClient: {}", result.error);
        
        // Retry on exception
        if (retry_count < static_cast<int>(config_.max_retries)) {
            uint32_t delay = config_.retry_delay_ms * (1 << retry_count);
            delay = std::min(delay, config_.max_retry_delay_ms);
            
            spdlog::warn("SecureTransportClient: Retrying after exception in {}ms", delay);
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            return transferWithRetry(endpoint, path, payload, retry_count + 1);
        }
        
        result.retry_count = retry_count;
        return result;
    }
}

} // namespace themis::sharding
