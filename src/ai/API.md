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

`PluginGenerationPrompt` contains a description, `PluginType` (default
`CUSTOM`), required capabilities, dependencies, `LLMModel`, `SecurityLevel`,
and flags requesting tests and documentation. `validatePrompt()` checks
supported enum values, description presence and length, list counts, token
syntax/length, and duplicate tokens. Invalid input returns an error before
endpoint I/O. Capability tokens are only syntax-checked and forwarded; there is
no provider acknowledgement or generated-manifest proof that they are
supported.

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
| `timeout_ms` | Must be positive; passed to the configured transport; curl applies it to request and connect timeouts. |
| `max_request_body_bytes` | Must be positive; limit checked after request serialization. |
| `max_response_body_bytes` | Must be positive; response limit checked after the transport returns and collects the body. |
| `endpoint_invoke_fn` | Preferred injectable `Result<std::string>` transport; receives endpoint, request JSON, and timeout. |
| `enable_c1_cai_safety_gate`, `c1_min_safety_score`, `c1_cai_eval_fn` | Optional fail-closed score callback gate. |
| `enable_sandbox_gate`, `sandbox_dir`, `output_dir`, `sandbox_verify_fn` | Optional artifact materialization and callback gate; not OS isolation. |
| `enable_c2_federated_telemetry`, `c2_federated_telemetry_fn` | Optional callback receiving local runtime measurements. |

When no `endpoint_invoke_fn` is set, the implementation next uses the instance
`setLlmHttpPostFn()` callback, if present, and otherwise uses curl. The
`setLlmHttpPostFn()` callback returns a response string and has no timeout
parameter; callers should prefer `endpoint_invoke_fn` when they need the
transport result/error and timeout contract. Only the selected transport is
retried (up to three attempts for non-HTTP errors); an exhausted route does not
fall through to another transport. HTTP status failures are not retried.
Curl enforces `timeout_ms`. `endpoint_invoke_fn` receives the timeout and must
enforce it; `setLlmHttpPostFn()` has no timeout and can block indefinitely.
Bounded retry count therefore does not guarantee bounded wall-clock latency
for callback routes that do not enforce a deadline.

An empty endpoint, non-positive timeout or size limit, invalid enum, or invalid
enabled-gate configuration is rejected before transport. When C1 is enabled,
the threshold and callback score must be finite values in `[0,1]`. Enabled C1,
C2, and sandbox settings require their callbacks/directories before transport.
Callback exceptions are converted to fail-closed results.

### Results, failures, and counters

Returned transport/HTTP/parse/schema/gate failures use `ERR_PLUGIN_LOAD_FAILED`;
no partial generated result is returned for those failures. Endpoint, C1,
sandbox, and C2 callback exceptions are converted to errors. Non-object
responses, wrongly typed fields, and malformed or oversized dependency entries
fail closed and increment `parse_errors`; response mapping does not leak JSON
type exceptions. `Stats` exposes cumulative
`validation_errors` (prompt, transport limits, or missing C2 callback),
`transport_errors` (exhausted endpoint attempts or C2 callback failures),
`http_errors` (HTTP status or oversized body), `parse_errors`,
`safety_rejections` (C1 configuration or score rejection),
`sandbox_rejections` (sandbox configuration or verification failure), and
`successes`. Endpoint transport failures are counted once after retries are
exhausted, not once per attempt. There is no reset operation in this API.
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
rule-based path only when critique/revision is needed. The result's
`used_rule_based_fallback` flag is true if the callback was absent, returned an
empty string, or threw while the engine requested critique/revision.
`ethics_evaluation_succeeded` is true only when the ethics evaluator returned a
scored result. `passesAcceptanceCriteria()` rejects evaluator failures, flagged
fallback results, non-finite or out-of-range scores, and thresholds outside
`[0,1]` or non-finite thresholds.
The result combines critique/revision state and CAI scores with ethics scores,
principle/domain identifiers, argument-chain identifiers, and latency.
`safety_score()` averages `cai_revised_score` and
`ethics_overall_score`; `passesAcceptanceCriteria()` compares that value with
the supplied threshold (default `0.80`).

If the ethics evaluator returns a status error, its score fields remain at
their default zero values and `ethics_evaluation_succeeded` remains false.
The acceptance helper rejects this partial result even when the caller uses a
zero threshold. Scores remain policy signals, not proof of factual correctness
or safe execution.

## References

- [Implementation details](./IMPLEMENTATION.md)
- [Trust boundaries](./TRUST.md)
- [`AIPluginGenerator` declaration](../../include/ai/ai_plugin_generator.h)
- [`CAIEthicsIntegration` declaration](../../include/ai/cai_ethics_integration.h)
- [`AIPluginGenerator` implementation](./ai_plugin_generator.cpp)
- [`CAIEthicsIntegration` implementation](./cai_ethics_integration.cpp)
