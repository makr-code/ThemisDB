### Context

This issue implements the roadmap item 'Cross-Platform Module Format' for the base domain. It is sourced from the consolidated roadmap under 🟢 Low Priority — Future (v1.9.0+) and targets milestone v1.4.0.

Primary detail section: Cross-Platform Module Format

### Goal

Deliver the scoped changes for Cross-Platform Module Format in src/base/ and complete the linked detail section in a release-ready state for v1.4.0.

### Detailed Scope

### Cross-Platform Module Format
**Priority:** Low
**Target Version:** v1.4.0

Universal module packaging format across Linux/macOS/Windows, including platform-independent manifest, auto-detected native library bundling, and resource embedding.

**Implementation Notes:**
- `[ ]` Define a `PluginBundle` format (zip archive with `manifest.json`, native `.so`/`.dll`/`.dylib`, optional WASM fallback, and Ed25519 signature file).
- `[ ]` Implement `PluginBundleLoader` in `module_loader.cpp` that unpacks to a temp dir, verifies signature, selects the correct native binary for the current platform, and delegates to the existing `PluginLoader`.
- `[ ]` Support WASM-only bundles as a portable fallback when no native library for the current platform is present.

---

### Acceptance Criteria

- [ ] Define a `PluginBundle` format (zip archive with `manifest.json`, native `.so`/`.dll`/`.dylib`, optional WASM fallback, and Ed25519 signature file).
- [ ] Implement `PluginBundleLoader` in `module_loader.cpp` that unpacks to a temp dir, verifies signature, selects the correct native binary for the current platform, and delegates to the existing `PluginLoader`.
- [ ] Support WASM-only bundles as a portable fallback when no native library for the current platform is present.

### Relationships

- Roadmap row: #243 (🟢 Low Priority — Future (v1.9.0+))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/base/FUTURE_ENHANCEMENTS.md#cross-platform-module-format
- Source key: roadmap:243:base:v1.4.0:cross-platform-module-format

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:243:base:v1.4.0:cross-platform-module-format -->
<!-- roadmap-ref: row=243;module=base;target=v1.4.0 -->
<!-- roadmap-detail: src/base/FUTURE_ENHANCEMENTS.md#cross-platform-module-format -->
