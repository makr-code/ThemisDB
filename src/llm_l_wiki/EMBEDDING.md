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

# LLM-L-Wiki embeddings

## Current capability

No embedding model, vector representation, embedding call, or index wiring is
present in the `src/llm_l_wiki/` source inventory. This module has no
source-verified embedding behavior or embedding-specific benchmark. Any such
capability in `src/llm_wiki/` belongs to that separate module and is not
inherited by this path.

## Contract required before implementation

An embedding integration must document and version all of the following before
persisting vectors:

- provider and immutable model/version identifier;
- input normalization, token and length limits, and behavior on empty input;
- output dimension, element type, normalization, and distance semantics;
- vector/index owner, persistence location, and workspace or tenant keying;
- handling for provider errors, timeouts, rate limits, and partial batches;
- re-embedding and index migration behavior when the model or dimensions change.

Do not mix vectors from different model versions or dimensions in one search
space unless an explicit compatibility contract and migration test prove that
the operation is valid.

## Validation plan

Use fixed input fixtures to verify output dimension, finite values, repeatability
where promised, and rejection of malformed inputs. For retrieval, record a
versioned corpus/query set and relevance labels, then report Recall@k or MRR,
latency percentiles, batch throughput, and peak memory. Test provider
unavailability, throttling, empty batches, dimension mismatch, and migration
rollback. These are required future checks, not evidence that this module
currently passes them.

## References

- [Model assumptions and evaluation](MODELING.md)
- [Research protocol](RESEARCH.md)
- [Implementation inventory](IMPLEMENTATION.md)

> AI-assisted documentation; exact model backend not verifiable from final document text alone.
