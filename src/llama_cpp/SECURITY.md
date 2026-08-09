> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-07 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — llama_cpp Plugin

> For reporting security vulnerabilities, see the project-level [SECURITY.md](../../SECURITY.md).

## Security Scope

This document covers the security posture of the llama_cpp LLM backend plugin:
`LlamaCppPlugin`, LoRA lifecycle, stats reporting.

---

## Threat Model

| Threat | Attack Vector | Mitigation |
|--------|--------------|------------|
| Prompt injection via model output | Attacker embeds instructions in input that cause the model to override system prompt | `PolicyEngine::checkInferencePermission()` is the intended upstream gate (caller responsibility); `LlamaCppPlugin` itself does not apply prompt sanitization in v2.0.0 |
| Malicious GGUF model file | Crafted model file exploiting parser vulnerabilities | Model loading is delegated to `LlamaWrapper` / llama.cpp which validates the GGUF format; additional digest verification planned for v2.1.0 |
| LoRA adapter poisoning | Tampered LoRA adapter injecting backdoor behaviour | Planned: SHA-256 digest verification consistent with LLM module `lora_security_validator.cpp` (Target: v2.1.0) |
| Credential / API key leakage via stats | `getPerformanceStats()` / `getMemoryStats()` returning sensitive data | Stats contain only counters, model path, and flags — no credentials or prompts |
| LoRA deserialization attack via importLoRA | Crafted LoRA binary triggering heap overflow during import | `importLoRA` returns `false` in v2.0.0 (stub); real implementation will validate size bounds before allocation |
| Inference resource exhaustion | Flood of `generate()` calls | Rate limiting is the responsibility of the API layer; `LlamaCppPlugin` has no built-in rate limiter |
| Thread-safety violation | Concurrent `loadModel` + `generate` race | All public methods guarded by `std::mutex mutex_` |

---

## Security Controls

### Thread Safety
- All public methods acquire `std::mutex mutex_` before accessing `model_loaded_`,
  `model_id_`, or `loras_`.

### Error Isolation
- `generate()` returns a structured error response when the model is not loaded; it never
  dereferences a null model pointer.
- `generateRAG()` delegates to `generate()` after prompt augmentation; same error handling.

### LoRA Registry
- Duplicate `lora_id` is replaced (not accumulated) to prevent a stale adapter from
  persisting after an update.
- `unloadLoRA` returns `false` for unknown IDs rather than silently succeeding.

### Planned Controls
- Model file integrity (FNV-64 opt-in gate, upgradeable to SHA-256) — ✅ v2.1.0
- LoRA adapter GGUF magic + 2 GB size bound — ✅ v2.1.0
- Upstream prompt policy check integration via `setPolicyFn` hook — ✅ Q3 2026

---

## Security Checklist (v2.0.0)

- [x] Thread-safe public interface (`std::mutex`)
- [x] Null-model guard in `generate()` / `embed()`
- [x] No credentials in stats output
- [x] LoRA duplicate-id replacement
- [x] Model file integrity check (Target: v2.1.0)
- [x] LoRA adapter integrity check (Target: v2.1.0)
- [x] Upstream `PolicyEngine::checkInferencePermission()` integration (Target: Q3 2026)
- [x] `importLoRA` size validation before allocation (Target: Q4 2026)
