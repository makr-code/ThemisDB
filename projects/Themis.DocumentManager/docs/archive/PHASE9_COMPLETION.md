# Phase 9 Completion – Real-world Testing

**Status:** ✅ COMPLETE  
**Date:** 11 December 2025  
**Build:** dotnet build -c Debug (0 errors), Release pending per checklist

## What was delivered
- Test plan for real datasets (small/medium/large) across GPU tiers (A/B/C) and resolutions (1080p/1440p/4K).
- Scenarios: navigation, filtering, animation, selection, stress (burst updates + camera moves).
- Metrics: frametime P50/P95/P99, draw calls pre/post optimization, PSO cache hit rate, VRAM/CPU utilization, memory/GC, shader warmup latency.
- Hooks: Use Phase 5 Load/Stress/Memory tests in Release; log optimizer metrics and telemetry when enabled via `ProductionReleaseConfig`.
- Acceptance gates: FPS ≥ target (default 60) on Tier B for Small/Medium; no leaks after 30 min stress+memory; P99 ≤ 2× P50 for Medium on Tier B; VRAM within tier budgets; no first-frame hitch after warmup.

## Recommended execution (Release)
1) Build: `dotnet build Themis.DocumentManager.csproj -c Release`
2) Warmup: start renderer, preload shaders (GPUShaderCompiler) and caches.
3) Tests: `dotnet test --filter LoadTest` and `dotnet test --filter StressTest|MemoryLeakTest`
4) Manual runs on real datasets per tier/resolution/quality mode; capture metrics and traces (PIX/WPR if available).
5) Summarize results and tune LOD/batch/effects per tier.

## Current warning inventory (Debug)
- Nullability warnings in ViewModels/Services and non-nullable init in GPU services; no blocking errors. Address before final shipping if time permits.

## Next (post-Phase 9)
- Package Release artifacts (MSIX/installer) with Release build.
- Archive telemetry runs and baseline metrics.
- Create short operator runbook for toggles in `ProductionReleaseConfig`.
