### Context

This issue implements the roadmap item 'Multi-Environment Config Overlay (dev/staging/prod)' for the config domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Mid-term (v1.9.0 – v2.0.0) and targets milestone v1.9.0.

Primary detail section: Multi-Environment Config Overlay (dev/staging/prod)

### Goal

Deliver the scoped changes for Multi-Environment Config Overlay (dev/staging/prod) in src/config/ and complete the linked detail section in a release-ready state for v1.9.0.

### Detailed Scope

### Multi-Environment Config Overlay (dev/staging/prod)
**Priority:** Medium
**Target Version:** v1.9.0

Config paths currently resolve against a single filesystem root. Add overlay support so that `dev` environments can override specific config files without modifying the `prod` set, using the same `PATH_MAPPING` table.

**Implementation Notes:**
- `[x]` Introduce `ConfigEnvironment` enum `{DEV, STAGING, PROD}` and `ConfigPathResolver::setEnvironment(ConfigEnvironment)`.
- `[x]` Environment-specific overlay root: `config/{env}/` checked first, then the standard `config/` root, then the legacy path.
- `[x]` Override via `THEMIS_CONFIG_ENV` environment variable (`dev`, `staging`, `prod`; default `prod`).
- `[x]` `PATH_MAPPING` keys are unchanged; only the filesystem search order gains the overlay prefix.
- `[x]` Cache key must incorporate `env` to prevent cross-environment cache poisoning: `cache_key = env + ":" + legacy_path`.
- `[x]` `setEnvironment()` clears the cache atomically to prevent stale overlay entries.
- `[x]` Decision: `getMetadata()` returns global metadata only; the overlay affects filesystem resolution only, not path mapping metadata.

**Performance Targets:**
- One additional filesystem `exists()` check per cache miss (overlay root probed first); negligible impact when cache hit rate > 95%.
- `setEnvironment()` clears the cache atomically to prevent stale overlay entries.

---

### Acceptance Criteria

- [ ] Introduce `ConfigEnvironment` enum `{DEV, STAGING, PROD}` and `ConfigPathResolver::setEnvironment(ConfigEnvironment)`.
- [ ] Environment-specific overlay root: `config/{env}/` checked first, then the standard `config/` root, then the legacy path.
- [ ] Override via `THEMIS_CONFIG_ENV` environment variable (`dev`, `staging`, `prod`; default `prod`).
- [ ] `PATH_MAPPING` keys are unchanged; only the filesystem search order gains the overlay prefix.
- [ ] Cache key must incorporate `env` to prevent cross-environment cache poisoning: `cache_key = env + ":" + legacy_path`.
- [ ] `setEnvironment()` clears the cache atomically to prevent stale overlay entries.
- [ ] Decision: `getMetadata()` returns global metadata only; the overlay affects filesystem resolution only, not path mapping metadata.
- [ ] One additional filesystem `exists()` check per cache miss (overlay root probed first); negligible impact when cache hit rate > 95%.

### Relationships

- Roadmap row: #224 (🟡 Medium Priority — Mid-term (v1.9.0 – v2.0.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/config/FUTURE_ENHANCEMENTS.md#multi-environment-config-overlay-devstatingprod
- Source key: roadmap:224:config:v1.9.0:multi-environment-config-overlay-devstatingprod

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:224:config:v1.9.0:multi-environment-config-overlay-devstatingprod -->
<!-- roadmap-ref: row=224;module=config;target=v1.9.0 -->
<!-- roadmap-detail: src/config/FUTURE_ENHANCEMENTS.md#multi-environment-config-overlay-devstatingprod -->
