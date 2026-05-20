// THEMIS_GAP_STATS: gaps=1 unimpl=0 stub=0 mock=0 sim=0 todo=1 debt=0 scanned=2026-05-18
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            secure_transport_client.cpp                        ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:50:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     226                                            ║
    • Open Issues:     TODOs: 1, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • afc6b2738b  2026-03-26  fix: Resolve BSI/RAG production blockers – JWT, mTLS, CRL... ║
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

// ============================================================================
// LZ4 bridges (stub #295)
// ============================================================================

void SecureTransportClient::setLz4CompressFn(Lz4CompressFn fn) {
    lz4CompressFn_ = std::move(fn);
}

void SecureTransportClient::clearLz4CompressFn() {
    lz4CompressFn_ = nullptr;
}

void SecureTransportClient::setLz4DecompressFn(Lz4DecompressFn fn) {
    lz4DecompressFn_ = std::move(fn);
}

void SecureTransportClient::clearLz4DecompressFn() {
    lz4DecompressFn_ = nullptr;
}

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
        // Use injected LZ4 compress bridge if available (stub #295 resolved).
        if (lz4CompressFn_ && lz4CompressFn_(data, compressed)) {
            return true;
        }
        // STUB/SIMULATION NOTE (stub #295):
        // Purpose: Reserve the LZ4 compression slot in the negotiation chain so
        //          that future LZ4 support can be added without changing callers.
        // Activation: Active when no Lz4CompressFn is injected via setLz4CompressFn()
        //             and THEMIS_HAS_LZ4 is not defined.
        // Production Delta: Sharding payloads that prefer LZ4 fall through to the
        //                   uncompressed transfer path, increasing inter-shard bandwidth.
        // Removal Plan: Link the lz4 vcpkg package and add a #ifdef THEMIS_HAS_LZ4
        //               branch; or inject via setLz4CompressFn(). Target: v2.1.0.
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
        
        // Send via mTLS POST, forwarding the authorization_token as an Authorization: Bearer
        // header so that the receiving endpoint can validate the service-to-service JWT.
        // This removes the need for the receiver to bypass JWT validation for mTLS calls.
        spdlog::debug("SecureTransportClient: Sending {} bytes (compressed: {}) to {}{}",
                     original_size, compressed, endpoint, path);
        
        std::string auth_header;
        if (!payload.authorization_token.empty()) {
            auth_header = "Bearer " + payload.authorization_token;
        }
        
        auto response = auth_header.empty()
            ? mtls_client_->post(endpoint, path, request)
            : mtls_client_->post(endpoint, path, request, auth_header);
        
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

