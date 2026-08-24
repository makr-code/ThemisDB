# Wave 1 Private Plugin Submodule Status & Integration Plan

**Date:** 2026-08-10  
**Status:** PRE-GA (awaiting promotion sign-off; submodule infrastructure ready)  
**Scope:** Wave-1 private plugin externalization via commit-pinned submodules  
**Authority:** ROADMAP.md §Private Plugin Externalization & Monetization Program; FUTURE_ENHANCEMENTS.md §private-plugin-externalization

---

## Executive Summary

Wave-1 private plugin repositories and `.gitmodules` entries are **provisioned and registered** (2026-07). Commit pins are **pending** post-GA content push. Infrastructure for fail-closed Community/Minimal builds is in place.

**4 Wave-1 Repositories:**
1. ✅ `makr-code/themisdb_ethic_ai` → `plugins/themisdb_ethic_ai/` (ethics_ai plugin root)
2. ✅ `makr-code/themisdb_storage` → `plugins/themisdb_storage/` (aggregate: user_storage_encrypted, azure_blob_storage, s3_blob_storage)
3. ✅ `makr-code/themisdb_importer` → `plugins/themisdb_importer/` (aggregate: mysql_importer, mongo_importer, kafka_importer, s3_importer)
4. ✅ `makr-code/themisdb_llm_wiki` → `plugins/themisdb_llm_wiki/` (LLM Wiki enterprise plugin; Phase A delivered, Phase B pending)

**Public Optional Submodules (Wave 1+):**
5. ✅ `makr-code/themisdb_geo` → `plugins/themisdb_geo/` (public optional; Phase 5-6 complete)
6. ✅ `makr-code/themisdb_timeseries` → `plugins/themisdb_timeseries/` (public optional; integrated fallback ready)
7. ✅ `makr-code/themisdb_plugin_signer` → `plugins/themisdb_plugin_signer/` (plugin validation/signing utilities)

---

## Current State (.gitmodules)

### Wave-1 Private Plugins (Entries Confirmed 2026-08-10)

```
[submodule "plugins/themisdb_ethic_ai"]
    path = plugins/themisdb_ethic_ai
    url = https://github.com/makr-code/themisdb_ethic_ai.git
    branch = develop

[submodule "plugins/themisdb_llm_wiki"]
    path = plugins/themisdb_llm_wiki
    url = https://github.com/makr-code/themisdb_llm_wiki.git
    branch = develop

[submodule "plugins/themisdb_storage"]
    path = plugins/themisdb_storage
    url = https://github.com/makr-code/themisdb_storage.git
    branch = develop

[submodule "plugins/themisdb_importer"]
    path = plugins/themisdb_importer
    url = https://github.com/makr-code/themisdb_importer.git
    branch = develop
```

### Public Optional Submodules (Entries Confirmed 2026-08-10)

```
[submodule "plugins/themisdb_plugin_signer"]
    path = plugins/themisdb_plugin_signer
    url = https://github.com/makr-code/themisdb_plugin_signer.git
    branch = develop

[submodule "plugins/themisdb_geo"]
    path = plugins/themisdb_geo
    url = https://github.com/makr-code/themisdb_geo.git
    branch = develop

[submodule "plugins/themisdb_timeseries"]
    path = plugins/themisdb_timeseries
    url = https://github.com/makr-code/themisdb_timeseries.git
    branch = develop
```

**Status:** ✅ All 7 submodule entries present in `.gitmodules` on 2026-08-10

---

## Content Push & Commit-Pin Timeline

### Pre-GA Checklist (Before GA tag v2.4.0)
- [x] `.gitmodules` fully configured with all Wave-1 repo URLs and branches
- [x] CMake integration verified for graceful fail-closed missing-submodule paths
- [x] Private plugin load hooks in `plugins/CMakeLists.txt` check for submodule existence
- [x] Public plugin reference implementations retained in monorepo for onboarding
- [x] Community-release CMake preset disables private plugin discovery (`THEMIS_ALLOW_MISSING_ROCKSDB=ON` also in effect)

### Post-GA Content Push Phase (Target: Week of 2026-08-19, after D-11 sign-off)
1. **Prepare initial content in Wave-1 repositories:**
   - Push `develop`-branch content snapshot to each Wave-1 repo (themisdb_ethic_ai, themisdb_storage, themisdb_importer, themisdb_llm_wiki)
   - Create stable commit hash for each repo on `develop` branch
   - Document repository README, LICENSE, and initial manifest metadata
   - Update each plugin.json manifest with visibility, allowed_editions, license_feature tags

2. **Update submodule pins in main repo:**
   ```bash
   cd /path/to/ThemisDB
   git submodule update --remote
   # (or manually pin each to specific commit hash)
   git add .gitmodules plugins/themisdb_*
   git commit -m "Pin Wave-1 private plugins to initial content release"
   git push origin develop
   ```

3. **Verify Community/Minimal fail-closed paths:**
   - Clone repo without private credentials → configure should proceed with warnings
   - Build `community-release` preset → missing private submodules skipped gracefully
   - Build `minimal` preset → no private modules attempted
   - Run `release_critical` CI → all tests pass without private module presence

4. **Create Wave-1 release branches (post-GA):**
   - Ensure all Wave-1 repos have `community`, `enterprise`, `hyperscaler`, `military` branches aligned with root governance
   - Document branch governance in each plugin's README

---

## CMake Integration Status

### Private Plugin Detection (plugins/CMakeLists.txt)

**Current behavior (2026-08-10):**
```cmake
# Graceful fail-closed for missing private plugin submodules
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/themisdb_ethic_ai/CMakeLists.txt")
    add_subdirectory(themisdb_ethic_ai)
else()
    message(WARNING "Private plugin themisdb_ethic_ai not found (submodule may not be initialized)")
endif()
```

✅ **Status:** All private plugins already guarded with `EXISTS()` checks; Community/Minimal builds do not fail hard.

### Public Optional Plugin Detection (plugins/CMakeLists.txt)

**Current behavior (2026-08-10):**
```cmake
# Public optional plugins default to ON but can be disabled
if(THEMIS_EXTERNALIZE_GEO_PLUGIN AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/themisdb_geo/CMakeLists.txt")
    add_subdirectory(themisdb_geo)
elseif(NOT THEMIS_EXTERNALIZE_GEO_PLUGIN)
    message(STATUS "Geo plugin externalization disabled; using integrated monorepo path")
endif()
```

✅ **Status:** CMake flags and fallback mechanisms in place; integrated monorepo paths are the default.

---

## CI/CD Policy Checklist (Q4 2026 Phase 2-3)

Per ROADMAP.md §Private Plugin Externalization, the following CI gates must be enforced:

- [ ] **Community lane CI:** Never attempts to clone, build, or test private plugins
  - Verification: Community workflow does not define `GIT_SUBMODULE_*` private credentials
  - Verification: Community preset logs show "Private plugin ... not found (submodule not initialized)" warnings only

- [ ] **Minimal lane CI:** Explicitly skips all private modules; only core monorepo tested
  - Verification: Minimal preset output confirms "... module disabled" for all private plugins

- [ ] **Enterprise/Hyperscaler/Military lanes:** Require private plugin credentials
  - Verification: Release workflows in edition-specific branches require encrypted secrets

- [ ] **SBOM & leakage rules:** No private repository references in public artifact manifests
  - Verification: Release packages (for community edition) contain no private-repo metadata

---

## Known Issues & Mitigations

| Issue | Status | Mitigation | Target |
|-------|--------|-----------|--------|
| **Private plugin submodule paths** | ✅ RESOLVED | Added optional submodule structure with `EXISTS()` guards in CMake | 2026-08-10 |
| **Commit pins undefined** | 🟡 IN PROGRESS | Content push to Wave-1 repos will establish first stable pins (post-GA) | 2026-08-19 |
| **CI policy checks not yet automated** | 🔴 PENDING | Create `ci-private-plugin-policy.yml` GitHub workflow (Q4 2026 Phase 2) | 2026-09-15 |
| **Edition-specific branch governance** | 🟡 IN PROGRESS | Document branch-mapping rules in each Wave-1 repo README (post-GA) | 2026-08-26 |

---

## References

- **ROADMAP.md:** § Private Plugin Externalization & Monetization Program (Phase 1-4)
- **FUTURE_ENHANCEMENTS.md:** § private-plugin-externalization (scope, design, tests)
- **.gitmodules:** Lines 36-74 (Wave-1 + public submodule entries)
- **plugins/CMakeLists.txt:** Private plugin loading logic with fail-closed gates
- **cmake/PrivatePlugins.cmake:** Private plugin feature macro definitions
- **BRANCHING_STRATEGY.md:** Edition-lane branch naming and propagation rules

---

## Next Actions

### Immediate (This session, pre-GA)
1. Confirm all 7 .gitmodules entries are present and correctly formatted ✅ DONE
2. Verify plugins/CMakeLists.txt has fail-closed guards for all private plugins ✅ VERIFIED
3. Update this status document to root governance (ai_working/) ✅ THIS DOCUMENT

### Post-GA (Week of 2026-08-19)
1. Push initial content snapshots to Wave-1 repositories
2. Update .gitmodules commit pins after content stabilization
3. Create automated CI policy checks for Community/Minimal lane compliance
4. Update each Wave-1 repo README with branch governance and visibility metadata

### Q4 2026 (Phases 2-3)
1. Finalize edition-specific branch structure across all Wave-1 repos
2. Implement SBOM leakage detection in release workflows
3. Document onboarding procedures for private plugin consumption

---

**Status:** Ready for GA. Wave-1 private plugin infrastructure complete; content push pending post-promotion.  
**Owner:** Engineering Lead / Plugin & Monetization Program  
**Last Updated:** 2026-08-10  
