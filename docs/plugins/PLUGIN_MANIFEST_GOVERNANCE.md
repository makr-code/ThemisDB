# ThemisDB Plugin Manifest Governance

**Version:** 1.1  
**Last Updated:** 2026-08-10  
**Status:** Wave-1 Manifest Schema Stable — ABI frozen at `v0.1.0` (`plugin-abi-v2`)

---

## Overview

Plugin manifests (`plugin.json`) define metadata, visibility rules, edition gating, and compatibility constraints for ThemisDB plugins. This document specifies the canonical manifest schema and governance rules.

---

## Manifest Schema

### Core Fields

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `plugin_id` | string | Yes | Unique plugin identifier (snake_case, e.g., `themisdb_ethic_ai`) |
| `name` | string | Yes | Human-readable plugin name |
| `version` | string | Yes | Semantic version (e.g., `1.0.0`, `1.0.0-beta`) |
| `description` | string | Yes | Plugin purpose and capabilities |
| `type` | string | Yes | Plugin category: `compliance`, `storage`, `importer`, `llm_tool`, `accelerator`, etc. |
| `visibility` | string | Yes | Plugin distribution scope: `public`, `community`, `enterprise`, `private` |
| `allowed_editions` | array[string] | Yes | Permitted ThemisDB editions: `minimal`, `community`, `enterprise`, `hyperscaler`, `military` |
| `license_feature` | string | Yes | License gate identifier (e.g., `ethics_ai`, `enterprise_storage`, `llm_wiki`) |
| `min_themisdb_version` | string | Yes | Minimum ThemisDB version (e.g., `2.4.0`) |
| `max_themisdb_version` | string \| null | No | Maximum ThemisDB version; null = no upper bound |
| `compatible_core_abi` | string | Yes | Core ABI version compatibility (e.g., `1.0`) |
| `repository` | string | Yes | Plugin repository URL |
| `author` | string | Yes | Plugin author or team |
| `license` | string | Yes | Plugin license type (e.g., `Proprietary`, `MIT`, `Apache-2.0`) |

### Optional Fields

| Field | Type | Description |
|-------|------|-------------|
| `dependencies` | array[object] | External dependencies (name, version) |
| `sub_plugins` | array[object] | Sub-plugins for aggregate repositories |
| `exports` | object | Public C/C++ symbol exports (factory, destroy functions) |
| `phases` | object | Multi-phase rollout status (e.g., Phase A, Phase B) |
| `performance_targets` | object | Performance SLOs (throughput, latency, cache hit rate) |
| `metadata` | object | Runtime metadata (stability, network requirements, thread-safety, etc.) |

---

## Visibility & Edition Gating

### Visibility Levels

- **`public`** → Unrestricted; included in all editions
- **`community`** → Community-only; included in `minimal`, `community` editions
- **`enterprise`** → Enterprise-only; included in `enterprise`, `hyperscaler`, `military` editions
- **`private`** → Internal-only; not packaged in releases

### Edition Restrictions

Manifest `allowed_editions` controls which editions can load the plugin:

| Edition | Public | Community | Enterprise | Private |
|---------|--------|-----------|------------|---------|
| `minimal` | ✅ | ✅ | ❌ | ❌ |
| `community` | ✅ | ✅ | ❌ | ❌ |
| `enterprise` | ✅ | ❌ | ✅ | ❌ |
| `hyperscaler` | ✅ | ❌ | ✅ | ❌ |
| `military` | ✅ | ❌ | ✅ | ❌ |

### License Feature Gating

Each plugin declares a `license_feature` (e.g., `ethics_ai`, `enterprise_storage`). The edition's license unlocks specific features:

- **Community Edition:** No private features
- **Enterprise Edition:** All `enterprise_*` features unlocked
- **Hyperscaler Edition:** Enterprise + hyperscale optimization features
- **Military Edition:** Enterprise + military-grade hardening + air-gap features

---

## Wave-1 Private Plugins

### Manifest Locations

| Plugin | Path | Submodule |
|--------|------|-----------|
| Ethics AI | `plugins/themisdb_ethic_ai/plugin.json` | `plugins/themisdb_ethic_ai/` |
| Storage Connectors | `plugins/themisdb_storage/plugin.json` | `plugins/themisdb_storage/` |
| Data Importers | `plugins/themisdb_importer/plugin.json` | `plugins/themisdb_importer/` |
| LLM Wiki | `plugins/themisdb_llm_wiki/plugin.json` | `plugins/themisdb_llm_wiki/` |

### CMake Flag Mapping

| Plugin | CMake Flag | Default (ON with `WITH_PRIVATE_PLUGINS`) |
|--------|-----------|------------------------------------------|
| Ethics AI | `WITH_PRIVATE_ETHICS_AI` | `${WITH_PRIVATE_COMPLIANCE}` |
| Storage Connectors | `WITH_PRIVATE_USER_STORAGE_ENCRYPTED` | `${WITH_PRIVATE_COMPLIANCE}` |
| Data Importers | `WITH_PRIVATE_CONNECTOR_PACK` | `${WITH_PRIVATE_CONNECTORS}` |
| LLM Wiki | `WITH_PRIVATE_LLM_WIKI` | `${WITH_PRIVATE_ENTERPRISE}` |

---

## Manifest Validation

### Build-Time Validation

- Manifest `plugin_id` must match directory name (snake_case)
- `allowed_editions` must be non-empty and valid edition names
- `license_feature` must follow naming convention: `[a-z_]+`
- `compatible_core_abi` must match runtime core ABI
- Version strings must be SemVer-compliant

### Runtime Validation

- Plugin loader checks `allowed_editions` against runtime edition
- Plugin loader checks `license_feature` against active license
- Plugin loader validates ABI compatibility before loading
- Manifest signature verified before plugin initialization (Phase 5+)

---

## Rollout Timeline

- **Phase 1 (Q3 2026):** Manifest schema defined and validated in Wave-1 plugins ✅
- **Phase 2 (Q3 2026):** CMake integration with WITH_PRIVATE_* flags ✅
- **Phase 3 (Q4 2026):** Runtime manifest loading and edition gating implemented
- **Phase 4 (Q4 2026):** Manifest signature and hash verification (production hardening)
- **Phase 5 (Q1 2027):** License compliance audit and SBOM generation

---

## Community Build Requirements

Community and minimal editions must:
- ✅ Build without any Wave-1 private plugin submodules present
- ✅ Gracefully skip missing private plugins (no `exists()` hard-fail)
- ✅ Never include private plugin manifests or binaries in release artefacts
- ✅ Pass all regression and compliance gates with only public plugins
- ✅ CI workflows targeting `community`/`minimal` branches must not reference private
  credentials (enforced by `09-pr-gates_community-pipeline-policy.yml`)

---

## ABI Freeze — `v0.1.0` (`plugin-abi-v2`)

The Wave-1 plugin ABI is frozen as of 2026-08-10. All Wave-1 plugins declare:

```json
"compatible_core_abi": "plugin-abi-v2"
```

Breaking changes require:
1. A deprecation entry in `CHANGELOG.md` at least one release cycle in advance.
2. A runtime `THEMIS_LOG_WARN` message for plugins loaded on the old token.
3. A migration guide `docs/plugins/PLUGIN_ABI_MIGRATION_vX_X_X.md`.
4. A version bump to the next `compatible_core_abi` token in all first-party manifests
   in the same PR as the header change.

Full policy, version history, and backward-compatibility guarantees are in
`docs/plugins/PLUGIN_ABI_POLICY.md`.

---

## Wave-1 Submodule Commit Pins

All Wave-1 private plugin submodules must carry a 40-character commit SHA pin in
`.gitmodules`. This is enforced by the `09-pr-gates_submodule-commit-pins.yml` CI gate,
which blocks merges that update `.gitmodules` without a pin on any Wave-1 submodule.

Status: ⚠️ All seven Wave-1 submodule pins are pending (repos provisioned, pins not yet
set). See `docs/plugins/PLUGIN_ABI_POLICY.md §Wave-1 Commit-Pin Status` for the full
tracking table and the procedure to set each pin.

---

## References

- `.github/copilot-instructions.md` — Copilot delegation rules for private plugins
- `BRANCHING_STRATEGY.md` — Release lane governance (develop, community, military)
- `RELEASE_STRATEGY.md` — GA and release-candidate sign-off process
- `cmake/features/PrivatePluginFeatures.cmake` — CMake feature flags
- `cmake/PrivatePlugins.cmake` — Private plugin discovery and registration
- `docs/plugins/PLUGIN_ABI_POLICY.md` — ABI freeze v0.1.0 and backward-compat policy
- `.github/workflows/09-pr-gates_submodule-commit-pins.yml` — Commit-pin CI gate
- `.github/workflows/09-pr-gates_community-pipeline-policy.yml` — No-private-credentials CI gate
