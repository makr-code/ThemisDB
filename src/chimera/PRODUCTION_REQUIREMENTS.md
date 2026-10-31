---
Author: unknown (original author not verified)
Created: 2026-10-31
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: AI documentation assistant
Human_Review: false
Human_Author: unknown
---

Exact model backend not verifiable from final document text alone.

# Chimera production-use assessment boundary

**Purpose / audience:** prevent module documentation from being mistaken for an operational sign-off. This document states evidence required before a deployment decision; it does not assert production readiness.

## Evidence required before production use

- Identify the exact adapter and compile-time feature definitions in the deployed binary.
- Confirm dependency versions and connection/authentication/TLS configuration against the chosen backend's requirements.
- Validate connection, timeout, retry, resource-limit, and error behavior for that adapter; do not assume configuration options are honored just because an interface accepts them.
- Establish whether operations are simulated or engine-backed. `THEMIS_BUILD_CHIMERA` defaults to `OFF`, and its enabled CMake source list includes external CHIMERA sources plus the local ThemisDB adapter.
- Verify that the required focused tests and deployment-specific integration tests actually ran for the same configuration.
- Obtain a security and operational review of the real deployment and record its evidence separately.

## Known source boundaries

The local MongoDB, Neo4j, and Qdrant code has compile-time conditional driver paths. `THEMISDB_ENGINE_AVAILABLE` gates engine-specific ThemisDB code. Neither condition alone proves a particular production feature is configured or supported. The checked-in benchmark stubs are not production measurements.

## References

`CMakeLists.txt`, `cmake/ChimeraAdapters.cmake`, `include/chimera/*.hpp`, `src/chimera/*.cpp`, [`SECURITY.md`](SECURITY.md), [`ARCHITECTURE.md`](ARCHITECTURE.md).

Issue/milestone: #7336 / DOC-WEEKLY-2026-44.
