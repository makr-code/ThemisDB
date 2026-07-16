# Importers Documentation

## Purpose

This directory contains operator- and developer-facing importer guides and references.
It is a secondary documentation layer and must stay aligned with module-level planning in `src/importers/`.

## Alignment Contract

Primary workload and behavior sources:

- `src/importers/FUTURE_ENHANCEMENTS.md`
- `src/importers/MODULE_GAPS.md`
- `src/importers/ROADMAP.md`

Rule:

- Newer planning documents are more relevant than older historical reports.
- This directory must not claim production behavior that is contradicted by newer gap/planning files.

## Current Scope

- `POSTGRES_IMPORTER_V2.md`
- `plugin_guide.md`
- `MDM_ARCHITECTURE.md`
- `MDM_API_REFERENCE.md`
- `MDM_USER_GUIDE.md`
- Wikipedia importer runtime flow is documented via `include/importers/README.md` and `src/importers/README.md` (full import, delta update, verify, export for `wikipedia.db`)

## Maintenance Notes

- Keep connector behavior, schema handling, and conflict semantics consistent with `src/importers/` docs.
- Use `docs/en/importers/PRIMARY_SOURCES.md` and `docs/de/importers/PRIMARY_SOURCES.md` as authoritative mapping pages.
- Treat archived or implementation-history reports as background context only.

## Installation

- Build the repository with the importer/plugin modules enabled; the Wikipedia importer ships as a built-in importer plugin.

## Usage

- Use `WikipediaIngestionPlugin::runFullImport(...)` for the initial dump, `runIncrementalUpdate(...)` for later deltas, `validateDatabase()` for verification, and `exportPortable(...)` for `wikipedia.db` + `manifest.json`.

---

Updated during docs-vs-planning alignment sweep on 2026-05-31.
