# Security - AQL Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

Report vulnerabilities via project-level SECURITY.md.

## Security Scope

Security in the AQL module focuses on safe handling of generated-query workflows, bounded assistance behavior, and resilient processing of user-derived inputs in translation and helper paths.

## Threat Model

| Threat | Current Mitigation Surface |
|---|---|
| prompt or input-shaping abuse in translation flows | sanitization and structured translation/validation stages in handler paths |
| unsafe generated-query propagation | validation and issue-reporting stages before downstream use |
| unbounded context or assistance growth | bounded context and helper constraints in conversation/tooling paths |
| unsafe bridge integrations | explicit module bridge surfaces with structured fallback behavior |
| degraded capability misuse | fail-closed or warning-backed degraded modes with diagnostics |

## Implemented Security Controls

- translation and assistance paths are separated from core execution backends.
- generated-query handling includes structured validation and scoring/annotation support.
- helper components provide bounded context and explicit error propagation surfaces.

## Security Follow-ups

- continue hardening of adversarial input patterns in translation and assistant routes.
- maintain strict boundary checks for optional integration surfaces.
- preserve actionable diagnostics for security-relevant failure classes.

## Sourcecode Verification (Module: aql/security)

- Verified files:
  - src/aql/llm_aql_handler.cpp
  - src/aql/aql_query_validator.cpp
  - src/aql/aql_conversation_context.cpp
  - src/aql/docs_assistant_functions.cpp
  - src/aql/classify_bridge.cpp
  - src/aql/llm_aql_embedding_bridge.cpp
- Verified controls:
  - structured translation/validation flow boundaries
  - bounded context/helper behavior surfaces
  - explicit bridge and fallback handling paths