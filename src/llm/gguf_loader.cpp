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

namespace themis {
namespace llm {

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
    // GGUF magic number: "GGUF"
    const char* data = static_cast<const char*>(mmap_base_);
    if (mmap_size_ < 8 || std::memcmp(data, "GGUF", 4) != 0) {
        return false;
    }
    
    // Version (uint32_t)
    uint32_t version = *reinterpret_cast<const uint32_t*>(data + 4);
    metadata_.version = std::to_string(version);
    
    return true;
}

bool GGUFLoader::parseMetadataKV() {
    // Simplified: Extract architecture from metadata
    // In real implementation, parse full KV metadata section
    metadata_.architecture = "llama";  // Stub
    metadata_.config["context_length"] = "4096";
    metadata_.config["embedding_length"] = "4096";
    metadata_.config["block_count"] = "32";
    
    return true;
}

bool GGUFLoader::parseTensorInfo() {
    // Simplified: Parse tensor information
    // In real implementation, iterate through tensor metadata section
    
    // Example tensors for Mistral-7B-like model
    TensorMetadata tensor;
    
    // Token embeddings
    tensor.name = "token_embd.weight";
    tensor.shape = {32000, 4096};
    tensor.dtype = "float16";
    tensor.offset = 1024;  // After header
    tensor.size = 32000 * 4096 * 2;  // float16 = 2 bytes
    metadata_.tensors.push_back(tensor);
    
    // Attention layers
    for (int i = 0; i < 32; i++) {
        tensor.name = "blk." + std::to_string(i) + ".attn_q.weight";
        tensor.shape = {4096, 4096};
        tensor.dtype = "float16";
        tensor.offset = tensor.offset + tensor.size;
        tensor.size = 4096 * 4096 * 2;
        metadata_.tensors.push_back(tensor);
        
        tensor.name = "blk." + std::to_string(i) + ".attn_k.weight";
        metadata_.tensors.push_back(tensor);
        
        tensor.name = "blk." + std::to_string(i) + ".attn_v.weight";
        metadata_.tensors.push_back(tensor);
        
        tensor.name = "blk." + std::to_string(i) + ".attn_output.weight";
        metadata_.tensors.push_back(tensor);
        
        // FFN layers
        tensor.name = "blk." + std::to_string(i) + ".ffn_gate.weight";
        tensor.shape = {4096, 14336};
        tensor.size = 4096 * 14336 * 2;
        metadata_.tensors.push_back(tensor);
        
        tensor.name = "blk." + std::to_string(i) + ".ffn_up.weight";
        metadata_.tensors.push_back(tensor);
        
        tensor.name = "blk." + std::to_string(i) + ".ffn_down.weight";
        tensor.shape = {14336, 4096};
        tensor.size = 14336 * 4096 * 2;
        metadata_.tensors.push_back(tensor);
    }
    
    // Output layer
    tensor.name = "output.weight";
    tensor.shape = {4096, 32000};
    tensor.dtype = "float16";
    tensor.size = 4096 * 32000 * 2;
    metadata_.tensors.push_back(tensor);
    
    return true;
}

std::string GGUFLoader::loadToThemisDB(const std::string& model_name) {
    if (!db_) {
        throw std::runtime_error("RocksDBWrapper not set. Use setDatabase() or constructor with db parameter.");
    }
    
    if (filepath_.empty() || mmap_base_ == nullptr) {
        throw std::runtime_error("No GGUF file parsed. Call parseFile() first.");
    }
    
    // Generate model URN
    std::string model_urn = "urn:themis:model:" + model_name + ":v1";
    std::string metadata_key = "llm:model:" + model_name + ":metadata";
    
    // 1. Store metadata
    std::ostringstream metadata_json;
    metadata_json << "{"
                  << "\"version\":\"" << metadata_.version << "\","
                  << "\"architecture\":\"" << metadata_.architecture << "\","
                  << "\"total_size\":" << metadata_.total_size << ","
                  << "\"num_tensors\":" << metadata_.tensors.size() << ","
                  << "\"urn\":\"" << model_urn << "\","
                  << "\"config\":{";
    
    bool first = true;
    for (const auto& [key, value] : metadata_.config) {
        if (!first) metadata_json << ",";
        metadata_json << "\"" << key << "\":\"" << value << "\"";
        first = false;
    }
    metadata_json << "},\"tensors\":[";
    
    first = true;
    for (const auto& tensor : metadata_.tensors) {
        if (!first) metadata_json << ",";
        metadata_json << "{"
                     << "\"name\":\"" << tensor.name << "\","
                     << "\"dtype\":\"" << tensor.dtype << "\","
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
    void* tensor_ptr = static_cast<char*>(mmap_base_) + tensor.offset;
    
    size_t remaining = tensor.size;
    size_t chunk_index = 0;
    size_t offset = 0;
    
    while (remaining > 0) {
        size_t current_chunk_size = std::min(remaining, chunk_size);
        
        // Create chunk key: llm:model:{model_name}:tensor:{tensor_name}:chunk:{index}
        std::ostringstream chunk_key;
        chunk_key << "llm:model:" << model_name 
                  << ":tensor:" << tensor.name 
                  << ":chunk:" << chunk_index;
        
        // Copy chunk data
        std::vector<uint8_t> chunk_data(current_chunk_size);
        std::memcpy(chunk_data.data(), 
                   static_cast<char*>(tensor_ptr) + offset, 
                   current_chunk_size);
        
        // Store chunk in RocksDB (will automatically use BlobDB for large chunks)
        if (!db_->put(chunk_key.str(), chunk_data)) {
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
            return static_cast<char*>(mmap_base_) + tensor.offset;
        }
    }
    return nullptr;
}

void GGUFLoader::unmapTensor(void* ptr) {
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

size_t GGUFLoader::getDtypeSize(const std::string& dtype) {
    if (dtype == "float32") return 4;
    if (dtype == "float16") return 2;
    if (dtype == "int8") return 1;
    if (dtype == "int32") return 4;
    return 0;
}

} // namespace llm
} // namespace themis
