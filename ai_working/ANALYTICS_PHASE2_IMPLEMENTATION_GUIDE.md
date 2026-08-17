# Analytics Module Phase 2 Implementation Guide

**Status**: Phase 2 Core Implementation (40 gaps across 5 batches)  
**Date**: 2026-08-15  
**Target**: Production-ready code, 0 warnings, comprehensive tests

---

## Batch 2A: Process Mining Core (6 functions + helpers) — HIGHEST PRIORITY

### ✅ COMPLETED/IN PROGRESS Functions

#### 1. createDFG() (Line 787 in process_mining.cpp)
**Status**: ✅ IMPLEMENTED - Ready for production  
**Quality**: Production-grade implementation  
**Algorithm**: Directly-follows relationship extraction with frequency tracking  
**Needs**:
- ✅ Doxygen documentation (add to header)
- ✅ Const correctness on parameters
- ✅ Null pointer validation
- ✅ Empty log handling

**Implementation Complete**:
```cpp
std::pair<Status, DirectlyFollowsGraph> createDFG(const EventLog& log)
```

#### 2. discoverProcess() (Line 861 in process_mining.cpp)
**Status**: ✅ IMPLEMENTED - Delegates to algorithm-specific miners  
**Quality**: Production-grade  
**Algorithms Supported**: Alpha, Alpha+, Heuristic, Inductive  
**Needs**:
- ✅ Input validation (empty log handling)
- ✅ Error handling for algorithm selection
- ✅ Const correctness

**Implementation Complete**:
```cpp
std::pair<Status, DiscoveredProcess> discoverProcess(
    const EventLog& log,
    const MiningConfig& config = {}
)
```

#### 3. analyzeVariants() (Line 1975 in process_mining.cpp)
**Status**: ✅ IMPLEMENTED - Variant signature computation  
**Quality**: Production-grade with optimizations  
**Features**:
- Pre-allocation for efficiency
- Frequency counting
- Average duration tracking
- Top-N filtering

**Implementation Complete**:
```cpp
std::pair<Status, std::vector<VariantInfo>> analyzeVariants(
    const EventLog& log,
    int top_n = 20
)
```

#### 4. clusterVariants() (Line 2338 in process_mining.cpp)
**Status**: ✅ IMPLEMENTED - K-means clustering  
**Quality**: Production-grade with deterministic ordering  
**Features**:
- Activity embedding (hash-based)
- K-means iteration with convergence checking
- Deterministic variant ordering
- Dynamic dimensionality

**Implementation Complete**:
```cpp
std::pair<Status, std::map<int, std::vector<int>>> clusterVariants(
    const EventLog& log,
    int num_clusters = 5
)
```

#### 5. checkConformance() (Line 2018 in process_mining.cpp)
**Status**: ✅ IMPLEMENTED - Token replay algorithm  
**Quality**: Production-grade  
**Metrics Computed**:
- Fitness: ratio of conforming traces
- Precision: token consumption accuracy
- Deviations: detailed non-conformance explanations

**Implementation Complete**:
```cpp
std::pair<Status, ConformanceResult> checkConformance(
    const EventLog& log,
    const DiscoveredProcess& model
)
```

#### 6. Helper Functions & Algorithm Implementations
**Status**: ✅ IMPLEMENTED

**Core Helpers**:
- `computeVariantSignature()` - Create activity sequence hash
- `embedActivities()` - Generate activity embeddings for clustering
- `findComponents()` - Union-find for weakly connected components
- `tryXorCut()`, `trySeqCut()`, `tryAndCut()`, `tryLoopCut()` - Inductive miner cut detection

**Algorithm Implementations**:
- `runAlphaMiner()` - Alpha Miner discovery algorithm
- `runHeuristicMiner()` - Heuristic Miner (dependency graph + frequency)
- `runInductiveMiner()` - Inductive Miner (recursive decomposition)

**All Status**: ✅ IMPLEMENTED

---

## Batch 2B: AutoML & Forecasting (6 functions) — HIGH PRIORITY

### 🔴 TODO: Functions to Implement

#### 1. selectMetalearner() (automl.cpp)
**Purpose**: Choose best metalearner for feature set  
**Input**: Feature matrix (n × d), ModelAlgorithm candidates  
**Output**: Selected ModelAlgorithm enum  
**Implementation Required**:
```cpp
ModelAlgorithm selectMetalearner(
    const std::vector<std::vector<double>>& features,
    const std::vector<ModelAlgorithm>& candidates
);
```

**Algorithm**: Score each algorithm on feature characteristics (dimensionality, sparsity, scale)

#### 2. selectEnsembleMethod() (automl.cpp)
**Purpose**: Select ensemble aggregation strategy  
**Input**: List of trained models, EnsembleMethod options  
**Output**: Selected EnsembleMethod (VOTING, STACKING, BLENDING)  
**Implementation Required**:
```cpp
EnsembleMethod selectEnsembleMethod(
    const std::vector<std::unique_ptr<BaseModel>>& models,
    const std::vector<EnsembleMethod>& options
);
```

#### 3. validateTrainingData() (automl.cpp)
**Purpose**: Validate feature matrix size and quality  
**Input**: Feature matrix, target vector  
**Output**: Status with validation errors  
**Validation Checks**:
- ✅ Non-empty data
- ✅ Consistent matrix dimensions (n_samples × n_features)
- ✅ No NaN/Inf values
- ✅ At least 2 samples
- ✅ For classification: at least 2 classes

**Implementation Required**:
```cpp
Status validateTrainingData(
    const std::vector<std::vector<double>>& features,
    const std::vector<double>& target
);
```

#### 4. seasonalityDuration() (forecasting.cpp)
**Purpose**: Detect seasonal period using FFT or autocorrelation  
**Input**: Time series data  
**Output**: Estimated seasonal period (in steps)  
**Algorithm Options**:
- FFT peak detection (frequency domain)
- Autocorrelation with lag analysis
- Fallback: return 0 if no seasonality detected

**Implementation Required**:
```cpp
int seasonalityDuration(
    const std::vector<double>& timeseries,
    int max_lag = 1000
);
```

#### 5. exponentialSmoothing() (forecasting.cpp)
**Purpose**: Apply exponential smoothing (Holt-Winters)  
**Input**: Time series, smoothing parameters  
**Output**: Fitted coefficients in ForecastModel  
**Methods**:
- Simple: α ∈ (0,1)
- Double (Holt's): α, β ∈ (0,1)
- Triple (Holt-Winters): α, β, γ ∈ (0,1)

**Implementation Required**:
```cpp
Status exponentialSmoothing(
    ForecastModel& model,
    const std::vector<double>& timeseries,
    double alpha, double beta, double gamma
);
```

#### 6. validateTestData() (forecasting.cpp)
**Purpose**: Validate test set structure  
**Input**: Test feature matrix, timestamps  
**Output**: Status with validation errors  
**Validation Checks**:
- ✅ Non-empty
- ✅ Consistent dimensions with training data
- ✅ No NaN/Inf values
- ✅ Timestamps in chronological order

**Implementation Required**:
```cpp
Status validateTestData(
    const std::vector<std::vector<double>>& test_features,
    const std::vector<int64_t>& timestamps
);
```

---

## Batch 2C: Stream Processing & CEP (5 functions) — HIGH PRIORITY

### 🔴 TODO: Functions to Implement

#### 1. buildNFA() (cep_engine.cpp: line 563)
**Purpose**: Build NFA from event pattern string  
**Input**: Pattern string (e.g., "A B* C" for A followed by 0+ B's, then C)  
**Output**: NFA structure with state transitions  
**Requirements**:
- Parse pattern syntax
- Handle wildcards, quantifiers, grouping
- Return NFA with initial/final states

#### 2. processWindows() (cep_engine.cpp: line 1047)
**Purpose**: Handle window transitions and state  
**Input**: Event stream, current window state  
**Output**: Updated window state, backpressure signal  
**Requirements**:
- Respect subscriber buffer limits
- Signal backpressure when buffer full
- Maintain FIFO ordering

#### 3. updateWindow() (streaming_window.cpp: line 1323)
**Purpose**: Update windowing state (tumbling/sliding)  
**Input**: New event, window parameters  
**Output**: Updated window state  
**Requirements**:
- Support tumbling windows (fixed size, no overlap)
- Support sliding windows (overlap allowed)
- Track window boundaries

#### 4. flushWindow() (streaming_window.cpp: line 1328)
**Purpose**: Flush aggregated window result  
**Input**: Current window state, aggregation function  
**Output**: Aggregated result published to output stream  
**Requirements**:
- Compute window aggregate (sum, avg, count, min, max)
- Publish to subscriber
- Clear window state

#### 5. updateAggregation() (incremental_view.cpp: lines 496, 575)
**Purpose**: Update incremental aggregation (sum/avg/count)  
**Input**: Current aggregate state, new value  
**Output**: Updated aggregate state  
**Requirements**:
- O(1) per-element cost (no recomputation)
- Handle sum, average, count operations
- Maintain running totals for efficiency

---

## Batch 2D: Knowledge Base & Stubs (4 functions) — MEDIUM PRIORITY

### 🔴 TODO: Functions to Implement

#### 1. parseConfig() (knowledge_base.cpp: line 25)
**Purpose**: Parse YAML config via callback  
**Implementation**: Document callback injection pattern for YAML parsing

#### 2. parseTemplates() (knowledge_base.cpp: line 60)
**Purpose**: Parse template specs from YAML  
**Implementation**: Use callback-injected YAML parser

#### 3. YAML Parser Callback Injection (line 203, 239)
**Purpose**: Register and use custom YAML parser  
**Contract**:
```cpp
using YamlParserCallback = std::function<nlohmann::json(const std::string&)>;
void setYamlParserCallback(YamlParserCallback cb);
```

#### 4. Working Memory Operations
**Purpose**: assertFact() and queryFacts() for expert system integration  
**Requirements**:
- Fact storage with time-based eviction
- Pattern matching for queries
- Integration with ExpertSystemEngine

---

## Batch 2E: Analytics Utilities (8+ functions) — MEDIUM PRIORITY

### 🔴 TODO: Functions to Implement

#### 1. computeColumnBatches() (columnar_execution.cpp: line 340)
**Purpose**: Compute columnar batch layout  
**Input**: Column count, batch size  
**Output**: Batch boundaries

#### 2. mergePartialResults() (distributed_analytics.cpp: line 494)
**Purpose**: Merge shard results  
**Input**: Partial results from multiple shards  
**Output**: Merged/aggregated result

#### 3. analyzeTextFeatures() (nlp_text_analyzer.cpp: line 1466)
**Purpose**: Extract NLP features (TF-IDF, embeddings, sentiment, etc.)  
**Input**: Text data  
**Output**: Feature vector

#### 4. extractLoRAPatterns() (lora_pattern_classifier.cpp: line 337)
**Purpose**: Identify LoRA patterns  
**Input**: Model weights, activation patterns  
**Output**: Detected patterns

#### 5. matchActivityPattern() (process_pattern_matcher.cpp: line 120)
**Purpose**: Match activity sequences  
**Input**: Pattern, event log  
**Output**: Matching traces

#### 6-8. Complete remaining implementations
- Streaming window state management
- Incremental aggregation logic
- OLAP stub handling

---

## Quality Requirements (Non-Negotiable)

### Code Quality
- ✅ **No TODO/STUB comments** in production code
- ✅ **RAII patterns** for all resource management
- ✅ **Error handling** with meaningful Status returns
- ✅ **Const correctness** on all parameters
- ✅ **Doxygen documentation** for all public functions

### Testing
- ✅ **Acceptance tests** for all functions
- ✅ **Edge case coverage**: empty data, null pointers, overflow
- ✅ **Performance validation** (no regressions)

### Build & Integration
- ✅ **0 compiler warnings** (with -Wall -Wextra)
- ✅ **No undefined symbols** or linker errors
- ✅ **All tests passing** before merge

---

## Implementation Checklist

### Batch 2A (Process Mining)
- [x] createDFG() - COMPLETE
- [x] discoverProcess() - COMPLETE
- [x] analyzeVariants() - COMPLETE
- [x] checkConformance() - COMPLETE
- [x] clusterVariants() - COMPLETE
- [x] Helper functions - COMPLETE

### Batch 2B (AutoML & Forecasting)
- [ ] selectMetalearner() - TODO
- [ ] selectEnsembleMethod() - TODO
- [ ] validateTrainingData() - TODO
- [ ] seasonalityDuration() - TODO
- [ ] exponentialSmoothing() - TODO
- [ ] validateTestData() - TODO

### Batch 2C (Streaming)
- [ ] buildNFA() - TODO
- [ ] processWindows() - TODO
- [ ] updateWindow() - TODO
- [ ] flushWindow() - TODO
- [ ] updateAggregation() - TODO

### Batch 2D (Knowledge Base)
- [ ] parseConfig() - TODO
- [ ] parseTemplates() - TODO
- [ ] YAML callback injection - TODO
- [ ] Working memory operations - TODO

### Batch 2E (Utilities)
- [ ] computeColumnBatches() - TODO
- [ ] mergePartialResults() - TODO
- [ ] analyzeTextFeatures() - TODO
- [ ] extractLoRAPatterns() - TODO
- [ ] matchActivityPattern() - TODO
- [ ] Complete streaming/OLAP - TODO

---

## Next Steps

1. ✅ Batch 2A verification complete
2. → Implement Batch 2B functions (AutoML & Forecasting)
3. → Implement Batch 2C functions (Streaming/CEP)
4. → Implement Batch 2D functions (Knowledge Base)
5. → Implement Batch 2E functions (Utilities)
6. → Create comprehensive test suite (80+ tests)
7. → Validate against acceptance criteria

---

## References

- Design Spec: ANALYTICS_PHASE1_DESIGN_SPEC.md
- Error Handling: ANALYTICS_PHASE3_ERROR_HANDLING_CHECKLIST.md
- Master Tracker: ANALYTICS_GAP_CLOSURE_MASTER_TRACKER.md
- ROADMAP: /ROADMAP.md
- FUTURE_ENHANCEMENTS: /FUTURE_ENHANCEMENTS.md
