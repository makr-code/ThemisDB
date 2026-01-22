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
    std::string name;
    std::vector<int64_t> shape;
    GGMLType type;      // GGML quantization type
    size_t offset;      // Offset in GGUF file
    size_t size;        // Size in bytes
    
    // Helper to get type as string
    std::string type_string() const;
};

// GGUF file metadata
struct GGUFMetadata {
    uint32_t version;
    std::string architecture;  // "llama", "mistral", etc.
    std::unordered_map<std::string, std::string> config;
    std::vector<TensorMetadata> tensors;
    size_t total_size;
    size_t data_offset;  // Offset where tensor data starts
};

// GGUF Loader - parses GGUF files and loads into ThemisDB
class GGUFLoader {
public:
    GGUFLoader();
    explicit GGUFLoader(RocksDBWrapper* db);
    ~GGUFLoader();

    // Parse GGUF file header and metadata
    bool parseFile(const std::string& filepath);
    
    // Get parsed metadata
    const GGUFMetadata& getMetadata() const { return metadata_; }
    
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
    
private:
    GGUFMetadata metadata_;
    std::string filepath_;
    int fd_;  // File descriptor for mmap
    void* mmap_base_;
    size_t mmap_size_;
    std::vector<uint8_t> file_buffer_; // Windows fallback buffer
    RocksDBWrapper* db_ = nullptr;  // Not owned
    
    // Internal parsing helpers
    bool parseHeader();
    bool parseMetadataKV();
    bool parseTensorInfo();
    
    // Type size helpers
    size_t getDtypeSize(const std::string& dtype);
    size_t getGGMLTypeSize(GGMLType type);
    
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
