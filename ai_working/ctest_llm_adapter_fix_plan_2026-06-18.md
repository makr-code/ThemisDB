# CTest LLM Adapter Fix Plan (2026-06-18)

## Goal
Repair the next full-CTest blocker in the legal ingestion LLM adapter without regressing the existing regex fallback behavior.

## Affected Files
- src/ingestion/llm_adapter.cpp
- include/ingestion/llm_adapter.h
- tests/ingestion/test_ingestion_llm_adapter.cpp (read-only unless production fix proves insufficient)
- tests/legal/test_legal_extraction.cpp (validation only)

## Acceptance Criteria
- `IngestionLlmAdapterFocusedTests` passes.
- Explicitly configured missing model path throws in Phase-2 tests.
- Readable configured model path reports available and yields a non-empty extractor function.
- No configured model still keeps regex fallback behavior.
- `LegalExtractionFocusedTests` stays green.

## Test Scope
- `cmake --build --preset windows-release --target test_ingestion_llm_adapter_focused --parallel 16`
- `ctest --preset windows-release --output-on-failure -R "IngestionLlmAdapterFocusedTests" -j 1`
- `cmake --build --preset windows-release --target test_legal_extraction_focused --parallel 16`
- `ctest --preset windows-release --output-on-failure -R "LegalExtractionFocusedTests" -j 1`
