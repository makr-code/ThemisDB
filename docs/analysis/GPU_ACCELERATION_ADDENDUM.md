# GPU/VRAM Acceleration Integration - Addendum zum Implementation Guide

**Erstellt**: 15. Januar 2026  
**Version**: 1.1 (Addendum)  
**Basis**: Bestehende GPU-Infrastruktur in `/src/acceleration` und `/include/acceleration`  
**Antwort auf Kommentar**: GPU-Acceleration (Vulkan, CUDA, etc.) einbeziehen

---

## Überblick: Vorhandene GPU-Infrastruktur

ThemisDB verfügt bereits über eine **umfassende GPU-Acceleration-Infrastruktur**, die in die LLM/LoRA-Integration einbezogen werden muss.

### Vorhandene Backend-Implementierungen

**Quelle**: `include/acceleration/compute_backend.h` (Zeilen 12-26)

```cpp
enum class BackendType {
    CPU,        // CPU-only (fallback)
    CUDA,       // NVIDIA CUDA ✅ VORHANDEN
    ZLUDA,      // AMD ZLUDA (CUDA compatibility) ✅ VORHANDEN
    HIP,        // AMD HIP ✅ VORHANDEN
    ROCM,       // AMD ROCm ✅ VORHANDEN
    DIRECTX,    // DirectX Compute Shaders ✅ VORHANDEN
    VULKAN,     // Vulkan Compute ✅ VORHANDEN
    OPENGL,     // OpenGL Compute Shaders ✅ VORHANDEN
    METAL,      // Apple Metal ✅ VORHANDEN
    ONEAPI,     // Intel OneAPI/SYCL ✅ VORHANDEN
    OPENCL,     // OpenCL ✅ VORHANDEN
    WEBGPU,     // WebGPU (future) ⚠️ GEPLANT
    AUTO        // Auto-detect best available ✅ VORHANDEN
};
```

### Vorhandene GPU-Komponenten

| Komponente | Datei | Status | Zweck |
|------------|-------|--------|-------|
| **GPUMemoryManager** | `include/llm/gpu_memory_manager.h` | ✅ Implementiert | Multi-GPU VRAM-Verwaltung |
| **BackendRegistry** | `include/acceleration/compute_backend.h` | ✅ Implementiert | Backend-Discovery & Management |
| **CUDABackend** | `include/acceleration/cuda_backend.h` | ✅ Implementiert | NVIDIA GPU-Beschleunigung |
| **VulkanBackend** | `src/acceleration/vulkan_backend_full.cpp` | ✅ Implementiert | Cross-Platform GPU (Vulkan) |
| **Kernel Fusion** | `include/llm/kernel_fusion_cuda.h` | ✅ Implementiert | CUDA Kernel-Optimierung |

---

## Integration: GPU-Acceleration in llama.cpp Wrapper

### Problem

Der aktuelle `LlamaWrapper` nutzt die GPU-Parameter (`n_gpu_layers`, `main_gpu`), aber die Integration mit dem `BackendRegistry` und `GPUMemoryManager` fehlt.

**Quelle**: `src/llm/llama_wrapper.cpp` (Zeilen 176-178)
```cpp
spdlog::info("  GPU layers: {}, Context: {}", 
             config_.n_gpu_layers, config_.n_ctx);
spdlog::info("  Flash Attention: {}", config_.use_flash_attn ? "enabled" : "disabled");
```

### Lösung: Backend-Aware Model Loading

#### Schritt 1: Backend Detection & Selection

**Erweiterte Datei**: `include/llm/llama_resource_manager.h`

```cpp
#pragma once
#include <llama.h>
#include <memory>
#include <string>
#include "acceleration/compute_backend.h"  // NEU: Backend-Integration
#include "llm/gpu_memory_manager.h"        // NEU: GPU Memory Management

namespace themis {
namespace llm {

/**
 * @brief GPU Backend Configuration für llama.cpp
 * 
 * Integriert ThemisDB Backend-Infrastruktur mit llama.cpp
 */
struct GPUBackendConfig {
    // Backend Selection
    acceleration::BackendType preferred_backend = acceleration::BackendType::AUTO;
    std::vector<acceleration::BackendType> fallback_backends;
    
    // GPU Device Selection
    int primary_gpu_id = 0;
    std::vector<int> secondary_gpus;  // Für Tensor Parallelism
    
    // Memory Management
    bool use_gpu_memory_manager = true;
    size_t max_vram_per_gpu = 24ULL * 1024 * 1024 * 1024;  // 24 GB
    size_t reserved_vram = 2ULL * 1024 * 1024 * 1024;      // 2 GB Reserve
    
    // Layer Offloading Strategy
    int n_gpu_layers = -1;  // -1 = all layers
    bool auto_detect_optimal_layers = true;  // Auto-tune basierend auf VRAM
    
    // Advanced GPU Features
    bool enable_flash_attention = true;
    bool enable_tensor_cores = true;
    bool enable_peer_to_peer = false;  // Multi-GPU P2P
    bool enable_unified_memory = false; // CUDA Unified Memory
};

/**
 * @brief Backend-Aware llama.cpp Model Handle
 * 
 * Erweitert LlamaModelHandle mit Backend-Integration
 */
class BackendAwareLlamaModelHandle {
public:
    explicit BackendAwareLlamaModelHandle(
        const std::string& model_path,
        const llama_model_params& params,
        const GPUBackendConfig& gpu_config
    );
    ~BackendAwareLlamaModelHandle();
    
    // Nicht kopierbar, nur movable
    BackendAwareLlamaModelHandle(const BackendAwareLlamaModelHandle&) = delete;
    BackendAwareLlamaModelHandle& operator=(const BackendAwareLlamaModelHandle&) = delete;
    BackendAwareLlamaModelHandle(BackendAwareLlamaModelHandle&& other) noexcept;
    BackendAwareLlamaModelHandle& operator=(BackendAwareLlamaModelHandle&& other) noexcept;
    
    llama_model* get() const noexcept { return model_.get(); }
    explicit operator bool() const noexcept { return model_ != nullptr; }
    
    // Backend Information
    acceleration::BackendType active_backend() const { return active_backend_; }
    std::string backend_name() const;
    
    // GPU Memory Information
    size_t vram_usage() const { return vram_allocated_; }
    std::vector<int> gpu_devices() const { return gpu_devices_; }
    
    // Memory Transfer (für Multi-GPU)
    bool transferToGPU(int target_gpu_id);
    bool prefetchToGPU();

private:
    struct ModelDeleter {
        void operator()(llama_model* model) const {
            if (model) llama_free_model(model);
        }
    };
    
    std::unique_ptr<llama_model, ModelDeleter> model_;
    
    // Backend Management
    acceleration::BackendType active_backend_;
    acceleration::IComputeBackend* backend_ = nullptr;
    
    // GPU Memory Management
    std::shared_ptr<GPUMemoryManager> gpu_memory_manager_;
    std::vector<int> gpu_devices_;
    size_t vram_allocated_ = 0;
    
    // Helper methods
    acceleration::BackendType selectBestBackend(const GPUBackendConfig& config);
    int determineOptimalGPULayers(const GPUBackendConfig& config, size_t model_size);
    void allocateGPUMemory(const GPUBackendConfig& config);
    void configureBackendSpecificFeatures();
};

} // namespace llm
} // namespace themis
```

#### Schritt 2: Backend Selection Logic

**Implementierung**: `src/llm/llama_resource_manager.cpp`

```cpp
#include "llm/llama_resource_manager.h"
#include "acceleration/backend_registry.h"
#include <spdlog/spdlog.h>

namespace themis {
namespace llm {

BackendAwareLlamaModelHandle::BackendAwareLlamaModelHandle(
    const std::string& model_path,
    const llama_model_params& params,
    const GPUBackendConfig& gpu_config) {
    
    // 1. Backend Selection
    active_backend_ = selectBestBackend(gpu_config);
    
    spdlog::info("Selected GPU backend: {}", 
                 acceleration::BackendRegistry::instance()
                     .getBackend(active_backend_)->name());
    
    // 2. GPU Memory Manager initialisieren
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
    
    // 3. Optimal GPU Layers ermitteln (falls Auto-Detect)
    llama_model_params adjusted_params = params;
    
    if (gpu_config.auto_detect_optimal_layers) {
        // Modell-Größe schätzen
        std::filesystem::path model_file(model_path);
        size_t model_size = std::filesystem::file_size(model_file);
        
        int optimal_layers = determineOptimalGPULayers(gpu_config, model_size);
        adjusted_params.n_gpu_layers = optimal_layers;
        
        spdlog::info("Auto-detected optimal GPU layers: {} (model size: {} GB)",
                    optimal_layers, model_size / (1024.0 * 1024.0 * 1024.0));
    } else {
        adjusted_params.n_gpu_layers = gpu_config.n_gpu_layers;
    }
    
    // 4. Backend-spezifische Configuration
    adjusted_params.main_gpu = gpu_config.primary_gpu_id;
    
    // Vulkan Backend
    if (active_backend_ == acceleration::BackendType::VULKAN) {
        spdlog::info("Using Vulkan backend for cross-platform GPU acceleration");
        // Vulkan-spezifische Flags (llama.cpp unterstützt Vulkan seit v0.12.0)
        // Note: llama.cpp detektiert Vulkan automatisch via compile flags
    }
    
    // CUDA Backend
    if (active_backend_ == acceleration::BackendType::CUDA) {
        spdlog::info("Using CUDA backend for NVIDIA GPU acceleration");
        
        // CUDA-spezifische Features
        if (gpu_config.enable_tensor_cores) {
            spdlog::info("  Tensor Cores: enabled (FP16/BF16 acceleration)");
        }
        if (gpu_config.enable_unified_memory) {
            spdlog::info("  CUDA Unified Memory: enabled");
            // Note: Requires CUDA-specific llama.cpp build
        }
    }
    
    // HIP/ROCm Backend (AMD)
    if (active_backend_ == acceleration::BackendType::HIP || 
        active_backend_ == acceleration::BackendType::ROCM) {
        spdlog::info("Using HIP/ROCm backend for AMD GPU acceleration");
    }
    
    // Metal Backend (Apple)
    if (active_backend_ == acceleration::BackendType::METAL) {
        spdlog::info("Using Metal backend for Apple Silicon GPU acceleration");
    }
    
    // 5. GPU Memory allokieren
    allocateGPUMemory(gpu_config);
    
    // 6. Model laden
    llama_model* raw_model = llama_model_load_from_file(
        model_path.c_str(),
        adjusted_params
    );
    
    if (!raw_model) {
        throw std::runtime_error("Failed to load model: " + model_path);
    }
    
    model_.reset(raw_model);
    
    // 7. Backend-spezifische Features konfigurieren
    configureBackendSpecificFeatures();
    
    // 8. VRAM-Usage loggen
    if (gpu_memory_manager_) {
        vram_allocated_ = gpu_memory_manager_->getGPUVRAM(gpu_config.primary_gpu_id);
        spdlog::info("Model loaded: {} GB VRAM allocated on GPU {}",
                    vram_allocated_ / (1024.0 * 1024.0 * 1024.0),
                    gpu_config.primary_gpu_id);
    }
}

acceleration::BackendType BackendAwareLlamaModelHandle::selectBestBackend(
    const GPUBackendConfig& config) {
    
    auto& registry = acceleration::BackendRegistry::instance();
    
    // AUTO: Automatische Backend-Auswahl
    if (config.preferred_backend == acceleration::BackendType::AUTO) {
        // Prioritätenliste für LLM Inferenz
        std::vector<acceleration::BackendType> priority = {
            acceleration::BackendType::CUDA,    // Beste Performance (NVIDIA)
            acceleration::BackendType::VULKAN,  // Cross-platform (AMD, NVIDIA, Intel)
            acceleration::BackendType::HIP,     // AMD ROCm
            acceleration::BackendType::METAL,   // Apple Silicon
            acceleration::BackendType::DIRECTX, // Windows Fallback
            acceleration::BackendType::OPENCL,  // Generic Fallback
            acceleration::BackendType::CPU      // Last Resort
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
    
    // Preferred Backend prüfen
    auto* backend = registry.getBackend(config.preferred_backend);
    if (backend && backend->isAvailable()) {
        return config.preferred_backend;
    }
    
    // Fallback Backends versuchen
    for (auto fallback : config.fallback_backends) {
        backend = registry.getBackend(fallback);
        if (backend && backend->isAvailable()) {
            spdlog::warn("Preferred backend not available, using fallback: {}",
                        backend->name());
            return fallback;
        }
    }
    
    // Worst case: CPU
    spdlog::error("No suitable GPU backend found, falling back to CPU");
    return acceleration::BackendType::CPU;
}

int BackendAwareLlamaModelHandle::determineOptimalGPULayers(
    const GPUBackendConfig& config,
    size_t model_size) {
    
    if (!gpu_memory_manager_) {
        return config.n_gpu_layers;  // Fallback
    }
    
    // Verfügbares VRAM ermitteln
    size_t free_vram = gpu_memory_manager_->getFreeGPUVRAM(config.primary_gpu_id);
    size_t usable_vram = free_vram - config.reserved_vram;
    
    spdlog::info("Available VRAM: {} GB, Usable: {} GB",
                free_vram / (1024.0 * 1024.0 * 1024.0),
                usable_vram / (1024.0 * 1024.0 * 1024.0));
    
    // Heuristik: 1 Layer ≈ model_size / 32 (für typische Transformer-Modelle)
    // Beispiel: 7B Modell mit 32 Layern ≈ ~250 MB pro Layer (bei Q4_K_M Quantisierung)
    
    // Modell-Größe pro Layer schätzen
    size_t estimated_total_layers = 32;  // Default (kann aus Modell-Metadaten gelesen werden)
    size_t bytes_per_layer = model_size / estimated_total_layers;
    
    // Maximale Layer berechnen, die in VRAM passen
    int max_layers = static_cast<int>(usable_vram / bytes_per_layer);
    
    // Sicherheits-Margin (80% Auslastung)
    max_layers = static_cast<int>(max_layers * 0.8);
    
    // Begrenzen auf tatsächliche Layer-Anzahl
    max_layers = std::min(max_layers, static_cast<int>(estimated_total_layers));
    
    if (max_layers <= 0) {
        spdlog::warn("Insufficient VRAM for GPU offloading, using CPU");
        return 0;
    }
    
    spdlog::info("Optimal GPU layers: {} / {} (estimated {} MB per layer)",
                max_layers, estimated_total_layers,
                bytes_per_layer / (1024 * 1024));
    
    return max_layers;
}

void BackendAwareLlamaModelHandle::allocateGPUMemory(
    const GPUBackendConfig& config) {
    
    if (!gpu_memory_manager_) return;
    
    // Pre-allocate VRAM für Model
    // (Tatsächliche Allokation erfolgt durch llama.cpp, aber wir tracken es)
    
    // Für Multi-GPU: Split across devices
    if (config.secondary_gpus.size() > 0) {
        spdlog::info("Multi-GPU setup detected: {} GPUs", 
                    1 + config.secondary_gpus.size());
        
        // Enable peer-to-peer access zwischen GPUs
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
    // Flash Attention (CUDA-spezifisch, erfordert Compute Capability >= 8.0)
    if (active_backend_ == acceleration::BackendType::CUDA) {
        // Note: llama.cpp detektiert Flash Attention automatisch
        // Hier könnten zusätzliche Optimierungen konfiguriert werden
        spdlog::debug("CUDA backend configured with Flash Attention support");
    }
    
    // Vulkan Compute Pipelines
    if (active_backend_ == acceleration::BackendType::VULKAN) {
        // Vulkan-spezifische Pipeline-Optimierungen
        spdlog::debug("Vulkan backend configured with compute pipelines");
    }
    
    // Metal Performance Shaders (Apple)
    if (active_backend_ == acceleration::BackendType::METAL) {
        spdlog::debug("Metal backend configured with MPS acceleration");
    }
}

std::string BackendAwareLlamaModelHandle::backend_name() const {
    auto& registry = acceleration::BackendRegistry::instance();
    auto* backend = registry.getBackend(active_backend_);
    return backend ? backend->name() : "Unknown";
}

bool BackendAwareLlamaModelHandle::transferToGPU(int target_gpu_id) {
    // Transfer model zwischen GPUs (für Load Balancing)
    if (!gpu_memory_manager_) return false;
    
    if (!gpu_memory_manager_->isGPUAvailable(target_gpu_id)) {
        spdlog::error("Target GPU {} not available", target_gpu_id);
        return false;
    }
    
    // Implementierung würde CUDA/Vulkan-spezifische Memory-Transfer nutzen
    // Für jetzt: Logging
    spdlog::info("Model transfer to GPU {} initiated", target_gpu_id);
    
    return true;
}

bool BackendAwareLlamaModelHandle::prefetchToGPU() {
    // Pre-fetch model weights für schnellere First-Token-Latency
    spdlog::debug("Prefetching model to GPU");
    return true;
}

} // namespace llm
} // namespace themis
```

#### Schritt 3: Integration in LlamaWrapper

**Modifizierte Datei**: `src/llm/llama_wrapper.cpp`

```cpp
// Zusätzliche Includes
#include "llm/llama_resource_manager.h"  // Mit Backend-Awareness

// Im Constructor
LlamaWrapper::LlamaWrapper(const Config& config)
    : config_(config) {
    
    // ... existing code ...
    
    // GPU Backend Configuration
    GPUBackendConfig gpu_config;
    gpu_config.preferred_backend = mapStringToBackendType(config_.gpu_backend);
    gpu_config.primary_gpu_id = config_.main_gpu;
    gpu_config.n_gpu_layers = config_.n_gpu_layers;
    gpu_config.enable_flash_attention = config_.use_flash_attn;
    gpu_config.auto_detect_optimal_layers = config_.auto_gpu_layers;
    
    // Multi-GPU Support
    if (config_.enable_multi_gpu) {
        gpu_config.secondary_gpus = config_.secondary_gpu_ids;
        gpu_config.enable_peer_to_peer = config_.gpu_peer_to_peer;
    }
    
    // Backend Discovery
    auto& backend_registry = acceleration::BackendRegistry::instance();
    backend_registry.autoDetect();
    
    auto available = backend_registry.getAvailableBackends();
    spdlog::info("Available GPU backends:");
    for (auto backend_type : available) {
        auto* backend = backend_registry.getBackend(backend_type);
        if (backend) {
            auto caps = backend->getCapabilities();
            spdlog::info("  - {}: {} VRAM, {} compute units",
                        backend->name(),
                        caps.maxMemoryBytes / (1024*1024*1024),
                        caps.computeUnits);
        }
    }
    
    gpu_backend_config_ = gpu_config;
    
    spdlog::info("GPU Backend Configuration:");
    spdlog::info("  Preferred: {}", config_.gpu_backend);
    spdlog::info("  GPU Layers: {}", gpu_config.n_gpu_layers);
    spdlog::info("  Primary GPU: {}", gpu_config.primary_gpu_id);
    spdlog::info("  Multi-GPU: {}", config_.enable_multi_gpu ? "enabled" : "disabled");
}

// Model Loading mit Backend-Awareness
bool LlamaWrapper::loadModel(const std::string& model_path, const json& config) {
    try {
        // Model-Parameter erstellen
        llama_model_params model_params = llama_model_default_params();
        model_params.use_mmap = config_.use_mmap;
        model_params.use_mlock = config_.use_mlock;
        
        // Backend-Aware Model Handle erstellen
        auto model_handle = std::make_unique<BackendAwareLlamaModelHandle>(
            model_path,
            model_params,
            gpu_backend_config_
        );
        
        spdlog::info("Model loaded with {} backend, {} GB VRAM",
                    model_handle->backend_name(),
                    model_handle->vram_usage() / (1024.0 * 1024.0 * 1024.0));
        
        // Context erstellen (wie vorher)
        // ...
        
        return true;
        
    } catch (const std::exception& e) {
        spdlog::error("Model loading failed: {}", e.what());
        return false;
    }
}
```

---

## Training mit GPU-Acceleration

### Problem

Das LoRA-Training nutzt aktuell keine GPU-Beschleunigung.

**Quelle**: `src/llm/lora_framework/lora_training_service.cpp` (Zeile 78)
```cpp
std::this_thread::sleep_for(std::chrono::milliseconds(10));  // Simulation!
```

### Lösung: GPU-Accelerated Training Loop

**Erweiterte Datei**: `include/llm/lora_framework/lora_training_service.h`

```cpp
#pragma once
#include "acceleration/compute_backend.h"
#include "llm/gpu_memory_manager.h"

namespace themis {
namespace llm {
namespace lora {

struct TrainingBackendConfig {
    // Backend Selection
    acceleration::BackendType backend = acceleration::BackendType::AUTO;
    
    // GPU Configuration
    std::vector<int> training_gpus;  // GPUs für Training
    bool use_mixed_precision = true;  // FP16 Training
    bool use_gradient_checkpointing = false;  // Memory-Einsparung
    
    // Data Parallelism (Multi-GPU)
    bool enable_data_parallel = false;
    int data_parallel_world_size = 1;
    
    // Optimization
    bool use_cuda_graphs = false;  // CUDA Graph Capture (reduziert Launch Overhead)
    bool use_kernel_fusion = true; // Fuse multiple kernels
};

class LoRATrainingService {
public:
    // ... existing API ...
    
    // NEU: Backend-Aware Training
    TrainingResult trainOnTheFlyGPU(
        const std::string& adapter_id,
        const TrainingData& data,
        const LoRAHyperparameters& hyperparameters,
        const TrainingBackendConfig& backend_config
    );

private:
    acceleration::IComputeBackend* training_backend_ = nullptr;
    std::shared_ptr<GPUMemoryManager> gpu_memory_manager_;
    
    // GPU Kernel Launcher
    void launchForwardKernel(/* params */);
    void launchBackwardKernel(/* params */);
    void launchOptimizerKernel(/* params */);
};

} // namespace lora
} // namespace llm
} // namespace themis
```

**Implementation**: `src/llm/lora_framework/lora_training_service.cpp`

```cpp
TrainingResult LoRATrainingService::trainOnTheFlyGPU(
    const std::string& adapter_id,
    const TrainingData& data,
    const LoRAHyperparameters& hyperparameters,
    const TrainingBackendConfig& backend_config) {
    
    // 1. Backend Selection
    auto& registry = acceleration::BackendRegistry::instance();
    training_backend_ = registry.getBackend(backend_config.backend);
    
    if (!training_backend_ || !training_backend_->isAvailable()) {
        spdlog::error("Training backend not available, falling back to CPU");
        return trainOnTheFly(adapter_id, data, hyperparameters);  // CPU Fallback
    }
    
    spdlog::info("Starting GPU-accelerated training with {} backend",
                training_backend_->name());
    
    // 2. GPU Memory Manager initialisieren
    GPUMemoryManager::Config mem_config;
    mem_config.enable_multi_gpu = backend_config.enable_data_parallel;
    mem_config.gpu_devices = backend_config.training_gpus;
    
    gpu_memory_manager_ = std::make_shared<GPUMemoryManager>(mem_config);
    
    // 3. LoRA Weights auf GPU allokieren
    size_t lora_memory = estimateLoRAMemory(hyperparameters);
    
    for (int gpu_id : backend_config.training_gpus) {
        void* gpu_ptr = gpu_memory_manager_->allocateGPU(
            adapter_id,
            lora_memory,
            gpu_id
        );
        
        if (!gpu_ptr) {
            throw std::runtime_error("GPU memory allocation failed");
        }
        
        spdlog::info("Allocated {} MB on GPU {}",
                    lora_memory / (1024 * 1024), gpu_id);
    }
    
    // 4. Training Loop mit GPU Kernels
    TrainingResult result;
    result.adapter_id = adapter_id;
    
    try {
        for (int epoch = 0; epoch < hyperparameters.num_epochs; ++epoch) {
            float epoch_loss = 0.0f;
            
            for (size_t batch_idx = 0; batch_idx < data.size(); 
                 batch_idx += hyperparameters.batch_size) {
                
                // Batch auf GPU kopieren
                auto batch = prepareBatch(data, batch_idx, hyperparameters.batch_size);
                
                // Forward Pass (GPU Kernel)
                launchForwardKernel(/* batch, lora_weights, ... */);
                
                // Loss berechnen (GPU)
                float loss = computeLossGPU(/* ... */);
                epoch_loss += loss;
                
                // Backward Pass (GPU Kernel für Gradient Computation)
                launchBackwardKernel(/* loss, lora_weights, ... */);
                
                // Optimizer Step (GPU Kernel - Adam/AdamW)
                launchOptimizerKernel(/* gradients, lora_weights, hyperparameters */);
                
                // Metriken aktualisieren
                updateMetrics(epoch, batch_idx, loss);
            }
            
            spdlog::info("Epoch {}: Loss = {:.4f}", epoch + 1, 
                        epoch_loss / data.size());
        }
        
        result.success = true;
        result.final_loss = epoch_loss / data.size();
        
    } catch (const std::exception& e) {
        spdlog::error("GPU training failed: {}", e.what());
        result.success = false;
        result.error_message = e.what();
    }
    
    // 5. GPU Memory freigeben
    gpu_memory_manager_->freeModel(adapter_id);
    
    return result;
}

void LoRATrainingService::launchForwardKernel(/* params */) {
    // CUDA/Vulkan/HIP Kernel Launch
    // Beispiel: Matrix-Multiplikation für LoRA Forward Pass
    
    if (training_backend_->type() == acceleration::BackendType::CUDA) {
        // CUDA Kernel
        // cudaLaunchKernel(forward_kernel, ...);
        spdlog::debug("Launched CUDA forward kernel");
        
    } else if (training_backend_->type() == acceleration::BackendType::VULKAN) {
        // Vulkan Compute Shader
        spdlog::debug("Launched Vulkan compute shader (forward)");
        
    } else {
        // Fallback
        spdlog::warn("GPU kernel launch not implemented for this backend");
    }
}

void LoRATrainingService::launchBackwardKernel(/* params */) {
    // Gradient Computation Kernel
    spdlog::debug("Launched backward kernel");
}

void LoRATrainingService::launchOptimizerKernel(/* params */) {
    // Adam/AdamW Optimizer Kernel
    spdlog::debug("Launched optimizer kernel");
}
```

---

## Configuration: GPU Backend Selection

**Neue Konfiguration**: `config/llm/gpu_backends.yaml`

```yaml
gpu:
  # Backend Preference (AUTO = auto-detect)
  backend: AUTO
  # backend: CUDA      # Force NVIDIA CUDA
  # backend: VULKAN    # Force Vulkan (cross-platform)
  # backend: HIP       # Force AMD HIP
  # backend: METAL     # Force Apple Metal
  
  # Fallback Order
  fallback_backends:
    - CUDA
    - VULKAN
    - HIP
    - METAL
    - DIRECTX
    - OPENCL
  
  # GPU Device Selection
  primary_gpu: 0
  secondary_gpus: []  # [1, 2, 3] für Multi-GPU
  
  # Layer Offloading
  n_gpu_layers: -1  # -1 = all layers
  auto_detect_optimal_layers: true
  
  # Advanced Features
  enable_flash_attention: true
  enable_tensor_cores: true
  enable_unified_memory: false
  
  # Multi-GPU
  enable_multi_gpu: false
  enable_peer_to_peer: false
  
  # Memory Management
  max_vram_per_gpu_gb: 24
  reserved_vram_gb: 2

training:
  # Training Backend
  backend: AUTO
  training_gpus: [0]
  
  # Mixed Precision
  use_mixed_precision: true
  
  # Multi-GPU Training
  enable_data_parallel: false
  data_parallel_world_size: 1
  
  # Optimization
  use_cuda_graphs: false
  use_kernel_fusion: true
  use_gradient_checkpointing: false
```

---

## Testing: GPU Backend Verification

**Neue Datei**: `tests/test_llm_gpu_backends.cpp`

```cpp
#include <gtest/gtest.h>
#include "llm/llama_resource_manager.h"
#include "acceleration/backend_registry.h"

TEST(LLMGPUBackends, BackendDetection) {
    auto& registry = acceleration::BackendRegistry::instance();
    registry.autoDetect();
    
    auto available = registry.getAvailableBackends();
    ASSERT_GT(available.size(), 0) << "No backends detected";
    
    for (auto backend_type : available) {
        auto* backend = registry.getBackend(backend_type);
        ASSERT_NE(backend, nullptr);
        EXPECT_TRUE(backend->isAvailable());
        
        auto caps = backend->getCapabilities();
        std::cout << backend->name() << ": "
                  << caps.maxMemoryBytes / (1024*1024*1024) << " GB VRAM, "
                  << caps.computeUnits << " compute units\n";
    }
}

TEST(LLMGPUBackends, VulkanBackendLoadModel) {
    // Test Vulkan Backend (cross-platform)
    themis::llm::GPUBackendConfig config;
    config.preferred_backend = acceleration::BackendType::VULKAN;
    config.n_gpu_layers = 10;
    
    // Model laden (erfordert Testmodell)
    // auto model = BackendAwareLlamaModelHandle("test_model.gguf", ..., config);
    // EXPECT_EQ(model.active_backend(), acceleration::BackendType::VULKAN);
}

TEST(LLMGPUBackends, CUDABackendLoadModel) {
    // Test CUDA Backend (NVIDIA)
    if (!acceleration::BackendRegistry::instance()
            .getBackend(acceleration::BackendType::CUDA)) {
        GTEST_SKIP() << "CUDA not available";
    }
    
    themis::llm::GPUBackendConfig config;
    config.preferred_backend = acceleration::BackendType::CUDA;
    config.enable_flash_attention = true;
    config.enable_tensor_cores = true;
    
    // Model laden
    // ...
}

TEST(LLMGPUBackends, MultiGPUMemoryAllocation) {
    auto& registry = acceleration::BackendRegistry::instance();
    auto* backend = registry.getBackend(acceleration::BackendType::CUDA);
    
    if (!backend || !backend->isAvailable()) {
        GTEST_SKIP() << "CUDA not available for multi-GPU test";
    }
    
    themis::llm::GPUMemoryManager::Config mem_config;
    mem_config.enable_multi_gpu = true;
    mem_config.gpu_devices = {0, 1};  // Erfordert 2 GPUs
    
    themis::llm::GPUMemoryManager manager(mem_config);
    
    // Allokiere auf beiden GPUs
    void* gpu0_ptr = manager.allocateGPU("test_model", 1024*1024*1024, 0);
    void* gpu1_ptr = manager.allocateGPU("test_model", 1024*1024*1024, 1);
    
    EXPECT_NE(gpu0_ptr, nullptr);
    EXPECT_NE(gpu1_ptr, nullptr);
    
    // Cleanup
    manager.freeModel("test_model");
}
```

---

## Zusammenfassung: GPU-Integration

### Vorhandene Infrastruktur (✅ Genutzt)

1. **Backend Registry**: Auto-Detection von CUDA, Vulkan, HIP, Metal, etc.
2. **GPU Memory Manager**: Multi-GPU VRAM-Tracking und Allokation
3. **Compute Backend Interface**: Abstrakte Schnittstelle für alle Backends

### Neue Komponenten (📝 Zu Implementieren)

1. **BackendAwareLlamaModelHandle**: Integration von Backend-Selection in Model Loading
2. **GPU-Accelerated Training**: CUDA/Vulkan Kernels für LoRA Training
3. **Configuration**: YAML-Config für Backend-Auswahl
4. **Tests**: Backend-Verifikation Tests

### Implementierungsreihenfolge (aktualisiert)

**Woche 1-2**: Backend-Aware Model Loading
- BackendAwareLlamaModelHandle implementieren
- Backend Selection Logic
- Integration in LlamaWrapper

**Woche 3-4**: GPU Memory Integration
- GPU Memory Manager in Model Loading integrieren
- Auto-Detection von optimalen GPU Layers
- Multi-GPU Support testen

**Woche 7-10**: GPU-Accelerated Training (Teil von Phase 1.2)
- CUDA/Vulkan Training Kernels
- Forward/Backward/Optimizer Kernels
- Mixed Precision Training

**Woche 11-12**: Multi-GPU Training
- Data Parallelism
- Peer-to-Peer Memory Transfer
- Load Balancing

---

**Version**: 1.1  
**Status**: Addendum Complete  
**Nächster Schritt**: Integration in Haupt-Implementation Guide
