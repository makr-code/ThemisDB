# RAG Judge Local Model Registration Plan

## Goal

Make RAGJudge/LLMJudgeClient auto-register a locally available GGUF/BIN model for focused integration tests so the judge does not fail with `No available models` when a repository-local `models/` directory exists.

## Affected Files

- `src/rag/llm_judge_client.cpp`

## Local Hypothesis

- `RAGJudge` constructs `LLMJudgeClient`, which creates a fresh `InferenceEngineEnhanced`.
- No plugin/model is registered into that engine.
- Focused test therefore fails before inference with `No available models` despite real model files being present.

## Planned Change

1. Add a private helper in `llm_judge_client.cpp` to resolve a local model path.
2. Prefer explicit env/config-like hints first, then search common `models/` directories relative to the current working directory.
3. Instantiate `LlamaCppPlugin`, call `loadModel()`, and register it under the configured judge model id.
4. Keep the behavior best-effort: no exception on missing files, only logging.

## Validation

1. Build `themis_tests`.
2. Re-run `RAGJudgeIntegrationTest.BasicEvaluation`.
3. If needed, inspect whether failure changes from `No available models` to a real model-load/inference result.