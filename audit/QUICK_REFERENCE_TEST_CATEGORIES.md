---
Author: "Copilot Task Agent"
Created: "2026-10-11"
Last Updated: "2026-10-11"
Status: "Quick Reference - Test Categories for Release Gates"
---

# Quick Reference: Test Categories for GitHub Release Gates & Governance

## 3-Minute Summary

Beyond the **6 core CTest categories** (function-level, module-level, cross-module, happy-path, error, edge-case), ThemisDB release gates require **8 additional test categories**:

### By Release Gate Workflow

| Workflow | Test Category | Purpose | Label | Tier A Target |
|----------|---|---|---|---|
| **gate-pr-core.yml** | Release-Critical | Blocks merge to develop | `release_critical` | 30-60 tests |
| **build-*-fast.yml** | Focused Module | Fast feedback (<5min) | `focused` | 30 tests |
| **benchmark-performance-gate.yml** | Performance | Regression detection | `performance` | 12-18 tests |
| **security-consolidated.yml** | Security (SAST/Auth/Crypto) | Vuln prevention | `security/*` | 18-30 tests |
| **gate-pr-security-codeql.yml** | CodeQL Static Analysis | Type/memory safety | `security/static` | (included above) |
| **compliance-governance-gates.yml** | Governance | Maturity/roadmap/edition | `governance/*` | 12-18 tests |
| Scheduled | Compliance (Audit/Data/RBAC) | Privacy/audit/access control | `compliance/*` | 12-18 tests |
| Scheduled | Integration/Soak/Chaos | E2E + stress + Byzantine | `integration/*` | 15-20 tests |
| Optional | Regression (Bug/CVE) | Re-introduction prevention | `regression/*` | 6-12 tests |

### 8 New Test Categories Explained

1. **Release-Critical** (30 marked tests)
   - Core library validation
   - Block PR merge if fail
   - Tier A modules: 5-10 each
   - Example: `test_transaction_atomicity` (release_critical)

2. **Focused Module** (496 targets)
   - Module-specific fast tests
   - Quick feedback on changes
   - Tier C modules: 3-5 each
   - Example: `test_tensor_allocation` (focused)

3. **Performance** (Benchmarks)
   - Throughput, latency, memory
   - Weekly validation
   - Tier A modules: 2-3 each
   - Example: `BM_server_1000_qps_throughput`

4. **Security** (Multiple flavors)
   - SAST: Use-after-free, SQL injection, type safety
   - Auth: Token expiry, permission enforcement
   - Crypto: Algorithm correctness, key management
   - Secrets: No hardcoded credentials
   - Tier A modules: 3-5 each
   - Example: `test_auth_expired_token_rejection` (security/auth)

5. **Governance** (Compliance)
   - Maturity: Wave 7/8/9 exit criteria
   - Roadmap: Feature alignment with ROADMAP.md
   - Docs: API docs complete, runbooks updated
   - Edition: Community/Minimal separation enforced
   - All modules: 1-3 each
   - Example: `test_server_wave7_criteria_met` (governance/maturity)

6. **Integration/E2E** (End-to-end)
   - Critical paths: Server→Auth→Query→Storage
   - Failover: Node crash, network partition
   - Soak: Long-running (hours/days)
   - Chaos: Random failures, Byzantine
   - Phase 3.4: 30-50 tests
   - Example: `test_authenticated_query_e2e` (integration/critical-path)

7. **Compliance** (Data/Audit)
   - Data Protection: Encryption, deletion, PII
   - Audit: Immutable logs, tamper detection
   - RBAC: Role enforcement, least privilege
   - Tier A modules: 2-3 each
   - Example: `test_pii_fields_encrypted_at_rest` (compliance/data-protection)

8. **Regression** (Bug Prevention)
   - Bug fixes get permanent tests
   - CVE validation
   - All modules: 1-2 each
   - Example: `test_issue_1234_null_pointer_fixed` (regression/issue-1234)

---

## CMake Registration (Copy-Paste Template)

```cmake
# In tests/your_module/CMakeLists.txt

add_test(NAME MyTest COMMAND your_test_executable)

# Tag test with ALL applicable labels
set_tests_properties(MyTest PROPERTIES LABELS "release_critical;security/auth;focused")

# Register for aggregates
set_property(GLOBAL APPEND PROPERTY THEMIS_RELEASE_CRITICAL_TARGETS your_test_executable)
set_property(GLOBAL APPEND PROPERTY THEMIS_FOCUSED_TARGETS your_test_executable)
set_property(GLOBAL APPEND PROPERTY THEMIS_SECURITY_TARGETS your_test_executable)

# Add benchmark if applicable
add_executable(MyBenchmark BenchmarkSource.cpp)
target_link_libraries(MyBenchmark benchmark::benchmark)
set_property(GLOBAL APPEND PROPERTY THEMIS_BENCHMARKS_ALL_TARGETS MyBenchmark)
```

---

## Phase 3 Label Requirements by Batch

### Batch 1: Minimal Modules (80-90 tests)
- `focused` label: 2-3 per module

### Batch 2: Sparse Tier A (200-300 tests)
- `release_critical`: 5-10 per Tier A module
- `security/*`: 3-5 per Security-critical module
- `governance/*`: 2-3 per Tier A module
- `performance`: 2-3 per Tier A module

### Batch 3: TODO→Test (50-100 tests)
- `governance/roadmap`: All rag TODOs

### Batch 4: Integration & Soak (30-50 tests)
- `integration/*`: All E2E, failover, soak, chaos tests
- `compliance/*`: Audit, data protection, RBAC
- `regression/*`: Bug/CVE fixes

---

## Key Differences: Core CTest vs. Release Gate Categories

| Aspect | Core CTest (6) | Release Gates (8) |
|--------|---|---|
| **Purpose** | Structural test organization | Workflow gate integration |
| **Labeling** | Implicit (code organization) | Explicit (CMake labels) |
| **CI/CD Mapping** | 1 gate (gate-pr-core) | 8+ gates (specific workflows) |
| **Block Merge?** | Some (release_critical subset) | Yes (each has own rule) |
| **Frequency** | All on PR | Mixed: PR/weekly/scheduled |
| **Example** | `test_function_valid_input()` | `test_function_valid_input() [release_critical;security/auth]` |

---

## Total Test Count Projection

| Component | Phase 3 Tests | Release-Critical | Focused | Performance | Security | Governance | Integration | Compliance | Regression | **Total** |
|---|---|---|---|---|---|---|---|---|---|---|
| **Tier A** (Server, Sharding, Storage, Query, Transaction, Auth) | ~300 | 30-60 | 30 | 12-18 | 18-30 | 12-18 | 15-20 | 12-18 | 6-12 | ~450-570 |
| **Tier B** (Index, RAG, LLM, Security, Analytics, Geo) | ~200 | - | 20 | 6-9 | 6-9 | 6-9 | 5-10 | 3-6 | 3-6 | ~250-310 |
| **Tier C** (Minimal modules + optional) | 80-90 | - | 15 | - | - | 4-8 | 2-5 | - | 2-4 | ~105-120 |
| **TOTAL** | **~580-590** | **30-60** | **65-75** | **18-27** | **24-39** | **22-35** | **22-35** | **15-24** | **11-22** | **~810-950** |

---

## Release Gate Workflow Mapping

### PR Gates (Must Pass Before Merge)
- ✅ `gate-pr-core.yml` — Runs `release_critical` label tests
- ✅ `gate-pr-security-codeql.yml` — Runs CodeQL (`security/static`)
- ✅ `build-*-fast.yml` lanes — Run `focused` label tests (per module)

### Scheduled Gates (Regular Validation)
- 📅 `benchmark-performance-gate.yml` (weekly) — `performance` label
- 📅 `security-consolidated.yml` (weekly) — `security/*` label
- 📅 `compliance-governance-gates.yml` (daily/weekly) — `governance/*` label

### Ad-Hoc/Manual Gates
- 🔄 `security-pentest-quarterly.yml` — Manual penetration testing
- 🔄 `release-docker-image.yml` — Release candidate validation
- 🔄 `release-mainline.yml` — Full release workflow

---

## Governance Compliance Checklist

- [ ] **Maturity:** Wave 7/8/9 exit criteria verified before GA
- [ ] **Roadmap:** All features in ROADMAP.md have corresponding tests
- [ ] **Documentation:** All public APIs have Doxygen comments
- [ ] **Edition:** Community/Minimal/Enterprise features properly separated
- [ ] **Security:** SAST clean, no hardcoded secrets, auth/crypto validated
- [ ] **Performance:** Benchmarks established, no regression
- [ ] **Audit:** All admin actions logged, tamper detection working
- [ ] **Data Protection:** PII encrypted, deletion requests honored
- [ ] **Regression:** All fixed bugs have permanent tests
- [ ] **Compliance:** RBAC enforced, least privilege maintained

---

## How to Use This Document

1. **For Phase 3 Batch 1 (minimal modules):** Add `focused` label to 2-3 core tests per module
2. **For Phase 3 Batch 2 (Tier A):** Add `release_critical`, `security/*`, `governance/*`, `performance` labels
3. **For Phase 3 Batch 3 (TODO→Test):** Tag rag TODO tests as `governance/roadmap`
4. **For Phase 3 Batch 4 (E2E):** Tag integration tests as `integration/*`
5. **For Phase 4 (CI/CD):** Configure workflow gates to filter by label regex

---

## Related Documents

- `CTEST_QUALITY_STANDARD.md` — Core 6 test categories (function/module/cross-module/happy/error/edge)
- `ADDITIONAL_TEST_CATEGORIES_RELEASE_GATES.md` — Full details (8 additional categories)
- `PHASE3_BATCH1_IMPLEMENTATION_CHECKLIST.md` — Ready-to-code checklist
- `EXECUTIVE_SUMMARY_TEST_DENSITY_2026-10-11.md` — Complete roadmap

---

*Last Updated: 2026-10-11 | Status: Ready for Phase 3 Implementation*
