---
name: "🔧 Simulation Code Removal"
about: Entfernung aller Sleep-Calls und Simulation-Code (Hoch - P1)
title: "[Cleanup] Remove Simulation Code and Replace with Real Implementations"
labels: priority:P1, type:refactoring, area:llm, effort:medium, phase:production
assignees: ''

---

## 📋 Beschreibung / Description

**DE**: Entfernung aller Simulation-Codes (sleep calls, stub responses, dummy embeddings) und Ersatz durch echte Implementierungen. Dies ist notwendig für korrekte Performance-Metriken und echte Produktionsvalidierung.

**EN**: Remove all simulation code (sleep calls, stub responses, dummy embeddings) and replace with real implementations. This is necessary for accurate performance metrics and real production validation.

**Related Analysis**: `INVESTIGATION_GAPS_SIMULATIONS_THEMISDB.md` §2.3, §5  
**Impact**: ⚠️ **Performance-Problem** - Metriken reflektieren nicht die Realität  
**Current Status**: 9 sleep() calls, 12 stub implementations, 50+ TODO comments

## 🎯 Ziele / Goals

- [ ] Alle sleep() calls entfernen (9 Stellen)
- [ ] Stub Implementations ersetzen (12 Stellen)
- [ ] Dummy Embeddings durch echte ersetzen
- [ ] Production Validator Tests implementieren (30+ TODOs)
- [ ] Accurate Performance Metrics

## 📝 Aufgaben / Tasks

### 1. Remove Sleep Calls (9 Stellen)
**Priorität**: P1 - Hoch

#### File: `production_validator.cpp`

**Location 1** (Lines 62):
```cpp
// OLD - Simulation
std::this_thread::sleep_for(std::chrono::milliseconds(50 + (i % 10) * 10));

// NEW - Real LLM inference
auto response = llm_plugin_->generate(prompt, generation_config);
```

**Location 2** (Line 314):
```cpp
// OLD - Artificial rate limiting
std::this_thread::sleep_for(std::chrono::milliseconds(10));

// NEW - Remove entirely (use actual request throttling if needed)
// Or implement proper rate limiter based on tokens/sec
```

---

#### File: `inference_engine_enhanced.cpp` (Line 456)

```cpp
// OLD - Worker thread polling with sleep
std::this_thread::sleep_for(std::chrono::milliseconds(100));

// NEW - Use condition variable for event-driven processing
std::unique_lock<std::mutex> lock(queue_mutex_);
queue_cv_.wait_for(lock, std::chrono::milliseconds(100), 
                   [this] { return !request_queue_.empty() || stop_flag_; });
```

---

#### File: `async_inference_engine.cpp` (Line 251)

```cpp
// OLD - RAG context encoding simulation
std::this_thread::sleep_for(std::chrono::milliseconds(100));

// NEW - Actual RAG context processing
auto rag_context = rag_retriever_->retrieve(query, top_k);
request.context = formatRAGContext(rag_context);
```

---

#### File: `lora_training_service.cpp` (Line 157)

```cpp
// OLD - Training loop rate limiting
std::this_thread::sleep_for(std::chrono::milliseconds(1));

// NEW - Remove (let GPU/CPU work at full speed)
// Or use actual batch processing throttle based on memory
```

---

#### File: `themis_help_lora.cpp` (Lines 341, 434)

```cpp
// OLD - Training simulation
std::this_thread::sleep_for(std::chrono::seconds(1));

// NEW - Actual LoRa training
auto result = lora_trainer_->train(training_config);
```

---

### 2. Replace Stub Implementations (12 Stellen)
**Priorität**: P1 - Hoch

#### File: `lora_orchestrator.cpp` (Entire file)

**Current**: "Minimal stubs to allow compilation"

**Replace with**:
- [ ] Real adapter listing from storage
- [ ] Adapter lifecycle management
- [ ] Service-level orchestration

---

#### File: `llama_wrapper.cpp` (Lines 474-483, 1664-1667)

**Current**:
```cpp
if (model_ == nullptr || context_ == nullptr) {
    spdlog::warn("Using stub response");
    std::string output = "[Generated response placeholder for: " + request.prompt + "]";
    return output;
}
```

**Replace with**:
- [ ] Proper error handling (throw exception, not stub response)
- [ ] Initialize model/context before inference
- [ ] Never return placeholder responses

```cpp
if (model_ == nullptr || context_ == nullptr) {
    throw std::runtime_error("Model/context not initialized. Call loadModel() first.");
}
```

---

### 3. Replace Dummy Embeddings (3 Stellen)
**Priorität**: P1 - Hoch

#### File: `inference_engine_enhanced.cpp`

**Location 1** (Line 252):
```cpp
// OLD - Dummy embeddings
std::vector<float> dummy_embedding(128, 0.0f);

// NEW - Real embeddings from model
auto embedding = embedding_model_->encode(text);
```

**Location 2** (Line 667):
```cpp
// OLD - Dummy embedding for similarity search
std::vector<float> embedding(128, 0.0f);

// NEW - Real embedding computation
auto embedding = computeEmbedding(text, embedding_model_);
```

**Location 3** (Line 699):
```cpp
// OLD - Dummy embeddings and KV cache
// TODO: In production, compute actual embeddings and KV cache

// NEW - Real implementation
auto embeddings = precomputeEmbeddings(frequent_prompts_);
kv_cache_ = buildKVCache(embeddings);
```

**Requirements**:
- Use llama.cpp embedding model (e.g., `all-MiniLM-L6-v2`)
- Support multiple embedding dimensions (128, 384, 768)
- Cache embeddings for frequently used texts

---

### 4. Implement Production Validator Tests (30+ TODOs)
**Priorität**: P1 - Hoch

#### Component Tests (8 TODOs)

**File**: `production_validator.cpp`

```cpp
// Lines 408-475 - All return true without testing

// Implement actual tests:
bool ProductionValidator::testModelLoading() {
    try {
        auto model_id = "test_model_7b";
        auto loaded = llm_plugin_->loadModel(model_id);
        return loaded && llm_plugin_->isModelLoaded(model_id);
    } catch (const std::exception& e) {
        spdlog::error("Model loading test failed: {}", e.what());
        return false;
    }
}

bool ProductionValidator::testInferencePipeline() {
    try {
        auto response = llm_plugin_->generate("Test prompt", config_);
        return !response.empty() && response != "[placeholder]";
    } catch (const std::exception& e) {
        spdlog::error("Inference test failed: {}", e.what());
        return false;
    }
}

bool ProductionValidator::testGPUOffload() {
    if (!gpu_available_) {
        spdlog::info("GPU not available, skipping GPU test");
        return true;  // Pass if GPU not available
    }
    
    try {
        auto gpu_layers = llm_plugin_->getGPULayers();
        return gpu_layers > 0;
    } catch (const std::exception& e) {
        spdlog::error("GPU test failed: {}", e.what());
        return false;
    }
}

// Implement remaining 5 tests similarly...
```

---

#### Integration Tests (14 TODOs)

**File**: `production_validator.cpp` (Lines 651-733)

```cpp
// Replace placeholder implementations

bool IntegrationTestSuite::testLazyLoaderWithGPUMemory() {
    // Actual test: Load model with GPU memory management
    auto model_loader = createLazyModelLoader();
    auto gpu_manager = createGPUMemoryManager();
    
    auto model = model_loader->load("test_model", gpu_manager);
    
    // Verify:
    // - Model loaded successfully
    // - GPU memory allocated
    // - Memory stats updated
    
    return model != nullptr && 
           gpu_manager->getAllocatedMemory() > 0;
}

// Implement remaining 13 tests...
```

---

### 5. Replace Placeholder Values
**Priorität**: P2 - Mittel

#### File: `production_validator.cpp` (Line 380)

```cpp
// OLD
result.throughput_tokens_per_sec = 1200.0;  // Placeholder

// NEW - Calculate from actual measurements
result.throughput_tokens_per_sec = 
    static_cast<double>(total_tokens_processed_) / total_time_seconds;
```

#### File: `production_validator.cpp` (Line 355)

```cpp
// OLD
result.uptime_pct = 99.9;  // TODO: Calculate actual uptime

// NEW - Calculate from measurements
auto uptime = std::chrono::duration_cast<std::chrono::seconds>(
    std::chrono::steady_clock::now() - test_start_time_
);
result.uptime_pct = (uptime.count() - downtime_seconds_) * 100.0 / uptime.count();
```

---

### 6. Simulate Quality Test → Real Quality Test
**Priorität**: P2 - Mittel

#### File: `production_validator.cpp` (Lines 883-893)

**Current**: `simulateQualityTest()` - Hardcoded 85% pass rate

**Replace with**:
```cpp
bool ProductionValidator::executeQualityTest(const QualityTest& test) {
    // 1. Run actual LLM inference
    auto response = llm_plugin_->generate(test.prompt, generation_config_);
    
    // 2. Check if response matches expected answers
    std::string response_lower = toLowerCase(response);
    for (const auto& expected : test.expected_answers) {
        if (response_lower.find(toLowerCase(expected)) != std::string::npos) {
            return true;  // Found expected answer
        }
    }
    
    // 3. Use fuzzy matching for partial credit
    for (const auto& expected : test.expected_answers) {
        if (fuzzyMatch(response, expected) > 0.8) {  // 80% similarity
            return true;
        }
    }
    
    spdlog::debug("Quality test failed. Expected: {}, Got: {}", 
                 test.expected_answers[0], response);
    return false;
}
```

---

### 7. Fix Production Validator Benchmark
**Priorität**: P1 - Hoch

#### File: `production_validator.cpp` (Lines 60-88)

**Current**: Uses sleep() to simulate inference

**Replace with**:
```cpp
// Real benchmark
for (int i = 0; i < 100; i++) {
    std::string prompt = generateBenchmarkPrompt(i % 10);
    
    auto req_start = std::chrono::high_resolution_clock::now();
    
    // REAL inference (not sleep!)
    try {
        GenerationConfig gen_config;
        gen_config.max_tokens = 50;  // Limit tokens for speed
        gen_config.temperature = 0.7;
        
        auto response = llm_plugin_->generate(prompt, gen_config);
        
        // Count actual tokens generated
        size_t tokens_generated = countTokens(response);
        total_tokens += tokens_generated;
        successful++;
        
    } catch (const std::exception& e) {
        spdlog::warn("Benchmark request {} failed: {}", i, e.what());
        failed++;
    }
    
    auto req_end = std::chrono::high_resolution_clock::now();
    double latency = std::chrono::duration<double, std::milli>(req_end - req_start).count();
    latencies.push_back(latency);
    
    // Track peak memory
    size_t current_memory = measureMemoryUsage();
    peak_memory_mb = std::max(peak_memory_mb, current_memory);
}
```

---

### 8. Testing & Validation
**Priorität**: P1 - Hoch

- [ ] Verify all sleep() calls removed
- [ ] Verify all stub implementations replaced
- [ ] Verify dummy embeddings replaced
- [ ] Run production validator with real tests
- [ ] Measure actual performance (no simulation)
- [ ] Compare performance before/after

**Test Approach**:
1. Run benchmarks with simulation code (baseline)
2. Remove simulation code
3. Run benchmarks with real code
4. Compare results (should be different/realistic)

---

## ✅ Akzeptanzkriterien / Acceptance Criteria

### Code Quality
- [ ] Zero sleep() calls in production code
- [ ] Zero stub responses that mask errors
- [ ] Zero dummy/placeholder embeddings
- [ ] All production validator tests implemented
- [ ] grep -r "sleep_for\|placeholder\|dummy\|stub\|TODO.*simulation" returns 0 matches

### Functional Requirements
- [ ] Production validator runs real tests
- [ ] Performance metrics reflect reality
- [ ] Quality tests use actual LLM responses
- [ ] All tests pass with real implementations

### Performance
- [ ] Benchmark latency reflects real inference time
- [ ] Throughput calculated from actual token generation
- [ ] No artificial delays in code paths

---

## 📊 Aufwand / Effort

**Geschätzte Zeit**: 2 Wochen (10 Arbeitstage)

**Breakdown**:
- Remove sleep() calls: 1 Tag
- Replace stub implementations: 2 Tage
- Real embeddings implementation: 2 Tage
- Production validator tests: 4 Tage
- Testing & validation: 2 Tage
- Documentation: 1 Tag

**Complexity**: Mittel - Viele kleine Änderungen

---

## 🏁 Definition of Done

- [ ] All sleep() calls removed
- [ ] All stub implementations replaced
- [ ] Real embeddings implemented
- [ ] All production validator tests working
- [ ] Performance metrics accurate
- [ ] All tests passing
- [ ] Code review approved
- [ ] Documentation updated

---

**Priority**: 🟠 **P1 - HIGH PRIORITY**  
**Impact**: Enables accurate performance measurement and validation  
**Timeline**: 2 weeks  
**Dependencies**: LLM Plugin must be functional

---

**Erstellt**: 15. Januar 2026  
**Status**: 🚧 Ready for Implementation
