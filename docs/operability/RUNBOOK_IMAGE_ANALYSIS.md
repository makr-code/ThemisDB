# Operator Runbook — Image Analysis Module

**Module:** `image_analysis`
**Owner:** Platform Engineering
**Wave D Ref:** D1 operability hardening — mixed OCR/object-detection soak coverage
**Last Updated:** 2026-09-16

---

## Quick Reference

| Scenario | Alert Pattern | Primary Action |
|---|---|---|
| OCR throughput drop | `[IMAGE:OCRThroughputDrop]` | Check Tesseract backend health |
| Detection model stall | `[IMAGE:DetectionModelStall]` | Reload ONNX session / restart plugin |
| Plugin load failure | `[IMAGE:PluginLoadFailed]` | Validate plugin manifest and signature |
| Cache eviction storm | `[IMAGE:CacheEvictionStorm]` | Increase result-cache size / tune TTL |
| Backend provider degraded | `[IMAGE:ProviderDegraded]` | Fall back to secondary backend / alert |

---

## Scenario 1 — OCR Throughput Drop

**Symptom:** `[IMAGE:OCRThroughputDrop]` in logs; OCR ops/s falls below threshold.

**Diagnosis steps:**
1. Check Tesseract process health: `systemctl status tesseract-service` (or process equivalent).
2. Verify input queue depth — if backlogged, upstream ingestion may be overloaded.
3. Inspect memory pressure: OOM events near the OCR worker could reduce concurrency.
4. Review soak test baseline: `ctest -L wave_d -R ImageAnalysisSoak_OCRThroughput --verbose`.

**Remediation:**
1. Restart Tesseract plugin backend if hung.
2. Scale OCR worker threads via `IMAGE_OCR_THREADS` config key.
3. If memory is the cause, increase worker JVM/process heap or reduce batch size.

**Escalation:** If throughput remains below 50% baseline after restart, escalate to image-analysis team.

---

## Scenario 2 — Object Detection Model Stall

**Symptom:** `[IMAGE:DetectionModelStall]` in logs; detection calls time out or return empty labels.

**Diagnosis steps:**
1. Check ONNX Runtime session status — session load errors appear in `themis.log`.
2. Verify model file integrity: `sha256sum models/yolov8n.onnx`.
3. Check GPU memory if running GPU inference: `nvidia-smi`.

**Remediation:**
1. Reload the ONNX session: trigger plugin lifecycle reload via admin API.
2. If GPU OOM, reduce batch size in `config/image_analysis.yaml` → `detection.batch_size`.
3. Fall back to CPU inference: set `detection.device=cpu` and restart.

**Escalation:** Persistent model corruption → re-download model artifact from registry.

---

## Scenario 3 — Plugin Load Failure

**Symptom:** `[IMAGE:PluginLoadFailed]` on startup or hot-reload; module fails to initialise.

**Diagnosis steps:**
1. Inspect plugin manifest: `cat plugins/image_analysis/manifest.json`.
2. Verify plugin signature matches the registered checksum.
3. Check `LD_LIBRARY_PATH` / dynamic linker for missing shared libs.

**Remediation:**
1. Restore plugin from artifact registry if manifest is corrupted.
2. Re-run signature validation: `themis-plugin-verify --plugin image_analysis`.
3. Ensure all runtime dependencies (`libonnxruntime`, `libtesseract`) are installed.

---

## Scenario 4 — Result Cache Eviction Storm

**Symptom:** `[IMAGE:CacheEvictionStorm]` — cache hit rate drops sharply; latency spikes.

**Diagnosis steps:**
1. Check cache metrics: `GET /metrics/image_analysis/cache_hit_rate`.
2. Verify cache max-size setting in `config/image_analysis.yaml` → `cache.max_entries`.
3. Identify if a new high-cardinality image set was ingested (large unique-key set).

**Remediation:**
1. Increase `cache.max_entries` to accommodate the new dataset.
2. Tune `cache.ttl_seconds` to reduce premature expiry.
3. If cache memory is bounded by system limits, partition cache by content type (OCR / detection).

---

## Scenario 5 — Backend Provider Degraded

**Symptom:** `[IMAGE:ProviderDegraded]` — primary backend (Tesseract or ONNX) returns errors; fallback provider activates.

**Diagnosis steps:**
1. Check primary backend error rate: `GET /metrics/image_analysis/backend_errors`.
2. Confirm fallback provider is active: look for `[IMAGE:FallbackActivated]` log entry.
3. Review recent deployments — a model update may have introduced a regression.

**Remediation:**
1. Pin to last known-good model version via config rollback.
2. Monitor fallback latency — secondary provider may be slower; alert if p95 > 2× baseline.
3. Once primary is restored, verify via health check before switching back.

---

## Log Pattern Reference

| Pattern | Level | Meaning |
|---|---|---|
| `[IMAGE:OCRThroughputDrop]` | WARN | OCR ops/s dropped below configured SLO |
| `[IMAGE:DetectionModelStall]` | ERROR | ONNX session timed out or returned empty results |
| `[IMAGE:PluginLoadFailed]` | ERROR | Plugin manifest/signature validation failed |
| `[IMAGE:CacheEvictionStorm]` | WARN | Cache hit rate < 60%; eviction rate spike |
| `[IMAGE:ProviderDegraded]` | ERROR | Primary backend degraded; fallback active |
| `[IMAGE:FallbackActivated]` | INFO | Switched to secondary backend provider |
| `[IMAGE:MixedSoak]` | DEBUG | Wave D soak mixed-workload event |

---

## Wave D Trace Cross-Reference

- Soak test: `tests/integration/test_image_analysis_soak.cpp` — covers D1 long-duration mixed workload
- Stress test: `tests/image_analysis/test_image_analysis_highcardinality_stress.cpp` — covers high-cardinality OCR + detection
- ROADMAP ref: `src/image_analysis/ROADMAP.md` — Wave D operability hardening item `[x]`
