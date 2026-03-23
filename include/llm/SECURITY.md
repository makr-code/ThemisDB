<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — LLM Module (Public Headers)

> For reporting security vulnerabilities, see the project-level [SECURITY.md](../../../SECURITY.md).

## Threat Model

| Threat | Mitigation |
|--------|------------|
| Prompt injection via user input | `llm_security_utils.h` sanitises all external content before prompt construction |
| LoRA adapter poisoning | `lora_security_validator.h` validates adapter provenance; `lora_certificate_store.h` stores certificates |
| VRAM exhaustion (DoS) | `token_quota_manager.h` per-tenant limits; `gpu_safe_fail.h` handles OOM gracefully |
| Unsafe model output (alignment) | `constitutional_reasoning_engine.h` + `ethical_guidelines_manager.h` enforce alignment constraints |
| Byzantine nodes in distributed inference | `byzantine_detector.h` detects and quarantines misbehaving inference nodes |
| Credential leakage via model download | `model_downloader.h` uses secure token injection; tokens never logged |
| KV-cache cross-tenant leakage | `PagedKvCacheManager` namespaces cache pages per tenant |
| Response manipulation via adversarial grammar | `grammar.h` validates grammar before constrained generation |

## Known Limitations

- Speculative decoding uses synthetic draft arrays; real draft-model output requires additional validation.
- Constitutional reasoning is best-effort; not a substitute for human review in high-stakes deployments.
- Implementation-level security details: `../../src/llm/SECURITY.md`.
