# Migration Guide: llama.cpp Feature Updates

**Version:** v1.3.1  
**Target Audience:** Administrators, DevOps  
**Estimated Time:** 15-30 minutes

---

## Overview

This guide helps you migrate existing ThemisDB LLM configurations to take advantage of new llama.cpp features:

- **Flash Attention**: 15-25% faster inference
- **KV-Cache Reuse**: 10-20x faster first-token for repeated prompts
- **Embeddings Extraction**: Unified model for generation + embeddings

**Breaking Changes:** None. All features are opt-in via configuration.

---

## Migration Steps

### Step 1: Backup Your Current Config

```bash
# Backup existing config
cp config/llm_config.yaml config/llm_config.yaml.backup

# Check current config structure
cat config/llm_config.yaml | grep -A 20 "llm_plugins:"
```

### Step 2: Add Performance Optimizations Section

If your config looks like this (v1.3.0):

```yaml
llm_plugins:
  llamacpp:
    type: "llama.cpp"
    enabled: true
    
    gpu:
      n_layers: 32
      use_cuda: true
      max_vram_mb: 14336
    
    context:
      n_ctx: 4096
      n_batch: 512
```

Update it to (v1.3.1):

```yaml
llm_plugins:
  llamacpp:
    type: "llama.cpp"
    enabled: true
    
    gpu:
      n_layers: 32
      use_cuda: true
      max_vram_mb: 14336
    
    context:
      n_ctx: 4096
      n_batch: 512
    
    # NEW: Performance optimizations
    optimizations:
      use_flash_attn: true          # Enable Flash Attention
      use_kv_cache_reuse: true      # Enable KV-Cache Reuse
      enable_embeddings: false      # Keep disabled (separate mode)
      
      # NEW: Prefix cache configuration
      prefix_cache:
        similarity_threshold: 0.95
        max_entries: 1000
        min_prefix_length: 20
        ttl_seconds: 7200
        enable_kv_caching: true
```

### Step 3: Choose Your Migration Path

#### Option A: Quick Migration (Recommended)

Add minimal config for immediate benefits:

```yaml
optimizations:
  use_flash_attn: true          # +15-25% speed
  use_kv_cache_reuse: true      # +10-20x first-token
```

This uses sensible defaults for prefix cache.

#### Option B: Full Migration

Copy the complete example from `config/llm_config.example.yaml`:

```bash
# Extract optimizations section from example
grep -A 20 "optimizations:" config/llm_config.example.yaml >> /tmp/optimizations.yaml

# Manually merge into your config
vim config/llm_config.yaml
```

### Step 4: Validate Configuration

```bash
# Test configuration syntax
python3 -c "import yaml; yaml.safe_load(open('config/llm_config.yaml'))"

# Start ThemisDB with validation
./themis-server --validate-config
```

### Step 5: Restart and Verify

```bash
# Restart ThemisDB
systemctl restart themis-server

# Check logs for new features
journalctl -u themis-server | grep -E "Flash Attention|KV-Cache Reuse"

# Expected output:
#   INFO  LlamaWrapper initialized:
#   INFO    Flash Attention: enabled
#   INFO    KV-Cache Reuse: enabled (10-20x first-token speedup)
```

---

## Feature-Specific Migration

### Flash Attention

**Minimum llama.cpp version:** b2000+

```yaml
optimizations:
  use_flash_attn: true
```

**Verification:**

```bash
# Monitor memory usage (should decrease ~30%)
nvidia-smi --query-gpu=memory.used --format=csv -l 1

# Monitor inference speed (should increase ~20%)
curl -X POST http://localhost:8080/api/llm/generate \
  -d '{"prompt": "Test", "max_tokens": 100}' | jq '.tokens_per_second'
```

### KV-Cache Reuse

**Best for:** RAG workloads with consistent system prompts

```yaml
optimizations:
  use_kv_cache_reuse: true
  
  prefix_cache:
    similarity_threshold: 0.95  # Higher = stricter matching
    max_entries: 1000           # Adjust based on available RAM
    ttl_seconds: 7200           # 2 hours
```

**Tuning for Your Workload:**

| Workload Type | similarity_threshold | max_entries | ttl_seconds |
|---------------|---------------------|-------------|-------------|
| Strict (legal, medical) | 0.98 | 500 | 14400 (4h) |
| Normal (general chatbot) | 0.95 | 1000 | 7200 (2h) |
| Loose (development) | 0.90 | 2000 | 3600 (1h) |

**Verification:**

```bash
# Check cache statistics
curl http://localhost:8080/api/llm/cache/stats | jq

# Expected output:
# {
#   "hits": 650,
#   "misses": 350,
#   "hit_rate": 0.65,
#   "total_entries": 142,
#   "avg_lookup_time_ms": 0.8
# }
```

### Embeddings Extraction

**When to enable:** Only for dedicated embedding servers

```yaml
optimizations:
  enable_embeddings: true  # Switches model to embeddings mode
```

**Warning:** Enabling this on a generation server will break inference. Use a separate instance.

**Separate Config for Embeddings:**

```yaml
# embeddings_config.yaml
llm_plugins:
  llamacpp_embeddings:
    type: "llama.cpp"
    
    model:
      path: "/models/mistral-7b.gguf"
    
    optimizations:
      enable_embeddings: true    # Embeddings mode
      use_flash_attn: true       # Still benefits from Flash Attention
      use_kv_cache_reuse: false  # Not needed for embeddings
```

---

## Troubleshooting

### Flash Attention Not Working

**Symptom:** Log shows "Flash Attention requested but not available"

**Solution:**
1. Check llama.cpp version: `llama-cli --version` (need b2000+)
2. Rebuild with CUDA: `cmake -DLLAMA_CUDA=ON`
3. Check GPU capability: Flash Attention needs Compute Capability 8.0+ (RTX 30xx, A100, etc.)

### Low Cache Hit Rate

**Symptom:** Cache hit rate < 30%

**Solutions:**
1. Lower `similarity_threshold` from 0.95 to 0.90
2. Check if system prompts are actually consistent:
   ```bash
   # Analyze request patterns
   journalctl -u themis-server | grep "system message" | sort | uniq -c
   ```
3. Increase `ttl_seconds` if workloads are bursty

### High Memory Usage

**Symptom:** RAM/VRAM usage increasing

**Solutions:**
1. Reduce `prefix_cache.max_entries`
2. Decrease `prefix_cache.ttl_seconds`
3. Disable `prefix_cache.enable_kv_caching` (cache tokens only)

---

## Rollback Instructions

If you encounter issues:

### Quick Rollback

```bash
# Restore backup
cp config/llm_config.yaml.backup config/llm_config.yaml

# Restart
systemctl restart themis-server
```

### Selective Rollback

Disable specific features:

```yaml
optimizations:
  use_flash_attn: false         # Disable Flash Attention
  use_kv_cache_reuse: false     # Disable KV-Cache Reuse
```

---

## Production Deployment Checklist

- [ ] Config backed up
- [ ] Changes tested in staging
- [ ] Monitoring dashboards updated (new metrics)
- [ ] Alert thresholds reviewed
- [ ] Team notified of changes
- [ ] Rollback plan documented
- [ ] Performance baseline captured
- [ ] Post-deployment verification scheduled

---

## Performance Monitoring

### Key Metrics to Track

```yaml
# Prometheus metrics (added in v1.3.1)
themis_llm_flash_attention_enabled
themis_llm_prefix_cache_hits
themis_llm_prefix_cache_misses
themis_llm_prefix_cache_hit_rate
themis_llm_inference_latency_ms
themis_llm_first_token_latency_ms
```

### Grafana Dashboard Updates

1. Add cache hit rate panel:
```promql
rate(themis_llm_prefix_cache_hits[5m]) / 
(rate(themis_llm_prefix_cache_hits[5m]) + rate(themis_llm_prefix_cache_misses[5m]))
```

2. Add first-token latency panel:
```promql
histogram_quantile(0.95, themis_llm_first_token_latency_ms)
```

---

## FAQ

**Q: Will these changes affect my existing LLM responses?**  
A: No. All optimizations are mathematically equivalent and produce identical results.

**Q: Can I enable features gradually?**  
A: Yes. Enable one at a time, validate, then enable the next.

**Q: What if I'm using vLLM instead of llama.cpp?**  
A: These features are llama.cpp-specific. vLLM has different optimization paths.

**Q: Do I need to retrain or reload models?**  
A: No. Models remain unchanged. These are inference-time optimizations.

**Q: What's the recommended order for enabling features?**  
A: 
1. Flash Attention (zero risk, immediate benefit)
2. KV-Cache Reuse (requires monitoring, huge benefit for RAG)
3. Embeddings Extraction (only if needed)

---

## Support

- **Documentation:** [docs/en/llm/FLASH_ATTENTION_IMPLEMENTATION.md](./FLASH_ATTENTION_IMPLEMENTATION.md)
- **Issues:** https://github.com/makr-code/ThemisDB/issues
- **Community:** Discord #llm-performance

---

## Changelog

### v1.3.1 (2026-01-05)
- Added Flash Attention support
- Added KV-Cache Reuse with prefix caching
- Added Embeddings Extraction capability
- Configuration structure expanded (backward compatible)

### v1.3.0 (2025-12-20)
- Initial llama.cpp integration
- Lazy model loading
- Multi-LoRA support
