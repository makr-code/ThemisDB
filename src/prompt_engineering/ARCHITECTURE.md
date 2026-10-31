---
Author: "ThemisDB Team"
Created: "2026-09-09"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Architecture — Prompt Engineering Module

## Scope and build boundary

Public interfaces are declared under [`../../include/prompt_engineering/`](../../include/prompt_engineering/); implementation is in this directory. The root CMake source list adds the module implementation to `themis_core`. The library provides prompt-related operations and hooks; it does not own LLM serving, transport, or host authorization.

## Main runtime paths

1. **Template lifecycle:** `PromptManager` validates, stores, reads, and renders templates. It can operate in memory or use non-owning RocksDB handles supplied by its caller. `injectContext()` performs literal placeholder substitution; it does not escape inserted values.
2. **Version and quality workflows:** `PromptVersionControl` manages prompt revisions; `PromptEvaluator` scores outputs; `PromptOptimizer` runs a bounded loop using caller-supplied callbacks and cases.
3. **Integration hooks:** `PromptEngineeringIntegration::beforeExecution()` constructs an execution context and enhanced prompt. The host performs the model call and supplies its result to `afterExecution()`.
4. **Feedback and observability:** feedback and metric objects collect module-local records; the host owns persistence policy, metrics transport, and alert routing.
5. **Rewrite API:** `RewriteEngine` executes registered rules through ordered phases. This is separate from the integration hooks and is not automatically applied to requests.

## Failure and concurrency boundaries

Validation results, optional lookups, and rewrite results communicate operation outcomes; callers must inspect them. Persistence failures and provider failures have operation-specific behavior—see [`IMPLEMENTATION.md`](./IMPLEMENTATION.md) and the corresponding headers. Do not infer a module-wide thread-safety guarantee; follow each interface's documented synchronization and lifetime contract.

## Integration references

- Public interfaces and exact declarations: [`../../include/prompt_engineering/README.md`](../../include/prompt_engineering/README.md)
- Hook sequence and error guidance: [`API.md`](./API.md)
- Persistence, rewrite, and test boundaries: [`IMPLEMENTATION.md`](./IMPLEMENTATION.md)
- Operational responsibilities: [`OPERATIONS.md`](./OPERATIONS.md)
