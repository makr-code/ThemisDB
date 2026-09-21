# ai_working Module

<!-- Status: current | validated: 2026-09-21 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The `ai_working` module is the designated context and working-artifact container for AI agent planning and execution activities within ThemisDB. It provides a governed location for AI-generated planning documents, gap analysis artifacts, wave execution reports, and agent collaboration materials produced during active development cycles.

## Scope

In scope:
- AI agent working documents and execution plans
- Wave execution status reports and completion summaries
- Cross-module gap analysis and action packets produced by AI agents
- Agent collaboration artifacts and design review facilitation documents

Out of scope:
- Runtime C++ source code implementation (no production code in this module)
- Module-internal API headers
- Test or benchmark executables

## Module Layout

| Path | Role |
|---|---|
| `ai_working/` (root) | AI working artifacts, wave plans, execution reports |
| `src/ai_working/` | Module governance documentation |
| `tests/ai_working/` | Placeholder test directory |
| `benchmarks/ai_working/` | Placeholder benchmark directory |

## Known Limitations

- This module contains no C++ implementation; all content is documentation and planning artifacts.
- Stale wave-specific documents are archived to `docs/ARCHIVED/` when superseded.
- The module has no release-critical runtime behavior.

## Related Documentation

- [ARCHITECTURE.md](ARCHITECTURE.md) — structural overview and context
- [ROADMAP.md](ROADMAP.md) — planned and completed work
- [FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md) — extension areas
- [CHANGELOG.md](CHANGELOG.md) — governance change history
- [AUDIT.md](AUDIT.md) — compliance and audit findings
- [SECURITY.md](SECURITY.md) — security considerations
