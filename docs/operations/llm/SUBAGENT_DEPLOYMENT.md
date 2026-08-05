# Subagent Deployment Guide - Operations & Troubleshooting

## Operational Deployment

This guide covers production deployment scenarios for ThemisDB's multi-subagent LLM orchestration.

### Minimal Deployment (Single Tenant)

```cpp
#include <themisdb/llm/subagent_factory.h>
#include <themisdb/llm/subagent_coordinator.h>
#include <memory>

int main() {
    // Step 1: Create shared infrastructure (passed from app)
    auto plugin = std::make_shared<ILLMPlugin>();        // Your inference backend
    auto worker_pool = std::make_shared<SharedWorkerPool>();
    auto model_loader = std::make_shared<ModelLoader>();
    auto lora_manager = std::make_shared<MultiLoRAManager>();
    auto quota_manager = std::make_shared<TokenQuotaManager>();

    // Step 2: Create factory
    auto factory = SubagentFactory::create(
        plugin,
        worker_pool,
        model_loader,
        lora_manager,
        quota_manager
    );

    // Step 3: Create subagent with configuration
    SubagentConfig config;
    config.id = "primary_assistant";
    config.model_id = "mistral-7b";
    config.budget.max_tokens_per_minute = 50000;
    config.budget.timeout_ms = 30000;
    config.isolation_level = SubagentIsolationLevel::STRICT;

    auto result = factory->createSubagent(config);
    if (!result.success) {
        std::cerr << "Failed to create subagent: " << result.error << std::endl;
        return 1;
    }

    auto subagent = result.subagent;

    // Step 4: Load model
    auto load_result = subagent->load();
    if (!load_result.success) {
        std::cerr << "Failed to load model: " << load_result.error << std::endl;
        return 1;
    }

    // Step 5: Run inference
    InferenceRequest req;
    req.prompt = "Hello, how are you?";

    auto infer_result = subagent->infer(req);
    if (infer_result.success) {
        std::cout << "Response: " << infer_result.output << std::endl;
    } else {
        std::cerr << "Inference failed: " << infer_result.error << std::endl;
    }

    // Step 6: Cleanup
    subagent->unload();
    factory->destroySubagent(config.id);

    return 0;
}
```

### Multi-Tenant Deployment

For multi-tenant systems, use tenant_id for quota isolation:

```cpp
SubagentConfig tenant_a_config;
tenant_a_config.id = "tenant_a_primary";
tenant_a_config.tenant_id = "customer_acme_corp";
tenant_a_config.model_id = "mistral-7b";
tenant_a_config.budget.max_tokens_per_minute = 100000;

SubagentConfig tenant_b_config;
tenant_b_config.id = "tenant_b_primary";
tenant_b_config.tenant_id = "customer_widgets_inc";
tenant_b_config.model_id = "llama2-13b";
tenant_b_config.budget.max_tokens_per_minute = 50000;

// Quotas are scoped by (tenant_id, model_id)
factory->createSubagent(tenant_a_config);
factory->createSubagent(tenant_b_config);

// Each tenant's inference is quota-isolated
```

### Multi-Subagent Coordination

Use SubagentCoordinator for parallel inference:

```cpp
auto coordinator = SubagentCoordinator::create(factory);

InferenceRequest req;
req.prompt = "Analyze this document...";

SubagentCoordinatorConfig coord_config;
coord_config.strategy = SubagentMergeStrategy::ENSEMBLE;
coord_config.timeout_ms = 5000;
coord_config.fail_on_any_error = false;  // Allow partial failures

auto result = coordinator->inferMultiple(
    {"analyst_1", "analyst_2", "analyst_3"},
    req,
    coord_config
);

if (result.success) {
    std::cout << "Ensemble result: " << result.merged_output << std::endl;
    std::cout << "Subagents succeeded: " << result.successful_count << std::endl;
    std::cout << "Subagents failed: " << result.failed_count << std::endl;
} else {
    std::cerr << "Coordination failed: " << result.error << std::endl;
}
```

---

## Configuration Best Practices

### 1. Quota Configuration

Set realistic quotas based on your inference backend:

```cpp
// Mistral 7B typical performance: ~50-100 tokens/sec
// Plan for: 8-10 concurrent users × 10 tokens/sec = 80-100 tokens/sec
// Safe quota: 50,000 tokens/minute (833 tokens/sec)

SubagentBudgetConfig budget;
budget.max_tokens_per_minute = 50000;
budget.max_tokens_per_request = 512;     // Prevent single large request
budget.timeout_ms = 30000;                // 30 second inference limit
budget.max_concurrent_requests = 8;       // Limit queue depth
```

### 2. Policy Enforcement

Define safety guardrails:

```cpp
// 1. Create policy
auto safety_policy = std::make_shared<PromptPolicy>();
safety_policy->addBlockRule("jailbreak", R"(ignore (all |previous )?instructions)");
safety_policy->addBlockRule("injection", R"(system\s*:\s*override)");

// 2. Register with factory
factory->registerPromptPolicy("default-safety", safety_policy);

// 3. Reference in subagent config
config.policy.prompt_policy_id = "default-safety";
config.policy.block_on_policy_violation = true;
```

### 3. Model and Adapter Selection

Match models to task:

```cpp
// Fast, lightweight models
SubagentConfig fast_model;
fast_model.model_id = "mistral-7b";
fast_model.lora_adapter_id = "fast-inference";  // Quantized LoRA

// Accurate, heavy models
SubagentConfig accurate_model;
accurate_model.model_id = "llama2-70b";
accurate_model.lora_adapter_id = "high-accuracy";

// Speculative execution with fallback
coordinator->inferMultiple(
    {"fast_model", "accurate_model"},
    req,
    {.strategy = SubagentMergeStrategy::FIRST_WIN}
);
```

### 4. VRAM Allocation

Distribute GPU memory efficiently:

```cpp
// Adapter 1 on GPU 0
SubagentConfig config1;
config1.gpu.enabled = true;
config1.gpu.strategy = SubagentMultiGPUStrategy::ROUND_ROBIN;
config1.gpu.devices = {0, 1};

// Adapter 2 on GPU 1
SubagentConfig config2;
config2.gpu.enabled = true;
config2.gpu.strategy = SubagentMultiGPUStrategy::ROUND_ROBIN;
config2.gpu.devices = {1, 2};

// Monitor VRAM usage
auto metrics = subagent->getMetrics();
std::cout << "VRAM used: " << metrics.vram_used_bytes / (1024*1024) << " MB" << std::endl;
```

---

## Monitoring & Observability

### Subagent Metrics

Track per-subagent performance:

```cpp
auto metrics = subagent->getMetrics();

// Throughput
std::cout << "Requests: " << metrics.total_requests << std::endl;
std::cout << "Success rate: " 
          << (100.0 * metrics.successful_inferences / metrics.total_requests) 
          << "%" << std::endl;

// Resource usage
std::cout << "Tokens consumed: " << metrics.tokens_consumed << std::endl;
std::cout << "VRAM peak: " << (metrics.vram_peak_bytes / (1024.0*1024)) << " MB" << std::endl;
std::cout << "Avg latency: " << metrics.avg_inference_latency_ms << " ms" << std::endl;
```

### Coordinator Diagnostics

Inspect coordination operations:

```cpp
auto diag = coordinator->getLastDiagnostics();

// Operation summary
std::cout << "Summary: " << diag.summary << std::endl;

// Timing breakdown
std::cout << "Fan-out: " << diag.fan_out_latency.count() << " ms" << std::endl;
std::cout << "Collect: " << diag.fan_in_latency.count() << " ms" << std::endl;
std::cout << "Merge: " << diag.merge_latency.count() << " ms" << std::endl;
std::cout << "Total: " << diag.total_latency.count() << " ms" << std::endl;

// Per-subagent diagnostics
for (const auto& log : diag.per_subagent_logs) {
    std::cout << "  " << log << std::endl;
}

// Merge errors (if any)
for (const auto& err : diag.merge_errors) {
    std::cerr << "Merge error: " << err << std::endl;
}
```

### Factory Statistics

Track factory-level usage:

```cpp
auto stats = factory->getFactoryStats();

std::cout << "Created subagents: " << stats.total_created << std::endl;
std::cout << "Currently active: " << stats.currently_active << std::endl;
std::cout << "Total inferences: " << stats.total_inference_requests << std::endl;
std::cout << "Failed inferences: " << stats.failed_inference_requests << std::endl;
```

---

## Troubleshooting

### Issue: Quota Exceeded

**Symptom**: Inference returns `SubagentErrorCode::QUOTA_EXCEEDED`

**Diagnosis**:
```cpp
// Check current quota usage
auto metrics = subagent->getMetrics();
std::cout << "Tokens consumed: " << metrics.tokens_consumed << std::endl;

// Check quota window
auto quota_info = quota_manager->getQuotaInfo(config.tenant_id, config.model_id);
std::cout << "Quota remaining: " << quota_info.remaining_tokens << std::endl;
```

**Solutions**:
1. Increase quota: `config.budget.max_tokens_per_minute = higher_value;`
2. Reduce batch size: `config.budget.max_tokens_per_request = smaller_value;`
3. Wait for quota window to reset (60-second sliding window)
4. Add additional subagents for load distribution

### Issue: Policy Violation

**Symptom**: Inference returns `SubagentErrorCode::POLICY_VIOLATION`

**Diagnosis**:
```cpp
// Check if policy is registered
auto factory_stats = factory->getFactoryStats();

// Review policy rules
auto policy = factory->getPolicy(config.policy.prompt_policy_id);
if (!policy) {
    std::cerr << "Policy not registered!" << std::endl;
}
```

**Solutions**:
1. Register missing policy: `factory->registerPromptPolicy(policy_id, policy);`
2. Disable policy enforcement (development only): `config.policy.block_on_policy_violation = false;`
3. Update prompts to comply with policy rules
4. Use advisory mode: `config.isolation_level = SubagentIsolationLevel::ADVISORY;`

### Issue: Subagent Not Ready

**Symptom**: `isReady()` returns false, inference fails

**Diagnosis**:
```cpp
auto state = subagent->getState();
std::cout << "Current state: " << (int)state << std::endl;

auto metrics = subagent->getMetrics();
std::cout << "Is ready: " << metrics.is_ready << std::endl;
```

**State Machine Reference**:
- `CREATED` (0): Initial state
- `LOADING` (1): Model/adapter loading in progress
- `READY` (2): Ready for inference
- `PAUSED` (3): Explicitly paused
- `UNLOADING` (4): Unloading in progress
- `TERMINATED` (5): Fully cleaned up
- `ERROR` (6): Unrecoverable error

**Solutions**:
1. Wait for state transition: Add retry loop with backoff
2. Check load result: `auto result = subagent->load();`
3. Inspect error: `auto error = subagent->getLastError();`
4. Recreate subagent if in ERROR state

### Issue: Coordinator Timeout

**Symptom**: Coordinator returns `timeout_exceeded`

**Diagnosis**:
```cpp
auto diag = coordinator->getLastDiagnostics();
std::cout << "Total latency: " << diag.total_latency.count() << " ms" << std::endl;
std::cout << "Timeout: " << diag.timeout_ms << " ms" << std::endl;

// Find slow subagent
for (const auto& log : diag.per_subagent_logs) {
    std::cout << "  " << log << std::endl;
}
```

**Solutions**:
1. Increase timeout: `coord_config.timeout_ms = 10000;` (10 seconds)
2. Reduce subagent count: Use fast models, skip slow ones
3. Improve subagent performance:
   - Use quantized models
   - Reduce max_tokens_per_request
   - Increase GPU allocation
4. Use FIRST_WIN strategy instead of waiting for all

### Issue: Partial Failures in Coordinator

**Symptom**: Some subagents succeed, others fail; merge strategy matters

**Diagnosis**:
```cpp
auto result = coordinator->inferMultiple(...);
std::cout << "Success: " << result.success << std::endl;
std::cout << "Successful count: " << result.successful_count << std::endl;
std::cout << "Failed count: " << result.failed_count << std::endl;
std::cout << "Error: " << result.error << std::endl;

// Check per-subagent results
for (const auto& [subagent_id, subresult] : result.per_subagent_results) {
    std::cout << subagent_id << ": " 
              << (subresult.success ? "OK" : "FAILED")
              << " - " << subresult.output << std::endl;
}
```

**Solutions**:
1. Use fault-tolerant strategy:
   - `FIRST_WIN`: Return first success
   - `ENSEMBLE`: Combine successes (ignore failures)
2. Set `fail_on_any_error = false` to allow partial failures
3. Implement custom merge function for domain-specific logic
4. Add monitoring to identify consistently failing subagents

### Issue: Memory Leak or High VRAM Usage

**Symptom**: Memory grows over time; VRAM allocation never decreases

**Diagnosis**:
```cpp
auto metrics = subagent->getMetrics();
std::cout << "Peak VRAM: " << (metrics.vram_peak_bytes / (1024*1024)) << " MB" << std::endl;
std::cout << "Current VRAM: " << (metrics.vram_used_bytes / (1024*1024)) << " MB" << std::endl;

// Run cleanup
subagent->resetMetrics();
```

**Solutions**:
1. Unload models when not in use: `subagent->unload();`
2. Clear response cache: (feature future)
3. Monitor batch size: Reduce `max_tokens_per_request`
4. Check for accumulating correlation IDs: Ensure audit logs are rotated
5. Profile with memory sanitizer: `cmake --preset community-asan`

---

## Performance Tuning

### Latency Optimization

```cpp
// Use FIRST_WIN for minimum latency
SubagentCoordinatorConfig config;
config.strategy = SubagentMergeStrategy::FIRST_WIN;
config.timeout_ms = 1000;  // Fast timeout, fall back to next subagent

// Or use async inference
auto async_result = subagent->inferAsync(req);
// Do other work...
auto final_result = async_result.get();  // Wait when needed
```

### Throughput Optimization

```cpp
// Use batching
std::vector<InferenceRequest> batch;
for (int i = 0; i < 32; i++) {
    batch.push_back(create_request(i));
}

auto batch_result = subagent->inferBatch(batch);
std::cout << "Processed " << batch_result.results.size() << " requests" << std::endl;
```

### Resource Optimization

```cpp
// Use quantization to reduce VRAM
SubagentConfig config;
config.gpu.quantization = SubagentQuantizationConfig::INT8;  // 75% VRAM reduction
config.budget.max_concurrent_requests = 16;                 // Increase concurrency

factory->createSubagent(config);
```

---

## Production Checklist

- [ ] All subagents configured with STRICT isolation level
- [ ] Quota limits validated against baseline inference performance
- [ ] Safety policies registered and tested with adversarial prompts
- [ ] VRAM allocation verified to fit GPU capacity (with headroom)
- [ ] Monitoring/metrics collection implemented
- [ ] Log aggregation configured for coordinator diagnostics
- [ ] Alert thresholds set for quota excess and policy violations
- [ ] Graceful shutdown path tested (unload → destroy)
- [ ] Load testing completed with 10x expected peak load
- [ ] Failover strategy documented (which subagent is primary/fallback)
- [ ] Cost model understood (tokens/minute × cost per token)
- [ ] SLA targets documented and validated

---

## See Also

- `SUBAGENT_ARCHITECTURE.md` — Architecture and design
- `include/llm/subagent_config.h` — Configuration reference
- `include/llm/subagent_factory.h` — Factory API
- `include/llm/subagent_coordinator.h` — Coordinator API
- `tests/llm/test_subagent_orchestration_focused.cpp` — Test examples
