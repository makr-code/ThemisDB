# Training Module — Examples

Examples for the `training` module demonstrating Paper 1 (Self-Optimising LoRA Loops) implementation patterns.

## Contents

| File | Paper | Issue | Status |
|------|-------|-------|--------|
| `database_optimizer_labeler.cpp` | Paper 1 §5 | IMPL-A1, IMPL-A3 | Specification / planned API |

## database_optimizer_labeler.cpp

Demonstrates:

1. **DATABASE_OPTIMIZER domain labeling** (IMPL-A1) — labeling `(query, explain_plan, Δlatency_ms)` triples using the confidence function `tanh(|Δlatency| / 50)`
2. **Quality filtering** via `LoRADataSelectionPipeline` (dedup, min confidence 0.85)
3. **Loop 4 training cycle** — `IncrementalLoRATrainer` triggered as Loop 4
4. **Gradient export** (IMPL-A3) — `exportGradient()` → `EncryptedGradient` (AES-256-GCM)
5. **FedAvg delta application** (IMPL-A3) — `applyGlobalDelta()` from `LoRAFederationCoordinator`

Calls to planned IMPL-A1/A3 APIs are marked with `/* PLANNED */` comments and serve as acceptance-criteria specification for those issues.

## Related Documentation

- Issue spec: `docs/issues/lora_loops/IMPL-A1-dataset-construction.md`
- Issue spec: `docs/issues/lora_loops/IMPL-A3-federation-hooks.md`
- Research paper: `docs/en/research/THEMISDB_LORA_RESEARCH_PAPER.md`
- Module ROADMAP: `include/training/ROADMAP.md` §Phase 7
