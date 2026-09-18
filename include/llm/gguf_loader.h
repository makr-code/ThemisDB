/**
 * @file gguf_loader.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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
    /**
     * @brief Tensor Metadata.
     * @return Return value.
     */
    virtual ~TensorMetadata() = default;
    std::string name;
    std::vector<int64_t> shape;
    GGMLType type;      // GGML quantization type
    size_t offset = 0;      // Offset in GGUF file
    size_t size = 0;        // Size in bytes
    
    /**
     * @brief Helper to get type as string
     * @return Return value.
     */
    std::string type_string() const;
};

// GGUF file metadata
struct GGUFMetadata {
    /**
     * @brief GGUFMetadata.
     * @return Return value.
     */
    virtual ~GGUFMetadata() = default;
    uint32_t version = 0;
    std::string architecture;  // "llama", "mistral", etc.
    std::unordered_map<std::string, std::string> config;
    std::vector<TensorMetadata> tensors;
    size_t total_size = 0;
    size_t data_offset = 0;  // Offset where tensor data starts
};

// GGUF Loader - parses GGUF files and loads into ThemisDB
class GGUFLoader {
public:
    GGUFLoader();
    /**
     * @brief GGUFLoader.
     * @param[in,out] db Input/output parameter.
     * @return Return value.
     */
    explicit GGUFLoader(RocksDBWrapper* db);
    ~GGUFLoader() noexcept;

    /**
     * @brief Parse GGUF file header and metadata.
     * @param[in] filepath Input parameter.
     * @return True when the operation succeeds.
     * @details Returns false and sets getLastError() on failure (including unsupported quantization formats).
     */
    bool parseFile(const std::string& filepath);
    
    // Get parsed metadata
    const GGUFMetadata& getMetadata() const { return metadata_; }
    
    // Returns a human-readable error description when parseFile() returns false.
    const std::string& getLastError() const { return last_error_; }
    
    /**
     * @brief Load tensor data into ThemisDB Blob Store Returns URN of stored model: urn:themis:model:{model_name}:v1 Requires RocksDBWrapper to be set (via constructor or setDatabase)
     * @param[in] model_name Name of the model.
     * @return Return value.
     */
    std::string loadToThemisDB(const std::string& model_name);
    
    /**
     * @brief Set the database instance (if not provided in constructor)
     * @param[in,out] db Input/output parameter.
     * @details Implements setDatabase without additional internal calls.
     */
    void setDatabase(RocksDBWrapper* db) { db_ = db; }
    
    /**
     * @brief Memory-mapped loading for zero-copy access
     * @param[in] tensor_name Name of the tensor.
     * @return Pointer to the result.
     */
    void* mmapTensor(const std::string& tensor_name);
    /**
     * @brief Unmap Tensor.
     * @param[in,out] ptr Input/output parameter.
     * @note Exception safety: noexcept.
     */
    void unmapTensor(void* ptr) noexcept;
    
    /**
     * @brief Extract specific tensor data
     * @param[in] tensor_name Name of the tensor.
     * @return Return value.
     */
    std::vector<uint8_t> getTensorData(const std::string& tensor_name);
    
    /**
     * @brief Validate Quantization Metadata.
     * @param[in] tensor_name Name of the tensor.
     * @return True when the operation succeeds.
     */
    bool validateQuantizationMetadata(const std::string& tensor_name) const;
    
    /**
     * @brief Is Format Supported.
     * @param[in] type Input parameter.
     * @return True when the operation succeeds.
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
    /**
     * @brief Parse Header.
     * @return True when the operation succeeds.
     */
    bool parseHeader();
    /**
     * @brief Parse Metadata KV.
     * @return True when the operation succeeds.
     */
    bool parseMetadataKV();
    /**
     * @brief Parse Tensor Info.
     * @return True when the operation succeeds.
     */
    bool parseTensorInfo();
    
    /**
     * @brief RAII resource cleanup — safe to call multiple times (idempotent).
     * @note Exception safety: noexcept.
     * @details Used by destructor and at the top of parseFile() to prevent fd/mmap leaks when parseFile() is called more than once on the same object.
     */
    void releaseResources() noexcept;
    
    // Type size helpers
    /**
     * @brief Get Dtype Size.
     * @param[in] dtype Input parameter.
     * @return Return value.
     */
    size_t getDtypeSize(const std::string& dtype) const;
    /**
     * @brief Get GGMLType Size.
     * @param[in] type Input parameter.
     * @return Return value.
     */
    size_t getGGMLTypeSize(GGMLType type) const;
    
    // Metadata parsing helpers
    /**
     * @brief Read String.
     * @param[in,out] offset Input/output parameter.
     * @param[in,out] out Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool readString(size_t& offset, std::string& out);
    /**
     * @brief Read Metadata Value.
     * @param[in,out] offset Input/output parameter.
     * @param[in] type Input parameter.
     * @param[in,out] out Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool readMetadataValue(size_t& offset, GGUFValueType type, std::string& out);
    
    /**
     * @brief Helper to store tensor data in chunks
     * @param[in] model_name Name of the model.
     * @param[in] tensor Input parameter.
     * @param[in] chunk_size Input parameter.
     * @return True when the operation succeeds.
     */
    bool storeTensorInChunks(const std::string& model_name, 
                            const TensorMetadata& tensor,
                            size_t chunk_size);
    
    // Alignment helper
    size_t alignOffset(size_t offset, size_t alignment = 32);
};

} // namespace llm
} // namespace themis
