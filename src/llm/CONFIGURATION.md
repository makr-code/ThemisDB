# LLM Module - Configuration Guide

<!-- Status: complete | validated: 2026-08-17 -->
<!-- Links: README.md · ARCHITECTURE.md · OPERATIONS.md -->

Version: 1.0 (Phase 6)
Last Updated: 2026-08-17
Module Path: src/llm/

---

## Configuration Methods

### 1. Environment Variables

#### GPU Configuration

```bash
# GPU device selection (default: -1 = auto-select)
export THEMIS_LLM_GPU_DEVICE=0

# GPU memory allocation as fraction of total VRAM (default: 0.8)
export THEMIS_LLM_GPU_MEMORY_FRACTION=0.8

# Allow CPU fallback if GPU unavailable (default: true)
export THEMIS_LLM_FALLBACK_TO_CPU=true

# GPU memory defragmentation threshold % (default: 80)
export THEMIS_LLM_GPU_DEFRAG_THRESHOLD=80

# Use Flash Attention if available (default: true)
export THEMIS_LLM_USE_FLASH_ATTENTION=true

# Use paged attention for long sequences (default: true)
export THEMIS_LLM_USE_PAGED_ATTENTION=true
```

#### Inference Configuration

```bash
# Default model name (default: first loaded model)
export THEMIS_LLM_DEFAULT_MODEL="llama2-7b"

# Number of worker threads (default: -1 = auto = CPU core count)
export THEMIS_LLM_NUM_WORKER_THREADS=8

# Batch size in tokens (default: 1024)
export THEMIS_LLM_BATCH_SIZE=1024

# Batch timeout in milliseconds (default: 100)
export THEMIS_LLM_BATCH_TIMEOUT_MS=100

# Default request timeout in milliseconds (default: 300000 = 5 min)
export THEMIS_LLM_DEFAULT_TIMEOUT_MS=300000

# Maximum concurrent requests (default: -1 = unlimited)
export THEMIS_LLM_MAX_CONCURRENT_REQUESTS=1024

# Enable continuous batching (default: true)
export THEMIS_LLM_CONTINUOUS_BATCHING=true
```

#### Cache Configuration

```bash
# Response cache maximum size in MB (default: 1024)
export THEMIS_LLM_RESPONSE_CACHE_SIZE_MB=1024

# Response cache eviction policy (default: lru)
# Options: lru, lfu, fifo
export THEMIS_LLM_CACHE_EVICTION_POLICY=lru

# Response cache TTL in seconds (default: 3600 = 1 hour)
export THEMIS_LLM_CACHE_TTL_SECONDS=3600

# KV cache number of pages (default: 8192)
export THEMIS_LLM_KV_CACHE_PAGES=8192

# KV cache page size in tokens (default: 16)
export THEMIS_LLM_KV_CACHE_PAGE_SIZE=16

# Offload KV cache to CPU (default: false)
export THEMIS_LLM_OFFLOAD_KV_CACHE_TO_CPU=false

# Prefix cache (persistent) size in MB (default: 256)
export THEMIS_LLM_PREFIX_CACHE_SIZE_MB=256
```

#### Model Management

```bash
# Model directory path (default: ./models)
export THEMIS_LLM_MODEL_PATH=/path/to/models

# Pre-load models on initialization (comma-separated)
export THEMIS_LLM_PRELOAD_MODELS="llama2-7b,mistral-7b"

# Maximum total model size in MB (default: 32000)
export THEMIS_LLM_MODEL_QUOTA_MB=32000

# Model cache eviction policy (default: lru)
export THEMIS_LLM_MODEL_EVICTION_POLICY=lru
```

#### Logging & Observability

```bash
# Logging level (default: INFO)
# Options: TRACE, DEBUG, INFO, WARNING, ERROR, CRITICAL, OFF
export THEMIS_LLM_LOG_LEVEL=INFO

# Enable Prometheus metrics (default: true)
export THEMIS_LLM_ENABLE_METRICS=true

# Prometheus metrics port (default: 9090)
export THEMIS_LLM_METRICS_PORT=9090

# Enable distributed tracing (default: false)
export THEMIS_LLM_ENABLE_TRACING=false

# Trace exporter (default: jaeger)
# Options: jaeger, zipkin, otlp
export THEMIS_LLM_TRACE_EXPORTER=jaeger

# Trace endpoint (default: localhost:6831)
export THEMIS_LLM_TRACE_ENDPOINT=localhost:6831

# Trace sample rate 0-1 (default: 0.1 = 10%)
export THEMIS_LLM_TRACE_SAMPLE_RATE=0.1

# Enable memory tracking (default: false)
export THEMIS_LLM_ENABLE_MEMORY_TRACKING=false
```

---

### 2. Configuration File (YAML)

**File Location:** `~/.themisdb/llm_config.yaml` or `/etc/themisdb/llm_config.yaml`

**Example Configuration:**

```yaml
# GPU Configuration
gpu:
  enabled: true
  device_id: 0                    # -1 = auto
  memory_fraction: 0.8            # 0.0-1.0
  fallback_to_cpu: true
  defrag_threshold_percent: 80
  flash_attention: true
  paged_attention: true

# Inference Configuration
inference:
  default_model: "llama2-7b"
  num_workers: 8                  # -1 = auto
  batch_size: 1024                # tokens
  batch_timeout_ms: 100
  default_timeout_ms: 300000      # 5 min
  max_concurrent_requests: 1024   # -1 = unlimited
  continuous_batching: true
  streaming:
    token_buffer_size: 64
    callback_timeout_ms: 5000
  sampling:
    temperature: 0.7
    top_p: 0.9
    top_k: 40
    repetition_penalty: 1.0

# Cache Configuration
cache:
  response:
    enabled: true
    max_size_mb: 1024
    eviction_policy: lru           # lru, lfu, fifo
    ttl_seconds: 3600              # 1 hour
  
  kv_cache:
    pages: 8192
    page_size: 16                  # tokens
    offload_to_cpu: false
  
  prefix_cache:
    enabled: true
    max_size_mb: 256
    backend: rocksdb               # rocksdb, memory

# Model Management
models:
  directory: /path/to/models
  preload:
    - llama2-7b
    - mistral-7b
  max_total_size_mb: 32000
  eviction_policy: lru
  download:
    enabled: true
    allowed_sources:
      - huggingface
      - ollama
    verify_checksums: true

# Logging & Observability
logging:
  level: INFO                      # TRACE, DEBUG, INFO, WARNING, ERROR
  format: json                     # json, text
  file: /var/log/themis-llm.log
  max_size_mb: 100
  max_files: 10

metrics:
  enabled: true
  port: 9090
  interval_seconds: 15

tracing:
  enabled: false
  exporter: jaeger                 # jaeger, zipkin, otlp
  endpoint: localhost:6831
  service_name: themisdb-llm
  sample_rate: 0.1

# Safety & Security
safety:
  policy_engine: enabled
  prompt_validation: strict        # off, basic, strict
  output_validation: basic
  audit_log: /var/log/themis-llm-audit.log

# Advanced
advanced:
  memory_tracking: false
  profiling: false
  debug_mode: false
  num_threads: -1                  # Worker thread count (-1 = auto)
```

---

### 3. Programmatic Configuration

```cpp
#include <llm/embedded_llm.h>
using namespace themisdb::llm;

// Create config object
auto config = LLMConfig{
    // GPU
    .gpu_enabled = true,
    .gpu_device_id = 0,
    .gpu_memory_fraction = 0.8f,
    .fallback_to_cpu = true,
    
    // Inference
    .default_model = "llama2-7b",
    .num_workers = 8,
    .batch_size = 1024,
    .batch_timeout_ms = 100,
    .default_timeout_ms = 300000,
    .max_concurrent_requests = 1024,
    .continuous_batching = true,
    
    // Cache
    .response_cache_size_mb = 1024,
    .kv_cache_pages = 8192,
    .prefix_cache_enabled = true,
    
    // Logging
    .log_level = LogLevel::kInfo,
    .metrics_enabled = true
};

// Load from file
auto loaded_config = ConfigLoader::loadFromFile("llm_config.yaml");

// Load from environment
auto env_config = ConfigLoader::loadFromEnvironment();

// Initialize LLM with config
EmbeddedLLM llm;
auto status = llm.initialize(config);
if (!status.ok()) {
    LOG(ERROR) << "Initialization failed: " << status.message();
}
```

---

## GPU Memory Tuning

### Understanding GPU Memory

**Memory Breakdown:**
```
Total VRAM = Model Weights + KV Cache + Activations + Temp Buffers

Example (RTX 4090 with 24GB VRAM):
├─ Model (Llama2-7B)     : ~14 GB
├─ KV Cache              : ~4 GB (configurable)
├─ Activations           : ~3 GB (depends on batch size)
├─ Temp Buffers          : ~2 GB
└─ Free                  : ~1 GB
```

### Optimization Strategies

**Strategy 1: Reduce Model Size**

```yaml
# Use quantized model instead of float16
models:
  quantization: Q4_K_M  # 4-bit quantization
                        # Reduces model size by ~75%
                        # Trade-off: Slight quality loss
```

**Strategy 2: Reduce KV Cache**

```yaml
# Smaller KV cache for shorter sequences
kv_cache:
  pages: 2048       # Reduced from 8192
  page_size: 16
```

**Strategy 3: Offload to CPU**

```yaml
# Offload KV cache to CPU (slower but uses less GPU VRAM)
cache:
  kv_cache:
    offload_to_cpu: true
    offload_percent: 0.5  # 50% in GPU, 50% in CPU
```

**Strategy 4: Batch Size**

```yaml
# Smaller batch size = smaller activations
inference:
  batch_size: 256  # Reduced from 1024
```

**Strategy 5: Context Length**

```cpp
// Shorter max_tokens = smaller KV cache
auto config = CompletionConfig{
    .max_tokens = 256  // Reduced from 512
};
```

### Memory Monitoring

```cpp
// Monitor GPU memory in real-time
auto stats = llm.getMemoryStats();

LOG(INFO) << "GPU Memory:";
LOG(INFO) << "  Total: " << stats.gpu_total_mb << " MB";
LOG(INFO) << "  Used:  " << stats.gpu_used_mb << " MB";
LOG(INFO) << "  Free:  " << stats.gpu_free_mb << " MB";
LOG(INFO) << "  Model: " << stats.model_size_mb << " MB";
LOG(INFO) << "  KV Cache: " << stats.kv_cache_mb << " MB";
LOG(INFO) << "  Activations: " << stats.activation_mb << " MB";

// Alert if memory usage is high
if (stats.gpu_used_mb > stats.gpu_total_mb * 0.9) {
    LOG(WARNING) << "GPU memory > 90% utilization";
    
    // Evict least-used model
    llm.evictLRUModel();
}
```

---

## Performance Tuning

### For Latency (Time-to-First-Token)

```yaml
inference:
  batch_size: 1        # No batching delay
  batch_timeout_ms: 0  # Immediate processing
  
models:
  preload:
    - fast-model      # Pre-load fastest model
    
gpu:
  flash_attention: true
  paged_attention: false  # Minimal overhead

cache:
  response:
    enabled: true     # Cache hits are instant
```

### For Throughput (Tokens/Second)

```yaml
inference:
  batch_size: 1024    # Large batches
  batch_timeout_ms: 500  # Wait to fill batch
  max_concurrent_requests: 1024
  
gpu:
  memory_fraction: 0.9  # Use more VRAM for larger batches
  
cache:
  kv_cache:
    pages: 16384  # Larger cache
    page_size: 32  # Larger pages
```

### For Memory Efficiency

```yaml
models:
  directory: /path/to/models
  quantization: Q4_K_M  # 4-bit quantization

cache:
  response:
    max_size_mb: 256  # Smaller cache
  kv_cache:
    pages: 2048  # Fewer pages
    offload_to_cpu: true

gpu:
  memory_fraction: 0.5  # Use less VRAM
```

---

## Inference Presets

### Preset: Fast (Low Latency)

```yaml
# For real-time, interactive applications
inference:
  default_model: fast-model
  batch_size: 1
  batch_timeout_ms: 0
  default_timeout_ms: 5000  # 5 sec timeout
  
models:
  preload: [fast-model]
  
gpu:
  flash_attention: true
```

### Preset: Balanced (Default)

```yaml
# For typical production use
inference:
  default_model: medium-model
  batch_size: 256
  batch_timeout_ms: 100
  default_timeout_ms: 60000  # 1 min timeout
  
models:
  preload: [medium-model]
```

### Preset: High Throughput

```yaml
# For batch processing
inference:
  default_model: large-model
  batch_size: 2048
  batch_timeout_ms: 1000
  max_concurrent_requests: 2048
  
models:
  max_total_size_mb: 64000  # Large quota
  
gpu:
  memory_fraction: 0.95  # Use most VRAM
```

### Preset: Memory Constrained

```yaml
# For edge devices or limited GPU memory
models:
  quantization: Q4_K_M  # 4-bit
  
cache:
  response:
    max_size_mb: 128  # Minimal cache
  kv_cache:
    pages: 1024
    offload_to_cpu: true
    
gpu:
  memory_fraction: 0.5
  fallback_to_cpu: true
```

---

## Troubleshooting Configuration Issues

### Issue: "Invalid Configuration Value"

**Solution:** Validate values against allowed ranges.

```bash
# Check configuration validity
themis-llm --validate-config llm_config.yaml

# Expected output: Configuration valid.
```

### Issue: Configuration Not Applied

**Solution:** Ensure configuration file in correct location.

```bash
# Check configuration file locations (in order):
# 1. THEMIS_LLM_CONFIG environment variable
# 2. ~/.themisdb/llm_config.yaml
# 3. /etc/themisdb/llm_config.yaml
# 4. ./llm_config.yaml (current directory)

# Verify file is readable
ls -la ~/.themisdb/llm_config.yaml
file ~/.themisdb/llm_config.yaml  # Should show YAML

# Check applied configuration
curl http://localhost:8080/config  # Or equivalent API
```

### Issue: GPU Memory Not Allocated Correctly

**Solution:** Check GPU memory fraction setting.

```bash
# Before startup
nvidia-smi  # Note available VRAM

# After startup
nvidia-smi  # Check allocated memory

# If mismatch, verify environment variable or config file
echo $THEMIS_LLM_GPU_MEMORY_FRACTION

# Reconfigure and restart
export THEMIS_LLM_GPU_MEMORY_FRACTION=0.7
systemctl restart themis-llm
```

---

**Last Updated:** 2026-08-17 (Phase 6)
**Status:** PRODUCTION (Wave 5 GA)
