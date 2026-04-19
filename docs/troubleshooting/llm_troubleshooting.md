# LLM Troubleshooting Guide

The `llm` module provides ThemisDB's LLM orchestration layer, including GGUF model loading, multi-GPU inference with paged KV cache, LoRA adapter management, continuous batching, ethical guidelines enforcement, Byzantine fault detection, and AI decision auditing.

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| `EmbeddedLlm: model file not found` | Wrong model path or missing GGUF file | Check `llm.model_path` and file permissions |
| `AdaptiveVramAllocator: OOM` | Model too large for available VRAM | Reduce `n_gpu_layers` or use a smaller quantization |
| `LoRA adapter load failed` | Incompatible adapter rank or base model | Verify adapter was trained on same base model version |
| Inference extremely slow | Model running on CPU instead of GPU | Set `n_gpu_layers: -1` to offload all layers |
| `EthicalGuidelinesManager: content blocked` | Output matched ethics policy | Review and tune ethics policy thresholds |
| `ByzantineDetector: replica disagreement` | Multi-model ensemble votes diverge | Check that all adapter replicas are identical |
| `ContinuousBatchScheduler: queue full` | Throughput too low for request rate | Increase `batch_size` or scale horizontally |
| `AiDecisionAuditor: audit write failed` | Audit log storage full | Clear old audit logs; increase audit storage |
| Context length exceeded | Prompt + response > model context window | Reduce input; enable context compression |
| `AdapterLoadBalancer: no healthy adapters` | All LoRA adapters failed health checks | Check adapter files; restart adapter workers |

## Common Issues

### Issue 1: GGUF Model File Not Found or Corrupted

**Description:** ThemisDB cannot load the GGUF model file at startup.

**Symptoms:**
- Log: `EmbeddedLlm: failed to load model: /var/lib/themisdb/models/llama-3-8b.Q4_K_M.gguf: No such file or directory`
- LLM features return `503 Service Unavailable`

**Cause:** Model file not downloaded, wrong path in configuration, or file was corrupted during download.

**Solution:**
```bash
# Verify file exists and is readable
ls -lh /var/lib/themisdb/models/
sha256sum /var/lib/themisdb/models/llama-3-8b.Q4_K_M.gguf

# Check config
themisdb-admin llm model-info

# Download model via admin API
curl -X POST http://localhost:9090/admin/llm/download-model \
     -H "Authorization: Bearer $ADMIN_TOKEN" \
     -d '{"model_id": "llama-3-8b-q4km"}'
```
```yaml
llm:
  model_path: /var/lib/themisdb/models/llama-3-8b.Q4_K_M.gguf
  model_format: gguf
```

---

### Issue 2: GPU Out of Memory (VRAM OOM)

**Description:** Model loading or inference fails with GPU memory exhaustion.

**Symptoms:**
- Log: `AdaptiveVramAllocator: CUDA OOM – requested 6144MB, available 4096MB`
- `nvidia-smi` shows GPU memory at 100%

**Cause:** Model is too large for the GPU's VRAM, or KV cache is consuming remaining VRAM.

**Solution:**
```yaml
llm:
  n_gpu_layers: 20             # offload only 20 of 32 layers; rest on CPU
  kv_cache:
    max_tokens: 4096           # reduce KV cache from default 8192
    type: f16                  # use "q8_0" or "q4_0" to save VRAM
  vram:
    max_fraction: 0.85         # leave 15% VRAM for OS/other processes
    reserve_mb: 512
```
```bash
# Check VRAM usage
nvidia-smi --query-gpu=memory.used,memory.free --format=csv

# Show model layer distribution
themisdb-admin llm vram-plan --model llama-3-8b
```

---

### Issue 3: LoRA Adapter Fails to Load

**Description:** A LoRA adapter cannot be applied to the base model.

**Symptoms:**
- Log: `AdapterRegistry: LoRA adapter 'legal-v2' incompatible: rank mismatch (expected 64, got 128)`
- Specialised model queries fall back to base model

**Cause:** Adapter was trained with a different rank or on a different base model checkpoint.

**Solution:**
```bash
# Check adapter metadata
themisdb-admin llm adapter info --name legal-v2

# List registered adapters
themisdb-admin llm adapter list

# Register a compatible adapter
themisdb-admin llm adapter register \
  --name legal-v2 \
  --path /var/lib/themisdb/adapters/legal-v2.gguf \
  --base-model llama-3-8b \
  --rank 64
```
```yaml
llm:
  adapters:
    - name: legal-v2
      path: /var/lib/themisdb/adapters/legal-v2.gguf
      rank: 64
      auto_load: true
```

---

### Issue 4: Inference Runs on CPU Instead of GPU

**Description:** Inference is 10–50× slower than expected because all computation is on the CPU.

**Symptoms:**
- Log: `EmbeddedLlm: n_gpu_layers=0; running on CPU only`
- `nvidia-smi` shows 0% GPU utilisation during inference

**Cause:** `n_gpu_layers` is set to `0` or CUDA libraries are not found.

**Solution:**
```yaml
llm:
  n_gpu_layers: -1              # -1 = offload ALL layers to GPU
  cuda:
    enabled: true
    device_ids: [0, 1]          # specify GPU indices
```
```bash
# Verify CUDA is available
nvidia-smi
ldconfig -p | grep libcuda

# Check compiled GPU support
themisdb --version | grep CUDA
```

---

### Issue 5: Ethical Guidelines Block Valid Requests

**Description:** The `EthicalGuidelinesManager` blocks responses that should be allowed.

**Symptoms:**
- Log: `EthicalGuidelinesManager: response blocked (category=harmful_content score=0.72 threshold=0.70)`
- Users receive `{"error": "content_policy_violation"}` on benign queries

**Cause:** Threshold is too low; topic-specific content (e.g., medical information) triggers the classifier.

**Solution:**
```yaml
llm:
  ethical_guidelines:
    enabled: true
    block_threshold: 0.85           # increase from 0.70
    log_threshold: 0.50
    categories:
      harmful_content:
        enabled: true
        threshold: 0.90
      bias:
        enabled: true
        threshold: 0.80
    allowlist_prompts:
      - "medical information for licensed practitioners"
```

---

### Issue 6: Continuous Batch Scheduler Queue Full

**Description:** Under high request load, the batch scheduler drops requests.

**Symptoms:**
- Log: `ContinuousBatchScheduler: queue full (max_queue_size=100); request dropped`
- HTTP 429 responses from LLM endpoints

**Cause:** Insufficient batch throughput for the incoming request rate.

**Solution:**
```yaml
llm:
  continuous_batching:
    enabled: true
    max_batch_size: 32              # increase from 8
    max_queue_size: 500             # increase from 100
    scheduler_interval_ms: 10
    prefill_chunk_size: 512
    max_sequence_length: 4096
```
```bash
# Monitor batch queue depth
curl -s http://localhost:9100/metrics | grep themisdb_llm_batch_queue
```

---

### Issue 7: Byzantine Detector Raises False Alarms

**Description:** The Byzantine detector reports disagreement among model replicas when all produce correct output.

**Symptoms:**
- Log: `ByzantineDetector: replica disagreement detected (divergence=0.15 threshold=0.10)`
- Ensemble responses are rejected even when correct

**Cause:** Different LoRA adapters produce slightly different probabilities for the same correct answer; threshold is too tight.

**Solution:**
```yaml
llm:
  byzantine_detector:
    enabled: true
    divergence_threshold: 0.25      # increase from 0.10
    min_replicas: 3
    voting_strategy: majority       # "majority" | "unanimous"
    log_divergence: true
```

---

### Issue 8: KV Cache Exceeds Memory Limit

**Description:** Paged KV cache grows unbounded and exhausts system RAM.

**Symptoms:**
- Log: `BlockTable: KV cache page allocation failed – no free pages`
- ThemisDB process RSS grows continuously during long sessions

**Cause:** `max_kv_cache_pages` is too large; long-running sessions are not evicted.

**Solution:**
```yaml
llm:
  kv_cache:
    max_pages: 2048                 # set explicit page limit
    page_size_tokens: 16
    eviction_policy: lru            # "lru" | "fifo"
    session_timeout_ms: 300000      # evict sessions idle > 5 min
```

---

### Issue 9: AI Decision Audit Log Write Failures

**Description:** The AI decision audit log fails to persist records.

**Symptoms:**
- Log: `AiDecisionAuditor: audit write failed: disk quota exceeded`
- Audit queries return incomplete history

**Cause:** Audit log directory is full; no rotation configured.

**Solution:**
```bash
# Check audit log disk usage
du -sh /var/lib/themisdb/audit/llm/

# Manually rotate and compress
themisdb-admin llm audit rotate --compress --keep-days 90
```
```yaml
llm:
  audit:
    enabled: true
    path: /var/lib/themisdb/audit/llm/
    max_size_mb: 1024
    retention_days: 90
    compress_rotated: true
```

---

### Issue 10: Constitutional Reasoning Engine Returns Empty Response

**Description:** Constitutional AI reasoning produces no output for complex prompts.

**Symptoms:**
- Log: `ConstitutionalReasoningEngine: critique phase produced empty revision`
- API response: `{"text": ""}`

**Cause:** Critique prompt is too restrictive; base model is not capable of constitutional revision.

**Solution:**
```yaml
llm:
  constitutional_reasoning:
    enabled: true
    max_revision_rounds: 3
    critique_temperature: 0.7
    revision_temperature: 0.5
    fallback_to_base: true          # return base response if revision fails
```

## Diagnostic Commands

```bash
# Model load status
themisdb-admin llm status

# List loaded adapters
themisdb-admin llm adapter list

# Show VRAM allocation plan
themisdb-admin llm vram-plan

# Test inference
themisdb-admin llm inference-test --prompt "Hello, world"

# Monitor batch queue
curl -s http://localhost:9100/metrics | grep themisdb_llm

# Tail LLM logs
journalctl -u themisdb -f | grep -E "llm|gguf|lora|adapter|vram|kv_cache|ethics"

# Show audit log entries
themisdb-admin llm audit show --last 50
```

## Configuration Reference

```yaml
llm:
  enabled: true
  model_path: /var/lib/themisdb/models/llama-3-8b.Q4_K_M.gguf
  model_format: gguf
  n_gpu_layers: -1              # -1 = all layers on GPU
  n_threads: 8
  context_size: 4096
  kv_cache:
    max_pages: 1024
    page_size_tokens: 16
    eviction_policy: lru
  continuous_batching:
    enabled: true
    max_batch_size: 16
    max_queue_size: 200
  adapters: []
  ethical_guidelines:
    enabled: true
    block_threshold: 0.85
  byzantine_detector:
    enabled: false
    divergence_threshold: 0.25
  audit:
    enabled: true
    path: /var/lib/themisdb/audit/llm/
    retention_days: 90
```

**Common misconfigurations:**

| Key | Wrong | Correct |
|-----|-------|---------|
| `n_gpu_layers` | `0` | `-1` or model-appropriate value |
| `ethical_guidelines.block_threshold` | `0.5` | `0.85–0.95` |
| `kv_cache.max_pages` | unlimited | `1024–4096` |
| `continuous_batching.max_queue_size` | `10` | `200–1000` |

## Known Limitations

- GGUF format only; ONNX and PyTorch native formats require the `llm_onnx` plugin.
- Multi-GPU tensor parallelism requires NCCL; see `docs/NCCL_RCCL_INTEGRATION_GUIDE.md`.
- LoRA adapters trained with rank > 256 significantly increase VRAM usage per request.
- Byzantine detection requires ≥ 3 adapter replicas and adds ~30% latency to ensemble inference.
- Constitutional reasoning doubles average inference time due to critique-revision loop.

## Related Documentation

- [LLM Module ROADMAP](../../src/llm/ROADMAP.md)
- [LLM Roadmap](../llm_roadmap.md)
- [GGUF Support](../GGUF_SUPPORT.md)
- [Paged Optimizer Guide](../llm_orchestration/PAGED_OPTIMIZER_GUIDE.md)
- [NCCL/RCCL Integration Guide](../NCCL_RCCL_INTEGRATION_GUIDE.md)
- [AI Decision Auditing Guide](../de/security/ai_decision_auditing_guide.md)
- [Multi-LoRA Fusion Guide](../MULTI_LORA_FUSION_GUIDE.md)
