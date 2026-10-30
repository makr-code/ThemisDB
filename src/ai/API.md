---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# AI Module API

AI provenance note: Exact model backend not verifiable from final document text alone.

## Audience and source

This is the developer-facing contract summary for the public headers
[`ai_plugin_generator.h`](../../include/ai/ai_plugin_generator.h) and
[`cai_ethics_integration.h`](../../include/ai/cai_ethics_integration.h).
Those headers define two distinct namespaces and result models; this page does
not add an API beyond them.

## Plugin generation API

The generator types are in `themis::plugins::ai`.

### Request and result

`PluginGenerationPrompt` contains a description, `PluginType`, required
capabilities, dependencies, `LLMModel`, `SecurityLevel`, and flags requesting
tests and documentation. `validatePrompt()` checks description presence and
length, list counts, token syntax/length, and duplicate tokens. Invalid input
returns an error before endpoint I/O. `generatePlugin()` repeats that validation
and returns `Result<GeneratedPlugin>`.

`GeneratedPlugin` contains generated header, implementation, test, and CMake
text, a `PluginManifest`, build dependency strings, a model-provided
`passed_security_checks` flag, and a security report. Code must not treat that
flag or report as independent verification.

### Configuration and transport

`AIPluginGenerator::Config` provides:

| Member | Contract |
|---|---|
| `llm_endpoint` | Endpoint string used by the selected transport. |
| `allowed_llm_endpoints` | Empty means no list check; otherwise the endpoint must exactly equal one entry. |
| `timeout_ms` | Passed to the configured transport; curl applies it to request and connect timeouts. |
| `max_request_body_bytes` | Limit checked after request serialization. |
| `max_response_body_bytes` | Response limit checked after the transport returns. |
| `endpoint_invoke_fn` | Preferred injectable `Result<std::string>` transport; receives endpoint, request JSON, and timeout. |
| `enable_c1_cai_safety_gate`, `c1_min_safety_score`, `c1_cai_eval_fn` | Optional fail-closed score callback gate. |
| `enable_sandbox_gate`, `sandbox_dir`, `output_dir`, `sandbox_verify_fn` | Optional artifact materialization and callback gate; not OS isolation. |
| `enable_c2_federated_telemetry`, `c2_federated_telemetry_fn` | Optional callback receiving local runtime measurements. |

When no `endpoint_invoke_fn` is set, the implementation next uses the instance
`setLlmHttpPostFn()` callback, if present, and otherwise uses curl. The
`setLlmHttpPostFn()` callback returns a response string and has no timeout
parameter; callers should prefer `endpoint_invoke_fn` when they need the
transport result/error and timeout contract.

### Results, failures, and counters

Returned transport/HTTP/parse/schema/gate failures use `ERR_PLUGIN_LOAD_FAILED`;
no partial generated result is returned for those failures. This does not cover
exceptions: typed response-field extraction and most callbacks are not wrapped
in exception handling, so a wrong JSON field type or a throwing callback may
escape `generatePlugin()`. Callbacks should return their declared `Result`
errors rather than throw. `Stats` exposes cumulative `validation_errors`,
`transport_errors`, `http_errors`, `parse_errors`, `safety_rejections`,
`sandbox_rejections`, and `successes`. There is no reset operation in this API.
The counters and instance callback setter are not synchronized; serialize calls
and configuration changes on one generator instance.

The header also declares `setLLMGenerateFn()` and static `setHttpPostFn()` /
`clearHttpPostFn()` hooks. No corresponding definitions are present in the
checked-in `src/ai/ai_plugin_generator.cpp`; use the implemented configuration
transport or `setLlmHttpPostFn()` path rather than assuming these declarations
are linkable.

## CAI/ethics API

`CAIEthicsIntegration` is in `themis::ai`, not
`themis::plugins::ai`. `evaluate(response, query, llm_fn)` returns
`CAIEvaluationResult` directly. An empty `llm_fn` selects the engine's
rule-based path. The result combines critique/revision state and CAI scores with
ethics scores, principle/domain identifiers, argument-chain identifiers, and
latency. `safety_score()` averages `cai_revised_score` and
`ethics_overall_score`; `passesAcceptanceCriteria()` compares that value with
the supplied threshold (default `0.80`).

If the ethics evaluator returns a status error, its score fields remain at
their default zero values. Callers must therefore treat the score threshold as
a policy decision, not a substitute for checking the evaluation inputs and
configuration.

## References

- [Implementation details](./IMPLEMENTATION.md)
- [Trust boundaries](./TRUST.md)
- [`AIPluginGenerator` declaration](../../include/ai/ai_plugin_generator.h)
- [`CAIEthicsIntegration` declaration](../../include/ai/cai_ethics_integration.h)
- [`AIPluginGenerator` implementation](./ai_plugin_generator.cpp)
- [`CAIEthicsIntegration` implementation](./cai_ethics_integration.cpp)
