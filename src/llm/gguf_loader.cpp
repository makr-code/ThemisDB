/**
 * @file gguf_loader.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.48
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 92/100
 * @note Gap Summary: total=5; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=2, Debt=0, C=0, H=0, M=8, L=0
 * @note Status: Production Ready - RAII hardening complete
 * @note This block is auto-generated and will be overwritten.
 * @note RAII improvements: File descriptor and mmap region now use RAII wrappers
 *       with exception-safe cleanup guarantees.
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

// ═══════════════════════════════════════════════════════════
// RAII Helpers for Resource Management
// ═══════════════════════════════════════════════════════════

#ifndef _WIN32
/**
 * @brief RAII wrapper for file descriptors
 * Ensures file is closed even if exceptions occur during processing.
 */
class FileDescriptorGuard {
public:
    explicit FileDescriptorGuard([[maybe_unused]] int fd) noexcept : fd_(fd) {}
    
    ~FileDescriptorGuard() noexcept {
        if (fd_ >= 0) {
            close(fd_);
            fd_ = -1;
        }
    }
    
    // Prevent copying
    FileDescriptorGuard(const FileDescriptorGuard&) = delete;
    FileDescriptorGuard& operator=(const FileDescriptorGuard&) = delete;
    
    // Allow moving
    FileDescriptorGuard(FileDescriptorGuard&& other) noexcept : fd_(other.release()) {}
    FileDescriptorGuard& operator=(FileDescriptorGuard&& other) noexcept {
        reset(other.release());
        return *this;
    }
    
    int get() const noexcept { return fd_; }
    int release() noexcept {
        int result = fd_;
        fd_ = -1;
        return result;
    }
    void reset([[maybe_unused]] int fd) noexcept {
        if (fd_ >= 0) {
            close(fd_);
        }
        fd_ = fd;
    }

private:
    int fd_;
};

/**
 * @brief RAII wrapper for mmap regions
 * Ensures mmap region is unmapped even if exceptions occur during processing.
 */
class MmapGuard {
public:
    MmapGuard(void* ptr, size_t size) noexcept : ptr_(ptr), size_(size) {}
    
    ~MmapGuard() noexcept {
        if (ptr_ != nullptr && ptr_ != MAP_FAILED) {
            munmap(ptr_, size_);
            ptr_ = nullptr;
            size_ = 0;
        }
    }
    
    // Prevent copying
    MmapGuard(const MmapGuard&) = delete;
    MmapGuard& operator=(const MmapGuard&) = delete;
    
    // Allow moving
    MmapGuard(MmapGuard&& other) noexcept : ptr_(other.release()), size_(other.size_) {
        other.size_ = 0;
    }
    MmapGuard& operator=(MmapGuard&& other) noexcept {
        reset(other.release(), other.size_);
        other.size_ = 0;
        return *this;
    }
    
    void* get() const noexcept { return ptr_; }
    void* release() noexcept {
        void* result = ptr_;
        ptr_ = nullptr;
        return result;
    }
    void reset(void* ptr, size_t size) noexcept {
        if (ptr_ != nullptr && ptr_ != MAP_FAILED) {
            munmap(ptr_, size_);
        }
        ptr_ = ptr;
        size_ = size;
    }

private:
    void* ptr_;
    size_t size_;
};
#endif

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
        [[fallthrough]];
        case GGMLType::F16:
        [[fallthrough]];
        case GGMLType::Q4_K:  // Q4_K_M and Q4_K_S share the same enum value
        [[fallthrough]];
        case GGMLType::Q8_0:
            return true;
        default:
            return false;
    }
}

// ═══════════════════════════════════════════════════════════
// BATCH 1.3: Tensor Buffer Validation Helpers
// ═══════════════════════════════════════════════════════════

/**
 * @brief Validates tensor metadata has valid shape information
 * @param tensor Tensor metadata to validate
 * @param tensor_name Name for error messaging
 * @throw std::runtime_error if shape is invalid or empty
 * @pre tensor shape must not be empty
 */
void validateTensorShape(const TensorMetadata& tensor, const std::string& tensor_name) {
    if (tensor.shape.empty()) {
        const std::string error_msg = "Tensor shape is empty for: " + tensor_name;
        spdlog::error("{}", error_msg);
        throw std::runtime_error(error_msg);
    }
    
    for (size_t i = 0; i < tensor.shape.size(); ++i) {
        if (tensor.shape[i] <= 0) {
            const std::string error_msg = "Tensor dimension[" + std::to_string(i) + "] is invalid (<=0) for: " + tensor_name;
            spdlog::error("{}", error_msg);
            throw std::runtime_error(error_msg);
        }
    }
}

/**
 * @brief Validates tensor buffer pointer and size
 * @param buffer Pointer to tensor buffer data
 * @param buffer_size Size of buffer in bytes
 * @param tensor_name Name for error messaging
 * @throw std::runtime_error if buffer is null or size is invalid
 * @pre buffer must be non-null and size must be positive
 */
void validateTensorBuffer(const void* buffer, size_t buffer_size, const std::string& tensor_name) {
    if (!buffer) {
        const std::string error_msg = "Tensor buffer is null for: " + tensor_name;
        spdlog::error("{}", error_msg);
        throw std::runtime_error(error_msg);
    }
    
    if (buffer_size == 0) {
        const std::string error_msg = "Tensor buffer size is zero for: " + tensor_name;
        spdlog::error("{}", error_msg);
        throw std::runtime_error(error_msg);
    }
}

/**
 * @brief Validates tensor offset is within file bounds
 * @param offset Offset into file
 * @param tensor_size Size of tensor data
 * @param file_size Total file size
 * @param tensor_name Name for error messaging
 * @throw std::runtime_error if offset or size would exceed file bounds
 * @pre offset + tensor_size must not exceed file_size
 */
void validateTensorOffset(size_t offset, size_t tensor_size, size_t file_size, const std::string& tensor_name) {
    if (offset >= file_size) {
        const std::string error_msg = "Tensor offset (" + std::to_string(offset) + 
                                      ") exceeds file size (" + std::to_string(file_size) + ") for: " + tensor_name;
        spdlog::error("{}", error_msg);
        throw std::runtime_error(error_msg);
    }
    
    if (offset + tensor_size > file_size || offset + tensor_size < offset) {  // Check for overflow
        const std::string error_msg = "Tensor data [offset=" + std::to_string(offset) + 
                                      ", size=" + std::to_string(tensor_size) + 
                                      "] exceeds file bounds [file_size=" + std::to_string(file_size) + "] for: " + tensor_name;
        spdlog::error("{}", error_msg);
        throw std::runtime_error(error_msg);
    }
}

GGUFLoader::GGUFLoader() 
    : fd_(-1), mmap_base_(nullptr), mmap_size_(0), db_(nullptr) {
}

GGUFLoader::GGUFLoader(RocksDBWrapper* db)
    : fd_(-1), mmap_base_(nullptr), mmap_size_(0), db_(db) {
}

GGUFLoader::~GGUFLoader() noexcept {
    releaseResources();
}

void GGUFLoader::releaseResources() noexcept {
#ifndef _WIN32
    if (mmap_base_ != nullptr && mmap_base_ != MAP_FAILED) {
        munmap(mmap_base_, mmap_size_);
    }
    mmap_base_ = nullptr;
    mmap_size_ = 0;
    
    if (fd_ >= 0) {
        close(fd_);
    }
    fd_ = -1;
#else
    mmap_base_ = nullptr;
    mmap_size_ = 0;
    file_buffer_.clear();
    file_buffer_.shrink_to_fit();
#endif
}

bool GGUFLoader::parseFile(const std::string& filepath) {
    // Release any resources from a previous parse before opening new ones.
    // Without this guard, calling parseFile() twice leaks the first fd/mmap.
    releaseResources();

    if (filepath.empty()) {
        last_error_ = "File path is empty";
        return false;
    }

    filepath_ = filepath;
    last_error_.clear();
    
#ifndef _WIN32
    // Use RAII guards to ensure cleanup on exception or early return
    int fd = open(filepath.c_str(), O_RDONLY);
    if (fd < 0) {
        last_error_ = "Failed to open file: " + filepath;
        return false;
    }
    
    FileDescriptorGuard fd_guard(fd);
    
    struct stat st;
    if (fstat(fd, &st) < 0) {
        last_error_ = "Failed to stat file: " + filepath;
        return false;
    }
    
    if (st.st_size <= 0) {
        last_error_ = "File is empty or invalid: " + filepath;
        return false;
    }
    
    size_t mmap_size = static_cast<size_t>(st.st_size);
    void* mmap_base = mmap(nullptr, mmap_size, PROT_READ, MAP_PRIVATE, fd, 0);
    
    if (mmap_base == MAP_FAILED || mmap_base == nullptr) {
        last_error_ = "Failed to mmap file: " + filepath;
        return false;
    }
    
    MmapGuard mmap_guard(mmap_base, mmap_size);
    
    // Move ownership from guards to member variables after successful mapping
    // This is safe because we'll release the guards without cleanup
    mmap_base_ = mmap_guard.release();
    mmap_size_ = mmap_size;
    fd_ = fd_guard.release();
    
    // BATCH 1.3: Validate mmap result before proceeding
    if (!mmap_base_ || mmap_size_ == 0) {
        last_error_ = "GGUF file mapping failed: mmap_base is null or size is zero";
        releaseResources();
        return false;
    }
    
#else
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file) {
        last_error_ = "Failed to open file: " + filepath;
        return false;
    }
    
    std::streamsize size = file.tellg();
    if (size <= 0) {
        last_error_ = "File is empty or invalid: " + filepath;
        return false;
    }
    
    file.seekg(0, std::ios::beg);
    try {
        file_buffer_.resize(static_cast<size_t>(size));
    } catch (const std::exception& e) {
        last_error_ = "Memory allocation failed: " + std::string(e.what());
        file_buffer_.clear();
        return false;
    }
    
    // Note: reinterpret_cast to char* for std::istream::read is explicitly
    // allowed and is the standard way to read binary data into a buffer
    if (!file.read(reinterpret_cast<char*>(file_buffer_.data()), size)) {
        last_error_ = "Failed to read file: " + filepath;
        file_buffer_.clear();
        file_buffer_.shrink_to_fit();
        return false;
    }
    
    mmap_size_ = static_cast<size_t>(size);
    mmap_base_ = file_buffer_.data();
    
    // BATCH 1.3: Validate buffer result before proceeding
    if (!mmap_base_ || mmap_size_ == 0) {
        last_error_ = "GGUF file buffer allocation failed: buffer is null or size is zero";
        file_buffer_.clear();
        return false;
    }
#endif
    
    // Parse GGUF structure
    if (!parseHeader()) {
        releaseResources();
        return false;
    }
    
    if (!parseMetadataKV()) {
        releaseResources();
        return false;
    }
    
    if (!parseTensorInfo()) {
        releaseResources();
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
    if (offset + 8 > mmap_size_) {
      return false;
    }
    
    const char* data = static_cast<const char*>(mmap_base_);
    uint64_t len;
    std::memcpy(&len, data + offset, sizeof(uint64_t));
    offset += 8;
    
    if (len > 1000000 || offset + len > mmap_size_) {
      return false;
    }
    
    out.assign(data + offset, len);
    offset += len;
    return true;
}

bool GGUFLoader::readMetadataValue(size_t& offset, GGUFValueType type, std::string& out) {
    const char* data = static_cast<const char*>(mmap_base_);
    
    switch (type) {
        case GGUFValueType::UINT8:
        [[fallthrough]];
        case GGUFValueType::INT8:
        [[fallthrough]];
        case GGUFValueType::BOOL: {
            if (offset + 1 > mmap_size_) {
              return false;
            }
            uint8_t val;
            std::memcpy(&val, data + offset, 1);
            offset += 1;
            out = std::to_string(val);
            return true;
        }
        case GGUFValueType::UINT16:
        [[fallthrough]];
        case GGUFValueType::INT16: {
            if (offset + 2 > mmap_size_) {
              return false;
            }
            uint16_t val;
            std::memcpy(&val, data + offset, 2);
            offset += 2;
            out = std::to_string(val);
            return true;
        }
        case GGUFValueType::UINT32:
        [[fallthrough]];
        case GGUFValueType::INT32:
        [[fallthrough]];
        case GGUFValueType::FLOAT32: {
            if (offset + 4 > mmap_size_) {
              return false;
            }
            uint32_t val;
            std::memcpy(&val, data + offset, 4);
            offset += 4;
            out = std::to_string(val);
            return true;
        }
        case GGUFValueType::UINT64:
        [[fallthrough]];
        case GGUFValueType::INT64:
        [[fallthrough]];
        case GGUFValueType::FLOAT64: {
            if (offset + 8 > mmap_size_) {
              return false;
            }
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
            if (offset + 12 > mmap_size_) {
              return false;
            }
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
        if (!readString(offset, key)) {
          return false;
        }
        
        // Read value type
        if (offset + 4 > mmap_size_) {
          return false;
        }
        uint32_t value_type_raw;
        std::memcpy(&value_type_raw, data + offset, 4);
        offset += 4;
        
        GGUFValueType value_type = static_cast<GGUFValueType>(value_type_raw);
        
        // Read value
        std::string value;
        if (!readMetadataValue(offset, value_type, value)) {
          return false;
        }
        
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
        if (!readString(offset, key)) {
          return false;
        }
        
        if (offset + 4 > mmap_size_) {
          return false;
        }
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
        if (!readString(offset, tensor.name)) {
          return false;
        }
        
        // Read n_dims
        if (offset + 4 > mmap_size_) {
          return false;
        }
        uint32_t n_dims;
        std::memcpy(&n_dims, data + offset, 4);
        offset += 4;
        
        // Read dimensions
        if (offset + n_dims * 8 > mmap_size_) {
          return false;
        }
        tensor.shape.resize(n_dims);
        for (uint32_t j = 0; j < n_dims; ++j) {
            uint64_t dim;
            std::memcpy(&dim, data + offset, 8);
            offset += 8;
            tensor.shape[j] = static_cast<int64_t>(dim);
        }
        
        // Read tensor type
        if (offset + 4 > mmap_size_) {
          return false;
        }
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
        if (offset + 8 > mmap_size_) {
          return false;
        }
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
        
        // BATCH 1.3: Validate tensor shape and bounds before storing
        try {
            validateTensorShape(tensor, "GGUFLoader::parseTensorInfo[" + tensor.name + "]");
            validateTensorOffset(tensor.offset, tensor.size, mmap_size_, 
                               "GGUFLoader::parseTensorInfo[" + tensor.name + "]");
        } catch (const std::exception& e) {
            last_error_ = e.what();
            spdlog::error("GGUFLoader: {}", last_error_);
            return false;
        }
        
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
    if (db_ == nullptr) {
        throw std::runtime_error("RocksDBWrapper not set. Use setDatabase() or constructor with db parameter.");
    }
    
    if (filepath_.empty() || mmap_base_ == nullptr || mmap_size_ == 0) {
        throw std::runtime_error("No GGUF file parsed. Call parseFile() first.");
    }
    
    if (model_name.empty()) {
        throw std::invalid_argument("Model name cannot be empty");
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
        if (!first) {
          metadata_json << ",";
        }
        metadata_json << "\"" << escapeJson(key) << "\":\"" << escapeJson(value) << "\"";
        first = false;
    }
    metadata_json << "},\"tensors\":[";
    
    first = true;
    for (const auto& tensor : metadata_.tensors) {
        if (!first) {
          metadata_json << ",";
        }
        metadata_json << "{"
                     << "\"name\":\"" << escapeJson(tensor.name) << "\","
                     << "\"dtype\":\"" << escapeJson(tensor.type_string()) << "\","
                     << "\"size\":" << tensor.size << ","
                     << "\"offset\":" << tensor.offset << ","
                     << "\"shape\":[";
        for (size_t i = 0; i < tensor.shape.size(); ++i) {
            if (i > 0) {
              metadata_json << ",";
            }
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
    if (tensor_name.empty() || mmap_base_ == nullptr || mmap_size_ == 0) {
        return nullptr;
    }
    
    // Find tensor
    for (const auto& tensor : metadata_.tensors) {
        if (tensor.name == tensor_name) {
            // Validate bounds before returning pointer
            if (metadata_.data_offset > mmap_size_ ||
                tensor.offset > mmap_size_ - metadata_.data_offset ||
                tensor.size > mmap_size_ - metadata_.data_offset - tensor.offset) {
                spdlog::warn("mmapTensor('{}') rejected: tensor offset/size out of bounds", tensor_name);
                return nullptr;
            }
            // Return pointer to tensor data in mmap region
            return static_cast<char*>(mmap_base_) + metadata_.data_offset + tensor.offset;
        }
    }
    return nullptr;
}

void GGUFLoader::unmapTensor(void* /*ptr*/) noexcept {
    // No-op for now since we keep entire file mapped
    // Individual tensor unmapping not needed with full file mmap
}

std::vector<uint8_t> GGUFLoader::getTensorData(const std::string& tensor_name) {
    if (tensor_name.empty()) {
        return {};
    }
    
    const TensorMetadata* tensor = nullptr;
    for (const auto& candidate : metadata_.tensors) {
        if (candidate.name == tensor_name) {
            tensor = &candidate;
            break;
        }
    }

    if (tensor == nullptr || mmap_base_ == nullptr || mmap_size_ == 0) {
        return {};
    }

    // IVB-04: Re-validate bounds immediately before copying raw bytes.
    // This protects against malformed metadata that may have slipped through
    // earlier parsing/validation phases.
    if (metadata_.data_offset > mmap_size_ ||
        tensor->offset > mmap_size_ - metadata_.data_offset) {
        spdlog::error("getTensorData('{}') rejected: invalid tensor offset (data_offset={}, tensor_offset={}, mmap_size={})",
                      tensor_name, metadata_.data_offset, tensor->offset, mmap_size_);
        return {};
    }
    const size_t tensor_start = metadata_.data_offset + tensor->offset;
    if (tensor->size > mmap_size_ - tensor_start) {
        spdlog::error("getTensorData('{}') rejected: tensor size {} exceeds mmap bounds (start={}, mmap_size={})",
                      tensor_name, tensor->size, tensor_start, mmap_size_);
        return {};
    }
    if (tensor->size > metadata_.total_size ||
        tensor_start > metadata_.total_size - tensor->size) {
        spdlog::error("getTensorData('{}') rejected: tensor range exceeds parsed file bounds (start={}, size={}, total_size={})",
                      tensor_name, tensor_start, tensor->size, metadata_.total_size);
        return {};
    }

    try {
        std::vector<uint8_t> data;
        data.reserve(tensor->size);
        data.resize(tensor->size);
        
        if (!data.empty()) {
            const auto* src = static_cast<const uint8_t*>(mmap_base_) + tensor_start;
            std::memcpy(data.data(), src, tensor->size);
        }
        return data;
    } catch (const std::exception& ex) {
        spdlog::error("getTensorData('{}') failed while copying tensor bytes: {}", tensor_name, ex.what());
        return {};
    }
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
    if (dtype == "float32") {
      return 4;
    }
    if (dtype == "float16") {
      return 2;
    }
    if (dtype == "int8") {
      return 1;
    }
    if (dtype == "int32") {
      return 4;
    }
    return 0;
}

} // namespace llm
} // namespace themis
