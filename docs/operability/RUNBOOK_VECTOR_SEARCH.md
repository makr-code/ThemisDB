[docs](../README.md) > [operability](./README.md) > [vector search](../../src/vector_search/README.md) > [runbook](./RUNBOOK_VECTOR_SEARCH.md)

**Author:** ThemisDB Contributors
**Created:** 2026-09-16
**Last Updated:** 2026-11-11
**Status:** review
**Datum:** 2026-11-11
**Primary (Quelle der Wahrheit):** `src/vector_search/RUNBOOK.md`, `src/vector_search/PRODUCTION_REQUIREMENTS.md`
**Bezug / Reference:** [Issue #7791](https://github.com/makr-code/ThemisDB/issues/7791)
**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot coding agent
**Human_Review:** false
**Human_Author:** unknown

> Exact model backend not verifiable from final document text alone.

# Vector Search Runbook Entry Point

This page is a navigation entry point. The source-backed procedures and
limitations for the shared vector-search runtime are maintained in the module
[runbook](../../src/vector_search/RUNBOOK.md).

The runtime is owned by `VectorIndexManager` in the shared `index` subsystem,
not by a standalone `src/vector_search/` service. This page does not define or
promise admin endpoints, Prometheus metric names, SLO thresholds, or automatic
index-rebuild behavior. Confirm deployment-specific controls with the owning
service before taking operational action.

Related references:

- [Vector-search module overview](../../src/vector_search/README.md)
- [Production requirements](../../src/vector_search/PRODUCTION_REQUIREMENTS.md)
- [Performance evidence limits](../../src/vector_search/PERFORMANCE_EXPECTATIONS.md)
