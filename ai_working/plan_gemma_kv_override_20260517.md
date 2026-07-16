# Plan: Gemma Native Load Compatibility (KV Override)

- Scope: `src/llm/model_loader.cpp`, `PERFORMANCE_EXPECTATIONS.md`
- Problem: Native llama loader fails on Gemma with missing key `gemma3.attention.layer_norm_rms_epsilon`.
- Approach:
  - Add targeted KV override in model load params for Gemma-family models.
  - Keep override narrowly scoped and logged.
  - Rebuild benchmark and re-run `BM_LlamaCpp_RealModel_GPUEvidence` for Gemma3.
  - Document outcome in performance expectations.
- Acceptance:
  - Build succeeds.
  - Error no longer fails on missing key (or clear next blocker appears).
