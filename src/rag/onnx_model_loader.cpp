/**
 * @file onnx_model_loader.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/onnx_model_loader.h"
#include "utils/logger.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>
#include <curl/curl.h>

namespace themis::rag::judge {

// ═══════════════════════════════════════════════════════════
// ONNX Model Loader Implementation
// ═══════════════════════════════════════════════════════════

struct ONNXModelLoader::Impl {
    std::unordered_map<std::string, ONNXModelInfo> cache;
    std::mutex cache_mutex = {};
};

ONNXModelLoader::ONNXModelLoader(const ONNXModelLoaderConfig& config)
    : impl_(std::make_unique<Impl>())
    , config_(config) {
    
    // Create cache directory if needed
    if (config_.create_cache_dir && !config_.cache_dir.empty()) {
        std::filesystem::create_directories(config_.cache_dir);
    }
    
    // Initialize default models
    initializeDefaultModels();
}

ONNXModelLoader::~ONNXModelLoader() = default;

void ONNXModelLoader::initializeDefaultModels() {
    // Register common NLI models
    registerModel(NLIModelFactory::getDebertaV3LargeMNLI());
    registerModel(NLIModelFactory::getRobertaLargeMNLI());
    registerModel(NLIModelFactory::getBartLargeMNLI());
}

std::optional<ONNXModelInfo> ONNXModelLoader::loadModel(const std::string& model_path) {
    if (model_path.empty()) {
        THEMIS_ERROR("Empty model path provided");
        return std::nullopt;
    }
    
    if (!std::filesystem::exists(model_path)) {
        THEMIS_ERROR("Model file not found: {}", model_path);
        return std::nullopt;
    }
    
    ONNXModelInfo info;
    info.model_path = model_path;
    info.model_name = std::filesystem::path(model_path).stem().string();
    info.model_size_bytes = std::filesystem::file_size(model_path);
    
    // Compute checksum if verification enabled
    if (config_.verify_checksum) {
        info.checksum = computeChecksum(model_path);
    }
    
    // Cache the model info
    {
        std::lock_guard<std::mutex> lock(impl_->cache_mutex);
        impl_->cache[info.model_name] = info;
    }
    
    THEMIS_INFO("Loaded ONNX model: {} ({} bytes)", info.model_name, info.model_size_bytes);
    return info;
}

std::optional<ONNXModelInfo> ONNXModelLoader::loadOrDownloadModel(
    const std::string& model_name,
    const std::string& model_url,
    const std::string& expected_checksum) {
    
    // Check if already cached
    if (isModelCached(model_name)) {
        std::string cached_path = getCachedModelPath(model_name);
        
        // Verify checksum if provided
        if (!expected_checksum.empty() && config_.verify_checksum) {
            if (!validateModelChecksum(cached_path, expected_checksum)) {
                THEMIS_WARN("Cached model checksum mismatch: {}", model_name);
                // Fall through to re-download
            } else {
                return loadModel(cached_path);
            }
        } else {
            return loadModel(cached_path);
        }
    }
    
    // Download if auto_download enabled
    if (config_.auto_download && !model_url.empty()) {
        std::string dest_path = config_.cache_dir + "/" + model_name + ".onnx";
        
        THEMIS_INFO("Downloading model: {} from {}", model_name, model_url);
        
        if (downloadFile(model_url, dest_path, config_.download_timeout_sec)) {
            // Verify checksum if provided
            if (!expected_checksum.empty() && config_.verify_checksum) {
                if (!validateModelChecksum(dest_path, expected_checksum)) {
                    THEMIS_ERROR("Downloaded model checksum mismatch: {}", model_name);
                    std::filesystem::remove(dest_path);
                    return std::nullopt;
                }
            }
            
            return loadModel(dest_path);
        } else {
            THEMIS_ERROR("Failed to download model: {}", model_name);
            return std::nullopt;
        }
    }
    
    THEMIS_ERROR("Model not found and auto_download disabled: {}", model_name);
    return std::nullopt;
}

bool ONNXModelLoader::isModelCached(const std::string& model_name) const {
    std::lock_guard<std::mutex> lock(impl_->cache_mutex);
    return impl_->cache.find(model_name) != impl_->cache.end();
}

std::string ONNXModelLoader::getCachedModelPath(const std::string& model_name) const {
    std::lock_guard<std::mutex> lock(impl_->cache_mutex);
    auto it = impl_->cache.find(model_name);
    if (it != impl_->cache.end()) {
        return it->second.model_path;
    }
    
    // Try to find in cache directory
    std::string potential_path = config_.cache_dir + "/" + model_name + ".onnx";
    if (std::filesystem::exists(potential_path)) {
        return potential_path;
    }
    
    return "";
}

bool ONNXModelLoader::validateModelChecksum(const std::string& model_path, const std::string& expected_checksum) {
    if (expected_checksum.empty()) {
        return true;  // No checksum to validate
    }
    
    std::string actual_checksum = computeChecksum(model_path);
    return actual_checksum == expected_checksum;
}

std::vector<std::string> ONNXModelLoader::listCachedModels() const {
    std::vector<std::string> models;
    std::lock_guard<std::mutex> lock(impl_->cache_mutex);
    
    for (const auto& [name, info] : impl_->cache) {
        models.push_back(name);
    }
    
    return models;
}

size_t ONNXModelLoader::clearCache(const std::string& model_name) {
    size_t cleared = 0;
    std::lock_guard<std::mutex> lock(impl_->cache_mutex);
    
    if (model_name.empty()) {
        // Clear all
        cleared = impl_-> static_cast<int>(cache.size());
        impl_->cache.clear();
    } else {
        // Clear specific model
        auto it = impl_->cache.find(model_name);
        if (it != impl_->cache.end()) {
            impl_->cache.erase(it);
            cleared = 1;
        }
    }
    
    return cleared;
}

void ONNXModelLoader::registerModel(const ONNXModelInfo& info) {
    model_registry_[info.model_name] = info;
}

std::optional<ONNXModelInfo> ONNXModelLoader::getModelInfo(const std::string& model_name) const {
    auto it = model_registry_.find(model_name);
    if (it != model_registry_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::string ONNXModelLoader::computeChecksum(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    
    constexpr size_t BUFFER_SIZE = 8192;
    char buffer[BUFFER_SIZE];
    
    while (file.read(buffer, BUFFER_SIZE) || file.gcount() > 0) {
        SHA256_Update(&sha256, buffer, file.gcount());
    }
    
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &sha256);
    
    std::ostringstream oss = {};
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    
    return oss.str();
}

// CURL write callback
static size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    return fwrite(ptr, size, nmemb, stream);
}

bool ONNXModelLoader::downloadFile(const std::string& url, const std::string& dest_path, int timeout_sec) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        return false;
    }
    
    FILE* fp = fopen(dest_path.c_str(), "wb");
    if (!fp) {
        curl_easy_cleanup(curl);
        return false;
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout_sec);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
    
    CURLcode res = curl_easy_perform(curl);
    
    fclose(fp);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        std::filesystem::remove(dest_path);
        return false;
    }
    
    return true;
}

// ═══════════════════════════════════════════════════════════
// NLI Model Factory Implementation
// ═══════════════════════════════════════════════════════════

ONNXModelInfo NLIModelFactory::getDebertaV3LargeMNLI() {
    ONNXModelInfo info;
    info.model_name = "deberta-v3-large-mnli";
    info.model_url = "https://huggingface.co/microsoft/deberta-v3-large-mnli/resolve/main/model.onnx";
    info.tokenizer_path = ""; // Would be downloaded separately
    info.model_size_bytes = 1420000000; // ~1.42 GB
    info.checksum = ""; // Would be provided by model source
    info.input_shape = {1, 512};  // batch_size=1, seq_len=512
    info.output_names = {"logits"};
    return info;
}

ONNXModelInfo NLIModelFactory::getRobertaLargeMNLI() {
    ONNXModelInfo info;
    info.model_name = "roberta-large-mnli";
    info.model_url = "https://huggingface.co/roberta-large-mnli/resolve/main/model.onnx";
    info.model_size_bytes = 1420000000; // ~1.42 GB
    info.checksum = "";
    info.input_shape = {1, 512};
    info.output_names = {"logits"};
    return info;
}

ONNXModelInfo NLIModelFactory::getBartLargeMNLI() {
    ONNXModelInfo info;
    info.model_name = "bart-large-mnli";
    info.model_url = "https://huggingface.co/facebook/bart-large-mnli/resolve/main/model.onnx";
    info.model_size_bytes = 1630000000; // ~1.63 GB
    info.checksum = "";
    info.input_shape = {1, 1024};
    info.output_names = {"logits"};
    return info;
}

std::vector<ONNXModelInfo> NLIModelFactory::getAllSupportedModels() {
    return {
        getDebertaV3LargeMNLI(),
        getRobertaLargeMNLI(),
        getBartLargeMNLI()
    };
}

} // namespace themis::rag::judge

