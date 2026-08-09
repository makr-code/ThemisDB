> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-08-09 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Stable Diffusion Plugin

> For reporting security vulnerabilities, see the project-level [SECURITY.md](../../SECURITY.md).

## Security Scope

This document covers the security posture of the Stable Diffusion image generation plugin:
`SDPlugin`, `SDPromptSanitizer`, `ISDGenerator`, `SDConfig`.

---

## Threat Model

| Threat | Attack Vector | Mitigation |
|--------|--------------|------------|
| Harmful image generation | User supplies a prompt designed to produce CSAM or other prohibited content | `SDPromptSanitizer::isAllowed()` called before every `generate()`; blocked prompts never reach the model |
| Prompt injection via negative_prompt | Attacker injects override instructions via `negative_prompt` field | `SDPlugin` calls `SDPromptSanitizer::isAllowed()` on `cfg.negative_prompt` before every generate call since v2.1.0; blocked negative prompts return `success=false` |
| Model file tampering | Crafted model file injecting unexpected behaviour | `SDPlugin::initialize()` verifies `model_sha256` (when provided) against computed SHA-256 and fails closed on mismatch |
| Path traversal via `blocked_keywords_file` | Attacker supplies `../../etc/passwd` as keyword file path | `SDPromptSanitizer::fromFile` opens only regular files; path normalisation is the responsibility of the caller |
| Denial of service via large generation | Attacker requests oversized outputs flooding memory/VRAM | `SDPlugin` enforces positive dimensions, max dimension `8192`, and overflow-safe pixel count checks |
| Information leakage via `prompt_hash` | Hashes of sensitive prompts in audit logs could leak content | FNV-1a is a non-cryptographic hash — it is used only as a stable fingerprint for deduplication, not as a privacy control; sensitive prompts should be filtered before logging |
| Inference resource exhaustion | Flood of `generate()` calls | Rate limiting is the responsibility of the API layer |

---

## Security Controls

### Content Policy (Prompt Sanitizer)
- `SDPromptSanitizer::isAllowed()` must return `true` before `generate()` reaches the model.
- Keyword list is configurable and loadable from a protected file.
- `sanitize()` removes all blocked keywords from the prompt before inference.

### Blocked Prompt Handling
- Blocked prompts return `success=false` with `error_message="prompt blocked by content policy"`.
- `blocked_count` in `getStatistics()` allows operators to detect policy violations.
- Prompt hash is still recorded for audit, even on blocked paths.

### Model Integrity
- `model_sha256` can be supplied in `SDConfig`; mismatch or unreadable digest causes
  `initialize()` failure (fail closed).

---

## Security Checklist

- [x] Content-policy check before every inference call
- [x] Blocked prompts return error, not silently succeed
- [x] Exception isolation: generator exceptions caught in `SDPlugin`
- [x] Prompt hash in audit log for blocked prompts
- [x] Apply sanitizer to `negative_prompt` (v2.1.0 ✅)
- [x] Model file integrity check via `model_sha256` gate
- [x] Maximum dimension enforcement in plugin (`<=8192`, overflow-safe)
- [ ] Rate limiting (caller / API layer responsibility)
- [x] Thread-safety guard: `generate_mutex_` serializes all generate paths (v2.1.0 ✅)
- [ ] Thread-safety audit for parallel `SDCppGenerator` calls (Target: Q1 2027)
