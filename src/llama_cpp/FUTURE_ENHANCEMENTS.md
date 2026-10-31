---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

> Exact model backend not verifiable from final document text alone.

# Future Enhancements — llama_cpp Plugin

## Scope

This document tracks approved enhancements beyond the current implementation in
`src/llama_cpp/llama_cpp_plugin.cpp` and `llama_cpp_registrar.cpp`. The existing
wrapper, RAG, embedding, streaming, batching, policy, and LoRA paths are described
in [`ARCHITECTURE.md`](ARCHITECTURE.md); their presence is not a proposal to
reimplement them.

No additional module-specific enhancement is currently approved. New proposals
must define acceptance criteria, source/interface scope, error behavior, tests,
and measurable performance targets where relevant before being marked planned.

## Design Constraints

- Preserve the `ILLMPlugin` public contract and document any intentional API
  change before implementation.
- Do not treat test-only synthetic output, zero-vector embedding fallback, or
  LoRA registry metadata as real backend behavior.
- Do not claim production readiness from a successful `loadModel()` call, the
  plugin loaded flag, or static capability flags.
- Keep authentication, authorization, admission control, and artifact
  authenticity at the appropriate service/deployment boundary.

## Required Interfaces

No new interfaces are approved. Existing public contracts are defined in
[`../../include/llama_cpp/llama_cpp_plugin.h`](../../include/llama_cpp/llama_cpp_plugin.h),
[`../../include/llama_cpp/llama_cpp_registrar.h`](../../include/llama_cpp/llama_cpp_registrar.h),
and [`../../include/llm/llm_plugin_interface.h`](../../include/llm/llm_plugin_interface.h).

## Implementation Notes

When an enhancement is approved, update the source, public contract, focused
tests, [`ROADMAP.md`](ROADMAP.md), and
[`PRODUCTION_REQUIREMENTS.md`](PRODUCTION_REQUIREMENTS.md) together. Distinguish
live-wrapper, injected callback, and test-stub behavior in both implementation
and validation evidence.

## Test Strategy

Future work must add focused tests for the changed behavior and its failure
cases. Run `ctest --preset windows-release -R 'llama_cpp' --output-on-failure`
and `python scripts/check_module_direct_doxygen.py --module llama_cpp` where
those configured targets are available. Real-backend changes additionally need
a probe using an approved model artifact; test fixtures alone are insufficient.

## Performance Targets

No new performance target is approved here. Use the existing module mappings in
[`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md) only after confirming
that the benchmark exercises the actual plugin and backend being evaluated.
Record hardware, build flags, model identity, workload, and repeated-run
distributions for any new target.

## Security / Reliability

Future work must preserve explicit failure responses, avoid representing
synthetic fallbacks as model output, and include input/resource limits appropriate
to the changed path. Model and adapter authenticity must use trusted deployment
evidence; GGUF magic and FNV-64 are not authenticity checks. Caller policy,
tenant isolation, quotas, timeouts, and rollback remain deployment
responsibilities unless a future approved interface explicitly changes that
boundary.
