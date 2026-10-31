---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "OpenAI API assistant (model unknown)"
Human_Review: false
Human_Author: "unknown"
---

> Original creation date was not recorded; this provenance block was first recorded on 2026-10-31.
> Exact model backend not verifiable from final document text alone.

Source level: 1
SOT domain: security
Canonical references: `src/query/SECURITY.md`, `src/query/PRODUCTION_REQUIREMENTS.md`, `include/query/`, `tests/query/CMakeLists.txt`
Related issue: #7371
Related milestone: `DOC-WEEKLY-2026-44`

# Query test security scope

Tests in this directory can provide regression evidence for security-relevant
query behavior; they are not enforcement mechanisms and do not replace the
query implementation, caller-provided authorization, or the repository
security reporting process.

Relevant test sources include `test_query_masking_policy.cpp`,
`test_query_safety_fixes.cpp`, `test_query_parser_scope_validation.cpp`,
`test_query_federation_scope_safety.cpp`, and
`test_query_resource_limits.cpp`. Their presence does not establish that a
specific CMake target was built or passed; confirm registration in
`tests/query/CMakeLists.txt` or `tests/CMakeLists.txt` and consult run evidence.

For current requirements and implementation boundaries, use
[`../../src/query/SECURITY.md`](../../src/query/SECURITY.md) and
[`../../src/query/PRODUCTION_REQUIREMENTS.md`](../../src/query/PRODUCTION_REQUIREMENTS.md).
Report security findings according to the root [`../../SECURITY.md`](../../SECURITY.md).
