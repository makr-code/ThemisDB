---
Author: "ThemisDB Team"
Created: "2026-10-21"
Last Updated: "2026-10-21"
Status: "active"
AI_Used: true
AI_Models: "GitHub Copilot"
AI_Contributor: "GitHub Copilot"
Human_Review: true
Human_Author: "ThemisDB Team"
---

# LLM Module Guide

AI provenance note: Exact model backend not verifiable from final document text alone.

This guide is the entry point for contributors working on ThemisDB's LLM subsystem. It focuses on the authoritative source files and the operational reading order for changes in this module.

## Primary sources of truth

Read these in order:

1. `src/llm/README.md` — top-level module purpose and quick-start summary
2. `src/llm/ARCHITECTURE.md` — runtime boundaries and control flow
3. `src/llm/RESEARCH.md` — design assumptions and failure posture
4. `src/llm/MODELING.md` — operational model and invariants
5. `src/llm/IMPLEMENTATION.md` — implementation organization and edge cases
6. `src/llm/API.md` — public contract and semantics
7. `src/llm/RUNBOOK.md` — diagnosis, recovery, and escalation
8. `src/llm/TRUST.md` — risk boundaries and trust assumptions

## Ownership boundaries

The module's ownership is clear:

- public contract: `include/llm/`
- implementation logic: `src/llm/`
- runtime model adapters: `src/llm/lora_framework/` and backend-specific folders
- test/validation artifacts: module-level tests and associated benchmark sources

## Common subsystem entry points

### Generation and runtime

- `include/llm/embedded_llm.h`
- `src/llm/embedded_llm.cpp`
- `include/llm/llama_wrapper.h`
- `src/llm/llama_wrapper.cpp`

### Model lifecycle and integrity

- `include/llm/model_loader.h`
- `src/llm/model_loader.cpp`
- `src/llm/llm_plugin_manager.cpp`

### Prompt safety and policy

- `include/llm/prompt_policy.h`
- `src/llm/prompt_policy.cpp`
- `include/llm/llm_security_utils.h`
- `src/llm/llm_security_utils.cpp`

### Routing and adapter behavior

- `include/llm/model_router.h`
- `src/llm/model_router.cpp`
- `src/llm/lora_router.cpp`

## Contributor workflow

Before changing behavior in this module:

1. identify the concrete API or runtime object impacted
2. read the corresponding implementation files in `src/llm/`
3. check the relevant contract in `include/llm/`
4. verify the failure path and degraded behavior before editing
5. update both source and docs when the behavior or contract changes

## Change checklist

For any LLM change, confirm the following:

- prompt safety remains explicit and fail-closed
- model loading still validates integrity when required
- plugin selection remains deterministic and logged
- route matching remains clear and testable
- degraded or fallback behavior is documented rather than implied
- the docs remain aligned with the actual code paths after the patch

## Guardrails

- Do not add a hidden default that bypasses checksum validation.
- Do not broaden routing rules silently; they must remain explicit and reviewable.
- Do not weaken prompt policies without a corresponding security review.
- Do not present a fallback embedding as a real semantic model.

## Conclusion

The LLM module is a bounded production runtime with clear ownership, explicit contracts, and operationally meaningful failure modes. The documentation should remain grounded in the actual code and treated as a companion to the implementation, not as a second source of truth.
