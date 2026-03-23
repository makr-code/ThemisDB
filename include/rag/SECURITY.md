<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — RAG Module

## Scope

Covers all public headers in `include/rag/`. Implementation hardening in `../../src/rag/`.

## Threat Model

| Threat | Impact | Mitigation |
|---|---|---|
| Prompt injection via retrieved documents | High — LLM hijacking | `PromptInjectionDetector` screens all retrieved chunks before context injection |
| Retrieval-augmented data exfiltration | High — sensitive data leakage | RBAC checks on `HybridRetriever` and `KnowledgeGraphRetriever` before returning chunks |
| LLM judge manipulation via adversarial answers | Medium — false quality scores | `JudgeEnsemble` uses majority vote; individual judge outliers are flagged |
| ONNX model code execution | High — RCE via malicious model | `OnnxModelLoader` validates model signatures and runs in restricted ONNX runtime sandbox |
| Hallucination suppression via feedback poisoning | Medium — false safety signal | `ContinuousLearningOrchestrator` validates feedback sources against authenticated principals |
| Citation forgery | Medium — trust manipulation | `CitationHighlighter` verifies citation spans against original retrieved documents |
| Streaming response injection | Medium — partial result injection | `StreamingRetriever` flushes and validates each SSE frame before delivery |
| Evaluation cache poisoning | Low — stale evaluation scores | `EvaluationCache` keys include content hash + model version; invalidated on model update |

## Security Controls

1. **Injection screening on retrieval** — `PromptInjectionDetector` validates all retrieved content before prompt assembly.
2. **RBAC on retrieval** — `HybridRetriever` and `KnowledgeGraphRetriever` enforce per-user read permissions.
3. **Ensemble majority vote** — `JudgeEnsemble` requires N/2+1 agreement; prevents single-judge manipulation.
4. **ONNX model signing** — `OnnxModelLoader` verifies SHA-256 digest of model file before loading.
5. **Citation span verification** — `CitationHighlighter` anchors citations to exact retrieved document offsets.
6. **Authenticated feedback** — `ContinuousLearningOrchestrator` requires signed feedback tokens.
7. **Content-hash cache keys** — `EvaluationCache` includes document + answer content hash to prevent stale results.

## Known Limitations

- Multi-modal RAG (`MultimodalRag`) — image/audio content is not yet scanned for embedded prompt injections; tracked for Q4 2026.
- Federated RAG (planned Q4 2026) will require isolated data silo trust model review.
