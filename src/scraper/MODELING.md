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

# Scraper Evaluation Model

**Purpose and audience:** Describes the scoring contract implemented by scraper maintainers and integrators. This is a document-quality/relevance filter, not a trained classifier or a legal-validity assessment.

## Inputs and outputs

`IScraperLLMEvaluator::evaluate(text, url, gap, threshold)` receives extracted page text and a `GapContext` containing an ID, description, and keyword list. The result carries `quality_score`, `gap_relevance`, summary, key entities, discard reason, and `below_threshold`. Scores are represented on a 0–1 scale; the interface does not independently validate that model output falls in that range.

## LLM path

When compiled with `THEMIS_ENABLE_LLM`, a default LLM plugin is available, and text is non-empty, the evaluator sends a German-language prompt containing the gap fields and a text prefix capped at 3,000 characters. It requests JSON with `quality_score`, `gap_relevance`, `summary`, `key_entities`, and `discard_reason`, at temperature 0.1 and a 256-token limit.

The parser extracts the outermost JSON object. Missing numeric score fields retain their zero defaults. Missing/invalid JSON sets a discard reason and marks the result below threshold; exceptions during inference use the heuristic path. The model path is therefore sensitive to model availability, model behavior, truncation, and schema omissions.

## Heuristic fallback

The fallback lowercases text and each configured keyword, then counts every non-overlapping substring occurrence. Its current scoring is:

```text
density       = hits / (keyword_count * (text_length / 1000 + 1))
gap_relevance = min(1, density * 3)
length_score  = min(1, text_length / 2000)
quality_score = 0.4 * length_score + 0.6 * gap_relevance
```

With no keywords, relevance is zero and quality is only the length contribution. Empty text is discarded. These are deterministic formula outputs for fixed input bytes and threshold, but they are coarse proxies: substring matches are not semantic matches, and text length is not a measure of correctness.

## Decision semantics and evaluation

The configured `quality_threshold` sets `below_threshold`; a non-empty `discard_reason` also causes discard. The pipeline does not require `gap_relevance` to exceed a separate threshold. Integrators should inspect both scores and the chosen path (LLM or heuristic) before using accepted documents in a high-consequence workflow.

Regression tests should pin identical text, keywords, and threshold, then assert score components, boundary behavior at the threshold, malformed model output, and inference failure fallback. Track acceptance/discard counts from `ScraperRunStats`; those counters describe pipeline decisions, not model accuracy. See [RESEARCH.md](./RESEARCH.md), [API.md](./API.md), and [`scraper_llm_evaluator.cpp`](./scraper_llm_evaluator.cpp).

**AI-assisted documentation:** Exact model backend not verifiable from final document text alone.
