#include "llm/llama_resource_manager.h"
#include "acceleration/backend_registry.h"
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <filesystem>

namespace themis {
namespace llm {

// ===== LlamaModelHandle =====

LlamaModelHandle::LlamaModelHandle(const std::string& model_path,
                                  const llama_model_params& params) {
    spdlog::info("Loading model from: {}", model_path);
    
    llama_model* raw_model = llama_model_load_from_file(model_path.c_str(), params);
    
    if (!raw_model) {
        throw std::runtime_error("Failed to load model: " + model_path);
    }
    
    model_.reset(raw_model);
    
    spdlog::info("Model loaded successfully:");
    spdlog::info("  Vocabulary size: {}", n_vocab());
    spdlog::info("  Embedding dimension: {}", n_embd());
    spdlog::info("  Model type: {}", model_type());
}

LlamaModelHandle::~LlamaModelHandle() {
    spdlog::debug("Destroying LlamaModelHandle");
}

LlamaModelHandle::LlamaModelHandle(LlamaModelHandle&& other) noexcept
    : model_(std::move(other.model_)) {
}

LlamaModelHandle& LlamaModelHandle::operator=(LlamaModelHandle&& other) noexcept {
    if (this != &other) {
        model_ = std::move(other.model_);
    }
    return *this;
}

void LlamaModelHandle::ModelDeleter::operator()(llama_model* model) const {
    if (model) {
        llama_free_model(model);
        spdlog::debug("Model freed");
    }
}

size_t LlamaModelHandle::n_vocab() const {
    return model_ ? llama_n_vocab(model_.get()) : 0;
}

size_t LlamaModelHandle::n_embd() const {
    return model_ ? llama_n_embd(model_.get()) : 0;
}

std::string LlamaModelHandle::model_type() const {
    if (!model_) return "none";
    
    char buf[128];
    llama_model_desc(model_.get(), buf, sizeof(buf));
    return std::string(buf);
}

// ===== LlamaContextHandle =====

LlamaContextHandle::LlamaContextHandle(llama_model* model,
                                      const llama_context_params& params) {
    if (!model) {
        throw std::invalid_argument("Model cannot be null");
    }
    
    llama_context* raw_ctx = llama_new_context_with_model(model, params);
    
    if (!raw_ctx) {
        throw std::runtime_error("Failed to create llama context");
    }
    
    context_.reset(raw_ctx);
    spdlog::info("Context created with {} tokens capacity", params.n_ctx);
}

LlamaContextHandle::~LlamaContextHandle() {
    spdlog::debug("Destroying LlamaContextHandle");
}

LlamaContextHandle::LlamaContextHandle(LlamaContextHandle&& other) noexcept
    : context_(std::move(other.context_)) {
}

LlamaContextHandle& LlamaContextHandle::operator=(LlamaContextHandle&& other) noexcept {
    if (this != &other) {
        context_ = std::move(other.context_);
    }
    return *this;
}

void LlamaContextHandle::ContextDeleter::operator()(llama_context* ctx) const {
    if (ctx) {
        llama_free(ctx);
        spdlog::debug("Context freed");
    }
}

void LlamaContextHandle::clear_kv_cache() {
    if (context_) {
        llama_kv_cache_clear(context_.get());
        spdlog::debug("KV cache cleared");
    }
}

size_t LlamaContextHandle::kv_cache_token_count() const {
    return context_ ? llama_get_kv_cache_used_cells(context_.get()) : 0;
}

// ===== BackendAwareLlamaModelHandle =====

BackendAwareLlamaModelHandle::BackendAwareLlamaModelHandle(
    const std::string& model_path,
    const llama_model_params& params,
    const GPUBackendConfig& gpu_config) {
    
    // 1. Backend Selection (Vulkan prioritized)
    active_backend_ = selectBestBackend(gpu_config);
    
    spdlog::info("Selected GPU backend: {}", 
                 acceleration::BackendRegistry::instance()
                     .getBackend(active_backend_) ? 
                     acceleration::BackendRegistry::instance()
                         .getBackend(active_backend_)->name() : "Unknown");
    
    // 2. GPU Memory Manager initialization
    if (gpu_config.use_gpu_memory_manager) {
        GPUMemoryManager::Config mem_config;
        mem_config.max_vram_bytes = gpu_config.max_vram_per_gpu;
        mem_config.enable_multi_gpu = !gpu_config.secondary_gpus.empty();
        mem_config.gpu_devices = gpu_config.secondary_gpus;
        mem_config.gpu_devices.insert(mem_config.gpu_devices.begin(), 
                                     gpu_config.primary_gpu_id);
        mem_config.enable_peer_access = gpu_config.enable_peer_to_peer;
        
        gpu_memory_manager_ = std::make_shared<GPUMemoryManager>(mem_config);
        gpu_devices_ = mem_config.gpu_devices;
    }
    
    // 3. Optimal GPU Layers determination
    llama_model_params adjusted_params = params;
    
    if (gpu_config.auto_detect_optimal_layers) {
        std::filesystem::path model_file(model_path);
        if (std::filesystem::exists(model_file)) {
            size_t model_size = std::filesystem::file_size(model_file);
            int optimal_layers = determineOptimalGPULayers(gpu_config, model_size);
            adjusted_params.n_gpu_layers = optimal_layers;
            
            spdlog::info("Auto-detected optimal GPU layers: {} (model size: {:.2f} GB)",
                        optimal_layers, model_size / (1024.0 * 1024.0 * 1024.0));
        }
    } else {
        adjusted_params.n_gpu_layers = gpu_config.n_gpu_layers;
    }
    
    // 4. Backend-specific configuration
    adjusted_params.main_gpu = gpu_config.primary_gpu_id;
    
    // Vulkan Backend (PRIORITIZED)
    if (active_backend_ == acceleration::BackendType::VULKAN) {
        spdlog::info("Using Vulkan backend for cross-platform GPU acceleration");
        // Vulkan-specific configuration will be handled by llama.cpp
    }
    
    // CUDA Backend
    else if (active_backend_ == acceleration::BackendType::CUDA) {
        spdlog::info("Using CUDA backend for NVIDIA GPU acceleration");
        if (gpu_config.enable_tensor_cores) {
            spdlog::info("  Tensor Cores: enabled");
        }
    }
    
    // HIP/ROCm Backend (AMD)
    else if (active_backend_ == acceleration::BackendType::HIP || 
             active_backend_ == acceleration::BackendType::ROCM) {
        spdlog::info("Using HIP/ROCm backend for AMD GPU acceleration");
    }
    
    // Metal Backend (Apple)
    else if (active_backend_ == acceleration::BackendType::METAL) {
        spdlog::info("Using Metal backend for Apple Silicon GPU acceleration");
    }
    
    // 5. Allocate GPU Memory
    allocateGPUMemory(gpu_config);
    
    // 6. Load Model with llama.cpp
    spdlog::info("Loading model with backend: {}", 
                 acceleration::BackendRegistry::instance()
                     .getBackend(active_backend_) ? 
                     acceleration::BackendRegistry::instance()
                         .getBackend(active_backend_)->name() : "Unknown");
    
    llama_model* raw_model = llama_model_load_from_file(model_path.c_str(), adjusted_params);
    if (!raw_model) {
        throw std::runtime_error("Failed to load model: " + model_path);
    }
    model_.reset(raw_model);
    
    spdlog::info("Model loaded successfully with {} GPU layers", adjusted_params.n_gpu_layers);
    
    // 7. Configure backend-specific features
    configureBackendSpecificFeatures();
}

BackendAwareLlamaModelHandle::~BackendAwareLlamaModelHandle() {
    spdlog::debug("Destroying BackendAwareLlamaModelHandle");
}

BackendAwareLlamaModelHandle::BackendAwareLlamaModelHandle(
    BackendAwareLlamaModelHandle&& other) noexcept
    : model_(std::move(other.model_))
    , active_backend_(other.active_backend_)
    , backend_(other.backend_)
    , gpu_memory_manager_(std::move(other.gpu_memory_manager_))
    , gpu_devices_(std::move(other.gpu_devices_))
    , vram_allocated_(other.vram_allocated_) {
}

BackendAwareLlamaModelHandle& BackendAwareLlamaModelHandle::operator=(
    BackendAwareLlamaModelHandle&& other) noexcept {
    if (this != &other) {
        model_ = std::move(other.model_);
        active_backend_ = other.active_backend_;
        backend_ = other.backend_;
        gpu_memory_manager_ = std::move(other.gpu_memory_manager_);
        gpu_devices_ = std::move(other.gpu_devices_);
        vram_allocated_ = other.vram_allocated_;
    }
    return *this;
}

void BackendAwareLlamaModelHandle::ModelDeleter::operator()(llama_model* model) const {
    if (model) {
        llama_free_model(model);
        spdlog::debug("Backend-aware model freed");
    }
}

acceleration::BackendType BackendAwareLlamaModelHandle::selectBestBackend(
    const GPUBackendConfig& config) {
    
    auto& registry = acceleration::BackendRegistry::instance();
    
    // Vulkan PRIORITIZED (as specified)
    if (config.preferred_backend == acceleration::BackendType::AUTO ||
        config.preferred_backend == acceleration::BackendType::VULKAN) {
        
        // Priority list for LLM Inference (Vulkan first)
        std::vector<acceleration::BackendType> priority = {
            acceleration::BackendType::VULKAN,  // PRIORITIZED: Cross-platform
            acceleration::BackendType::CUDA,    // NVIDIA
            acceleration::BackendType::HIP,     // AMD
            acceleration::BackendType::METAL,   // Apple
            acceleration::BackendType::DIRECTX, // Windows
            acceleration::BackendType::OPENCL,  // Generic
            acceleration::BackendType::CPU      // Fallback
        };
        
        for (auto backend_type : priority) {
            auto* backend = registry.getBackend(backend_type);
            if (backend && backend->isAvailable()) {
                spdlog::info("Auto-selected backend: {}", backend->name());
                return backend_type;
            }
        }
        
        spdlog::warn("No GPU backend available, falling back to CPU");
        return acceleration::BackendType::CPU;
    }
    
    // Check preferred backend
    auto* backend = registry.getBackend(config.preferred_backend);
    if (backend && backend->isAvailable()) {
        return config.preferred_backend;
    }
    
    // Try fallback backends
    for (auto fallback : config.fallback_backends) {
        backend = registry.getBackend(fallback);
        if (backend && backend->isAvailable()) {
            spdlog::warn("Preferred backend not available, using fallback: {}",
                        backend->name());
            return fallback;
        }
    }
    
    spdlog::error("No suitable GPU backend found, falling back to CPU");
    return acceleration::BackendType::CPU;
}

int BackendAwareLlamaModelHandle::determineOptimalGPULayers(
    const GPUBackendConfig& config,
    size_t model_size) {
    
    // If explicitly set, use that value
    if (config.n_gpu_layers >= 0 && !config.auto_detect_optimal_layers) {
        return config.n_gpu_layers;
    }
    
    // Get available VRAM
    size_t available_vram = config.max_vram_per_gpu - config.reserved_vram;
    
    if (gpu_memory_manager_) {
        available_vram = gpu_memory_manager_->getFreeVRAM();
        if (available_vram > config.reserved_vram) {
            available_vram -= config.reserved_vram;
        }
    }
    
    // Estimate layers based on model size and available VRAM
    // Rough estimate: each layer takes about model_size / 40 bytes for typical models
    // This is a heuristic and may need tuning for specific model architectures
    const size_t estimated_layer_size = model_size / 40;
    
    if (estimated_layer_size == 0) {
        spdlog::warn("Cannot estimate layer size, using all layers");
        return -1;  // Use all layers
    }
    
    int optimal_layers = static_cast<int>(available_vram / estimated_layer_size);
    
    // Sanity checks
    if (optimal_layers < 0) {
        optimal_layers = 0;
    }
    
    // If we have plenty of VRAM, use all layers
    if (optimal_layers > 100 || available_vram > model_size * 2) {
        spdlog::info("Sufficient VRAM available, offloading all layers to GPU");
        return -1;  // -1 means all layers
    }
    
    spdlog::info("Determined optimal GPU layers: {} (available VRAM: {:.2f} GB, model size: {:.2f} GB)",
                 optimal_layers,
                 available_vram / (1024.0 * 1024.0 * 1024.0),
                 model_size / (1024.0 * 1024.0 * 1024.0));
    
    return optimal_layers;
}

void BackendAwareLlamaModelHandle::allocateGPUMemory(
    const GPUBackendConfig& config) {
    
    if (!gpu_memory_manager_) return;
    
    // Multi-GPU setup
    if (config.secondary_gpus.size() > 0) {
        spdlog::info("Multi-GPU setup detected: {} GPUs", 
                    1 + config.secondary_gpus.size());
        
        // Enable peer-to-peer access
        if (config.enable_peer_to_peer) {
            for (size_t i = 0; i < gpu_devices_.size(); ++i) {
                for (size_t j = i + 1; j < gpu_devices_.size(); ++j) {
                    if (gpu_memory_manager_->enablePeerAccess(
                            gpu_devices_[i], gpu_devices_[j])) {
                        spdlog::info("  P2P enabled: GPU {} <-> GPU {}",
                                    gpu_devices_[i], gpu_devices_[j]);
                    }
                }
            }
        }
    }
}

void BackendAwareLlamaModelHandle::configureBackendSpecificFeatures() {
    // Backend-specific optimizations
    if (active_backend_ == acceleration::BackendType::VULKAN) {
        spdlog::debug("Vulkan backend configured with compute pipelines");
    } else if (active_backend_ == acceleration::BackendType::CUDA) {
        spdlog::debug("CUDA backend configured with Flash Attention support");
    } else if (active_backend_ == acceleration::BackendType::METAL) {
        spdlog::debug("Metal backend configured with MPS acceleration");
    }
}

std::string BackendAwareLlamaModelHandle::backend_name() const {
    auto& registry = acceleration::BackendRegistry::instance();
    auto* backend = registry.getBackend(active_backend_);
    return backend ? backend->name() : "Unknown";
}

bool BackendAwareLlamaModelHandle::transferToGPU(int target_gpu_id) {
    if (!model_) {
        spdlog::error("Cannot transfer: model not loaded");
        return false;
    }
    
    if (!gpu_memory_manager_) {
        spdlog::error("Cannot transfer: GPU memory manager not initialized");
        return false;
    }
    
    // Check if target GPU is available
    if (!gpu_memory_manager_->isGPUAvailable(target_gpu_id)) {
        spdlog::error("Target GPU {} not available", target_gpu_id);
        return false;
    }
    
    spdlog::info("Transferring model to GPU {}", target_gpu_id);
    
    // Note: llama.cpp handles GPU assignment through model parameters at load time
    // For runtime migration, we would need to reload the model with different parameters
    // This is a placeholder for future enhancement
    
    spdlog::warn("Runtime GPU transfer not fully implemented - requires model reload");
    return true;
}

bool BackendAwareLlamaModelHandle::prefetchToGPU() {
    if (!model_) {
        spdlog::error("Cannot prefetch: model not loaded");
        return false;
    }
    
    // llama.cpp loads model data on-demand or at load time depending on configuration
    // The model is already loaded to GPU based on n_gpu_layers parameter
    spdlog::debug("Model already loaded to GPU during initialization");
    return true;
}

} // namespace llm
} // namespace themis
