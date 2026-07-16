### Context

This issue implements the roadmap item 'CLI Migration Scanner' for the config domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: CLI Migration Scanner

### Goal

Deliver the scoped changes for CLI Migration Scanner in src/config/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### CLI Migration Scanner
**Priority:** High
**Target Version:** v1.8.0

Implement a CLI tool (`tools/config_migration_scanner`) that scans a deployment directory tree for files referencing legacy config paths and outputs a migration report (current path → new path, deprecation status, removal deadline).

**Implementation Notes:**
- `[x]` New binary target in `tools/config_migration_scanner.cpp`; links against `config_path_resolver` only (minimal dependencies).
- `[x]` Accepts `--root <dir>` (default `.`) and `--output {text,json,csv}` flags; scans `.yaml`, `.json`, `.toml`, `.ini`, `.env` files recursively.
- `[x]` For each discovered legacy path reference, outputs: `file`, `line`, `legacy_path`, `new_path`, `deprecated_date`, `removal_date`, `migration_guide_url` (from `PathMappingMetadata`).
- `[x]` `--dry-run` mode: prints what would be renamed without modifying files.
- `[x]` `--fix` mode: rewrites file contents replacing legacy path strings with new paths (with backup `.bak` files).
- `[x]` Returns exit code `1` if any paths past `removal_date` are found (usable as a CI gate).

**Performance Targets:**
- Scan of 10,000 config files (avg 100 lines each) completes in < 5 s on a single thread.
- `--fix` mode for 500 files rewrites in < 10 s (bounded by disk I/O).

---

### Acceptance Criteria

- [ ] New binary target in `tools/config_migration_scanner.cpp`; links against `config_path_resolver` only (minimal dependencies).
- [ ] Accepts `--root <dir>` (default `.`) and `--output {text,json,csv}` flags; scans `.yaml`, `.json`, `.toml`, `.ini`, `.env` files recursively.
- [ ] For each discovered legacy path reference, outputs: `file`, `line`, `legacy_path`, `new_path`, `deprecated_date`, `removal_date`, `migration_guide_url` (from `PathMappingMetadata`).
- [ ] `--dry-run` mode: prints what would be renamed without modifying files.
- [ ] `--fix` mode: rewrites file contents replacing legacy path strings with new paths (with backup `.bak` files).
- [ ] Returns exit code `1` if any paths past `removal_date` are found (usable as a CI gate).
- [ ] Scan of 10,000 config files (avg 100 lines each) completes in < 5 s on a single thread.
- [ ] `--fix` mode for 500 files rewrites in < 10 s (bounded by disk I/O).

### Relationships

- Roadmap row: #60 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/config/FUTURE_ENHANCEMENTS.md#cli-migration-scanner
- Source key: roadmap:60:config:v1.8.0:cli-migration-scanner

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:60:config:v1.8.0:cli-migration-scanner -->
<!-- roadmap-ref: row=60;module=config;target=v1.8.0 -->
<!-- roadmap-detail: src/config/FUTURE_ENHANCEMENTS.md#cli-migration-scanner -->
