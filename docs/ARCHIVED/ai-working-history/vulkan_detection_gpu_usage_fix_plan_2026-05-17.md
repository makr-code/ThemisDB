# Vulkan Detection & GPU Usage Fix Plan (2026-05-17)

## Scope
- Ensure Vulkan is enabled by default for non-MINIMAL editions unless explicitly overridden.
- Ensure modular LLM build uses real Vulkan LoRA kernels instead of simulation-only fallback.

## Files
- cmake/CMakeLists.txt
- cmake/ModularBuild.cmake

## Acceptance Criteria
- `THEMIS_ENABLE_VULKAN` defaults to ON at configure time (except explicit overrides / MINIMAL).
- Modular LLM source list includes `src/llm/lora_framework/kernels/vulkan_kernels.cpp`.
- `src/llm/lora_framework/kernels/vulkan_embedding_lookup.cpp` is not selected as primary modular Vulkan kernel path.
- Benchmark target `bench_vulkan_lora` builds successfully with `windows-bench-release` preset.

## Verification
- Rebuild `bench_vulkan_lora` with `cmake --build --preset windows-bench-release --target bench_vulkan_lora --parallel 4`.
- Re-check generated unity source list and/or build output for selected Vulkan kernel source file.
