> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — LLM Module

> For reporting security vulnerabilities, see the project-level [SECURITY.md](../../../SECURITY.md).

## Security Scope

This document covers the security posture of the LLM module, including inference engines
(`AsyncInferenceEngine`, `InferenceEngineEnhanced`), model management (GGUF loader, LoRA hot-loading,
quantization pipeline), GPU VRAM allocation, KV-cache, streaming output, the OpenAI-compatible adapter,
`ModelRouter`, and all security utility components.

## Threat Model

| Threat | Attack Vector | Mitigation |
|--------|--------------|------------|
| Prompt injection | Malicious instructions embedded in user-supplied or ingested text reaching the inference engine | `llm_security_utils.cpp` detects and sanitises known injection patterns; `SemanticValidator` in the ingestion pipeline provides an upstream filter |
| API key / credential exposure | Inference API keys or model-provider tokens written to logs or error responses | API key values are never passed to any log sink; only request identifiers and HTTP status codes are recorded |
| GPU VRAM memory leakage between models | Residual activations or weight fragments from a previously loaded model accessible after hot-swap | `vram_secure_clear.cpp` performs explicit VRAM zeroing on model unload; `ActiveVRAMAllocator` enforces per-model VRAM isolation |
| Malicious GGUF model files | Crafted model file exploiting parser vulnerabilities or injecting unexpected metadata | `gguf_loader` validates magic bytes, format version, and metadata field types before any memory allocation; oversized tensors are rejected |
| Model poisoning via LoRA adapters | Tampered LoRA adapter injecting backdoor behaviours | `lora_security_validator.cpp` verifies SHA-256 digest of every adapter file against a trusted manifest before loading; adapter files without a manifest entry are rejected |
| Output manipulation via grammar bypass | Attacker-supplied BNF grammar designed to escape constrained generation | Grammar is parsed and validated before compilation; recursive grammars are bounded by a depth limit |
| Speculative decoding exploitation | Synthetic logit arrays used in current implementation may diverge from verifier in unexpected ways | Known limitation; real draft-model logits are on the roadmap; production deployments should enable the verifier-model consistency check |
| Denial of service via resource exhaustion | Flood of inference requests exhausting GPU VRAM or CPU memory | Per-model resource quotas (max concurrent requests, VRAM ceiling, token rate); `ActiveVRAMAllocator` LRU eviction and CPU spilling prevent hard OOM |
| Federated inference data interception | Inter-node communication in planned federated inference (Issue #1928) | Not yet implemented; mTLS will be required before the feature is enabled in production |

## Security Controls

### Prompt Safety
- `llm_security_utils.cpp` provides a sanitisation utility callable from any module that passes user-supplied text to an inference engine.
- Constitutional reasoning engine and ethical guidelines manager run as post-generation filters to catch policy-violating outputs before they are returned to callers.
- Prompt injection detection is applied to both the system prompt and user message segments independently.

### Model Integrity
- GGUF loader: format validation is mandatory and cannot be bypassed via configuration.
- LoRA loader: SHA-256 integrity verification is enforced; adapters loaded without a trusted manifest entry are rejected with an error, not silently skipped.
- Model quantization pipeline (GGUF/AWQ/GPTQ) produces and records a digest of the output artefact.

### Memory & VRAM Isolation
- `vram_secure_clear.cpp` is called unconditionally on model unload, including on error paths.
- `ActiveVRAMAllocator` allocates per-model VRAM regions; cross-region access is not possible through the public API.
- CPU-spilled tensors are written to a process-private memory-mapped file; the file is deleted on process exit.

### Secrets Management
- No API keys or model provider credentials are hard-coded.
- Credentials are injected via environment variables or a secrets provider; they are read once at startup and stored in process memory only.
- Log formatters have an explicit deny-list for field names matching common credential patterns (`api_key`, `token`, `secret`, `password`, `authorization`).

### Transport Security
- The OpenAI-compatible adapter enforces TLS for all inbound connections when deployed with the standard server configuration.
- Federated inference (planned) will require mTLS; plaintext inter-node communication will not be supported.

## Data Handling

- Inference inputs and outputs are not persisted by default; the request deduplication cache is in-memory with a configurable TTL.
- KV-cache entries are keyed by a hash of the prompt; raw prompt text is not stored in the cache index.
- Multi-modal inputs (images) are processed in memory and not written to disk.

## Known Limitations

- **Speculative decoding** currently uses synthetic logit arrays for the draft model, which may produce non-deterministic acceptance rates. Real draft-model logit integration is on the roadmap.
- **Cancellation** of in-flight requests is best-effort; a cancelled request may complete inference and discard the result rather than halting the GPU kernel.
- **Federated inference** (Issue #1928) is not yet implemented. The security design (mTLS, node authentication, result attestation) must be completed before the feature is enabled.
- Prompt injection mitigation covers known pattern classes; novel jailbreak techniques may require updating the `llm_security_utils.cpp` pattern set.

## Dependency Security

| Dependency | Purpose | Notes |
|------------|---------|-------|
| llama.cpp | Core inference backend | Version pinned in vcpkg.json; GGUF loader validation wraps llama.cpp loader |
| CUDA runtime | GPU kernel execution | System-provided; VRAM isolation enforced at allocator level above the runtime |
| OpenSSL / system TLS | TLS for OpenAI-compatible adapter | Peer verification enabled; CA bundle validated at startup |
