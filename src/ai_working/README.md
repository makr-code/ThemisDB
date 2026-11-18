# ai_working Module

<!-- Status: current | validated: 2026-11-18 -->
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
| `tests/ai_working/` | Standard-library Python tests; no C++ tests or CTest target |
| `benchmarks/ai_working/` | README only; no benchmark code or target |

## Developer Quickstart

There is no C++ module target to build or run. The available governance commands are:

```bash
python scripts/archive_ai_working.py --check-freshness
python scripts/archive_ai_working.py --check-freshness --strict
python scripts/archive_ai_working.py --archive-stale --dry-run
python -m unittest discover -s tests/ai_working -p 'test_*.py'
python scripts/ai-working-compact.py --all --dry-run --repo-root .
```

`--check-freshness` checks nine required module documents for presence, a placeholder pattern, and minimum content length; it does not validate factual currency. Missing, unreadable, or placeholder documents fail; documents shorter than 200 non-whitespace characters warn unless `--strict` is supplied. Review `--archive-stale --dry-run` output before running without `--dry-run`: it archives every unprotected top-level Markdown file in `src/ai_working/`, regardless of age or semantic status.

The root `ai_working/` documents are a separate working-artifact area, not module source or runtime data. `scripts/ai-working-compact.py --all` compacts eligible root files, archives files older than the configured cutoff (90 days by default), and removes empty root files and old archive entries (older than 365 days by default). Use `--dry-run` to preview these operations. Compacted pages are explicitly non-normative; unreadable inputs fail the command instead of being silently omitted. Archive name collisions receive a unique suffix.

Each maintenance-workflow push loop makes at most three attempts for non-fast-forward conflicts and records its run log and freshness report as a 14-day artifact. The compaction job reports a protected-branch push refusal as a warning and skips it; the separate wiki-sync job does not use that skip policy. A successful maintenance job therefore does not prove that generated changes were pushed. The module has no C++ runtime, public API, invalid-input runtime behavior, fallback, or recovery loop; those concepts do not apply here.

## Known Limitations

- This module contains no C++ implementation; all content is documentation and planning artifacts.
- The module archive helper moves non-protected top-level Markdown files to `docs/ARCHIVED/ai-working-history/`; it does not determine whether they are superseded.
- The module has no release-critical runtime behavior.
- The focused Python tests cover the freshness whitespace threshold, generated-summary authority labeling, unreadable-source failure, and archive-collision preservation; they do not establish full lifecycle or performance coverage.

## Related Documentation

- [ARCHITECTURE.md](ARCHITECTURE.md) — structural overview and context
- [ROADMAP.md](ROADMAP.md) — planned and completed work
- [FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md) — extension areas
- [CHANGELOG.md](CHANGELOG.md) — governance change history
- [AUDIT.md](AUDIT.md) — compliance and audit findings
- [SECURITY.md](SECURITY.md) — security considerations
