# ThemisDB Future Enhancements (Root Index)

> Status: Active
> Last Updated: 2026-08-28

This root file is the canonical entry point for future enhancement planning.
Detailed implementation backlogs live close to code and are aggregated in module-level documents.

## Root Planning Scope

- Cross-module enhancement direction and release prioritization are tracked in `ROADMAP.md`.
- Module-specific enhancement details are tracked in `src/FUTURE_ENHANCEMENTS.md` and `src/<module>/FUTURE_ENHANCEMENTS.md`.
- Archived enhancement snapshots are stored in `docs/ARCHIVED/ai-working-history/`.

## Source-Verified Roadmap Reality (2026-09-07)

The enhancement plan must follow the same evidence discipline as the roadmap itself: no status claim is treated as release evidence unless backed by source, tests, or CI artifacts.

### Wave model, reality-based

- **Wave A — Runtime reliability**: dedicated transaction/GPU CI execution is green again on `develop`, but GPU reduction and representative-hardware evidence remain the main release blockers.
- **Wave B — Performance consolidation**: materially complete for search, access model, and LLM wiki, but representative-hardware and benchmark closure remain incomplete.
- **Wave C — Security and audit validation**: implementation is present and useful, but end-to-end certification remains partial.
- **Wave D — Operability and soak hardening**: mostly planning; active implementation is still low compared to earlier WAVE claims.

### Realistic target sequence

1. Close the remaining Query FTS gate plus GPU/Transaction representative-hardware evidence gaps.
2. Complete representative-hardware benchmarks for critical modules.
3. Convert audit and security evidence from design-validation into end-to-end proof.
4. Finish Wave D operability and soak-test work only after Wave A/B evidence is stable.

### Findings-fix execution order (2026-09-09)

1. Build a source-validated remediation backlog from `ROADMAP.md`, module roadmaps, and current CI logs.
2. Repair the Wave-B transaction CI lane on `develop` so it provisions the same required dependency set as the working mainline/release workflows. ✅ closed 2026-09-09 (`34313051247`)
3. Repair the Wave-A GPU CI lane on `develop` so public/community execution does not fetch unavailable private submodules and so target compilation status is reported correctly. ✅ closed 2026-09-09 (`34313042741`)
4. Close the Query FTS performance gate (`<=100ms` on 100K documents) with reproducible evidence; production-backed executor coverage/harness is now in-tree, but the final representative measurement remains open.
5. Refresh representative-hardware and p95/p99 baselines for GPU, Transaction, Query, LLM Wiki, Auth, and Voice.
6. Execute the remaining high-risk hardening backlog in `llm`, `server`, `training`, and `llm_wiki`.
7. Re-sync root governance docs only after blocker status is re-validated from code, tests, CI, and benchmarks. ✅ in progress after the CI-green revalidation
8. Request human GA/program sign-off only after no critical blocker remains open.

## Required Enhancement Sources

- [src/FUTURE_ENHANCEMENTS.md](src/FUTURE_ENHANCEMENTS.md)
- [src/README.md](src/README.md)
- [ROADMAP.md](ROADMAP.md)
- [CHANGELOG.md](CHANGELOG.md)

## Governance Constraints

- Enhancement entries should include measurable targets, failure behavior, and test strategy.
- Ambiguous placeholders like "improve" or "optimize" must be replaced with concrete acceptance criteria.
- Branch and release assumptions must align with `BRANCHING_STRATEGY.md`, `RELEASE_STRATEGY.md`, and `VERSIONING.md`.
