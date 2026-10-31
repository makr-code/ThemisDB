# Metadata Module Developer Guide

**Author:** ThemisDB Contributors
**Created:** 2026-10-31
**Last Updated:** 2026-10-31
**Status:** review
**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot coding agent / ThemisDB documentation orchestrator
**Human_Review:** false
**Human_Author:** ThemisDB Contributors
**Source level:** level1 — module-local
**SOT domain:** module-behavior
**Canonical source references:** [module README](./README.md), [module architecture](./ARCHITECTURE.md), [public metadata interfaces](../../include/metadata/), [metadata tests](../../tests/metadata/)
**Related issue / milestone:** [#7133](https://github.com/makr-code/ThemisDB/issues/7133) · DOC-WEEKLY-2026-W44

> Exact model backend not verifiable from final document text alone.

## Start here

1. Read [`README.md`](./README.md) for module scope and
   [`ARCHITECTURE.md`](./ARCHITECTURE.md) for execution planes and dependencies.
2. Use [MODELING.md](./MODELING.md) to understand schema, statistics, version,
   constraint, audit, and lineage objects.
3. Find the owning interface in [`include/metadata`](../../include/metadata/)
   and implementation in [`src/metadata`](./).
4. Find and read the focused test before changing a behavior; the mapping is
   summarized in [IMPLEMENTATION.md](./IMPLEMENTATION.md) and registered in
   [`tests/metadata/CMakeLists.txt`](../../tests/metadata/CMakeLists.txt).
5. Check [ROADMAP.md](./ROADMAP.md) for active module work. Planning is not
   evidence that a behavior has shipped.

## Ownership boundaries

- Metadata owns schema discovery and metadata surfaces; storage owns the
  underlying persistence engine.
- `StatisticsCollector` serves query-planning estimates; it does not own the
  query planner.
- `InformationSchema` exposes read-only views derived from `SchemaManager`.
- `CatalogExporter` serializes and sends catalog payloads; its tests inject the
  HTTP function and do not prove external service availability.
- `IMetadataExportPolicy` has standalone implementations, but the
  `CatalogExporter` constructor/publish API does not accept one; filtering and
  delay behavior are not implied for exporter calls.
- Lineage and pluggable security/encryption interfaces do not imply that every
  application caller is wired to use them.
- Vector is a schema type here, not an embedding-generation feature. See
  [EMBEDDING.md](./EMBEDDING.md).

## Change checklist

- State the affected public contract, error behavior, thread-safety, and
  ownership/lifetime constraints before editing.
- Add a focused regression for empty, malformed, missing, conflict, and
  failure-path behavior relevant to the change.
- For schema/version changes, consider audit history and rollback behavior.
- For catalog exports, test transport errors and payload shape without a live
  endpoint.
- Keep performance claims tied to a present benchmark source and recorded run;
  the module's [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md)
  is a target/mapping document, not a benchmark result.
- Update only the relevant module docs; do not use `docs/` as an independent
  source of truth.

## Documentation map

- [Research notes](./RESEARCH.md)
- [Modeling](./MODELING.md)
- [Embedding boundary](./EMBEDDING.md)
- [Implementation map](./IMPLEMENTATION.md)
- [API reference](./API.md)
- [Runbook](./RUNBOOK.md)
- [Operations](./OPERATIONS.md)
- [Trust and security](./TRUST.md)
