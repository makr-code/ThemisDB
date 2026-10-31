---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Scraper Research and Evaluation

**Purpose and audience:** This page records the engineering assumptions and validation method for maintainers evaluating changes to the scraper pipeline. It describes current code, not a claim that external-source coverage or extraction accuracy has been independently measured.

## Problem and method

The module turns configured seed URLs and selected catalog sources into candidate records. The implementation separates retrieval mode (`STATIC`, `JS_RENDERED`, `API_JSON`, or `API_GRAPHQL`), HTML search discovery, text extraction, quality/relevance scoring, and record writing. The default HTML path extracts text and skips content shorter than 50 characters. The API path flattens JSON response content into text. See [the implementation map](./IMPLEMENTATION.md) and [the public API](./API.md).

Evaluation is rule/model based; it does not train a scraper model:

- With `THEMIS_ENABLE_LLM` and an available default plugin, `ScraperLLMEvaluator` sends at most the first 3,000 text characters together with the gap ID, description, and keywords. The request asks for JSON quality and relevance scores, a summary, entities, and a discard reason.
- When LLM inference is unavailable or throws, evaluation falls back to keyword-density and text-length scoring. This is a different method, not an equivalent substitute for model evaluation.
- The default discard decision is based on `quality_score < quality_threshold` or a non-empty discard reason. `gap_relevance` is reported separately and is not itself the threshold.

## Assumptions and limitations

- The gap description and keywords supplied by the caller represent the intended retrieval/evaluation target. Empty or poorly selected keywords reduce the meaning of heuristic relevance.
- HTML search extraction uses structural heuristics and may not recognize site-specific layouts. Without pugixml, form discovery and result parsing return empty results.
- LLM output is not authoritative. It can be malformed or affected by untrusted page text; a response with no valid JSON is marked for discard, while inference exceptions activate the heuristic path.
- No evaluation corpus, measured precision/recall, or validated cross-site accuracy results are maintained in this module. Avoid describing scores as legal or factual verification.
- The repository benchmark mapping is documented in [PERFORMANCE_EXPECTATIONS.md](./PERFORMANCE_EXPECTATIONS.md); it includes adjacent extraction suites and does not establish scraper-specific relevance accuracy.

## Evaluation protocol for changes

Use controlled fixtures and the focused scraper tests rather than live websites as the primary regression signal:

1. Hold the gap context and fixture HTML/JSON constant; use injected HTTP, search, API, renderer, evaluator, and writer implementations where available.
2. Cover each retrieval mode used by the changed code, empty/malformed responses, pagination termination, URL-policy rejection, content below the 50-character extraction floor, both evaluator outcomes, and partial writer failure.
3. Assert output records, provenance, counters, and explicit termination behavior; do not infer quality from a successful fetch.
4. Run the affected tests in `tests/scraper/`. Use representative live-source checks only as a separate, policy-approved integration exercise.

Relevant evidence: [`scraper_llm_evaluator.cpp`](./scraper_llm_evaluator.cpp), [`scraper_plugin.cpp`](./scraper_plugin.cpp), [`tests/scraper/`](../../tests/scraper/), and [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md).

**AI-assisted documentation:** Exact model backend not verifiable from final document text alone.
