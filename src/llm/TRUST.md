---
Author: "ThemisDB Team"
Created: "2026-10-21"
Last Updated: "2026-10-21"
Status: "active"
AI_Used: true
AI_Models: "GitHub Copilot"
AI_Contributor: "GitHub Copilot"
Human_Review: true
Human_Author: "ThemisDB Team"
---

# LLM Module Trust and Risk Model

AI provenance note: Exact model backend not verifiable from final document text alone.

This document defines the trust boundaries and operational risks for the LLM module as implemented under `src/llm`.

## Trust boundaries

The key trust boundaries in the module are:

- model file to runtime integrity boundary
- plugin selection to inference execution boundary
- prompt input to policy enforcement boundary
- route selection to target model boundary
- degraded embedding fallback to caller semantics boundary

These boundaries matter because the runtime is allowed to operate only after each step is validated or explicitly rejected.

## Risk model

The main risks in the LLM module are:

1. unsafe or unvalidated model loads
2. incorrect route selection to the wrong backend or model
3. prompt policy bypass or overbroad redaction
4. permission or availability issues when a default plugin cannot be resolved
5. degraded embedding fallback being mistaken for semantic quality
6. operational failures hiding behind permissive runtime behavior

The implementation addresses these risks by keeping validation and selection explicit instead of implicit.

## Controls and invariants

The current controls include:

- checksum validation in `LazyModelLoader::verifyModelChecksum()`
- explicit plugin registration and default selection in `LLMPluginManager`
- deterministic route ordering in `ModelRouter`
- explicit block or redact outcomes in `PromptPolicy::apply()`
- deterministic fallback embedding output for degraded runtime conditions

The module also keeps several invariants explicit in code:

- empty rule identifiers and empty targets are invalid
- invalid regex definitions are rejected early
- model or plugin selection errors are surfaced, not hidden
- degraded output remains clearly separated from a true semantic model result

## Trust assumptions that should not be made

The LLM module does not assume the following:

- a model path is valid just because it exists on disk
- any plugin is safe to use without explicit selection or validation
- a prompt is harmless because it passed a single regex check
- a fallback embedding is semantically equivalent to a real model embedding
- a route without a matching target is safe to default quietly

These assumptions would weaken the runtime and create hidden trust failures.

## Incident handling

When the runtime is compromised or behaves unexpectedly:

1. determine whether the failure is model integrity, policy, routing, or plugin-state related
2. isolate the boundary where the decision became invalid
3. verify the active plugin and route definition before changing logic
4. preserve logs and rule state for incident review
5. validate the corrected behavior with a focused generation or routing test

## Current posture

The LLM subsystem is a practical production runtime with strong explicit validation boundaries. It is not a magical runtime that assumes the rest of the system is trustworthy. The trust posture is deliberate: deterministic behavior, explicit failure, and clear degraded-mode semantics are the operational assumptions that make the module usable under real load.
