---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# ONNX CLIP Plugin Architecture

Exact model backend not verifiable from final document text alone.

## Purpose and boundary

`src/onnx_clip` implements `themis::plugins::image::ONNXClipPlugin`, a concrete
`IImageAnalysisBackend` implementation. The active behavior is a deterministic
reference generator; it does not create an ONNX Runtime session or generate
learned CLIP features. The module owns plugin lifecycle, per-instance
configuration/state, reference vectors, request statistics, optional digest
verification, and optional read-only file mapping.

The host owns caller authentication, rate limiting, payload and total-batch
limits, vector persistence, access policy, and any actual model/provider
integration. The interface and component boundaries are defined in
[`onnx_clip_plugin.h`](./onnx_clip_plugin.h) and
[`include/plugins/image_analysis_interface.h`](../../include/plugins/image_analysis_interface.h).

## Components and build boundary

| Component | Responsibility |
|---|---|
| `onnx_clip_plugin.h` | Declares the public class, lifecycle and embedding methods, reload operation, and hash callback type. |
| `onnx_clip_plugin.cpp` | Implements deterministic image/text vector generation, config reads, batch chunking, statistics, request tracking, digest verification, and mmap resource lifetime. |
| `CMakeLists.txt` | Adds a shared plugin target only when the ONNX image plugin option is enabled and an `onnxruntime` package is found; OpenCV discovery is optional. |
| `include/plugins/image_analysis_interface.h` | Defines shared result/config types, backend enum, plugin contract, and dynamic export macro. |
| `tests/onnx_clip/` and `tests/test_onnx_clip_*.cpp` | Exercise reference behavior and lifecycle contracts with generated inputs/mock model files. |

The CMake target's ONNX Runtime link dependency is not evidence that the current
source calls ONNX Runtime. The implementation source does not use ONNX Runtime
or OpenCV APIs.

## Runtime flow

```text
host config / request
        |
        v
ONNXClipPlugin --- snapshot shared Impl under pointer-swap mutex
        |                     |
        |                     +-- per-Impl mutex + RequestGuard
        v
validate ready / empty input
        |
        +-- image: 64-bit byte hash + selected metadata -> xorshift64* -> L2 normalize
        |
        +-- text: lowercase/split/hash -> xorshift64* -> L2 normalize
        |
        v
EmbeddingResult(s) + counters/latency
```

`ImageMetadata` affects the image seed through width, height, channel count, and
bits per channel only. Image bytes are not decoded. The text tokenizer splits
on whitespace and punctuation and lowercases; it is not CLIP BPE. The two
outputs are not learned or semantically aligned. Details are in
[`MODELING.md`](./MODELING.md) and [`EMBEDDING.md`](./EMBEDDING.md).

## State and concurrency

The plugin stores `std::shared_ptr<Impl>`. Request methods briefly take
`impl_swap_mtx_` to snapshot that pointer, then increment an in-flight counter
through `RequestGuard` and serialize work on the snapshot's mutex. The shared
snapshot keeps old state alive if a concurrent reload replaces `impl_`.

`reloadModel()` creates and verifies a new `Impl`, takes the pointer-swap lock,
checks the current implementation is ready, copies cumulative counters, and
publishes the new pointer. It then waits up to 30 seconds for requests using
the old snapshot. A drain timeout returns `false` after publication; it does
not restore the old implementation. Hash/config failure before publication
leaves the old implementation in place. See
[`IMPLEMENTATION.md`](./IMPLEMENTATION.md) for the operator-visible distinction.

`shutdown()` serializes on the current implementation mutex, releases any
mapping, and clears readiness. The public methods do not provide caller-level
authorization or admission control.

## Configuration and resources

- `model.embedding_dim` defaults to 512; non-positive values reset to 512.
- `max_batch_size` defaults to 16 for CPU or 64 for other backend values and
  clamps to a minimum of 1. It controls sequential chunk size only; no maximum
  configured value or total request size is enforced.
- `BackendType::AUTO` selects CPU in `initialize()`. Other enums are stored,
  not probed or used to select an inference provider.
- `model.path` and `model.expected_sha256` together enable digest verification.
  OpenSSL is used when compiled in; otherwise the static `ModelHashFn` callback
  is consulted and absence of that callback skips verification.
- Top-level `enable_mmap_loading=true` attempts a read-only mapping when a
  model path is set. The mapping is not passed to an inference runtime; failure
  is silently ignored.

## Failure domains

| Failure | Effect | Owner of next action |
|---|---|---|
| Empty image/text input | A failed `EmbeddingResult` with an explanatory message | Caller corrects or rejects request |
| Request before initialization/after shutdown | Failed result; batch returns one failed item per input | Host lifecycle and routing |
| Digest mismatch/read failure with verifier active | Initialization/reload fails before replacement publication | Deployment pipeline verifies artifact and expected digest |
| Non-OpenSSL build without hash callback | Digest check is skipped despite configured digest | Deployment configuration must reject this state for hardened use |
| mmap unavailable or unreadable | Initialization continues without mapping | No recovery required; mapping is optional and unused for inference |
| Old-snapshot drain timeout | `reloadModel()` returns false after new snapshot has been published | Operator checks current statistics/config and explicitly reloads saved config if rollback is needed |
| Semantic retrieval quality absent | Expected limitation of reference vectors; not a provider outage | Use a model-backed implementation and evaluation gate before semantic use |

## Non-goals of the current implementation

- No model artifact parsing, image decoding, CLIP preprocessing, ONNX graph
  execution, GPU execution, tokenizer vocabulary, or cross-modal alignment.
- No dynamic provider detection, per-caller quotas, model provenance policy,
  vector storage, external metrics exporter, or plugin-local audit logging.
- No claim that a file mapping loads or accelerates a model.

## Source references

- [`onnx_clip_plugin.cpp`](./onnx_clip_plugin.cpp)
- [`onnx_clip_plugin.h`](./onnx_clip_plugin.h)
- [`include/plugins/image_analysis_interface.h`](../../include/plugins/image_analysis_interface.h)
- [`tests/onnx_clip/`](../../tests/onnx_clip/)
- [`CMakeLists.txt`](./CMakeLists.txt)
