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

# AI Generation and Safety Modeling

AI provenance note: Exact model backend not verifiable from final document text alone.

## Plugin-generation state flow

The generator is an endpoint adapter with explicit validation and normalization;
it does not train or rank models. A request carries a plugin description, type,
capability and dependency lists, and generation preferences. The implemented
flow is:

1. Validate description and list bounds/tokens before network activity.
2. Serialize the request and invoke the configured endpoint through the
   injected function or curl transport with a bounded timeout.
3. Reject transport errors and non-success HTTP status.
4. Parse the structured response and require a non-empty implementation body;
   apply field-size and manifest normalization rules.
5. Run configured safety and sandbox-verification callbacks, then return the
   typed result or a structured error.

The output contains generated source, tests, CMake text, manifest data,
dependencies, and a security report. These fields are model-provided content;
the generator's structural validation is not compilation or static analysis.
The description must be 1–8,192 characters; each capabilities/dependencies list
is capped at 64 unique tokens (maximum token lengths 128/256 characters).
Default request and response size limits are 256 KiB and 8 MiB, and the default
transport timeout is 10 seconds. Transient transport failures get at most three
attempts with 100 ms then 200 ms backoff; HTTP status errors fail immediately.

## Optional gates and decision model

The C1 safety callback returns a score compared with a configured minimum.
When the gate is enabled, missing callbacks, non-finite scores, and scores below
the threshold are rejection conditions. The sandbox gate writes the generated
bundle and optionally asks a callback to verify it; no claim of OS process
isolation follows from writing files into the configured directory.

The CAI integration has a different input/output contract: it combines a
critique/revision result with ethics dimensions and exposes an aggregate safety
score. Because the principles, evaluator weights, and optional LLM function
affect the result, comparisons require those inputs to be held constant and
reported.

## Failure model and measurable boundaries

Validation errors should occur before transport. Transport, HTTP, JSON, schema,
required-field, safety-gate, and sandbox-callback failures remain separate
failure classes in the generator statistics. The configured request/response
size bounds and timeout constrain resource use, but they do not constrain
semantic quality.

The current unit tests use callback-supplied responses; the dedicated benchmark
also uses a canned response. Therefore, latency from those tests and benchmarks
measures framework work around the injected function, not provider inference.
Evaluation of code correctness or safety needs independent compiler,
test-suite, static-analysis, and human review evidence.

## Reproducibility

Use the focused target and CTest name documented in
[research notes](./RESEARCH.md). For comparisons, retain the prompt fixture,
model/provider version, endpoint settings, retry and timeout values, callback
configuration, gate thresholds, generated response, and artifact hashes.
Report rejection counts by failure class alongside success and latency; a
success-rate change alone can hide a change in which gate rejected output.

## Source references

- `include/ai/ai_plugin_generator.h`
- `src/ai/ai_plugin_generator.cpp`
- `include/ai/cai_ethics_integration.h`
- `src/ai/cai_ethics_integration.cpp`
- `tests/ai/test_ai_plugin_generator.cpp`
- `benchmarks/ai/bench_ai_plugin_generator.cpp`
