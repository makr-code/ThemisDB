/**
 * @file secure_transport_client.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "sharding/mtls_client.h"
#include "utils/zstd_codec.h"
#include "utils/lz4_codec.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <optional>

namespace themis::sharding {

struct SecureTransportClientTestAccess;

/**
 * @brief Secure Transport Client
 * 
 * Shared transport abstraction for secure data transfer between shards.
 * Reuses WAL shipper transport capabilities:
 * - mTLS for secure communication
 * - Compression (Zstd/LZ4) for efficient transfer
 * - Retry logic with exponential backoff
 * - Integrity checks (checksums/signatures)
 * 
 * Used by both WALShipper and AdapterSyncManager to avoid code duplication.
 */
class SecureTransportClient {
public:
    /**
     * @brief Transport configuration
     */
    struct Config {
        // Compression configuration
        enum class CompressionType {
            None,
            LZ4,
            Zstd
        };
        CompressionType compression = CompressionType::Zstd;
        int compression_level = 3;
        size_t compression_threshold = 1024;  // Only compress if > 1KB
        
        // Retry configuration
        uint32_t max_retries = 5;
        uint32_t retry_delay_ms = 1000;
        uint32_t max_retry_delay_ms = 60000;
        
        // mTLS configuration
        std::string cert_path;
        std::string key_path;
        std::string ca_cert_path;
        
        // Request timeout
        uint32_t request_timeout_ms = 30000;
    };
    
    /**
     * @brief Payload to transfer
     */
    struct Payload {
        std::string data;                    // Binary data
        std::string content_type = "application/octet-stream";
        std::string checksum;                // Optional: SHA-256 checksum
        std::string signature;               // Optional: Digital signature
        nlohmann::json metadata;             // Optional: Metadata
        std::string authorization_token;     // Optional: JWT token for Authorization header
    };
    
    /**
     * @brief Transfer result
     */
    struct TransferResult {
        bool success = false;
        int status_code = 0;
        std::string error;
        size_t bytes_sent = 0;
        size_t bytes_compressed = 0;
        double compression_ratio = 1.0;
        int retry_count = 0;
    };
    
    /**
     * @brief Construct transport client
     * @param config Configuration
     */
    explicit SecureTransportClient(const Config& config);
    
    ~SecureTransportClient();
    
    /**
     * @brief Transfer payload to endpoint
     * @param endpoint Target endpoint (e.g., "https://shard-001:8080")
     * @param path API path (e.g., "/api/v1/lora/receive")
     * @param payload Data to transfer
     * @return Transfer result
     */
    TransferResult transfer(
        const std::string& endpoint,
        const std::string& path,
        const Payload& payload
    );
    
    /**
     * @brief Check if client is ready
     * @return true if mTLS client is initialized
     */
    bool isReady() const;
    
    /**
     * @brief Get underlying mTLS client
     * @return Shared pointer to mTLS client
     */
    std::shared_ptr<MTLSClient> getMTLSClient() const;
    
private:
    friend struct SecureTransportClientTestAccess;

    Config config_;
    std::shared_ptr<MTLSClient> mtls_client_;

    // ─── LZ4 bridges (stub #295) ──────────────────────────────────────────────

    /// @brief Type alias for LZ4 compression injection.
    using Lz4CompressFn = std::function<bool(const std::string& input,
                                              std::string&       output)>;

    /// @brief Type alias for LZ4 decompression injection.
    using Lz4DecompressFn = std::function<bool(const std::string& input,
                                                std::string&       output,
                                                std::size_t        original_size)>;

    /**
     * @brief Install an LZ4 compression callback for compressData().
     * @param fn Callable receiving (input, output) → success.
     */
    void setLz4CompressFn(Lz4CompressFn fn);

    /**
     * @brief Remove the LZ4 compression bridge.
     */
    void clearLz4CompressFn();

    /**
     * @brief Install an LZ4 decompression callback.
     * @param fn Callable receiving (input, output, original_size) → success.
     */
    void setLz4DecompressFn(Lz4DecompressFn fn);

    /**
     * @brief Remove the LZ4 decompression bridge.
     */
    void clearLz4DecompressFn();

    Lz4CompressFn   lz4CompressFn_;
    Lz4DecompressFn lz4DecompressFn_;
    
    /**
     * @brief Compress data if configured
     * @param data Input data
     * @param compressed Output compressed data (if compression applied)
     * @param compression_codec Optional output codec string ("zstd" or "lz4")
     * @return true if compression was applied
     */
    bool compressData(const std::string& data,
                      std::string& compressed,
                      std::string* compression_codec = nullptr);
    
    /**
     * @brief Perform transfer with retry logic
     * @param endpoint Target endpoint
     * @param path API path
     * @param payload Data to transfer
     * @param retry_count Current retry count
     * @return Transfer result
     */
    TransferResult transferWithRetry(
        const std::string& endpoint,
        const std::string& path,
        const Payload& payload,
        int retry_count
    );
};

} // namespace themis::sharding
