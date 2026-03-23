<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Process Module

## Scope

Covers all public headers in `include/process/`. Implementation hardening in `../../src/process/`.

## Threat Model

| Threat | Impact | Mitigation |
|---|---|---|
| XXE via malicious BPMN/EPK XML | High — file read / SSRF | `BpmnSerializer` and `EpkSerializer` use libxml2 with `XML_PARSE_NONET` and entity expansion disabled |
| YAML bomb / billion-laughs in VCC-VPB | High — CPU/memory DoS | `VccVpbImporter` uses a safe YAML parser with node count and alias depth limits |
| Process definition injection (stored XSS) | Medium — UI injection | `ProcessModelManager` validates UTF-8 and strips HTML entities before storage |
| RocksDB key collision | Medium — data overwrite | `ProcessModelManager` uses content-addressed version suffix; overwrite requires explicit force flag |
| LLM prompt injection via process descriptor | Medium — LLM manipulation | `LlmProcessDescriptor` escapes special tokens before constructing prompts |
| Unauthorized process graph retrieval | Medium — information disclosure | `ProcessGraphRag` checks read permissions via RBAC before returning results |
| Graph-RAG embedding data leakage | Low — partial content exposure | Embeddings are stored separately from raw process content; raw access requires owner role |

## Security Controls

1. **XXE disabled** — `BpmnSerializer` sets `XML_PARSE_NONET | XML_PARSE_NOENT` on all document loads.
2. **YAML depth limit** — `VccVpbImporter` rejects documents with alias depth > 10 or node count > 100,000.
3. **Schema validation** — all process definitions validated against JSON schema before `ProcessModelManager` write.
4. **RBAC on retrieval** — `ProcessGraphRag` and `ProcessModelManager` enforce read/write role checks.
5. **Immutable versions** — `ProcessModelManager` never overwrites existing version keys; creates new versions only.
6. **Prompt token escaping** — `LlmProcessDescriptor` escapes `<|`, `[INST]`, and `###` tokens.

## Known Limitations

- BPMN external sub-process references (`<callActivity>` with absolute URIs) are resolved only against a local allowlist — not yet validated against all BPMN 2.0 conformance scenarios.
