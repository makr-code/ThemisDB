# ThemisDB Plugin ABI Policy

**Version:** 0.1.0 (Wave-1 Freeze)
**Last Updated:** 2026-08-10
**Status:** Frozen — backward-incompatible changes require a version bump and deprecation notice

---

## Overview

This document defines the ABI (Application Binary Interface) contract for ThemisDB
plugins, the freeze status of `v0.1.0`, and the backward-compatibility policy that
governs all future changes to the plugin interface.

---

## ABI Version — `v0.1.0` (Wave-1 Freeze)

### What is frozen

The following constitutes the frozen `v0.1.0` plugin ABI:

| Contract element | Location | Frozen since |
|-----------------|----------|-------------|
| Plugin factory symbol `themisdb_<id>_create` | `include/plugins/plugin_interface.h` | Wave-1 |
| Plugin destroy symbol `themisdb_<id>_destroy` | `include/plugins/plugin_interface.h` | Wave-1 |
| `ThemisPluginInfo` struct layout | `include/plugins/plugin_interface.h` | Wave-1 |
| `ThemisPluginContext` opaque pointer | `include/plugins/plugin_interface.h` | Wave-1 |
| `compatible_core_abi` manifest field identifier `"plugin-abi-v2"` | `plugin.json` in all Wave-1 plugins | Wave-1 |
| Edition gating fields (`allowed_editions`, `license_feature`) | `plugin.json` schema | Wave-1 |

> **Note on manifest ABI token:** The canonical runtime token for the Wave-1 ABI series
> is `"plugin-abi-v2"` (legacy naming from pre-Wave-1 internal development). The semantic
> freeze label used in governance documentation is `v0.1.0`. These are the same contract;
> all new plugins must declare `"compatible_core_abi": "plugin-abi-v2"` until the ABI is
> bumped to `v0.2.0` / `"plugin-abi-v3"`.

---

## Backward-Compatibility Policy

### Guaranteed (always backward-compatible)

- Adding new **optional** fields to `plugin.json` (loader ignores unknown fields)
- Adding new `capabilities` sub-keys (additive, old plugins default to `false`)
- Adding new `metadata` sub-keys (informational, not load-critical)
- Adding new `config_schema` properties with a `default` value
- Expanding the allowed set of `type` values for new plugin categories
- Extending the `allowed_editions` enumeration with new edition names

### Breaking (requires ABI bump to `v0.2.0`)

- Changing the **signature** of the factory or destroy C symbol
- Changing the **layout or alignment** of `ThemisPluginInfo` (add/remove/reorder fields)
- Changing the **type or semantics** of `ThemisPluginContext`
- Removing or renaming a **required** `plugin.json` field
- Changing the runtime **load-or-reject semantics** of `compatible_core_abi`
- Changing the edition gating evaluation logic

### Deprecation procedure (before breaking change)

1. Announce the deprecation in `CHANGELOG.md` under `## Deprecated` at least one release
   cycle before the breaking change.
2. Emit a runtime `THEMIS_LOG_WARN` message for any plugin loaded that uses the old
   ABI token, pointing to the migration guide.
3. Create a migration guide in `docs/plugins/` named
   `PLUGIN_ABI_MIGRATION_vX_X_X.md`.
4. Bump `compatible_core_abi` to the next token in all first-party manifests **in the
   same PR** as the breaking header change.
5. Update this document and `PLUGIN_MANIFEST_GOVERNANCE.md` in the same PR.

---

## ABI Version History

| ABI Token | Governance label | Introduced | Status |
|-----------|-----------------|-----------|--------|
| `plugin-abi-v1` | pre-Wave-1 | Internal dev | ⛔ Retired — not supported |
| `plugin-abi-v2` | `v0.1.0` | Wave-1 (2026-08) | ✅ Active — frozen |
| `plugin-abi-v3` | `v0.2.0` | — | 🔵 Planned (Wave-2+) |

---

## Submodule Commit-Pin Requirement

All Wave-1 private plugin submodules **must** carry a 40-character commit SHA pin in
`.gitmodules` under the matching `[submodule]` block:

```ini
[submodule "plugins/themisdb_ethic_ai"]
    path = plugins/themisdb_ethic_ai
    url  = https://github.com/makr-code/themisdb_ethic_ai.git
    branch = develop
    commit = <40-hex-sha>
```

### Why pins are required

- **Reproducible builds:** every CI run and local checkout resolves to the same source,
  preventing silent regressions from a force-push or tag move on the plugin repository.
- **Supply-chain integrity:** a pinned commit hash is a prerequisite for SBOM generation
  (`sbom-ci.yml`) and signature verification (`themisdb_plugin_signer`).
- **Release gating:** the `09-pr-gates_submodule-commit-pins.yml` CI gate blocks merges
  to `develop`/`community`/`enterprise`/`hyperscaler`/`military` that update `.gitmodules`
  without a commit pin on any Wave-1 private plugin submodule.

### How to set a commit pin

```bash
# 1. Initialise and update the submodule to the target branch HEAD
git submodule update --init --remote plugins/themisdb_ethic_ai

# 2. Capture the resolved SHA
SHA=$(git -C plugins/themisdb_ethic_ai rev-parse HEAD)

# 3. Add the commit field to .gitmodules (manual edit or sed)
#    commit = $SHA

# 4. Stage and commit
git add .gitmodules
git commit -m "chore: pin themisdb_ethic_ai submodule to $SHA"
```

### Wave-1 commit pin status

| Submodule path | Commit pin status |
|----------------|------------------|
| `plugins/themisdb_ethic_ai` | ⚠️ Pending — repo provisioned, pin not yet set |
| `plugins/themisdb_llm_wiki` | ⚠️ Pending — repo provisioned, pin not yet set |
| `plugins/themisdb_storage` | ⚠️ Pending — repo provisioned, pin not yet set |
| `plugins/themisdb_importer` | ⚠️ Pending — repo provisioned, pin not yet set |
| `plugins/themisdb_plugin_signer` | ⚠️ Pending — repo provisioned, pin not yet set |
| `plugins/themisdb_geo` | ⚠️ Pending — repo provisioned, pin not yet set |
| `plugins/themisdb_timeseries` | ⚠️ Pending — repo provisioned, pin not yet set |

> Tracked in: `09-pr-gates_submodule-commit-pins.yml` CI gate (blocks `.gitmodules` PRs
> on `develop`+ that omit a pin).

---

## References

- `docs/plugins/PLUGIN_MANIFEST_GOVERNANCE.md` — full manifest schema and governance rules
- `docs/plugins/COMMUNITY_BUILD_VALIDATION_GUIDE.md` — community build requirements
- `.github/workflows/09-pr-gates_submodule-commit-pins.yml` — commit-pin CI gate
- `.github/workflows/09-pr-gates_community-pipeline-policy.yml` — no-private-credentials CI gate
- `include/plugins/plugin_interface.h` — canonical C ABI header
