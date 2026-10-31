---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# LLM-L-Wiki modeling

## Purpose and current model status

This document is the model/evaluation boundary for `src/llm_l_wiki/`. No model,
weight artifact, tokenizer, inference interface, training procedure, or
model-backed call path is present in the module's source inventory. Therefore
there is no model configuration or model-quality result to report.

The `llm` module, `llama_cpp`, and the separately implemented `llm_wiki` module
must not be treated as evidence that this path currently loads or calls a
language model.

## Assumptions that remain open

Before model selection or integration, the owning design must specify:

- whether model inference is in scope at all, and whether it is local or remote;
- supported task(s), input/output schema, and maximum input/output sizes;
- model identity and immutable artifact/version provenance;
- privacy, retention, access, and licensing constraints for prompts and
  outputs;
- the non-model baseline and acceptance criteria for quality, latency, and
  resource use.

Until recorded, none of these are defaults for this module.

## Evaluation and release evidence

For any proposed model, compare against a deterministic baseline on a
versioned, representative evaluation set. Report task-specific quality,
grounded-answer/source attribution errors, refusal or safety failures, p50/p95
latency, throughput, and peak memory. Include sample counts, confidence
intervals where meaningful, hardware, runtime options, and exact artifact
hashes. Test empty and malformed inputs, long contexts, conflicting evidence,
and inference timeouts.

No metric value or target is established here. An implementation should fail
closed when required model configuration or artifact validation is missing;
the exact error contract must be defined in its API before release.

## Trade-offs and limits

Model quality cannot be separated from corpus, prompt, retrieval, and decoding
configuration. Avoid attributing a retrieval or grounding failure to model
quality without isolating those factors. No training or fine-tuning is evidenced
for this module, so do not claim learned behavior.

## References

- [Research and reproducibility protocol](RESEARCH.md)
- [API status](API.md)
- [Implementation evidence](IMPLEMENTATION.md)

> AI-assisted documentation. Exact model backend not verifiable from final document text alone.
