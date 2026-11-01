---
Author: "ThemisDB Contributors"
Created: "2026-08-18"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Related: "Issue #7349"
---

Exact model backend not verifiable from final document text alone.

# Governance Compliance Framework Interfaces

This page documents module software surfaces; it does not claim that ThemisDB,
the governance module, or an emitted report is certified or legally compliant.
The prior “production-ready” label, detailed regulatory control mappings,
performance/scalability numbers, operational shell commands, and compliance
outcome examples are withdrawn unless independently verified against current
regulatory sources and deployment evidence.

## Current module surface

- `ComplianceFramework` declares EU AI Act, SOC 2 Type I/II, ISO 27001, GDPR,
  CCPA, HIPAA, and PCI-DSS identifiers.
- `ComplianceFrameworkRegistry` stores in-process requirement/control
  definitions and exposes add/get/list/count and JSON import/export operations.
- `ComplianceFrameworkLoader` exposes built-in framework loading, file loading,
  validation, and framework-version lookup.
- `ComplianceValidationEngine` and framework-specific validators accept a
  caller-provided `ComplianceContext` and return structured result/report
  types.
- `ComplianceReporter` and `compliance_reporting.h` expose separate reporting
  and analysis interfaces; their output depends on supplied policy/evidence
  state.

Consult the public headers for exact inputs, result fields, and supported
operations:
[`compliance_framework.h`](../../include/governance/compliance_framework.h),
[`compliance_mapping.h`](../../include/governance/compliance_mapping.h),
[`compliance_validator.h`](../../include/governance/compliance_validator.h),
[`compliance_reporter.h`](../../include/governance/compliance_reporter.h), and
[`compliance_reporting.h`](../../include/governance/compliance_reporting.h).

## Interpretation limits

- Module-provided mappings and result scores are software data, not legal
  interpretations or independent assessments.
- Completeness depends on the loaded definitions, caller-provided context,
  configured control status, and evidence.
- The repository implementation does not prove that every relevant production
  control is integrated or that reports are retained durably.
- Treat previous hand-written mapping tables, CLI examples, numeric
  performance claims, and test-count assertions as stale.

## Test and build evidence

`tests/governance/test_compliance_framework.cpp` is included by the governance
focused-test CMake loop as target
`module_governance_test_compliance_framework_focused` and CTest name
`test_compliance_framework_governance_FocusedTests`; CMake adds the
framework/mapping/validator implementation files to that target. For an
already configured build, the intended commands are:

```bash
cmake --build <build-dir> --target module_governance_test_compliance_framework_focused
ctest --test-dir <build-dir> -R '^test_compliance_framework_governance_FocusedTests$' --output-on-failure
```

Confirm the target/name in that build's CTest listing first. These are
instructions only; this documentation update did not run them.

There is no module-specific `ENABLE_GOVERNANCE` CMake option established here.
Use repository CMake options and configuration documented at the root rather
than the old `cmake -B build -DENABLE_GOVERNANCE=ON` example.

## References

- [Compliance headers](../../include/governance/README.md)
- [Governance public API](API.md)
- [Test registration](../../tests/governance/CMakeLists.txt)
- [Trust boundaries](TRUST.md)
