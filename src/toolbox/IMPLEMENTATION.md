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

# Toolbox implementation notes

**Purpose and audience:** Explains lifecycle, construction, execution, and failure invariants for developers modifying `src/toolbox`.

## Construction and lifecycle

`IngestionToolbox::createDefault()` creates a workflow engine, installs the built-in NER and LLM extraction steps, and uses `NullTextGenerationBackend` unless a real backend is injected. Tensor decomposition and tensor-core steps are initially registered with null backends. Configure production dependencies with [`ToolboxBuilder`](../../include/toolbox/toolbox_builder.h), then create the toolbox once and inject or register it in the server bootstrap.

`ToolboxBuilder` is single-use: `build()` or `buildWithBridges()` marks it built, and a second build throws `std::logic_error`. Empty profile paths and null workflow engines, format extractors, or factories are rejected. Profile files are loaded in registration order; a load failure is warned and does not prevent returning the toolbox. Do not treat successful construction as proof that every configured profile loaded.

`buildWithBridges()` conditionally creates an AQL bridge when a graph writer exists and a RAG bridge when either graph or vector writer exists. Bridge-construction exceptions are warnings and leave the corresponding bridge pointer empty. See [`toolbox_builder.cpp`](./toolbox_builder.cpp).

## Extraction path

`extractEntities()` and `extractEntitySet()` build a small `ExtractionContext` from the supplied text, MIME hint, and filename hint, then call `WorkflowEngine::execute()`. Empty input returns immediately with an empty result. Workflow errors also produce an empty result object; the caller must inspect the exported counters because an empty output alone does not distinguish an ordinary no-match from a workflow failure.

For non-empty text of at least eight bytes, a result with no entities is counted as an error by the `extractEntities()` path. `extractEntitySet()` considers either nodes or chunks sufficient for success. This threshold is an observability rule, not a workflow rejection rule.

## Registry and concurrency

The preferred ownership model is explicit instance injection. If process-global access is needed, initialize `ToolboxRegistry` during single-threaded startup before consumers call `instance()`. An uninitialized access throws `std::logic_error`; a second initialize replaces the previous instance and logs a warning. `reset()` is for isolated tests or controlled teardown and must not run while toolbox operations are active.

The toolbox façade and registry accessors synchronize their shared state, and workflow execution is delegated to the engine. A `ToolboxBuilder` instance itself is not thread-safe. Follow the individual helper contracts: `TextChunker` instances are not thread-safe, while stateless text helpers may be shared.

## Bridge behavior

`ContentToolboxBridge` requires non-null toolbox and initialized content manager dependencies. `ingest()` first stores/scans content through `ContentManager`, then enriches extracted text. No extracted text is a successful content-ingest result with no enrichment. Graph/vector writer failures are logged and counted separately, but do not change the overall `ok` result after enrichment. Check `graphWriteFailuresTotal()` and `vectorWriteFailuresTotal()` to detect partial sink failures.

`enrichExisting()` retrieves previously stored content and re-runs enrichment; it does not re-ingest the raw blob, repeat security scanning, or perform deduplication. Re-enrichment is not intrinsically idempotent for external sinks.

## Change validation

- Helper changes: `ctest --preset linux-release -R test_toolbox_primitives_toolbox_FocusedTests --output-on-failure`.
- Builder, registry, and bridge contracts: `ctest --preset linux-release -R test_toolbox_contract_hardening_focused_toolbox_FocusedTests --output-on-failure`.
- Ingestion integration: `ctest --preset linux-release -R test_toolbox_ingestion_toolbox_FocusedTests --output-on-failure`.
- Performance-sensitive changes: use `bench_toolbox_native_workloads` and the procedure in [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md); report actual hardware and raw results.

## References

- [`API.md`](./API.md)
- [`MODULE_GUIDE.md`](./MODULE_GUIDE.md)
- [`RUNBOOK.md`](./RUNBOOK.md)
- [`tests/toolbox/README.md`](../../tests/toolbox/README.md)

> Exact model backend not verifiable from final document text alone.
