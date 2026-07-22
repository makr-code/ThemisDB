# GGUF / llama.cpp SSM-Mamba Status (P0-D03)

Status: current  
Validated: 2026-07-22  
Target branch: `develop`

## Scope

Primary evidence sources:
- `/home/runner/work/ThemisDB/ThemisDB/src/llm/llama_wrapper.cpp`
- `/home/runner/work/ThemisDB/ThemisDB/src/llama_cpp/llama_cpp_plugin.cpp`
- `/home/runner/work/ThemisDB/ThemisDB/docs/llm_orchestration/GGUF_SUPPORT.md`
- `/home/runner/work/ThemisDB/ThemisDB/include/aql/llm_query_context.h`

## Objective

Determine whether the current ThemisDB llama.cpp / GGUF integration already provides a production-ready path for Mamba/SSM backends, and document the governance constraints for any future SSM state lifecycle.

## Findings

1. **GGUF base-model ingestion exists, but it is generic and transformer-oriented.**
   - `LlamaCppPlugin::loadModel()` forwards a supplied model path into `LlamaWrapper` and derives `context_length` from generic config keys such as `context_length` / `n_ctx`.
   - `LlamaWrapper::loadModelFromThemisDB()` persists model blobs to a temporary file and defaults unknown formats to `.gguf`.
   - Existing documentation in `docs/llm_orchestration/GGUF_SUPPORT.md` describes GGUF support in terms of standard GGUF model loading and quantized tensor conversion.

2. **No repository evidence was found for a dedicated Mamba/SSM plugin contract.**
   - There is no `ISSMPlugin` interface in the current public LLM headers.
   - No Mamba-/SSM-specific model loading branch was identified in the current llama.cpp plugin or wrapper paths.
   - No hidden-state checkpoint/resume contract analogous to the proposed `SSMStateStore` exists yet.

3. **Current snapshot consistency primitives are retrieval-scoped, not SSM-state-scoped.**
   - `LLMQueryContext` already carries `snapshot_ts` for MVCC-consistent retrieval.
   - This is the correct anchor point for a future `SSMStateSnapshot`, but the binding is not implemented today.

4. **Governance baseline remains compatible with a future internal state store.**
   - ThemisDB's current model-loading path treats model material as internal data handled within ThemisDB-controlled storage / temp-file boundaries.
   - Nothing in the current implementation suggests or requires a parallel external database for SSM state.
   - Therefore the intended governance rule is unchanged: ThemisDB remains the System-of-Record; RocksDB may only be used as an internal persistence primitive.

## Decision

P0-D03 outcome: **negative for direct production Mamba enablement, positive for staged interface work**.

- **Not available today:** production-ready GGUF/llama.cpp Mamba integration, SSM hidden-state lifecycle, resume/restore semantics, tenant-safe persistent state snapshots.
- **Available today:** enough existing GGUF / llama.cpp infrastructure to prepare interface contracts and governance boundaries in Phase 1 without disturbing the current transformer path.

## Required follow-up before P1/P2

- Add explicit `ISSMPlugin` and `SSMStateStore` contracts before any backend-specific implementation.
- Bind any future state snapshot to `LLMQueryContext::snapshot_ts` for MVCC-consistent resume semantics.
- Keep all persistent SSM state internal to ThemisDB-managed storage; no external sidecar database path.
- Require tenant-isolated snapshot keys, auditable restore/invalidate events, and explicit fallback to the existing transformer/RAG path on state failure.
