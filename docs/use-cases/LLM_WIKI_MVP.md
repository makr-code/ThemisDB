# LLM Wiki MVP (ThemisDB)

## Setup

No extra dependencies are required for the default mode.

Optional embedding providers:
- `sentence-transformers` (if installed)
- `openai` (requires `OPENAI_API_KEY`)

## Configuration (Environment Variables)

- `THEMIS_LLM_WIKI_EMBEDDING_PROVIDER` (`hash` default, `mock`, `sentence-transformers`, `openai`)
- `THEMIS_LLM_WIKI_EMBEDDING_MODEL` (used by `sentence-transformers`)
- `OPENAI_API_KEY` and `OPENAI_EMBEDDING_MODEL` (used by `openai`)

## Usage

### 1) Index markdown documentation

```bash
python3 /home/runner/work/ThemisDB/ThemisDB/scripts/llm_wiki_mvp.py index \
  --source-root /home/runner/work/ThemisDB/ThemisDB \
  --output /home/runner/work/ThemisDB/ThemisDB/artifacts/llm-wiki-mvp/index.json
```

### 2) Query indexed docs

```bash
python3 /home/runner/work/ThemisDB/ThemisDB/scripts/llm_wiki_mvp.py query \
  --index /home/runner/work/ThemisDB/ThemisDB/artifacts/llm-wiki-mvp/index.json \
  --question "How do I configure sharding?" \
  --top-k 5 \
  --min-score 0.15
```

### 3) JSON output (for integrations)

```bash
python3 /home/runner/work/ThemisDB/ThemisDB/scripts/llm_wiki_mvp.py query \
  --index /home/runner/work/ThemisDB/ThemisDB/artifacts/llm-wiki-mvp/index.json \
  --question "How do I configure sharding?" \
  --json
```

## MVP Guardrails

- Query/chunk filtering blocks known prompt-injection style patterns and secret-exfiltration cues.
- Output reports `query_flagged_for_prompt_injection` and `filtered_unsafe_chunks`.

## Known MVP Limits

- JSON index is optimized for small/medium corpora and local workflows.
- Retrieval is cosine top-k over stored vectors (no ANN index yet).
- Guardrails are baseline heuristics and should be expanded for production hardening.

## Next Steps

- Optional ThemisDB-native vector index persistence.
- Reranking stage for improved precision.
- Structured evaluation harness (Recall@k, MRR, p95 latency dashboards).
