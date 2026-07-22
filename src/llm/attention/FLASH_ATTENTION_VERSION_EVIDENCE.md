# Flash Attention SM90 Version Evidence (P0-D01)

Status: current  
Validated: 2026-07-22  
Source file: `/home/runner/work/ThemisDB/ThemisDB/src/llm/attention/cuda/flash_attention_cuda.cu`

## Scope

Audit objective: determine whether the SM90 path uses FlashAttention-3 specific Hopper primitives (TMA/WGMMA), or runs a non-FA3 kernel path.

## Findings

1. SM90 dispatch exists:
   - `FlashAttentionCUDA::forward()` routes to `launchKernelSM90()` when `compute_capability_ >= 90` and `enable_flash_v3` is set.
2. The effective SM90 kernel launch currently uses FP32 kernel path:
   - `launchKernelSM90()` launches `flash_attention_fwd_kernel_fp32(...)`.
3. No Hopper-specific FA3 primitives were found in this implementation:
   - No `wgmma` usage.
   - No TMA async copy path.
   - No SM90-only FA3 kernel specialization beyond dispatch naming/comments.

## Conclusion

Current SM90 runtime path in this file is **not an FA3-class kernel implementation**; it is a generic tiled kernel path (FP32 launch) executed on SM90 hardware.

## Acceptance Outcome for P0-D01

- [x] FA-version clarity for SM90 established.
- [x] Evidence documented with file-level references.

