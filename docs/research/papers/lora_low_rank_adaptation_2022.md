# LoRA: Low-Rank Adaptation of Large Language Models

**Metadaten:**
- Author(en): Edward J. Hu, Yelong Shen, Phillip Wallis, Zeyuan Allen-Zhu, Yuanzhi Li, Shean Wang, Lu Wang, Weizhu Chen
- Konferenz/Journal: 10th International Conference on Learning Representations (ICLR 2022)
- Jahr: 2022
- Link: [OpenReview](https://openreview.net/forum?id=nZeVKeeFYf9) · [arXiv:2106.09685](https://arxiv.org/abs/2106.09685)
- Zitierweise: `hu2022lora`
- Tags: `lora`, `peft`, `fine-tuning`, `low-rank`, `adapter`, `llm`, `parameter-efficient`
- ThemisDB-Versionen: v1.3.0+ (implemented in `src/llm/lora/` and `src/training/`)
- Status: [x] Fully Implemented

## 📋 Executive Summary

LoRA freezes the pre-trained model weights and injects trainable rank-decomposition matrices into the attention layers: for a weight matrix `W₀ ∈ R^{d×k}`, LoRA adds `ΔW = BA` where `B ∈ R^{d×r}`, `A ∈ R^{r×k}`, and rank `r ≪ min(d, k)`. Only A and B are trained (≈0.1–1% of original parameters). At inference, the LoRA delta is merged into W₀ with zero overhead. This enables fine-tuning of 7B–70B models on a single GPU while preserving full model quality — the foundational technique for ThemisDB's legal-domain model adaptation.

Directly referenced in `src/training/FUTURE_ENHANCEMENTS.md` [1] and `src/llm/FUTURE_ENHANCEMENTS.md` [9].

## 🎯 Key Findings

- **Parameter efficiency**: Training GPT-3 with r=4 uses 10,000× fewer trainable parameters than full fine-tuning; VRAM usage drops from 1,200 GB to 35 GB.
- **No inference latency**: W₀ + BA can be pre-merged; zero overhead vs. original model at inference.
- **Rank r**: r=4–16 sufficient for most tasks; r=64 for complex domain adaptation; higher r has diminishing returns.
- **Target modules**: Applying LoRA to Wq and Wv (query/value projections) captures most task-specific information; Wk and Wo yield diminishing returns.
- **Task-specific quality**: LoRA matches full fine-tuning on GLUE benchmarks; surpasses adapter tuning and prefix tuning in training stability.
- **Composability**: Multiple LoRA adapters (e.g., legal + German language) can be merged with configurable α weights.

## 🔗 Direct Influence on ThemisDB

### Affected Modules

- [x] LLM module → `src/llm/lora/` (LoRA adapter loading/merging/hot-swap)
- [x] Training module → `src/training/incremental_lora_trainer.cpp`
- [x] LLM deployment plugin → `src/llm/llm_deployment_plugin.cpp` (LoRA adapter versioning)
- [x] Security → `src/llm/lora_security_validator.cpp` (signature verification for LoRA adapters)

### What Was Adopted?

1. **Rank-decomposition injection**: ThemisDB's `LoRAAdapter` stores `{A, B, alpha, rank, target_modules}` per adapter; merged into base model weights at load time via `AdapterRegistry::hotSwap()`.
2. **Target modules**: Default targets are `q_proj`, `v_proj` for llama.cpp models; configurable per model architecture via `LoRAConfig::target_modules`.
3. **Alpha scaling**: `delta_W = (alpha/rank) * B * A` — α is an additional scaling hyperparameter (default α = rank, i.e., scaling = 1.0).
4. **GGUF-compatible adapter format**: ThemisDB stores LoRA adapters in GGUF container format (llama.cpp compatible); adapters are encrypted at rest via `utils/hkdf_helper.cpp`.
5. **Incremental training**: `IncrementalLoRATrainer` implements gradient checkpointing + Adam optimizer for the A,B matrices only; base model weights frozen.

### How Was It Adapted?

| LoRA Concept | ThemisDB Adaptation | Rationale |
|---|---|---|
| Full FP16/BF16 training | QLoRA 4-bit base + BF16 LoRA training | 7B model fits in 12 GB VRAM with QLoRA |
| Static adapter merge | Hot-swap via `AdapterRegistry::hotSwap()` | Production deployment requires zero-downtime adapter updates |
| Single adapter | Multi-adapter fusion (`multi_lora_fusion_guide.md`) | Legal domain + German language adapters stacked |
| Manual hyperparameter tuning | `TrainingPipeline::auto_rank_search()` | Automated rank search over r ∈ {4, 8, 16, 32} |

### Performance Impact

| Metric | Paper Claim | ThemisDB Result | Delta | Reason |
|--------|-------------|-----------------|-------|--------|
| Trainable parameters vs. full fine-tuning | 0.1% (r=4, GPT-3) | 0.3% (r=8, 7B model) | +0.2 pp | Larger target module set |
| GLUE average (vs. full fine-tune) | -0.4 pp | -1.1 pp (legal domain) | -0.7 pp | Legal terminology out-of-distribution |
| Adapter load latency | 0 ms (pre-merged) | <50 ms (hot-swap without merge) | +50 ms | On-the-fly merging for hot-swap path |

## ⚠️ Limitations & Open Questions

- LoRA rank r is fixed at construction; cannot adapt during training.
  - ThemisDB solution: AdaLoRA variant planned (adaptive rank per weight matrix); see `src/training/FUTURE_ENHANCEMENTS.md`.
- Standard LoRA uses equal learning rates for A and B matrices; LoRA+ shows asymmetric rates improve quality.
  - ThemisDB solution: LoRA+ optimizer planned as training enhancement.
- Multi-adapter stacking requires careful α weighting to avoid interference.
  - ThemisDB solution: Orthogonal adapter initialization; see `docs/MULTI_LORA_FUSION_GUIDE.md`.

## 🔬 Validation

- [x] Code reviewed against paper
- [x] Unit tests written (adapter merge correctness, rank verification)
- [x] Benchmark executed (legal NER task with r=8 adapter vs. base model)
- [x] Documentation updated (`docs/THEMIS_HELP_LORA_IMPLEMENTATION_SUMMARY.md`)
- [ ] Module README linked with paper reference
- [ ] implementation_influence index updated

## 📚 Related Work

- [QLoRA — Dettmers et al. (2023)](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#72-qlora)
- [Adapter Tuning — Houlsby et al. (2019)](https://arxiv.org/abs/1902.00751)
- [LoRA+ — Zhao et al. (2024)](https://arxiv.org/abs/2402.12354)
- [`src/llm/lora/` implementation](../../../src/llm/)
- [`src/training/FUTURE_ENHANCEMENTS.md`](../../../src/training/FUTURE_ENHANCEMENTS.md)
- [HuggingFace PEFT Library](https://github.com/huggingface/peft)

---
**Last Updated:** 2026-04-06  
**Next Review:** 2026-09-30
