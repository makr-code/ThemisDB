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

# LLM-L-Wiki trust and evidence model

## Current trust contract

No source in `src/llm_l_wiki/` defines accepted evidence origins, provenance
fields, confidence semantics, transformation history, or verification rules.
The empty translation unit provides no trust enforcement. Therefore no result
from this module can currently be described as source-grounded, provenance
preserving, or safe to inject into an LLM prompt.

This is deliberately distinct from guarantees documented for the separate
`src/llm_wiki/` module. Those guarantees require that module's own code and
contracts and do not transfer by name.

## Required trust decisions before use

An implementation must define and test:

1. Which source identities and source classes are accepted, and who approves
   changes to that allowlist.
2. How source identity, content revision/hash, retrieval time, and any
   transformations are attached to each evidence item.
3. Which transformations preserve provenance and which require revalidation.
4. How stale, missing, contradictory, low-confidence, or unverifiable evidence
   is handled; no unverified item should be silently promoted to trusted state.
5. What evidence may cross tenant/workspace or model-provider boundaries.

The API must carry enough metadata for downstream callers to distinguish
original source text from derived summaries or model-generated text.

## Review and incident response

Reviewers should trace a sample output back to its original source and verify
that edits, summaries, and derived claims remain distinguishable. If provenance
is missing or cannot be verified in an actual deployment, stop treating the
affected output as trusted evidence, preserve the relevant request/source
identifiers, and escalate through the verified service owner's incident
process. No module-local incident tooling is present.

## References

- [Research/evaluation record](RESEARCH.md)
- [Security boundary](SECURITY.md)
- [API status](API.md)

> AI-assisted documentation; exact model backend not verifiable from final document text alone.
