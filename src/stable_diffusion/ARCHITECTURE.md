> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

# Stable Diffusion Plugin — Architecture Guide

<!-- Status: current | validated: 2026-08-09 | Primary: src/stable_diffusion/ -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

**Version:** 1.0
**Last Updated:** 2026-08-09
**Module Path:** `src/stable_diffusion/`

---

## 1. Overview

The Stable Diffusion plugin integrates [stable-diffusion.cpp](https://github.com/leejet/stable-diffusion.cpp)
into ThemisDB's plugin system as an `IImageGenerationBackend`. It converts text prompts into PNG
images with mandatory provenance metadata and built-in content-policy enforcement.

The plugin separates three orthogonal concerns:

1. **Content policy** — `SDPromptSanitizer` screens and sanitises prompts before inference.
2. **Inference** — `ISDGenerator` (strategy) decouples model execution from lifecycle.
3. **Provenance** — `SDPlugin` applies `plugin_version`, `generation_timestamp`, and
   `prompt_hash` unconditionally on every result path.

---

## 2. Design Principles

- **Policy-first** — prompts are screened before reaching the generator; blocked prompts
  never touch the model.
- **Stub-first** — `SDStubGenerator` is always available; stable-diffusion.cpp is optional.
- **Injection-friendly** — `SDPlugin(std::unique_ptr<ISDGenerator>, SDPromptSanitizer)`
  constructor enables full unit testing.
- **Provenance unconditional** — timestamps and prompt hashes applied in `SDPlugin`, not
  delegated to the generator.
- **Zero libcrypto dependency** — `prompt_hash` uses FNV-1a 64-bit (stable fingerprint;
  not cryptographic).

---

## 3. Component Diagram

```
┌──────────────────────────────────────────────────────────────────┐
│  IImageGenerationBackend  (include/plugins/image_generation_interface.h) │
└───────────────────────────┬──────────────────────────────────────┘
                            │ implements
                ┌───────────▼──────────────────┐
                │         SDPlugin             │
                │  ┌─────────────────────────┐ │
                │  │   SDPromptSanitizer      │ │  content-policy
                │  └─────────────────────────┘ │
                │  ┌─────────────────────────┐ │
                │  │     ISDGenerator         │ │  ← strategy
                │  │  ┌──────────────────┐   │ │
                │  │  │ SDStubGenerator  │   │ │  (CI / no model)
                │  │  ├──────────────────┤   │ │
                │  │  │ InMemorySDGen.   │   │ │  (test double)
                │  │  ├──────────────────┤   │ │
                │  │  │ (SDCppGenerator) │   │ │  (real model, optional)
                │  │  └──────────────────┘   │ │
                │  └─────────────────────────┘ │
                └──────────────────────────────┘
```

---

## 4. Key Data Flows

### 4.1 generate(prompt, cfg)

```
SDPlugin::generate(prompt, cfg)
  ├─ SDPromptSanitizer::isAllowed(prompt)   → if blocked: return error result
  ├─ SDPromptSanitizer::sanitize(prompt)    → sanitised_prompt
  ├─ compute prompt_hash = FNV-1a(sanitised_prompt)
  ├─ validate dimensions / control inputs / LoRA scale
  ├─ optional applyLoRA(cfg.lora_adapter_path, cfg.lora_scale)
  ├─ ISDGenerator::generate(sanitised, cfg) → rgb_bytes, width, height, seed_used
  ├─ encodeMinimalPng(rgb, width, height)   → png_data
  ├─ computePerceptualHash(rgb, width, height) → optional perceptual_hash
  ├─ apply provenance stamps
  └─ return GeneratedImage { success=true, png_data, prompt_hash, ... }
```

### 4.2 Error Paths

| Condition | Behaviour |
|-----------|-----------|
| Plugin not initialised | `success=false` |
| Blocked prompt | `success=false`, `blocked_count++` |
| Generator throws | `success=false`, `error_count++`, `error_message=exception.what()` |
| All success paths | `success=true`, provenance stamps set |

---

## 5. Configuration (`SDConfig`)

| Field | Default | Constraint |
|---|---|---|
| `model_path` | `""` | path to model file |
| `width` | `512` | clamped to `[1, ∞)` |
| `height` | `512` | clamped to `[1, ∞)` |
| `steps` | `20` | clamped to `[1, ∞)` |
| `cfg_scale` | `7.0` | classifier-free guidance |
| `sampler` | `"euler_a"` | sampler name |
| `seed` | `-1` | `-1` = random |
| `blocked_keywords_file` | `""` | path to keyword blocklist |
| `negative_prompt` | `""` | negative conditioning |
| `model_sha256` | `""` | optional expected digest for model file integrity gate |

---

## 6. Content Policy (`SDPromptSanitizer`)

- Keywords are stored lower-case; matching is case-insensitive.
- `isAllowed(prompt)` returns `false` if any keyword is found in the prompt.
- `sanitize(prompt)` removes all occurrences of blocked keywords.
- Loaded from file via `SDPromptSanitizer::fromFile(path)` (one keyword per line, `#` = comment).

---

## 7. PNG Encoding

v2.2.0 includes a real PNG encoder that writes IHDR + IDAT + IEND using stored-deflate
(zlib stream), CRC-32, and Adler-32 in `SDPlugin::encodeMinimalPng`.

---

## 8. Thread Safety

`SDPlugin` serialises `generate()`, `generateBatch()`, and `generateImg2Img()` with
`generate_mutex_`. A dedicated parallel-call audit for `SDCppGenerator` is still pending.

## 9. Request Validation Gates

- positive dimensions only, max dimension `8192`
- overflow-safe RGB shape validation (`width * height * 3`)
- ControlNet image shape validation and `control_strength ∈ [0,1]`
- LoRA scale validation (`>0`, finite)

---

## 10. Testing Strategy

| Type | Files | Count |
|---|---|---|
| Unit (stub mode + core backend paths) | `src/stable_diffusion/tests/test_sd_plugin.cpp` | 62 |
