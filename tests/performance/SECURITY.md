---
Author: "ThemisDB Team"
Created: "2026-04-17"
Last Updated: "2026-10-31"
Status: "active"
Source_Level: "level1"
SOT_Domain: "security"
Canonical_Sources:
  - "tests/performance/CMakeLists.txt"
  - "tests/performance/*.cpp"
  - "SECURITY.md"
  - "src/performance/SECURITY.md"
Related_Issue: "#7366"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Security Scope — Performance Tests

This directory contains C++ test programs, not a production authentication,
authorization, or security-control boundary. Its CMake targets link test
frameworks and the performance implementation; their presence does not establish
security-review coverage or a passing security test suite.

Do not place credentials, production query data, or sensitive benchmark inputs
in test fixtures or retained test output. Review test data and artifacts under
the repository's root [`SECURITY.md`](../../SECURITY.md) process. Runtime
security and privacy considerations for the performance module are documented
in [`../../src/performance/SECURITY.md`](../../src/performance/SECURITY.md)
and [`../../src/performance/TRUST.md`](../../src/performance/TRUST.md).

Exact model backend not verifiable from final document text alone.
