# Layered Retrieval Orchestrator - Architecture & Integration Guide

**Version:** 1.0.0  
**Status:** Production Ready  
**Date:** 2026-07-01

## Table of Contents

1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Layer Details](#layer-details)
4. [Integration Guide](#integration-guide)
5. [Error Handling](#error-handling)
6. [Performance Characteristics](#performance-characteristics)
7. [API Reference](#api-reference)

---

## Overview

The **LayeredRetrievalOrchestrator** is the master controller for ThemisDB's hybrid knowledge retrieval architecture. It orchestrates the complete retrieval pipeline across four specialized layers:

1. **ANN Frontdoor** - Fast semantic candidate retrieval
2. **Tensor Mid-Layer** - Compression and routing refinement
3. **Graph Truth Layer** - Exact validation and evidence assembly
4. **LLM/LoRA Final Layer** - Grounded generation with domain adaptation

### Key Principles

- **Separation of Concerns**: Each layer handles specific responsibilities
- **Fail-Graceful**: Graceful degradation when layers fail
- **Observable**: Complete tracing and correlation through all layers
- **Configurable**: Enable/disable layers independently
- **Thread-Safe**: Safe concurrent access for read operations

---

## Architecture

### Data Flow

```
Query Vector + Query Text
    ↓
[ANN Frontdoor]
├─ Input: embedding vector, query context
├─ Process: semantic similarity search across multiple backends
└─ Output: ranked candidate list with similarity scores
    ↓
[Tensor Mid-Layer]
├─ Input: ANN candidates, routing context
├─ Process: compression, redundancy reduction, routing decisions
└─ Output: refined candidate summaries with routing metadata
    ↓
[Graph Truth Layer]
├─ Input: tensor summaries, graph context
├─ Process: exact relation validation, evidence assembly, ACL/policy checks
└─ Output: validated evidence bundle with complete provenance
    ↓
[LLM/LoRA Final Layer]
├─ Input: validated evidence, query context
├─ Process: package resolution, domain adaptation, generation
└─ Output: grounded answer with provenance metadata
    ↓
Final Result
├─ success: bool
├─ final_answer: string
├─ evidence_bundle: validated evidence list
├─ provenance_trail: complete chain of custody
└─ layer_decisions: per-layer routing metadata
```

### Conceptual Sequence

```
User Query
    ↓
Embedding Generation
    ↓
LayeredRetrievalOrchestrator::execute()
    ├─ executeAnnLayer()
    │  └─ search() → AnnFrontdoorResult
    ├─ executeTensorLayer()
    │  ├─ plan() → TensorLayerPlan
    │  └─ summarize() → TensorLayerSummary
    ├─ executeGraphLayer()
    │  ├─ validateInputContract()
    │  └─ validate() → GraphTruthValidationResult
    └─ executeLlmLayer()
       └─ resolve() → FinalLayerResolution
    ↓
LayeredRetrievalResult
    ├─ Evidence with provenance
    ├─ Layer-by-layer decision metadata
    └─ Final answer with confidence
    ↓
Return to User
```

---

## Layer Details

### Layer 1: ANN Frontdoor

**Responsibility**: Fast approximate nearest-neighbor retrieval

**Input**:
- Query embedding (float array)
- Query dimensionality
- ANN context (scope, dataset size, hot/cold tier flag)

**Output**:
- Ranked list of candidate IDs
- Per-candidate distance scores
- Routing strategy metadata
- Confidence information

**Key Features**:
- Multi-backend support (HNSW, ScaNN, DiskANN, Distributed)
- Hot/cold tiering awareness
- Scope-based routing
- Fallback to brute-force when necessary

**Configuration**:
```cpp
LayeredRetrievalConfig config;
config.ann_k = 100;  // Number of candidates to retrieve
config.enable_ann_layer = true;
```

### Layer 2: Tensor Mid-Layer

**Responsibility**: Compression, routing, and candidate refinement

**Input**:
- ANN candidates
- Tensor context (tenant, domain, base model, shard info)

**Output**:
- Refined candidate summaries
- Routing decision with reason codes
- Tensor-space fingerprints
- Federated shard summaries (if applicable)

**Key Features**:
- TT-SVD, Quantization, Sampling, Hashing compression strategies
- Quality-based, shard-aware, and adaptive routing
- Redundancy detection and elimination
- Cross-shard federated summaries

**Configuration**:
```cpp
LayeredRetrievalConfig config;
config.tensor_top_k = 50;  // Refined candidate count
config.enable_tensor_layer = true;
```

### Layer 3: Graph Truth Layer

**Responsibility**: Exact validation and evidence assembly

**Input**:
- Tensor summaries
- Query text for context
- Graph validation configuration

**Output**:
- Validated evidence bundle (only candidates passing validation)
- Complete provenance records
- ACL/policy validation results
- Multi-hop relationship evidence

**Key Features**:
- Exact semantic relation validation
- Input contract validation (fail-fast on malformed input)
- ACL/policy enforcement (fail-closed on policy violation)
- Multi-hop evidence discovery
- Complete chain-of-custody provenance tracking

**Configuration**:
```cpp
LayeredRetrievalConfig config;
config.graph_top_k = 20;  // Maximum validated evidence count
config.enable_graph_layer = true;
config.fail_closed_on_graph_error = true;  // Reject on validation error
```

### Layer 4: LLM/LoRA Final Layer

**Responsibility**: Grounded generation with domain adaptation

**Input**:
- Validated evidence bundle
- Query text
- LLM package and adapter request

**Output**:
- Generated answer grounded in evidence
- Package resolution metadata
- Model-switch compatibility information
- Adapter selection details

**Key Features**:
- Package-oriented lifecycle management
- Compatibility gate for model switching
- Draft adapter discovery for speculative decoding
- Rollback capability to known-good packages

**Configuration**:
```cpp
LayeredRetrievalConfig config;
config.enable_llm_layer = true;
config.allow_layer_fallback = true;  // Use fallback template if LLM fails
```

---

## Integration Guide

### Basic Usage

```cpp
#include "search/layered_retrieval_orchestrator.h"

// 1. Create orchestrator
auto orchestrator = std::make_shared<search::LayeredRetrievalOrchestrator>();

// 2. Inject layer implementations
orchestrator->setAnnFrontdoor(ann_frontdoor);
orchestrator->setTensorMidLayer(tensor_layer);
orchestrator->setGraphTruthValidator(graph_validator);
orchestrator->setFinalLayerOrchestrator(final_layer);

// 3. Configure behavior
search::LayeredRetrievalConfig config;
config.ann_k = 100;
config.tensor_top_k = 50;
config.graph_top_k = 20;
orchestrator->setConfig(config);

// 4. Execute retrieval
std::vector<float> query_embedding(128, 0.5f);
search::LayeredRetrievalContext context;
context.query_text = "What is ThemisDB?";
context.correlation_id = "trace-123";
context.tenant_id = "org-42";
context.principal = "user@example.com";

auto result = orchestrator->execute(
    query_embedding.data(),
    query_embedding.size(),
    "What is ThemisDB?",
    context
);

// 5. Process result
if (result.success) {
    std::cout << "Answer: " << result.final_answer << std::endl;
    
    for (const auto& evidence : result.evidence_bundle) {
        std::cout << "  - Source: " << evidence.candidate_id 
                  << " (score: " << evidence.graph_score << ")" << std::endl;
    }
    
    std::cout << "Total latency: " << result.total_latency_ms.count() << "ms" << std::endl;
} else {
    std::cout << "Retrieval failed: " << result.final_answer << std::endl;
}
```

### Health Monitoring

```cpp
// Check orchestrator health
if (orchestrator->isHealthy()) {
    std::cout << "All layers configured and ready" << std::endl;
} else {
    std::cout << "Warning: Some layers missing\n" << orchestrator->statusReport();
}
```

### Selective Layer Enablement

```cpp
// Disable tensor and LLM layers for faster retrieval
search::LayeredRetrievalConfig config = orchestrator->config();
config.enable_tensor_layer = false;
config.enable_llm_layer = false;
config.fail_closed_on_graph_error = false;  // Allow graceful degradation
orchestrator->setConfig(config);

// Execute with reduced pipeline (ANN → Graph)
auto result = orchestrator->execute(query_embedding.data(), 128, query_text);
```

### Error Handling

```cpp
auto result = orchestrator->execute(query_embedding.data(), 128, query_text, context);

// Check layer-by-layer status
for (const auto& decision : result.layer_decisions) {
    std::cout << decision.layer_name << ": ";
    if (decision.success) {
        std::cout << "✓ (latency: " << decision.elapsed_ms.count() << "ms)" << std::endl;
    } else {
        std::cout << "✗ " << decision.routing_reason << std::endl;
        for (const auto& error : decision.errors) {
            std::cout << "  Error: " << error << std::endl;
        }
    }
}

// Provenance chain
for (const auto& prov : result.provenance_trail) {
    std::cout << "Evidence " << prov.evidence_id << " path: ";
    for (const auto& decision : prov.layer_decisions) {
        std::cout << decision << " → ";
    }
    std::cout << std::endl;
}
```

---

## Error Handling

### Layer Failure Modes

| Layer | Failure Mode | Handling |
|-------|--------------|----------|
| ANN | No candidates found | Skip to tensor layer, may result in empty evidence |
| ANN | Backend unavailable | Fall back to brute-force or skip |
| Tensor | Invalid input | Continue with ANN candidates (fallback) |
| Tensor | Compression error | Pass candidates unchanged |
| Graph | Validation failed | Fail-closed if enabled, otherwise use unapproved evidence |
| Graph | ACL violation | Reject evidence (fail-closed) |
| LLM | Package not found | Generate fallback template answer |
| LLM | Model incompatible | Try fallback package or template |

### Fallback Strategy

The orchestrator implements multi-tier fallback:

1. **Layer Fallback**: Skip failing layer, continue with upstream results
2. **Package Fallback**: Try alternate LLM package if primary fails
3. **Adapter Fallback**: Use draft adapter if primary unavailable
4. **Template Fallback**: Generate structured answer from evidence if LLM fails

### Fail-Closed Behavior

When `config.fail_closed_on_graph_error = true`:
- **Graph validation failure** → Reject retrieval entirely
- **ACL policy violation** → Reject evidence (not included in result)
- **Input contract violation** → Escalate error

This ensures strict correctness guarantees for compliance scenarios.

---

## Performance Characteristics

### Latency Profile

Typical end-to-end latency by layer (on moderate hardware):

| Layer | Typical Latency | Notes |
|-------|-----------------|-------|
| ANN | 10-100ms | Depends on backend and dataset size |
| Tensor | 5-50ms | Compression overhead |
| Graph | 20-200ms | Depends on graph complexity and validation rules |
| LLM | 100-5000ms | Depends on generation settings and model size |
| **Total** | **150-5350ms** | Dominated by LLM layer |

### Memory Profile

- **Per-query memory**: ~1-10MB (query vector + intermediate summaries)
- **Layer-specific overhead**: <1MB per layer
- **Evidence storage**: ~100KB per evidence item (with provenance)

### Concurrency

- Multiple threads can call `execute()` concurrently
- Configuration changes must be externally synchronized with execute calls
- No shared mutable state between separate orchestrator instances

---

## API Reference

### LayeredRetrievalConfig

```cpp
struct LayeredRetrievalConfig {
    // Enable/disable layers
    bool enable_ann_layer = true;
    bool enable_tensor_layer = true;
    bool enable_graph_layer = true;
    bool enable_llm_layer = true;
    
    // Layer parameters
    int ann_k = 100;
    int tensor_top_k = 50;
    int graph_top_k = 20;
    
    // Error handling
    bool allow_layer_fallback = true;
    bool fail_closed_on_graph_error = true;
    
    // Resource limits
    size_t max_candidates = 10'000;
    uint32_t timeout_ms = 5'000;
};
```

### LayeredRetrievalContext

```cpp
struct LayeredRetrievalContext {
    std::string query_text;
    std::string correlation_id;      // For tracing
    std::string tenant_id;           // Multi-tenancy
    std::string principal;           // For ACL
    std::string domain;              // Domain specialization
    std::string base_model_id;       // Model selection
    std::string requested_package_id; // LLM package selection
    std::string confidence_policy_version;
    std::string confidence_threshold_key;
    bool shard_aware = false;
    bool trace_enabled = false;
};
```

### LayeredRetrievalResult

```cpp
struct LayeredRetrievalResult {
    // Outcome
    bool success;
    std::string final_answer;
    
    // Decision trail
    std::vector<LayerRoutingDecision> layer_decisions;
    
    // Evidence and provenance
    std::vector<rag::GraphTruthEvidence> evidence_bundle;
    std::vector<rag::ProvenanceRecord> provenance_trail;
    
    // Observability
    std::string correlation_id;
    std::chrono::milliseconds total_latency_ms;
    nlohmann::json debug_trace;
};
```

### Main Methods

```cpp
// Execute complete retrieval pipeline
LayeredRetrievalResult execute(
    const float* query_vector,
    std::size_t query_dim,
    const std::string& query_text,
    const LayeredRetrievalContext& context = {});

// Dependency injection
void setAnnFrontdoor(std::shared_ptr<index::AnnFrontdoor> frontdoor);
void setTensorMidLayer(std::shared_ptr<tensor::TensorMidLayer> layer);
void setGraphTruthValidator(std::shared_ptr<rag::GraphTruthValidator> validator);
void setFinalLayerOrchestrator(std::shared_ptr<llm::FinalLayerOrchestrator> layer);

// Configuration
void setConfig(const LayeredRetrievalConfig& config);
const LayeredRetrievalConfig& config() const noexcept;

// Health and status
bool isHealthy() const noexcept;
std::string statusReport() const;
```

---

## Testing

### Unit Tests

Comprehensive test suite covers:
- Complete happy-path retrieval
- Per-layer success/failure scenarios
- Error handling and fallback logic
- Configuration validation
- Observability and tracing
- Edge cases (null vectors, empty results, etc.)

Run tests:
```bash
ctest -R test_layered_retrieval_orchestrator -V
```

### Integration Tests

Integration tests with real layer implementations verify:
- End-to-end retrieval quality
- Latency characteristics
- Correctness of evidence assembly
- Provenance accuracy

---

## Migration Guide

### From Direct Layer Usage to Orchestrator

**Before** (direct layer usage):
```cpp
auto ann_result = ann_frontdoor->search(query_vec, dim, k);
auto tensor_summary = tensor_layer->summarize(context);
auto graph_result = graph_validator->validate(query, tensor_summary);
// Manual error handling and coordination needed
```

**After** (using orchestrator):
```cpp
auto result = orchestrator->execute(query_vec, dim, query_text, context);
// All layers coordinated automatically with proper error handling
```

### Rollout Strategy

1. **Phase 1**: Deploy orchestrator alongside existing implementations
2. **Phase 2**: Enable orchestrator for new queries (feature flag)
3. **Phase 3**: Gradually shift traffic to orchestrator
4. **Phase 4**: Retire direct layer usage (if applicable)

---

## References

- `include/search/layered_retrieval_orchestrator.h` - Header file
- `src/search/layered_retrieval_orchestrator.cpp` - Implementation
- `tests/search/test_layered_retrieval_orchestrator.cpp` - Test suite
- `TARGET_ARCHITECTURE.md` - Architecture overview
- `DISTRIBUTED_TENSOR_SHARDING.md` - Tensor layer details
