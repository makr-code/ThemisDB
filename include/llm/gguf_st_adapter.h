/**
 * @file gguf_st_adapter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "storage/blob_storage_manager.h"
#include "llm/adapter_registry.h"
#include <vector>
#include <string>
#include <optional>
#include <cstdint>
#include <map>

namespace themis {
namespace llm {


struct GGUFSTConfig {
    enum class SizeMode {
        FULL,              // GGUF + SafeTensors + Signature + Manifest (~12-20MB)
        COMPACT,           // GGUF + Signature + Manifest (~8-16MB)
        ULTRA_COMPACT,     // GGUF + minimal metadata (~8MB)
        SIGNATURE_ONLY     // Only signature + manifest for registry (~100KB)
    };
    
    SizeMode size_mode = SizeMode::COMPACT;
    
    enum class QuantizationType {
        F32,      // Full precision (baseline, 64MB)
        F16,      // Half precision (32MB)
        Q8_0,     // 8-bit quantization (16MB)
        Q4_K_M,   // 4-bit K-quants medium (8MB) - RECOMMENDED
        Q2_K      // 2-bit K-quants (4MB, ~2% accuracy loss)
    };
    
    QuantizationType quantization = QuantizationType::Q4_K_M;
    
    bool compress_manifest = true;
    bool compress_safetensors = false;  // SafeTensors already efficient
    
    int zstd_level = 3;  // Fast compression with good ratio
    
    bool embed_safetensors = false;  // Default: COMPACT mode
    
    bool include_signature = true;
    
    bool include_manifest = true;
};

struct SectionHeader {
    /**
     * @brief Section Header.
     * @return Return value.
     */
    virtual ~SectionHeader() = default;

    SectionHeader(SectionHeader&&) noexcept = default;

    SectionHeader& operator=(SectionHeader&&) noexcept = default;

    SectionHeader(const SectionHeader&) = default;
    SectionHeader& operator=(const SectionHeader&) = default;
    SectionHeader() = default;

    char magic[4];          // Section identifier
    uint32_t version = 0;       // Section format version
    uint64_t data_size = 0;     // Size of data following header
    uint32_t flags = 0;         // Compression flags, etc.
    uint32_t reserved = 0;      // Reserved for future use
};

class GGUFSTAdapter {
public:
    /**
     * @brief GGUFSTAdapter.
     * @return Return value.
     */
    virtual ~GGUFSTAdapter() = default;

    GGUFSTAdapter(GGUFSTAdapter&&) noexcept = default;

    GGUFSTAdapter& operator=(GGUFSTAdapter&&) noexcept = default;

    GGUFSTAdapter(const GGUFSTAdapter&) = delete;
    GGUFSTAdapter& operator=(const GGUFSTAdapter&) = delete;

    explicit GGUFSTAdapter(
        std::shared_ptr<storage::BlobStorageManager> storage,
        const GGUFSTConfig& config = {}
    );
    
    // Write Operations
    
    struct AdapterComponents {
        std::vector<uint8_t> gguf_data;              // GGUF tensor data
        std::optional<std::vector<uint8_t>> safetensors_data;  // Optional SafeTensors
        AdapterSignature signature;                   // Cryptographic signature
        AdapterMetadata metadata;                     // Complete metadata
    };
    
    /**
     * @brief Write Adapter.
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] components Input parameter.
     * @return Return value.
     */
    std::optional<storage::BlobRef> writeAdapter(
        const std::string& adapter_id,
        const AdapterComponents& components
    );
    
    // Read Operations
    
    /**
     * @brief Read Adapter.
     * @param[in] ref Input parameter.
     * @return Return value.
     */
    std::optional<AdapterComponents> readAdapter(const storage::BlobRef& ref);
    
    /**
     * @brief Read Metadata.
     * @param[in] ref Input parameter.
     * @return Return value.
     */
    std::optional<AdapterMetadata> readMetadata(const storage::BlobRef& ref);
    
    /**
     * @brief Read Signature.
     * @param[in] ref Input parameter.
     * @return Return value.
     */
    std::optional<AdapterSignature> readSignature(const storage::BlobRef& ref);
    
    // Verification Operations
    
    struct VerificationResult {
        bool valid = false;
        bool signature_valid = false;
        bool safetensors_match = false;  // If SafeTensors present, does it match GGUF?
        bool manifest_valid = false;
        std::vector<std::string> errors;
        
        /**
         * @brief To String.
         * @return Return value.
         */
        std::string toString() const;
    };
    
    /**
     * @brief Verify Adapter.
     * @param[in] ref Input parameter.
     * @return Return value.
     */
    VerificationResult verifyAdapter(const storage::BlobRef& ref);
    
    // Utility Operations
    
    struct FormatInfo {
        GGUFSTConfig::SizeMode size_mode;
        GGUFSTConfig::QuantizationType quantization;
        bool has_safetensors = false;
        bool has_signature = false;
        bool has_manifest = false;
        size_t gguf_size_bytes = 0;
        size_t safetensors_size_bytes = 0;
        size_t signature_size_bytes = 0;
        size_t manifest_size_bytes = 0;
        size_t total_size_bytes = 0;
        
        /**
         * @brief To Json.
         * @return Return value.
         */
        nlohmann::json toJson() const;
    };
    
    /**
     * @brief Get Format Info.
     * @param[in] ref Input parameter.
     * @return Return value.
     */
    FormatInfo getFormatInfo(const storage::BlobRef& ref);
    
    /**
     * @brief Extract Safe Tensors.
     * @param[in] ref Input parameter.
     * @return Return value.
     */
    std::optional<std::vector<uint8_t>> extractSafeTensors(const storage::BlobRef& ref);
    
    /**
     * @brief Requantize.
     * @param[in] ref Input parameter.
     * @param[in] target_quantization Input parameter.
     * @return Return value.
     */
    std::optional<storage::BlobRef> requantize(
        const storage::BlobRef& ref,
        GGUFSTConfig::QuantizationType target_quantization
    );
    
    // Compression Statistics
    
    struct CompressionStats {
        size_t uncompressed_size = 0;
        size_t compressed_size = 0;
        double compression_ratio = 0.0;  // compressed / uncompressed
        double space_saved_percent = 0.0;
        
        /**
         * @brief To Json.
         * @return Return value.
         */
        nlohmann::json toJson() const;
    };
    
    /**
     * @brief Get Compression Stats.
     * @param[in] ref Input parameter.
     * @return Return value.
     */
    CompressionStats getCompressionStats(const storage::BlobRef& ref);
    
    const GGUFSTConfig& getConfig() const { return config_; }
    
    /**
     * @brief Set Config.
     * @param[in] config Input parameter.
     * @details Implements setConfig without additional internal calls.
     */
    void setConfig(const GGUFSTConfig& config) { config_ = config; }
    
private:
    std::shared_ptr<storage::BlobStorageManager> storage_;
    GGUFSTConfig config_;
    
    // Internal helpers
    /**
     * @brief Compress Data.
     * @param[in] data Input parameter.
     * @param[in] level Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> compressData(const std::vector<uint8_t>& data, int level);
    /**
     * @brief Decompress Data.
     * @param[in] compressed_data Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> decompressData(const std::vector<uint8_t>& compressed_data);
    
    /**
     * @brief Write Section Header.
     * @param[in,out] buffer Input/output parameter.
     * @param[in] header Input parameter.
     * @return True when the operation succeeds.
     */
    bool writeSectionHeader(std::vector<uint8_t>& buffer, const SectionHeader& header);
    /**
     * @brief Read Section Header.
     * @param[in] data Input parameter.
     * @param[in] offset Input parameter.
     * @return Return value.
     */
    std::optional<SectionHeader> readSectionHeader(const std::vector<uint8_t>& data, size_t offset);
    
    /**
     * @brief Serialize Metadata.
     * @param[in] metadata Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> serializeMetadata(const AdapterMetadata& metadata);
    /**
     * @brief Deserialize Metadata.
     * @param[in] data Input parameter.
     * @return Return value.
     */
    std::optional<AdapterMetadata> deserializeMetadata(const std::vector<uint8_t>& data);
    
    /**
     * @brief Serialize Signature.
     * @param[in] signature Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> serializeSignature(const AdapterSignature& signature);
    /**
     * @brief Deserialize Signature.
     * @param[in] data Input parameter.
     * @return Return value.
     */
    std::optional<AdapterSignature> deserializeSignature(const std::vector<uint8_t>& data);
    
    // Section magic constants (4 bytes, not null-terminated)
    static constexpr uint8_t SAFETENSORS_MAGIC[4] = {'S', 'T', 'N', 'S'};
    static constexpr uint8_t SIGNATURE_MAGIC[4] = {'T', 'S', 'G', 'N'};
    static constexpr uint8_t MANIFEST_MAGIC[4] = {'T', 'M', 'F', 'T'};
    static constexpr uint32_t SECTION_VERSION = 1;
    
    // Compression flags
    static constexpr uint32_t FLAG_COMPRESSED_ZSTD = 0x0001;
    static constexpr uint32_t FLAG_COMPRESSED_LZ4 = 0x0002;
};

} // namespace llm
} // namespace themis
