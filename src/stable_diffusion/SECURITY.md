<!-- Status: current | validated: 2026-04-07 -->
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
| Model file tampering | Crafted model file injecting unexpected behaviour | Planned SHA-256 digest check before model load (Target: v2.1.0) |
| Path traversal via `blocked_keywords_file` | Attacker supplies `../../etc/passwd` as keyword file path | `SDPromptSanitizer::fromFile` opens only regular files; path normalisation is the responsibility of the caller |
| Denial of service via large generation | Attacker requests 8192×8192 images flooding GPU VRAM | Callers and the API layer must enforce max dimensions; `SDPlugin` itself has no dimension cap in v2.0.0 |
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

### Model Integrity (Planned)
- v2.1.0 will add SHA-256 digest verification of model files before loading.

---

## Security Checklist (v2.0.0)

- [x] Content-policy check before every inference call
- [x] Blocked prompts return error, not silently succeed
- [x] Exception isolation: generator exceptions caught in `SDPlugin`
- [x] Prompt hash in audit log for blocked prompts
- [x] Apply sanitizer to `negative_prompt` (v2.1.0 ✅)
- [ ] Model file integrity check (Target: v2.1.0)
- [ ] Maximum dimension enforcement (caller / API layer responsibility)
- [ ] Rate limiting (caller / API layer responsibility)
- [ ] Thread-safety audit (Target: v2.1.0)
