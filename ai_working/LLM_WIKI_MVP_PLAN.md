# LLM Wiki MVP Plan

- Scope: add a minimal, modular documentation RAG pipeline for markdown sources with a CLI entry point.
- Files:
  - `scripts/llm_wiki_mvp.py` (index + query pipeline, pluggable embedding adapter, guardrails)
  - `tests/test_llm_wiki_mvp.py` (unit + end-to-end integration tests)
  - `docs/architecture/llm_wiki_mvp_adr.md` (design note with options and recommendation)
  - `docs/use-cases/LLM_WIKI_MVP.md` and `README.md` links (setup/config/usage/limits/metrics)
- Acceptance:
  - markdown ingestion, heading-aware chunking, embeddings, retrieval, source-cited output
  - configurable provider via env with safe default
  - tests run locally without extra dependencies
