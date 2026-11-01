---
AI_Used: true
AI_Models: unknown
AI_Contributor: "ThemisDB documentation orchestrator"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# RAG Quality Control — Implementation Reference

## Scope and integration boundary

This guide covers the source-level behavior of `QualityControlPipeline`. It is
an explicit component, not a mandatory response gate: the calling service
decides whether to invoke it and how to handle its result. It does not generate
or retry answers itself.

The public contract is in `include/rag/quality_control_pipeline.h`; the
implementation is `src/rag/quality_control_pipeline.cpp`. The factory API is
declared in `include/rag/quality_control_factory.h` as
`QualityControlFactory` (not `QualityPipelineFactory`).

## Implemented pipeline behavior

`runQualityControl(query, answer, documents)` can run the enabled fast,
balanced, and thorough stages in sequence. It returns a `QualityCheckResult`
with status, dimension scores, stage timings, failure reasons, confidence, and
citation coverage when that check runs.

- A failed fast stage returns `FAILED`.
- A failed balanced stage returns `RETRY_NEEDED`; the caller owns any
  regeneration/retry decision.
- A failed thorough stage returns `ESCALATE`.
- Otherwise the final result is marked `PASSED`.
- Learning feedback is sent only to a callback explicitly supplied by the
  caller. The implementation does not send HTTP/gRPC to
  `learning_orchestrator_url`.

The header also declares retry and per-stage timeout configuration fields.
The current implementation does not use `enable_auto_retry`, `max_retries`,
or the timeout fields to perform retries or enforce deadlines. Treat threshold
and timeout values as configuration data, not proof of those behaviors.

## Components and current limits

| Component | Source-level role | Limit |
|---|---|---|
| `LLMJudgeClient` | Judge component held by the pipeline | Backend availability and actual behavior depend on its configured/injected implementation. |
| `GEvalEvaluator` | Evaluation component used by the pipeline | Do not interpret its presence as a measured quality result. |
| `NLIFaithfulnessVerifier` | Thorough-stage faithfulness component | Model-backed inference depends on the configured model/runtime; verify the selected path. |
| `CitationHighlighter` | Can populate citation coverage in the thorough stage | Coverage is an evaluation signal, not authorization or proof of factuality. |
| Learning callback | Receives query and result when configured | Callback invocation is local; no external orchestrator transport is implemented here. |

`QualityControlPipeline` logs up to the first 50 query characters in its
current implementation. Apply service-level redaction, access controls, and
retention policies.

## Build and focused test references

RAG test targets and CTest names are generated from `test_*.cpp` in
`tests/rag/CMakeLists.txt`. There is no `test_quality_control_pipeline` target
or `quality_control_demo` target registered there. The closest direct
regression source is `tests/rag/test_rag_judge.cpp`:

```bash
cmake --build <build-dir> --target module_rag_test_rag_judge_focused
ctest --test-dir <build-dir> -R '^test_rag_judge_RagFocusedTests$' --output-on-failure
```

Tests establish covered component behavior; they do not prove that a
deployment invokes the pipeline or enforces its results.

## References

- [API contracts](API.md)
- [Implementation guide](IMPLEMENTATION.md)
- [Trust boundaries](TRUST.md)
- `include/rag/quality_control_pipeline.h`
- `include/rag/quality_control_factory.h`
- `src/rag/quality_control_pipeline.cpp`
- `tests/rag/CMakeLists.txt`
