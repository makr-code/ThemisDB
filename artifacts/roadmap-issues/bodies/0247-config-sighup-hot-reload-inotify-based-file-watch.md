### Context

This issue implements the roadmap item 'SIGHUP Hot-Reload: inotify-Based File Watch' for the config domain. It is sourced from the consolidated roadmap under 🟢 Low Priority — Future (v1.9.0+) and targets milestone v1.8.0.

Primary detail section: SIGHUP Hot-Reload: inotify-Based File Watch

### Goal

Deliver the scoped changes for SIGHUP Hot-Reload: inotify-Based File Watch in src/config/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### SIGHUP Hot-Reload: inotify-Based File Watch
**Priority:** Low
**Target Version:** v1.8.0

`config_path_resolver.cpp` (line 1764) explicitly logs "SIGHUP hot-reload not supported on Windows" and on POSIX registers a SIGHUP handler that sets a flag, but there is no inotify/kqueue watch that would trigger reload when config files actually change on disk. Operators must manually send SIGHUP.

**Implementation Notes:**
- `[x]` Add an optional `ConfigFileWatcher` class (Linux: inotify, macOS: kqueue, Windows: `ReadDirectoryChangesW`) that watches the `config/` directory tree and invokes a reload callback when any `.yaml`/`.json` file changes.
- `[x]` Wire `ConfigFileWatcher` into `ConfigPathResolver::startHotReload()` as an optional enhancement alongside the existing SIGHUP path.
- `[x]` Debounce rapid file-system events (e.g., editor save-then-rename) with a 200 ms settling window.

---



[1] G. Baazizi, H. B. Lahmar, D. Colazzo, G. Ghelli, and C. Sartiani, "Schema Inference for Massive
JSON Datasets," *Proc. 20th International Conference on Extending Database Technology (EDBT)*,
pp. 222–233, Mar. 2017. DOI: 10.5441/002/edbt.2017.21.

[2] L. Pina, L. Zheng, M. Rinard, and J. Gama, "Incremental Schema Validation for JSON Documents,"
*Proc. 36th IEEE International Conference on Data Engineering (ICDE)*, pp. 469–480, Apr. 2020.
DOI: 10.1109/ICDE48307.2020.00047.

[3] Internet Engineering Task Force (IETF), "JSON Schema: A Media Type for Describing JSON
Documents," Internet-Draft draft-bhutton-json-schema-01, Dec. 2020.
Available: https://json-schema.org/specification.html

[4] Internet Engineering Task Force (IETF), "JSON Schema Validation: A Vocabulary for Structural
Validation of JSON," Internet-Draft draft-bhutton-json-schema-validation-01, Dec. 2020.
Available: https://json-schema.org/specification.html
*(Normative specification for `allOf`, `anyOf`, `oneOf`, `$ref`, `format`, `uniqueItems` keywords
implemented in § "ConfigSchemaValidator: Extended JSON Schema Keyword Support".)*

### Acceptance Criteria

- [x] Add an optional `ConfigFileWatcher` class (Linux: inotify, macOS: kqueue, Windows: `ReadDirectoryChangesW`) that watches the `config/` directory tree and invokes a reload callback when any `.yaml`/`.json` file changes.
- [x] Wire `ConfigFileWatcher` into `ConfigPathResolver::startHotReload()` as an optional enhancement alongside the existing SIGHUP path.
- [x] Debounce rapid file-system events (e.g., editor save-then-rename) with a 200 ms settling window.

### Relationships

- Roadmap row: #247 (🟢 Low Priority — Future (v1.9.0+))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/config/FUTURE_ENHANCEMENTS.md#sighup-hot-reload-inotify-based-file-watch
- Source key: roadmap:247:config:v1.8.0:sighup-hot-reload-inotify-based-file-watch

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:247:config:v1.8.0:sighup-hot-reload-inotify-based-file-watch -->
<!-- roadmap-ref: row=247;module=config;target=v1.8.0 -->
<!-- roadmap-detail: src/config/FUTURE_ENHANCEMENTS.md#sighup-hot-reload-inotify-based-file-watch -->
