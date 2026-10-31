---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# LLM Wiki Embedding Path

## Scope

This page documents the vector representation relevant to wiki ingestion and
retrieval. The plugin-level options are declared in
[`../../include/llm_wiki/llm_wiki_plugin_interface.h`](../../include/llm_wiki/llm_wiki_plugin_interface.h);
the current hash helper is in
`wikipedia/llm_wiki_plugin_impl.cpp`. An option name in the interface is not
proof that a provider is available in every build.

## Current hash baseline

The source helper tokenizes with `[A-Za-z0-9_-]+`, lowercases each token, and
hashes each token using FNV-1a into a dense vector. Each token increments one
bucket; the completed vector is L2-normalized when its norm is nonzero.
Non-positive dimensions are replaced with 128. Empty or tokenless text
therefore produces a zero vector.

This is a deterministic bag-of-tokens representation, not a semantic
embedding model. It discards token order and context, maps distinct tokens to
the same bucket when hashes collide, and recognizes only the tokenizer's
ASCII character class. It is suitable as a dependency-light baseline or for
testing, but quality must be measured before using it as evidence of semantic
retrieval.

## Configurable providers

`WikiIngestOptions` declares `hash` as its default provider and names
`mock`, `sentence-transformers`, and `openai` as alternatives. The interface
documents extra runtime requirements for some providers. Before selecting a
non-default provider, verify its implementation, edition availability,
configuration validation, secret handling, and error behavior in the target
build. Do not assume a credentials-backed provider is active because a string
option is declared.

Embedding dimension and model name are part of reproducibility: record them
with the corpus revision and index settings. If a change alters vector
dimension or tokenization, re-index the affected corpus; mixing vectors from
incompatible embedding configurations can invalidate similarity comparisons.

## Evaluation

Compare providers using the same fixed corpus, query set, relevance judgments,
chunking, and ranking configuration. Record retrieval quality (Recall@k and
MRR), empty-vector frequency, ingestion cost, query latency, and memory use.
`WikiWorkspaceStats` declares evaluation and latency fields, but a declared
field is not a measured result. See [`RESEARCH.md`](RESEARCH.md) and
[`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md) for evaluation
context and the currently documented targets.

## Failure and safety considerations

- An empty/tokenless input cannot yield a useful nonzero hash vector.
- Hash collisions and out-of-vocabulary token forms can reduce recall without
  producing a runtime error.
- Provider initialization or model-loading failures must be surfaced rather
  than silently described as equivalent semantic behavior.
- Keep embedding configuration separate from prompt-injection screening.
  Vectorization is not an input-safety control.
- Do not place API credentials in workspace documents or persisted state.

## Related documents

- [`MODELING.md`](MODELING.md) — schema and provenance model
- [`API.md`](API.md) — `WikiIngestOptions` and provider-related fields
- [`TRUST.md`](TRUST.md) — input and credential trust boundaries

> Exact model backend not verifiable from final document text alone.
