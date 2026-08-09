> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-08-09 -->
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
| `ISDGenerator::applyLoRA(lora_path, scale)` | LoRA hot-loading | Implemented; request-time validation required |
| `SDGenerationConfig::control_*` fields | ControlNet path | Implemented; control-image shape/strength validation required |
| `GeneratedImage::perceptual_hash` | Dedup/Audit | Implemented; non-fatal if unavailable |

---

## Delivered Since Initial Baseline

- ControlNet request fields and validation are implemented in `SDGenerationConfig`/`Img2ImgConfig`
  and enforced in `SDPlugin`.
- LoRA hot-loading request path (`applyLoRA`) is implemented and validated.
- Optional `GeneratedImage::perceptual_hash` metadata is implemented with non-fatal fallback.
- Model integrity check (`model_sha256`) is implemented during `initialize()`.
- Dimension guards are implemented for request and output validation.

---

## Planned Features

### 1. Performance/Hardening Follow-ups (Target: Q3–Q4 2026)

**Problem:** Roadmap Phase 5 contains open hardening items.

**Solution:** Keep benchmark and concurrency-audit deliverables:
- benchmark target added: `bench_stable_diffusion_release_gates` for time-to-PNG (stub + in-memory proxy); publish real-model baseline next
- thread-safety audit target added in `SDPluginRealBackendE2ETests`; publish CI/runtime evidence next

**Tests:** Benchmark harness entry + stress tests gated for SD backend availability.

---

### 2. Real-backend E2E Automation (Target: Q3 2026)

**Problem:** Focused unit tests validate logic, but no dedicated stable_diffusion E2E gate
exists for real model execution in CI.

**Solution:** Maintain opt-in E2E target that runs with `THEMIS_ENABLE_STABLE_DIFFUSION=ON`,
a test model path, and SHA-256 verification enabled.

**Tests:** one text2img and one img2img E2E case asserting valid PNG output and metadata.

---

## Security / Reliability

- Any new image decoder path must validate dimensions before allocation to prevent
  integer-overflow-based heap attacks.
- Model integrity verification (SHA-256 policy) is enforced when `model_sha256` is provided.
- Any future batch override in `SDCppGenerator` must preserve per-prompt
  `SDPromptSanitizer::isAllowed()` checks.
