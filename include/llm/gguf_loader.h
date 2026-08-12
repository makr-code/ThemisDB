/**
 * @file gguf_loader.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>

// Forward declarations
namespace themis {
    class RocksDBWrapper;
}

namespace themis {
namespace llm {

// GGML/GGUF quantization types
enum class GGMLType : uint32_t {
    F32 = 0,
    F16 = 1,
    Q4_0 = 2,
    Q4_1 = 3,
    Q5_0 = 6,
    Q5_1 = 7,
    Q8_0 = 8,
    Q8_1 = 9,
    Q2_K = 10,
    Q3_K = 11,
    Q4_K = 12,
    Q5_K = 13,
    Q6_K = 14,
    Q8_K = 15,
    I8 = 16,
    I16 = 17,
    I32 = 18,
    // Add aliases for K-means variants
    Q4_K_S = 12,  // Small variant (same base type)
    Q4_K_M = 12,  // Medium variant (same base type)
};

// GGUF value types for metadata
enum class GGUFValueType : uint32_t {
    UINT8 = 0,
    INT8 = 1,
    UINT16 = 2,
    INT16 = 3,
    UINT32 = 4,
    INT32 = 5,
    FLOAT32 = 6,
    BOOL = 7,
    STRING = 8,
    ARRAY = 9,
    UINT64 = 10,
    INT64 = 11,
    FLOAT64 = 12,
};

// GGUF tensor metadata
struct TensorMetadata {
    virtual ~TensorMetadata() = default;
    std::string name;
    std::vector<int64_t> shape;
    GGMLType type;      // GGML quantization type
    size_t offset = 0;      // Offset in GGUF file
    size_t size = 0;        // Size in bytes
    
    // Helper to get type as string
    std::string type_string() const;
};

// GGUF file metadata
struct GGUFMetadata {
    virtual ~GGUFMetadata() = default;
    uint32_t version = 0;
    std::string architecture;  // "llama", "mistral", etc.
    std::unordered_map<std::string, std::string> config;
    std::vector<TensorMetadata> tensors;
    size_t total_size = 0;
    size_t data_offset = 0;  // Offset where tensor data starts
};

// GGUF Loader - parses GGUF files and loads into ThemisDB
/** @brief GGUF Loader - parses GGUF files and loads into ThemisDB. */
class GGUFLoader {
public:
    GGUFLoader();
    explicit GGUFLoader(RocksDBWrapper* db);
    ~GGUFLoader() noexcept;

    // Parse GGUF file header and metadata.
    // Returns false and sets getLastError() on failure (including unsupported
    // quantization formats).
    bool parseFile(const std::string& filepath);
    
    // Get parsed metadata
    const GGUFMetadata& getMetadata() const { return metadata_; }
    
    // Returns a human-readable error description when parseFile() returns false.
    const std::string& getLastError() const { return last_error_; }
    
    // Load tensor data into ThemisDB Blob Store
    // Returns URN of stored model: urn:themis:model:{model_name}:v1
    // Requires RocksDBWrapper to be set (via constructor or setDatabase)
    std::string loadToThemisDB(const std::string& model_name);
    
    // Set the database instance (if not provided in constructor)
    void setDatabase(RocksDBWrapper* db) { db_ = db; }
    
    // Memory-mapped loading for zero-copy access
    void* mmapTensor(const std::string& tensor_name);
    void unmapTensor(void* ptr);
    
    // Extract specific tensor data
    std::vector<uint8_t> getTensorData(const std::string& tensor_name);
    
    /**
     * @brief Validate quantization metadata for a tensor
     * 
     * Checks that the tensor's quantization format is valid:
     * - Block sizes match expected values
     * - Data size is consistent with tensor dimensions
     * - Quantization type is supported
     * 
     * @param tensor_name Name of the tensor to validate
     * @return true if validation passes, false otherwise
     */
    bool validateQuantizationMetadata(const std::string& tensor_name) const;
    
    /**
     * @brief Check whether a GGML quantization type is supported by this loader.
     *
     * Supported formats can be converted to an internal representation for
     * inference and training.  Unsupported formats produce a clear error from
     * parseFile() instead of silently returning raw bytes.
     *
     * Supported: F32, F16, Q4_K (Q4_K_M), Q8_0
     * Not supported: Q4_0, Q4_1, Q5_0, Q5_1, Q8_1, Q5_K, Q6_K, Q2_K, Q3_K
     */
    static bool isFormatSupported(GGMLType type);
    
private:
    GGUFMetadata metadata_;
    std::string filepath_;
    std::string last_error_;  // Set on parseFile() failure
    int fd_ = 0;  // File descriptor for mmap
    void* mmap_base_;
    size_t mmap_size_ = 0;
    std::vector<uint8_t> file_buffer_; // Windows fallback buffer
    RocksDBWrapper* db_ = nullptr;  // Not owned
    
    // Internal parsing helpers
    bool parseHeader();
    bool parseMetadataKV();
    bool parseTensorInfo();
    
    // RAII resource cleanup — safe to call multiple times (idempotent).
    // Used by destructor and at the top of parseFile() to prevent fd/mmap
    // leaks when parseFile() is called more than once on the same object.
    void releaseResources() noexcept;
    
    // Type size helpers
    size_t getDtypeSize(const std::string& dtype) const;
    size_t getGGMLTypeSize(GGMLType type) const;
    
    // Metadata parsing helpers
    bool readString(size_t& offset, std::string& out);
    bool readMetadataValue(size_t& offset, GGUFValueType type, std::string& out);
    
    // Helper to store tensor data in chunks
    bool storeTensorInChunks(const std::string& model_name, 
                            const TensorMetadata& tensor,
                            size_t chunk_size);
    
    // Alignment helper
    size_t alignOffset(size_t offset, size_t alignment = 32);
};

} // namespace llm
} // namespace themis
