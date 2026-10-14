> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-07 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — llama_cpp Plugin

> For reporting security vulnerabilities, see the project-level [SECURITY.md](../../SECURITY.md).

<!-- Status: current | validated: 2026-10-14 | Primary: src/llama_cpp/ -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Security Scope

This document covers the security posture of the llama_cpp LLM backend plugin:
`LlamaCppPlugin`, LoRA lifecycle, stats reporting.

---

## Threat Model

| Threat | Attack Vector | Mitigation |
|--------|--------------|------------|
| Prompt injection via model output | Attacker embeds instructions in input that cause the model to override system prompt | `PolicyEngine::checkInferencePermission()` is the intended upstream gate (caller responsibility); the plugin enforces policy through `setPolicyFn()` before real or bridged inference executes |
| Malicious GGUF model file | Crafted model file exploiting parser vulnerabilities | Model loading is delegated to `LlamaWrapper` / llama.cpp which validates the GGUF format; digest verification remains opt-in and fail-closed when `verify_model_digest` is enabled |
| LoRA adapter poisoning | Tampered LoRA adapter injecting backdoor behaviour | `importLoRA()` validates GGUF magic bytes and a 2 GB size bound before accepting payloads; summary and caller-owned metadata remain sanitized |
| Credential / API key leakage via stats | `getPerformanceStats()` / `getMemoryStats()` returning sensitive data | Stats contain only counters, model path, and flags — no credentials or prompts |
| LoRA deserialization attack via importLoRA | Crafted LoRA binary triggering heap overflow during import | `importLoRA()` validates the payload size and GGUF magic before accepting data; malformed input fails closed |
| Inference resource exhaustion | Flood of `generate()` calls | Rate limiting is the responsibility of the API layer; `LlamaCppPlugin` has no built-in rate limiter |
| Thread-safety violation | Concurrent `loadModel` + `generate` race | All public methods guarded by `std::mutex mutex_`; shared state is snapshotted under lock before expensive work |

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
- Model file integrity (opt-in non-cryptographic FNV-64 checksum placeholder; SHA-256 remains the planned cryptographic upgrade) — ✅ v2.1.0
- LoRA adapter GGUF magic + 2 GB size bound — ✅ v2.1.0
- Upstream prompt policy check integration via `setPolicyFn` hook — ✅ Q3 2026

---

## Security Checklist (v2.1.0)

- [x] Thread-safe public interface (`std::mutex`)
- [x] Null-model guard in `generate()` / `embed()`
- [x] No credentials in stats output
- [x] LoRA duplicate-id replacement
- [x] Opt-in model file integrity check via `verify_model_digest` + `expected_model_digest`
- [x] LoRA adapter integrity check via GGUF magic + size bound
- [x] Upstream inference policy gate via `setPolicyFn()`
- [x] `importLoRA` size validation before allocation
