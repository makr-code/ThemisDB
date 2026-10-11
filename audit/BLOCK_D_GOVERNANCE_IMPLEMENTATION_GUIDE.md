---
Author: "Copilot Task Agent"
Created: "2026-10-11"
Last Updated: "2026-10-11"
Status: "Block D - Governance/Compliance Planning"
---

# Block D: Governance & Compliance Framework

## Purpose

Block D implements test-based enforcement of governance gates that validate:
1. **Maturity Wave Exit Criteria** (Wave 7/8/9 readiness)
2. **Roadmap Alignment** (features match ROADMAP.md items)
3. **Edition Compliance** (Community/Minimal/Enterprise separation)
4. **Documentation Governance** (API docs complete + SOT alignment)

These tests enable automated compliance validation before GA release and ensure consistent quality across edition releases.

---

## D1: Wave Exit Criteria Validation Tests

### Background: ThemisDB Release Waves

ThemisDB uses a structured release process with numbered "waves" representing maturity gates:

- **Wave 1-6:** Development phases (RFC → PR → implementation)
- **Wave 7 (RC1):** Release Candidate 1 — Feature frozen, bug fixes only
  - Exit criteria: Code review coverage, test density, documentation complete
- **Wave 8 (RC2):** Release Candidate 2 — Performance validated, security passed
  - Exit criteria: Performance benchmarks passed, security scan clean, compliance gates
- **Wave 9 (GA):** General Availability — Production ready
  - Exit criteria: SLA signed, runbooks deployed, support escalation tested

### D1 Implementation: Wave Exit Criteria Tests

**For Wave 7 (Code Review & Test Density):**

```cpp
// tests/governance/test_wave7_exit_criteria.cpp
#include <gtest/gtest.h>
#include "governance/wave_validator.h"

namespace themis {
namespace tests {

class Wave7ExitCriteriaTest : public ::testing::Test {
  // Validates Wave 7 exit requirements:
  // - All public APIs have Doxygen comments
  // - Test density >= 1 test per 500 LOC (governance/roadmap)
  // - Code review tracking (REVIEWED marker on critical functions)
  // - Documentation completeness (SOT alignment)
};

TEST_F(Wave7ExitCriteriaTest, AllPublicAPIsDocumented) {
  // Parse Doxygen XML output
  // Verify @brief, @param, @return, @throws present
  // Fail if coverage < 95%
  EXPECT_GE(doxygen_coverage_percent(), 95.0);
}

TEST_F(Wave7ExitCriteriaTest, TestDensityRequirement) {
  // Read PHASE1_COVERAGE_BASELINE_2026-10-11.md
  // For each Tier A module:
  //   lines_of_code = 45230 (example: Server)
  //   required_tests = lines_of_code / 500 = ~90 tests
  //   actual_tests = ctest --count-tests
  //   EXPECT_GE(actual_tests, required_tests)
  EXPECT_GE(get_module_test_count("server"), 90);
  EXPECT_GE(get_module_test_count("sharding"), 56);
  EXPECT_GE(get_module_test_count("storage"), 62);
}

TEST_F(Wave7ExitCriteriaTest, CodeReviewTracking) {
  // Scan critical functions for REVIEWED marker
  // Example: bool authenticate_token(const Token& t) { /* REVIEWED */ }
  // Tier A modules must have >=90% critical functions reviewed
  EXPECT_GE(reviewed_function_ratio("server"), 0.90);
}

TEST_F(Wave7ExitCriteriaTest, DocumentationSOTAlignment) {
  // Compare Doxygen @module tags with MODULES_AND_APIS.md
  // Verify consistency in module descriptions
  EXPECT_TRUE(doxygen_matches_sot("ai_context/MODULES_AND_APIS.md"));
}

} // namespace tests
} // namespace themis
```

**For Wave 8 (Performance & Security):**

```cpp
// tests/governance/test_wave8_exit_criteria.cpp

TEST_F(Wave8ExitCriteriaTest, PerformanceBenchmarksPassed) {
  // Compare against PERFORMANCE_EXPECTATIONS.md thresholds
  // For Server module: QPS >= 1000, p99 latency <= 50ms
  // For Sharding: Cross-shard latency <= 100ms
  EXPECT_GE(get_throughput("server_queries_per_second"), 1000.0);
  EXPECT_LE(get_latency_p99("server_query_latency_ms"), 50.0);
}

TEST_F(Wave8ExitCriteriaTest, SecurityScanClean) {
  // Run CodeQL checker (gate-pr-security-codeql.yml equivalent)
  // Expect 0 HIGH/CRITICAL alerts
  int critical_alerts = run_codeql_check();
  EXPECT_EQ(critical_alerts, 0);
}

TEST_F(Wave8ExitCriteriaTest, ComplianceGatesPassed) {
  // Verify compliance-governance-gates.yml checks pass
  // - Branch protection rules enforced
  // - Required status checks enabled
  // - Force-push disabled
  EXPECT_TRUE(verify_branch_protection_rules());
}

TEST_F(Wave8ExitCriteriaTest, AuditLoggingFunctional) {
  // Test audit module tracks admin operations
  // Verify immutability (tamper-detection enabled)
  EXPECT_TRUE(audit_logger_is_functional());
  EXPECT_TRUE(audit_tamper_detection_enabled());
}
```

**For Wave 9 (GA Readiness):**

```cpp
// tests/governance/test_wave9_exit_criteria.cpp

TEST_F(Wave9ExitCriteriaTest, SLAValidation) {
  // Verify SLA targets from RELEASE_STRATEGY.md
  // Example: 99.95% availability, <1% data loss
  EXPECT_GE(get_availability_sla_percent(), 99.95);
  EXPECT_LE(get_data_loss_percent(), 1.0);
}

TEST_F(Wave9ExitCriteriaTest, RunbooksDeployed) {
  // Check existence of runbooks for critical scenarios
  // - Failover procedure documented
  // - Disaster recovery validated
  // - Capacity planning guide published
  EXPECT_TRUE(runbook_exists("failover-procedure.md"));
  EXPECT_TRUE(runbook_exists("disaster-recovery.md"));
}

TEST_F(Wave9ExitCriteriaTest, SupportEscalationTested) {
  // Verify support workflow is functional
  // - Escalation paths defined
  // - On-call rotation active
  // - Incident response validated
  EXPECT_TRUE(support_escalation_configured());
}
```

### D1 Test Configuration

```cmake
# tests/governance/CMakeLists.txt

add_executable(test_wave7_exit_criteria test_wave7_exit_criteria.cpp)
target_link_libraries(test_wave7_exit_criteria 
  gtest gtest_main 
  themis_governance
  themis_doxygen_parser
)
set_tests_properties(test_wave7_exit_criteria PROPERTIES 
  LABELS "governance/maturity;governance/wave7"
)

add_executable(test_wave8_exit_criteria test_wave8_exit_criteria.cpp)
target_link_libraries(test_wave8_exit_criteria 
  gtest gtest_main 
  themis_governance
  themis_codeql_parser
  themis_benchmark_validator
)
set_tests_properties(test_wave8_exit_criteria PROPERTIES 
  LABELS "governance/maturity;governance/wave8"
)

# Run as part of release gate
set_property(GLOBAL APPEND PROPERTY THEMIS_GOVERNANCE_MATURITY_TARGETS 
  test_wave7_exit_criteria test_wave8_exit_criteria
)
```

---

## D2: Roadmap→Test Mapping Verification

### Overview

Every feature in ROADMAP.md should have a corresponding test. This test category validates that implementation tests are created and labeled appropriately.

### Implementation

**Scanning ROADMAP.md:**

```markdown
# Example ROADMAP.md entry

## Tier A: Critical Path (Q4 2026)

- [x] CUDA geospatial distance and containment kernels (Target: Q3 2026)
  - Tests: test_cuda_geospatial_distance_valid_points, test_cuda_geospatial_containment_*
  - Labels: `governance/roadmap;performance;cuda`
  - Coverage: function, module, edge-cases, performance

- [ ] Sharding rebalance with minimal data movement (Target: Q4 2026)
  - Tests: test_sharding_rebalance_*, test_sharding_data_movement_*
  - Status: Planned for Phase 3 Batch 2
  - Estimated effort: 40-60 hours
```

**Automated Validation Test:**

```cpp
// tests/governance/test_roadmap_alignment.cpp
#include <gtest/gtest.h>
#include "governance/roadmap_parser.h"
#include "governance/test_analyzer.h"

namespace themis {
namespace tests {

class RoadmapAlignmentTest : public ::testing::Test {
public:
  static void SetUpTestSuite() {
    roadmap_items = parse_roadmap_md("ROADMAP.md");
    implemented_tests = scan_test_directories();
  }
  
  static std::vector<RoadmapItem> roadmap_items;
  static std::vector<TestMetadata> implemented_tests;
};

// Test 1: All completed items have tests
TEST_F(RoadmapAlignmentTest, CompletedItemsHaveTests) {
  for (const auto& item : roadmap_items) {
    if (item.status == "completed" || item.checkbox == "[x]") {
      EXPECT_TRUE(tests_exist_for(item.description))
        << "Feature '" << item.description << "' is marked complete but has no tests";
    }
  }
}

// Test 2: All tests have governance/roadmap label
TEST_F(RoadmapAlignmentTest, TestsProperlyLabeled) {
  for (const auto& test : implemented_tests) {
    if (test.feature_name != "") {
      EXPECT_TRUE(has_label(test, "governance/roadmap"))
        << "Test '" << test.name << "' implements feature '" 
        << test.feature_name << "' but missing governance/roadmap label";
    }
  }
}

// Test 3: In-progress items have test placeholders
TEST_F(RoadmapAlignmentTest, InProgressItemsHavePlans) {
  for (const auto& item : roadmap_items) {
    if (item.status == "in_progress" || item.checkbox == "[~]") {
      // Either tests exist, or item has test plan in comments
      EXPECT_TRUE(
        tests_exist_for(item.description) || 
        has_test_plan_comment(item)
      ) << "In-progress item '" << item.description 
        << "' missing test plan or implementation";
    }
  }
}

// Test 4: Coverage distribution matches plan
TEST_F(RoadmapAlignmentTest, CoverageDistributionMatches) {
  for (const auto& item : roadmap_items) {
    auto tests = get_tests_for(item.description);
    
    // Verify CTest category distribution
    // function (40-60%), module (15-20%), cross-module (8-15%), etc.
    double function_pct = get_label_percentage(tests, "function");
    double module_pct = get_label_percentage(tests, "module");
    
    EXPECT_GE(function_pct, 40.0) << "Insufficient function-level tests for " << item.description;
    EXPECT_GE(module_pct, 15.0) << "Insufficient module-level tests for " << item.description;
  }
}

} // namespace tests
} // namespace themis
```

**Test Configuration:**

```cmake
# tests/governance/CMakeLists.txt

add_executable(test_roadmap_alignment test_roadmap_alignment.cpp)
target_link_libraries(test_roadmap_alignment 
  gtest gtest_main
  themis_governance
)
set_tests_properties(test_roadmap_alignment PROPERTIES 
  LABELS "governance/roadmap"
)

# This test should be part of compliance gates
set_property(GLOBAL APPEND PROPERTY THEMIS_GOVERNANCE_TARGETS 
  test_roadmap_alignment
)
```

---

## D3: Edition Compliance Tests

### Background: Edition Strategy

ThemisDB supports multiple editions with feature flags:
- **Community** (THEMIS_EDITION_COMMUNITY): All features enabled
- **Minimal** (THEMIS_EDITION_MINIMAL): Only core (Server, Auth, Storage)
- **Enterprise** (THEMIS_EDITION_ENTERPRISE): Community + audit, compliance, advanced
- **Military** (THEMIS_EDITION_MILITARY): Enterprise + hardened crypto, FedRAMP prep

### D3 Implementation

```cpp
// tests/governance/test_edition_compliance.cpp
#include <gtest/gtest.h>
#include "config/edition_config.h"

namespace themis {
namespace tests {

// Test Community Edition (all features)
class EditionCommunityTest : public ::testing::Test {
protected:
  EditionCommunityTest() {
    set_edition_override(Edition::kCommunity);
  }
};

TEST_F(EditionCommunityTest, AllModulesAvailable) {
  EXPECT_TRUE(is_module_available("server"));
  EXPECT_TRUE(is_module_available("auth"));
  EXPECT_TRUE(is_module_available("storage"));
  EXPECT_TRUE(is_module_available("query"));
  EXPECT_TRUE(is_module_available("rag"));       // RAG available in Community
  EXPECT_TRUE(is_module_available("llm"));       // LLM available in Community
  EXPECT_TRUE(is_module_available("gpu"));       // GPU available in Community
}

TEST_F(EditionCommunityTest, AdvancedFeaturesEnabled) {
  EXPECT_TRUE(feature_enabled("advanced_indexing"));
  EXPECT_TRUE(feature_enabled("sharding_v2"));
  EXPECT_TRUE(feature_enabled("distributed_transactions"));
}

// Test Minimal Edition (core only)
class EditionMinimalTest : public ::testing::Test {
protected:
  EditionMinimalTest() {
    set_edition_override(Edition::kMinimal);
  }
};

TEST_F(EditionMinimalTest, OnlyCoreModulesAvailable) {
  EXPECT_TRUE(is_module_available("server"));
  EXPECT_TRUE(is_module_available("auth"));
  EXPECT_TRUE(is_module_available("storage"));
  EXPECT_TRUE(is_module_available("query"));
  EXPECT_TRUE(is_module_available("transaction"));
  
  // These should NOT be available in Minimal
  EXPECT_FALSE(is_module_available("rag"));
  EXPECT_FALSE(is_module_available("llm"));
  EXPECT_FALSE(is_module_available("gpu"));
  EXPECT_FALSE(is_module_available("voice"));
  EXPECT_FALSE(is_module_available("analytics"));
}

TEST_F(EditionMinimalTest, AdvancedFeaturesDisabled) {
  EXPECT_FALSE(feature_enabled("advanced_indexing"));
  EXPECT_FALSE(feature_enabled("distributed_transactions"));
}

TEST_F(EditionMinimalTest, CoreFeaturesFunctional) {
  // Server: ✓ (required)
  EXPECT_TRUE(server_accepts_connections());
  
  // Auth: ✓ (required)
  EXPECT_TRUE(auth_module_validates_tokens());
  
  // Storage: ✓ (required)
  EXPECT_TRUE(storage_executes_queries());
}

// Test Enterprise Edition (Community + compliance)
class EditionEnterpriseTest : public ::testing::Test {
protected:
  EditionEnterpriseTest() {
    set_edition_override(Edition::kEnterprise);
  }
};

TEST_F(EditionEnterpriseTest, CommunityFeaturesPlus) {
  // All Community features
  EXPECT_TRUE(is_module_available("server"));
  EXPECT_TRUE(is_module_available("rag"));
  EXPECT_TRUE(is_module_available("llm"));
  
  // Plus enterprise features
  EXPECT_TRUE(feature_enabled("audit_logging"));
  EXPECT_TRUE(feature_enabled("data_encryption_at_rest"));
  EXPECT_TRUE(feature_enabled("compliance_framework"));
  EXPECT_TRUE(feature_enabled("rbac_advanced"));
}

TEST_F(EditionEnterpriseTest, ComplianceFeaturesEnabled) {
  EXPECT_TRUE(audit_logger_present());
  EXPECT_TRUE(encryption_at_rest_configured());
  EXPECT_TRUE(rbac_enforcement_active());
}

// Test Military Edition (Enterprise + hardened)
class EditionMilitaryTest : public ::testing::Test {
protected:
  EditionMilitaryTest() {
    set_edition_override(Edition::kMilitary);
  }
};

TEST_F(EditionMilitaryTest, MilitarySpecificFeatures) {
  EXPECT_TRUE(feature_enabled("fips_mode"));
  EXPECT_TRUE(feature_enabled("federal_crypto_requirements"));
  EXPECT_TRUE(feature_enabled("tamper_detection"));
  EXPECT_TRUE(feature_enabled("multiparty_encryption"));
}

TEST_F(EditionMilitaryTest, CryptoHardening) {
  EXPECT_TRUE(uses_fips_approved_algorithms());
  EXPECT_EQ(get_tls_version_minimum(), "1.3");
  EXPECT_EQ(get_default_cipher_suite(), "TLS_AES_256_GCM_SHA384");
}

} // namespace tests
} // namespace themis
```

### D3 Configuration

```cmake
# tests/governance/CMakeLists.txt

# Community Edition test
add_executable(test_edition_community test_edition_compliance.cpp)
target_compile_definitions(test_edition_community PRIVATE THEMIS_EDITION_TEST=1)
target_link_libraries(test_edition_community 
  gtest gtest_main
  themis_config
  themis_server themis_auth themis_storage
)
set_tests_properties(test_edition_community PROPERTIES 
  LABELS "governance/edition;compliance/edition"
)

# Minimal Edition test
add_executable(test_edition_minimal test_edition_compliance.cpp)
target_compile_definitions(test_edition_minimal PRIVATE THEMIS_EDITION_TEST=1 THEMIS_EDITION_MINIMAL=1)
target_link_libraries(test_edition_minimal themis_config)
set_tests_properties(test_edition_minimal PROPERTIES 
  LABELS "governance/edition"
)

# Enterprise test
add_executable(test_edition_enterprise test_edition_compliance.cpp)
target_compile_definitions(test_edition_enterprise PRIVATE THEMIS_EDITION_TEST=1 THEMIS_EDITION_ENTERPRISE=1)
set_tests_properties(test_edition_enterprise PROPERTIES 
  LABELS "governance/edition"
)

# Military test
add_executable(test_edition_military test_edition_compliance.cpp)
target_compile_definitions(test_edition_military PRIVATE THEMIS_EDITION_TEST=1 THEMIS_EDITION_MILITARY=1)
set_tests_properties(test_edition_military PROPERTIES 
  LABELS "governance/edition;security/compliance"
)

# Aggregate target for edition compliance
set_property(GLOBAL APPEND PROPERTY THEMIS_EDITION_COMPLIANCE_TARGETS 
  test_edition_community test_edition_minimal test_edition_enterprise test_edition_military
)
```

---

## D4: Documentation Governance Gates

### Overview

Ensures API documentation is complete, consistent with SOT, and includes required information.

### Implementation

```cpp
// tests/governance/test_docs_governance.cpp
#include <gtest/gtest.h>
#include "governance/doxygen_parser.h"
#include "governance/sot_validator.h"

namespace themis {
namespace tests {

class DocsGovernanceTest : public ::testing::Test {
protected:
  static void SetUpTestSuite() {
    doxygen_xml = parse_doxyfile("Doxyfile.audit");
    sot_content = load_file("ai_context/MODULES_AND_APIS.md");
  }
  
  static DoxygenXMLDoc doxygen_xml;
  static std::string sot_content;
};

// Test 1: All public APIs documented
TEST_F(DocsGovernanceTest, PublicAPIsHaveDoxygen) {
  auto public_functions = get_public_function_symbols();
  
  for (const auto& func : public_functions) {
    auto docs = doxygen_xml.get_documentation(func);
    EXPECT_TRUE(docs.has_brief_description()) 
      << "Function " << func << " missing @brief";
    EXPECT_TRUE(docs.has_return_description())
      << "Function " << func << " missing @return";
    EXPECT_GT(docs.detailed_description.length(), 50)
      << "Function " << func << " has insufficient documentation";
  }
}

// Test 2: Parameter documentation complete
TEST_F(DocsGovernanceTest, ParametersFullyDocumented) {
  auto functions_with_params = get_functions_with_parameters();
  
  for (const auto& func : functions_with_params) {
    auto params = get_function_parameters(func);
    auto docs = doxygen_xml.get_documentation(func);
    
    EXPECT_EQ(params.size(), docs.param_descriptions.size())
      << "Function " << func << " has " << params.size() 
      << " parameters but only " << docs.param_descriptions.size() 
      << " @param tags";
  }
}

// Test 3: SOT consistency
TEST_F(DocsGovernanceTest, SOTConsistency) {
  // Parse MODULES_AND_APIS.md
  auto modules = extract_module_list(sot_content);
  
  // Compare with Doxygen modules
  auto doxygen_modules = doxygen_xml.get_modules();
  
  for (const auto& module : modules) {
    EXPECT_TRUE(contains(doxygen_modules, module))
      << "Module " << module << " in SOT but not in Doxygen";
  }
}

// Test 4: Runbooks exist (Tier A modules only)
TEST_F(DocsGovernanceTest, TierARunbooksExist) {
  std::vector<std::string> tier_a_modules = {
    "server", "auth", "storage", "query", "transaction", "sharding"
  };
  
  for (const auto& module : tier_a_modules) {
    EXPECT_TRUE(file_exists(fmt::format("docs/runbooks/{}.md", module)))
      << "Runbook missing for Tier A module: " << module;
  }
}

// Test 5: Thread safety documented
TEST_F(DocsGovernanceTest, ThreadSafetyDocumented) {
  auto public_classes = get_public_classes();
  
  for (const auto& cls : public_classes) {
    auto docs = doxygen_xml.get_documentation(cls);
    EXPECT_TRUE(
      contains_any(docs.full_description, {
        "thread-safe", "thread safe", "Thread-Safe", "NOT thread-safe"
      })
    ) << "Class " << cls << " missing thread-safety documentation";
  }
}

} // namespace tests
} // namespace themis
```

### D4 Configuration

```cmake
# tests/governance/CMakeLists.txt

add_executable(test_docs_governance test_docs_governance.cpp)
target_link_libraries(test_docs_governance 
  gtest gtest_main
  themis_doxygen_parser
  themis_governance
)
set_tests_properties(test_docs_governance PROPERTIES 
  LABELS "governance/docs"
)

set_property(GLOBAL APPEND PROPERTY THEMIS_GOVERNANCE_TARGETS 
  test_docs_governance
)
```

---

## Block D Timeline

| Component | Tests | Effort | Duration | Status |
|-----------|-------|--------|----------|--------|
| **D1.1** | Wave 7 exit criteria | 15 tests | 15-20 hours | Ready |
| **D1.2** | Wave 8 exit criteria | 10 tests | 10-15 hours | Ready |
| **D1.3** | Wave 9 exit criteria | 5 tests | 5-10 hours | Ready |
| **D2** | Roadmap alignment | 4 tests | 10-15 hours | Ready |
| **D3** | Edition compliance | 15 tests | 15-20 hours | Ready |
| **D4** | Docs governance | 8 tests | 10-15 hours | Ready |
| **Total** | ~57 tests | 65-95 hours | 2-3 weeks | Ready |

---

## Next: Block D → Block A

Once governance tests are implemented:

1. All tests tagged with `governance/*` labels
2. CMake registration complete
3. Wave exit criteria validators functional
4. Edition compliance enforced

Then proceed to **Block A: Immediate Implementation (Phase 3 Batch 1)**
- 80-90 tests for minimal modules (distributed_tensor, llama_cpp, stable_diffusion, execution)
- All tagged with `focused` label
- CTest category distribution per CTEST_QUALITY_STANDARD.md

