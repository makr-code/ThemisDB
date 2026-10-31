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

> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

# Stable Diffusion Plugin — Architecture Guide

<!-- Status: current | validated: 2026-10-31 | Primary: src/stable_diffusion/ -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

**Version:** 1.0
**Last Updated:** 2026-10-31
**Module Path:** `src/stable_diffusion/`

---

## 1. Overview

The Stable Diffusion plugin integrates [stable-diffusion.cpp](https://github.com/leejet/stable-diffusion.cpp)
into ThemisDB's plugin system as an `IImageGenerationBackend`. It converts text prompts into PNG
images with success-path provenance metadata and configurable keyword-based prompt filtering.

The plugin separates three orthogonal concerns:

1. **Content policy** — `SDPromptSanitizer` screens and sanitises prompts before inference.
2. **Inference** — `ISDGenerator` (strategy) decouples model execution from lifecycle.
3. **Provenance** — `SDPlugin` populates provenance fields along the request path; not every
   failure result currently has every field. **TODO:** reconcile this with the public interface
   header, which describes provenance fields as mandatory.

---

## 2. Design Principles

- **Policy-first** — prompts are screened before reaching the generator; blocked prompts
  never touch the model.
- **Stub-first** — `SDStubGenerator` is always available; stable-diffusion.cpp is optional.
- **Injection-friendly** — `SDPlugin(std::unique_ptr<ISDGenerator>, SDPromptSanitizer)`
  constructor enables full unit testing.
- **Provenance owned by the plugin** — `SDPlugin`, not the generator, populates provenance
  fields. `generation_timestamp` is currently assigned only after successful generation;
  prompt hashes are absent on some early error paths.
- **Non-cryptographic prompt fingerprint** — `prompt_hash` uses FNV-1a 64-bit. Optional
  model SHA-256 verification is separate and requires the build-time SHA-256 gate/OpenSSL.

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
  ├─ check initialized / SDPromptSanitizer::isAllowed(prompt)
  │                                      → if blocked: error + hash of original prompt
  ├─ check negative_prompt policy       → if blocked: error + hash of original prompt
  ├─ SDPromptSanitizer::sanitize(prompt)    → sanitised_prompt
  ├─ compute prompt_hash = FNV-1a(sanitised_prompt)
  ├─ validate dimensions / control inputs / LoRA scale
  ├─ optional applyLoRA(cfg.lora_adapter_path, cfg.lora_scale)
  ├─ ISDGenerator::generate(sanitised, cfg) → rgb_bytes, width, height, seed_used
  ├─ encodeMinimalPng(rgb, width, height)   → png_data
  ├─ computePerceptualHash(rgb, width, height) → optional perceptual_hash
  ├─ on success, set generation_timestamp and remaining result metadata
  └─ return GeneratedImage { success=true, png_data, prompt_hash, ... }
```

### 4.2 Error Paths

| Condition | Behaviour |
|-----------|-----------|
| Plugin not initialised | `success=false`; `plugin_version` set, no prompt hash or timestamp |
| Blocked prompt | `success=false`, `blocked_count++`; hash uses original prompt, no timestamp |
| Other early error | `success=false`; hash may be set after policy checks, no timestamp |
| Successful generation | `success=true`; hash uses sanitised prompt and timestamp is set |

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
| `model_sha256` | `""` | optional expected digest; enforced only when the SHA-256 build gate is enabled |

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
`generate_mutex_`; `SDCppGenerator` also guards its API calls with `api_mutex_`.
An opt-in real-backend parallel-generation test exists in
`src/stable_diffusion/tests/test_sd_plugin_real_backend_e2e.cpp`; execution against a
configured model/runtime is not established by the presence of that test.

## 9. Request Validation Gates

- positive dimensions only, max dimension `8192`
- overflow-safe RGB shape validation (`width * height * 3`)
- ControlNet image shape validation and `control_strength ∈ [0,1]`
- LoRA scale validation (`>0`, finite)

---

## 10. Testing Strategy

| Suite | Source | Cases / condition |
|---|---|---|
| Focused plugin tests | `src/stable_diffusion/tests/test_sd_plugin.cpp` | 62; CMake registration requires OpenSSL |
| Registrar tests | `src/stable_diffusion/tests/test_sd_plugin_registrar.cpp` | 12; CMake registration requires OpenSSL |
| Real-backend E2E | `src/stable_diffusion/tests/test_sd_plugin_real_backend_e2e.cpp` | 2; additionally requires the feature option, OpenSSL, and linked library |
| Modular focused tests | `tests/stable_diffusion/test_*.cpp` | 5 CTest targets discovered from the module CMake file; the high-cardinality test uses simulations, not `SDPlugin` |

## Module Dependencies

### Direct Upstream Dependencies (this module uses)
| Module | Interface / File | Purpose |
|--------|-----------------|---------|
| stable-diffusion.cpp | optional external library | Linked into `stable_diffusion_plugin` when enabled and found |

### Downstream Integration
| Consumer | Evidence | Notes |
|----------|----------|-------|
| PluginManager | `SDPluginAdapter` / `SDPluginRegistrar` | Adapter implements `IThemisPlugin` and provides registration helpers |
| server | TODO | Only generic image-generation plugin-type handling was found; direct Stable Diffusion registration/API use is unconfirmed |

## Integration Points

### Potential Server Integration (unverified)
The plugin exposes `IImageGenerationBackend` and a `PluginManager` adapter. The scoped source
does not establish a direct Stable Diffusion server registration or request path.
**TODO:** verify server integration before documenting a server-facing contract.

### Critical Integration: stable-diffusion.cpp Plugin Binding
**Files:** `src/stable_diffusion/` ↔ stable-diffusion.cpp (external)
**Contract:** Model and ControlNet weights are loaded at initialisation; LoRA adapters are applied per-request within validated scale bounds.
**Thread Safety:** `SDCppGenerator` serializes access to its context. The downstream
library's broader runtime/threading guarantees are not established here.
**TODO:** `include/stable_diffusion/sd_plugin.h` still describes real `SDCppGenerator`
img2img denoising as future work, while the current `sd_generator.h` implementation
provides an img2img override.
