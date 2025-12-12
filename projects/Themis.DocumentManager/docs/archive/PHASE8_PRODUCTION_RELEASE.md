# Phase 8: Production Release

**Status:** In Progress  
**Goal:** Ship-ready configuration, hardening, and release checklist for the DirectX 11 3D Graph Renderer stack.

## Release Checklist (DX11 Stack)
- **Build configuration:** `Release`, `net8.0-windows`, `UseWPF=true`, trim unused features off.
- **Feature toggles:** Enable advanced optimization and GPU effects; keep debug visualizers off.
- **Logging:** Set to warning/error, disable verbose frame logs.
- **GPU requirements:** D3D11 feature level 11_0+, Shader Model 5.0, 2GB+ VRAM recommended.
- **Tests:** Run Phase 5 load/stress/memory tests; capture metrics against Phase 7 optimizer.
- **Assets:** Precompile shaders, warm pipeline state cache if applicable.
- **Telemetry:** Enable frame timing + optimization metrics, disable chatty debug output.

## Production Defaults (Render)
- **Optimization:** On (frustum culling, LOD, instancing, PSO cache)
- **Effects:** On (shadows, SSAO, normal/parallax) where performance budget allows
- **Quality guardrail:** Target ≥60 FPS; drop to lower LOD when under budget
- **Batch size:** 2048 instances (tune per GPU)

## Commands
```
# Build Release
powershell -NoLogo -Command "cd C:\VCC\themis\projects\Themis.DocumentManager; dotnet build Themis.DocumentManager.csproj -c Release"

# Run Performance Suite (Debug/Release)
powershell -NoLogo -Command "cd C:\VCC\themis\projects\Themis.DocumentManager; dotnet test --filter LoadTest"
```

## Integration Points
- Wire `ProductionReleaseConfig` into renderer bootstrap to toggle optimization/effects/logging.
- Export optimizer metrics into HUD/telemetry overlay for runtime observability.
- Preload shaders via `GPUShaderCompiler` and cache bytecode (avoid first-frame hitches).
- Invoke `AdvancedOptimizationEngine` before command submission (already integrated in Phase 7).

## Acceptance Criteria
- Build succeeds in `Release` configuration.
- Phase 5 performance tests pass with no regressions.
- FPS ≥60 on target hardware with medium-sized graphs; graceful LOD drop when budget exceeded.
- No blocking warnings/errors in build output; logging at warning/error only.
- Documentation updated (Phase 1-8 complete).

## Next (Phase 9 Preview)
- Field tests with real datasets.
- Capture GPU/CPU traces under production conditions.
- Tune thresholds per hardware tier.
