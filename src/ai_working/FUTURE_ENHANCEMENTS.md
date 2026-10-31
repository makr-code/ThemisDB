# Future Enhancements — ai_working Module

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ROADMAP.md · PERFORMANCE_EXPECTATIONS.md -->
<!-- AI_Used: true | AI_Models: unknown | AI_Contributor: GitHub Copilot | Human_Review: false | Human_Author: unknown -->
<!-- Exact model backend not verifiable from final document text alone. -->

## Scope

- Structured lifecycle management for AI working artifacts beyond manual archival.
- Tooling integration so CI can validate artifact freshness and governance compliance.
- Potential lightweight utility code (C++) if working-context indexing is needed at runtime.

## Design Constraints

- The module must remain documentation-first; C++ implementation is only permitted if a concrete runtime need is established by a roadmap item.
- Archival conventions must stay aligned with `docs/ARCHIVED/` governance.
- No dependency on optional or external backends.

## Required Interfaces

| Interface | Requirement |
|---|---|
| Archival script | `scripts/archive_ai_working.py --archive-stale` moves every non-protected `.md` file at the top level of `src/ai_working/` into a date-stamped archive directory. It does not determine whether a file is stale. |
| CI governance check | `scripts/archive_ai_working.py --check-freshness` checks nine required docs for presence, a known placeholder pattern, and at least 200 non-whitespace characters. “Freshness” does not mean checking dates or factual accuracy. |
| Artifact index | optional structured index of active working documents for agent discovery |

## Implementation Notes

- Archival automation should be implemented as a lightweight Python script in `scripts/`.
- The freshness command is invoked in `.github/workflows/maintenance-ai-working.yml`; the separate `gate-pr-doc-metadata` workflow is not the implementation of this module-specific check.
- Any future C++ utility must follow RAII, `std::unique_ptr`, and Doxygen documentation rules.

## Test Strategy

- There are currently no tests under `tests/ai_working/`; the directory's CMake file declares no targets. Add direct tests for missing/short/placeholder docs and archive collisions before extending this Python tooling.
- Keep the archive helper's `--dry-run` path available for review; verify destination and collision behavior before moving working docs.

## Performance Targets

- No runtime performance targets apply; this module has no hot paths.
- Proposed target, not measured: archival script completes in under 10 seconds for a typical repository checkout. Record checkout size and command timing before treating this as an acceptance threshold.

## Security / Reliability

- Working artifacts must not contain credentials, secrets, or sensitive deployment parameters.
- Archival must preserve history and must not allow silent data loss.
- Secret scanning must be applied to all artifacts committed to the working directory.
