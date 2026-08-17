# LLM Module - Operations & Troubleshooting Guide

<!-- Status: complete | validated: 2026-08-17 -->
<!-- Links: README.md · ARCHITECTURE.md · THREADING.md · CONFIGURATION.md -->

Version: 1.0 (Phase 6)
Last Updated: 2026-08-17
Module Path: src/llm/

---

## Table of Contents

1. [Model Loading Runbook](#model-loading-runbook)
2. [Error Handling Strategies](#error-handling-strategies)
3. [Performance Tuning](#performance-tuning)
4. [Debugging Checklist](#debugging-checklist)
5. [Common Issues & Solutions](#common-issues--solutions)
6. [Monitoring & Observability](#monitoring--observability)
7. [Emergency Recovery](#emergency-recovery)

---

## Model Loading Runbook

### Pre-Flight Checklist

Before loading a model, verify:

```bash
# 1. Verify model file exists and is readable
ls -lh /path/to/model.gguf
file /path/to/model.gguf  # Should identify as GGUF format

# 2. Check GPU availability and VRAM
nvidia-smi  # For NVIDIA GPUs
# Expected: Device count > 0, Free memory > model size

# 3. Verify sufficient free disk space
df -h /path/to/models/
# Expected: At least model size + 10% free space

# 4. Check module initialization status
curl http://localhost:8080/health  # Or equivalent health endpoint
# Expected: {"status": "ready", "models": [...]}
```

### Loading Procedure

**Synchronous Loading (Blocking):**

```cpp
#include <llm/embedded_llm.h>
using namespace themisdb::llm;

EmbeddedLLM llm;
llm.initialize();  // Initialize module first

// Attempt load
LoadConfig config{
    .gpu_memory_fraction = 0.8f,
    .fallback_to_cpu = true,
    .num_threads = -1  // Auto-detect
};

auto status = llm.loadModel("path/to/model.gguf", config);

if (!status.ok()) {
    LOG(ERROR) << "Model load failed: " << status.message();
    
    // Diagnose failure
    if (status.code() == StatusCode::kOutOfMemory) {
        // GPU/CPU memory exhausted
        LOG(INFO) << "Free up memory and retry";
    } else if (status.code() == StatusCode::kNotFound) {
        // File not found
        LOG(ERROR) << "Model file not found; check path";
    } else if (status.code() == StatusCode::kInvalidArgument) {
        // File format invalid
        LOG(ERROR) << "Model format unsupported; expected GGUF";
    } else {
        // Other error; check logs
        LOG(ERROR) << "Unexpected error; see module logs";
    }
    return status;
}

LOG(INFO) << "Model loaded successfully";
```

**Asynchronous Loading (Non-Blocking):**

```cpp
auto load_handle = llm.loadModelAsync(
    "path/to/model.gguf",
    LoadConfig{...},
    [](Status status) {
        if (status.ok()) {
            LOG(INFO) << "Model load complete";
        } else {
            LOG(ERROR) << "Model load failed: " << status.message();
        }
    }
);

// Check status later
std::this_thread::sleep_for(std::chrono::seconds(1));
auto progress = llm.getLoadProgress(load_handle);
LOG(INFO) << "Load progress: " << progress.percentage << "%";
```

### Troubleshooting Model Load

| Error | Cause | Solution |
|---|---|---|
| `kNotFound` | File doesn't exist | Verify path: `ls -la /path/to/model.gguf` |
| `kInvalidArgument` | Not a GGUF file | Use `file` command: `file model.gguf` |
| `kOutOfMemory` | GPU/CPU memory exhausted | Reduce batch size, enable CPU fallback, or unload other models |
| `kFailedPrecondition` | Module not initialized | Call `llm.initialize()` first |
| `kInternal` | Backend error | Check backend logs: `tail -f /var/log/themis-llm.log` |

### Model Unloading

**Safe Unload:**

```cpp
// Before unloading, ensure no queries are in-flight
auto active_count = llm.getActiveQueryCount("model_name");
if (active_count > 0) {
    LOG(WARNING) << "Model has " << active_count << " active queries";
    LOG(INFO) << "Waiting for queries to complete...";
    
    // Wait up to 30 seconds
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    while (llm.getActiveQueryCount("model_name") > 0 && 
           std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    if (llm.getActiveQueryCount("model_name") > 0) {
        LOG(ERROR) << "Queries still active; forcing unload (may cause errors)";
    }
}

// Unload
auto status = llm.unloadModel("model_name");
if (!status.ok()) {
    LOG(ERROR) << "Unload failed: " << status.message();
}
```

---

## Error Handling Strategies

### Error Hierarchy

```
Status (base)
├── kOk (success)
├── kCancelled (request cancelled by caller)
├── kInvalidArgument (input validation failed)
├── kNotFound (model/adapter not found)
├── kAlreadyExists (duplicate load attempt)
├── kPermissionDenied (access denied)
├── kResourceExhausted (VRAM/memory/quota exhausted)
├── kFailedPrecondition (precondition not met)
├── kAborted (operation aborted)
├── kOutOfRange (parameter out of range)
├── kUnimplemented (feature not implemented)
├── kInternal (internal error)
├── kUnavailable (backend/service unavailable)
├── kDataLoss (data loss detected)
├── kDeadlineExceeded (timeout)
├── kUnauthenticated (auth failure)
└── kPermissionDenied (permission error)
```

### Policy Violation Handling

**Scenario:** Prompt violates safety policy.

```cpp
auto result = llm.complete(prompt, config);

if (result.status().code() == StatusCode::kPolicyViolation) {
    // Policy check failed
    auto error = llm.getPolicyViolationDetails();
    LOG(WARNING) << "Policy violation: " << error.reason;
    
    // Options:
    // 1. Modify prompt and retry
    std::string modified_prompt = filterPrompt(prompt, error.violated_rules);
    result = llm.complete(modified_prompt, config);
    
    // 2. Use fallback response
    return CompletionResponse{.text = "Unable to process request due to safety policy."};
    
    // 3. Escalate to human review
    auditLog.log(AuditEvent{
        .type = AuditEventType::kPolicyViolation,
        .prompt = prompt,
        .reason = error.reason
    });
}
```

### Timeout Handling

**Scenario:** Request exceeds configured timeout.

```cpp
auto config = CompletionConfig{
    .timeout_ms = 30000  // 30 seconds
};

auto result = llm.complete(prompt, config);

if (result.status().code() == StatusCode::kDeadlineExceeded) {
    LOG(WARNING) << "Request timed out after 30s";
    
    // Option 1: Retry with shorter prompt or smaller model
    auto shorter_prompt = truncatePrompt(prompt, 512);
    auto shorter_config = config;
    shorter_config.max_tokens = 256;  // Reduce output
    
    result = llm.complete(shorter_prompt, shorter_config);
    
    // Option 2: Cancel and return partial result
    llm.cancel(request_handle);
    return CompletionResponse{
        .text = "Inference interrupted (timeout)",
        .is_complete = false
    };
    
    // Option 3: Increase timeout (if system not overloaded)
    if (getSystemLoad() < 0.8) {
        config.timeout_ms = 60000;  // Try 60 seconds
        result = llm.complete(prompt, config);
    }
}
```

### VRAM Exhaustion Handling

**Scenario:** GPU memory exhausted.

```cpp
auto result = llm.complete(prompt, config);

if (result.status().code() == StatusCode::kOutOfMemory ||
    result.status().code() == StatusCode::kResourceExhausted) {
    
    LOG(ERROR) << "Out of memory";
    
    auto gpu_state = llm.getGPUState();
    LOG(INFO) << "GPU state: " << gpu_state.used_mb << " / " 
              << gpu_state.total_mb << " MB";
    
    // Strategy 1: Unload least-recently-used model
    auto lru_model = llm.evictLRUModel();
    LOG(INFO) << "Evicted model: " << lru_model;
    
    // Retry
    result = llm.complete(prompt, config);
    if (result.ok()) {
        LOG(INFO) << "Retry succeeded after eviction";
        return result;
    }
    
    // Strategy 2: Fall back to CPU inference
    LOG(WARNING) << "GPU inference failed; retrying on CPU";
    auto cpu_config = config;
    cpu_config.use_gpu = false;
    result = llm.complete(prompt, cpu_config);
    
    // Strategy 3: Return backpressure signal to caller
    if (!result.ok()) {
        return Status(StatusCode::kResourceExhausted, 
                     "GPU memory exhausted; service overloaded");
    }
}
```

### Backend Unavailable Handling

**Scenario:** LLM backend (llama.cpp, VLLM) crashes or becomes unavailable.

```cpp
auto result = llm.complete(prompt, config);

if (result.status().code() == StatusCode::kUnavailable) {
    LOG(ERROR) << "Backend unavailable";
    
    // Check backend health
    auto health = llm.getBackendHealth();
    if (!health.is_alive) {
        LOG(ERROR) << "Backend process died";
        
        // Attempt restart
        auto restart_status = llm.restartBackend();
        if (restart_status.ok()) {
            LOG(INFO) << "Backend restarted";
            
            // Reload model and retry
            auto reload_status = llm.reloadCurrentModel();
            if (reload_status.ok()) {
                result = llm.complete(prompt, config);
                return result;
            }
        }
        
        // If restart fails, signal circuit breaker
        circuitBreaker.trip();
        return Status(StatusCode::kUnavailable, "Backend unavailable; circuit breaker open");
    }
}
```

---

## Performance Tuning

### Latency Optimization

**Goal:** Minimize first-token latency (Time-to-First-Token / TTFT).

```cpp
// 1. Pre-load models on startup
llm.preloadModels({"default-model", "large-model"});

// 2. Use model caching
llm.enableModelCache(ModelCacheConfig{
    .max_size_mb = 2048,
    .eviction_policy = EvictionPolicy::kLRU
});

// 3. Reduce prompt preprocessing
auto config = CompletionConfig{
    .use_cache = true,              // Reuse KV cache
    .num_beams = 1,                 // Greedy decoding (fastest)
    .do_sample = false,             // No sampling overhead
    .temperature = 1.0f             // Required for greedy
};

// 4. Optimize GPU execution
llm.setGPUConfig(GPUConfig{
    .use_flash_attention = true,    // If supported
    .use_paged_attention = true,    // For long sequences
    .fuse_kernels = true            // Kernel fusion
});

// Measure TTFT
auto start = std::chrono::steady_clock::now();
auto result = llm.complete(prompt, config);
auto ttft = std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::steady_clock::now() - start
).count();
LOG(INFO) << "First token latency: " << ttft << " ms";
```

### Throughput Optimization

**Goal:** Maximize tokens/second and batch size.

```cpp
// 1. Enable continuous batching
llm.enableContinuousBatching(true);

// 2. Configure batching window
auto config = CompletionConfig{
    .batch_size = 1024,             // Max tokens per batch
    .batch_timeout_ms = 100         // Wait up to 100ms to fill batch
};

// 3. Use smaller models for higher throughput
llm.setDefaultModel("medium-model");  // Faster than "large-model"

// 4. Reduce output length
config.max_tokens = 256;  // Shorter output = faster

// Measure throughput
auto total_tokens = 0;
auto start = std::chrono::steady_clock::now();

for (int i = 0; i < num_requests; ++i) {
    auto result = llm.complete(prompt, config);
    total_tokens += result.value().token_count;
}

auto elapsed_s = std::chrono::duration_cast<std::chrono::seconds>(
    std::chrono::steady_clock::now() - start
).count();

auto throughput = total_tokens / elapsed_s;
LOG(INFO) << "Throughput: " << throughput << " tokens/sec";
```

### Memory Optimization

**Goal:** Minimize GPU/CPU memory footprint.

```cpp
// 1. Quantize model (8-bit or 4-bit)
auto config = LoadConfig{
    .quantization = QuantizationType::kQ4_K_M,  // 4-bit quantization
    .gpu_memory_fraction = 0.6f                 // Use less VRAM
};
llm.loadModel("model.gguf", config);

// 2. Use paged attention for long sequences
llm.setGPUConfig(GPUConfig{
    .use_paged_attention = true,
    .page_size = 16  // Tokens per page
});

// 3. Enable KV cache offloading
llm.setGPUConfig(GPUConfig{
    .offload_kv_cache_to_cpu = true,
    .kv_cache_in_vram_percent = 0.5f  // 50% in VRAM, 50% in CPU
});

// 4. Monitor memory usage
auto stats = llm.getMemoryStats();
LOG(INFO) << "GPU memory: " << stats.gpu_used_mb << " / " 
          << stats.gpu_total_mb << " MB";
LOG(INFO) << "CPU memory: " << stats.cpu_used_mb << " MB";
```

### Batch Processing

**Goal:** Process multiple requests efficiently.

```cpp
// Batch inference with streaming
std::vector<std::string> prompts = {...};
std::vector<std::string> results;

auto batch_config = CompletionConfig{
    .batch_size = 64,           // Process 64 requests concurrently
    .batch_timeout_ms = 500     // Wait up to 500ms to fill batch
};

// Option 1: Sequential with batching
for (const auto& prompt : prompts) {
    auto result = llm.complete(prompt, batch_config);
    results.push_back(result.value().text);
}

// Option 2: Parallel with work queue
ThreadPool pool(8);  // 8 workers
std::queue<std::string> work_queue;
std::mutex queue_mutex;

for (const auto& prompt : prompts) {
    pool.enqueue([&]() {
        auto result = llm.complete(prompt, batch_config);
        {
            std::lock_guard lock(queue_mutex);
            results.push_back(result.value().text);
        }
    });
}

pool.wait();  // Wait for all to complete
```

---

## Debugging Checklist

### Step 1: Verify Module Initialization

```cpp
// Check if module is initialized
if (!llm.isInitialized()) {
    LOG(ERROR) << "Module not initialized";
    return;
}

// Get initialization status
auto init_status = llm.getInitStatus();
LOG(INFO) << "Init status: " << init_status.message();

// Check backend availability
auto backends = llm.listAvailableBackends();
LOG(INFO) << "Available backends: " << backends.size();
for (const auto& b : backends) {
    LOG(INFO) << "  - " << b.name << " (" << b.version << ")";
}
```

### Step 2: Check Model State

```cpp
// List loaded models
auto models = llm.listLoadedModels();
LOG(INFO) << "Loaded models: " << models.size();
for (const auto& m : models) {
    LOG(INFO) << "  - " << m.name << ": " << m.size_mb << " MB";
}

// Check model-specific state
auto model_state = llm.getModelState("model_name");
LOG(INFO) << "Model state: " << model_state.state;  // loaded/loading/error
LOG(INFO) << "Active queries: " << model_state.active_query_count;
LOG(INFO) << "Cache hits: " << model_state.cache_hit_count;
LOG(INFO) << "Cache misses: " << model_state.cache_miss_count;
```

### Step 3: Monitor GPU/CPU Resources

```cpp
// Get system resource state
auto system_state = llm.getSystemState();

LOG(INFO) << "GPU " << system_state.gpu_device_id << ":";
LOG(INFO) << "  Used: " << system_state.gpu_used_mb << " MB";
LOG(INFO) << "  Total: " << system_state.gpu_total_mb << " MB";
LOG(INFO) << "  Utilization: " << system_state.gpu_utilization_percent << "%";
LOG(INFO) << "  Temperature: " << system_state.gpu_temp_celsius << "°C";

LOG(INFO) << "System:";
LOG(INFO) << "  CPU Threads: " << system_state.cpu_thread_count;
LOG(INFO) << "  Memory Used: " << system_state.memory_used_mb << " MB";
LOG(INFO) << "  Memory Total: " << system_state.memory_total_mb << " MB";
```

### Step 4: Inspect Request State

```cpp
// Submit request and get handle
auto handle = llm.submit(request);

// Poll request state
auto state = llm.getRequestState(handle);
LOG(INFO) << "Request state: " << state.state;  // queued/executing/completed
LOG(INFO) << "Elapsed: " << state.elapsed_ms << " ms";
LOG(INFO) << "Tokens generated: " << state.tokens_generated;
LOG(INFO) << "Estimated remaining: " << state.estimated_remaining_ms << " ms";

// For stuck requests, get more detail
if (state.elapsed_ms > 60000) {  // Over 60 seconds
    auto details = llm.getRequestDetails(handle);
    LOG(WARNING) << "Request stuck:";
    LOG(WARNING) << "  Worker: " << details.assigned_worker;
    LOG(WARNING) << "  GPU: " << details.gpu_device;
    LOG(WARNING) << "  Memory: " << details.memory_used_mb << " MB";
    LOG(WARNING) << "  Last activity: " << details.last_activity_ms << " ms ago";
    
    // Consider cancelling
    llm.cancel(handle);
}
```

### Step 5: Check Logs

```bash
# Tail LLM module logs
tail -f /var/log/themis-llm.log

# Filter for errors
grep "ERROR" /var/log/themis-llm.log | tail -20

# Filter for warnings
grep "WARNING" /var/log/themis-llm.log | tail -20

# Trace performance issues
grep "latency" /var/log/themis-llm.log | tail -10

# Check GPU errors
grep -i "cuda" /var/log/themis-llm.log | tail -20
```

---

## Common Issues & Solutions

### Issue: "CUDA Out of Memory" on First Inference

**Symptoms:**
- Model loads successfully
- First inference fails with `kOutOfMemory`
- Subsequent inferences work

**Cause:** KV cache allocation requires first-token execution path.

**Solution:**
```cpp
// Reduce KV cache pages
auto config = LoadConfig{
    .kv_cache_pages = 4096,  // Reduced from 8192
    .gpu_memory_fraction = 0.7f
};
llm.loadModel("model.gguf", config);
```

---

### Issue: Inconsistent Inference Results

**Symptoms:**
- Same prompt produces different outputs
- Results vary between runs

**Cause:** Sampling (temperature > 0) produces non-deterministic results.

**Solution:**
```cpp
// For deterministic results, disable sampling
auto config = CompletionConfig{
    .temperature = 1.0f,  // Required for greedy
    .do_sample = false,   // Disable sampling
    .seed = 42            // Set seed if sampling needed
};

auto result = llm.complete(prompt, config);
```

---

### Issue: Inference Hangs (No Progress)

**Symptoms:**
- Inference submitted but no result
- Worker thread appears stuck

**Cause:** Deadlock, infinite loop, or GPU hang.

**Solution:**
```cpp
// Use timeout to detect hangs
auto config = CompletionConfig{
    .timeout_ms = 30000  // 30-second timeout
};

auto result = llm.complete(prompt, config);

if (result.status().code() == StatusCode::kDeadlineExceeded) {
    LOG(ERROR) << "Inference hung; cancelling";
    llm.cancel(handle);
    
    // Check GPU state
    if (!llm.isGPUHealthy()) {
        LOG(ERROR) << "GPU unhealthy; restarting backend";
        llm.restartBackend();
    }
}
```

---

### Issue: High Latency (Slow Inference)

**Symptoms:**
- First-token latency > 500 ms
- Tokens generating slowly

**Cause:** CPU fallback, small batch size, or CPU-bound inference.

**Solution:**
```cpp
// Check if using CPU
auto state = llm.getSystemState();
if (state.gpu_device == -1) {  // -1 means CPU
    LOG(WARNING) << "Using CPU inference (slow); verify GPU availability";
}

// Optimize config
auto config = CompletionConfig{
    .use_cache = true,           // Use KV cache
    .num_beams = 1,              // Greedy (no beam search overhead)
    .batch_size = 512,           // Larger batches
    .batch_timeout_ms = 500      // Wait for batch to fill
};

// Monitor throughput
auto metrics = llm.getMetrics();
LOG(INFO) << "Tokens/sec: " << metrics.tokens_per_second;
LOG(INFO) << "TTFT: " << metrics.time_to_first_token_ms << " ms";
LOG(INFO) << "Inter-token latency: " << metrics.inter_token_latency_ms << " ms";
```

---

### Issue: Memory Leak Suspected

**Symptoms:**
- GPU memory usage increases over time
- `nvidia-smi` shows increasing GPU memory after many inferences

**Cause:** KV cache not properly freed, or model not unloaded.

**Solution:**
```cpp
// Enable memory tracking
llm.enableMemoryTracking(true);

// Run memory leak test
std::vector<std::string> prompts = generateTestPrompts(1000);

for (const auto& prompt : prompts) {
    auto result = llm.complete(prompt, config);
    
    if (i % 100 == 0) {
        auto stats = llm.getMemoryStats();
        LOG(INFO) << "Iteration " << i << ": GPU memory = " 
                  << stats.gpu_used_mb << " MB";
    }
}

// Check memory report
auto leak_report = llm.getMemoryLeakReport();
if (!leak_report.leaks.empty()) {
    LOG(ERROR) << "Memory leaks detected:";
    for (const auto& leak : leak_report.leaks) {
        LOG(ERROR) << "  " << leak.description << ": " << leak.size_bytes << " bytes";
    }
}
```

---

## Monitoring & Observability

### Prometheus Metrics

The LLM module exports the following Prometheus metrics:

```prometheus
# Counter: Total inference requests
themis_llm_inference_requests_total{model="llama2-7b", status="success"}

# Gauge: Currently active requests
themis_llm_active_requests{model="llama2-7b"}

# Histogram: Inference latency
themis_llm_inference_latency_ms_bucket{model="llama2-7b", le="100"}

# Gauge: GPU memory usage
themis_llm_gpu_memory_bytes{device="0", type="used"}

# Counter: Cache hits/misses
themis_llm_cache_hits_total{type="response"}
themis_llm_cache_misses_total{type="response"}

# Gauge: Model load time
themis_llm_model_load_duration_ms{model="llama2-7b"}

# Counter: Errors
themis_llm_errors_total{type="OutOfMemory"}
```

**Scrape Configuration:**
```yaml
global:
  scrape_interval: 15s

scrape_configs:
  - job_name: 'themisdb-llm'
    static_configs:
      - targets: ['localhost:9090']
```

### Distributed Tracing

Enable OpenTelemetry tracing to track request flow:

```cpp
// Enable tracing
llm.enableTracing(TracingConfig{
    .exporter = "jaeger",  // or "zipkin", "otlp"
    .endpoint = "localhost:6831",
    .service_name = "themisdb-llm",
    .sample_rate = 0.1  // Sample 10% of requests
});

// Trace shows:
// 1. Policy validation time
// 2. Model selection time
// 3. Queue wait time
// 4. Inference execution time
// 5. Response caching time
```

---

## Emergency Recovery

### Scenario: Service Completely Unresponsive

**Steps:**

1. **Check Process Status**
   ```bash
   ps aux | grep themis
   pgrep -a themis
   ```

2. **Check Port Availability**
   ```bash
   lsof -i :8080  # Check if port open
   netstat -tulpn | grep 8080
   ```

3. **Check Logs for Errors**
   ```bash
   tail -200 /var/log/themis-llm.log | grep -i error
   tail -50 /var/log/systemd-journal  # System logs
   ```

4. **Force Restart**
   ```bash
   systemctl restart themis-llm
   sleep 5
   curl http://localhost:8080/health
   ```

5. **If Still Unresponsive**
   ```bash
   # Kill all processes
   pkill -9 themis
   
   # Check for locked files
   lsof | grep themis-llm
   
   # Restart service
   systemctl start themis-llm
   ```

### Scenario: GPU Driver Crash

**Steps:**

1. **Verify GPU State**
   ```bash
   nvidia-smi
   dmesg | tail -50  # Look for GPU errors
   ```

2. **Reload GPU Driver**
   ```bash
   sudo rmmod nvidia_uvm
   sudo rmmod nvidia
   sudo modprobe nvidia
   nvidia-smi  # Verify
   ```

3. **Restart Service**
   ```bash
   systemctl restart themis-llm
   ```

### Scenario: Out of Disk Space (Model Directory)

**Steps:**

1. **Check Disk Usage**
   ```bash
   df -h /path/to/models
   du -sh /path/to/models/*
   ```

2. **Free Space**
   ```bash
   # Remove least-used model
   rm /path/to/models/least-used-model.gguf
   ```

3. **Restart Service**
   ```bash
   systemctl restart themis-llm
   ```

---

**Last Updated:** 2026-08-17 (Phase 6)
**Status:** PRODUCTION (Wave 5 GA)
