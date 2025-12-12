# Phase 9: Real-world Testing

**Status:** In Progress  
**Goal:** Validate the DirectX11 3D Graph Renderer in production-like conditions with real datasets, capture telemetry, and close remaining gaps.

## Test Matrix
- **Datasets:**
  - Small (≤10k nodes / ≤20k edges)
  - Medium (50k–150k nodes / 100k–300k edges)
  - Large (250k–500k nodes / 500k–1M edges)
- **Hardware Tiers:**
  - Tier A: iGPU / low VRAM (≤2 GB)
  - Tier B: Mid GPU (4–8 GB)
  - Tier C: High GPU (≥10 GB)
- **Resolutions:** 1080p, 1440p, 4K
- **Quality Modes:** High (all effects), Balanced (shadows+SSAO on, parallax off), Performance (effects off, LOD aggressive)

## Scenarios
- **Navigation:** Pan/zoom/orbit through dense graph regions.
- **Filtering:** Apply node/edge filters and update visibility.
- **Animation:** Play layout transitions or time-series animation.
- **Selection:** Rapid selection and hover of nodes/edges.
- **Stress:** Burst edge additions/removals; high-frequency camera moves.

## Metrics to Capture
- Frametime P50/P95/P99 and FPS
- Draw calls before/after optimization; PSO cache hit rate
- GPU: VRAM usage, GPU utilization
- CPU: total and per-thread utilization, GC collections
- Memory: working set, LOH/Gen2 growth (leak check)
- Load times: shader warmup, first-frame latency

## Tools & Hooks
- Use `LoadTestRunner` (Phase 5) in Release to compare with/without optimization.
- Enable performance telemetry if `ProductionReleaseConfig.EnablePerformanceTelemetry` is true.
- Log optimizer metrics: `RenderOptimizationMetrics.Summary()` per run.
- Capture GPU/CPU traces (e.g., PIX, Windows Performance Recorder) where available.

## Execution Steps
1) Build Release: `dotnet build Themis.DocumentManager.csproj -c Release`
2) Warm-up: start renderer, preload shaders (GPUShaderCompiler) and caches.
3) Run load tests (Release): `dotnet test --filter LoadTest`
4) Run stress/memory tests: `dotnet test --filter StressTest|MemoryLeakTest`
5) Run manual scenarios on real datasets across tiers/resolutions/modes.
6) Record metrics, screenshots, and traces.

## Acceptance Criteria
- FPS ≥ target (configurable, default 60) for Small/Medium on Tier B+ in Balanced mode.
- No crashes or leaks after 30 min stress + memory test.
- Frametime P99 within 2× P50 for Medium on Tier B in Balanced mode.
- VRAM within tier budget; LOD/batch adapt to stay within budget.
- Shader warmup prevents first-frame hitching; no recurring hitches during interaction.

## Reporting
- Summarize per tier and dataset: FPS/frametime, GPU/CPU/memory, draw-call deltas.
- Flag regressions vs Phase 5 baselines.
- Provide tuning notes (LOD thresholds, batch sizes, effect toggles) per tier.
