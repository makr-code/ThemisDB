---
Author: "Copilot Task Agent"
Created: "2026-10-11"
Last Updated: "2026-10-11"
Status: "Release Gate Test Categories - Reference"
---

# Additional Test Categories for Release Gates, Governance & Security

## Context

Beyond the 6 core CTest categories (function, module, cross-module, happy-path, error, edge-case), ThemisDB requires **additional test categories** aligned with GitHub release gates, governance compliance, and security standards.

---

## 1. Release-Critical Tests (Gate: gate-pr-core.yml)

### Purpose
Validate core library paths that block any release or merge to develop/protected branches.

### Characteristics
- Pre-defined set (30 marked tests from `.github/FOCUSED_TEST_GATES.md`)
- Must pass 100% before PR approval to main release branches
- Label: `release_critical`
- CTest filter: `--label-regex "release_critical"`
- Includes: transaction handling, auth, storage, query, sharding

### Example Test Classes
```cpp
TEST(TransactionTest, test_transaction_commit_atomicity) { ... }      // release_critical
TEST(AuthTest, test_token_expiry_enforcement) { ... }                 // release_critical
TEST(StorageTest, test_durability_after_crash_simulation) { ... }     // release_critical
```

### Integration with Phase 3 Tests
- **Tier A modules** (Server, Sharding, Storage, Transaction, Auth, Query) should register **5-10 tests each** as `release_critical`
- Distribution: 80-90% function/module-level, 10-20% cross-module
- Total: 30-60 release_critical tests across all Tier A modules

### CI/CD Requirement
- ✅ Workflow: `gate-pr-core.yml` (PR gate, required to pass before merge)
- ✅ Target: `themis_release_critical_tests` (aggregate CMake target)
- ✅ Failure Impact: PR blocks, must fix + re-approve

---

## 2. Focused Module Tests (Gate: build-*-fast.yml lanes)

### Purpose
Fast feedback on specific module changes without running full test suite.

### Characteristics
- 496 module-specific test targets (per `.github/FOCUSED_TEST_GATES.md`)
- Module-level correctness and API contracts
- Quick execution (<5 minutes per lane)
- Label: `focused` or module-specific regex
- CTest filter: `-R "ModuleNameFocusedTests"`

### Example Test Classes
```cpp
TEST(DistributedTensorFocusedTests, test_tensor_allocation) { ... }
TEST(LlamaCppFocusedTests, test_model_loading) { ... }
TEST(StableDiffusionFocusedTests, test_image_generation) { ... }
```

### Integration with Phase 3 Tests
- **Each module** in Phase 3 should have **3-5 focused tests** for critical paths
- Distributed_tensor: tensor allocation, GPU transfer, serialization
- Llama_cpp: model loading, tokenization, inference
- Stable_diffusion: image generation, parameter validation
- Execution: task execution, cancellation, error propagation

### CI/CD Requirement
- ✅ Workflow: `build-*-fast.yml` lanes (path-triggered, fast feedback)
- ✅ Target: Module-specific `module_*_focused` target
- ✅ Failure Impact: Module-specific lane fails, blocks merge

---

## 3. Benchmark/Performance Tests (Gate: benchmark-performance-gate.yml)

### Purpose
Detect performance regressions and validate against baselines.

### Characteristics
- 19 aggregate benchmark targets
- Weekly schedule + manual trigger
- Validation: `PERFORMANCE_EXPECTATIONS.md` per module
- Label: `performance` or `benchmark`
- Measured: throughput, latency, memory usage

### Example Test Classes
```cpp
BENCHMARK(ServerBenchmarks, test_request_throughput_1000qps) { ... }
BENCHMARK(ShardingBenchmarks, test_rebalancing_latency) { ... }
BENCHMARK(StorageBenchmarks, test_compaction_throughput) { ... }
BENCHMARK(QueryBenchmarks, test_join_latency_1m_rows) { ... }
```

### Integration with Phase 3 Tests
- **Tier A modules** should have **2-3 performance benchmarks** each
- Examples:
  - Server: request throughput, latency percentiles (p50/p95/p99)
  - Sharding: rebalancing latency, data distribution fairness
  - Storage: write throughput, read latency, compaction impact
  - Query: join performance, scan latency, aggregation throughput
- Document expected ranges in `PERFORMANCE_EXPECTATIONS.md`

### CI/CD Requirement
- ✅ Workflow: `benchmark-performance-gate.yml` (weekly schedule)
- ✅ Target: `themis_benchmarks_all_eligible` (aggregate)
- ✅ Baseline: Tracked in `.github/cmake-source-coverage-baseline.txt`
- ✅ Failure Impact: Performance regression warning, requires discussion

---

## 4. Security Tests (Gate: security-consolidated.yml + gate-pr-security-codeql.yml)

### Purpose
Validate security properties and prevent vulnerabilities.

### Categories

#### 4a. Static Analysis Tests (SAST)
- **Label:** `security/static`
- **Tool:** CodeQL (PR gate), Clang Static Analyzer
- **Focus:** Use-after-free, SQL injection, credential exposure, type safety
- **Example Tests:**
  ```cpp
  TEST(SecuritySAST, test_sql_query_injection_prevention) { ... }
  TEST(SecuritySAST, test_buffer_overflow_protection) { ... }
  TEST(SecuritySAST, test_null_pointer_dereference_guard) { ... }
  ```

#### 4b. Secret Scanning Tests
- **Label:** `security/secrets`
- **Tool:** Gitleaks, GitHub secret scanning
- **Focus:** No API keys, credentials, private keys in code
- **Validation:** Forbidden patterns in `.github/security-compliance.json`
  - AWS keys: `AKIA[0-9A-Z]{16}`
  - GitHub PAT: `ghp_[A-Za-z0-9]{36}`
  - Private keys: `-----BEGIN PRIVATE KEY-----`

#### 4c. Dependency Audit Tests
- **Label:** `security/dependency`
- **Tool:** GitHub dependency review, Trivy
- **Focus:** No high/critical CVEs in dependencies
- **Example Tests:**
  ```cpp
  TEST(SecurityDependency, test_vcpkg_baseline_no_vulnerabilities) { ... }
  TEST(SecurityDependency, test_rocksdb_version_patched) { ... }
  ```

#### 4d. Authentication & Authorization Tests
- **Label:** `security/auth`
- **Focus:** Token validation, permission enforcement, role-based access
- **Example Tests:**
  ```cpp
  TEST(SecurityAuth, test_expired_token_rejection) { ... }
  TEST(SecurityAuth, test_permission_escalation_blocked) { ... }
  TEST(SecurityAuth, test_cross_tenant_isolation) { ... }
  ```

#### 4e. Cryptography Tests
- **Label:** `security/crypto`
- **Focus:** Correct algorithm usage, key management, random number generation
- **Example Tests:**
  ```cpp
  TEST(SecurityCrypto, test_aes256_cbc_correct_IV_handling) { ... }
  TEST(SecurityCrypto, test_tls_certificate_validation) { ... }
  TEST(SecurityCrypto, test_random_number_entropy) { ... }
  ```

### Integration with Phase 3 Tests
- **Tier A modules** should register **3-5 security tests each** 
- Tier A: Server (auth, TLS, rate limiting), Storage (encryption, access control)
- Tier A: Transaction (audit logging, isolation), Auth (token validation, RBAC)
- Security module already strong (92 tests), add 5-10 more for edge cases

### CI/CD Requirement
- ✅ Workflow: `security-consolidated.yml` (scheduled weekly, manual dispatch)
- ✅ Workflow: `gate-pr-security-codeql.yml` (PR gate for CodeQL)
- ✅ SARIF upload: GitHub Security tab
- ✅ Failure Impact: Security findings create issues, blocks release

---

## 5. Governance & Compliance Tests (Gate: compliance-governance-gates.yml)

### Purpose
Validate module maturity, roadmap compliance, and governance adherence.

### Categories

#### 5a. Maturity Verification Tests
- **Label:** `governance/maturity`
- **Focus:** Wave-7/8/9 exit criteria for GA releases
- **Example Tests:**
  ```cpp
  TEST(GovernanceMaturity, test_server_wave7_exit_criteria_met) { ... }
  TEST(GovernanceMaturity, test_sharding_wave8_exit_criteria_met) { ... }
  TEST(GovernanceMaturity, test_transaction_wave9_exit_criteria_met) { ... }
  ```

#### 5b. Roadmap Alignment Tests
- **Label:** `governance/roadmap`
- **Focus:** Feature/test alignment with ROADMAP.md
- **Example Tests:**
  ```cpp
  TEST(GovernanceRoadmap, test_distributed_tensor_phase1_deliverables_complete) { ... }
  TEST(GovernanceRoadmap, test_rag_module_30_todos_converted_to_tests) { ... }
  ```

#### 5c. Documentation Governance Tests
- **Label:** `governance/docs`
- **Focus:** API docs, architecture docs, runbooks aligned with code
- **Tool:** gate-pr-doxygen-governance.yml
- **Example Tests:**
  ```cpp
  TEST(GovernanceDocs, test_all_public_apis_have_doxygen_comments) { ... }
  TEST(GovernanceDocs, test_error_codes_documented) { ... }
  TEST(GovernanceDocs, test_runbook_covers_failure_modes) { ... }
  ```

#### 5d. Release Gate Tests
- **Label:** `governance/release`
- **Focus:** Release candidate validation (RC tags)
- **Example Tests:**
  ```cpp
  TEST(GovernanceRelease, test_rc_tag_version_matches_package) { ... }
  TEST(GovernanceRelease, test_changelog_updated_for_rc) { ... }
  TEST(GovernanceRelease, test_security_advisory_check_passed) { ... }
  ```

#### 5e. Edition Compliance Tests
- **Label:** `governance/edition`
- **Focus:** Community/Minimal/Enterprise separation
- **Example Tests:**
  ```cpp
  TEST(GovernanceEdition, test_enterprise_features_not_in_minimal) { ... }
  TEST(GovernanceEdition, test_community_credentials_not_in_minimal) { ... }
  TEST(GovernanceEdition, test_military_features_isolated_correctly) { ... }
  ```

### Integration with Phase 3 Tests
- **Each module** in Phase 3 should register **1-3 governance tests**
- Focus: Roadmap alignment, documentation completeness, wave-gate compliance
- Register with label: `governance/<type>`

### CI/CD Requirement
- ✅ Workflow: `compliance-governance-gates.yml` (scheduled daily, on-merge)
- ✅ Trigger: ROADMAP.md, FUTURE_ENHANCEMENTS.md changes, tags `v*-rc.*`
- ✅ Failure Impact: Creates GitHub issues, blocks release, requires waiver approval

---

## 6. Integration/E2E Tests (Cross-Tier)

### Purpose
Validate multi-module workflows and end-to-end scenarios.

### Categories

#### 6a. Critical Path Tests
- **Label:** `integration/critical-path`
- **Focus:** Server→Auth→Query→Storage→Transaction flow
- **Example Tests:**
  ```cpp
  TEST(IntegrationCriticalPath, test_authenticated_query_execution_e2e) { ... }
  TEST(IntegrationCriticalPath, test_multi_node_transaction_commit_e2e) { ... }
  ```

#### 6b. Failover/Recovery Tests
- **Label:** `integration/failover`
- **Focus:** Node failure, network partition, data recovery
- **Example Tests:**
  ```cpp
  TEST(IntegrationFailover, test_server_crashes_requests_retry_on_replica) { ... }
  TEST(IntegrationFailover, test_storage_corruption_detected_and_recovered) { ... }
  ```

#### 6c. Soak Tests (Long-Running)
- **Label:** `integration/soak`
- **Duration:** Hours to days
- **Focus:** Stability under continuous load, memory leaks, resource exhaustion
- **Example Tests:**
  ```cpp
  TEST(IntegrationSoak, test_server_1m_requests_no_memory_leak) { ... }
  TEST(IntegrationSoak, test_storage_10k_writes_per_second_24h) { ... }
  ```

#### 6d. Chaos Tests
- **Label:** `integration/chaos`
- **Focus:** Random failures, network delays, Byzantine conditions
- **Example Tests:**
  ```cpp
  TEST(IntegrationChaos, test_sharding_tolerates_2_node_failure) { ... }
  TEST(IntegrationChaos, test_query_survives_random_network_latency) { ... }
  ```

### Integration with Phase 3 Tests
- **Batch 4** (integration & soak tests) focuses on these categories
- 30-50 new tests across critical-path, failover, soak, chaos
- Label all with `integration/<category>`

### CI/CD Requirement
- ✅ Workflow: `gate-pr-integration.yml` or separate workflow (not on every PR, scheduled)
- ✅ Trigger: Weekly for soak, manually for chaos (performance cost)
- ✅ Failure Impact: Blocks release, requires root-cause investigation

---

## 7. Compliance Tests (Data Privacy, Audit)

### Purpose
Validate data handling, privacy, and audit trail compliance.

### Categories

#### 7a. Data Protection Tests
- **Label:** `compliance/data-protection`
- **Focus:** Encryption at rest/transit, data deletion, PII handling
- **Example Tests:**
  ```cpp
  TEST(ComplianceDataProtection, test_pii_fields_encrypted_at_rest) { ... }
  TEST(ComplianceDataProtection, test_deletion_request_purges_all_records) { ... }
  TEST(ComplianceDataProtection, test_tls_encryption_enforced) { ... }
  ```

#### 7b. Audit Logging Tests
- **Label:** `compliance/audit`
- **Focus:** Immutable audit logs, tamper detection, retention
- **Example Tests:**
  ```cpp
  TEST(ComplianceAudit, test_all_administrative_actions_logged) { ... }
  TEST(ComplianceAudit, test_audit_log_tamper_detection) { ... }
  TEST(ComplianceAudit, test_audit_retention_complies_with_policy) { ... }
  ```

#### 7c. RBAC/Access Control Tests
- **Label:** `compliance/rbac`
- **Focus:** Role-based access, least privilege, segregation of duties
- **Example Tests:**
  ```cpp
  TEST(ComplianceRBAC, test_user_cannot_exceed_assigned_role) { ... }
  TEST(ComplianceRBAC, test_admin_separation_enforced) { ... }
  ```

### Integration with Phase 3 Tests
- Focus on Security and Auth modules (already strong)
- Add 5-10 compliance tests per Tier A module

---

## 8. Regression Tests

### Purpose
Prevent re-introduction of previously fixed bugs.

### Categories

#### 8a. Bug Regression Tests
- **Label:** `regression/<bug-id>`
- **Focus:** Each fixed bug gets a permanent test
- **Example Tests:**
  ```cpp
  TEST(Regression, test_issue_1234_null_pointer_in_parser) { ... }
  TEST(Regression, test_pr_5678_deadlock_in_transaction_rollback) { ... }
  ```

#### 8b. CVE Regression Tests
- **Label:** `regression/cve`
- **Focus:** Vulnerabilities are fixed and not re-introduced
- **Example Tests:**
  ```cpp
  TEST(Regression, test_cve_2026_1234_sql_injection_fixed) { ... }
  TEST(Regression, test_cve_2026_5678_buffer_overflow_fixed) { ... }
  ```

### Integration with Phase 3 Tests
- Add at least 1 regression test per Tier A/B module
- 10-15 total regression tests

---

## Summary: Complete Test Category Framework

| Category | Purpose | Phase 3 Integration | CI/CD Gate | Label |
|----------|---------|---|---|---|
| **Function-Level** | Unit APIs | 50-60% of tests | gate-pr-core (partial) | none |
| **Module-Level** | Component integration | 15-20% of tests | build-*-fast | `focused` |
| **Cross-Module** | Dependency flow | 8-15% of tests | gate-pr-core (partial) | none |
| **Happy-Path** | Normal operation | 10-15% of tests | gate-pr-core (partial) | none |
| **Error/Failure** | Error handling | 8-12% of tests | gate-pr-core (partial) | none |
| **Edge-Case** | Boundary conditions | 5-8% of tests | gate-pr-core (partial) | none |
| **Release-Critical** | Core validation | 5-10% per Tier A | gate-pr-core ✅ | `release_critical` |
| **Focused** | Fast module feedback | 3-5% per module | build-*-fast ✅ | `focused` |
| **Benchmark** | Performance regression | 2-5% per Tier A | benchmark-performance-gate ✅ | `performance` |
| **Security** | Vuln/auth/crypto | 3-5% per Tier A | security-consolidated ✅ | `security/*` |
| **Governance** | Maturity/compliance | 1-3% per module | compliance-governance-gates ✅ | `governance/*` |
| **Integration/Soak/Chaos** | E2E + stress | 5-10% (Phase 3.4) | Scheduled separately ✅ | `integration/*` |
| **Compliance** | Data/audit/RBAC | 2-3% per Tier A | Scheduled separately ✅ | `compliance/*` |
| **Regression** | Bug/CVE prevention | 1-2% per module | gate-pr-core (optional) | `regression/*` |

---

## Phase 3 Implementation Roadmap

### Phase 3 Batch 1: Minimal Modules (2-3 weeks)
- 80-90 tests (function/module/cross-module/happy/error/edge)
- Register 2-3 tests per module as `focused`
- No release_critical, security, or governance tests (these are Tier A focus)

### Phase 3 Batch 2: Sparse Tier A (4-6 weeks)
- 200-300 tests total
- **Register per module:**
  - Server: 5-10 `release_critical`, 3-5 `security/*`, 2-3 `governance/*`, 2-3 `performance`
  - Sharding: 5-10 `release_critical`, 2-3 `integration/*`, 2-3 `performance`
  - Storage: 5-10 `release_critical`, 2-3 `compliance/*`, 2-3 `performance`

### Phase 3 Batch 3: TODO→Test Conversion (3-4 weeks)
- 50-100 tests
- rag module: 30 TODOs → tests with `governance/roadmap` labels

### Phase 3 Batch 4: Integration & Soak (2-3 weeks)
- 30-50 tests
- Register as `integration/*`, `regression/*`, `compliance/*`

### Phase 4: CI/CD Integration (1-2 weeks)
- Setup CMake labels for all categories
- Configure workflow gates to filter by label
- Create baseline reports per category

---

## Recommended CMake Registration Pattern

```cmake
# Module CMakeLists.txt

add_test(NAME MyTest COMMAND my_test_executable)

# Mark for different gates
set_tests_properties(MyTest PROPERTIES LABELS "release_critical;focused;security/auth")

# Register for aggregates
set_property(GLOBAL APPEND PROPERTY THEMIS_RELEASE_CRITICAL_TARGETS my_test_executable)
set_property(GLOBAL APPEND PROPERTY THEMIS_FOCUSED_TARGETS my_test_executable)
set_property(GLOBAL APPEND PROPERTY THEMIS_SECURITY_TARGETS my_test_executable)

# Add to benchmark aggregates if applicable
add_executable(MyBenchmark MyBenchmark.cpp)
target_link_libraries(MyBenchmark benchmark::benchmark)
set_property(GLOBAL APPEND PROPERTY THEMIS_BENCHMARKS_ALL_TARGETS MyBenchmark)
```

---

*This document supplements the CTest Quality Standard and provides GitHub release gate alignment.*

