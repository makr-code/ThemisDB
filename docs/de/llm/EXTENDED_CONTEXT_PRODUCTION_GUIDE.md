# Extended Context Window (32K-128K) - Production Guide

**Status:** Production-Ready (v1.4.0+)  
**Vorherige Status:** Experimental (v1.4.0-alpha)  
**Datum:** Januar 2026

---

## Übersicht

Dieser Leitfaden beschreibt die Production-Ready Implementierung von Extended Context Windows (32K-128K Tokens) mit RoPE/YARN-Skalierung in ThemisDB v1.4.0+. Die Features wurden von experimentellem Status in Production-Ready überführt.

### Was ist neu in v1.4.0 (Production)?

✅ **Production-Stabilität erreicht:**
- RoPE/YARN Integration finalisiert auf Model- und API-Ebene
- Thread-Safety für Context Scaling mit LoRA/Adapters
- Durchgängiges RAM/VRAM Profiling & Monitoring
- Feature Flags und Backward-Compatibility

✅ **Dokumentierte Limitierungen:**
- Klare Memory-Requirements für verschiedene Context-Größen
- Best Practices für Production-Deployment
- Monitoring und Alerting Guidelines

⚠️ **Bekannte Einschränkungen:**
- Context Scaling mit LoRA Adapters erfordert Sequential Operations
- Memory-Footprint skaliert linear mit Context Size
- Quality kann bei sehr hohen Scaling Factors (>16x) abnehmen

---

## Architektur-Übersicht

### RoPE/YARN Integration

```
┌─────────────────────────────────────────────────────────────┐
│                    LLM Model Loader                          │
│  ┌────────────────────────────────────────────────────────┐ │
│  │  1. Model Config Validation                            │ │
│  │     - Check rope_scaling_enabled flag                  │ │
│  │     - Validate original_context vs max_context         │ │
│  │     - Check memory requirements                        │ │
│  └────────────────────────────────────────────────────────┘ │
│  ┌────────────────────────────────────────────────────────┐ │
│  │  2. RoPE Scaling Method Selection                      │ │
│  │     ┌──────────┬──────────┬──────────┬──────────┐     │ │
│  │     │  Linear  │   NTK    │   YARN   │ Dynamic  │     │ │
│  │     └──────────┴──────────┴──────────┴──────────┘     │ │
│  │     • rope_freq_scale  • rope_freq_base               │ │
│  │     • yarn_ext_factor  • yarn_attn_factor             │ │
│  └────────────────────────────────────────────────────────┘ │
│  ┌────────────────────────────────────────────────────────┐ │
│  │  3. Context Creation with llama.cpp                    │ │
│  │     llama_context_params ctx_params;                   │ │
│  │     ctx_params.rope_scaling_type = YARN;               │ │
│  │     ctx_params.n_ctx = max_context;                    │ │
│  └────────────────────────────────────────────────────────┘ │
│  ┌────────────────────────────────────────────────────────┐ │
│  │  4. Memory Profiling & Monitoring                      │ │
│  │     - Track RAM/VRAM usage                             │ │
│  │     - Export Prometheus metrics                        │ │
│  │     - Log memory warnings                              │ │
│  └────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

### Integration Points

1. **Model Loading** (`src/llm/model_loader.cpp`)
   - Lines 613-664: RoPE/YARN Configuration
   - Supports: linear, ntk, yarn, dynamic scaling methods

2. **Context Creation** (`llama_context_params`)
   - `rope_scaling_type`: Scaling method enum
   - `rope_freq_scale`: Frequency scaling factor
   - `yarn_ext_factor`, `yarn_attn_factor`, `yarn_beta_fast`, `yarn_beta_slow`

3. **Memory Management** (`src/llm/gpu_memory_manager.cpp`)
   - RAM/VRAM tracking per model
   - Prometheus metrics export
   - Memory pressure alerts

4. **LoRA Integration** (Thread-Safety)
   - Sequential LoRA operations recommended
   - Mutex-based synchronization
   - Configurable lock timeouts

---

## Konfiguration

### Basis-Konfiguration (4K → 32K)

```yaml
# config/llm_extended_context.yaml
extended_context:
  enabled: true
  maturity_status: "stable"
  backward_compatible: true

rope_scaling:
  enabled: true
  method: "yarn"  # Empfohlen für hohe Scaling Factors
  original_context: 4096
  max_context: 32768
  
  yarn:
    ext_factor: 1.0
    attn_factor: 1.0
    beta_fast: 32.0
    beta_slow: 1.0
```

### Memory Management

```yaml
memory:
  profiling:
    enabled: true
    log_interval: 60
    prometheus_metrics: true
  
  limits:
    max_ram_mb: 16384   # 16GB RAM
    max_vram_mb: 24576  # 24GB VRAM
    max_cache_mb: 8192  # 8GB KV Cache
  
  estimation:
    bytes_per_param: 0.5  # Q4 quantization
    bytes_per_token: 256  # LLaMA 7B F16
    safety_margin: 1.2    # 20% overhead
```

### Thread-Safety für LoRA

```yaml
thread_safety:
  lora_adapter_warnings:
    enabled: true
    sequential_only: true  # Empfohlen für Production
    lock_timeout_ms: 1000
  
  synchronization:
    use_mutex: true
    use_rwlock: false  # Experimental
```

### Production Monitoring

```yaml
production:
  validation:
    enabled: true
    check_memory: true
    check_model_support: true
    check_rope_config: true
    fail_on_validation_error: true
  
  monitoring:
    enabled: true
    alerts:
      memory_threshold: 85  # Alert at 85% memory usage
      context_threshold: 90000  # Alert at 90K tokens
      rope_errors: true
      oom_events: true
  
  fallback:
    auto_reduce_context: true
    reduction_factor: 0.5
    min_context: 4096
    disable_on_failures: true
    failure_threshold: 3
```

---

## Memory Requirements

### Schätzung für 7B-Modell (Q4 Quantization)

**Memory Components:**
- **Base Model:** Model weights (~4.5 GB for 7B Q4)
- **KV Cache:** Scales with context length
- **System Overhead:** ~10-20% additional (OS, drivers, etc.)

| Context Size | Base Memory | KV Cache | Total RAM | Total VRAM* |
|--------------|-------------|----------|-----------|-------------|
| 4K (native)  | 4.5 GB      | 1 GB     | 5.5 GB    | 6-7 GB      |
| 8K           | 4.5 GB      | 2 GB     | 6.5 GB    | 8-9 GB      |
| 16K          | 4.5 GB      | 4 GB     | 8.5 GB    | 10-12 GB    |
| 32K          | 4.5 GB      | 8 GB     | 12.5 GB   | 16-20 GB    |
| 64K          | 4.5 GB      | 16 GB    | 20.5 GB   | 28-32 GB    |
| 128K         | 4.5 GB      | 32 GB    | 36.5 GB   | 52-60 GB    |

**\*VRAM varies based on GPU offload layers:**
- Full GPU offload: VRAM ≈ Total RAM + 20-30% overhead
- Partial offload (16 layers): VRAM ≈ Total RAM / 2 + overhead
- CPU only: VRAM = 0 GB

**Note:** Total VRAM includes GPU driver overhead, kernel memory, and
working buffers which can add 20-50% beyond the calculated values.

**Formel:**
```
KV_Cache_Size = n_ctx × n_layers × hidden_size × 2 (key + value) × dtype_size
```

**Beispiel (LLaMA 7B):**
- `n_ctx`: 32768 tokens
- `n_layers`: 32
- `hidden_size`: 4096
- `dtype`: FP16 (2 bytes)

```
KV_Cache = 32768 × 32 × 4096 × 2 × 2 bytes
         = 17.18 GB
```

### Schätzung für 13B/70B-Modelle

**13B Modell @ 32K Context:**
- Base Memory: ~8 GB (Q4)
- KV Cache: ~14 GB
- **Total: ~22 GB RAM, ~30 GB VRAM**

**70B Modell @ 32K Context:**
- Base Memory: ~40 GB (Q4)
- KV Cache: ~18 GB
- **Total: ~58 GB RAM, ~80 GB VRAM**

---

## RoPE Scaling Methods

### 1. Linear Scaling

**Verwendung:** Einfache 2x Skalierung

**Vorteile:**
- ✅ Sehr einfach zu implementieren
- ✅ Minimaler Overhead
- ✅ Stabil für 2x Scaling

**Nachteile:**
- ❌ Quality-Degradation bei >2x
- ❌ Nicht empfohlen für >8K context

**Konfiguration:**
```yaml
rope_scaling:
  method: "linear"
  original_context: 4096
  max_context: 8192  # Maximal 2x empfohlen
```

**Performance:**
- Scaling Factor: 2x
- Quality Loss: ~5-10%
- Memory Overhead: 0%

---

### 2. NTK-Aware Scaling

**Verwendung:** Moderate Skalierung (4x-8x)

**Vorteile:**
- ✅ Bessere Quality als Linear
- ✅ Stabil bis 8x Scaling
- ✅ Dynamische Frequency Base

**Nachteile:**
- ❌ Begrenzt auf ~8x Scaling
- ❌ Komplexere Parameterierung

**Konfiguration:**
```yaml
rope_scaling:
  method: "ntk"
  original_context: 4096
  max_context: 32768  # 8x Scaling
```

**Performance:**
- Scaling Factor: 4x-8x
- Quality Loss: ~3-5%
- Memory Overhead: 0%

---

### 3. YaRN Scaling (Empfohlen)

**Verwendung:** Hohe Skalierung (8x-32x)

**Vorteile:**
- ✅ **Beste Quality bei hohen Factors**
- ✅ Skaliert bis 32x mit minimaler Quality-Loss
- ✅ Fine-grained Parameter-Control

**Nachteile:**
- ❌ Komplexere Konfiguration
- ❌ Leicht erhöhter Compute-Overhead

**Konfiguration:**
```yaml
rope_scaling:
  method: "yarn"
  original_context: 4096
  max_context: 131072  # 32x Scaling
  
  yarn:
    ext_factor: 1.0      # High-frequency preservation
    attn_factor: 1.0     # Attention temperature
    beta_fast: 32.0      # High-freq cutoff
    beta_slow: 1.0       # Low-freq cutoff
```

**Performance:**
- Scaling Factor: 8x-32x
- Quality Loss: ~1-3%
- Memory Overhead: <1%

**Parameter Tuning:**

```yaml
# Für bessere High-Frequency Detail Preservation:
yarn:
  ext_factor: 2.0      # ↑ Preserve mehr Details
  beta_fast: 16.0      # ↓ Lower cutoff

# Für bessere Long-Range Pattern Preservation:
yarn:
  beta_slow: 2.0       # ↑ Higher cutoff
  attn_factor: 0.8     # ↓ Reduce attention temp
```

---

### 4. Dynamic Scaling (Experimental)

**Verwendung:** Adaptive Skalierung basierend auf Input Length

**Vorteile:**
- ✅ Optimiert für tatsächliche Prompt-Länge
- ✅ Kein Over-Allocation

**Nachteile:**
- ❌ Experimental
- ❌ Variable Performance

**Konfiguration:**
```yaml
rope_scaling:
  method: "dynamic"
  original_context: 4096
  max_context: 32768
```

---

## Production Deployment

### Pre-Deployment Checklist

- [ ] **Memory Validation**
  - Verfügbare RAM/VRAM prüfen
  - Memory Estimation durchführen
  - Safety Margin einplanen (20-30%)

- [ ] **Model Testing**
  - Model mit Extended Context laden
  - Inference-Tests durchführen
  - Quality-Benchmarks ausführen

- [ ] **Configuration Review**
  - RoPE Scaling Method validieren
  - Memory Limits setzen
  - Thread-Safety Konfiguration prüfen

- [ ] **Monitoring Setup**
  - Prometheus Metrics aktivieren
  - Grafana Dashboards konfigurieren
  - Alert Rules definieren

- [ ] **Backup-Plan**
  - Fallback auf native Context vorbereiten
  - Auto-Reduction aktivieren
  - Failure Thresholds setzen

### Deployment Steps

#### 1. Configuration Update

```bash
# Kopiere Extended Context Config
cp config/llm_extended_context.yaml config/llm_extended_context.production.yaml

# Edit für Production
vim config/llm_extended_context.production.yaml
```

#### 2. Enable Extended Context

```yaml
# In llm_config.example.yaml oder config.yaml
llm_plugins:
  llamacpp:
    context:
      n_ctx: 32768  # Target context size
      extended_context:
        enabled: true
        config_file: "config/llm_extended_context.production.yaml"
```

#### 3. Restart with Validation

```bash
# Restart ThemisDB Server mit Validation
themis_server --config config.yaml --validate-llm-config

# Check Logs für Validation Results
tail -f logs/themis_server.log | grep "RoPE"
```

#### 4. Monitor Initial Performance

```bash
# Prometheus Metrics
curl http://localhost:9090/metrics | grep themis_llm

# Memory Usage
curl http://localhost:8765/api/v1/llm/memory-stats

# Context Usage
curl http://localhost:8765/api/v1/llm/context-stats
```

### Gradual Rollout Strategy

**Phase 1: Canary (10% Traffic)**
```yaml
# Start mit kleinem Context für 10% Traffic
max_context: 8192  # 2x Scaling
method: "linear"
```

**Phase 2: Beta (50% Traffic)**
```yaml
# Erhöhe auf 16K für 50% Traffic
max_context: 16384  # 4x Scaling
method: "ntk"
```

**Phase 3: Production (100% Traffic)**
```yaml
# Full Rollout mit 32K Context
max_context: 32768  # 8x Scaling
method: "yarn"
```

---

## Monitoring & Observability

### Prometheus Metrics

#### Memory Metrics

```promql
# Total VRAM Usage
themis_llm_vram_used_bytes{model="llama-7b"}

# Context Cache Size
themis_llm_context_cache_bytes{model="llama-7b"}

# Memory Pressure
rate(themis_llm_memory_pressure_total[5m])
```

#### Context Metrics

```promql
# Average Context Length
avg(themis_llm_context_length{model="llama-7b"})

# Context Length Distribution
histogram_quantile(0.95, 
  rate(themis_llm_context_length_bucket[5m])
)

# RoPE Scaling Errors
rate(themis_llm_rope_scaling_errors_total[5m])
```

#### Performance Metrics

```promql
# Inference Latency by Context Length
rate(themis_llm_inference_duration_seconds_sum[5m]) 
/ 
rate(themis_llm_inference_duration_seconds_count[5m])

# Throughput (tokens/sec)
rate(themis_llm_tokens_generated_total[5m])
```

### Grafana Dashboard

```json
{
  "dashboard": {
    "title": "ThemisDB Extended Context Monitoring",
    "panels": [
      {
        "title": "VRAM Usage by Model",
        "targets": [
          {
            "expr": "themis_llm_vram_used_bytes"
          }
        ]
      },
      {
        "title": "Context Length Distribution",
        "targets": [
          {
            "expr": "histogram_quantile(0.95, themis_llm_context_length_bucket)"
          }
        ]
      },
      {
        "title": "RoPE Scaling Errors",
        "targets": [
          {
            "expr": "rate(themis_llm_rope_scaling_errors_total[5m])"
          }
        ]
      }
    ]
  }
}
```

### Alerting Rules

```yaml
# Prometheus Alert Rules
groups:
  - name: themis_extended_context
    rules:
      # Alert on high memory usage
      - alert: HighVRAMUsage
        expr: themis_llm_vram_used_bytes / themis_llm_vram_total_bytes > 0.85
        for: 5m
        annotations:
          summary: "High VRAM usage (>85%)"
          description: "Model {{ $labels.model }} using {{ $value }}% VRAM"
      
      # Alert on RoPE scaling errors
      - alert: RoPEScalingErrors
        expr: rate(themis_llm_rope_scaling_errors_total[5m]) > 0
        for: 1m
        annotations:
          summary: "RoPE scaling errors detected"
          description: "Model {{ $labels.model }} experiencing RoPE errors"
      
      # Alert on OOM events
      - alert: LLMOutOfMemory
        expr: rate(themis_llm_oom_events_total[5m]) > 0
        for: 1m
        annotations:
          summary: "LLM Out of Memory events"
          description: "Model {{ $labels.model }} experiencing OOM"
```

---

## Performance Tuning

### Optimierung für verschiedene Use Cases

#### 1. RAG Workloads (Long Context Retrieval)

**Characteristics:**
- Large context windows (16K-32K)
- Consistent system prompts
- High cache hit rate

**Optimierungen:**
```yaml
optimizations:
  use_kv_cache_reuse: true  # Reuse KV cache for system prompts
  prefix_cache:
    enabled: true
    similarity_threshold: 0.95
    max_entries: 1000

rope_scaling:
  method: "yarn"
  max_context: 32768
```

**Expected Performance:**
- First Token Latency: 2-3s (cold cache)
- First Token Latency: 200-500ms (warm cache)
- Throughput: 50-80 tokens/sec

---

#### 2. Code Generation (Ultra-Long Context)

**Characteristics:**
- Very large context (64K-128K)
- Full codebase context
- Lower throughput acceptable

**Optimierungen:**
```yaml
rope_scaling:
  method: "yarn"
  max_context: 131072  # 128K
  yarn:
    ext_factor: 2.0  # Better high-freq preservation

memory:
  limits:
    max_vram_mb: 40960  # 40GB VRAM
```

**Expected Performance:**
- First Token Latency: 5-10s
- Throughput: 20-30 tokens/sec
- Memory Usage: 32-48GB VRAM

---

#### 3. Chat Applications (Medium Context)

**Characteristics:**
- Moderate context (8K-16K)
- High throughput required
- Interactive latency

**Optimierungen:**
```yaml
rope_scaling:
  method: "ntk"
  max_context: 16384

optimizations:
  continuous_batching:
    enabled: true
    max_batch_size: 32
```

**Expected Performance:**
- First Token Latency: 500-1000ms
- Throughput: 100-150 tokens/sec
- Memory Usage: 8-12GB VRAM

---

## Thread-Safety mit LoRA Adapters

### Problem

Extended Context + LoRA Adapters können zu Race Conditions führen:

1. **Context Switch während LoRA Load:**
   - Thread A lädt LoRA Adapter
   - Thread B ändert Context Size
   - → Inkonsistenter State

2. **Concurrent Adapter Switching:**
   - Thread A aktiviert Adapter 1
   - Thread B aktiviert Adapter 2
   - → KV Cache Corruption

### Lösung: Sequential Operations

#### Konfiguration

```yaml
thread_safety:
  lora_adapter_warnings:
    enabled: true
    sequential_only: true  # Enforce sequential LoRA ops
    lock_timeout_ms: 1000
  
  synchronization:
    use_mutex: true  # Mutex für Context Access
```

#### Code Pattern (C++)

```cpp
// Safe LoRA Adapter Switching
std::lock_guard<std::mutex> lock(context_mutex_);

// 1. Remove current adapter
if (current_adapter_) {
    llama_lora_adapter_remove(ctx, current_adapter_);
}

// 2. Load new adapter
auto* new_adapter = llama_lora_adapter_load(adapter_path.c_str());

// 3. Apply adapter
llama_lora_adapter_set(ctx, new_adapter, 1.0f);

current_adapter_ = new_adapter;
```

### Performance Impact

| Operation | Without Lock | With Mutex | With RWLock |
|-----------|--------------|------------|-------------|
| Read-only Inference | 100% | 95% | 98% |
| LoRA Switch | Race Condition | Safe | Safe |
| Concurrent Requests | Crash Risk | Sequential | Mostly Concurrent |

**Recommendation:** Use Mutex für Production (thread-safe, predictable)

---

## Troubleshooting

### Problem: OOM (Out of Memory)

**Symptome:**
```
ERROR: Failed to allocate KV cache
ERROR: CUDA out of memory
```

**Diagnose:**
```bash
# Check VRAM Usage
nvidia-smi

# Check Memory Estimation
curl http://localhost:8765/api/v1/llm/estimate-memory?context=32768
```

**Lösungen:**
1. Reduce Context Size:
   ```yaml
   max_context: 16384  # Halve context size
   ```

2. Use Lower Precision:
   ```yaml
   # Switch to Q4 quantization
   model_path: "models/llama-7b-q4.gguf"
   ```

3. Offload fewer layers to GPU:
   ```yaml
   gpu_layers: 16  # Reduce from 32
   ```

---

### Problem: Quality Degradation

**Symptome:**
- Repetitive outputs
- Incoherent long-range dependencies
- Hallucinations

**Diagnose:**
```bash
# Test with different scaling methods
curl -X POST http://localhost:8765/api/v1/llm/test \
  -d '{"context": 32768, "method": "yarn"}'
```

**Lösungen:**
1. Try Different Scaling Method:
   ```yaml
   rope_scaling:
     method: "yarn"  # Try yarn instead of linear
   ```

2. Reduce Scaling Factor:
   ```yaml
   max_context: 16384  # Reduce from 32K to 16K
   ```

3. Tune YaRN Parameters:
   ```yaml
   yarn:
     ext_factor: 2.0  # Increase for better quality
     beta_fast: 16.0  # Lower cutoff
   ```

---

### Problem: High Latency

**Symptome:**
- Slow first token generation (>5s)
- Low throughput (<20 tokens/sec)

**Diagnose:**
```bash
# Profile inference
curl http://localhost:8765/api/v1/llm/profile?enable=true

# Check KV cache reuse
curl http://localhost:8765/api/v1/llm/cache-stats
```

**Lösungen:**
1. Enable KV Cache Reuse:
   ```yaml
   optimizations:
     use_kv_cache_reuse: true
   ```

2. Enable Flash Attention:
   ```yaml
   optimizations:
     use_flash_attn: true  # Requires Ampere+ GPU
   ```

3. Use Continuous Batching:
   ```yaml
   continuous_batching:
     enabled: true
     max_batch_size: 32
   ```

---

### Problem: LoRA Adapter Crashes

**Symptome:**
```
ERROR: Context corruption after LoRA switch
SIGSEGV in llama_decode
```

**Diagnose:**
```bash
# Check thread-safety config
grep "thread_safety" config/llm_extended_context.yaml

# Check concurrent requests
curl http://localhost:8765/api/v1/llm/active-requests
```

**Lösungen:**
1. Enable Sequential LoRA Operations:
   ```yaml
   thread_safety:
     lora_adapter_warnings:
       sequential_only: true
   ```

2. Increase Lock Timeout:
   ```yaml
   lock_timeout_ms: 2000  # Increase from 1000ms
   ```

3. Disable Concurrent Requests during Switch:
   ```yaml
   lora:
     pause_inference_during_switch: true
   ```

---

## Best Practices

### 1. Start Small, Scale Gradually

```yaml
# Phase 1: Test with 2x scaling
max_context: 8192
method: "linear"

# Phase 2: Move to 4x scaling
max_context: 16384
method: "ntk"

# Phase 3: Production with 8x scaling
max_context: 32768
method: "yarn"
```

### 2. Always Monitor Memory

```yaml
memory:
  profiling:
    enabled: true
    log_interval: 60
    prometheus_metrics: true
  
  limits:
    max_vram_mb: 24576  # Set explicit limits
    enforce_memory_limits: true
```

### 3. Use Feature Flags

```yaml
extended_context:
  enabled: true
  backward_compatible: true  # Fallback to native context
  
production:
  validation:
    enabled: true
    fail_on_validation_error: true
```

### 4. Test Quality Before Production

```bash
# Run quality benchmarks
python benchmarks/llm/test_extended_context.py \
  --model llama-7b \
  --contexts 4096,8192,16384,32768 \
  --methods linear,ntk,yarn

# Compare perplexity scores
python benchmarks/llm/compare_perplexity.py \
  --baseline 4096 \
  --extended 32768
```

### 5. Plan for Failure

```yaml
production:
  fallback:
    auto_reduce_context: true  # Auto-reduce on memory pressure
    disable_on_failures: true   # Disable after 3 failures
    failure_threshold: 3
```

---

## Migration von v1.4.0-alpha zu v1.4.0-stable

### Breaking Changes

**Keine Breaking Changes** - Vollständig rückwärtskompatibel.

### Configuration Updates

```bash
# 1. Backup existing config
cp config/llm_config.example.yaml config/llm_config.example.yaml.bak

# 2. Add extended context config
cp config/llm_extended_context.yaml config/

# 3. Update llm_config.example.yaml
# Add reference to extended_context config
```

### Neue Features in v1.4.0-stable

✅ **RoPE/YARN Finalization:**
- Alle Scaling Methods production-ready
- YaRN Parameters fully configurable

✅ **RAM/VRAM Profiling:**
- Prometheus Metrics integriert
- Memory estimation utilities
- Alert thresholds konfigurierbar

✅ **Thread-Safety:**
- LoRA Adapter synchronization
- Context access mutex
- Configurable lock timeouts

✅ **Feature Flags:**
- `maturity_status`: "stable"
- `backward_compatible`: true
- Validation checks aktivierbar

---

## Referenzen

### Dokumentation

- [LLAMA_CPP_INTEGRATION.md](./LLAMA_CPP_INTEGRATION.md) - llama.cpp Integration
- [Chapter 16: ML](../../compendium/docs/chapter_16_ml.md) - ML Integration Architektur
- [INVESTIGATION_GAPS_SIMULATIONS_THEMISDB.md](../../INVESTIGATION_GAPS_SIMULATIONS_THEMISDB.md) - Gap Analysis

### Wissenschaftliche Papers

- **YaRN:** "YaRN: Efficient Context Window Extension of Large Language Models" (arXiv:2309.00071)
- **NTK-Aware:** "Scaling Rotary Positional Embeddings via NTK-Aware Interpolation" (arXiv:2306.15595)
- **RoPE:** "RoFormer: Enhanced Transformer with Rotary Position Embedding" (arXiv:2104.09864)

### External Resources

- llama.cpp Documentation: https://github.com/ggerganov/llama.cpp
- GGUF Format Specification: https://github.com/ggerganov/ggml/blob/master/docs/gguf.md

---

## Support

Bei Problemen oder Fragen:

- 🐛 Bug Reports: [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
- 💡 Feature Requests: [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
- 📧 Email: support@themisdb.org
- 💬 Discord: [ThemisDB Community](https://discord.gg/themisdb)

---

**Version:** v1.4.0-stable  
**Status:** Production-Ready  
**Last Updated:** April 2026  
**License:** MIT
