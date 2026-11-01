---
Author: "ThemisDB Team"
Created: "2026-10-18"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Prompt Engineering Module Capabilities

## Scope

`prompt_engineering` is a C++ library module for prompt-related lifecycle and quality operations. It does not host or call an LLM; a consuming service owns model transport and execution.

## Implemented surfaces

- Create, retrieve, validate, and render prompt templates through `PromptManager`.
- Track prompt revisions with `PromptVersionControl`.
- Score outputs and run bounded optimization workflows with caller-provided evaluation data and callbacks.
- Collect feedback and export module-local metrics.
- Expose separate helpers for injection heuristics, structured output, RAG prompt construction, and rewrite rules.

## Limits and references

Context substitution does not authorize or escape supplied values. Injection detection is heuristic and is not an authorization decision. `RewriteEngine` is implemented as a separate API but is not automatically called by the integration hooks. See [`API.md`](./API.md), [`ARCHITECTURE.md`](./ARCHITECTURE.md), [`TRUST.md`](./TRUST.md), and [`ROADMAP.md`](./ROADMAP.md) for detailed contracts and current test/benchmark gaps.
