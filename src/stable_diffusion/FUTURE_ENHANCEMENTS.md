<!-- Status: current | validated: 2026-04-07 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Future Enhancements — Stable Diffusion Plugin

## Scope

Planned enhancements beyond v2.0.0. Core implementation in `src/stable_diffusion/sd_plugin.cpp`,
`src/stable_diffusion/sd_prompt_sanitizer.cpp`, `src/stable_diffusion/sd_config.cpp`.

---

## Design Constraints

- `IImageGenerationBackend` interface must remain stable; new capabilities added as optional
  methods with default implementations.
- Content-policy screening (`SDPromptSanitizer::isAllowed`) must be called before every
  `ISDGenerator::generate` call — no code path may bypass it.
- Provenance stamps (`plugin_version`, `generation_timestamp`, `prompt_hash`) must always
  be applied by `SDPlugin`, never delegated to the generator.
- Stub mode (no model file) must continue to produce a valid `GeneratedImage` with
  `success=true` after any enhancement.
- FNV-1a hash for `prompt_hash` must remain the same algorithm across versions to keep
  hash values stable in audit logs.

---

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `ISDGenerator::applyLoRA(lora_path, scale)` | LoRA hot-loading | Optional API addition for runtime adapter switching |
| `SDGenerationConfig::controlnet_*` fields | ControlNet path | Backward-compatible additive config expansion |
| `GeneratedImage::perceptual_hash` | Dedup/Audit | Optional output metadata for duplicate detection |

---

## Planned Features

### 1. ControlNet Support (Target: Q4 2026)

**Problem:** No image conditioning available.

**Solution:** Extend `SDGenerationConfig` with `controlnet_image` (byte buffer) and
`controlnet_model_path`. `SDCppGenerator::generate()` passes these to
`stable_diffusion_generate_with_control()`.

**Inputs:** Reference image (RGB bytes), conditioning strength `[0.0, 1.0]`.
**Errors:** ControlNet model not found → fall back to text-to-image without conditioning;
caller is warned via `GeneratedImage::error_message` (non-fatal).
**Tests:** 3 unit tests with `InMemorySDGenerator` extended to accept ControlNet params.

---

### 2. LoRA Adapter Hot-Loading (Target: Q4 2026)

**Problem:** Runtime LoRA switching is not available in `ISDGenerator`.

**Solution:** Add optional `applyLoRA(lora_path, scale)` API for `SDCppGenerator`, with
no-op default in non-model generators.

**Errors:** Missing LoRA file → return failure with actionable error text.
**Tests:** Unit tests with mocked generator + integration test behind `THEMIS_ENABLE_STABLE_DIFFUSION`.

---

### 3. Perceptual Hash for Output Deduplication (Target: Q4 2026)

**Problem:** `prompt_hash` fingerprints text input only; identical outputs are not detectable.

**Solution:** Add optional `pHash` metadata for generated images to support deduplication and audits.

**Errors:** Hashing failure must not fail generation; return empty `pHash` plus warning in logs.
**Tests:** Deterministic pHash fixture tests over known image vectors.

---

### 4. Performance/Hardening Follow-ups (Target: Q1 2027)

**Problem:** Roadmap Phase 5 contains open hardening items.

**Solution:** Add benchmark and concurrency-audit deliverables:
- benchmark: time-to-PNG for 512×512 (stub vs real model)
- thread-safety audit for parallel `SDCppGenerator` usage

**Tests:** Benchmark harness entry + stress tests gated for SD backend availability.

---

## Security / Reliability

- Any new image decoder path must validate dimensions before allocation to prevent
  integer-overflow-based heap attacks.
- Model integrity verification (SHA-256 policy) remains required before production use
  in high-assurance deployments.
- Any future batch override in `SDCppGenerator` must preserve per-prompt
  `SDPromptSanitizer::isAllowed()` checks.
