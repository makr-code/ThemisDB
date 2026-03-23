<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ../../src/llm/ROADMAP.md -->

# Roadmap — LLM Module (Public Headers)

> Implementation roadmap: `../../src/llm/ROADMAP.md`

## Current Status

v1.16.0 — Production-ready. ~75 public headers. Full inference, LoRA, KV-cache, constitutional AI, and OpenAI-compat interfaces available.

## Completed ✅

- [x] Plugin architecture (`ILlmPlugin`, `LlmPluginManager`)
- [x] Paged KV-cache (vLLM-style)
- [x] Continuous batching scheduler
- [x] Multi-LoRA manager with security validation
- [x] Constitutional reasoning and ethics headers
- [x] OpenAI-compat adapter (v1.16.0)
- [x] Speculative decoder interface
- [x] Mixed-precision inference
- [x] CUDA kernel fusion
- [x] Per-tenant token quota manager
- [x] Vision model headers

## Planned

- [ ] Federated inference (Issue #1928) (Target: v1.17.0)
- [ ] Real draft-model logits for speculative decoding (Target: v1.17.0)
- [ ] Persistent disk-backed KV-cache (Target: v1.17.0)
- [ ] Hard cancellation for in-flight requests (Target: v1.17.0)

## Production Readiness Checklist

- [x] Plugin interface stable
- [x] Constitutional AI safety headers
- [x] Per-tenant quota enforcement
- [ ] Federated inference interface
- [ ] Persistent KV-cache interface
