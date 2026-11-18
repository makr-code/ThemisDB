---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-11-18"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# AI Module Implementation

AI provenance note: Exact model backend not verifiable from final document text alone.

## Scope and ownership

This guide describes the implemented plugin-generation path in
`src/ai/ai_plugin_generator.cpp` and the separate CAI/ethics adapter in
`src/ai/cai_ethics_integration.cpp`. The corresponding public contracts are
[`include/ai/ai_plugin_generator.h`](../../include/ai/ai_plugin_generator.h)
and [`include/ai/cai_ethics_integration.h`](../../include/ai/cai_ethics_integration.h).
The generator creates artifact content; it does not compile, load, or deploy the
generated plugin.

## Generator execution order

`AIPluginGenerator::generatePlugin()` applies these checks in order:

1. Validate plugin/model/security enum values, the description, and
   capability/dependency lists before endpoint I/O.
   The description must contain 1–8,192 bytes. Each list permits at most 64
   unique tokens; capability tokens are limited to 128 characters and dependency
   tokens to 256. Tokens must be non-empty and contain no whitespace; allowed
   punctuation is `_`, `-`, `.`, `:`, `/`, and `+`.
2. Remove ASCII control characters from the description except tab, newline, and
   carriage return. This is input normalization, not a prompt-injection defense.
3. Validate configuration before endpoint I/O: endpoint must be non-empty,
   timeout and request/response size limits must be positive, and enabled C1,
   C2, or sandbox gates must have valid thresholds/callbacks/directories.
4. Serialize the request (`description`, integer plugin/model/security enums,
   capabilities, dependencies, and test/document flags). Reject it if it exceeds
   `Config::max_request_body_bytes`.
5. If `allowed_llm_endpoints` is non-empty, require an exact string match for
   `llm_endpoint`; no URL normalization is performed.
6. Select `endpoint_invoke_fn`, instance `setLlmHttpPostFn()`, then curl. Retry
   only the selected route at most three times for non-HTTP errors, with 100 ms
   and 200 ms sleeps between attempts. HTTP status failures are not retried and
   exhausting one route does not trigger failover to another.
7. Reject responses larger than `max_response_body_bytes`, parse a JSON object,
   and use the object under `generated_plugin` when present; reject a malformed
   nested payload or non-object response.
8. Map and validate generated fields and dependency entries, convert wrong
   field types into parse errors, require non-empty
   `implementation_code`, then run enabled C1, sandbox, and C2 gates in that
   order. Return the typed result only if all enabled gates succeed.

The response-size check happens after the transport returns its body. In
particular, the curl callback appends response bytes before this check; the
configured limit is not a streaming memory cap.
Curl enforces `timeout_ms`; `endpoint_invoke_fn` receives the timeout but must
enforce it, while `setLlmHttpPostFn()` has no timeout parameter and may block
indefinitely. The retry count is bounded, but callback routes do not have a
wall-clock bound unless their implementations enforce a deadline.

## Response mapping and invariants

- `header_code`, `implementation_code`, `test_code`, and `cmake_code` are each
  limited to 1 MiB; `security_report` is limited to 64 KiB.
- An empty or overlong name falls back to `generated_plugin`; an empty or
  overlong version falls back to `0.1.0`.
- A description longer than 8,192 bytes is truncated. Dependency entries
  that are non-strings or longer than 256 characters are omitted.
- The manifest type comes from the original request. A model-provided
  `passed_security_checks` value is data, not proof that checks were run.
- Structural acceptance does not compile or statically analyze generated code.

The request is rejected on validation/configuration, returned transport errors,
HTTP, invalid JSON syntax, schema-size, required-field, or enabled-gate failure.
These returned `Result` failures use `ERR_PLUGIN_LOAD_FAILED`; callback
exceptions on transport, C1, sandbox, and C2 paths are converted to the same
fail-closed result model. `getStats()` provides cumulative counters by failure
class; endpoint transport failures are counted once after bounded attempts
are exhausted (C2 failures share the transport counter). Invalid response
shape, wrong field types, and invalid dependency entries increment
`parse_errors` and return a structured error.

## Gate semantics

- **C1:** When enabled, a callback is mandatory. Its finite score must be at
  least `c1_min_safety_score`; the threshold and returned score must both be in
  `[0,1]`. Invalid or absent configuration is rejected before transport;
  callback errors, exceptions, non-finite/out-of-range scores, and scores below
  the threshold reject generation.
- **Sandbox:** When enabled, generated files are written under `sandbox_dir`,
  read back for byte equality, and copied to `output_dir`. An optional callback
  can apply additional policy. Directory materialization is not process
  isolation or static analysis.
- **C2:** When enabled, a callback is mandatory and receives local size
  measurements, the model-provided `passed_security_checks` value, and the C1
  score when available. Callback failure rejects the generation.

## CAI/ethics integration

`CAIEthicsIntegration::evaluate()` runs constitutional critique/revision and
then the ethics evaluator, producing a `CAIEvaluationResult` with revision
state, principle IDs, scores, ethics-evaluator status, fallback provenance,
and elapsed time. A status error leaves `ethics_evaluation_succeeded` false.
When a needed callback is absent, returns empty output, or throws, the engine
uses the rule-based path and sets `used_rule_based_fallback`; the acceptance
helper rejects either degraded state, invalid scores, and invalid thresholds.
This result is a policy signal, not proof of factual correctness or safe
execution.

## Focused validation

The generator tests use an injected deterministic endpoint callback. From a
configured build directory, run:

```bash
cmake --build <build-dir> --target module_ai_test_ai_plugin_generator_focused
ctest --test-dir <build-dir> -R '^test_ai_plugin_generator_ai_FocusedTests$' --output-on-failure
cmake --build <build-dir> --target module_ai_test_ai_cai_ethics_integration_focused
ctest --test-dir <build-dir> -R '^test_ai_cai_ethics_integration_ai_FocusedTests$' --output-on-failure
```

These deterministic tests verify invalid/degraded configuration, selected-route
retry bounds, mapping, response errors, gates, fallback provenance, and
statistics; they do not measure a live provider's inference quality or latency.
`benchmarks/ai/bench_ai_plugin_generator.cpp` likewise measures callback-only
orchestration and malformed-response handling, not provider queueing or
inference.

## References

- [AI module guide](./MODULE_GUIDE.md)
- [API contract](./API.md)
- [Architecture](./ARCHITECTURE.md)
- [Research and evaluation](./RESEARCH.md)
- [Modeling and failure boundaries](./MODELING.md)
- [`ai_plugin_generator.cpp`](./ai_plugin_generator.cpp)
- [`cai_ethics_integration.cpp`](./cai_ethics_integration.cpp)
- [`test_ai_plugin_generator.cpp`](../../tests/ai/test_ai_plugin_generator.cpp)
