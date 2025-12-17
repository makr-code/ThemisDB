#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>

namespace themis {
namespace llm {

// GGUF tensor metadata
struct TensorMetadata {
    std::string name;
    std::vector<int64_t> shape;
    std::string dtype;  // "float32", "float16", "int8", etc.
    size_t offset;      // Offset in GGUF file
    size_t size;        // Size in bytes
};

// GGUF file metadata
struct GGUFMetadata {
    std::string version;
    std::string architecture;  // "llama", "mistral", etc.
    std::unordered_map<std::string, std::string> config;
    std::vector<TensorMetadata> tensors;
    size_t total_size;
};

// GGUF Loader - parses GGUF files and loads into ThemisDB
class GGUFLoader {
public:
    GGUFLoader();
    ~GGUFLoader();

    // Parse GGUF file header and metadata
    bool parseFile(const std::string& filepath);
    
    // Get parsed metadata
    const GGUFMetadata& getMetadata() const { return metadata_; }
    
    // Load tensor data into ThemisDB Blob Store
    // Returns URN of stored model: urn:themis:model:{model_name}:v1
    std::string loadToThemisDB(const std::string& model_name);
    
    // Memory-mapped loading for zero-copy access
    void* mmapTensor(const std::string& tensor_name);
    void unmapTensor(void* ptr);
    
    // Extract specific tensor data
    std::vector<uint8_t> getTensorData(const std::string& tensor_name);
    
private:
    GGUFMetadata metadata_;
    std::string filepath_;
    int fd_;  // File descriptor for mmap
    void* mmap_base_;
    size_t mmap_size_;
    
    // Internal parsing helpers
    bool parseHeader();
    bool parseMetadataKV();
    bool parseTensorInfo();
    size_t getDtypeSize(const std::string& dtype);
};

} // namespace llm
} // namespace themis
