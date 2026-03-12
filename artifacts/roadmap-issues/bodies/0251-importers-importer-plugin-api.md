### Context

This issue implements the roadmap item 'Importer Plugin API' for the importers domain. It is sourced from the consolidated roadmap under 🟢 Low Priority — Future (v1.9.0+) and targets milestone v1.8.0.

Primary detail section: Importer Plugin API

### Goal

Deliver the scoped changes for Importer Plugin API in src/importers/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### Importer Plugin API
**Priority:** Low
**Target Version:** v1.9.0

Add a stable plugin API (`IImporter` + `ImporterPlugin` factory) that allows third-party importers to be compiled as shared libraries and loaded at runtime via `ImporterRegistry::loadPlugin(path)`. This is required for proprietary source connectors (Oracle, MSSQL, Salesforce) that cannot be distributed with ThemisDB due to licensing.

**Implementation Notes:**
- Define a C-linkage plugin ABI in `include/importers/importer_plugin.h`; use a `THEMIS_IMPORTER_PLUGIN_V1` versioned struct to allow ABI evolution without breaking existing plugins.
- `ImporterRegistry::loadPlugin(path)` uses `dlopen`/`dlsym` (Linux/macOS) or `LoadLibrary`/`GetProcAddress` (Windows) to load the factory symbol `themis_importer_create`.
- Plugin isolation: each plugin runs in a sandboxed thread group with a configurable memory limit; a plugin that allocates beyond its limit is terminated and the import job fails gracefully.
- Document the plugin API with a worked example Oracle importer skeleton in `docs/importers/plugin_guide.md`.

**Performance Targets:**
- Plugin load time (cold `dlopen`) ≤ 50 ms; negligible impact on import throughput once loaded.
- Plugin API version check on load adds ≤ 1 ms startup overhead.

---

### Acceptance Criteria

- [ ] Define a C-linkage plugin ABI in `include/importers/importer_plugin.h`; use a `THEMIS_IMPORTER_PLUGIN_V1` versioned struct to allow ABI evolution without breaking existing plugins.
- [ ] `ImporterRegistry::loadPlugin(path)` uses `dlopen`/`dlsym` (Linux/macOS) or `LoadLibrary`/`GetProcAddress` (Windows) to load the factory symbol `themis_importer_create`.
- [ ] Plugin isolation: each plugin runs in a sandboxed thread group with a configurable memory limit; a plugin that allocates beyond its limit is terminated and the import job fails gracefully.
- [ ] Document the plugin API with a worked example Oracle importer skeleton in `docs/importers/plugin_guide.md`.
- [ ] Plugin load time (cold `dlopen`) ≤ 50 ms; negligible impact on import throughput once loaded.
- [ ] Plugin API version check on load adds ≤ 1 ms startup overhead.

### Relationships

- Roadmap row: #251 (🟢 Low Priority — Future (v1.9.0+))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/importers/FUTURE_ENHANCEMENTS.md#importer-plugin-api
- Source key: roadmap:251:importers:v1.8.0:importer-plugin-api

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:251:importers:v1.8.0:importer-plugin-api -->
<!-- roadmap-ref: row=251;module=importers;target=v1.8.0 -->
<!-- roadmap-detail: src/importers/FUTURE_ENHANCEMENTS.md#importer-plugin-api -->
