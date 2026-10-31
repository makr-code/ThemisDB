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

# Chimera security boundaries

**Purpose / audience:** identify security-relevant boundaries for maintainers. This is not a security audit or assurance that controls are complete.

## Boundaries requiring configuration-specific review

- External connections, credentials, TLS, authentication, and authorization depend on the selected adapter and its compiled driver path. Do not infer these controls from the common adapter interface.
- `MongoDBAdapter` masks credentials in its stored connection string; that alone is not a comprehensive secret-handling review. Neo4j and Qdrant have separate connection implementations and compile-time guards.
- `THEMISDB_ENGINE_AVAILABLE` controls engine-specific dispatch code. Simulation behavior is not a substitute for testing the production engine path.
- Retry policies can amplify load if incorrectly configured. Current `RetryExecutor::should_retry` does not inspect the concrete error code, so selective retry semantics are not established.
- `THEMIS_BUILD_CHIMERA` defaults to `OFF`; enabling the option does not establish a secure deployment configuration.

## Required review before deployment

Review the exact build definitions, external dependency versions, connection/authentication settings, secret redaction, error/log output, resource/time limits, and adapter-specific failure behavior. Run security and integration validation for the deployed configuration and record it separately. Report vulnerabilities through the project-level [`SECURITY.md`](../../SECURITY.md).

## Evidence

`include/chimera/{mongodb_adapter,neo4j_adapter,qdrant_adapter,retry_executor,themisdb_adapter}.hpp`, corresponding `src/chimera/*.cpp`, `CMakeLists.txt`, `cmake/ChimeraAdapters.cmake`.

Issue/milestone: #7336 / DOC-WEEKLY-2026-44.
