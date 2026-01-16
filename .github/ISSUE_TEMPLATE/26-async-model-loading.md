---
name: Performance - Async Model Loading
about: Implement asynchronous model loading for improved performance
title: '[Performance] Implement Async Model Loading (v1.3.0)'
labels: ['performance', 'llm', 'priority-medium', 'phase-2']
assignees: ''
---

## 📋 Overview

Implement asynchronous model loading to improve performance and user experience. Currently, model loading is synchronous, blocking the calling thread during model initialization.

**Related Documentation**: 
- `docs/analysis/LLM_LORA_SYSTEM_ANALYSIS.md` - Performance optimization
- `src/llm/model_loader.cpp` - Current synchronous implementation

**Current Status**: Marked as v1.3.0 feature in code (line 84: `// TODO: Implement async loading in v1.3.0`)

## 🎯 Goals

Enable non-blocking model loading with background initialization, improving responsiveness and allowing concurrent operations.

## 📊 Current Status

**Completion**: 0% (Synchronous loading works, async not implemented)

### ✅ Already Implemented
- Synchronous model loading with `getOrLoadModel()`
- Model caching and LRU eviction
- VRAM/RAM tracking
- Lazy loading on first inference

### ❌ Missing Implementation
- Asynchronous model loading
- Background thread management
- Loading progress callbacks
- Concurrent load requests handling
- Model preloading API

## 📝 Detailed Requirements

### 1. Async Model Loading API

**Priority**: 🟡 High  
**Effort**: 1 week

**Implementation Tasks**:
- [ ] Add `preloadModelAsync()` method
- [ ] Implement thread pool for model loading
- [ ] Add `std::future` return for async operations
- [ ] Support loading progress callbacks
- [ ] Handle concurrent load requests

**Files to Modify**:
- `src/llm/model_loader.cpp` - Implement async methods
- `include/llm/model_loader.h` - Update interface

**Code Example**:
```cpp
// New async API
std::future<CachedModel*> LazyModelLoader::preloadModelAsync(
    const std::string& model_id,
    const std::string& model_path,
    const json& load_config,
    std::function<void(float)> progress_callback
) {
    return std::async(std::launch::async, [=, this]() {
        spdlog::info("Starting async model load: {}", model_id);
        
        // Check if already loading
        {
            std::lock_guard<std::mutex> lock(loading_mutex_);
            if (loading_models_.count(model_id) > 0) {
                spdlog::info("Model already loading: {}", model_id);
                return waitForModelLoad(model_id);
            }
            loading_models_.insert(model_id);
        }
        
        // Load model with progress reporting
        auto* model = loadModelInternalWithProgress(
            model_id, 
            model_path, 
            load_config,
            progress_callback
        );
        
        // Remove from loading set
        {
            std::lock_guard<std::mutex> lock(loading_mutex_);
            loading_models_.erase(model_id);
            loading_cv_.notify_all();
        }
        
        return model;
    });
}
```

### 2. Thread Pool Management

**Priority**: 🟡 High  
**Effort**: 3-4 days

**Implementation Tasks**:
- [ ] Create thread pool for model loading
- [ ] Limit concurrent model loads (default: 2)
- [ ] Queue pending load requests
- [ ] Support load priority
- [ ] Add thread pool metrics

**Code Example**:
```cpp
class ModelLoadingThreadPool {
public:
    ModelLoadingThreadPool(size_t num_threads = 2)
        : stop_(false) {
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(mutex_);
                        cv_.wait(lock, [this] {
                            return stop_ || !tasks_.empty();
                        });
                        
                        if (stop_ && tasks_.empty()) {
                            return;
                        }
                        
                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }
                    task();
                }
            });
        }
    }
    
    template<class F>
    auto enqueue(F&& f) -> std::future<decltype(f())> {
        using return_type = decltype(f());
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::forward<F>(f)
        );
        std::future<return_type> res = task->get_future();
        
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (stop_) {
                throw std::runtime_error("Thread pool stopped");
            }
            tasks_.emplace([task]() { (*task)(); });
        }
        cv_.notify_one();
        return res;
    }
    
private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool stop_;
};
```

### 3. Loading Progress Callbacks

**Priority**: 🟢 Medium  
**Effort**: 2-3 days

**Implementation Tasks**:
- [ ] Add progress reporting during model load
- [ ] Report loading stages (download, parse, GPU upload)
- [ ] Calculate percentage completion
- [ ] Support cancellation
- [ ] Add ETA estimation

**Code Example**:
```cpp
CachedModel* LazyModelLoader::loadModelInternalWithProgress(
    const std::string& model_id,
    const std::string& model_path,
    const json& config,
    std::function<void(float)> progress_callback
) {
    // Stage 1: File reading (0-30%)
    if (progress_callback) progress_callback(0.0f);
    
    auto file_data = readModelFile(model_path);
    if (progress_callback) progress_callback(0.3f);
    
    // Stage 2: Model parsing (30-60%)
    llama_model_params params = llama_model_default_params();
    configureModelParams(params, config);
    if (progress_callback) progress_callback(0.5f);
    
    llama_model* lmodel = llama_load_model_from_file(
        model_path.c_str(), 
        params
    );
    if (progress_callback) progress_callback(0.6f);
    
    // Stage 3: Context creation (60-80%)
    llama_context_params ctx_params = llama_context_default_params();
    configureContextParams(ctx_params, config);
    if (progress_callback) progress_callback(0.7f);
    
    llama_context* lctx = llama_new_context_with_model(lmodel, ctx_params);
    if (progress_callback) progress_callback(0.8f);
    
    // Stage 4: GPU upload (80-100%)
    if (config.value("n_gpu_layers", 0) > 0) {
        // Upload layers to GPU
        uploadLayersToGPU(lmodel, lctx);
    }
    if (progress_callback) progress_callback(1.0f);
    
    return createCachedModel(model_id, lmodel, lctx);
}
```

### 4. Concurrent Request Handling

**Priority**: 🟡 High  
**Effort**: 2-3 days

**Implementation Tasks**:
- [ ] Handle multiple concurrent load requests for same model
- [ ] Deduplicate simultaneous loads
- [ ] Wait for in-progress loads
- [ ] Support load cancellation
- [ ] Add timeout handling

**Code Example**:
```cpp
CachedModel* LazyModelLoader::waitForModelLoad(
    const std::string& model_id,
    std::chrono::seconds timeout = std::chrono::seconds(300)
) {
    auto start = std::chrono::steady_clock::now();
    
    while (true) {
        {
            std::unique_lock<std::mutex> lock(loading_mutex_);
            
            // Check if loading completed
            if (loading_models_.count(model_id) == 0) {
                // Model should be in cache now
                std::lock_guard<std::mutex> cache_lock(mutex_);
                auto it = models_.find(model_id);
                if (it != models_.end()) {
                    return it->second.get();
                }
                return nullptr; // Load failed
            }
            
            // Wait for notification or timeout
            auto elapsed = std::chrono::steady_clock::now() - start;
            if (elapsed >= timeout) {
                throw std::runtime_error("Model load timeout");
            }
            
            loading_cv_.wait_for(lock, std::chrono::seconds(1));
        }
    }
}
```

### 5. Model Preloading API

**Priority**: 🟢 Medium  
**Effort**: 2 days

**Implementation Tasks**:
- [ ] Add model preloading hints
- [ ] Support preload on startup
- [ ] Configurable preload list
- [ ] Background preloading (low priority)
- [ ] Preload status monitoring

**Code Example**:
```cpp
void LazyModelLoader::preloadModels(
    const std::vector<std::string>& model_paths,
    bool background = true
) {
    spdlog::info("Preloading {} models", model_paths.size());
    
    std::vector<std::future<CachedModel*>> futures;
    
    for (const auto& path : model_paths) {
        std::string model_id = extractModelId(path);
        json default_config;
        
        if (background) {
            // Low priority background loading
            futures.push_back(preloadModelAsync(
                model_id, 
                path, 
                default_config
            ));
        } else {
            // Immediate loading
            getOrLoadModel(model_id, path, default_config);
        }
    }
    
    // Wait for all if not background
    if (!background) {
        for (auto& future : futures) {
            future.wait();
        }
    }
}
```

## ✅ Acceptance Criteria

### Functional Requirements
- [ ] Async model loading works without blocking
- [ ] Progress callbacks are invoked correctly
- [ ] Concurrent loads are handled properly
- [ ] Model preloading API works
- [ ] All tests pass

### Performance Requirements
- [ ] No blocking during async load
- [ ] Progress reporting overhead < 1%
- [ ] Thread pool overhead < 100ms
- [ ] Concurrent loads correctly queued

### Code Quality Requirements
- [ ] Thread-safe implementation
- [ ] No race conditions
- [ ] Proper resource cleanup
- [ ] Memory leak free (Valgrind clean)

## 🔗 Dependencies

**C++ Standard Library**: `<future>`, `<thread>`, `<async>`  
**Existing Model Loader**: Already implemented

## 📈 Implementation Plan

### Week 1
- [ ] Day 1-2: Thread pool implementation
- [ ] Day 3: Async loading API
- [ ] Day 4: Progress callbacks
- [ ] Day 5: Concurrent request handling

### Week 2 (Optional)
- [ ] Day 1-2: Model preloading API
- [ ] Day 3-4: Tests and benchmarks
- [ ] Day 5: Documentation

## 🔍 Testing Strategy

### Unit Tests
```bash
./build/tests/test_model_loader --gtest_filter="*Async*"
./build/tests/test_model_loader --gtest_filter="*Concurrent*"
```

### Performance Tests
```bash
./build/benchmarks/bench_model_loading --benchmark_filter="async"
```

## 📚 References

- [C++ Async Programming](https://en.cppreference.com/w/cpp/thread/async)
- [Thread Pool Pattern](https://en.wikipedia.org/wiki/Thread_pool)

## 🏁 Definition of Done

- [ ] All implementation tasks complete
- [ ] All acceptance criteria met
- [ ] All tests passing
- [ ] No performance regression
- [ ] Code review completed
- [ ] Documentation updated

## 📝 Notes

**Priority**: MEDIUM - Performance optimization, not critical for v1.2  
**Target Version**: v1.3.0  
**Estimated Completion**: 1-2 weeks with 1 FTE
