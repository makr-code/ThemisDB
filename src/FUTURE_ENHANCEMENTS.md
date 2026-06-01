# Future Enhancements - Source Root

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ROADMAP.md · ARCHITECTURE.md -->

## Scope

- improvements to source-root aggregation, cross-module documentation governance, and analysis workflows
- alignment work between module-local docs and source-root summary artifacts

## Design Constraints

- root-level docs must not replace module-local source-of-truth contracts.
- aggregation artifacts must stay clearly distinguished from owning implementation docs.
- inventory and matrix semantics must remain explicit for module rows versus the <root> row.

## Required Interfaces

| Interface | Requirement |
|---|---|
| root inventory docs | reflect current source-root and module aggregation semantics |
| root summary docs | explain the relationship between module-local docs and cross-module summaries |
| analysis artifacts | remain clearly labeled as review/planning aids rather than runtime contracts |

## Implementation Notes

- keep root-level summaries synchronized with inventory semantics and module counts.
- document root-level cross-module artifacts explicitly in navigation docs.
- tighten wording where earlier summaries implied only module directories under src mattered.

## Test Strategy

- regenerate developer-doc inventory after root-level doc changes.
- verify filename matrix still represents <root> separately from module rows.
- cross-check root summaries against a representative sample of current module-local docs.

## Performance Targets

- no runtime performance target; this file governs documentation and aggregation quality.
- inventory regeneration should remain deterministic and reproducible.

## Security / Reliability

- prevent governance drift between root summaries and module-local docs.
- keep security-sensitive aggregation wording aligned with current audits.
- avoid ambiguous wording that conflates module rows and root-level summary docs.