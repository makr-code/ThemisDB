# Validated Remediation Backlog

## Prioritized Module Order

- llm
- server
- sharding

## Batch Exit Criteria

- No new CRITICAL findings
- Tests green (`release_critical` plus Wave 5/6 regressions)
- Failure/recovery semantics documented
- Wave 7 baseline stable; Wave 8/Fault-Injection as follow-up sign-off

## server

- [CRITICAL] smart_ptr_misuse (src/server/http_server.cpp:2541) — Raw new without immediate smart pointer wrapping

## aql

- [CRITICAL] new_without_delete (src/aql/docs_assistant_functions.cpp:554) — CRITICAL severity suggests real issue
- [HIGH] legacy_duplication (src/aql/llm_aql_handler.cpp:1611) — Explicitly marked legacy code

## themis

- [CRITICAL] audit_logging (src/themis/license_info.cpp:209) — CRITICAL severity suggests real issue

