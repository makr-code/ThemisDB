# LLM Module Security Audit

## Audit Summary

- **Overall status:** 8 findings are currently open (`GAP-LLM-001..008`).
- **Critical (S0):** none open.
- **Scope:** `src/llm` runtime kernels and safety pipeline reliability.
- **Issue traceability:** implementation scope is tracked in [issue #5039](https://github.com/makr-code/ThemisDB/issues/5039).

## Findings Table

| Finding ID | File | Severity | Priority | Status | Owner | Evidence |
| --- | --- | --- | --- | --- | --- | --- |
| GAP-LLM-001 / S1-LLM-001 | `src/llm/attention/cuda/flash_attention_cuda.cu` | High | P1 | Open | LLM team | `FlashAttentionCUDA::computeAttention` is still a stub (`return false`). |
| GAP-LLM-002 / S1-LLM-002 | `src/llm/attention/cuda/flash_attention_cuda.cu` | High | P1 | Open | LLM team | `FlashAttentionCUDA::computeAttentionBackward` is still a stub (`return false`). |
| GAP-LLM-003 / S1-LLM-003 | `src/llm/attention/cuda/flash_attention_cuda.cu` | High | P1 | Open | LLM team | `FlashAttentionCUDA::computeSparseAttention` is still a stub (`return false`). |
| GAP-LLM-004 / S2-LLM-001 | `src/llm/safety/classifier.cpp` | Medium | P2 | Open | Safety team | Classifier uses deterministic placeholder logic instead of model-backed inference. |
| GAP-LLM-006 / S2-LLM-002 | `src/llm/safety/guardian.cpp` | Medium | P2 | Open | Safety team | Prompt sanitization relies on exact literal matching and is bypassable by obfuscation. |
| GAP-LLM-007 / S2-LLM-003 | `src/llm/safety/guardian.cpp` | Medium | P3 | Open | Safety team | Topic blocklist is static matching without contextual checks. |
| GAP-LLM-005 / S3-LLM-001 | `src/llm/safety/classifier.cpp` | Low | P3 | Open | Safety team | `classifyBatch` runs sequential single-item classification. |
| GAP-LLM-008 / S3-LLM-002 | `src/llm/safety/monitoring.cpp` | Low | P3 | Open | Platform team | Monitoring counters are in-memory only; no exporter/durable sink wiring. |

## Recommended Remediation

1. Implement CUDA flash-attention forward/backward/sparse execution paths or add explicit feature-gating with deterministic fallback behavior.
2. Replace deterministic safety placeholders with model-backed classifier and calibrated thresholds.
3. Harden guardian matching with normalization and semantic/context-aware checks.
4. Add durable/exported monitoring sink for safety/audit telemetry.

## Validation Notes

- Findings are synchronized with `/tmp/workspace/makr-code/ThemisDB/src/llm/MODULE_GAPS.md`.
- Severity mapping is synchronized with `/tmp/workspace/makr-code/ThemisDB/src/llm/SECURITY.md`.
- Re-audit required after each remediation block tied to issue #5039.
