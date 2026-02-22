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

- A. Radford et al., "Learning transferable visual models from natural language supervision," in *Proc. 38th International Conf. Machine Learning (ICML)*, 2021, pp. 8748–8763. arXiv:2103.00020. [https://arxiv.org/abs/2103.00020](https://arxiv.org/abs/2103.00020)
- A. Kirillov et al., "Segment anything," in *Proc. IEEE/CVF International Conf. Computer Vision (ICCV)*, 2023, pp. 4015–4026. DOI: [10.1109/ICCV51070.2023.00371](https://doi.org/10.1109/ICCV51070.2023.00371)
- J. Li et al., "BLIP-2: Bootstrapping language-image pre-training with frozen image encoders and large language models," in *Proc. 40th International Conf. Machine Learning (ICML)*, 2023, pp. 19730–19742. arXiv:2301.12597. [https://arxiv.org/abs/2301.12597](https://arxiv.org/abs/2301.12597)
- K. He, X. Zhang, S. Ren, and J. Sun, "Deep residual learning for image recognition," in *Proc. IEEE Conf. Computer Vision and Pattern Recognition (CVPR)*, 2016, pp. 770–778. DOI: [10.1109/CVPR.2016.90](https://doi.org/10.1109/CVPR.2016.90)
- J. Redmon, S. Divvala, R. Girshick, and A. Farhadi, "You only look once: Unified, real-time object detection," in *Proc. IEEE Conf. Computer Vision and Pattern Recognition (CVPR)*, 2016, pp. 779–788. DOI: [10.1109/CVPR.2016.91](https://doi.org/10.1109/CVPR.2016.91)
