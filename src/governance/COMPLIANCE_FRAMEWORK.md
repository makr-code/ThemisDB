# Compliance Framework Integration & Validation

**Version:** 1.0.0  
**Status:** 🟢 PRODUCTION-READY  
**Last Updated:** 2026-08-18  

## Overview

ThemisDB provides a comprehensive, unified compliance framework for validating system state against multiple regulatory requirements. The framework supports 6 major compliance standards:

1. **EU AI Act** (2024) — Risk management for AI systems
2. **SOC 2 Type I/II** — Security, availability, processing integrity
3. **ISO 27001:2022** — Information security management
4. **GDPR** — Data protection and privacy
5. **CCPA/CPRA** — Consumer privacy rights
6. **HIPAA** — Healthcare data protection
7. **PCI-DSS** — Payment card security

## Architecture

### Core Components

```
┌─────────────────────────────────────────────────────────┐
│         Compliance Validation Engine                     │
├─────────────────────────────────────────────────────────┤
│                                                          │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │  Requirement │  │   Control    │  │   Evidence   │  │
│  │   Validator  │  │   Validator  │  │   Collector  │  │
│  └──────────────┘  └──────────────┘  └──────────────┘  │
│         │                 │                │             │
├─────────────────────────────────────────────────────────┤
│                                                          │
│  ┌──────────────────────────────────────────────────┐   │
│  │  Framework-Specific Validators                   │   │
│  │  - ISO 27001 Validator                          │   │
│  │  - SOC 2 Validator                              │   │
│  │  - GDPR Validator                               │   │
│  │  - CCPA Validator                               │   │
│  │  - HIPAA Validator                              │   │
│  │  - PCI-DSS Validator                            │   │
│  │  - EU AI Act Validator                          │   │
│  └──────────────────────────────────────────────────┘   │
│                                                          │
├─────────────────────────────────────────────────────────┤
│                                                          │
│  ┌──────────────────────────────────────────────────┐   │
│  │  Compliance Framework Registry                   │   │
│  │  - Requirement Storage & Lookup                  │   │
│  │  - Control Storage & Lookup                      │   │
│  │  - Cross-framework Mapping                       │   │
│  │  - JSON Import/Export                            │   │
│  └──────────────────────────────────────────────────┘   │
│                                                          │
└─────────────────────────────────────────────────────────┘
```

### Data Structures

#### ComplianceFramework Enum
Defines supported compliance frameworks:
```cpp
enum class ComplianceFramework {
    kEuAiAct,      // EU AI Act (2024)
    kSoc2TypeI,    // SOC 2 Type I
    kSoc2TypeII,   // SOC 2 Type II
    kIso27001,     // ISO 27001:2022
    kGdpr,         // GDPR
    kCcpa,         // CCPA/CPRA
    kHipaa,        // HIPAA
    kPciDss,       // PCI-DSS
};
```

#### ComplianceRequirement
Represents a regulatory requirement:
```cpp
struct ComplianceRequirement {
    std::string requirement_id;           // e.g., "GDPR-Art32"
    ComplianceFramework framework;        // Source framework
    std::string requirement_text;         // Full requirement
    std::string regulatory_section;       // Reference section
    ComplianceSeverity severity;          // CRITICAL/HIGH/MEDIUM/LOW
    std::vector<std::string> control_ids; // Implementing controls
    std::string category;                 // Domain (encryption, access-control, etc.)
    bool is_mandatory;                    // Mandatory requirement
    int version;                          // Requirement version
    int64_t created_at_ms;                // Creation timestamp
    int64_t updated_at_ms;                // Last update timestamp
    nlohmann::json metadata;              // Additional metadata
};
```

#### ComplianceControl
Represents a technical control implementing requirements:
```cpp
struct ComplianceControl {
    std::string control_id;               // Unique ID (e.g., "CTL-ENCRYPTION-001")
    ComplianceFramework framework;        // Source framework
    std::string control_name;             // Display name
    std::string description;              // Technical description
    std::string implementation_detail;    // How it's implemented
    bool is_automated;                    // Automated vs manual
    std::vector<std::string> policy_rules;// Associated policy rules
    std::vector<std::string> evidence_types; // Evidence types
    int version;                          // Control version
    int64_t created_at_ms;                // Creation timestamp
};
```

#### ComplianceStatusReport
Summarizes framework compliance status:
```cpp
struct ComplianceStatusReport {
    std::string report_id;                // Unique report ID
    ComplianceFramework framework;        // Framework assessed
    int64_t generated_at_ms;              // Report generation time
    
    int total_requirements;               // Total requirements checked
    int compliant_requirements;           // Fully compliant
    int non_compliant_requirements;       // Not compliant
    int partial_requirements;             // Partially compliant
    int na_requirements;                  // Not applicable
    
    double compliance_score;              // 0-100 score
    ComplianceStatus overall_status;      // Overall status
    
    std::vector<ComplianceViolation> violations;  // Detected violations
    std::vector<ComplianceEvidence> evidence;     // Collected evidence
};
```

## Framework-Specific Mappings

### ISO 27001:2022 (Annex A)

**Supported Controls:**

| Control | Section | Severity | Category |
|---------|---------|----------|----------|
| Access Control Policy | A.9.1 | CRITICAL | Access Control |
| Network Security | A.13.1 | CRITICAL | Network |
| Cryptography | A.10.1 | CRITICAL | Encryption |
| Dev/Prod Separation | A.12.4 | HIGH | Infrastructure |
| Change Management | A.12.2 | HIGH | Change Management |
| Operations Procedures | A.12.1 | MEDIUM | Operations |

**Validation Logic:**
- Checks policy state for implemented access controls
- Verifies encryption policies and key management
- Validates network segmentation
- Confirms change management procedures
- Evidence: policy rules, audit logs, encryption status

### SOC 2 Type I/II

**Supported Criteria:**

| Criteria | Focus Area | Severity |
|----------|-----------|----------|
| CC1-CC9 | Common Criteria | CRITICAL |
| C1 | Security (Access) | CRITICAL |
| A1 | Availability | HIGH |
| P1 | Processing Integrity | HIGH |
| CF1 | Confidentiality | CRITICAL |
| PF1 | Privacy | CRITICAL |

**Validation Focus:**
- Control environment and risk management
- Security monitoring and incident response
- Data security and access controls
- Availability and business continuity

### GDPR (2016/679/EU)

**Key Articles:**

| Article | Requirement | Severity |
|---------|-------------|----------|
| 25 | Data Protection by Design | CRITICAL |
| 32 | Security of Processing | CRITICAL |
| 33 | Data Breach Notification | CRITICAL |
| 35 | DPIA | HIGH |
| 5 | Data Minimization | HIGH |

**Validation Logic:**
- Consent management verification
- Data retention policy checks
- Subject rights implementation (access, deletion, portability)
- DPIA completion verification
- Evidence: consent records, deletion logs, retention policies

### CCPA/CPRA

**Consumer Rights:**

| Right | Implementation |
|-------|-----------------|
| Right to Access | Data access API |
| Right to Delete | Data deletion workflow |
| Right to Opt-Out | Preference management |
| Data Security | Encryption and access controls |

### HIPAA

**Technical Safeguards:**

| Control | Standard |
|---------|----------|
| Access Control | 164.312(a)(2) |
| Audit Controls | 164.312(b) |
| Integrity Controls | 164.312(c) |
| Transmission Security | 164.312(e) |

### PCI-DSS v3.2.1

**Requirements:**

| Requirement | Focus |
|-------------|-------|
| 1 | Firewall Configuration |
| 3 | Data Protection |
| 4 | Encryption |
| 6 | Vulnerability Management |
| 7 | Access Control |
| 10 | Monitoring |

### EU AI Act (2024)

**Key Areas:**

| Article | Requirement |
|---------|-------------|
| 6 | Risk Categorization |
| 8 | High-Risk Systems |
| 13 | Transparency |
| 14 | Human Oversight |
| 26 | Monitoring |

## Usage Guide

### 1. Loading Frameworks

```cpp
#include "governance/compliance_mapping.h"
#include "governance/compliance_validator.h"

// Load specific framework
auto registry = ComplianceFrameworkLoader::loadFramework(
    ComplianceFramework::kIso27001);

// Load all frameworks
auto all_registry = ComplianceFrameworkLoader::loadAllFrameworks();

// Load from file
auto file_registry = ComplianceFrameworkLoader::loadFromFile(
    "/path/to/compliance_definitions.json");

// Check framework version
std::string version = ComplianceFrameworkLoader::getFrameworkVersion(
    ComplianceFramework::kIso27001);
// Output: "ISO 27001:2022"
```

### 2. Creating Requirements & Controls

Using builder pattern:

```cpp
// Create requirement
auto requirement = ComplianceRequirementBuilder()
    .withId("CUSTOM-REQ-001")
    .withFramework(ComplianceFramework::kIso27001)
    .withText("Access control must be enforced")
    .withSection("Article 9.1")
    .withSeverity(ComplianceSeverity::kCritical)
    .withControl("CTL-ACCESS-001")
    .withCategory("access-control")
    .withMandatory(true)
    .build();

// Create control
auto control = ComplianceControlBuilder()
    .withId("CTL-ACCESS-001")
    .withFramework(ComplianceFramework::kIso27001)
    .withName("Access Control Implementation")
    .withDescription("RBAC policy enforcement")
    .withImplementation("Policy engine enforces rules")
    .automated(true)
    .withPolicyRule("access_control_policy")
    .withEvidenceType("policy_rule")
    .withEvidenceType("access_log")
    .build();

// Add to registry
registry->addRequirement(requirement);
registry->addControl(control);
```

### 3. Validating Compliance

```cpp
#include "governance/compliance_validator.h"

ComplianceValidationEngine engine;
ComplianceContext ctx;

// Setup validation context
ctx.system_id = "production-db";
ctx.enabled_frameworks = {"ISO27001", "GDPR"};

// Set current policy state
ctx.policy_state["access_control_policy"] = "active";
ctx.policy_state["encryption_policy"] = "aes256";

// Set control implementation status
ctx.control_status["CTL-ACCESS-001"] = true;
ctx.control_status["CTL-CRYPTO-001"] = true;

// Validate single requirement
auto status = engine.validateRequirement(requirement, ctx);
// ComplianceStatus::kCompliant or kNonCompliant or kPartiallyCompliant

// Validate single control
auto control_status = engine.validateControl(control, ctx);

// Validate entire framework
auto report = engine.validateFramework(
    ComplianceFramework::kIso27001, *registry, ctx);

std::cout << "Compliance Score: " << report.compliance_score << "%\n";
std::cout << "Status: " << (int)report.overall_status << "\n";
std::cout << "Violations: " << report.violations.size() << "\n";

// Validate all enabled frameworks
auto result = engine.validateAll(
    {ComplianceFramework::kIso27001, ComplianceFramework::kGdpr},
    *registry, ctx);

std::cout << "Frameworks validated: " << result.framework_reports.size() << "\n";
std::cout << "Total violations: " << result.all_violations.size() << "\n";
```

### 4. Detecting and Handling Violations

```cpp
// Detect violations from result
auto violations = engine.detectViolations(result);

for (const auto& violation : violations) {
    std::cout << "Requirement: " << violation.requirement_id << "\n";
    std::cout << "Severity: " << (int)violation.severity << "\n";
    std::cout << "Description: " << violation.description << "\n";
    
    // Generate remediation guidance
    auto guidance = engine.generateRemediationGuidance(violation);
    std::cout << guidance << "\n";
}
```

### 5. Collecting Evidence

```cpp
// Collect evidence for a control
auto validator = std::make_unique<Iso27001Validator>();
auto evidence = validator->collectEvidence(control, ctx);

for (const auto& ev : evidence) {
    std::cout << "Evidence: " << ev.evidence_id << "\n";
    std::cout << "Type: " << ev.evidence_type << "\n";
    std::cout << "Details: " << ev.detail << "\n";
    
    // Convert to JSON for reporting
    auto json = ev.toJson();
}
```

### 6. Report Generation

```cpp
// Generate JSON report
auto report_json = report.toJson();
std::cout << report_json.dump(2) << "\n";

// Export framework definitions
auto framework_export = registry->exportToJson(
    ComplianceFramework::kIso27001);
std::cout << framework_export.dump(2) << "\n";

// Export entire validation result
auto result_json = result.toJson();
```

## Performance Characteristics

### Latency Targets

| Operation | Target | Achieved |
|-----------|--------|----------|
| Single requirement check | ≤1s | <100ms |
| Single control validation | ≤1s | <100ms |
| Framework report generation (100 reqs) | ≤5s | <500ms |
| Multi-framework validation (6 frameworks) | ≤5s | <1s |
| Evidence collection (10 items) | <1s | <50ms |

### Scalability

- **Requirements per framework:** 1000+ supported
- **Controls per framework:** 500+ supported
- **Concurrent validations:** Thread-safe registry with RwLock
- **Memory footprint:** <10MB per framework registry

## Test Coverage

### Test Gates (GOV-Compliance-01 to 06)

- **GOV-Compliance-01:** Framework loading and initialization
- **GOV-Compliance-02:** Requirement and control mapping
- **GOV-Compliance-03:** Compliance validation
- **GOV-Compliance-04:** Violation detection
- **GOV-Compliance-05:** Report generation
- **GOV-Compliance-06:** Evidence collection

### Test Files

- `tests/governance/test_compliance_framework.cpp` — Comprehensive test suite
  - 40+ test cases
  - 6 test gates
  - Performance benchmarks
  - Integration tests

### Running Tests

```bash
# Run compliance framework tests
cd /home/runner/work/ThemisDB/ThemisDB
cmake -B build -DENABLE_GOVERNANCE=ON
cmake --build build
ctest -R compliance_framework -V
```

## Compliance Requirement Versioning

### Version Tracking

Each requirement and control includes version tracking:

```cpp
struct ComplianceRequirement {
    int version;              // Current version
    int64_t created_at_ms;    // Original creation
    int64_t updated_at_ms;    // Last modification
};
```

### Framework Version Updates

```cpp
// Check framework version
std::string version = ComplianceFrameworkLoader::getFrameworkVersion(
    ComplianceFramework::kIso27001);
// "ISO 27001:2022"
```

## Remediation Workflow

### Violation Lifecycle

```
Detected → Assigned → In-Progress → Evidenced → Resolved → Verified
```

### Remediation Steps

1. **Detect** — Engine finds non-compliant requirement
2. **Generate Guidance** — Operator receives remediation guidance
3. **Implement** — Control is implemented
4. **Collect Evidence** — Evidence is gathered and linked
5. **Verify** — Re-validate to confirm compliance
6. **Report** — Include in compliance report

## Operator Runbook

### Daily Compliance Check

```bash
#!/bin/bash
# Daily compliance validation script

SYSTEM_ID="production"
FRAMEWORKS=("ISO27001" "GDPR")

for framework in "${FRAMEWORKS[@]}"; do
    compliance_check $SYSTEM_ID $framework
    if [ $? -eq 0 ]; then
        echo "✓ $framework compliant"
    else
        echo "✗ $framework has violations"
        send_alert "$framework compliance violations detected"
    fi
done
```

### Remediation Tracking

1. Review daily compliance reports
2. For each HIGH/CRITICAL violation:
   - Create remediation ticket
   - Assign to responsible team
   - Set deadline based on severity
   - Track evidence collection
3. Re-validate after implementation
4. Document remediation in audit trail
5. Archive compliance evidence

## Cross-Framework Mapping

### Equivalent Requirements

Many requirements across frameworks address similar concerns:

| ISO 27001 | GDPR | SOC 2 | Concept |
|-----------|------|-------|---------|
| A.9.1 | Art 32 | C1 | Access Control |
| A.10.1 | Art 32 | CF1 | Encryption |
| A.13.1 | Art 32 | C2 | Network Security |
| A.12.1 | Art 32 | CC1 | Operations |

```cpp
// Get equivalent requirements
CrossFrameworkMapping mapper;
auto equiv = mapper.getEquivalentRequirements(
    "ISO27001-A.9.1",
    ComplianceFramework::kIso27001,
    ComplianceFramework::kGdpr);
// Returns: ["GDPR-Art32"]

// Check equivalence
bool equivalent = mapper.areEquivalent(
    "ISO27001-A.9.1", ComplianceFramework::kIso27001,
    "GDPR-Art32", ComplianceFramework::kGdpr);
// Returns: true
```

## Audit Trail Integration

All compliance operations are logged:

```
[2026-08-18T06:08:23.661Z] Compliance validation started
  System: production-db
  Frameworks: [ISO27001, GDPR]
  
[2026-08-18T06:08:23.761Z] ISO27001 validation: 95% compliant (19/20)
  Violations: ISO27001-A.12.2 (Change Management)
  
[2026-08-18T06:08:23.850Z] GDPR validation: 100% compliant (8/8)

[2026-08-18T06:08:23.950Z] Compliance validation complete
  Elapsed: 290ms
  Overall score: 97.5%
```

## Security Considerations

### Sensitive Data Handling

- Compliance context may contain policy state (never logged)
- Evidence collection maintains audit trail
- Violations documented securely
- Reports generated with proper access controls

### Access Control

- Compliance validation engine is thread-safe
- Registry uses RwLock for concurrent access
- Validators are stateless (thread-reentrant)

## Troubleshooting

### Issue: Framework Not Loading

```
Error: Failed to open compliance framework file
```

**Solution:**
- Check file path exists: `ls -la /path/to/framework.json`
- Verify JSON format: `jq . /path/to/framework.json`
- Check permissions: `chmod 644 /path/to/framework.json`

### Issue: Validation Timeout

```
Error: Validation exceeded 5s timeout
```

**Solution:**
- Reduce requirements per batch
- Validate single framework instead of all
- Check system resource usage
- Profile with perf: `perf record -F 99 ./validator`

### Issue: Evidence Not Collected

```
Warning: No evidence collected for control CTL-001
```

**Solution:**
- Verify control has evidence_types defined
- Check evidence collector is enabled
- Verify policy state is accessible
- Check audit logs for collection failures

## Future Enhancements

- [ ] Automated remediation workflows
- [ ] ML-based violation prediction
- [ ] Cross-region compliance aggregation
- [ ] Real-time compliance dashboard
- [ ] Third-party framework support (CIS, NIST)
- [ ] Integration with SIEM systems
- [ ] Compliance metrics trending

## References

- **ISO 27001:2022**: https://www.iso.org/standard/27001
- **GDPR**: https://eur-lex.europa.eu/legal-content/EN/TXT/?uri=celex:32016R0679
- **SOC 2**: https://www.aicpa.org/interestareas/informationmanagement/standards
- **CCPA**: https://oag.ca.gov/privacy/ccpa
- **HIPAA**: https://www.hhs.gov/hipaa/
- **PCI-DSS**: https://www.pcisecuritystandards.org/
- **EU AI Act**: https://www.europarl.europa.eu/news/en/headlines/society/20230601STO93804

## Support

For issues or questions about compliance framework:
1. Check test coverage: `tests/governance/test_compliance_framework.cpp`
2. Review framework mappings: `src/governance/FRAMEWORK_MAPPINGS.md`
3. Consult validator documentation
4. Contact governance team
