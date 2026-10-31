---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Distributed Knowledge Implementation Notes

## Purpose and layout

This guide is for contributors changing the in-process federation components.
Public contracts are declared under `include/distributed_knowledge/`;
implementations are under `src/distributed_knowledge/`; focused tests are under
`tests/distributed_knowledge/`.

| Surface | Implementation | Public interface |
|---|---|---|
| Capability announcement | `adapter_capability_announcement.cpp` | `adapter_capability_announcement.h` |
| LoRA aggregation | `lora_federation_coordinator.cpp` | `lora_federation_coordinator.h` |
| RAG merge | `federated_rag_merger.cpp` | `federated_rag_merger.h` |
| Feedback sync | `cross_shard_feedback_sync.cpp` | `cross_shard_feedback_sync.h` |
| Distillation | `federated_distillation_coordinator.cpp` | `federated_distillation_coordinator.h` |

The library target is `themis_distributed_knowledge`, uses C++17, and links
JSON and threads publicly. Optional `fmt`, `yaml-cpp`, `themis_core`, and
`themis_security` target integrations are wired by
[CMakeLists.txt](./CMakeLists.txt).

**Build-registration limitation:** the current module
[`CMakeLists.txt`](./CMakeLists.txt) lists the capability, LoRA, RAG, and
feedback implementation files, but not
`federated_distillation_coordinator.cpp`, although that source and its public
header exist. The module test CMake file includes it only in its fallback
source list when `themis_distributed_knowledge` is not a target; separate
top-level test registration also references it. This source inspection does
not establish whether the production library is intended to provide this
implementation through another target. Do not infer library-link coverage
from the file's presence; confirm the target ownership/build wiring.

## State and invariants

- Capability announcements set the publisher's local shard ID and current
  timestamp before serialization. Inbound JSON is parsed into the public
  announcement type before the optional callback.
- LoRA submissions are keyed by shard ID and accepted only for the current
  round; duplicate submissions are ignored. Aggregation requires the minimum
  participant count, filters numeric JSON values per key, applies configured
  privacy noise, then advances the round and clears pending contributions.
- RAG merge is stateless with respect to shard data. Strategy output is
  deduplicated (when configured) and truncated to `top_k`. Failed shards are
  skipped; timeout behavior may throw when all supplied results time out.
- Feedback publishing checks the optional embedding-dimension constraint,
  anonymizes the origin field, and sends JSON through a caller-supplied gossip
  callback. Inbound summaries are deduplicated, checked by configured policy
  callbacks, and delivered to the optional consumer callback.
- Distillation rejects empty submissions, evaluates round/privacy and policy
  bounds before broadcast, and calls registered student callbacks after
  preparing the round.

Stateful coordinators protect internal state with mutexes. Avoid invoking
blocking network operations while holding module locks; transport, retry,
durability, and cross-node transaction behavior belong to the integrating
service.

## Error and edge behavior

Constructors reject invalid configs with `std::invalid_argument`. Depending
on the API, callers can also observe `std::runtime_error` for insufficient
participants, all-shard timeout, exhausted privacy budget, missing pending
labels, or policy rejection. Some input conditions are intentionally ignored
(for example duplicate or wrong-round LoRA contributions), while feedback
policy rejection increments a counter and drops the message. Preserve these
documented distinctions when changing contracts.

## Development and verification

Use [API.md](./API.md) before changing exposed types, and keep the module's
[architecture](./ARCHITECTURE.md) and
[production requirements](./PRODUCTION_REQUIREMENTS.md) aligned when runtime
behavior changes. Add focused regressions in the corresponding test file;
CTest registers module tests with the `distributed_knowledge` label and a
120-second timeout. The test target naming and registration live in
`tests/distributed_knowledge/CMakeLists.txt`.

Run the narrow CTest case first, then the module label and mapped release
benchmarks when behavior or performance changes. See
[MODULE_GUIDE.md](./MODULE_GUIDE.md) for the navigation path and
[ROADMAP.md](./ROADMAP.md) for known unfinished hardening.
