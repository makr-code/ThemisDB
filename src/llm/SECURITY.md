# Security - LLM Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

Report vulnerabilities via SECURITY.md.

## Threat Model

| Threat | Mitigation surface |
|---|---|
| Prompt and request abuse | prompt policy and request guard paths |
| Unauthorized or unsafe inference execution | policy checks before backend inference submission |
| Adapter/model path misuse | lifecycle managers and security utility checks |
| Resource exhaustion and denial pressure | quotas, scheduler limits, and runtime validation |
| Cache leakage across contexts | dedicated cache control and lifecycle invalidation surfaces |
| Streaming misuse and partial-response hazards | streaming handler and cancellation-aware execution flow |

## Security Controls

- Prompt-policy and request guard surfaces run before execution paths.
- Runtime validation and quota controls bound unsafe request pressure.
- Adapter/model lifecycle goes through dedicated manager surfaces.
- Security utilities and audit-related LLM paths provide operational traceability.

## Known Limitations

- Security posture depends on deployment configuration and enabled backend/plugin combinations.
- Some distributed/federated execution hardening remains roadmap work.
- Cache and streaming edge-case behavior needs continuous regression coverage under load.

## Sourcecode Verification (Module: llm/security)

- Verified files:
  - src/llm/prompt_policy.cpp
  - src/llm/llm_security_utils.cpp
  - src/llm/production_validator.cpp
  - src/llm/token_quota_manager.cpp
  - src/llm/multi_lora_manager.cpp
  - src/llm/llm_plugin_manager.cpp
  - src/llm/llm_response_cache.cpp
  - src/llm/streaming_handler.cpp
  - src/llm/openai_compat_adapter.cpp
- Verified controls:
  - prompt/request policy and validation gates
  - lifecycle and path handling surfaces for adapters/models
  - resource quota and streaming/cancellation safety-relevant paths
