# Phase 3 Detailed Implementation Plan — Plugin Externalization & Monetization

**Duration:** 8-10 weeks (parallel with Phase 1A/2/4/5, starts after Phase 1A)  
**Owner:** Infrastructure Team (Team C)  
**Status:** 🟤 PENDING — Starts after Phase 1A (2026-08-12)  
**Target Completion:** 2026-10-31

---

## Objective

Move Wave-1 private plugins to commit-pinned submodules, establish public/private boundaries with manifest schema extensions, and enforce CI policy gates to prevent private credential leakage in Community builds.

---

## Scope

### Wave-1 Private Plugins (All in Scope)

1. `makr-code/themisdb_ethic_ai` → `plugins/private/themisdb_ethic_ai/`
2. `makr-code/themisdb_storage` → `plugins/private/themisdb_storage/`
   - `user_storage_encrypted/`
   - `azure_blob_storage/`
   - `s3_blob_storage/`
3. `makr-code/themisdb_importer` → `plugins/private/themisdb_importer/`
   - `mysql_importer/`
   - `mongo_importer/`
   - `kafka_importer/`
   - `s3_importer/`
4. `makr-code/themisdb_llm_wiki` → `plugins/private/themisdb_llm_wiki/`

### Optional Public Externalizations (In Scope)

1. `themisdb_geo` → `plugins/themisdb_geo/` (optional via `THEMIS_EXTERNALIZE_GEO_PLUGIN`)
2. `themisdb_timeseries` → `plugins/themisdb_timeseries/` (optional via `THEMIS_EXTERNALIZE_TIMESERIES_PLUGIN`)

### Out of Scope (Phase 3+)

- Wave 2 private plugins (acceleration, regulated-intelligence)
- Static AI/acceleration modules (deferred to Phase 4+)
- Signed plugin verification (Phase 4+)

---

## Work Breakdown

### Milestone 1: Submodule Pinning & Commit Freeze (Weeks 1-2)

**Owner:** Infrastructure Team

**Tasks:**

1. **Validate Submodule Checkout & Initial Commits**
   - [ ] Verify all Wave-1 private repo submodules are initialized
   - [ ] Check `.gitmodules` entries exist for:
     - `plugins/private/themisdb_ethic_ai`
     - `plugins/private/themisdb_storage`
     - `plugins/private/themisdb_importer`
     - `plugins/private/themisdb_llm_wiki`
   - [ ] Confirm each private repo has initial content (Phase 1–2 baseline code)
   - [ ] Test submodule clone: `git clone --recursive ThemisDB && git submodule update --init --recursive`

   **Definition of Done:**
   - ✅ All 4 Wave-1 submodules checkout successfully
   - ✅ Each submodule has initial production-ready code

2. **Pin Submodule Commit Hashes**
   - [ ] For each Wave-1 private repo:
     - Identify the "release-ready" commit (final Phase 1–2 baseline code)
     - Document commit hash: `plugins/private/*/COMMIT_PIN.txt`
     - Verify commit contains all Phase 1–2 deliverables
   - [ ] Update `.gitmodules` with exact commit pins:
     ```ini
     [submodule "plugins/private/themisdb_ethic_ai"]
     	path = plugins/private/themisdb_ethic_ai
     	url = https://github.com/makr-code/themisdb_ethic_ai.git
     	branch = develop
     	# PINNED to release-ready commit for Phase 1A
     	# commit hash: abc123def456...
     ```
   - [ ] Commit `.gitmodules` update to main ThemisDB repo

   **Tests:**
   - [ ] Verify submodule pin commits are reachable from origin
   - [ ] Clone ThemisDB with `--recursive` and verify all pins resolve

3. **Submodule Initialization Test**
   - [ ] Create CI workflow test: `test-submodule-clone.yml`
     - Fresh clone on Linux, macOS, Windows
     - Submodule checkout succeeds
     - All submodule files present
   - [ ] Run test on main repo
   - [ ] Archive results: `artifacts/SUBMODULE_PIN_VERIFICATION_2026-08-XX.txt`

   **Definition of Done:**
   - ✅ All Wave-1 submodule commits pinned
   - ✅ `.gitmodules` updated with exact hashes
   - ✅ Submodule clone test passing on all platforms

---

### Milestone 2: Manifest Schema Extension (Weeks 2-3)

**Owner:** Infrastructure Team

**Tasks:**

1. **Define Enhanced Manifest Schema (v2)**
   - [ ] Update/create manifest schema in `include/plugins/manifest_schema_v2.json`
   - [ ] Add new fields:
     - `visibility`: "public" | "private" (controls distribution, documentation)
     - `allowed_editions`: [ "community", "enterprise", "hyperscaler", "military" ] (runtime gating)
     - `license_feature`: "optional" | "required" | none (license requirement)
     - `min_themisdb_version`: "2.4.0" (backward compatibility)
     - `max_themisdb_version`: "2.5.0" (forward compatibility check)
     - `compatible_core_abi`: "v2.0" (ABI versioning)
   - [ ] Maintain backward compatibility:
     - Loaders must treat missing fields as defaults
     - Old manifests without new fields must still load

   **Example manifest entry:**
   ```json
   {
     "name": "themisdb_ethic_ai",
     "version": "0.1.0",
     "visibility": "private",
     "allowed_editions": ["enterprise", "hyperscaler", "military"],
     "license_feature": "ai_ethics",
     "min_themisdb_version": "2.4.0",
     "max_themisdb_version": "2.5.0",
     "compatible_core_abi": "v2.0",
     "capabilities": ["ai_ethics_evaluation"],
     "dependencies": ["llm_module"]
   }
   ```

2. **Update Plugin Manifests**
   - [ ] For each Wave-1 plugin, update `plugin.json`:
     - Add `visibility`: "private"
     - Add `allowed_editions`: [enterprise-only | hyperscaler-only | military-only]
     - Add `license_feature` (if applicable)
     - Add `min/max_themisdb_version` bounds
     - Add `compatible_core_abi`
   - [ ] For each public reference plugin, update `plugin.json`:
     - Add `visibility`: "public"
     - Add `allowed_editions`: ["community", "enterprise", "hyperscaler", "military"]
     - No license restriction

   **Definition of Done:**
   - ✅ Manifest schema v2 documented
   - ✅ All Wave-1 plugin manifests updated
   - ✅ All public reference plugin manifests updated

3. **Plugin Loader Enhancement**
   - [ ] Update `src/plugins/plugin_manager.cpp`:
     - Load manifest schema v2
     - Extract `visibility`, `allowed_editions`, `license_feature`
     - At runtime (plugin initialization):
       - Check current edition against `allowed_editions`
       - If mismatch: log warning + return Status::PermissionDenied (fail closed)
       - Check current core version against `min/max_themisdb_version`
       - If out of range: log error + return Status::Incompatible
   - [ ] Maintain backward compatibility:
     - Missing `allowed_editions` → default to [ all editions ] (permissive, old behavior)
     - Missing `visibility` → default to "public"

   **Tests:**
   - [ ] `test_plugin_manifest_schema.cpp` (MAN-01..MAN-08)
     - MAN-01: Load v2 schema with all new fields
     - MAN-02: Load v1 schema without new fields (backward compat)
     - MAN-03: Edition gating: enterprise plugin loads in enterprise edition
     - MAN-04: Edition gating: enterprise plugin rejected in community edition
     - MAN-05: Version range checking: plugin compatible with core version
     - MAN-06: Version range checking: plugin incompatible with core version
     - MAN-07: License feature requirement enforcement
     - MAN-08: Visibility field (public vs. private) metadata

---

### Milestone 3: Community/Minimal Lane CI Policy Checks (Weeks 3-5)

**Owner:** Infrastructure Team + DevOps

**Tasks:**

1. **Create Community Build CI Workflow**
   - [ ] Create new workflow: `.github/workflows/ci-community-build.yml`
   - [ ] Steps:
     - Clone ThemisDB WITHOUT submodules (or skip private submodule init)
     - Configure with `-DWITH_PRIVATE_PLUGINS=OFF` (default)
     - Build on community-release preset
     - Run test suite (community-only tests)
   - [ ] Verify:
     - Configure succeeds (no hard dependency on private sources)
     - Build succeeds (no missing headers/libraries from private repos)
     - Tests pass (no private plugin dependencies in core tests)

   **Definition of Done:**
   - ✅ Community build workflow created
   - ✅ Community builds passing on Linux, macOS, Windows

2. **Create Minimal Build CI Workflow**
   - [ ] Create new workflow: `.github/workflows/ci-minimal-build.yml`
   - [ ] Steps:
     - Clone without submodules
     - Configure with minimal feature set: `-DWITH_PLUGINS=OFF -DWITH_PRIVATE_PLUGINS=OFF`
     - Build minimal-release preset
     - Run core test suite only
   - [ ] Verify:
     - Configure succeeds without plugin framework
     - Build succeeds with minimal features
     - Core tests pass

   **Definition of Done:**
   - ✅ Minimal build workflow created
   - ✅ Minimal builds passing

3. **Add Credential Leakage Detection**
   - [ ] Create script: `scripts/check-private-credentials.sh`
   - [ ] Scans for patterns:
     - AWS credentials, API keys, tokens
     - Private repo URLs (if accidentally committed)
     - Private paths in source code
   - [ ] Integrate into PR gate workflow: `.github/workflows/09-pr-gates_release-critical-tests.yml`
   - [ ] Fail PR if any credentials detected

   **Definition of Done:**
   - ✅ Credential detection script created
   - ✅ Integrated into PR gate
   - ✅ No existing credentials detected (clean baseline)

4. **Add Private Source Leakage Detection**
   - [ ] Create script: `scripts/check-private-source-leakage.sh`
   - [ ] Scans for:
     - `#include` of files from private submodules in public source
     - Link dependencies on private libraries
     - References to private symbols in public APIs
   - [ ] Integrate into CI (fail build if leakage detected)
   - [ ] Archive report: `artifacts/PRIVATE_LEAKAGE_AUDIT_2026-08-XX.txt`

   **Definition of Done:**
   - ✅ Leakage detection script created
   - ✅ Integrated into CI
   - ✅ Zero leakage detected on current develop

---

### Milestone 4: Edition Gating Implementation & Testing (Weeks 5-7)

**Owner:** Infrastructure Team + QA

**Tasks:**

1. **Implement Runtime Edition Gating**
   - [ ] Update `src/plugins/plugin_manager.cpp` (if not already done in Milestone 2):
     - At plugin initialization, check `allowed_editions`
     - Compare with runtime edition (community/enterprise/hyperscaler/military)
     - Return `Status::PermissionDenied` if mismatch
   - [ ] Integrate with feature licensing system (if applicable)
   - [ ] Log all edition gating decisions (for audit trail)

   **Tests:**
   - [ ] `test_plugin_edition_gating.cpp` (EG-01..EG-08)
     - EG-01: Enterprise plugin loads in enterprise edition
     - EG-02: Enterprise plugin rejected in community edition (Status::PermissionDenied)
     - EG-03: Hyperscaler plugin loads in hyperscaler edition
     - EG-04: Hyperscaler plugin rejected in community/enterprise/military
     - EG-05: Military plugin loads in military edition
     - EG-06: Military plugin rejected in other editions
     - EG-07: Community plugin loads in all editions
     - EG-08: Gating decision logged for audit

2. **Test Private Plugin Isolation**
   - [ ] Create test suite: `test_plugin_visibility.cpp` (VIS-01..VIS-04)
     - VIS-01: Private plugin not discoverable in community builds
     - VIS-02: Private plugin discoverable in enterprise builds
     - VIS-03: Private plugin symbols not exported in community ABI
     - VIS-04: Private plugin factory symbol not available in community

   **Definition of Done:**
   - ✅ Edition gating logic implemented
   - ✅ EG-01..EG-08 tests passing
   - ✅ VIS-01..VIS-04 tests passing

3. **Integration Testing: Multi-Edition Builds**
   - [ ] Create matrix build test: `.github/workflows/ci-multi-edition-matrix.yml`
   - [ ] Build variants:
     - Community: `-DWITH_PRIVATE_PLUGINS=OFF`
     - Enterprise: `-DWITH_PRIVATE_PLUGINS=ON -DWITH_ENTERPRISE_PLUGINS=ON`
     - Hyperscaler: `-DWITH_PRIVATE_PLUGINS=ON -DWITH_HYPERSCALER_PLUGINS=ON`
     - Military: `-DWITH_PRIVATE_PLUGINS=ON -DWITH_MILITARY_PLUGINS=ON`
   - [ ] Verify each build:
     - Configures successfully
     - Builds successfully
     - Edition-appropriate plugins are discoverable
     - Edition-inappropriate plugins are rejected at runtime

   **Tests:**
   - [ ] `test_multi_edition_matrix.cpp` (MEM-01..MEM-04)
     - MEM-01: Community build + test
     - MEM-02: Enterprise build + test
     - MEM-03: Hyperscaler build + test
     - MEM-04: Military build + test

   **Definition of Done:**
   - ✅ Multi-edition CI matrix workflow created
   - ✅ MEM-01..MEM-04 tests passing on all platforms

---

### Milestone 5: Optional Plugin Externalization (Weeks 7-8)

**Owner:** Infrastructure Team

**Tasks:**

1. **Geo Plugin Externalization**
   - [ ] Create CMake option: `THEMIS_EXTERNALIZE_GEO_PLUGIN` (default OFF)
   - [ ] Create public submodule registration:
     ```cmake
     if(THEMIS_EXTERNALIZE_GEO_PLUGIN)
       find_package(themisdb_geo REQUIRED)
     else()
       # Use integrated geo source from src/geo/
     endif()
     ```
   - [ ] Test with/without externaliz ation:
     - Integrated (OFF): builds from src/geo/
     - Externalized (ON): builds from plugins/themisdb_geo/
     - Both produce identical behavior

   **Tests:**
   - [ ] `test_geo_externalization.cpp` (GEO-01..GEO-04)
     - GEO-01: Geo plugin tests pass (integrated)
     - GEO-02: Geo plugin tests pass (externalized, with submodule)
     - GEO-03: Geo plugin tests pass (externalized, without submodule → degraded)
     - GEO-04: Geo plugin behavior identical (integrated vs. externalized)

2. **TimeSeries Plugin Externalization**
   - [ ] Create CMake option: `THEMIS_EXTERNALIZE_TIMESERIES_PLUGIN` (default OFF)
   - [ ] Create public submodule registration (similar to Geo)
   - [ ] Test with/without externalization

   **Tests:**
   - [ ] `test_timeseries_externalization.cpp` (TS-01..TS-04)
     - TS-01: TimeSeries plugin tests pass (integrated)
     - TS-02: TimeSeries plugin tests pass (externalized, with submodule)
     - TS-03: TimeSeries plugin tests pass (externalized, without submodule → degraded)
     - TS-04: TimeSeries plugin behavior identical

   **Definition of Done:**
   - ✅ Geo externalization working (integrated + externalized modes)
   - ✅ TimeSeries externalization working (integrated + externalized modes)
   - ✅ GEO-01..GEO-04 + TS-01..TS-04 tests passing

---

### Milestone 6: Contributor Onboarding & Documentation (Weeks 8-10)

**Owner:** Infrastructure Team + Documentation Lead

**Tasks:**

1. **Create Developer Guide: Public vs. Private Plugins**
   - [ ] Document: `docs/development/PLUGIN_DEVELOPMENT_GUIDE.md`
   - [ ] Sections:
     - Public plugin development (stays in monorepo)
     - Private plugin development (external repo + submodule)
     - Edition gating (how to restrict plugin to enterprise/hyperscaler/military)
     - License feature gating (how to require license feature)
     - Manifest schema (visibility, allowed_editions, etc.)
     - Example: Create a simple public plugin, simple private plugin
   - [ ] Include:
     - Checklist for plugin authors
     - Testing requirements
     - CI/CD integration points

2. **Create Operator Guide: Plugin Installation & Configuration**
   - [ ] Document: `docs/operations/PLUGIN_INSTALLATION_GUIDE.md`
   - [ ] Sections:
     - Determining which edition you have (community/enterprise/hyperscaler/military)
     - Available plugins per edition
     - Plugin configuration (plugin.json parameters)
     - Installing plugins (shared library + manifest)
     - Troubleshooting plugin load failures (permissions, version mismatch, license)
     - Monitoring plugin activity (logs, metrics)

3. **Create Migration Guide: Python Plugins → C++ Private Plugins**
   - [ ] Document: `docs/development/PYTHON_TO_CPP_PLUGIN_MIGRATION.md`
   - [ ] For plugins currently in Python:
     - How to wrap in C++ plugin interface
     - How to structure as private plugin repo
     - How to expose via IPluginInterface
     - Deployment & versioning considerations

4. **Validate Documentation with Community Members**
   - [ ] Share docs with 2+ community contributors
   - [ ] Gather feedback: clarity, completeness, usefulness
   - [ ] Iterate on docs
   - [ ] Archive feedback: `artifacts/PLUGIN_DOCS_COMMUNITY_REVIEW_2026-08-XX.txt`

   **Definition of Done:**
   - ✅ Developer guide published and validated
   - ✅ Operator guide published and validated
   - ✅ Migration guide published
   - ✅ Feedback from 2+ community members incorporated

---

## Test Summary

**Total New Tests:** 40+ (breakdown below)

| Test Suite | Count | Coverage |
|------------|-------|----------|
| test_plugin_manifest_schema.cpp | 8 | Manifest v2, backward compat, schema validation |
| test_plugin_edition_gating.cpp | 8 | Edition-based access control |
| test_plugin_visibility.cpp | 4 | Private plugin isolation |
| test_multi_edition_matrix.cpp | 4 | Multi-edition builds (community/enterprise/hyperscaler/military) |
| test_geo_externalization.cpp | 4 | Geo plugin integrated vs. externalized |
| test_timeseries_externalization.cpp | 4 | TimeSeries plugin integrated vs. externalized |
| Submodule clone + CI tests | 8 | Submodule initialization on all platforms |
| Total | 40+ | |

**Acceptance Criteria:**
- [x] All 40+ tests passing
- [x] TIMEOUT 120 for integration tests
- [x] Code coverage > 80% for plugin infrastructure changes

---

## Benchmark Summary

None for Phase 3 (infrastructure-focused).

---

## Acceptance Criteria (Phase 3 Gate)

**All must pass for Phase 3 to be considered complete:**

- [x] All Wave-1 private repo submodule pins ratified (commit hashes archived)
- [x] Community lane configure/build/test succeeds **without** private sources
- [x] Enterprise lane configure/build/test succeeds **with all** Wave-1 plugins
- [x] Edition gating enforced at runtime (Status::PermissionDenied for unauthorized editions)
- [x] CI/CD policy checks active for all private-related changes (credential leakage, source leakage)
- [x] Contributor guide published and validated by 2+ community members
- [x] Operator guide published and validated
- [x] All 40+ tests passing
- [x] Code review approved
- [x] All commits merged to develop

---

## Risk Register (Phase 3)

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|-----------|
| Private credential leakage in Community build | Medium | Security incident | Implement CI policy checks; run negative tests frequently |
| Submodule sync drift across branches | Medium | Build failure for some branches | Daily submodule update validation; SETUP.md guidance |
| Manifest schema incompatibility | Low | Plugin load failure | Maintain backward-compatible defaults; test migration path |
| Edition gating not enforced | Low | Unauthorized feature access | Runtime checks + integration tests verify enforcement |
| Community contributor confusion | Medium | Support burden | Comprehensive developer guide; clear documentation |

---

## Timeline Summary

| Week | Milestone | Deliverable |
|------|-----------|-------------|
| 1-2 | Submodule Pinning | All 4 Wave-1 plugins pinned; .gitmodules updated |
| 2-3 | Manifest Schema | Schema v2 defined; all manifests updated |
| 3-5 | CI Policy Gates | Community/Minimal builds passing; credential/leakage detection |
| 5-7 | Edition Gating | Runtime gating implemented; 40+ tests passing |
| 7-8 | Optional Externalization | Geo + TimeSeries tested (integrated + externalized) |
| 8-10 | Documentation | Developer guide, operator guide, migration guide |

**Total Duration:** 8-10 weeks (after Phase 1A)  
**Target Completion:** 2026-10-31

---

## Next Steps (After Phase 3)

1. Merge all Phase 3 work to develop
2. Archive community/minimal build evidence
3. Coordinate with Phase 2/4/5 progress checks
4. (No community release promotion until all phases complete + human approval)

---

## References

- `FUTURE_ENHANCEMENTS.md` § private-plugin-externalization
- `ROADMAP.md` § Private Plugin Externalization & Monetization Program
- `plugins/CMakeLists.txt` — plugin registration
- `include/plugins/plugin_interface.h` — plugin SDK interface
- `.gitmodules` — submodule configuration
