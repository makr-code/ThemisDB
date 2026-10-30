---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# LLM-L-Wiki API status

## Supported interface

No public header, exported symbol, service endpoint, or module-owned API test
was found for `llm_l_wiki` in the checked-out source/build inventory. This
module therefore documents **no supported callable API**. The filename
`wikipedia/wiki_workspace_orchestrator.cpp` does not provide an interface; it is
empty.

`include/wikipedia/wiki_workspace_orchestrator.h` and
`include/llm_wiki/llm_wiki_plugin_interface.h` are not under an
`llm_l_wiki` API namespace and must not be presented as this module's contract.
See the separate [`llm_wiki` implementation`](../llm_wiki/README.md) only when
working on that module.

## Requirements for a future API proposal

Before consumers integrate, specify:

- operation names and ownership (public SDK, internal C++ call, or service);
- required and optional inputs, validation, size bounds, and empty-value
  behavior;
- output schema, ordering, provenance, and ownership/lifetime;
- error/status mapping for invalid input, unavailable dependencies, timeout,
  cancellation, and partial results;
- thread-safety, workspace isolation, and side effects;
- compatibility/versioning policy and focused contract tests.

Do not publish guessed signatures or compatibility shims. Any public API must
be backed by a header contract, implementation, and tests in the same module.

## References

- [Architecture boundary](ARCHITECTURE.md)
- [Implementation inventory](IMPLEMENTATION.md)
- [Module documentation requirements](../../.github/MODULE_DEVELOPER_DOCUMENTATION_GUIDELINES.md)

> AI-assisted documentation. Exact model backend not verifiable from final document text alone.
