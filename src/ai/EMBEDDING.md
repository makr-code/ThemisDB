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

# Embedding Boundary in the AI Module

AI provenance note: Exact model backend not verifiable from final document text alone.

## Scope

This page records what the current `src/ai` implementation does—and does not—own
for embeddings. It is for developers deciding where to add or integrate an
embedding provider.

`PluginType::EMBEDDING` is a plugin category in
[`include/plugins/plugin_interface.h`](../../include/plugins/plugin_interface.h).
`AIPluginGenerator` accepts a `PluginType` in a generation request and copies it
to the generated plugin manifest. That classification lets generated plugin
artifacts describe an embedding-provider plugin; it does not implement one.

## Current contract

The checked-in AI module exposes no embedding function that tokenizes text,
produces vectors, defines vector dimensions, or manages an embedding model.
There is consequently no `src/ai` embedding model, dataset, embedding-quality
benchmark, or provider-specific API to configure here. The generator's
`SecurityLevel` and `passed_security_checks` fields are not evidence that an
embedding implementation was evaluated.

Do not route embedding inference through `AIPluginGenerator` as if it were an
embedding runtime. Its endpoint contract generates plugin artifacts and requires
non-empty `implementation_code`; it does not define an embedding request or
vector response schema.

## Integration and evaluation boundary

When adding an embedding provider, first define its owning plugin interface and
runtime integration outside this generator contract. The provider contract must
then specify input text handling, output dimension and numeric representation,
failure behavior, and model/version identity. No values for those properties can
be inferred from the `EMBEDDING` enum entry alone.

Evaluate the provider against its own deterministic fixtures and benchmark
baselines. Record vector shape and numeric validity, retrieval quality, and
latency/memory results with the provider and model versions. Keep those results
distinct from `benchmarks/ai/bench_ai_plugin_generator.cpp`, whose canned
endpoint response measures plugin-generation orchestration, not embedding
inference.

## References

- [AI module guide](./MODULE_GUIDE.md)
- [AI generation research](./RESEARCH.md)
- [`PluginType`](../../include/plugins/plugin_interface.h)
- [`AIPluginGenerator` request and result types](../../include/ai/ai_plugin_generator.h)
- [AI generator benchmark](../../benchmarks/ai/bench_ai_plugin_generator.cpp)
