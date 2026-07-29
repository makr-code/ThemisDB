# LLM Wiki MVP (ThemisDB)

## Setup

No extra dependencies are required for the default mode.

Set `$REPO_ROOT` to the repository root before running the examples below:

```bash
export REPO_ROOT=$(git rev-parse --show-toplevel)
```

Optional embedding providers:
- `sentence-transformers` (if installed)
- `openai` (requires `OPENAI_API_KEY`)

## Configuration (Environment Variables)

- `THEMIS_LLM_WIKI_EMBEDDING_PROVIDER` (`hash` default, `mock`, `sentence-transformers`, `openai`)
- `THEMIS_LLM_WIKI_EMBEDDING_MODEL` (used by `sentence-transformers`)
- `OPENAI_API_KEY` and `OPENAI_EMBEDDING_MODEL` (used by `openai`)

## Modes

### 1) Classic JSON index/query mode (RAG-like baseline)

```bash
python3 $REPO_ROOT/scripts/llm_wiki_mvp.py index \
  --source-root $REPO_ROOT \
  --output $REPO_ROOT/artifacts/llm-wiki-mvp/index.json
```

```bash
python3 $REPO_ROOT/scripts/llm_wiki_mvp.py query \
  --index $REPO_ROOT/artifacts/llm-wiki-mvp/index.json \
  --question "How do I configure sharding?" \
  --top-k 5 \
  --min-score 0.15 \
  --json
```

### 2) Persistent LLM Wiki workspace mode

This mode keeps a compounding knowledge base under one workspace root:

- `raw_sources/` immutable source copies
- `wiki/pages/` LLM-maintained pages
- `wiki/index.md` content catalog
- `wiki/log.md` append-only ingest/query/lint timeline
- `wiki/schema.md` maintenance rules
- `wiki/state.json` structured state for links/assertions/tasks

#### Initialize workspace

```bash
python3 $REPO_ROOT/scripts/llm_wiki_mvp.py wiki-init \
  --workspace-root $REPO_ROOT/artifacts/llm-wiki-workspace
```

#### Ingest one source (creates summary + concept links + log entry)

```bash
python3 $REPO_ROOT/scripts/llm_wiki_mvp.py wiki-ingest \
  --workspace-root $REPO_ROOT/artifacts/llm-wiki-workspace \
  --source $REPO_ROOT/docs/architecture/llm_wiki_mvp_adr.md \
  --title "LLM Wiki ADR"
```

#### Query workspace and save answer as a page

```bash
python3 $REPO_ROOT/scripts/llm_wiki_mvp.py wiki-query \
  --workspace-root $REPO_ROOT/artifacts/llm-wiki-workspace \
  --question "What are the key governance constraints?" \
  --top-k 5 \
  --min-score 0.1 \
  --save-as-page \
  --page-title "Governance Constraints" \
  --json
```

#### Run wiki lint checks

```bash
python3 $REPO_ROOT/scripts/llm_wiki_mvp.py wiki-lint \
  --workspace-root $REPO_ROOT/artifacts/llm-wiki-workspace \
  --json
```

Lint reports:
- orphan pages
- missing backlinks
- stale synthesis pages
- unresolved contradiction-review tasks

## Guardrails

- Query/chunk filtering blocks known prompt-injection style patterns and secret-exfiltration cues.
- Output reports `query_flagged_for_prompt_injection` and `filtered_unsafe_chunks`.
- Contradiction cues create explicit review tasks in persistent workspace mode.

## Current Limits

- Persistent mode is still an MVP orchestration layer, not full RocksDB-native persistence.
- Retrieval is cosine top-k over stored vectors (no ANN index yet).
- Summarization and concept extraction are heuristic and should be replaced by stronger model workflows in production.

## Next Steps

- Move workspace state (`pages`, `links`, `assertions`, `tasks`, `log`) to ThemisDB-native storage.
- Add graph-native traversal and contradiction resolution workflows.
- Add structured quality evaluation (Recall@k, MRR, p95 latency, citation coverage).
