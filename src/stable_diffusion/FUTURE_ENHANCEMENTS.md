---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone. Original creation dates
> could not be verified from available history; `Created` records this AI-assisted revision.

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
- Optional model integrity check (`model_sha256`) is implemented during `initialize()` when the
  SHA-256 build gate is enabled and OpenSSL is available.
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

### 2. Real-backend E2E verification

**Current state:** The opt-in `SDPluginRealBackendE2ETests` target exists and includes
text2img/img2img and parallel-generate cases. This document does not establish CI wiring or
an executed run with a configured model/runtime.

**Follow-up:** **TODO:** verify whether CI invokes this target; if not, wire it in and publish
execution evidence using `THEMIS_ENABLE_STABLE_DIFFUSION=ON` and a test model path.

**Tests:** the source currently contains text2img/img2img and parallel-generation cases; target
execution remains opt-in.

---

## Security / Reliability

- Any new image decoder path must validate dimensions before allocation to prevent
  integer-overflow-based heap attacks.
- Model integrity verification is enforced when `model_sha256` is provided and the
  `THEMIS_SD_ENABLE_SHA256` build gate is enabled with OpenSSL available; otherwise the digest
  is not checked.
- The public API header describes provenance fields as mandatory, but current result paths do
  not populate every field on failures. **TODO:** reconcile the implementation with that
  contract before treating failure-result provenance as guaranteed.
- Any future batch override in `SDCppGenerator` must preserve per-prompt
  `SDPromptSanitizer::isAllowed()` checks.
