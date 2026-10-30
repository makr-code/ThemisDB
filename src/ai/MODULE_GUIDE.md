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

# AI Module Developer Guide

AI provenance note: Exact model backend not verifiable from final document text alone.

## Start here

The module's implemented source is intentionally small: the plugin artifact
generator and a CAI/ethics integration adapter. Read the
[architecture](./ARCHITECTURE.md) for boundaries, then the
[implementation guide](./IMPLEMENTATION.md) and [API contract](./API.md) before
integrating either path.

| Need | Document |
|---|---|
| Method assumptions and measured evidence | [RESEARCH.md](./RESEARCH.md), [MODELING.md](./MODELING.md) |
| Embedding scope and provider boundary | [EMBEDDING.md](./EMBEDDING.md) |
| Development and test flow | [IMPLEMENTATION.md](./IMPLEMENTATION.md) |
| Public types and callback behavior | [API.md](./API.md) |
| Incident diagnosis and recovery | [RUNBOOK.md](./RUNBOOK.md), [OPERATIONS.md](./OPERATIONS.md) |
| Trust boundaries and security risks | [TRUST.md](./TRUST.md), [SECURITY.md](./SECURITY.md) |
| Planned work and current limitations | [ROADMAP.md](./ROADMAP.md), [FUTURE_ENHANCEMENTS.md](./FUTURE_ENHANCEMENTS.md) |

## Source ownership

- `include/ai/ai_plugin_generator.h` and
  `include/ai/cai_ethics_integration.h` define the public API.
- `src/ai/ai_plugin_generator.cpp` validates a generation request, invokes an
  endpoint, maps the response, and optionally runs configured gates.
- `src/ai/cai_ethics_integration.cpp` adapts constitutional reasoning results
  into the ethics evaluator's input and returns combined scores.
- `tests/ai/test_ai_plugin_generator.cpp` exercises the generator with injected
  endpoint responses. The test target and CTest name are documented in
  [IMPLEMENTATION.md](./IMPLEMENTATION.md).

`PluginType::EMBEDDING` is a shared plugin category, not an embedding inference
implementation in `src/ai`; see [EMBEDDING.md](./EMBEDDING.md).

## Integration invariants

1. Generation validation precedes endpoint I/O. Keep callers from treating a
   successful JSON parse as code validation.
2. Keep endpoint strings and configured allow-list entries identical; the
   generator compares them literally.
3. Enabled C1, sandbox, and C2 gates fail closed. Configure their callbacks
   before enabling a gate; do not silently bypass a failed gate.
4. Generated artifacts remain untrusted. The generator does not compile, load,
   or deploy them, and writing them to `sandbox_dir` does not isolate execution.
5. A single generator instance has unsynchronized counters and callback state.
   Serialize its calls and configuration changes.

## Before changing behavior

Check the source and tests named above, update the relevant contract page in
the same change, and run the focused generator test target. For claims about
provider quality or live inference performance, add provider-backed evidence;
the callback-based test and benchmark fixtures do not establish those claims.

## References

- [README](./README.md)
- [Architecture](./ARCHITECTURE.md)
- [Runbook](./RUNBOOK.md)
- [Operations](./OPERATIONS.md)
- [Trust model](./TRUST.md)
