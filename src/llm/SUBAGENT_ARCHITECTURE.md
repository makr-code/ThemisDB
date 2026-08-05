# LLM Subagent Orchestration - Architecture & Deployment Guide

## Overview

ThemisDB's **LLM Subagent Orchestration Layer** enables independent, parallel LLM inference with shared database access. Each subagent runs isolated inference workloads (different models, LoRA adapters, policies) while sharing common infrastructure (caches, quotas, storage).

**Target Use Cases:**
- Multi-agent reasoning (ask → analyze → summarize)
- A/B testing inference results (model A vs model B)
- Consensus decision-making (majority vote across inference results)
- Parallel task decomposition (split task, solve independently, merge)
- Diversified inference (speculative decoding, ensemble methods)

---

## Architecture Layers

### 1. Subagent Configuration Layer (`subagent_config.h`)

Defines isolated configuration per subagent:

```
SubagentConfig
├── id: "assistant_1"
├── model_id: "mistral-7b"
├── lora_adapter_id: "customer-support" (optional)
├── isolation_level: STRICT
├── budget: TokensPerMinute, Timeout, Concurrency
├── policy: PromptPolicy ID, Ethics Profile ID
├── gpu: MultiGPUStrategy, Quantization
└── observability: AuditLogging, Metrics, Tracing
```

**Key Properties:**
- Each subagent independently configured
- Independent quota, policy, and resource constraints
- Non-breaking: existing callers unaffected
- Opt-in: existing infrastructure unchanged

### 2. Subagent Factory (`subagent_factory.h` + `subagent_factory_impl.cpp`)

Lifecycle management factory:

```
SubagentFactory
├── createSubagent(config) → Subagent
├── destroySubagent(id) → void
├── getSubagent(id) → Subagent
├── listSubagents() → [id, ...]
├── validateConfig(config) → [errors]
├── registerPromptPolicy(id, policy) → void
└── getFactoryStats() → {total_created, active, ...}
```

**Responsibilities:**
- Configuration validation (model, adapter, quota, policy)
- Resource allocation (VRAM, quota buckets, thread slots)
- Lifecycle state machine (CREATED → LOADING → READY → UNLOADING → TERMINATED)
- Subagent registry (discovery, inspection)
- Policy and quota management

**Dependencies:**
- Shared ILLMPlugin (inference backend)
- Shared SharedWorkerPool (async execution)
- Shared ModelLoader (model caching)
- Shared MultiLoRAManager (LoRA lifecycle)
- Shared TokenQuotaManager (per-subagent quotas)

### 3. Subagent Instance (`subagent.h` + embedded in factory)

Individual inference entity:

```
Subagent
├── Lifecycle: load(), warm(), unload(), pause(), resume()
├── Inference: infer(), inferAsync(), inferBatch(), inferStream()
├── Observability: getMetrics(), getState(), getLastError()
└── Quota: checkQuota(), consumeQuota(), resetQuota()
```

**Lifecycle States:**
```
CREATED → LOADING → READY → (PAUSED ↔ READY) → UNLOADING → TERMINATED
            ↓                                               ↓
            └─────────────────── ERROR ←──────────────────┘
```

**Key Guarantees:**
- Each subagent has independent configuration and quota
- Failures in one subagent do not affect others
- Thread-safe for concurrent inference from multiple threads
- Shared database and cache access (read-only, concurrent-safe)

### 4. Subagent Coordinator (`subagent_coordinator.h` + `subagent_coordinator_impl.cpp`)

Orchestrates parallel inference across multiple subagents:

```
SubagentCoordinator
├── inferMultiple(subagent_ids, request, config) → AggregateResult
├── inferMultipleBatch(subagent_ids, requests, config) → [AggregateResult]
├── getLastDiagnostics() → CoordinationDiagnostics
└── getStats() → CoordinatorStats
```

**Fan-Out/Fan-In Pattern:**
1. **Fan-out**: Submit request to all subagents asynchronously
2. **Collect**: Wait for results with timeout (parallel)
3. **Merge**: Apply merge strategy to aggregate results
4. **Return**: Final result + per-subagent diagnostics

**Merge Strategies:**
- **FIRST_WIN**: Return first successful result (lowest latency)
- **ALL_SUCCEED**: All subagents must succeed
- **MAJORITY_VOTE**: Aggregate via consensus (requires structured output)
- **BEST_SCORE**: Return result with highest quality score
- **ENSEMBLE**: Combine results from all subagents
- **CUSTOM**: User-provided merge function

**Error Handling:**
- Partial failures: some subagents succeed, others fail
- Timeout handling: per-subagent and overall deadline
- Backpressure: configurable fail-on-any-error policy
- Detailed diagnostics: per-subagent logs, latencies, merge errors

---

## Resource Isolation Guarantees

### Per-Subagent Quota Management

Each subagent has independent token quotas enforced by `TokenQuotaManager`:

```cpp
// Factory level: shared quota manager
auto quota_manager = std::make_shared<TokenQuotaManager>();

// Per subagent: independent bucket
quota_manager->setQuota(tenant_id, model_id, max_tokens_per_minute);

// At inference time: check before submission
auto result = quota_manager->check(tenant_id, model_id, estimated_tokens);
if (!result.allowed) {
    // Request blocked by quota
}

// After inference: consume actual tokens
quota_manager->consume(tenant_id, model_id, actual_tokens_consumed);
```

### Per-Subagent Policy Enforcement

Each subagent can have independent prompt policies:

```cpp
SubagentConfig config;
config.policy.prompt_policy_id = "no-jailbreak";
config.policy.ethics_profile_id = "constitution-ai";
config.policy.block_on_policy_violation = true;
```

Policies are applied before inference submission and block violating requests.

### Per-Subagent Metrics

Each subagent tracks independent metrics:

```cpp
struct SubagentMetrics {
    uint64_t total_requests;           // Subagent-specific
    uint64_t successful_inferences;    // Subagent-specific
    uint64_t failed_inferences;        // Subagent-specific
    size_t tokens_consumed;            // Current window
    uint64_t vram_used_bytes;          // VRAM tracking
    // ... other fields
};

auto metrics = subagent->getMetrics();
```

### Isolation Levels

Configured per subagent:

- **NONE**: No enforcement (not recommended for production)
- **ADVISORY**: Policy/quota violations logged but not enforced
- **STRICT**: Policy/quota violations block requests
- **STRICT_WITH_PREEMPTION**: Strict + resource preemption (experimental)

---

## Deployment Patterns

### Pattern 1: Multi-Agent Reasoning

Split task across specialized agents, collect results:

```cpp
// Create factory with shared infrastructure
auto factory = SubagentFactory::create(plugin, worker_pool, model_loader, ...);

// Create specialized agents
SubagentConfig asker_config;
asker_config.id = "asker";
asker_config.model_id = "mistral-7b";
factory->createSubagent(asker_config);

SubagentConfig analyzer_config;
analyzer_config.id = "analyzer";
analyzer_config.model_id = "llama2-13b";
factory->createSubagent(analyzer_config);

// Create coordinator for parallel inference
auto coordinator = SubagentCoordinator::create(factory);

// Submit to all agents
InferenceRequest req;
req.prompt = "Analyze this document...";

SubagentCoordinatorConfig coord_config;
coord_config.strategy = SubagentMergeStrategy::ENSEMBLE;

auto result = coordinator->inferMultiple(
    {"asker", "analyzer"},
    req,
    coord_config
);

// Merged result contains outputs from both agents
std::cout << result.merged_output << std::endl;
```

### Pattern 2: A/B Testing

Compare results from two model configurations:

```cpp
SubagentConfig model_a;
model_a.id = "model_a";
model_a.model_id = "mistral-7b";
model_a.lora_adapter_id = "v1.0";

SubagentConfig model_b;
model_b.id = "model_b";
model_b.model_id = "mistral-7b";
model_b.lora_adapter_id = "v2.0";

factory->createSubagent(model_a);
factory->createSubagent(model_b);

// Coordinator with best-score merge
SubagentCoordinatorConfig config;
config.strategy = SubagentMergeStrategy::BEST_SCORE;
config.timeout_ms = 5000;

auto result = coordinator->inferMultiple({"model_a", "model_b"}, req, config);
// Returns result with highest quality_score
```

### Pattern 3: Consensus Decision-Making

Majority vote across agents:

```cpp
SubagentCoordinatorConfig config;
config.strategy = SubagentMergeStrategy::MAJORITY_VOTE;
config.custom_merge_fn = [](const auto& results) {
    // Custom logic: count votes, return majority decision
    // (implementation depends on output format)
};

auto result = coordinator->inferMultiple(
    {"analyst_1", "analyst_2", "analyst_3"},
    req,
    config
);
```

### Pattern 4: Fallback Chain

Speculative execution with fallback:

```cpp
SubagentCoordinatorConfig config;
config.strategy = SubagentMergeStrategy::FIRST_WIN;
config.timeout_ms = 3000;  // Fast timeout

// Will use first agent to return successfully
// If first_choice fails, will use next_choice, etc.
auto result = coordinator->inferMultiple(
    {"fast_model", "accurate_model", "fallback_model"},
    req,
    config
);
```

---

## Configuration Best Practices

### 1. Quota Configuration

Set realistic per-minute quotas:

```cpp
SubagentConfig config;
// For a 7B model: 50k tokens/minute = ~833 tokens/sec
// (typical inference: 10-50 tokens/sec per user)
config.budget.max_tokens_per_minute = 50000;
config.budget.max_tokens_per_request = 512;
config.budget.timeout_ms = 30000;
config.budget.max_concurrent_requests = 8;
```

### 2. Policy Enforcement

Register corporate policies upfront:

```cpp
auto no_jailbreak_policy = std::make_shared<PromptPolicy>();
no_jailbreak_policy->addBlockRule(
    "jailbreak_attempt",
    R"(ignore (all |previous )?instructions)"
);

factory->registerPromptPolicy("no-jailbreak", no_jailbreak_policy);

// Reference in subagent config
config.policy.prompt_policy_id = "no-jailbreak";
config.policy.block_on_policy_violation = true;  // Enforce strictly
```

### 3. Multi-Tenant Isolation

Use tenant_id for quota scoping:

```cpp
SubagentConfig config;
config.tenant_id = "customer_acme_corp";
config.budget.max_tokens_per_minute = 100000;  // Per-tenant quota

factory->createSubagent(config);  // Quota is scoped to tenant
```

### 4. GPU Placement

Distribute adapters across GPUs:

```cpp
SubagentConfig config_gpu0;
config_gpu0.id = "adapter_1_gpu0";
config_gpu0.gpu.enabled = true;
config_gpu0.gpu.strategy = SubagentMultiGPUStrategy::ROUND_ROBIN;
config_gpu0.gpu.devices = {0, 1, 2, 3};

factory->createSubagent(config_gpu0);
```

---

## Observability and Diagnostics

### Subagent Metrics

Access per-subagent runtime metrics:

```cpp
auto metrics = subagent->getMetrics();

std::cout << "Total requests: " << metrics.total_requests << std::endl;
std::cout << "Successful: " << metrics.successful_inferences << std::endl;
std::cout << "Failed: " << metrics.failed_inferences << std::endl;
std::cout << "Tokens consumed: " << metrics.tokens_consumed << std::endl;
std::cout << "VRAM peak: " << metrics.vram_peak_bytes / (1024*1024) << " MB" << std::endl;
```

### Coordinator Diagnostics

Detailed coordination operation diagnostics:

```cpp
auto diag = coordinator->getLastDiagnostics();

std::cout << "Summary: " << diag.summary << std::endl;
std::cout << "Fan-out latency: " << diag.fan_out_latency.count() << " ms" << std::endl;
std::cout << "Fan-in latency: " << diag.fan_in_latency.count() << " ms" << std::endl;
std::cout << "Merge latency: " << diag.merge_latency.count() << " ms" << std::endl;

for (const auto& log : diag.per_subagent_logs) {
    std::cout << "Log: " << log << std::endl;
}
```

### Factory Statistics

Track factory-level statistics:

```cpp
auto stats = factory->getFactoryStats();

std::cout << "Total created: " << stats.total_created << std::endl;
std::cout << "Currently active: " << stats.currently_active << std::endl;
std::cout << "Total inference requests: " << stats.total_inference_requests << std::endl;
```

---

## Performance Characteristics

### Latency

- Subagent load/unload: ~100ms (model-dependent)
- Single inference: Backend-dependent (typically 100-5000ms)
- Coordinator fan-out: ~1-10ms
- Coordinator fan-in: Limited by slowest subagent
- Merge: Strategy-dependent (typically <50ms)

### Memory

- Per subagent: Model VRAM + LoRA adapter VRAM + runtime buffers
- Shared infrastructure: ~1-2GB (worker pool, caches, quota manager)
- Coordinator overhead: <100MB (future collection, diagnostics)

### Throughput

- Coordinator: Can handle multiple concurrent coordination operations
- Subagent: Limited by backend inference throughput
- Quota enforcement: O(1) per-subagent

---

## Integration with ThemisDB

### Shared Infrastructure

Subagents access these ThemisDB components:

- **AsyncInferenceEngine**: Parallel request execution
- **SharedWorkerPool**: Worker thread pool
- **WikiIndexStore**: RAG data (concurrent read-safe)
- **LLMResponseCache**: Response caching
- **TokenQuotaManager**: Per-subagent quotas
- **PromptPolicy**: Prompt safety gates
- **LLMInteractionStore**: Audit logging

### Thread Safety

- All subagent methods are thread-safe
- Multiple threads can call infer() / inferAsync() concurrently
- Shared infrastructure is protected by internal locks
- Coordinator is thread-safe for concurrent coordination operations

### Error Handling

Errors are propagated with descriptive messages:

```cpp
auto result = subagent->infer(request);
if (!result.success) {
    std::cerr << "Inference failed: " << result.error << std::endl;
    // Handle error: quota exceeded, policy violation, timeout, etc.
}
```

---

## Roadmap & Future Enhancements

### Phase A (Complete) ✅
- [x] SubagentConfig struct + factory interface
- [x] Subagent lifecycle (load/unload/pause/resume)
- [x] Basic inference operations (sync, async, batch)

### Phase B (Complete) ✅
- [x] SubagentLifecycleManager integration
- [x] Per-subagent quota tracking
- [x] Policy enforcement

### Phase C (Complete) ✅
- [x] SubagentCoordinator with fan-out/fan-in
- [x] Merge strategies (FIRST_WIN, ENSEMBLE, etc.)
- [x] Partial failure handling

### Phase D (Complete) ✅
- [x] Comprehensive test suite (SO-01..SO-48)
- [x] Concurrent load testing
- [x] Resource isolation validation

### Phase E (In Progress)
- [ ] Operational deployment guide
- [ ] Helm charts for Kubernetes
- [ ] Prometheus metrics exporters
- [ ] Observability dashboards

### Future Phases
- [ ] Fairness enforcement (cross-subagent preemption)
- [ ] Adaptive quota allocation
- [ ] Cost-aware merge strategies
- [ ] Distributed coordinator (multi-node)
- [ ] Real-time subagent priority adjustment

---

## See Also

- `ROADMAP.md` — Module-level roadmap
- `llm_api_contract.h` — Inference API guarantees
- `shared_worker_pool.h` — Worker pool documentation
- `token_quota_manager.h` — Quota management
- `prompt_policy.h` — Policy gate implementation
