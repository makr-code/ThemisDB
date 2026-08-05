# Community Build Validation Guide

**Version:** 1.0  
**Last Updated:** 2026-08-05  
**Scope:** Wave-1 Private Plugin Integration  
**Objective:** Ensure ThemisDB community/minimal builds work seamlessly without private plugin submodules

---

## Community Build Requirements

### Functional Requirements

1. **No Private Content in Community Builds**
   - ✅ Community builds must NOT include any private plugin source code
   - ✅ No private plugin binaries in release artefacts
   - ✅ No private plugin manifests in documentation or packaging
   - ✅ No credentials, keys, or secrets for private repositories

2. **Graceful Degradation**
   - ✅ Missing private plugin submodules do NOT cause CMake configure failures
   - ✅ Missing private plugin submodules do NOT cause build failures
   - ✅ Missing private plugin submodules are logged at INFO level, not WARN/ERROR
   - ✅ All public plugins and core functionality fully functional without private plugins

3. **Build Reproducibility**
   - ✅ Community builds are byte-identical whether private submodules are present or not
   - ✅ No conditional compilation based on private plugin presence
   - ✅ All tests pass regardless of private plugin state

4. **Edition Gating**
   - ✅ Edition detection at build time correctly prevents private plugin loading on community/minimal editions
   - ✅ Plugin loader checks license_feature against active edition
   - ✅ Private plugins fail-closed if loaded on unsupported editions

---

## Validation Test Matrix

### Build Scenarios

| Scenario | Private Plugins | CMake Result | Build Result | Edition Gate |
|----------|-----------------|--------------|--------------|--------------|
| **Community Full** | Absent | ✅ PASS | ✅ PASS | ✅ Pass (no private features) |
| **Community Partial** | Only ethics_ai | ⚠️ INFO skip | ✅ PASS | ✅ Pass (plugin not loaded) |
| **Enterprise Full** | All present | ✅ PASS | ✅ PASS | ✅ Pass (plugins loaded) |
| **Enterprise Partial** | Missing importer | ⚠️ INFO skip | ✅ PASS | ✅ Pass (partial feature set) |

### Test Checklist

**1. Community Build (No Private Submodules)**

```bash
# Configure: should detect missing submodules gracefully
cmake --preset community-release \
  -DWITH_PRIVATE_PLUGINS=OFF

# Expected output:
#   - themisdb_ethic_ai not available (optional private source missing)
#   - themisdb_storage not available (optional private source missing)
#   - themisdb_importer not available (optional private source missing)
#   - themisdb_llm_wiki not available (optional private source missing)
```

- [ ] Configure passes without error
- [ ] Build completes without error
- [ ] No private plugin symbols in binary
- [ ] Release artefacts contain only public plugins
- [ ] Regression test suite passes (PASS/FAIL parity with private-present build)

**2. Enterprise Build (All Private Submodules Present)**

```bash
# Configure: should discover all private submodules
cmake --preset community-release \
  -DWITH_PRIVATE_PLUGINS=ON

# Expected output:
#   - ethics_ai (private plugin)
#   - themisdb_storage (private plugin aggregate)
#   - themisdb_importer (private plugin aggregate)
#   - LLM Wiki (private plugin)
```

- [ ] Configure passes without error
- [ ] Build completes without error
- [ ] All private plugin symbols properly linked
- [ ] Private plugins initialize correctly at runtime
- [ ] Plugin loader checks edition gate (skips on community edition)

**3. Mixed Build (Partial Private Submodules)**

```bash
# Configure: some private plugins present, some absent
# (e.g., themisdb_ethic_ai present, others missing)

cmake --preset community-release \
  -DWITH_PRIVATE_ETHICS_AI=ON \
  -DWITH_PRIVATE_CONNECTOR_PACK=OFF \
  -DWITH_PRIVATE_LLM_WIKI=OFF
```

- [ ] Configure passes without error
- [ ] Build completes without error
- [ ] Present private plugins initialized correctly
- [ ] Absent private plugins skipped gracefully
- [ ] No false positives/negatives on feature availability

**4. Edition Gating Test**

```cpp
// Test: plugin loader respects edition restrictions
SCENARIO("Community edition rejects private plugins") {
  Edition ed = Edition::COMMUNITY;
  
  // Load private plugin manifest
  PluginManifest manifest = LoadManifest("themisdb_ethic_ai");
  
  // Edition check should fail
  EXPECT_FALSE(manifest.allowed_editions.contains(ed));
  EXPECT_EQ("ethics_ai", manifest.license_feature);
}
```

- [ ] Plugin loader correctly identifies edition from build metadata
- [ ] Private plugins rejected on community/minimal editions
- [ ] Enterprise plugins accepted on enterprise/hyperscaler/military editions
- [ ] Plugin initialization fails gracefully with edition-rejection message

---

## CI/CD Validation

### GitHub Actions Workflow

**Community Lane (`.github/workflows/cmake-multi-platform.yml`)**

- ✅ `WITH_PRIVATE_PLUGINS=OFF` by default
- ✅ Submodules checked out with `--filter=blob:none` (excludes private repos)
- ✅ CMake configure step shows no private plugin warnings
- ✅ Build step completes successfully
- ✅ Test step runs full regression suite
- ✅ Release artefact generated contains ONLY public plugins

**Enterprise Lane (`.github/workflows/packaging-release.yml`)**

- ✅ `WITH_PRIVATE_PLUGINS=ON` by default
- ✅ All submodules checked out (requires credentials for private repos)
- ✅ CMake configure step discovers all private plugins
- ✅ Build step completes successfully with all plugins linked
- ✅ Test step runs extended test suite (private plugin tests included)
- ✅ Release artefact generated contains all plugins

---

## Fallback Behavior

### Missing Plugin Submodule Handling

**For each Wave-1 plugin:**

```cmake
if(WITH_PRIVATE_ETHICS_AI)
    # Attempt to add private plugin
    _themis_add_optional_private_plugin_dir("Private plugin: ethics_ai"
        "${plugins_root}/themisdb_ethic_ai"
        "${CMAKE_CURRENT_BINARY_DIR}/private_ethics_ai")
    # If CMakeLists.txt not found → logs INFO message and skips
endif()
```

**Error Behavior:**

- ❌ NOT an error if submodule CMakeLists.txt missing
- ❌ NOT a warning if WITH_PRIVATE_* enabled but submodule absent
- ✅ Informational message at INFO level only
- ✅ Build continues normally without the missing plugin

### Feature Availability

**For features gated by private plugins:**

```cpp
// Example: ethics_ai features
#ifdef WITH_PRIVATE_ETHICS_AI
  // Ethics AI evaluation framework available
  auto evaluator = CreateEthicsEvaluator(...);
#else
  // Fallback: stub evaluator or disabled feature
  THEMIS_LOG_INFO("Ethics AI plugin not available; some compliance features disabled");
#endif
```

---

## Regression Test Coverage

### Gate Tests (All Editions)

| Gate | Community | Enterprise | Notes |
|------|-----------|-----------|-------|
| **Build Reproducibility** | ✅ | ✅ | Byte-identical output |
| **Performance** | ✅ | ✅ | Wave 7/8/9 gates apply to all |
| **Security** | ✅ | ✅ | Sanitizers clean on both |
| **Functional** | ✅ | ✅ | Core functionality in both |

### Private Plugin Tests (Enterprise Only)

| Test Suite | Status | Coverage |
|-----------|--------|----------|
| **LWP-01..08** | Enterprise | Ingest + query (if llm_wiki loaded) |
| **LWP-GATE-01** | Enterprise | Edition-gate negative test |
| **Storage Plugin Tests** | Enterprise | If storage aggregate present |
| **Importer Plugin Tests** | Enterprise | If importer aggregate present |
| **Ethics AI Tests** | Enterprise | If ethics_ai plugin present |

---

## Validation Checklist (Pre-Release)

**Before GA promotion on v2.4.0:**

- [ ] Community build (no private plugins) passes all gates
- [ ] Enterprise build (all private plugins) passes all gates
- [ ] Mixed build (subset of private plugins) passes all gates
- [ ] Release artefact for community edition contains NO private plugin binaries
- [ ] Release artefact for enterprise edition contains all private plugin binaries
- [ ] SBOM for community edition does NOT list private plugins
- [ ] SBOM for enterprise edition DOES list all private plugins
- [ ] Edition gating works correctly at runtime (plugin loader respects manifest)
- [ ] Documentation accurately reflects which features are edition-gated
- [ ] No credentials, keys, or secrets accidentally included in any build artefact

---

## Troubleshooting

### Issue: Missing private plugin causes CMake error

**Root cause:** `EXISTS()` check failed or PATH not set correctly

**Solution:**
1. Verify submodule is initialized: `git submodule update --init --recursive`
2. Verify CMakeLists.txt exists in submodule root
3. Verify `_themis_add_optional_private_plugin_dir()` function is called
4. Check CMake log for actual error vs informational message

### Issue: Private plugin loads on community edition

**Root cause:** Edition gating not implemented at plugin loader

**Solution:**
1. Verify plugin manifest `allowed_editions` is set correctly
2. Verify plugin loader checks manifest against `THEMIS_EDITION_SELECTED`
3. Add test case to LWP-GATE-01 (negative test)

### Issue: Build output differs between private-present and private-absent

**Root cause:** Conditional compilation based on private plugin presence

**Solution:**
1. Remove all `#ifdef WITH_PRIVATE_*` guards from core functionality
2. Use runtime gating (plugin loader checks) instead of compile-time gating
3. Verify feature availability via API, not preprocessor directives

---

## References

- `cmake/features/PrivatePluginFeatures.cmake` — Feature flags
- `cmake/PrivatePlugins.cmake` — Private plugin discovery
- `docs/plugins/PLUGIN_MANIFEST_GOVERNANCE.md` — Manifest schema
- `.github/workflows/cmake-multi-platform.yml` — Community build workflow
- `.github/workflows/packaging-release.yml` — Enterprise/release packaging workflow
