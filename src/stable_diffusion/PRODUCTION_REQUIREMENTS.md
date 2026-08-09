<!--
Last Updated: 2026-08-09
Source Level: level1
SOT Domain: module-behavior
Canonical Sources:
- /home/runner/work/ThemisDB/ThemisDB/src/stable_diffusion/sd_plugin.cpp
- /home/runner/work/ThemisDB/ThemisDB/include/stable_diffusion/sd_generator.h
- /home/runner/work/ThemisDB/ThemisDB/include/plugins/image_generation_interface.h
- /home/runner/work/ThemisDB/ThemisDB/src/stable_diffusion/tests/test_sd_plugin.cpp
Related Milestone: DOC-WEEKLY-2026-W32
-->

# Stable Diffusion Production Requirements

## Feature Gates

- [x] **ControlNet request handling**
  - `SDGenerationConfig` / `Img2ImgConfig` must carry `control_image_rgb`, `control_width`, `control_height`, `control_model_path`, `control_strength`.
  - Invalid control image shape or strength outside `[0.0, 1.0]` must fail request.
  - Evidence: `sd_plugin.cpp` validation + focused tests `N4`, `N6`.

- [x] **LoRA request handling**
  - `lora_adapter_path` + `lora_scale` must be validated (`scale > 0`, finite).
  - LoRA apply failure must fail request with actionable error.
  - Evidence: `sd_plugin.cpp` apply path + focused tests `N5`, `N4`.

- [x] **pHash metadata behavior**
  - Successful generations must attempt `GeneratedImage::perceptual_hash`.
  - pHash computation failure must be non-fatal.
  - Evidence: `sd_plugin.cpp::computePerceptualHash` + focused tests `P4`, `P5`.

- [x] **Model SHA-256 integrity check**
  - `initialize(model_path, config)` must reject model when `model_sha256` does not match computed SHA-256.
  - Evidence: `sd_plugin.cpp` initialization gate + focused tests `J4`, `J5`.

- [x] **Dimension guards**
  - Reject non-positive dimensions, dimensions above `8192`, and overflow-risk image sizes.
  - Apply same guard set to generation output validation and img2img/control input validation.
  - Evidence: `validateGenerationDimensions`, `validateRgbBufferShape`, focused test `K4`.

## Verification Gates

- [x] **Focused test gate**
  - `SDPluginFocusedTests` must pass (62 tests in `test_sd_plugin.cpp`).
  - `SDPluginRegistrarTests` must pass (12 tests in `test_sd_plugin_registrar.cpp`).
  - Latest local evidence (2026-08-09): both suites pass on `community-release-allow-missing-rocksdb`
    with `THEMIS_AUTO_BOOTSTRAP_DEPS=ON`.

- [~] **Benchmark gate**
  - A stable_diffusion benchmark target must exist under `/home/runner/work/ThemisDB/ThemisDB/benchmarks/stable_diffusion/`.
  - Required release criterion: publish baseline (latency + memory) for stub and real backend.
  - Current state: benchmark target exists (`bench_stable_diffusion_release_gates`); baseline publication remains open and
    the local build is currently blocked outside this module by missing `rocksdb/db.h` in
    `src/storage/backup_manager.cpp` plus an existing compile error in `src/storage/rocksdb_wrapper.cpp`.

- [~] **E2E gate**
  - Real-backend E2E must run with `THEMIS_ENABLE_STABLE_DIFFUSION=ON` and a verified model file.
  - Required checks: initialization success with matching SHA-256, one text2img and one img2img path returning valid PNG.
  - Current state: dedicated target `SDPluginRealBackendE2ETests` is present (opt-in via `THEMIS_SD_E2E_MODEL_PATH`); CI wiring and evidence publication remain open.
