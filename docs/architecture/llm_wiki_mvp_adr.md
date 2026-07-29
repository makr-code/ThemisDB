# ADR: LLM Wiki MVP for ThemisDB

## Status

Proposed / MVP implementation in this PR.

## Context and Goal

ThemisDB already has documentation ingestion and RAG-oriented building blocks. For a focused "LLM Wiki" MVP we need a small, operator-friendly flow that indexes repository markdown and enables source-cited retrieval for developer/operator questions.

## Target Architecture

`Ingestion -> Heading-aware Chunking -> Embedding Adapter -> JSON Index Store -> Top-k Retrieval -> Source-cited Output`

### Data Flow

1. **Ingestion**: scan markdown files under a configurable repository/document root.
2. **Chunking**: split by markdown heading/section and paragraph windows, keep line-range metadata.
3. **Embedding**: provider adapter interface; default deterministic local hash embedding.
4. **Index/Store**: JSON artifact (`artifacts/llm-wiki-mvp/index.json`) with chunks + metadata + embeddings.
5. **Retrieval**: cosine similarity, top-k and minimum-score threshold.
6. **Answer context output**: query result includes source (`file_path`, `section_title`, `line_start`, `line_end`) and preview text.

## Integration Options and Trade-offs

### Option A: In-repo JSON MVP (selected)
- **Pros**: lowest complexity, no external infra, edition-neutral, easy local/on-prem usage.
- **Cons**: memory/file-size limits on larger corpora, no distributed ANN acceleration yet.

### Option B: ThemisDB-native vector persistence + AQL retrieval
- **Pros**: native operational model, scalability, potential for hybrid graph/vector joins.
- **Cons**: tighter runtime coupling, larger MVP scope, more deployment prerequisites.

### Option C: External vector store (cloud/managed)
- **Pros**: mature ANN and ops tooling.
- **Cons**: external dependency, network latency, compliance/on-prem constraints.

## MVP Recommendation

Ship Option A now as `scripts/llm_wiki_mvp.py` with pluggable embedding providers:
- default: `hash` (safe local deterministic baseline),
- `mock` for tests,
- optional: `sentence-transformers` or `openai` through env/config fallback.

This keeps a clear migration path to Option B/C by preserving a provider abstraction and explicit index schema.

## Security and Guardrails (MVP)

- query and chunk content are screened for prompt-injection style patterns (e.g., "ignore previous instructions", "reveal secret", key/password terms),
- unsafe chunks are excluded from retrieval output,
- result payload indicates whether query was flagged and how many chunks were filtered.

## Quality Metrics (MVP Targets)

- **Recall@k on fixture set**: >= 0.8 for deterministic integration fixtures (`tests/test_llm_wiki_mvp.py`).
- **Local retrieval latency**: target p95 < 200 ms for <= 5k chunks on developer hardware.
- **Source citation coverage**: 100% of retrieval hits include file + section + line range.

## Deployment / Edition Friendliness

- No hard dependency on cloud services.
- Works for local/offline environments by default.
- Optional provider-based expansion supports enterprise/hyperscaler/cloud scenarios without changing query API shape.
