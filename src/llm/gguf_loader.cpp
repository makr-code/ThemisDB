// THEMIS_GAP_STATS: gaps=3 unimpl=3 stub=0 mock=0 sim=0 todo=0 debt=0 scanned=2026-05-18
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            gguf_loader.cpp                                    ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:49:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   94.0/100                                       ║
    • Total Lines:     754                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • d275653619  2026-04-14  update after codefindings               ║
    • a2d7c07202  2026-04-14  update after codefindings               ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "llm/gguf_loader.h"
#include "storage/rocksdb_wrapper.h"
#ifndef _WIN32
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#else
#include <fstream>
#endif
#include <cstring>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <spdlog/spdlog.h>

namespace themis {
namespace llm {

// Helper: Convert TensorMetadata type to string
std::string TensorMetadata::type_string() const {
    switch (type) {
        case GGMLType::F32: return "F32";
        case GGMLType::F16: return "F16";
        case GGMLType::Q4_0: return "Q4_0";
        case GGMLType::Q4_1: return "Q4_1";
        case GGMLType::Q5_0: return "Q5_0";
        case GGMLType::Q5_1: return "Q5_1";
        case GGMLType::Q8_0: return "Q8_0";
        case GGMLType::Q8_1: return "Q8_1";
        case GGMLType::Q4_K: return "Q4_K_M";
        case GGMLType::Q5_K: return "Q5_K";
        case GGMLType::Q6_K: return "Q6_K";
        case GGMLType::Q8_K: return "Q8_K";
        case GGMLType::I8: return "I8";
        case GGMLType::I16: return "I16";
        case GGMLType::I32: return "I32";
        default: return "UNKNOWN";
    }
}

// isFormatSupported: returns true for quantization types that the loader can
// convert to an internal representation.  Unsupported types cause parseFile()
// to return false with a descriptive error rather than silently returning raw
// bytes that would produce numerical corruption downstream.
bool GGUFLoader::isFormatSupported(GGMLType type) {
    switch (type) {
        case GGMLType::F32:
        case GGMLType::F16:
        case GGMLType::Q4_K:  // Q4_K_M and Q4_K_S share the same enum value
        case GGMLType::Q8_0:
            return true;
        default:
            return false;
    }
}

GGUFLoader::GGUFLoader() 
    : fd_(-1), mmap_base_(nullptr), mmap_size_(0), db_(nullptr) {
}

GGUFLoader::GGUFLoader(RocksDBWrapper* db)
    : fd_(-1), mmap_base_(nullptr), mmap_size_(0), db_(db) {
}

GGUFLoader::~GGUFLoader() {
#ifndef _WIN32
    if (mmap_base_ != nullptr) {
        munmap(mmap_base_, mmap_size_);
    }
    if (fd_ >= 0) {
        close(fd_);
    }
#endif
}

bool GGUFLoader::parseFile(const std::string& filepath) {
    filepath_ = filepath;
    last_error_.clear();
#ifndef _WIN32
    fd_ = open(filepath.c_str(), O_RDONLY);
    if (fd_ < 0) {
        return false;
    }
    struct stat st;
    if (fstat(fd_, &st) < 0) {
        close(fd_);
        fd_ = -1;
        return false;
    }
    mmap_size_ = st.st_size;
    mmap_base_ = mmap(nullptr, mmap_size_, PROT_READ, MAP_PRIVATE, fd_, 0);
    if (mmap_base_ == MAP_FAILED) {
        close(fd_);
        fd_ = -1;
        mmap_base_ = nullptr;
        return false;
    }
#else
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file) {
        return false;
    }
    std::streamsize size = file.tellg();
    if (size <= 0) {
        return false;
    }
    file.seekg(0, std::ios::beg);
    file_buffer_.resize(static_cast<size_t>(size));
    // Note: reinterpret_cast to char* for std::istream::read is explicitly
    // allowed and is the standard way to read binary data into a buffer
    if (!file.read(reinterpret_cast<char*>(file_buffer_.data()), size)) {
        file_buffer_.clear();
        return false;
    }
    mmap_size_ = static_cast<size_t>(size);
    mmap_base_ = file_buffer_.data();
#endif
    
    // Parse GGUF structure
    if (!parseHeader()) {
        return false;
    }
    
    if (!parseMetadataKV()) {
        return false;
    }
    
    if (!parseTensorInfo()) {
        return false;
    }
    
    metadata_.total_size = mmap_size_;
    return true;
}

bool GGUFLoader::parseHeader() {
    // GGUF v3 header structure:
    // - Magic: "GGUF" (4 bytes)
    // - Version: uint32_t (4 bytes)
    // - Tensor count: uint64_t (8 bytes)
    // - Metadata KV count: uint64_t (8 bytes)
    
    const char* data = static_cast<const char*>(mmap_base_);
    if (mmap_size_ < 24) {  // Minimum header size
        return false;
    }
    
    // Check magic number
    if (std::memcmp(data, "GGUF", 4) != 0) {
        return false;
    }
    
    // Read version
    uint32_t version;
    std::memcpy(&version, data + 4, sizeof(uint32_t));
    metadata_.version = version;
    
    // Only support version 3
    if (version != 3) {
        return false;
    }
    
    // Read tensor count and metadata count
    uint64_t tensor_count, kv_count;
    std::memcpy(&tensor_count, data + 8, sizeof(uint64_t));
    std::memcpy(&kv_count, data + 16, sizeof(uint64_t));
    
    // Basic sanity checks
    if (tensor_count > 100000 || kv_count > 10000) {
        return false;  // Unreasonable counts
    }
    
    return true;
}

bool GGUFLoader::readString(size_t& offset, std::string& out) {
    if (offset + 8 > mmap_size_) return false;
    
    const char* data = static_cast<const char*>(mmap_base_);
    uint64_t len;
    std::memcpy(&len, data + offset, sizeof(uint64_t));
    offset += 8;
    
    if (len > 1000000 || offset + len > mmap_size_) return false;
    
    out.assign(data + offset, len);
    offset += len;
    return true;
}

bool GGUFLoader::readMetadataValue(size_t& offset, GGUFValueType type, std::string& out) {
    const char* data = static_cast<const char*>(mmap_base_);
    
    switch (type) {
        case GGUFValueType::UINT8:
        case GGUFValueType::INT8:
        case GGUFValueType::BOOL: {
            if (offset + 1 > mmap_size_) return false;
            uint8_t val;
            std::memcpy(&val, data + offset, 1);
            offset += 1;
            out = std::to_string(val);
            return true;
        }
        case GGUFValueType::UINT16:
        case GGUFValueType::INT16: {
            if (offset + 2 > mmap_size_) return false;
            uint16_t val;
            std::memcpy(&val, data + offset, 2);
            offset += 2;
            out = std::to_string(val);
            return true;
        }
        case GGUFValueType::UINT32:
        case GGUFValueType::INT32:
        case GGUFValueType::FLOAT32: {
            if (offset + 4 > mmap_size_) return false;
            uint32_t val;
            std::memcpy(&val, data + offset, 4);
            offset += 4;
            out = std::to_string(val);
            return true;
        }
        case GGUFValueType::UINT64:
        case GGUFValueType::INT64:
        case GGUFValueType::FLOAT64: {
            if (offset + 8 > mmap_size_) return false;
            uint64_t val;
            std::memcpy(&val, data + offset, 8);
            offset += 8;
            out = std::to_string(val);
            return true;
        }
        case GGUFValueType::STRING: {
            return readString(offset, out);
        }
        case GGUFValueType::ARRAY: {
            // For arrays, just skip for now (simplified)
            if (offset + 12 > mmap_size_) return false;
            uint32_t arr_type;
            uint64_t arr_len;
            std::memcpy(&arr_type, data + offset, 4);
            std::memcpy(&arr_len, data + offset + 4, 8);
            offset += 12;
            
            // Skip array elements based on type
            GGUFValueType elem_type = static_cast<GGUFValueType>(arr_type);
            for (uint64_t i = 0; i < arr_len; ++i) {
                std::string dummy;
                if (!readMetadataValue(offset, elem_type, dummy)) {
                    return false;
                }
            }
            out = "[array:" + std::to_string(arr_len) + "]";
            return true;
        }
        default:
            return false;
    }
}

bool GGUFLoader::parseMetadataKV() {
    const char* data = static_cast<const char*>(mmap_base_);
    
    // Start after header (24 bytes)
    size_t offset = 24;
    
    // Read KV count
    uint64_t kv_count;
    std::memcpy(&kv_count, data + 16, sizeof(uint64_t));
    
    // Parse each key-value pair
    for (uint64_t i = 0; i < kv_count; ++i) {
        std::string key;
        if (!readString(offset, key)) return false;
        
        // Read value type
        if (offset + 4 > mmap_size_) return false;
        uint32_t value_type_raw;
        std::memcpy(&value_type_raw, data + offset, 4);
        offset += 4;
        
        GGUFValueType value_type = static_cast<GGUFValueType>(value_type_raw);
        
        // Read value
        std::string value;
        if (!readMetadataValue(offset, value_type, value)) return false;
        
        // Store important metadata
        metadata_.config[key] = value;
        
        // Extract architecture if present
        if (key == "general.architecture") {
            metadata_.architecture = value;
        }
    }
    
    return true;
}

size_t GGUFLoader::alignOffset(size_t offset, size_t alignment) {
    return (offset + alignment - 1) & ~(alignment - 1);
}

bool GGUFLoader::parseTensorInfo() {
    const char* data = static_cast<const char*>(mmap_base_);
    
    // Get current offset (after metadata)
    size_t offset = 24;  // Start after header
    
    // Skip metadata KV pairs to get to tensor info
    uint64_t kv_count;
    std::memcpy(&kv_count, data + 16, sizeof(uint64_t));
    
    // Re-skip metadata (we already parsed it)
    offset = 24;
    for (uint64_t i = 0; i < kv_count; ++i) {
        std::string key, value;
        if (!readString(offset, key)) return false;
        
        if (offset + 4 > mmap_size_) return false;
        uint32_t value_type_raw;
        std::memcpy(&value_type_raw, data + offset, 4);
        offset += 4;
        
        if (!readMetadataValue(offset, static_cast<GGUFValueType>(value_type_raw), value)) {
            return false;
        }
    }
    
    // Now parse tensor information
    uint64_t tensor_count;
    std::memcpy(&tensor_count, data + 8, sizeof(uint64_t));
    
    metadata_.tensors.clear();
    metadata_.tensors.reserve(tensor_count);
    
    for (uint64_t i = 0; i < tensor_count; ++i) {
        TensorMetadata tensor;
        
        // Read tensor name
        if (!readString(offset, tensor.name)) return false;
        
        // Read n_dims
        if (offset + 4 > mmap_size_) return false;
        uint32_t n_dims;
        std::memcpy(&n_dims, data + offset, 4);
        offset += 4;
        
        // Read dimensions
        if (offset + n_dims * 8 > mmap_size_) return false;
        tensor.shape.resize(n_dims);
        for (uint32_t j = 0; j < n_dims; ++j) {
            uint64_t dim;
            std::memcpy(&dim, data + offset, 8);
            offset += 8;
            tensor.shape[j] = static_cast<int64_t>(dim);
        }
        
        // Read tensor type
        if (offset + 4 > mmap_size_) return false;
        uint32_t type_raw;
        std::memcpy(&type_raw, data + offset, 4);
        offset += 4;
        tensor.type = static_cast<GGMLType>(type_raw);
        
        // Reject unsupported quantization formats immediately.  Silently
        // continuing would return raw quantized bytes to callers, causing
        // downstream numerical corruption.
        if (!isFormatSupported(tensor.type)) {
            last_error_ = "Unsupported quantization format " + tensor.type_string()
                          + " in tensor '" + tensor.name + "'"
                          + ". Supported formats: F32, F16, Q4_K_M, Q8_0."
                          + " Download a Q4_K_M or Q8_0 variant of this model.";
            spdlog::error("GGUFLoader: {}", last_error_);
            return false;
        }
        
        // Read tensor offset
        if (offset + 8 > mmap_size_) return false;
        uint64_t tensor_offset;
        std::memcpy(&tensor_offset, data + offset, 8);
        offset += 8;
        
        // Calculate tensor size based on type and shape
        size_t num_elements = 1;
        for (auto dim : tensor.shape) {
            num_elements *= dim;
        }
        tensor.size = num_elements * getGGMLTypeSize(tensor.type);
        tensor.offset = tensor_offset;
        
        metadata_.tensors.push_back(tensor);
    }
    
    // Store data offset (aligned to 32 bytes)
    metadata_.data_offset = alignOffset(offset, 32);
    
    return true;
}

size_t GGUFLoader::getGGMLTypeSize(GGMLType type) const {
    switch (type) {
        case GGMLType::F32: return 4;
        case GGMLType::F16: return 2;
        case GGMLType::Q4_0: return 18;  // 32 values per block
        case GGMLType::Q4_1: return 20;
        case GGMLType::Q5_0: return 22;
        case GGMLType::Q5_1: return 24;
        case GGMLType::Q8_0: return 34;  // 32 values per block
        case GGMLType::Q8_1: return 36;
        case GGMLType::Q4_K: return 144; // Q4_K_M: 256 values per block
        case GGMLType::Q5_K: return 176;
        case GGMLType::Q6_K: return 210;
        case GGMLType::Q8_K: return 292;
        case GGMLType::I8: return 1;
        case GGMLType::I16: return 2;
        case GGMLType::I32: return 4;
        default: return 4;  // Default to FP32 size
    }
}

std::string GGUFLoader::loadToThemisDB(const std::string& model_name) {
    if (!db_) {
        throw std::runtime_error("RocksDBWrapper not set. Use setDatabase() or constructor with db parameter.");
    }
    
    if (filepath_.empty() || mmap_base_ == nullptr) {
        throw std::runtime_error("No GGUF file parsed. Call parseFile() first.");
    }
    
    // Helper lambda to escape JSON strings (basic escaping)
    auto escapeJson = [](const std::string& str) -> std::string {
        std::string escaped;
        escaped.reserve(str.size());
        for (char c : str) {
            switch (c) {
                case '"': escaped += "\\\""; break;
                case '\\': escaped += "\\\\"; break;
                case '\b': escaped += "\\b"; break;
                case '\f': escaped += "\\f"; break;
                case '\n': escaped += "\\n"; break;
                case '\r': escaped += "\\r"; break;
                case '\t': escaped += "\\t"; break;
                default: escaped += c; break;
            }
        }
        return escaped;
    };
    
    // Generate model URN
    std::string model_urn = "urn:themis:model:" + model_name + ":v1";
    std::string metadata_key = "llm:model:" + model_name + ":metadata";
    
    // 1. Store metadata
    std::ostringstream metadata_json;
    metadata_json << "{"
                  << "\"version\":" << metadata_.version << ","
                  << "\"architecture\":\"" << escapeJson(metadata_.architecture) << "\","
                  << "\"total_size\":" << metadata_.total_size << ","
                  << "\"num_tensors\":" << metadata_.tensors.size() << ","
                  << "\"urn\":\"" << escapeJson(model_urn) << "\","
                  << "\"config\":{";
    
    bool first = true;
    for (const auto& [key, value] : metadata_.config) {
        if (!first) metadata_json << ",";
        metadata_json << "\"" << escapeJson(key) << "\":\"" << escapeJson(value) << "\"";
        first = false;
    }
    metadata_json << "},\"tensors\":[";
    
    first = true;
    for (const auto& tensor : metadata_.tensors) {
        if (!first) metadata_json << ",";
        metadata_json << "{"
                     << "\"name\":\"" << escapeJson(tensor.name) << "\","
                     << "\"dtype\":\"" << escapeJson(tensor.type_string()) << "\","
                     << "\"size\":" << tensor.size << ","
                     << "\"offset\":" << tensor.offset << ","
                     << "\"shape\":[";
        for (size_t i = 0; i < tensor.shape.size(); ++i) {
            if (i > 0) metadata_json << ",";
            metadata_json << tensor.shape[i];
        }
        metadata_json << "]}";
        first = false;
    }
    metadata_json << "]}";
    
    std::string metadata_str = metadata_json.str();
    std::vector<uint8_t> metadata_bytes(metadata_str.begin(), metadata_str.end());
    
    if (!db_->put(metadata_key, metadata_bytes)) {
        throw std::runtime_error("Failed to store model metadata in RocksDB");
    }
    
    // 2. Store tensor data in chunks (64 MB chunks to avoid memory exhaustion)
    const size_t CHUNK_SIZE = 64 * 1024 * 1024; // 64 MB
    
    for (const auto& tensor : metadata_.tensors) {
        if (!storeTensorInChunks(model_name, tensor, CHUNK_SIZE)) {
            throw std::runtime_error("Failed to store tensor: " + tensor.name);
        }
    }
    
    // 3. Store model URN mapping
    std::string urn_key = "llm:model:urn:" + model_urn;
    std::vector<uint8_t> model_name_bytes(model_name.begin(), model_name.end());
    if (!db_->put(urn_key, model_name_bytes)) {
        throw std::runtime_error("Failed to store URN mapping in RocksDB");
    }
    
    return model_urn;
}

bool GGUFLoader::storeTensorInChunks(const std::string& model_name,
                                     const TensorMetadata& tensor,
                                     size_t chunk_size) {
    // Get pointer to tensor data in mmap region
    // Tensor data starts at data_offset + tensor.offset
    void* tensor_ptr = static_cast<char*>(mmap_base_) + metadata_.data_offset + tensor.offset;
    
    size_t remaining = tensor.size;
    size_t chunk_index = 0;
    size_t offset = 0;
    
    // Use a reusable buffer to avoid repeated allocations
    std::vector<uint8_t> chunk_buffer;
    chunk_buffer.reserve(chunk_size);
    
    while (remaining > 0) {
        size_t current_chunk_size = std::min(remaining, chunk_size);
        
        // Create chunk key: llm:model:{model_name}:tensor:{tensor_name}:chunk:{index}
        std::ostringstream chunk_key;
        chunk_key << "llm:model:" << model_name 
                  << ":tensor:" << tensor.name 
                  << ":chunk:" << chunk_index;
        
        // Resize buffer and copy chunk data
        chunk_buffer.resize(current_chunk_size);
        std::memcpy(chunk_buffer.data(), 
                   static_cast<char*>(tensor_ptr) + offset, 
                   current_chunk_size);
        
        // Store chunk in RocksDB (will automatically use BlobDB for large chunks)
        if (!db_->put(chunk_key.str(), chunk_buffer)) {
            return false;
        }
        
        remaining -= current_chunk_size;
        offset += current_chunk_size;
        chunk_index++;
    }
    
    // Store chunk count for this tensor
    std::ostringstream count_key;
    count_key << "llm:model:" << model_name 
              << ":tensor:" << tensor.name 
              << ":chunk_count";
    
    std::string count_str = std::to_string(chunk_index);
    std::vector<uint8_t> count_bytes(count_str.begin(), count_str.end());
    
    return db_->put(count_key.str(), count_bytes);
}

void* GGUFLoader::mmapTensor(const std::string& tensor_name) {
    // Find tensor
    for (const auto& tensor : metadata_.tensors) {
        if (tensor.name == tensor_name) {
            // Return pointer to tensor data in mmap region
            return static_cast<char*>(mmap_base_) + metadata_.data_offset + tensor.offset;
        }
    }
    return nullptr;
}

void GGUFLoader::unmapTensor(void* /*ptr*/) {
    // No-op for now since we keep entire file mapped
    // Individual tensor unmapping not needed with full file mmap
}

std::vector<uint8_t> GGUFLoader::getTensorData(const std::string& tensor_name) {
    void* ptr = mmapTensor(tensor_name);
    if (ptr == nullptr) {
        return {};
    }
    
    // Find tensor size
    for (const auto& tensor : metadata_.tensors) {
        if (tensor.name == tensor_name) {
            std::vector<uint8_t> data(tensor.size);
            std::memcpy(data.data(), ptr, tensor.size);
            return data;
        }
    }
    
    return {};
}

bool GGUFLoader::validateQuantizationMetadata(const std::string& tensor_name) const {
    // Find tensor
    const TensorMetadata* tensor = nullptr;
    for (const auto& t : metadata_.tensors) {
        if (t.name == tensor_name) {
            tensor = &t;
            break;
        }
    }
    
    if (tensor == nullptr) {
        return false;  // Tensor not found
    }
    
    // Calculate expected number of elements
    size_t num_elements = 1;
    for (auto dim : tensor->shape) {
        if (dim <= 0) {
            return false;  // Invalid dimension
        }
        num_elements *= static_cast<size_t>(dim);
    }
    
    // Validate block size and data size based on quantization type
    size_t expected_block_size = 0;
    size_t elements_per_block = 0;
    
    switch (tensor->type) {
        case GGMLType::F32:
            expected_block_size = 4;
            elements_per_block = 1;
            break;
        case GGMLType::F16:
            expected_block_size = 2;
            elements_per_block = 1;
            break;
        case GGMLType::Q4_0:
            expected_block_size = 18;
            elements_per_block = 32;
            break;
        case GGMLType::Q4_1:
            expected_block_size = 20;
            elements_per_block = 32;
            break;
        case GGMLType::Q5_0:
            expected_block_size = 22;
            elements_per_block = 32;
            break;
        case GGMLType::Q5_1:
            expected_block_size = 24;
            elements_per_block = 32;
            break;
        case GGMLType::Q8_0:
            expected_block_size = 34;
            elements_per_block = 32;
            break;
        case GGMLType::Q8_1:
            expected_block_size = 36;
            elements_per_block = 32;
            break;
        case GGMLType::Q4_K:
            expected_block_size = 144;
            elements_per_block = 256;
            break;
        case GGMLType::Q5_K:
            expected_block_size = 176;
            elements_per_block = 256;
            break;
        case GGMLType::Q6_K:
            expected_block_size = 210;
            elements_per_block = 256;
            break;
        case GGMLType::Q8_K:
            expected_block_size = 292;
            elements_per_block = 256;
            break;
        case GGMLType::I8:
            expected_block_size = 1;
            elements_per_block = 1;
            break;
        case GGMLType::I16:
            expected_block_size = 2;
            elements_per_block = 1;
            break;
        case GGMLType::I32:
            expected_block_size = 4;
            elements_per_block = 1;
            break;
        default:
            return false;  // Unsupported type
    }
    
    // Calculate expected data size
    size_t expected_size;
    if (elements_per_block > 1) {
        // Block-based quantization
        size_t num_blocks = (num_elements + elements_per_block - 1) / elements_per_block;
        expected_size = num_blocks * expected_block_size;
    } else {
        // Element-wise encoding
        expected_size = num_elements * expected_block_size;
    }
    
    // Validate tensor size matches expected size
    if (tensor->size != expected_size) {
        return false;  // Size mismatch
    }
    
    // Validate tensor offset is within file bounds
    if (metadata_.data_offset + tensor->offset + tensor->size > metadata_.total_size) {
        return false;  // Tensor extends beyond file
    }
    
    return true;
}

size_t GGUFLoader::getDtypeSize(const std::string& dtype) const {
    if (dtype == "float32") return 4;
    if (dtype == "float16") return 2;
    if (dtype == "int8") return 1;
    if (dtype == "int32") return 4;
    return 0;
}

} // namespace llm
} // namespace themis
