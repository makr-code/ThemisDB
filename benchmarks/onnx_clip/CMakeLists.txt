# Benchmarks for ONNX CLIP v0.3.0 — Phase 2A: Benchmark Framework & Fixtures
# SPDX-License-Identifier: Apache-2.0

# Gate: only build if benchmark support is enabled
if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: onnx_clip")

# CPU latency benchmarks (Phase 2A)
themis_add_standard_benchmark(
    bench_onnx_clip_cpu
    bench_onnx_clip_cpu.cpp
    DESCRIPTION "ONNX CLIP v0.3.0 CPU latency benchmarks (Phase 2A)"
)

# Multi-backend throughput and memory benchmarks (Phase 2A)
themis_add_standard_benchmark(
    bench_onnx_clip_vit_backend
    bench_onnx_clip_vit_backend.cpp
    DESCRIPTION "ONNX CLIP v0.3.0 multi-backend throughput benchmarks (Phase 2A)"
)

# Optional: Link against ONNX plugin if available
if(TARGET themis_image_onnx_clip)
    message(STATUS "    Linking ONNX CLIP benchmarks to plugin target")
    target_link_libraries(bench_onnx_clip_cpu PRIVATE themis_image_onnx_clip)
    target_link_libraries(bench_onnx_clip_vit_backend PRIVATE themis_image_onnx_clip)
endif()

message(STATUS "ONNX CLIP benchmarks configuration complete")

