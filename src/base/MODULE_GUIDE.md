---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

Source level: 1
SOT domain: module-behavior
Canonical references: `include/base/`, `include/themis/base/`, `src/base/`, `tests/base/CMakeLists.txt`
Related issues: #7093, #7167

# Base module guide

## Audience and ownership

For maintainers changing shared runtime helpers, resource pools, module loading, dependency ordering, or reload. The base module is infrastructure; keep feature-specific behavior in its owning module.

## Find the matching API

- Memory buffers and connection slots: `include/base/` and corresponding `src/base/` implementation.
- Module loading and replacement: `include/themis/base/module_loader.h`, `hot_reload_manager.h`, `plugin_dependency_graph.h`.
- Tests and registered targets: `tests/base/CMakeLists.txt`.

## Maintainer workflow

1. Establish which public header and namespace the caller uses.
2. Trace ownership, lifetime, timeout, shutdown, or staged-load transitions in implementation.
3. Cover exhaustion/timeout and teardown/failure cases, not only successful acquisition or activation.
4. Use the closest test under `tests/base/`; `test_base_reload_regression.cpp` covers reload regressions.
5. Update `API.md` for caller-visible semantics and `IMPLEMENTATION.md` for changed runtime invariants.

See `ARCHITECTURE.md`, `ROADMAP.md`, and `SECURITY.md` for wider module context.
