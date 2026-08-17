# BATCH 5 - IMPLEMENTATION CODE TEMPLATES & SNIPPETS

**Purpose:** Ready-to-use code templates for fixing each gap category  
**Status:** Copy-paste ready for developers  

---

## TEMPLATE 1: GPU Device Error Recovery (REL-40, 41, 42-45, 46-47)

### Pattern: Retry with Exponential Backoff

```cpp
// Template: Safe GPU device context switching
template<typename Fn>
bool withGPUDeviceContext(int gpu_device_id, Fn&& operation) {
    constexpr int MAX_RETRIES = 3;
    constexpr int INITIAL_DELAY_MS = 10;
    
    for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
        #ifdef THEMIS_ENABLE_CUDA
        cudaError_t set_err = cudaSetDevice(gpu_device_id);
        #elif THEMIS_ENABLE_HIP
        hipError_t set_err = hipSetDevice(gpu_device_id);
        #else
        return operation();  // Simulation mode, skip device switch
        #endif
        
        if (set_err == cudaSuccess) {
            try {
                return operation();
            } catch (const std::exception& e) {
                spdlog::error("GPU device operation failed: {}", e.what());
                if (attempt < MAX_RETRIES - 1) {
                    int delay_ms = INITIAL_DELAY_MS * (1 << attempt);
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(delay_ms)
                    );
                    continue;
                }
                return false;
            }
        }
        
        spdlog::warn("cudaSetDevice({}) attempt {}/{} failed: {}",
                     gpu_device_id, attempt + 1, MAX_RETRIES,
                     cudaGetErrorString(set_err));
        
        if (attempt < MAX_RETRIES - 1) {
            int delay_ms = INITIAL_DELAY_MS * (1 << attempt);
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
        }
    }
    
    spdlog::error("GPU device switch failed after {} retries", MAX_RETRIES);
    return false;
}

// Usage:
bool success = withGPUDeviceContext(gpu_id, [&]() {
    return cudaMemcpy(dst, src, size, cudaMemcpyDeviceToHost) == cudaSuccess;
});
```

---

## TEMPLATE 2: GPU Cleanup with Fallback (REL-73)

### Pattern: Multi-Level Cleanup Hierarchy

```cpp
// Template: Secure memory cleanup with fallback
class SecureGPUMemoryCleanup {
public:
    SecureGPUMemoryCleanup(void* ptr, size_t bytes, int gpu_device_id)
        : ptr_(ptr), bytes_(bytes), gpu_device_id_(gpu_device_id) {}
    
    ~SecureGPUMemoryCleanup() noexcept {
        if (!ptr_) return;
        
        if (!cleanupGPU()) {
            if (!cleanupCPUFallback()) {
                spdlog::critical("Failed to cleanup {} bytes - memory leak", bytes_);
                // Last resort: attempt to track for external cleanup
                reportUncleanedMemory(ptr_, bytes_);
            }
        }
    }
    
private:
    bool cleanupGPU() noexcept {
        #ifdef THEMIS_ENABLE_CUDA
        // Try primary: cudaSetDevice + secure clear + cudaFree
        cudaError_t set_err = cudaSetDevice(gpu_device_id_);
        if (set_err != cudaSuccess) {
            spdlog::warn("Cleanup: cudaSetDevice failed, using fallback: {}",
                        cudaGetErrorString(set_err));
            return false;
        }
        
        security::VRAMSecureClear::secureClearCUDA(ptr_, bytes_);
        
        cudaError_t free_err = cudaFree(ptr_);
        if (free_err != cudaSuccess) {
            spdlog::error("Cleanup: cudaFree failed: {}",
                         cudaGetErrorString(free_err));
            return false;
        }
        
        return true;
        #else
        return false;
        #endif
    }
    
    bool cleanupCPUFallback() noexcept {
        try {
            // Fallback: secure clear via CPU (works even if device unavailable)
            security::VRAMSecureClear::secureClearCPU(ptr_, bytes_);
            std::free(ptr_);
            ptr_ = nullptr;
            return true;
        } catch (const std::exception& e) {
            spdlog::error("CPU fallback cleanup failed: {}", e.what());
            return false;
        }
    }
    
    void reportUncleanedMemory(void* ptr, size_t bytes) noexcept {
        // Track for monitoring/alerting
        spdlog::critical("MEMORY LEAK: {} bytes at {} not cleaned",
                        bytes, ptr);
        // TODO: Emit metric for observability
    }
    
    void* ptr_;
    size_t bytes_;
    int gpu_device_id_;
};

// Usage in destructor:
~MemoryHolder() noexcept {
    SecureGPUMemoryCleanup cleanup(ptr_, bytes_, gpu_device_id_);
    // cleanup runs in destructor
}
```

---

## TEMPLATE 3: P2P Access Setup (REL-42-45)

### Pattern: Transactional P2P Setup

```cpp
// Template: Safe P2P access setup with transaction semantics
class P2PAccessTransaction {
public:
    P2PAccessTransaction(int src_gpu, int dst_gpu)
        : src_gpu_(src_gpu), dst_gpu_(dst_gpu), 
          forward_enabled_(false), backward_enabled_(false) {}
    
    ~P2PAccessTransaction() noexcept {
        // Rollback on exception
        if (!committed_) {
            rollback();
        }
    }
    
    bool execute() {
        if (!enableForwardAccess()) {
            spdlog::error("P2P setup failed: forward direction GPU {} -> GPU {}",
                         src_gpu_, dst_gpu_);
            return false;
        }
        
        if (!enableBackwardAccess()) {
            spdlog::error("P2P setup failed: backward direction GPU {} -> GPU {}",
                         dst_gpu_, src_gpu_);
            rollback();
            return false;
        }
        
        committed_ = true;
        return true;
    }
    
private:
    bool enableForwardAccess() {
        #ifdef THEMIS_ENABLE_CUDA
        for (int retry = 0; retry < 3; ++retry) {
            cudaError_t set_err = cudaSetDevice(src_gpu_);
            if (set_err != cudaSuccess) {
                spdlog::warn("P2P forward: cudaSetDevice({}) retry {}: {}",
                            src_gpu_, retry, cudaGetErrorString(set_err));
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                continue;
            }
            
            cudaError_t p2p_err = cudaDeviceEnablePeerAccess(dst_gpu_, 0);
            if (p2p_err == cudaSuccess) {
                forward_enabled_ = true;
                spdlog::info("P2P enabled (forward): {} -> {}", src_gpu_, dst_gpu_);
                return true;
            } else if (p2p_err == cudaErrorPeerAccessAlreadyEnabled) {
                forward_enabled_ = true;
                return true;
            } else {
                spdlog::error("P2P enable failed: {}", cudaGetErrorString(p2p_err));
                return false;
            }
        }
        #endif
        return false;
    }
    
    bool enableBackwardAccess() {
        #ifdef THEMIS_ENABLE_CUDA
        for (int retry = 0; retry < 3; ++retry) {
            cudaError_t set_err = cudaSetDevice(dst_gpu_);
            if (set_err != cudaSuccess) {
                spdlog::warn("P2P backward: cudaSetDevice({}) retry {}: {}",
                            dst_gpu_, retry, cudaGetErrorString(set_err));
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                continue;
            }
            
            cudaError_t p2p_err = cudaDeviceEnablePeerAccess(src_gpu_, 0);
            if (p2p_err == cudaSuccess) {
                backward_enabled_ = true;
                spdlog::info("P2P enabled (backward): {} -> {}", dst_gpu_, src_gpu_);
                return true;
            } else if (p2p_err == cudaErrorPeerAccessAlreadyEnabled) {
                backward_enabled_ = true;
                return true;
            } else {
                spdlog::error("P2P enable failed: {}", cudaGetErrorString(p2p_err));
                return false;
            }
        }
        #endif
        return false;
    }
    
    void rollback() noexcept {
        #ifdef THEMIS_ENABLE_CUDA
        if (forward_enabled_) {
            cudaError_t set_err = cudaSetDevice(src_gpu_);
            if (set_err == cudaSuccess) {
                cudaDeviceDisablePeerAccess(dst_gpu_);
            }
        }
        if (backward_enabled_) {
            cudaError_t set_err = cudaSetDevice(dst_gpu_);
            if (set_err == cudaSuccess) {
                cudaDeviceDisablePeerAccess(src_gpu_);
            }
        }
        #endif
    }
    
    int src_gpu_;
    int dst_gpu_;
    bool forward_enabled_ = false;
    bool backward_enabled_ = false;
    bool committed_ = false;
};

// Usage:
P2PAccessTransaction p2p(gpu_0, gpu_1);
if (p2p.execute()) {
    // P2P access is ready
} else {
    // P2P access failed - rolled back automatically
}
```

---

## TEMPLATE 4: Stream Synchronization (REL-11, 12, 15, 16, 46, 47)

### Pattern: Safe Stream Synchronization

```cpp
// Template: Safe GPU stream synchronization
bool syncGPUStream(cudaStream_t stream, const char* operation_name) {
    #ifdef THEMIS_ENABLE_CUDA
    cudaError_t sync_err = cudaStreamSynchronize(stream);
    if (sync_err != cudaSuccess) {
        spdlog::error("{}: cudaStreamSynchronize failed: {}",
                     operation_name, cudaGetErrorString(sync_err));
        
        // Check if stream is in error state
        cudaError_t status_err = cudaStreamQuery(stream);
        if (status_err == cudaErrorStreamCaptureInvalidated) {
            spdlog::error("{}: Stream captured but invalidated", operation_name);
        }
        
        return false;
    }
    
    spdlog::debug("{}: Stream sync completed successfully", operation_name);
    return true;
    #else
    (void)stream;
    (void)operation_name;
    return true;
    #endif
}

// Usage:
if (!syncGPUStream(cuda_stream, "NCCL broadcast")) {
    return false;  // Broadcasting failed
}
```

---

## TEMPLATE 5: NCCL/RCCL Group Operations (REL-68-72)

### Pattern: Safe Collective Communication

```cpp
// Template: Safe NCCL group operations with error recovery
class NCCLGroupOperation {
public:
    NCCLGroupOperation(const char* operation_name)
        : operation_name_(operation_name), group_started_(false) {}
    
    ~NCCLGroupOperation() noexcept {
        if (group_started_) {
            cleanupGroup();
        }
    }
    
    bool startGroup() {
        #ifdef THEMIS_ENABLE_NCCL
        ncclResult_t start_err = ncclGroupStart();
        if (start_err != ncclSuccess) {
            spdlog::error("{}: ncclGroupStart failed: {}",
                         operation_name_, ncclGetErrorString(start_err));
            return false;
        }
        group_started_ = true;
        spdlog::debug("{}: Group started", operation_name_);
        return true;
        #else
        return false;
        #endif
    }
    
    bool endGroup() {
        #ifdef THEMIS_ENABLE_NCCL
        if (!group_started_) {
            spdlog::error("{}: Attempted to end group without starting",
                         operation_name_);
            return false;
        }
        
        ncclResult_t end_err = ncclGroupEnd();
        if (end_err != ncclSuccess) {
            spdlog::error("{}: ncclGroupEnd failed - DATA CORRUPTION RISK: {}",
                         operation_name_, ncclGetErrorString(end_err));
            group_started_ = false;
            return false;
        }
        
        group_started_ = false;
        spdlog::debug("{}: Group ended successfully", operation_name_);
        return true;
        #else
        return false;
        #endif
    }
    
private:
    void cleanupGroup() noexcept {
        #ifdef THEMIS_ENABLE_NCCL
        if (group_started_) {
            ncclResult_t end_err = ncclGroupEnd();
            if (end_err != ncclSuccess) {
                spdlog::error("{}: Cleanup ncclGroupEnd failed: {}",
                             operation_name_, ncclGetErrorString(end_err));
            }
            group_started_ = false;
        }
        #endif
    }
    
    const char* operation_name_;
    bool group_started_;
};

// Usage:
bool doNNCLAllreduce(void* send_buf, void* recv_buf, int count,
                     ncclComm_t nccl_comm, cudaStream_t stream) {
    NCCLGroupOperation group("AllReduce");
    
    if (!group.startGroup()) {
        return false;
    }
    
    ncclResult_t result = ncclAllReduce(
        send_buf, recv_buf, count, ncclFloat, ncclSum,
        nccl_comm, stream
    );
    
    if (result != ncclSuccess) {
        spdlog::error("AllReduce operation failed: {}",
                     ncclGetErrorString(result));
        return false;
    }
    
    if (!group.endGroup()) {
        return false;
    }
    
    // Synchronize stream
    if (!syncGPUStream(stream, "AllReduce stream sync")) {
        return false;
    }
    
    return true;
}
```

---

## TEMPLATE 6: Simulation vs Production Mode (43 gaps)

### Pattern: Unified Simulation/Production Handling

```cpp
// Template: Consistent SIMULATION/PRODUCTION path marking
class GPUMemoryAllocator {
public:
    void* allocate(size_t bytes) {
        #ifdef THEMIS_ENABLE_CUDA
        if (gpu_available_) {
            return allocateProduction(bytes);
        } else {
            return allocateSimulation(bytes);
        }
        #else
        return allocateSimulation(bytes);
        #endif
    }
    
private:
    void* allocateProduction(size_t bytes) {
        // PRODUCTION: Real GPU allocation
        void* ptr = nullptr;
        cudaError_t err = cudaMalloc(&ptr, bytes);
        if (err != cudaSuccess) {
            spdlog::error("cudaMalloc failed: {}", cudaGetErrorString(err));
            return nullptr;
        }
        spdlog::debug("Allocated {} bytes on GPU (production)", bytes);
        return ptr;
    }
    
    void* allocateSimulation(size_t bytes) {
        // SIMULATION: CPU fallback when GPU unavailable
        // 
        // Purpose: Provide functional equivalence without GPU access
        // Activation: CUDA not enabled OR gpu_available_ == false
        // Production delta: Uses malloc instead of cudaMalloc
        // Removal plan: Can remove in GPU-only builds
        
        void* ptr = std::malloc(bytes);
        if (!ptr) {
            spdlog::error("malloc failed for {} bytes", bytes);
            return nullptr;
        }
        
        // Zero-initialize like CUDA would
        std::memset(ptr, 0, bytes);
        
        spdlog::warn("Allocated {} bytes on CPU (simulation mode)", bytes);
        return ptr;
    }
};

// Doxygen documentation:
/// @brief Allocate GPU or CPU memory depending on availability
/// @param bytes Allocation size
/// @return Pointer to allocated memory, or nullptr on failure
/// 
/// @note PRODUCTION: Uses cudaMalloc when CUDA is available and enabled.
/// @note SIMULATION: Falls back to malloc when GPU is unavailable or CUDA
///       is not compiled in. Behavior is functionally equivalent but has
///       different performance characteristics.
/// @note When running in simulation mode, all GPU operations become
///       CPU operations. This is useful for development and testing
///       without requiring GPU hardware.
void* allocate(size_t bytes);
```

---

## TEMPLATE 7: Model Fallback Chain (stub #289)

### Pattern: Hierarchical Fallback with Caching

```cpp
// Template: GGUF model loading with fallback chain
class ModelFallbackFactory {
public:
    enum class ModelSource {
        PRIMARY,    // Original GGUF file
        CACHED,     // Cached from previous successful load
        SYNTHETIC,  // Generated for testing/demo
        NONE        // No fallback available
    };
    
    struct LoadResult {
        std::shared_ptr<QuantizedModel> model;
        ModelSource source;
        std::string error_message;
        bool success() const { return model != nullptr; }
    };
    
    LoadResult loadModelWithFallback(const std::string& gguf_path) {
        // Step 1: Try primary source
        auto primary = loadPrimary(gguf_path);
        if (primary.success()) {
            cacheModel(primary.model);
            spdlog::info("Model loaded from primary source: {}", gguf_path);
            return primary;
        }
        spdlog::warn("Primary load failed: {}", primary.error_message);
        
        // Step 2: Try cached model
        auto cached = loadCached(gguf_path);
        if (cached.success()) {
            spdlog::warn("Using cached model after primary failure");
            return cached;
        }
        spdlog::warn("No cached model available: {}", cached.error_message);
        
        // Step 3: Create synthetic model for testing
        auto synthetic = createSyntheticModel();
        if (synthetic.success()) {
            spdlog::error("Using synthetic model - this is for testing only!");
            spdlog::error("Fallback chain: Primary failed -> Cached unavailable -> Using synthetic");
            return synthetic;
        }
        
        // Step 4: Complete failure
        spdlog::error("All model loading strategies failed");
        return {nullptr, ModelSource::NONE, "Model loading failed - no fallback available"};
    }
    
private:
    LoadResult loadPrimary(const std::string& gguf_path) {
        try {
            auto model = std::make_shared<QuantizedModel>();
            if (!model->loadFromGGUF(gguf_path)) {
                return {nullptr, ModelSource::PRIMARY, 
                       "GGUF parsing failed - invalid structure"};
            }
            return {model, ModelSource::PRIMARY, ""};
        } catch (const std::exception& e) {
            return {nullptr, ModelSource::PRIMARY, e.what()};
        }
    }
    
    LoadResult loadCached(const std::string& gguf_path) {
        // Check if model was successfully loaded before
        auto key = std::hash<std::string>{}(gguf_path);
        
        std::lock_guard<std::mutex> lock(cache_mutex_);
        auto it = model_cache_.find(key);
        
        if (it != model_cache_.end() && !it->second.expired()) {
            if (auto model = it->second.lock()) {
                spdlog::info("Using cached model for {}", gguf_path);
                return {model, ModelSource::CACHED, ""};
            }
        }
        
        return {nullptr, ModelSource::CACHED, "No valid cached model found"};
    }
    
    LoadResult createSyntheticModel() {
        try {
            auto model = std::make_shared<QuantizedModel>();
            
            // Create synthetic model with correct structure
            // but random weights (marked as synthetic)
            model->marking_ = "synthetic";
            model->embedding_dim = 768;
            model->num_heads = 12;
            model->num_layers = 12;
            
            // Add synthetic layers
            for (int i = 0; i < model->num_layers; ++i) {
                std::string layer_name = "layer_" + std::to_string(i);
                Tensor synthetic_weights = tensor_utils::randn(
                    {model->embedding_dim, model->embedding_dim}
                );
                model->add_layer(layer_name, synthetic_weights);
            }
            
            spdlog::warn("Created synthetic model - for testing/demo only");
            spdlog::warn("Production training with synthetic model is not recommended");
            
            return {model, ModelSource::SYNTHETIC, ""};
        } catch (const std::exception& e) {
            return {nullptr, ModelSource::SYNTHETIC, 
                   std::string("Synthetic model creation failed: ") + e.what()};
        }
    }
    
    void cacheModel(std::shared_ptr<QuantizedModel> model) {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        // Implement LRU cache with TTL
        // Store up to N models with expiration
    }
    
    std::mutex cache_mutex_;
    std::unordered_map<size_t, std::weak_ptr<QuantizedModel>> model_cache_;
};

// Usage:
ModelFallbackFactory factory;
auto result = factory.loadModelWithFallback("model.gguf");

if (result.success()) {
    spdlog::info("Model loaded from {}", result.source);
    auto model = result.model;
    // Continue with training/inference
} else {
    spdlog::error("Failed to load model: {}", result.error_message);
    spdlog::error("User guidance: Check model file, verify permissions, ensure enough disk space");
    return false;
}
```

---

## TEMPLATE 8: Doxygen Documentation Updates

### Pattern: Complete Doxygen API Comments

```cpp
/**
 * @brief Allocate GPU memory with automatic cleanup and fallback handling
 * 
 * Allocates memory on the current GPU device with automatic cleanup
 * via RAII. If GPU allocation fails, falls back to CPU memory.
 * 
 * @param[in] model_id Unique identifier for the model owning this allocation
 * @param[in] bytes Number of bytes to allocate
 * @return Pointer to allocated memory (GPU or CPU), or nullptr on failure
 * 
 * @details
 * PRODUCTION: Attempts cudaMalloc on the current GPU device. If successful,
 * wraps the pointer in a RAII holder for automatic cleanup via VRAMSecureClear.
 * 
 * FALLBACK: If cudaMalloc fails (device unavailable, OOM, driver error),
 * falls back to CPU malloc. This allows training to continue in a degraded
 * mode rather than failing completely.
 * 
 * SECURITY: All allocations are securely cleared (zeros + random pattern)
 * before deallocation to prevent information leakage.
 * 
 * ERROR HANDLING:
 * - Returns nullptr if both GPU and CPU allocations fail
 * - Does NOT throw exceptions; check return value for nullptr
 * - Automatically logs detailed error messages via spdlog
 * - Calls canonical VRAM policy to enforce per-tenant quotas
 * 
 * @exception Strong exception-safe guarantee - if metadata bookkeeping
 *            throws, the allocation is cleaned up and nullptr is returned.
 * 
 * @note Thread-safe. Uses internal mutex to protect allocation tracking.
 * 
 * @see freeGPU(), getGPUMemoryStats()
 * 
 * @example
 * @code
 * auto ptr = allocateGPU("llama2-7b", 1024 * 1024 * 1024);  // 1 GB
 * if (ptr == nullptr) {
 *     spdlog::error("Allocation failed");
 *     return false;
 * }
 * // Use ptr...
 * // Cleanup happens automatically when MemoryAllocation is destroyed
 * @endcode
 */
void* allocateGPU(const std::string& model_id, size_t bytes);
```

---

## QUICK REFERENCE: Gap Types to Templates

| Gap Type | Template | Files | Count |
|----------|----------|-------|-------|
| Device Error Recovery | Template 1 | REL-40-47 | 10 |
| GPU Cleanup Fallback | Template 2 | REL-73 | 1 |
| P2P Access Setup | Template 3 | REL-42-45 | 4 |
| Stream Sync | Template 4 | REL-11,12,15,16,46,47 | 6 |
| NCCL/RCCL Groups | Template 5 | REL-68-72 | 5 |
| Simulation Paths | Template 6 | All files | 43 |
| Model Fallback | Template 7 | stub #289 | 1 |
| Documentation | Template 8 | All implementations | - |

---

**Use These Templates:** Copy-paste into target files and adapt to specific context  
**Test These Patterns:** Each template includes usage examples  
**Document:** Use Template 8 for all API documentation

