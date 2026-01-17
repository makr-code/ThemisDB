---
name: "📦 PagedBlockManager Integration"
about: Connect InferenceEngine to PagedBlockManager for memory optimization (High Priority - P1)
title: "[LLM] Integrate PagedBlockManager with InferenceEngine"
labels: priority:P1, type:feature, area:llm, effort:small, phase:production
assignees: ''

---

## 📋 Beschreibung / Description

**DE**: Aktuell wird keine PagedBlockManager Instanz an den InferenceEngine übergeben. Dies führt zu suboptimalem Memory Management und verhindert volle Nutzung von Paged Attention.

**EN**: Currently no PagedBlockManager instance is passed to InferenceEngine. This leads to suboptimal memory management and prevents full use of paged attention.

**Related Analysis**: `REMAINING_GAPS_SUMMARY.md` §6 (Priority 1)  
**Current Status**: `src/llm/llamacpp_inference_engine.cpp:27` - TODO comment  
**Impact**: ⚠️ **Performance** - Memory management not optimal, long context performance degradation

## 🎯 Ziele / Goals

- [ ] PagedBlockManager Instanz erstellen und konfigurieren
- [ ] InferenceEngine mit PagedBlockManager verbinden
- [ ] Optimale Block Sizes konfigurieren
- [ ] Tests mit langen Kontexten
- [ ] Performance Benchmarks

## 📝 Aufgaben / Tasks

### 1. PagedBlockManager Configuration
**Priorität**: P1 - High

**Current Code** (Line 27):
```cpp
// TODO: Pass actual PagedBlockManager instance
```

**Configuration Design**:
```cpp
// File: include/llm/paged_block_manager.h

struct PagedBlockManagerConfig {
    size_t block_size = 16;              // Tokens per block (typical: 16-32)
    size_t num_blocks = 2048;            // Total blocks (depends on VRAM)
    size_t max_context_length = 4096;    // Maximum context window
    bool enable_sliding_window = false;   // Sliding window attention
    size_t sliding_window_size = 2048;   // Window size if enabled
    
    // Memory management
    float memory_pool_size_gb = 4.0f;    // VRAM pool for KV cache
    bool enable_swapping = false;         // Swap to system RAM if VRAM full
    
    // Performance tuning
    size_t prefill_chunk_size = 512;     // Tokens per prefill chunk
    bool enable_chunked_prefill = true;  // Chunked vs full prefill
};
```

**Implementation**:
```cpp
class PagedBlockManager {
public:
    explicit PagedBlockManager(const PagedBlockManagerConfig& config);
    
    // Block allocation
    BlockHandle allocateBlock();
    void freeBlock(BlockHandle handle);
    
    // Context management
    ContextHandle createContext(size_t max_tokens);
    void releaseContext(ContextHandle handle);
    
    // KV cache management
    void* getBlockPointer(BlockHandle handle);
    size_t getBlockSize() const;
    
    // Statistics
    size_t getUsedBlocks() const;
    size_t getFreeBlocks() const;
    float getMemoryUsageGB() const;
};
```

**Tasks**:
- [ ] Define PagedBlockManagerConfig structure
- [ ] Implement PagedBlockManager class
- [ ] Add block allocation/deallocation
- [ ] Add context management
- [ ] Add memory statistics

---

### 2. InferenceEngine Integration
**Priorität**: P1 - High

**Update InferenceEngine Constructor**:
```cpp
// File: include/llm/llamacpp_inference_engine.h

class LlamaCppInferenceEngine {
public:
    struct Config {
        // Existing config...
        
        // PagedBlockManager configuration
        std::shared_ptr<PagedBlockManager> block_manager;  // Optional, created if null
        PagedBlockManagerConfig block_manager_config;      // Used if block_manager is null
    };
    
    explicit LlamaCppInferenceEngine(const Config& config);
};
```

**Implementation** (`src/llm/llamacpp_inference_engine.cpp`):
```cpp
LlamaCppInferenceEngine::LlamaCppInferenceEngine(const Config& config)
    : config_(config)
{
    // Initialize PagedBlockManager
    if (config_.block_manager) {
        // Use provided instance
        block_manager_ = config_.block_manager;
        spdlog::info("Using provided PagedBlockManager");
    } else {
        // Create new instance with config
        spdlog::info("Creating new PagedBlockManager");
        spdlog::info("  Block size: {}", config_.block_manager_config.block_size);
        spdlog::info("  Num blocks: {}", config_.block_manager_config.num_blocks);
        spdlog::info("  Max context: {}", config_.block_manager_config.max_context_length);
        
        block_manager_ = std::make_shared<PagedBlockManager>(
            config_.block_manager_config
        );
        
        spdlog::info("✓ PagedBlockManager initialized");
    }
    
    // Log memory statistics
    spdlog::info("Memory pool: {:.2f} GB", 
                 block_manager_->getMemoryUsageGB());
}
```

**Tasks**:
- [ ] Add block_manager to InferenceEngine config
- [ ] Initialize block manager in constructor
- [ ] Add automatic configuration based on VRAM
- [ ] Add logging for debugging
- [ ] Handle null case gracefully

---

### 3. Context Window Management
**Priorität**: P1 - High

**Implementation**:
```cpp
bool LlamaCppInferenceEngine::loadModel(
    const std::string& model_path,
    const std::string& model_name
) {
    // ... existing model loading ...
    
    // Configure context with PagedBlockManager
    if (block_manager_) {
        llama_context_params ctx_params = llama_context_default_params();
        
        // Calculate optimal context size based on blocks
        size_t block_size = block_manager_->getBlockSize();
        size_t num_blocks = block_manager_->getFreeBlocks();
        size_t max_context = std::min(
            block_size * num_blocks,
            config_.max_context_length
        );
        
        ctx_params.n_ctx = max_context;
        ctx_params.n_batch = config_.batch_size;
        
        // Enable paged attention if available
        ctx_params.use_paged_attention = true;
        
        spdlog::info("Context configuration:");
        spdlog::info("  Max tokens: {}", max_context);
        spdlog::info("  Batch size: {}", config_.batch_size);
        spdlog::info("  Paged attention: enabled");
        
        context_ = llama_new_context_with_model(model_, ctx_params);
        
        // Create context handle in block manager
        context_handle_ = block_manager_->createContext(max_context);
    }
    
    return true;
}
```

**Tasks**:
- [ ] Calculate optimal context size from available blocks
- [ ] Configure llama.cpp with paged attention
- [ ] Create context handle in block manager
- [ ] Add error handling for insufficient memory
- [ ] Log configuration details

---

### 4. Inference with Block Management
**Priorität**: P1 - High

**Implementation**:
```cpp
std::string LlamaCppInferenceEngine::generate(
    const std::string& prompt,
    const GenerationParams& params
) {
    if (!block_manager_) {
        spdlog::warn("No block manager, using default memory management");
        return generateDefault(prompt, params);
    }
    
    // Tokenize prompt
    auto tokens = tokenize(prompt);
    
    // Check if context fits in available blocks
    size_t required_blocks = (tokens.size() + params.max_tokens) / 
                             block_manager_->getBlockSize() + 1;
    
    if (required_blocks > block_manager_->getFreeBlocks()) {
        spdlog::warn("Insufficient blocks: need {}, have {}", 
                     required_blocks, block_manager_->getFreeBlocks());
        
        // Try to free some blocks (eviction policy)
        if (!block_manager_->freeOldestBlocks(required_blocks)) {
            throw std::runtime_error("Out of memory for KV cache");
        }
    }
    
    // Allocate blocks for this generation
    std::vector<BlockHandle> allocated_blocks;
    for (size_t i = 0; i < required_blocks; ++i) {
        auto handle = block_manager_->allocateBlock();
        allocated_blocks.push_back(handle);
    }
    
    // Generate with paged KV cache
    std::string output = generateWithPagedKV(tokens, params, allocated_blocks);
    
    // Free blocks after generation (or cache for reuse)
    if (!params.cache_kv) {
        for (auto handle : allocated_blocks) {
            block_manager_->freeBlock(handle);
        }
    }
    
    return output;
}
```

**Tasks**:
- [ ] Check block availability before generation
- [ ] Allocate blocks for KV cache
- [ ] Handle out-of-memory gracefully
- [ ] Implement block eviction policy
- [ ] Free blocks after generation
- [ ] Support KV cache reuse

---

### 5. Testing and Benchmarks
**Priorität**: P1 - High

**Test Cases**:
```cpp
// Test file: tests/test_paged_block_manager.cpp

TEST(PagedBlockManagerTest, BasicAllocation) {
    PagedBlockManagerConfig config;
    config.num_blocks = 100;
    config.block_size = 16;
    
    PagedBlockManager mgr(config);
    
    auto handle = mgr.allocateBlock();
    EXPECT_TRUE(handle.valid());
    EXPECT_EQ(mgr.getUsedBlocks(), 1);
    EXPECT_EQ(mgr.getFreeBlocks(), 99);
    
    mgr.freeBlock(handle);
    EXPECT_EQ(mgr.getFreeBlocks(), 100);
}

TEST(PagedBlockManagerTest, OutOfMemory) {
    PagedBlockManagerConfig config;
    config.num_blocks = 10;
    
    PagedBlockManager mgr(config);
    
    // Allocate all blocks
    std::vector<BlockHandle> handles;
    for (int i = 0; i < 10; ++i) {
        handles.push_back(mgr.allocateBlock());
    }
    
    // Should fail on 11th allocation
    EXPECT_THROW(mgr.allocateBlock(), std::runtime_error);
}

TEST(PagedBlockManagerTest, LongContextGeneration) {
    // Load model with PagedBlockManager
    LlamaCppInferenceEngine::Config config;
    config.block_manager_config.num_blocks = 2048;
    config.block_manager_config.block_size = 32;
    
    LlamaCppInferenceEngine engine(config);
    engine.loadModel("models/llama-2-7b.gguf", "llama-2");
    
    // Generate with long context (16K tokens)
    std::string long_prompt(50000, 'a');  // ~10K tokens
    auto output = engine.generate(long_prompt, {.max_tokens = 100});
    
    EXPECT_FALSE(output.empty());
}

TEST(PagedBlockManagerTest, MemoryReuse) {
    // Generate multiple times
    // Verify blocks are reused (KV cache)
    // Second generation should be faster
}
```

**Benchmarks**:
```cpp
void benchmarkLongContext() {
    LlamaCppInferenceEngine engine(config);
    
    // Test with different context lengths
    for (size_t ctx_len : {1024, 2048, 4096, 8192, 16384}) {
        std::string prompt = generatePrompt(ctx_len);
        
        auto start = std::chrono::high_resolution_clock::now();
        auto output = engine.generate(prompt, {.max_tokens = 100});
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end - start
        ).count();
        
        spdlog::info("Context length: {}, Time: {}ms", ctx_len, duration);
    }
}
```

**Tasks**:
- [ ] Create comprehensive test suite
- [ ] Test block allocation/deallocation
- [ ] Test out-of-memory handling
- [ ] Test long context generation (8K, 16K, 32K tokens)
- [ ] Benchmark performance vs no paging
- [ ] Profile memory usage

---

## ✅ Akzeptanzkriterien / Acceptance Criteria

- [ ] PagedBlockManager implemented and tested
- [ ] InferenceEngine integrated with block manager
- [ ] Optimal block sizes configured
- [ ] Long context generation works (up to 32K tokens)
- [ ] Memory usage optimized (efficient VRAM use)
- [ ] KV cache reuse implemented
- [ ] Out-of-memory handled gracefully
- [ ] Comprehensive tests pass (>90% coverage)
- [ ] Performance improved vs baseline (measure with benchmarks)
- [ ] Documentation updated

## 📊 Effort Estimation

- **Aufwand / Effort**: 3-5 days (Small-Medium)
- **Komplexität / Complexity**: Medium
- **Risiko / Risk**: Low-Medium

## 🔗 Related Issues

- Issue #01: LLM Infrastructure
- Issue #26: Async Model Loading
- Original analysis: `REMAINING_GAPS_SUMMARY.md` §6

## 📚 References

- Code location: `src/llm/llamacpp_inference_engine.cpp:27`
- PagedBlockManager: `include/llm/paged_block_manager.h`
- llama.cpp paged attention: https://github.com/ggerganov/llama.cpp/pull/2762
- vLLM paper: https://arxiv.org/abs/2309.06180

---

**Priority**: P1 - High priority for long context performance  
**Impact**: Memory optimization, long context support  
**Status**: Ready to implement
