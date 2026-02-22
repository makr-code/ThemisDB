# Image Analysis Plugin – Future Enhancements

> **Note:** Items here are *not* on the active roadmap. They are ideas and backlog entries for later consideration. See [`roadmap.md`](roadmap.md) for committed near-term work.

---

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

## Research / References

- [ ] TODO: Add reference – *CLIP: Learning Transferable Visual Models* – arXiv:2103.00020
- [ ] TODO: Add reference – *YOLOv8: Real-Time Object Detection* (URL placeholder)
- [ ] TODO: Add reference – *Segment Anything Model (SAM)* – arXiv:2304.02643
- [ ] TODO: Add reference – *BLIP-2: Bootstrapping Language-Image Pre-training* – arXiv:2301.12597
