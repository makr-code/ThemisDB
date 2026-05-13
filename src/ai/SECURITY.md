> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-05-13 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — AI Module

> Report vulnerabilities via the project-level [SECURITY.md](../../../SECURITY.md).

## Threat Model

| Threat | Mitigation |
|--------|-----------|
| Prompt injection via crafted `description` field | Length limit (≤ 8192 chars) enforced in `validatePrompt()`; content sanitisation planned for Phase 2 |
| Sensitive prompt content leaked to logs | `spdlog::debug` output truncated to 80 characters (`prompt.description.substr(0, 80)`) |
| Malicious LLM response executing arbitrary code | Security-sandbox pipeline planned for Phase 2 — generated code must pass sandboxed build before returning `GeneratedPlugin` |
| SSRF via `Config::llm_endpoint` | Phase-2 HTTP client must validate endpoint against an allow-list; no network calls are made in Phase 1 |
| Untrusted `GeneratedPlugin` artefacts propagated to production | Fail-closed: Phase 1 returns no `GeneratedPlugin`; Phase-2 output validation (manifest completeness, code structure) required before use |

## Security Controls (Phase 1)

- `validatePrompt()` rejects empty or oversized prompt descriptions before any
  processing occurs.
- No HTTP client or external network call is made in the current Phase-1 implementation.
- Log output is bounded and truncated to prevent prompt content leakage in log sinks.
- The module depends only on `spdlog`, `utils/error_registry`, and `utils/expected`;
  no third-party network or parsing libraries are linked.

## Phase-2 Security Requirements

The following controls MUST be implemented when the LLM endpoint is wired (Phase 2):

- **Allow-list validation** of `Config::llm_endpoint` before making HTTP requests
  (prevent SSRF to internal services).
- **Response size limit** on the LLM API response (reject responses exceeding a
  configurable maximum to prevent memory exhaustion).
- **Structured JSON schema validation** of the LLM response before populating
  `GeneratedPlugin` fields.
- **Security sandbox** for any generated code artefacts — generated headers and
  implementation files must be compiled and analysed in an isolated environment
  before being returned to the caller.
- **Redaction of full prompt content** from all non-DEBUG log levels; no sensitive
  `required_capabilities` or `dependencies` data in persistent logs.
- **Retry budget limit** — bounded retries with backoff to prevent unintentional
  denial-of-service against the LLM endpoint.

## Known Limitations

- Phase-1 implementation performs no sandbox isolation; the planned Phase-2 sandbox
  directory (`Config::sandbox_dir`) is accepted in the config but not yet used.
- `validatePrompt()` does not currently inspect `required_capabilities` or
  `dependencies` fields for malicious patterns; this is a Phase-3 item.

## Dependency Security

- Depends on `utils/error_registry` and `utils/expected` — internal utilities;
  no known CVEs.
- Future Phase-2 HTTP client dependency: security review required before merging;
  must be checked against the GitHub Advisory Database.
