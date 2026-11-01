---
Author: ThemisDB Contributors
Created: 2026-10-31
Creation_Date_Note: Creation date of this rewritten page; the original file creation date is unknown.
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: themisdb-doc-orchestrator
Human_Review: false
Human_Author: unknown
Source_Level: level1
SOT_Domain: module-behavior
Canonical_References:
  - tests/security/CMakeLists.txt
  - tests/CMakeLists.txt
  - tests/security/test_security_negative_integration.cpp
  - tests/security/test_security_evidence_collector.cpp
  - tests/security/test_security_wavec_production_validation_focused.cpp
  - SECURITY.md
Related_Issue: "#7379"
Related_Milestone: DOC-WEEKLY-2026-W44
---

Exact model backend not verifiable from final document text alone.

# Security tests: scope and limits

This directory contains tests for security-related interfaces and code paths.
Source names include negative-integration, evidence-collector, input-validation,
FIPS, and Wave-C production-validation cases. The test target rules are defined
in [`CMakeLists.txt`](CMakeLists.txt) and [`tests/CMakeLists.txt`](../CMakeLists.txt).

Tests are verification artifacts, not runtime controls. Their existence does not
establish that a control is enabled in a deployment, that every relevant caller
is covered, or that the product is certified or production-ready. For current
module status, consult [`src/security/ROADMAP.md`](../../src/security/ROADMAP.md)
and the active module evidence. Report vulnerabilities through the project
[`SECURITY.md`](../../SECURITY.md).
