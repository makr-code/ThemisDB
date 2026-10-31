---
Author: "ThemisDB Team"
Created: "2026-05-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Security - AQL Module

Report vulnerabilities through the project-level [`SECURITY.md`](../../SECURITY.md)
process.

## Security scope and trust boundary

`src/aql/` contains assistance paths that handle natural-language input,
provider-generated query text, optional schema context, and adapter/tool
callbacks. The AQL module does not replace host authentication, query-execution
authorization, or policy enforcement.

## Source-supported controls

- `LLMAQLHandler` sanitizes and bounds prompt inputs, validates generated AQL
  using a configured parser service when available, and uses a weaker
  string-level validator fallback otherwise.
- Translation applies collection-access and schema-scope checks before
  returning generated output. These module checks are not a substitute for
  authorization at the final execution boundary.
- `LLMAQLEmbeddingBridge` converts handler exceptions to an empty embedding
  with a warning; the few-shot library can fall back to lexical ranking.
- `AQLConversationContext` applies configured history bounds and eviction.
- Agent tools are application-provided callbacks; the host must authorize the
  action and validate its arguments.

These are source-visible controls, not a finding that every caller or
deployment is fully protected. Parser acceptance does not prove semantic
correctness, intent fidelity, or permission.

## Security follow-up

- **TODO:** Reconcile the current handler behavior with the frozen assistance
  contract; see [`API.md`](API.md).
- **TODO:** Verify authorization coverage through each host execution route and
  every registered agent tool.
- Preserve bounded input/context behavior and avoid exposing raw prompts,
  credentials, or sensitive query values in diagnostics.

## Canonical implementation references

- [`llm_aql_handler.cpp`](llm_aql_handler.cpp)
- [`aql_conversation_context.cpp`](aql_conversation_context.cpp)
- [`llm_aql_embedding_bridge.cpp`](llm_aql_embedding_bridge.cpp)
- [`include/aql/llm_aql_handler.h`](../../include/aql/llm_aql_handler.h)
- [`include/aql/aql_agent.h`](../../include/aql/aql_agent.h)
- [`TRUST.md`](TRUST.md)
