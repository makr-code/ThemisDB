<!-- Status: current | validated: 2026-10-07 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Image Analysis Module - Future Enhancements

## Scope
- Extend the plugin-based OCR and detection stack without breaking the shared `include/plugins/` contract.
- Add optional video frame analysis and model registry support behind explicit build/runtime flags.
- Improve performance evidence and reliability for the Tesseract and YOLOv8 plugin paths.
- Keep all planned improvements tied to the existing `ImageAnalysisManager` + `IImageAnalysisBackend` design.

## Design Constraints
- [ ] New backend plugins must implement `IImageAnalysisBackend` and remain compatible with the canonical interfaces in `include/plugins/` (Target: ongoing)
- [ ] Native backend dependencies must remain behind explicit build flags (`HAVE_TESSERACT`, `HAVE_ONNXRUNTIME`) and must not become silent runtime assumptions (Target: ongoing)
- [ ] Any embedding or detection output schema change must be versioned and aligned to `PERFORMANCE_EXPECTATIONS.md` before merge (Target: ongoing)
- [ ] Security and backend-isolation defaults must remain fail-closed; all new backends must degrade gracefully under dependency or inference failure (Target: ongoing)
- [ ] Performance gates for OCR and detection must remain documented and revalidated before any GA claim (Target: ongoing)

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `IImageAnalysisBackend::detectObjects()` | runtime analysis callers | must remain compatible with structured result returns |
| `IImageAnalysisBackend::generateEmbedding()` | embedding-capable consumer paths | must be explicit about feature dimension and compatibility |
| `ImageAnalysisManager::getBestPluginForCapability()` | selector logic | routing must prefer valid plugin capabilities over stale fallback assumptions |
| `PluginConfig` | all backends | configuration keys must be documented and versioned |

## Implementation Notes

### Additional OCR Engine Support
**Priority:** Medium
**Target:** Q2 2027

- Add alternative OCR backends via the existing plugin mechanism without changing the shared `IImageAnalysisBackend` contract.
- Gate activation on build flags analogous to `HAVE_TESSERACT` and require explicit error handling when the dependency is absent.
- Ensure timeout and confidence semantics remain compatible with the current `TesseractOCRPlugin` behaviour.

### Video Frame Analysis
**Priority:** Low
**Target:** Q3 2027

- Introduce an optional video pipeline that analyses sampled frames through the existing `IImageAnalysisBackend` contract.
- Keep frame extraction and analysis decoupled from static image paths to avoid shared mutable state.
- Define maximum sampling rate and CPU limits before enabling the feature in production.

### Custom Model Pipeline
**Priority:** Low
**Target:** Q3 2027

- Define a model registry or validation layer before shipping custom ONNX models into the runtime path.
- Require integrity verification for non-default model files and explicit rollback semantics.
- Keep the default path fail-closed when model validation fails.

### Multimodal Embedding Improvement
**Priority:** Medium
**Target:** Q2 2027

- Extend embedding generation through well-defined plugin capabilities rather than hidden index assumptions.
- Keep the output dimension and compatibility contract explicit for downstream vector consumers.
- Add parity tests when a new embedding backend is introduced.

### Async Processing Queue
**Priority:** Medium
**Target:** Q4 2026

- Add optional asynchronous analysis queue behind explicit configuration.
- Document queue depth, back-pressure, retry policy, and timeout semantics before turning it on in production.
- Continue to guarantee graceful degradation when a backend is overloaded or unavailable.

## Test Strategy
- Add regression tests covering plugin manager selection logic and capability routing.
- Add negative tests for absent dependencies and malformed image payloads.
- Extend benchmark coverage for OCR and detection under the actual release profile.
- Keep parity checks whenever a new plugin backend changes the result schema or output dimensions.

## Performance Targets
- OCR path must remain within the documented P99 gate in `PERFORMANCE_EXPECTATIONS.md` after any backend change.
- Detection path must remain within the documented P99 gate after any model or backend change.
- Any new feature that increases memory or latency must include a re-baseline in `PERFORMANCE_EXPECTATIONS.md` before merge.

## Security / Reliability
- All new backends must fail with explicit structured errors rather than crashing or silently returning stale data.
- Any custom model path must validate integrity before activation.
- Video and file-based processing must reject unsafe paths or malformed payloads before invocation.
- New features must remain within the same resource model as the existing plugin life cycle.
