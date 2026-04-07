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
| `IImageGenerationBackend::generateBatch(prompts, cfg)` | Batch API | Returns `vector<GeneratedImage>`; each entry independent |
| `ISDGenerator::generateImg2Img(image, prompt, cfg)` | img2img path | New optional method; mask parameter added for inpainting |
| `ISDGenerator::applyLoRA(lora_path, scale)` | LoRA hot-loading | Consistent with LLM module pattern |

---

## Planned Features

### 1. Real PNG Encoder — stb_image_write (Target: Q3 2026)

**Problem:** Stub encoder produces IHDR+IEND only; pixel data is not included.

**Solution:** Integrate `stb_image_write.h` (header-only) to encode the RGB byte buffer as
a proper PNG with IDAT chunk.

**Constraints:** Header-only; no CMake `find_library` required. Add `third_party/stb/` path.
**Tests:** Add 2 unit tests verifying PNG magic bytes and correct image dimensions via a
minimal PNG reader (read IHDR width/height fields).

---

### 2. SDCppGenerator — Real stable-diffusion.cpp (Target: Q3 2026)

**Problem:** `SDStubGenerator` returns black pixels; no real inference in v2.0.0.

**Solution:** Add `SDCppGenerator : ISDGenerator` that wraps `stable_diffusion_init()` /
`stable_diffusion_generate()` from stable-diffusion.cpp.

**Inputs:** `SDConfig` (model_path, sampler, steps, cfg_scale, seed), `SDGenerationConfig`.
**Outputs:** Raw RGB buffer.
**Constraints:** Compiled only when `THEMIS_ENABLE_STABLE_DIFFUSION=ON`; linker-guarded.
**Errors:** Model not found → `initialize()` returns `false`; generation failure → throw.
**Tests:** Integration test with a tiny 64×64 GGUF model file in CI fixtures.
**Perf target:** ≤ 10 s for 512×512, 20 steps on RTX 3090 or equivalent.

---

### 3. ControlNet Support (Target: Q4 2026)

**Problem:** No image conditioning available.

**Solution:** Extend `SDGenerationConfig` with `controlnet_image` (byte buffer) and
`controlnet_model_path`. `SDCppGenerator::generate()` passes these to
`stable_diffusion_generate_with_control()`.

**Inputs:** Reference image (RGB bytes), conditioning strength `[0.0, 1.0]`.
**Errors:** ControlNet model not found → fall back to text-to-image without conditioning;
caller is warned via `GeneratedImage::error_message` (non-fatal).
**Tests:** 3 unit tests with `InMemorySDGenerator` extended to accept ControlNet params.

---

### 4. Image-to-Image (img2img) (Target: Q4 2026)

**Problem:** No support for modifying existing images.

**Solution:** Add `generateImg2Img(prompt, input_image_png, cfg)` to `IImageGenerationBackend`.
`SDPlugin` decodes input PNG, passes it to `ISDGenerator::generateImg2Img()`.

**Constraints:** Input PNG must be decoded before passing to the generator; `stb_image.h`
will be used for decoding.
**Errors:** Invalid PNG → `success=false`, `error_message`.
**Tests:** 3 unit tests with preset input/output via `InMemorySDGenerator`.

---

### 5. Batch Generation (Target: Q4 2026)

**Problem:** Callers must loop and call `generate()` N times for batch jobs.

**Solution:** Add `generateBatch(prompts, cfg)` returning `vector<GeneratedImage>`.
Default implementation loops over `generate()`; `SDCppGenerator` may override with a
parallelised implementation.

**Perf target:** ≥ 4× throughput vs. sequential single calls for batch size ≥ 8 on GPU.
**Tests:** 2 unit tests (batch of 1, batch of 3) with `InMemorySDGenerator`.

---

## Security / Reliability

- All new image decoders (stb_image) must validate dimensions before allocation to
  prevent integer-overflow-based heap attacks.
- ControlNet model files must be subject to the same SHA-256 integrity check planned
  for the main model (Target: v2.1.0).
- `generateBatch` must apply `SDPromptSanitizer::isAllowed()` to every prompt
  independently before processing.
