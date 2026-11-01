---
Author: ThemisDB Contributors
Created: 2026-06-01
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
---

# Public Header Future Enhancements - user_storage_encrypted

Exact model backend not verifiable from final document text alone.

This page records possible header/API work only. It does not declare approved
commitments. Runtime behavior and module-level work belong in
[`../../src/user_storage_encrypted/FUTURE_ENHANCEMENTS.md`](../../src/user_storage_encrypted/FUTURE_ENHANCEMENTS.md)
and [`../../src/user_storage_encrypted/ROADMAP.md`](../../src/user_storage_encrypted/ROADMAP.md).

## Candidate API work

- Add focused examples for the actual `themis::plugins::user_storage` public
  types and result/error handling.
- Document platform support per declaration; POSIX-specific helpers such as
  `PipeGuard` and `TimedFileOperation` must not be presented as portable.
- Review the mismatch between the operational API and
  `themis::user_storage_encrypted` contract types before promising stability
  or guarantees.
- Add compile-only checks for public headers and supported compiler/platform
  combinations.
- Review deprecation, ABI, and source-compatibility policy with maintainers
  before introducing new annotations or compatibility promises.

No C++20 Concepts, module-interface wrapper, header-only implementation, or
Graph-layer mapping is established by the current module sources. Such work
requires explicit architectural approval.

## References

- [Header inventory](README.md)
- [Header architecture](ARCHITECTURE.md)
- [Canonical module roadmap](../../src/user_storage_encrypted/ROADMAP.md)
