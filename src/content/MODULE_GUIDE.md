**Author:** ThemisDB Team
**Created:** 2026-10-30
**Last Updated:** 2026-10-30
**Status:** active
**Source level:** Level 1 — `src/content/`
**SOT domain:** `module-behavior`
**Canonical source references:** [module README](README.md), [architecture](ARCHITECTURE.md), [public headers](../../include/content/README.md), [test registration](../../tests/content/CMakeLists.txt), [manager API](../../include/content/content_manager.h)
**Related issue:** #7109
**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot
**Human_Review:** false
**Human_Author:** ThemisDB Team
**Provenance note:** Exact model backend not verifiable from final document text alone.

# Content module developer guide

## Purpose and first steps

This guide helps a developer locate ownership, make a focused content-module change, and
validate it. Read the [module README](README.md) for scope, then
[architecture](ARCHITECTURE.md) for processing boundaries and the
[roadmap](ROADMAP.md) for tracked work. Public declarations live in
[`include/content/`](../../include/content/README.md); implementation lives here under
`src/content/`; focused module tests live in `tests/content/`.

## Select the right extension point

| Change | Primary seam |
|---|---|
| New format extraction | `IContentProcessor` contract and format processor/header |
| Ingestion routing or storage handoff | `ContentManager` |
| Validation or MIME rejection | `ContentValidator`, MIME/type registry |
| Pipeline-stage enablement/retry | `ProcessorChainConfig` |
| Async intake/back-pressure | `AsyncIngestionWorker` |
| Embedding generation | `EmbeddingPipeline` and manager integration |
| Security checks | `ContentSecurityManager`, policy, validator, and threat-specific interface |
| Metrics/error semantics | `ContentMetrics` and typed content errors |

Keep public API changes in `include/content/` and explain ownership, lifetime,
threading, errors, and edge cases in the header contract. Do not put vendor dependency
implementation under this module unless it is repository-owned.

## Implementation checklist

1. Confirm which existing processor or manager path owns the behavior.
2. Trace the public declaration into `src/content/` before making a change.
3. Preserve bounded inputs, explicit errors, and configured stage behavior.
4. Treat optional build features as conditional; check the
   [header overview](../../include/content/README.md) for current feature flags.
5. Add or extend focused tests in the existing content test suite. Docs-only work does
   not require adding a test file.
6. Update the matching canonical module document when behavior or a public contract
   changes.

## Build and focused test selection

The content test CMake file registers `test_*.cpp` files as focused tests and separately
registers the soak suite when present. Use the existing configured build directory:

```bash
cmake --build <build-dir> --target module_content_test_content_embedding_pipeline_focused
ctest --test-dir <build-dir> -R '^test_content_embedding_pipeline_content_FocusedTests$'
```

Replace the target and CTest expression with the corresponding filename stem from
[`tests/content/CMakeLists.txt`](../../tests/content/CMakeLists.txt). A configured
build may use different enabled dependencies; this guide does not prescribe a
repository-wide preset.

## Common edge cases

- Empty/unreadable streams and MIME values that cannot be classified.
- Optional processor absent or disabled versus actual extraction failure.
- Non-streaming content exceeding the configured buffer limit.
- Exact chunk boundaries and reassembly with overlap.
- Stage overrides at MIME and category levels and old boolean JSON configuration.
- Empty embedding vectors and model/backend ambiguity.

Test coverage is evidence for specific cases, not proof that every format, dependency
combination, or deployment is covered.

## Sources

- [Module README](README.md), [architecture](ARCHITECTURE.md),
  [roadmap](ROADMAP.md), and [security](SECURITY.md).
- [Public-header overview](../../include/content/README.md),
  [manager API](../../include/content/content_manager.h),
  [processor API](../../include/content/content_processor.h), and
  [stage configuration](../../include/content/processor_chain_config.h).
- [Content test registration](../../tests/content/CMakeLists.txt),
  [pipeline tests](../../tests/content/test_content_pipeline.cpp), and
  [stage hardening tests](../../tests/content/test_content_pipeline_hardening.cpp).
