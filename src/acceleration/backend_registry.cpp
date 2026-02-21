/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            backend_registry.cpp                               ║
  Version:         0.0.22                                             ║
  Last Modified:   2026-02-21 19:29:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     285                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bf5228e16  2026-02-21  feat(acceleration): add CapabilityRequirements, satisfies... ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "acceleration/compute_backend.h"
#include "acceleration/plugin_loader.h"
#include "acceleration/cpu_backend.h"
#include "utils/logger.h"
#include <algorithm>
#include <mutex>
#include <iostream>

namespace themis {
namespace acceleration {

// The canonical fallback chain: highest priority first, CPU always last.
// All getBestXBackend() methods traverse this list in order.
static const std::vector<BackendType> kFallbackOrder = {
    BackendType::CUDA,
    BackendType::HIP,
    BackendType::ZLUDA,
    BackendType::VULKAN,
    BackendType::DIRECTX,
    BackendType::ROCM,
    BackendType::ONEAPI,
    BackendType::METAL,
    BackendType::OPENCL,
    BackendType::OPENGL,
    BackendType::WEBGPU,
    BackendType::CPU,
};

// Singleton instance
BackendRegistry::BackendRegistry() : pluginLoader_(std::make_unique<PluginLoader>()) {
    // Always register CPU backends (fallback)
    registerBackend(std::make_unique<CPUVectorBackend>());
    registerBackend(std::make_unique<CPUGraphBackend>());
    registerBackend(std::make_unique<CPUGeoBackend>());
}

BackendRegistry::~BackendRegistry() {
    shutdownAll();
}

BackendRegistry& BackendRegistry::instance() {
    static BackendRegistry instance;
    return instance;
}

void BackendRegistry::registerBackend(std::unique_ptr<IComputeBackend> backend) {
    if (backend && backend->isAvailable()) {
        std::cout << "Registered backend: " << backend->name() 
                  << " (Type: " << static_cast<int>(backend->type()) << ")" << std::endl;
        backends_.push_back(std::move(backend));
    }
}

size_t BackendRegistry::loadPlugins(const std::string& pluginDirectory) {
    std::cout << "Loading acceleration plugins from: " << pluginDirectory << std::endl;
    
    size_t count = pluginLoader_->loadPluginsFromDirectory(pluginDirectory);
    
    // Register backends from loaded plugins
    for (auto* plugin : pluginLoader_->getLoadedPlugins()) {
        // Try to create each type of backend
        if (auto vectorBackend = plugin->createVectorBackend()) {
            registerBackend(std::move(vectorBackend));
        }
        
        if (auto graphBackend = plugin->createGraphBackend()) {
            registerBackend(std::move(graphBackend));
        }
        
        if (auto geoBackend = plugin->createGeoBackend()) {
            registerBackend(std::move(geoBackend));
        }
    }
    
    return count;
}

bool BackendRegistry::loadPlugin(const std::string& pluginPath) {
    std::cout << "Loading acceleration plugin: " << pluginPath << std::endl;
    
    if (!pluginLoader_->loadPlugin(pluginPath)) {
        return false;
    }
    
    // Get the newly loaded plugin
    auto plugins = pluginLoader_->getLoadedPlugins();
    if (!plugins.empty()) {
        auto* plugin = plugins.back();
        
        // Null-safety: Validate plugin pointer before use
        if (!plugin) {
            THEMIS_ERROR("Plugin pointer is null after loading");
            return false;
        }
        
        // Register backends
        if (auto vectorBackend = plugin->createVectorBackend()) {
            registerBackend(std::move(vectorBackend));
        }
        
        if (auto graphBackend = plugin->createGraphBackend()) {
            registerBackend(std::move(graphBackend));
        }
        
        if (auto geoBackend = plugin->createGeoBackend()) {
            registerBackend(std::move(geoBackend));
        }
        
        return true;
    }
    
    return false;
}

IComputeBackend* BackendRegistry::getBackend(BackendType type) const {
    for (const auto& backend : backends_) {
        if (backend->type() == type) {
            return backend.get();
        }
    }
    return nullptr;
}

// static
const std::vector<BackendType>& BackendRegistry::getFallbackOrder() noexcept {
    return kFallbackOrder;
}

// Internal helper: traverse kFallbackOrder and return the first backend that
// satisfies reqs and can be downcast to T*. T must be IComputeBackend or a
// subtype (IVectorBackend, IGraphBackend, IGeoBackend).
template <typename T>
static T* selectTyped(const std::vector<std::unique_ptr<IComputeBackend>>& backends,
                      const BackendRegistry::CapabilityRequirements& reqs) {
    for (auto type : kFallbackOrder) {
        for (const auto& backend : backends) {
            if (backend->type() == type && BackendRegistry::satisfies(backend->getCapabilities(), reqs)) {
                T* typed = dynamic_cast<T*>(backend.get());
                if (typed) return typed;
            }
        }
    }
    return nullptr;
}

IComputeBackend* BackendRegistry::selectBackendFor(const CapabilityRequirements& reqs) const {
    return selectTyped<IComputeBackend>(backends_, reqs);
}

IVectorBackend* BackendRegistry::selectVectorBackendFor(const CapabilityRequirements& reqs) const {
    return selectTyped<IVectorBackend>(backends_, reqs);
}

IGraphBackend* BackendRegistry::selectGraphBackendFor(const CapabilityRequirements& reqs) const {
    return selectTyped<IGraphBackend>(backends_, reqs);
}

IGeoBackend* BackendRegistry::selectGeoBackendFor(const CapabilityRequirements& reqs) const {
    return selectTyped<IGeoBackend>(backends_, reqs);
}

IVectorBackend* BackendRegistry::getBestVectorBackend() const {
    for (auto type : kFallbackOrder) {
        for (const auto& backend : backends_) {
            if (backend->type() == type && backend->getCapabilities().supportsVectorOps) {
                auto* vectorBackend = dynamic_cast<IVectorBackend*>(backend.get());
                if (vectorBackend) {
                    return vectorBackend;
                }
            }
        }
    }
    return nullptr;
}

IGraphBackend* BackendRegistry::getBestGraphBackend() const {
    for (auto type : kFallbackOrder) {
        for (const auto& backend : backends_) {
            if (backend->type() == type && backend->getCapabilities().supportsGraphOps) {
                auto* graphBackend = dynamic_cast<IGraphBackend*>(backend.get());
                if (graphBackend) {
                    return graphBackend;
                }
            }
        }
    }
    return nullptr;
}

IGeoBackend* BackendRegistry::getBestGeoBackend() const {
    for (auto type : kFallbackOrder) {
        for (const auto& backend : backends_) {
            if (backend->type() == type && backend->getCapabilities().supportsGeoOps) {
                auto* geoBackend = dynamic_cast<IGeoBackend*>(backend.get());
                if (geoBackend) {
                    return geoBackend;
                }
            }
        }
    }
    return nullptr;
}

void BackendRegistry::autoDetect() {
    std::cout << "Auto-detecting acceleration backends..." << std::endl;
    
    // Try to load plugins from standard locations
    std::vector<std::string> pluginPaths = {
        "./plugins",                    // Current directory
        "./lib/themis/plugins",         // Relative to binary
        "/usr/local/lib/themis/plugins", // System-wide (Linux)
        "/opt/themis/plugins",          // Alternative system location
#ifdef _WIN32
        "C:/Program Files/ThemisDB/plugins",
#endif
    };
    
    for (const auto& path : pluginPaths) {
        loadPlugins(path);
    }
    
    std::cout << "Total backends available: " << backends_.size() << std::endl;
    
    // Print available backends
    for (const auto& backend : backends_) {
        auto caps = backend->getCapabilities();
        std::cout << "  - " << backend->name() 
                  << " (Vector:" << (caps.supportsVectorOps ? "Yes" : "No")
                  << " Graph:" << (caps.supportsGraphOps ? "Yes" : "No")
                  << " Geo:" << (caps.supportsGeoOps ? "Yes" : "No") << ")" << std::endl;
    }
}

std::vector<BackendType> BackendRegistry::getAvailableBackends() const {
    std::vector<BackendType> types;
    types.reserve(backends_.size());
    
    for (const auto& backend : backends_) {
        types.push_back(backend->type());
    }
    
    return types;
}

void BackendRegistry::shutdownAll() {
    std::cout << "Shutting down all acceleration backends..." << std::endl;
    
    for (auto& backend : backends_) {
        backend->shutdown();
    }
    backends_.clear();
    
    if (pluginLoader_) {
        pluginLoader_->unloadAllPlugins();
    }
}

} // namespace acceleration
} // namespace themis
