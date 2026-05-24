# Pull Request

## Description

<!-- Describe the changes in this PR -->

## Linked Issues

<!-- Reference related issues: "Closes #123", "Fixes #456", "Related to #789" -->

## Type of Change

- [ ] Bug fix (non-breaking)
- [ ] New feature (non-breaking)
- [ ] Refactoring (non-breaking)
- [ ] Documentation
- [ ] Breaking change (requires MAJOR version bump — see [VERSIONING.md](../VERSIONING.md))
- [ ] Security fix
- [ ] Other:

## Breaking Change Checklist

<!-- Only fill out if "Breaking change" is checked above -->

- [ ] MAJOR version bump planned in `VERSION` and `CMakeLists.txt`
- [ ] Migration guide added in `docs/migration/`
- [ ] Announcement prepared for GitHub Discussions (≥ 2 weeks before release)
- [ ] CHANGELOG `### Removed` / `### Changed` section updated

## Testing

- [ ] Unit tests added/updated
- [ ] Integration tests added/updated
- [ ] Manual testing performed
- [ ] Benchmarks run (if performance-sensitive change)

## 📚 Research & Knowledge (wenn applicable)

- [ ] Diese PR basiert auf wissenschaftlichen Paper(s) oder Best Practices?
  - Falls JA: Research-Dateien in `/docs/research/` angelegt?
  - Falls JA: Im Modul-README unter "Wissenschaftliche Grundlagen" verlinkt?
  - Falls JA: In `/docs/research/implementation_influence/` eingetragen?

**Relevante Quellen:**
- [ ] Paper: <!-- docs/research/papers/<file>.md -->
- [ ] Best Practice: <!-- docs/research/best_practices/<file>.md -->
- [ ] Architecture Decision: <!-- docs/research/architecture_decisions/adr_<NNN>.md -->

## AI-Generated Code (KI-generierter Code)

<!-- Tool-Referenz: siehe `.github/instructions/cpp-language-service-tools.instructions.md` -->

- [ ] Symbol-Referenzen mit `GetSymbolReferences_CppTools` geprüft (siehe `.github/instructions/cpp-language-service-tools.instructions.md`)
- [ ] Keine rohen Pointer und kein `new`/`delete` ohne explizites Review eingeführt
- [ ] RAII und Exception-Safety für neue/angepasste Pfade geprüft
- [ ] Keine unnötig komplexen KI-Abstraktionen eingeführt
- [ ] Performance-Metriken geprüft, falls Hotpath betroffen

## Checklist

- [ ] Code follows project style guidelines (clang-format / clang-tidy)
- [ ] Self-review completed
- [ ] Documentation updated (if needed)
- [ ] CHANGELOG.md updated under `[Unreleased]`
- [ ] No new warnings introduced
- [ ] Security-sensitive paths reviewed by security maintainer (if applicable)

## Scanner and IntelliSense Gates

- [ ] IntelliSense/Compiler: no new errors in changed files
- [ ] clang-tidy/cppcheck: no new high-risk findings in changed files
- [ ] Gap Scanner: no new `critical` findings in categories `security`, `input_validation`, `query_correctness`, `distributed_consistency`, `concurrency`, `memory`
- [ ] Gap Scanner: no new `high` findings in the same categories (or explicitly approved)
- [ ] Gap Scanner delta report attached (baseline vs current), not only absolute totals
- [ ] New `unknown` scanner findings triaged (fixed, re-categorized, or justified)

