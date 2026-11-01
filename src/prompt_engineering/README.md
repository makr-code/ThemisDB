---
Author: "ThemisDB Team"
Created: "2026-08-10"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# ThemisDB Prompt Engineering Module

## Purpose and ownership

The module provides C++ library interfaces for prompt templates, context substitution, validation, version history, scoring and optimization, feedback, prompt-specific metrics, and rewrite rules. It does not own model serving, inference transport, or the policy decisions of the calling service.

## Runtime flow and boundaries

For the integration façade, the host calls `PromptEngineeringIntegration::beforeExecution()`, sends the returned `ExecutionContext::enhanced_prompt` through its own LLM adapter, and then calls `afterExecution()`. The module does not make the model call, and callers define their own authorization, retry, and output-validation policies.

`RewriteEngine` is a separate implemented API. Its code is built into `themis_core`, but the integration hooks do not invoke it automatically. See [`IMPLEMENTATION.md`](./IMPLEMENTATION.md) before assuming a request path applies rewrite rules.

## Source map

- Public contracts: [`../../include/prompt_engineering/`](../../include/prompt_engineering/)
- Implementations: this directory
- Focused tests: [`../../tests/prompt_engineering/`](../../tests/prompt_engineering/)
- Benchmarks: [`../../benchmarks/prompt_engineering/`](../../benchmarks/prompt_engineering/)
- API, implementation, and runtime guidance: [`API.md`](./API.md), [`IMPLEMENTATION.md`](./IMPLEMENTATION.md), [`MODULE_GUIDE.md`](./MODULE_GUIDE.md)

The public-header index lists all current interfaces. Source declarations and implementation are authoritative for exact signatures and behavior.

## Build and verification

The root CMake configuration registers prompt-engineering implementation sources in `themis_core`. Focused test targets are discovered from `tests/prompt_engineering/CMakeLists.txt`; benchmark targets are registered by the repository benchmark build when enabled. Use [`MODULE_GUIDE.md`](./MODULE_GUIDE.md) for the existing configure, build, and CTest commands.

Code presence and documentation do not establish release readiness. Test results, benchmark baselines, and human acceptance must be verified independently.
