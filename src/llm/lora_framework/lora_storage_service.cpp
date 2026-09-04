/**
 * @file lora_storage_service.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=5, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/lora_storage_service.h"
#include <stdexcept>
#include <spdlog/spdlog.h>
#include <fstream>
#include <filesystem>
#include <algorithm>

namespace themis {
namespace llm {
namespace lora {

namespace fs = std::filesystem;

/**
 * @brief Implementation class for LoRAStorageService
 */
class LoRAStorageService::Impl {
public:
    explicit Impl(const Config& config) : config_(config) {
        // Create storage directory if it doesn't exist
        if (config_.backend == Backend::FileSystem) {
            if (!fs::exists(config_.filesystem_path)) {
                fs::create_directories(config_.filesystem_path);
                spdlog::info("Created LoRA storage directory: {}", config_.filesystem_path);
            }
        }
        
        spdlog::info("LoRAStorageService initialized:");
        spdlog::info("  Backend: {}", backendToString(config_.backend));
        spdlog::info("  Path: {}", config_.filesystem_path);
        spdlog::info("  Versioning: {}", config_.enable_versioning);
        spdlog::info("  Max versions: {}", config_.max_versions);
    }
    
    bool saveAdapter(
        const std::string& adapter_id,
        const AdapterWeights& weights,
        const AdapterMetadata& metadata
    ) {
        try {
            if (config_.backend == Backend::FileSystem) {
                return saveToFilesystem(adapter_id, weights, metadata);
            }
            // ThemisDB and S3 backends are only available in the full-build
            // configuration (lora_storage_service_themisdb.cpp, linked via
            // cmake/CMakeLists.txt). This file provides the FileSystem-only
            // fallback used by the modular build (cmake/ModularBuild.cmake).
            spdlog::error("Backend '{}' not supported in this build configuration; use FileSystem backend or link against lora_storage_service_themisdb.cpp",
                          backendToString(config_.backend));
            return false;
        } catch (const std::exception& e) {
            spdlog::error("Failed to save adapter {}: {}", adapter_id, e.what());
            return false;
        }
    }
    
    std::optional<AdapterWeights> loadAdapter(const std::string& adapter_id) {
        try {
            if (config_.backend == Backend::FileSystem) {
                return loadFromFilesystem(adapter_id);
            }
            // ThemisDB and S3 backends are only available in the full-build
            // configuration (lora_storage_service_themisdb.cpp).
            spdlog::error("Backend '{}' not supported in this build configuration; use FileSystem backend or link against lora_storage_service_themisdb.cpp",
                          backendToString(config_.backend));
            return std::nullopt;
        } catch (const std::exception& e) {
            spdlog::error("Failed to load adapter {}: {}", adapter_id, e.what());
            return std::nullopt;
        }
    }
    
    std::optional<AdapterMetadata> loadMetadata(const std::string& adapter_id) {
        try {
            if (config_.backend == Backend::FileSystem) {
                return loadMetadataFromFilesystem(adapter_id);
            }
            // ThemisDB and S3 backends are only available in the full-build
            // configuration (lora_storage_service_themisdb.cpp).
            spdlog::error("Backend '{}' not supported in this build configuration; use FileSystem backend or link against lora_storage_service_themisdb.cpp",
                          backendToString(config_.backend));
            return std::nullopt;
        } catch (const std::exception& e) {
            spdlog::error("Failed to load metadata for {}: {}", adapter_id, e.what());
            return std::nullopt;
        }
    }
    
    bool deleteAdapter(const std::string& adapter_id) {
        try {
            if (config_.backend == Backend::FileSystem) {
                fs::path adapter_dir = fs::path(config_.filesystem_path) / adapter_id;
                if (fs::exists(adapter_dir)) {
                    fs::remove_all(adapter_dir);
                    spdlog::info("Deleted adapter: {}", adapter_id);
                    return true;
                }
                return false;
            }
            // ThemisDB and S3 backends are only available in the full-build
            // configuration (lora_storage_service_themisdb.cpp).
            spdlog::error("Backend '{}' not supported in this build configuration; use FileSystem backend or link against lora_storage_service_themisdb.cpp",
                          backendToString(config_.backend));
            return false;
        } catch (const std::exception& e) {
            spdlog::error("Failed to delete adapter {}: {}", adapter_id, e.what());
            return false;
        }
    }
    
    bool exists(const std::string& adapter_id) const {
        if (config_.backend == Backend::FileSystem) {
            fs::path adapter_dir = fs::path(config_.filesystem_path) / adapter_id;
            return fs::exists(adapter_dir / "weights.bin");
        }
        return false;
    }
    
    std::vector<std::string> listAdapters() const {
        std::vector<std::string> adapters;
        
        if (config_.backend == Backend::FileSystem) {
            try {
                for (const auto& entry : fs::directory_iterator(config_.filesystem_path)) {
                    if (entry.is_directory()) {
                        adapters.push_back(entry.path().filename().string());
                    }
                }
            } catch (const std::exception& e) {
                spdlog::error("Failed to list adapters: {}", e.what());
            }
        }
        
        return adapters;
    }
    
    std::string createVersion(const std::string& adapter_id) {
        if (!config_.enable_versioning) {
            return "v1";
        }
        
        auto versions = listVersions(adapter_id);
        int max_version = 0;
        
        for (const auto& v : versions) {
            if (static_cast<int>(v.size()) > 1 && v[0] == 'v') {
                try {
                    int num = std::stoi(v.substr(1));
                    max_version = std::max(max_version, num);
                } catch (...) {}
            }
        }
        
        std::string new_version = "v" + std::to_string(max_version + 1);
        
        // Copy current to new version
        if (config_.backend == Backend::FileSystem) {
            fs::path adapter_dir = fs::path(config_.filesystem_path) / adapter_id;
            fs::path current = adapter_dir / "weights.bin";
            fs::path versioned = adapter_dir / ("weights_" + new_version + ".bin");
            
            if (fs::exists(current)) {
                fs::copy_file(current, versioned, fs::copy_options::overwrite_existing);
                spdlog::info("Created version {} for adapter {}", new_version, adapter_id);
            }
        }
        
        return new_version;
    }
    
    bool rollbackToVersion(const std::string& adapter_id, const std::string& version) {
        if (config_.backend == Backend::FileSystem) {
            fs::path adapter_dir = fs::path(config_.filesystem_path) / adapter_id;
            fs::path versioned = adapter_dir / ("weights_" + version + ".bin");
            fs::path current = adapter_dir / "weights.bin";
            
            if (fs::exists(versioned)) {
                fs::copy_file(versioned, current, fs::copy_options::overwrite_existing);
                spdlog::info("Rolled back adapter {} to version {}", adapter_id, version);
                return true;
            }
            
            spdlog::error("Version {} not found for adapter {}", version, adapter_id);
            return false;
        }
        
        return false;
    }
    
    std::vector<std::string> listVersions(const std::string& adapter_id) const {
        std::vector<std::string> versions;
        
        if (config_.backend == Backend::FileSystem) {
            fs::path adapter_dir = fs::path(config_.filesystem_path) / adapter_id;
            
            if (!fs::exists(adapter_dir)) {
                return versions;
            }
            
            try {
                for (const auto& entry : fs::directory_iterator(adapter_dir)) {
                    std::string filename = entry.path().filename().string();
                    if (filename.starts_with("weights_v") && filename.ends_with(".bin")) {
                        // Extract version from filename
                        size_t start = 8; // Length of "weights_"
                        size_t end = filename.find(".bin");
                        std::string version = filename.substr(start, end - start);
                        versions.push_back(version);
                    }
                }
            } catch (const std::exception& e) {
                spdlog::error("Failed to list versions: {}", e.what());
            }
        }
        
        std::sort(versions.begin(), versions.end());
        return versions;
    }
    
    bool updateMetadata(const std::string& adapter_id, const AdapterMetadata& metadata) {
        if (config_.backend == Backend::FileSystem) {
            fs::path adapter_dir = fs::path(config_.filesystem_path) / adapter_id;
            fs::path metadata_path = adapter_dir / "metadata.json";
            
            try {
                json j = metadata.toJSON();
                std::ofstream file(metadata_path);
                file << j.dump(2);
                spdlog::info("Updated metadata for adapter: {}", adapter_id);
                return true;
            } catch (const std::exception& e) {
                spdlog::error("Failed to update metadata: {}", e.what());
                return false;
            }
        }
        
        return false;
    }
    
    json getStats() const {
        json stats;
        stats["backend"] = backendToString(config_.backend);
        stats["versioning_enabled"] = config_.enable_versioning;
        stats["max_versions"] = config_.max_versions;
        stats["total_adapters"] = listAdapters().size();
        
        return stats;
    }

private:
    Config config_;
    
    static std::string backendToString(Backend backend) {
        switch (backend) {
            case Backend::ThemisDB: return "ThemisDB";
            case Backend::FileSystem: return "FileSystem";
            case Backend::S3: return "S3";
            default: return "Unknown";
        }
    }
    
    bool saveToFilesystem(
        const std::string& adapter_id,
        const AdapterWeights& weights,
        const AdapterMetadata& metadata
    ) {
        fs::path adapter_dir = fs::path(config_.filesystem_path) / adapter_id;
        fs::create_directories(adapter_dir);
        
        // Save weights
        fs::path weights_path = adapter_dir / "weights.bin";
        std::ofstream weights_file(weights_path, std::ios::binary);
        if (!weights_file) {
            spdlog::error("Failed to create weights file: {}", weights_path.string());
            return false;
        }
        weights_file.write(reinterpret_cast<const char*>(weights.data.data()), weights.data.size());
        weights_file.close();
        
        // Save metadata
        fs::path metadata_path = adapter_dir / "metadata.json";
        std::ofstream metadata_file(metadata_path);
        if (!metadata_file) {
            spdlog::error("Failed to create metadata file: {}", metadata_path.string());
            return false;
        }
        json j = metadata.toJSON();
        metadata_file << j.dump(2);
        metadata_file.close();
        
        spdlog::info("Saved adapter {} to filesystem ({} bytes)", 
                     adapter_id, weights.data.size());
        
        // Create version if enabled
        if (config_.enable_versioning) {
            createVersion(adapter_id);
        }
        
        return true;
    }
    
    std::optional<AdapterWeights> loadFromFilesystem(const std::string& adapter_id) {
        fs::path adapter_dir = fs::path(config_.filesystem_path) / adapter_id;
        fs::path weights_path = adapter_dir / "weights.bin";
        
        if (!fs::exists(weights_path)) {
            spdlog::warn("Weights file not found: {}", weights_path.string());
            return std::nullopt;
        }
        
        AdapterWeights weights;
        
        // Load weights
        std::ifstream weights_file(weights_path, std::ios::binary | std::ios::ate);
        if (!weights_file) {
            spdlog::error("Failed to open weights file: {}", weights_path.string());
            return std::nullopt;
        }
        
        size_t file_size = weights_file.tellg();
        weights_file.seekg(0, std::ios::beg);
        
        weights.data.resize(file_size);
        weights_file.read(reinterpret_cast<char*>(weights.data.data()), file_size);
        weights.size_bytes = file_size;
        weights_file.close();
        
        spdlog::info("Loaded adapter {} from filesystem ({} bytes)", 
                     adapter_id, file_size);
        
        return weights;
    }
    
    std::optional<AdapterMetadata> loadMetadataFromFilesystem(const std::string& adapter_id) {
        fs::path adapter_dir = fs::path(config_.filesystem_path) / adapter_id;
        fs::path metadata_path = adapter_dir / "metadata.json";
        
        if (!fs::exists(metadata_path)) {
            spdlog::warn("Metadata file not found: {}", metadata_path.string());
            return std::nullopt;
        }
        
        std::ifstream metadata_file(metadata_path);
        if (!metadata_file) {
            spdlog::error("Failed to open metadata file: {}", metadata_path.string());
            return std::nullopt;
        }
        
        json j;
        metadata_file >> j;
        
        return AdapterMetadata::fromJSON(j);
    }
};

// LoRAStorageService public interface

LoRAStorageService::LoRAStorageService(const Config& config)
    : impl_(std::make_unique<Impl>(config)) {}

LoRAStorageService::~LoRAStorageService() = default;

bool LoRAStorageService::saveAdapter(
    const std::string& adapter_id,
    const AdapterWeights& weights,
    const AdapterMetadata& metadata
) {
    return impl_->saveAdapter(adapter_id, weights, metadata);
}

std::optional<AdapterWeights> LoRAStorageService::loadAdapter(const std::string& adapter_id) {
    return impl_->loadAdapter(adapter_id);
}

std::optional<AdapterMetadata> LoRAStorageService::loadMetadata(const std::string& adapter_id) {
    return impl_->loadMetadata(adapter_id);
}

bool LoRAStorageService::deleteAdapter(const std::string& adapter_id) {
    return impl_->deleteAdapter(adapter_id);
}

bool LoRAStorageService::exists(const std::string& adapter_id) const {
    return impl_->exists(adapter_id);
}

std::vector<std::string> LoRAStorageService::listAdapters() const {
    return impl_->listAdapters();
}

std::string LoRAStorageService::createVersion(const std::string& adapter_id) {
    return impl_->createVersion(adapter_id);
}

bool LoRAStorageService::rollbackToVersion(const std::string& adapter_id, const std::string& version) {
    return impl_->rollbackToVersion(adapter_id, version);
}

std::vector<std::string> LoRAStorageService::listVersions(const std::string& adapter_id) const {
    return impl_->listVersions(adapter_id);
}

bool LoRAStorageService::updateMetadata(const std::string& adapter_id, const AdapterMetadata& metadata) {
    return impl_->updateMetadata(adapter_id, metadata);
}

json LoRAStorageService::getStats() const {
    return impl_->getStats();
}

} // namespace lora
} // namespace llm
} // namespace themis

