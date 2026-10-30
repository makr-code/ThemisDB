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

# AI Module Research Notes

AI provenance note: Exact model backend not verifiable from final document text alone.

## Purpose and research question

The `ai` module owns a controlled plugin-generation path and a CAI/ethics
integration. The engineering question is whether untrusted model responses can
be validated, converted into typed results, and rejected when configured safety
checks fail. These controls do not establish that generated code is correct or
safe to deploy.

## Generation method

`AIPluginGenerator::generatePlugin()` validates the request before endpoint
I/O, serializes the request, invokes either an injected transport or the curl
transport, parses the response, and maps the generated fields into
`GeneratedPlugin`. Validation constrains the prompt description and capability
and dependency tokens. Response validation enforces required and size-bounded
fields. Transport failures, non-2xx responses, invalid JSON, and missing
`implementation_code` produce errors instead of a successful partial result.

The current request contract requires a non-empty description of at most 8,192
characters; capability and dependency lists each allow at most 64 unique tokens,
with token limits of 128 and 256 characters respectively. Defaults include a
10-second timeout, 256 KiB serialized-request limit, and 8 MiB response limit.
Transient transport failures are retried up to three attempts with 100/200 ms
backoff; HTTP status failures are not retried.

Optional C1 safety and sandbox-verification callbacks can reject generated
output. The configured sandbox path materializes an artifact bundle and can
invoke a verification callback; artifact materialization alone is not operating
system isolation or proof that code is safe.

`CAIEthicsIntegration` evaluates a response through its critique/revision
contract and the ethics evaluator, returning per-stage scores and timing. When
no LLM function is provided, the implementation follows its rule-based path.
Its score and acceptance criteria are policy signals, not an independent
measure of factual correctness.

## Evaluation evidence

- `tests/ai/test_ai_plugin_generator.cpp` uses deterministic endpoint callbacks
  to test validation, response parsing, size limits, allow-list behavior,
  optional gates, and statistics.
- `tests/ai/test_ai_generation_soak_60min.cpp` exercises sustained behavior
  with controlled endpoint scenarios; it is not evidence of throughput or
  quality for a live provider.
- `benchmarks/ai/bench_ai_plugin_generator.cpp` injects a callback returning
  canned JSON. Its validation, mapping, and malformed-response results exclude
  real model inference latency and output quality.
- `src/ai/cai_ethics_integration.cpp` is a separate evaluation path; results
  depend on the configured principles, weights, and supplied response/query.

## Reproducibility and limits

Build and run the focused generator tests after configuring module tests:

```bash
cmake --build <build-dir> --target module_ai_test_ai_plugin_generator_focused
ctest --test-dir <build-dir> -R '^test_ai_plugin_generator_ai_FocusedTests$' --output-on-failure
```

For a live-provider evaluation, record endpoint/model identifier, model and
prompt versions, request limits, timeout/retry configuration, generated
artifact hashes, callback and gate configuration, and the exact test prompts.
Keep live-provider latency and output-quality results separate from the
callback-based microbenchmark. Run generated artifacts through the project's
independent build, test, and security review before deployment.

## References

- [Modeling notes](./MODELING.md)
- [AI module architecture](./ARCHITECTURE.md)
- `include/ai/ai_plugin_generator.h`
- `src/ai/ai_plugin_generator.cpp`
- `include/ai/cai_ethics_integration.h`
- `src/ai/cai_ethics_integration.cpp`
- `tests/ai/test_ai_plugin_generator.cpp`
- `benchmarks/ai/bench_ai_plugin_generator.cpp`
