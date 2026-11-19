---
name: hybrid-search-diagnostics
description: Diagnose empty, partial, or poorly ranked ThemisDB hybrid search results by separating lexical retrieval, vector retrieval, fusion, and reranking.
metadata:
  AI_Used: true
  AI_Models: "unknown"
  AI_Contributor: "GitHub Copilot"
  Human_Review: false
  Human_Author: "unknown"
---

> Exact model backend not verifiable from final document text alone.

# Diagnose hybrid search results

Use this skill for a specific retrieval incident. Use `rag-evaluation` for
dataset-level regression analysis.

1. Establish the target version, exact search path, authorized collection and
   filters, requested result limit, and symptom. Consult `src/search/API.md`
   for that path. Do not equate local `HybridSearch` with distributed search
   or assume that a C++ interface is exposed over HTTP.
2. Compare available lexical and vector candidate counts and stage status.
   Distinguish a healthy empty stage from a timeout, backend failure, skipped
   stage, or missing diagnostics. Do not weaken filters to get more hits.
3. If expected candidates appear before fusion but not after it, inspect rank
   positions, fusion limits, deduplication, and optional reranking. Do not
   compare raw lexical and vector scores as if they shared a scale.
4. Report the first stage where evidence shows a discrepancy. Treat default
   status values as inconclusive unless the implementation populates them
   for that path. For example, the local hybrid path can report partial
   results without populating `primary_error_code`.
5. Propose a bounded, read-only comparison with identical authorized filters.
   Preserve partial, empty, and failed outcomes as separate results. Do not
   rebuild indexes, change search configuration, or broaden source access.
