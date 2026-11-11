---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-11-11"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Developer LLM Wiki — Delta Report

Datum: 2026-11-11
Status: Active

## Summary

- Added: 11
- Removed: 0
- Changed: 37

## Added

- include/llm/llama_lora_adapter.h
- include/llm/lora_framework/adalora_strategy_config.h
- include/llm/lora_framework/adapter_provenance.h
- include/llm/lora_framework/lora_dataset_loader.h
- include/llm/lora_framework/lora_safetensors_package.h
- include/llm/lora_framework/lora_training_backend.h
- include/llm/lora_framework/peft_subprocess_backend.h
- include/llm/lora_framework/training_snapshot.h
- include/llm/mode_spec_bootstrap.h
- include/storage/mvcc_snapshot_pins.h
- include/training/adalora_training_pipeline.h

## Removed

- none

## Changed

- .github/workflows/build-mainline.yml
- .github/workflows/build-sanitizer-nightly.yml
- .github/workflows/build-wave-b-transaction.yml
- .github/workflows/compliance-supply-chain.yml
- .github/workflows/gate-pr-core.yml
- .github/workflows/release-build-matrix.yml
- .github/workflows/reusable-cmake-build.yml
- ai_context/developer_llm_wiki/PROCESS.md
- docs/code_maturity_report.md
- docs/de/aql/multi_vector_search.md
- docs/operability/RUNBOOK_PROCESS_MANAGER.md
- docs/operability/RUNBOOK_VECTOR_SEARCH.md
- docs/performance/BENCHMARK_RUNBOOK.md
- docs/performance/PERFORMANCE_EXPECTATIONS.md
- include/llm/ai_orchestrator.h
- include/llm/continuous_batch_scheduler.h
- include/llm/embedded_llm.h
- include/llm/inference_engine_enhanced.h
- include/llm/llama_wrapper.h
- include/llm/lora_framework/lora_orchestrator.h
- include/llm/multi_lora_manager.h
- include/process/process_light_retriever.h
- include/server/lora_api_handler.h
- include/storage/mvcc_chain_pruner.h
- include/storage/mvcc_store.h
- include/training/ada_lora_adapter.h
- scripts/generate_docs_database.py
- src/aql/ROADMAP.md
- src/cdc/ROADMAP.md
- src/graph/ROADMAP.md
- src/llm/ROADMAP.md
- src/process/FUTURE_ENHANCEMENTS.md
- src/process/README.md
- src/process/ROADMAP.md
- src/vector_search/FUTURE_ENHANCEMENTS.md
- src/vector_search/README.md
- src/vector_search/ROADMAP.md
