---
Author: ThemisDB Team
Created: 2026-10-18
Last Updated: 2026-10-31
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

Exact model backend not verifiable from final document text alone.

# Ethics AI Module Features

## Role

The module provides an ethics plugin runtime that loads philosophy profiles, initializes and continues debates, stores arguments/decisions, assembles RAG context, evaluates decisions, and exposes plugin metrics/lifecycle operations.

## Source-backed capabilities

- Profile loading and lookup through the loader/registry APIs.
- Debate, argument, decision, and context helper paths in the module runtime.
- Selection and LDM planning/helper APIs, including equal initial weight in non-selection plans.
- Focused test and benchmark targets registered by CMake.

## Limitations and trade-offs

- The current plugin decision flow delegates to `EthicalDiscourseEngine`; it does not invoke `EthicsSelectionRouter`, `DiscourseOrchestrator`, `MetaVerdictBuilder`, or `MirrorSchoolHandler`.
- The fixed plugin CMake source list compiles some LDM helpers, but compile inclusion is not runtime integration. The default orchestrator fallback ignores dilemma text and selects synthetic output from `school_id.size() % 3` (the source comment calls it hash-based).
- LDM helper defaults use deterministic synthetic output. LDM benchmark fixtures use a mock registry/stub inference and do not establish production latency or decision quality.
- The asset directory contains 26 YAML profile files; the loaded count depends on the configured profile directory.
- This page makes no regulatory-compliance or release-readiness claim.

## References

- Runtime and source limits: [README.md](README.md), [ARCHITECTURE.md](ARCHITECTURE.md)
- Current implementation work: [ROADMAP.md](ROADMAP.md), [FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md)
- Benchmark mapping and evidence limits: [PERFORMANCE_EXPECTATIONS.md](PERFORMANCE_EXPECTATIONS.md)
- Security and operational requirements: [SECURITY.md](SECURITY.md), [PRODUCTION_REQUIREMENTS.md](PRODUCTION_REQUIREMENTS.md)
