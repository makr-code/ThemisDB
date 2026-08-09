[docs](../../index.md) > [en](../index.md) > [llama_cpp](./index.md) > [reference](./reality-check.md)
**Date:** 2026-04-16
**Status:** review
**Primary Source:**
- `src/llama_cpp/llama_cpp_plugin.cpp`
- `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`
- `src/llama_cpp/README.md`
- `src/llama_cpp/ARCHITECTURE.md`
- `src/llama_cpp/AUDIT.md`

**Reference:**
- Issue: `[MODULE] llama_cpp`
- Context: Task 1 — Reality check of primary docs against implementation.

---

# Reality Check — llama_cpp

## Result

`llama_cpp` is functionally implemented (including streaming, batch, registrar, and optional `LlamaWrapper` delegation), but several primary docs are outdated.

## Documented Deviations with Evidence

| Area | Doc claim | Implementation observation | Evidence |
|---|---|---|---|
| Version/maturity | `src/llama_cpp/README.md` still describes v2.0.0/Beta with 30 tests | Implementation and tests include v2.1.0 features and 50 tests (A–N) | `src/llama_cpp/README.md`, `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`, `tests/CMakeLists.txt:22155` |
| Capabilities | `src/llama_cpp/ARCHITECTURE.md` lists `supports_streaming=false` | Code sets `supports_streaming=true` and `supports_batching=true` | `src/llama_cpp/ARCHITECTURE.md`, `src/llama_cpp/llama_cpp_plugin.cpp:311-317` |
| RAG flow | `ARCHITECTURE.md` documents naive context concatenation | `generateRAG()` uses `RAGContextAssembler` with token budgeting | `src/llama_cpp/ARCHITECTURE.md`, `src/llama_cpp/llama_cpp_plugin.cpp:236-285` |
| Audit status | `src/llama_cpp/AUDIT.md` still reports 30 tests and missing streaming | Streaming (`generateStream`) is implemented and tested; suite is 50 tests | `src/llama_cpp/AUDIT.md`, `src/llama_cpp/llama_cpp_plugin.cpp:365-383`, `src/llama_cpp/tests/test_llama_cpp_plugin.cpp` |
| Compile-flag naming | `FUTURE_ENHANCEMENTS.md` references `THEMIS_ENABLE_LLAMA_CPP` | Build + code use `THEMIS_LLM_ENABLED` | `src/llama_cpp/FUTURE_ENHANCEMENTS.md:43`, `src/llama_cpp/CMakeLists.txt:46-50`, `src/llama_cpp/llama_cpp_plugin.cpp` |

## Update — v2.4.0 (2026-08-09)

All deviations from the 2026-04-16 reality check have been resolved. The primary sources now reflect implementation reality:

| Area | Resolution |
|---|---|
| Version/maturity | ROADMAP, AUDIT, CHANGELOG updated to v2.4.0; 89 tests (groups A–X) |
| Capabilities | ARCHITECTURE.md flow diagrams updated; all `supports_*` flags correct |
| RAG flow | ARCHITECTURE.md now shows mutex-snapshot pattern and RAGContextAssembler |
| Audit status | AUDIT.md updated: 0 open security issues, 0 open functional issues |
| Security | All 4 SECURITY.md checkboxes set to `[x]`; GGUF magic, digest gate, policy hook |
| Concurrency | `data_race` CRITICAL findings resolved; atomic counters; retry logic |
| Server integration | `initFromServerConfig()` added as clean startup integration point |
