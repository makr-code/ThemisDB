/**
 * @file gguf_st_adapter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/// GGUF-ST Hybrid Format: GGUF + Embedded SafeTensors + ThemisDB Extensions
/// 
/// File Structure:
/// ┌─────────────────────────────────────┐
/// │ GGUF Header (Magic: "GGUF")         │
/// │ GGUF Metadata (standard)            │
/// ├─────────────────────────────────────┤
/// │ GGUF Tensor Data (quantized)        │
/// ├─────────────────────────────────────┤
/// │ [OPTIONAL] SafeTensors Section      │
/// │   - Header: "STNS" (SafeTeNSors)    │
/// │   - SafeTensors data (FP16/FP32)    │
/// ├─────────────────────────────────────┤
/// │ ThemisDB Signature Section          │
/// │   - Header: "TSGN" (ThemisSignature)│
/// │   - AdapterSignature JSON           │
/// ├─────────────────────────────────────┤
/// │ ThemisDB Manifest Section           │
/// │   - Header: "TMFT" (ThemisManifest) │
/// │   - AdapterMetadata JSON            │
/// └─────────────────────────────────────┘

/// GGUF-ST Format Configuration
struct GGUFSTConfig {
    /// Size optimization mode
    enum class SizeMode {
        FULL,              // GGUF + SafeTensors + Signature + Manifest (~12-20MB)
        COMPACT,           // GGUF + Signature + Manifest (~8-16MB)
        ULTRA_COMPACT,     // GGUF + minimal metadata (~8MB)
        SIGNATURE_ONLY     // Only signature + manifest for registry (~100KB)
    };
    
    SizeMode size_mode = SizeMode::COMPACT;
    
    /// Quantization type for GGUF portion
    enum class QuantizationType {
        F32,      // Full precision (baseline, 64MB)
        F16,      // Half precision (32MB)
        Q8_0,     // 8-bit quantization (16MB)
        Q4_K_M,   // 4-bit K-quants medium (8MB) - RECOMMENDED
        Q2_K      // 2-bit K-quants (4MB, ~2% accuracy loss)
    };
    
    QuantizationType quantization = QuantizationType::Q4_K_M;
    
    /// Lossless compression for metadata/manifest
    bool compress_manifest = true;
    bool compress_safetensors = false;  // SafeTensors already efficient
    
    /// ZSTD compression level (1-22, higher = better compression, slower)
    int zstd_level = 3;  // Fast compression with good ratio
    
    /// Whether to embed SafeTensors (for verification/HuggingFace compatibility)
    bool embed_safetensors = false;  // Default: COMPACT mode
    
    /// Whether to include signature section
    bool include_signature = true;
    
    /// Whether to include manifest section
    bool include_manifest = true;
};

/// GGUF-ST Section Header
struct SectionHeader {
    virtual ~SectionHeader() = default;

    /// @brief Move constructor — transfers magic, version, data_size, flags, and reserved fields.
    /// @note Move semantics: POD fields copied from source; source left in zero-initialised state.
    SectionHeader(SectionHeader&&) noexcept noexcept = default;

    /// @brief Move assignment operator.
    /// @note Move semantics: all POD fields transferred; source left in zero-initialised state.
    SectionHeader& operator=(SectionHeader&&) noexcept noexcept = default;

    SectionHeader(const SectionHeader&) = default;
    SectionHeader& operator=(const SectionHeader&) = default;
    SectionHeader() = default;

    char magic[4];          // Section identifier
    uint32_t version = 0;       // Section format version
    uint64_t data_size = 0;     // Size of data following header
    uint32_t flags = 0;         // Compression flags, etc.
    uint32_t reserved = 0;      // Reserved for future use
};

/// GGUF-ST Adapter - Read/Write hybrid format adapters
/// Extends BlobStorageManager for storage operations
class GGUFSTAdapter {
public:
    virtual ~GGUFSTAdapter() = default;

    /// @brief Move constructor — transfers storage shared_ptr and config; source config reset to defaults.
    /// @note Move semantics: std::shared_ptr move transfers co-ownership; GGUFSTConfig is trivially copyable.
    GGUFSTAdapter(GGUFSTAdapter&&) noexcept noexcept = default;

    /// @brief Move assignment operator.
    /// @note Move semantics: storage_ and config_ replaced; old storage_ ref-count decremented.
    GGUFSTAdapter& operator=(GGUFSTAdapter&&) noexcept noexcept = default;

    GGUFSTAdapter(const GGUFSTAdapter&) = delete;
    GGUFSTAdapter& operator=(const GGUFSTAdapter&) = delete;

    explicit GGUFSTAdapter(
        std::shared_ptr<storage::BlobStorageManager> storage,
        const GGUFSTConfig& config = {}
    );
    
    // Write Operations
    
    /// Create GGUF-ST adapter file from components
    struct AdapterComponents {
        std::vector<uint8_t> gguf_data;              // GGUF tensor data
        std::optional<std::vector<uint8_t>> safetensors_data;  // Optional SafeTensors
        AdapterSignature signature;                   // Cryptographic signature
        AdapterMetadata metadata;                     // Complete metadata
    };
    
    /// Write adapter to storage
    /// @param adapter_id Unique adapter identifier
    /// @param components Adapter components to write
    /// @return Storage reference or nullopt on failure
    std::optional<storage::BlobRef> writeAdapter(
        const std::string& adapter_id,
        const AdapterComponents& components
    );
    
    // Read Operations
    
    /// Read adapter from storage
    /// @param ref Blob storage reference
    /// @return Adapter components or nullopt if not found/invalid
    std::optional<AdapterComponents> readAdapter(const storage::BlobRef& ref);
    
    /// Read only metadata (fast, for registry queries)
    /// @param ref Blob storage reference
    /// @return Adapter metadata or nullopt
    std::optional<AdapterMetadata> readMetadata(const storage::BlobRef& ref);
    
    /// Read only signature (for verification without loading full adapter)
    /// @param ref Blob storage reference
    /// @return Adapter signature or nullopt
    std::optional<AdapterSignature> readSignature(const storage::BlobRef& ref);
    
    // Verification Operations
    
    /// Verify adapter integrity
    struct VerificationResult {
        bool valid = false;
        bool signature_valid = false;
        bool safetensors_match = false;  // If SafeTensors present, does it match GGUF?
        bool manifest_valid = false;
        std::vector<std::string> errors;
        
        std::string toString() const;
    };
    
    VerificationResult verifyAdapter(const storage::BlobRef& ref);
    
    // Utility Operations
    
    /// Get format information
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
        
        nlohmann::json toJson() const;
    };
    
    FormatInfo getFormatInfo(const storage::BlobRef& ref);
    
    /// Extract SafeTensors from GGUF-ST file (for HuggingFace compatibility)
    std::optional<std::vector<uint8_t>> extractSafeTensors(const storage::BlobRef& ref);
    
    /// Convert between quantization levels
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
        
        nlohmann::json toJson() const;
    };
    
    CompressionStats getCompressionStats(const storage::BlobRef& ref);
    
    /// Get current configuration
    const GGUFSTConfig& getConfig() const { return config_; }
    
    /// Set configuration
    void setConfig(const GGUFSTConfig& config) { config_ = config; }
    
private:
    std::shared_ptr<storage::BlobStorageManager> storage_;
    GGUFSTConfig config_;
    
    // Internal helpers
    std::vector<uint8_t> compressData(const std::vector<uint8_t>& data, int level);
    std::vector<uint8_t> decompressData(const std::vector<uint8_t>& compressed_data);
    
    bool writeSectionHeader(std::vector<uint8_t>& buffer, const SectionHeader& header);
    std::optional<SectionHeader> readSectionHeader(const std::vector<uint8_t>& data, size_t offset);
    
    std::vector<uint8_t> serializeMetadata(const AdapterMetadata& metadata);
    std::optional<AdapterMetadata> deserializeMetadata(const std::vector<uint8_t>& data);
    
    std::vector<uint8_t> serializeSignature(const AdapterSignature& signature);
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
