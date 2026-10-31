---
Author: "ThemisDB Team"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# AQL Assistance Production Requirements

## Purpose and evidence boundary

This page records operational requirements for applications deploying AQL
assistance. The requirements are policy guidance; their presence here does not
prove that every application path or deployment enforces them. Verify
implementation and host integration separately.

The AQL assistance module is not the core query executor and is not, by itself,
an authorization authority. The implementation exposes configurable input
validation limits, parser validation, collection access/scope checks, timeouts,
retry settings, and circuit-breaker configuration. Exact values and behavior
depend on the configured handler and host application.

## Requirements for a deployment

- **MUST:** Authenticate the caller and authorize the final query at the host
  execution boundary. Do not treat generated AQL, parser acceptance, confidence,
  or supplied schema context as permission.
- **MUST:** Keep input bounds, validation mode, retry count, and timeout policy
  explicit and reviewed for the deployment.
- **MUST:** Treat user prompts, provider output, schema context, and tool
  arguments as untrusted; do not execute partial streamed output.
- **MUST:** Use reviewed application callbacks and authorize each operation
  exposed through an agent/tool integration.
- **MUST:** Protect provider credentials and avoid retaining raw prompts or
  sensitive query values in logs unless covered by an approved incident process.
- **MUST NOT:** Disable application authorization or bypass validation to
  recover provider availability.

## Configuration and build boundary

Relevant public configuration is declared in
[`include/aql/llm_aql_handler.h`](../../include/aql/llm_aql_handler.h),
[`include/aql/aql_conversation_context.h`](../../include/aql/aql_conversation_context.h),
and the applicable adapter headers. No universal production-mode environment
variable, admin command, or set of production defaults is established by these
module sources.

Build inclusion is feature/configuration dependent; consult the selected CMake
configuration and `cmake/ModularBuild.cmake`. Do not infer that a particular
provider or bridge is compiled or enabled solely because its public header
exists.

## Verification checklist

- [ ] Host authentication and execution authorization are verified in the
  deployed request path.
- [ ] Handler validation, input limits, retry, timeout, context, and
  circuit-breaker configuration are reviewed.
- [ ] Optional parser/provider/embedding services and their failure behavior are
  verified for the deployed build.
- [ ] Logging, metrics, access, and retention controls are verified in the
  host application.
- [ ] Focused tests and deployment-specific checks have been run and recorded.

## Source references

- [`llm_aql_handler.h`](../../include/aql/llm_aql_handler.h)
- [`aql_conversation_context.h`](../../include/aql/aql_conversation_context.h)
- [`aql_agent.h`](../../include/aql/aql_agent.h)
- [`SECURITY.md`](SECURITY.md)
- [`TRUST.md`](TRUST.md)
