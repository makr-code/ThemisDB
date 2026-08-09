# stable_diffusion ROADMAP (Secondary Mirror)

This file is a level4 publication mirror and is **not** canonical.

- Canonical roadmap: `/home/runner/work/ThemisDB/ThemisDB/src/stable_diffusion/ROADMAP.md`
- Production gates: `/home/runner/work/ThemisDB/ThemisDB/src/stable_diffusion/PRODUCTION_REQUIREMENTS.md`

## Snapshot (2026-08-09)

- [x] ControlNet request handling implemented and covered in focused tests.
- [x] LoRA request handling implemented and covered in focused tests.
- [x] Perceptual hash metadata behavior implemented (`GeneratedImage::perceptual_hash`).
- [x] Model SHA-256 integrity check implemented via `model_sha256` initialize gate.
- [x] Dimension guards implemented (`<=8192`, overflow-safe).
- [x] Focused suite present: `SDPluginFocusedTests` (62 tests).
- [~] Benchmark gate in progress (`bench_stable_diffusion_release_gates` added; baseline publication pending).
- [ ] Real-model E2E gate pending (no dedicated stable_diffusion E2E target yet).
