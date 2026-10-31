# ai_working Module

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->
<!-- AI_Used: true | AI_Models: unknown | AI_Contributor: GitHub Copilot | Human_Review: false | Human_Author: unknown -->
<!-- Exact model backend not verifiable from final document text alone. -->

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
| `tests/ai_working/` | README and CMake minimum-version declaration only; no tests or registered test target |
| `benchmarks/ai_working/` | README only; no benchmark code or target |

## Developer Quickstart

There is no C++ module target to build or run. The available governance commands are:

```bash
python scripts/archive_ai_working.py --check-freshness
python scripts/archive_ai_working.py --check-freshness --strict
python scripts/archive_ai_working.py --archive-stale --dry-run
```

`--check-freshness` validates nine required module documents for presence, a placeholder pattern, and minimum content length; it does not validate their factual currency. `--strict` makes too-short documents fail. Review `--archive-stale --dry-run` output before running without `--dry-run`: the script archives every top-level Markdown file in `src/ai_working/` that is not protected, regardless of its actual age or status.

The root `ai_working/` documents are a separate working-artifact area. Their compaction, age-based archive rotation, cleanup, and wiki synchronization are handled by `scripts/ai-working-compact.py` and `.github/workflows/maintenance-ai-working.yml`; they are not module source or runtime data.

## Known Limitations

- This module contains no C++ implementation; all content is documentation and planning artifacts.
- The module archive helper moves non-protected top-level Markdown files to `docs/ARCHIVED/ai-working-history/`; it does not determine whether they are superseded.
- The module has no release-critical runtime behavior.

## Related Documentation

- [ARCHITECTURE.md](ARCHITECTURE.md) — structural overview and context
- [ROADMAP.md](ROADMAP.md) — planned and completed work
- [FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md) — extension areas
- [CHANGELOG.md](CHANGELOG.md) — governance change history
- [AUDIT.md](AUDIT.md) — compliance and audit findings
- [SECURITY.md](SECURITY.md) — security considerations
