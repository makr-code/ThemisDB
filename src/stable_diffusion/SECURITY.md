---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone. Original creation dates
> could not be verified from available history; `Created` records this AI-assisted revision.

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
| Harmful image generation | User supplies a prompt designed to produce prohibited content | `SDPromptSanitizer::isAllowed()` applies the configured literal keyword list before inference. This is not comprehensive content moderation or a safety guarantee. |
| Prompt injection via negative_prompt | Attacker injects override instructions via `negative_prompt` field | `SDPlugin` calls `SDPromptSanitizer::isAllowed()` on `cfg.negative_prompt` before every generate call since v2.1.0; blocked negative prompts return `success=false` |
| Model file tampering | Crafted model file injecting unexpected behaviour | When the SHA-256 build gate is enabled, `SDPlugin::initialize()` checks the supplied `model_sha256` and fails closed on mismatch or digest-read failure. The gate can be unavailable when its build dependency is absent. |
| Unsafe `blocked_keywords_file` path | Caller supplies an unintended path | `fromFile()` opens the supplied path with `std::ifstream`; it does not normalize or restrict paths to regular files. Validate and authorize the path at the caller boundary. |
| Denial of service via large generation | Attacker requests oversized outputs flooding memory/VRAM | `SDPlugin` enforces positive dimensions, max dimension `8192`, and overflow-safe pixel count checks |
| Information leakage via `prompt_hash` | Hashes of sensitive prompts in audit logs could leak content | FNV-1a is non-cryptographic and not a privacy control. Blocked prompts are fingerprinted from the original prompt; accepted prompts from the sanitized prompt. Avoid logging sensitive-prompt fingerprints unless acceptable. |
| Inference resource exhaustion | Flood of `generate()` calls | Rate limiting is the responsibility of the API layer |

---

## Security Controls

### Content Policy (Prompt Sanitizer)
- `SDPromptSanitizer::isAllowed()` must return `true` before `generate()` reaches the model.
- Keyword list is configurable and loadable from a plain-text file. **TODO:** file ownership,
  permissions, and path authorization are caller/deployment responsibilities and are not
  enforced by `SDPromptSanitizer`.
- `sanitize()` removes all blocked keywords from the prompt before inference.

### Blocked Prompt Handling
- Blocked prompts return `success=false` with `error_message="prompt blocked by content policy"`.
- `blocked_count` in `getStatistics()` allows operators to detect policy violations.
- `GeneratedImage.prompt_hash` is populated on blocked-prompt results; this module does not
  persist it to an audit log.

### Model Integrity
- `model_sha256` can be supplied in `SDConfig`; mismatch or unreadable digest causes
  `initialize()` failure only when `THEMIS_SD_ENABLE_SHA256` is enabled and OpenSSL was
  found at configure time. Otherwise the supplied digest is not enforced.

---

## Security Checklist

- [x] Content-policy check before every inference call
- [x] Blocked prompts return error, not silently succeed
- [x] Exception isolation: generator exceptions caught in `SDPlugin`
- [x] Prompt hash returned on blocked-prompt results (persistence is caller-owned)
- [x] Apply sanitizer to `negative_prompt` (v2.1.0 ✅)
- [x] Optional model file integrity check via `model_sha256` when the SHA-256 build gate is available
- [x] Maximum dimension enforcement in plugin (`<=8192`, overflow-safe)
- [ ] Rate limiting (caller / API layer responsibility)
- [x] Thread-safety guard: `generate_mutex_` serializes all generate paths (v2.1.0 ✅)
- [~] Opt-in parallel `SDCppGenerator` real-backend test exists; execution evidence for a configured model/runtime is pending
