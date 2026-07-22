# NVFP4 KV-Cache Capability Assessment (P0-D02)

Status: current  
Validated: 2026-07-22  
Primary scope:
- `/home/runner/work/ThemisDB/ThemisDB/include/llm/paged_kv_cache.h`
- `/home/runner/work/ThemisDB/ThemisDB/include/llm/mixed_precision_inference.h`
- `/home/runner/work/ThemisDB/ThemisDB/src/llm/paged_kv_cache.cpp`

## Objective

Assess whether the current KV-cache stack already supports NVFP4 storage, and define the minimal enablement step for P2-D01.

## Findings

1. `PagedKVCache::Config` previously had no quantization selector field.
2. `PagedKVCache` storage remains float-vector based in current implementation:
   - `store()` receives `std::vector<float>`.
   - `retrieve()` returns `std::vector<float>`.
3. `mixed_precision_inference.h` exposes generic precision modes (including `Q4`), but no explicit NVFP4 KV-cache contract for `PagedKVCache`.

## Implemented immediate step

- `PagedKVCache::Config` now includes:
  - `KVQuantizationType` enum: `FP16`, `INT8`, `NVFP4`
  - `kv_quantization` config field (default `FP16`)

This creates the integration contract needed for incremental P2-D01 wiring without changing default runtime behavior.

## Gap summary to close in P2-D01

- Quantized KV payload format in `PagedKVCache` (currently float payload).
- Serialization/deserialization and read/write path for NVFP4 blocks.
- Accuracy and regression tests for FP16 vs NVFP4.
- Backend capability wiring for runtime support checks.

## Decision

P0-D02 is **sufficiently positive** to proceed with P2-D01 implementation:
- configuration/API groundwork is now present,
- runtime quantized storage path remains pending by design for Phase 2.

