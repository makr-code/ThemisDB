# Image Analysis Plugin – Future Enhancements

> **Note:** Items here are *not* on the active roadmap. They are ideas and backlog entries for later consideration. See [`roadmap.md`](roadmap.md) for committed near-term work.

---

## Scope

- Enhancements to the ONNX CLIP image embedding plugin: new model support (BLIP-2, Florence-2, DepthAnything), multi-modal fusion (image + text), batch inference optimisation, and infrastructure hardening.
- Entry-point: `plugins/image_analysis/onnx_clip/CMakeLists.txt` (compatibility shim) · canonical implementation: `src/onnx_clip/`.
- Out of scope: training or fine-tuning models; this plugin only handles inference and embedding generation.
- Covers model registry management, dynamic model loading, and adversarial input sanitisation.

## Design Constraints

- [ ] All model files MUST be validated against a SHA-256 manifest before the ONNX Runtime session is created.
- [ ] Image inputs MUST be sanitised to reject files with malformed headers, excessively large dimensions (> 10,000 × 10,000 px), or pixel value outliers indicative of adversarial perturbation.
- [ ] ONNX Runtime session creation MUST complete within 2 s per model on first load; subsequent inferences use the cached session.
- [ ] Batch inference MUST process images in configurable batch sizes (default 64); batch size must not exceed available VRAM minus 512 MB headroom.
- [ ] Model loading and unloading MUST be thread-safe; concurrent inference requests MUST not cause data races.
- [ ] GPU execution provider (CUDA EP) MUST fall back to CPU EP automatically if GPU is unavailable, with a logged warning.

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `IImageEmbedder` | `ImageAnalysisPlugin`, ThemisDB vector index | `embed(image_bytes) → float[]`, `embed_batch(images) → float[][]` |
| `IModelRegistry` | `ImageAnalysisPlugin` | `load(model_id, path)`, `unload(model_id)`, `list()` |
| `IImageSanitizer` | `IImageEmbedder` impls | Validates dimensions, format, pixel statistics before inference |
| `IModelManifest` | `IModelRegistry` | JSON manifest with model name, SHA-256, input shape, embedding dim |
| `IBatchInferenceScheduler` | `ImageAnalysisPlugin` | Accumulates single-image requests into optimal batches |

## Idea Backlog

### New Model Types

- [ ] **Depth estimation** (e.g., MiDaS, DepthAnything) – per-pixel depth maps.
- [ ] **Semantic segmentation** (SegFormer, SAM) – pixel-level class masks.
- [ ] **Face recognition / verification** (ArcFace via ONNX) – identity matching.
- [ ] **Image quality assessment** (BRISQUE, NIQE) – automatic quality scoring.
- [ ] **Anomaly detection** (PatchCore, EfficientAD) – detect out-of-distribution images.

### Multi-modal

- [ ] **Audio-visual embedding** – joint embedding of video frames + audio (e.g., ImageBind).
- [ ] **Document understanding** (LayoutLM, Donut) – structured extraction from document images.
- [ ] **Medical imaging** – DICOM reader + radiology-specific models.

### Infrastructure

- [ ] **TensorRT engine caching** – serialise optimised TRT engines for faster startup.
- [ ] **ONNX model optimisation pipeline** – auto-apply graph optimisations via `onnxoptimizer`.
- [ ] **Remote inference backend** – delegate inference to a Triton server.
- [ ] **Model registry** – version-controlled model store integrated with ThemisDB storage.

### Developer Experience

- [ ] **Visualisation tool** – CLI to visualise embeddings (t-SNE / UMAP projection).
- [ ] **Benchmark script** – measure throughput and latency per backend automatically.

---

## Test Strategy

- Unit tests for `IImageSanitizer`: assert rejection of oversized images, malformed JPEG/PNG headers, and known adversarial pixel patterns.
- SHA-256 manifest tests: assert that loading a model with a tampered manifest file fails with `MODEL_CHECKSUM_MISMATCH`.
- Embedding correctness tests: CLIP embedding of a reference image set must reproduce cosine-similarity rankings within ± 0.001 of a known-good baseline.
- Batch inference tests: 64-image batch must produce identical embeddings to 64 sequential single-image calls.
- GPU fallback tests: disable CUDA EP; assert plugin initialises on CPU EP and logs a `GPU_UNAVAILABLE` warning.
- Throughput benchmark: 1,000-image batch on GPU must complete in ≤ 2.5 s (≥ 400 images/s); recorded in CI as a regression gate.

## Performance Targets

- CLIP embedding latency ≤ 10 ms/image (CPU, ViT-B/32); ≤ 2 ms/image (GPU, RTX 3080 or equivalent).
- Batch inference throughput ≥ 400 images/s at batch size 64 on GPU.
- ONNX Runtime session creation ≤ 2 s per model on first load (warm cache thereafter ≤ 50 ms).
- Memory overhead per loaded model ≤ 500 MB RAM (CPU) / ≤ 500 MB VRAM (GPU) for ViT-B/32.
- `IImageSanitizer` check overhead ≤ 1 ms per image.

## Security / Reliability

- Model files MUST be validated via SHA-256 manifest before ONNX Runtime session creation; mismatches abort loading.
- Image inputs MUST be sanitised to reject adversarial inputs: maximum pixel L∞ norm check, dimension bounds, and format header validation.
- No raw image bytes or model weights may be logged at any log level.
- GPU VRAM allocation failures MUST trigger automatic fallback to CPU EP with a warning, not a crash.
- Model unload (`IModelRegistry::unload`) MUST release all ONNX Runtime sessions and free VRAM before returning.

## Research / References

- A. Radford et al., "Learning transferable visual models from natural language supervision," in *Proc. 38th International Conf. Machine Learning (ICML)*, 2021, pp. 8748–8763. arXiv:2103.00020. [https://arxiv.org/abs/2103.00020](https://arxiv.org/abs/2103.00020)
- A. Kirillov et al., "Segment anything," in *Proc. IEEE/CVF International Conf. Computer Vision (ICCV)*, 2023, pp. 4015–4026. DOI: [10.1109/ICCV51070.2023.00371](https://doi.org/10.1109/ICCV51070.2023.00371)
- J. Li et al., "BLIP-2: Bootstrapping language-image pre-training with frozen image encoders and large language models," in *Proc. 40th International Conf. Machine Learning (ICML)*, 2023, pp. 19730–19742. arXiv:2301.12597. [https://arxiv.org/abs/2301.12597](https://arxiv.org/abs/2301.12597)
- K. He, X. Zhang, S. Ren, and J. Sun, "Deep residual learning for image recognition," in *Proc. IEEE Conf. Computer Vision and Pattern Recognition (CVPR)*, 2016, pp. 770–778. DOI: [10.1109/CVPR.2016.90](https://doi.org/10.1109/CVPR.2016.90)
- J. Redmon, S. Divvala, R. Girshick, and A. Farhadi, "You only look once: Unified, real-time object detection," in *Proc. IEEE Conf. Computer Vision and Pattern Recognition (CVPR)*, 2016, pp. 779–788. DOI: [10.1109/CVPR.2016.91](https://doi.org/10.1109/CVPR.2016.91)
