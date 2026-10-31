---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_Sources: "tests/llama_cpp/CMakeLists.txt; tests/llama_cpp/test_model_loading.cpp; tests/llama_cpp/test_inference_execution.cpp; tests/llama_cpp/test_llama_errors.cpp"
Related_Issue: "#7356"
Related_Milestone: "DOC-WEEKLY-2026-44"
---

> Exact model backend not verifiable from final document text alone.

# tests/llama_cpp

This directory contains llama_cpp loading, inference, and stress test sources.
`tests/llama_cpp/CMakeLists.txt` registers each `test_*.cpp` here as a
test/stress target; these tests use in-process fixtures and do not establish
real-model quality or throughput. Focused plugin contract tests are separately
defined in `src/llama_cpp/tests/` and compiled with
`THEMIS_LLAMA_CPP_STUB_MODE`.
