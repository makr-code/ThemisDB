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

# ONNX CLIP Implementation Notes

Exact model backend not verifiable from final document text alone.

## Ownership and build

`src/onnx_clip/onnx_clip_plugin.h` declares `ONNXClipPlugin`; its implementation
and private `Impl` are in `onnx_clip_plugin.cpp`. The class implements the
image backend contract from
[`include/plugins/image_analysis_interface.h`](../../include/plugins/image_analysis_interface.h).
The implementation keeps the public declaration free of provider-specific
types and exports the dynamic plugin entry point unless
`THEMIS_IMAGE_PLUGIN_DISABLE_EXPORT` is defined.

The source-local [`CMakeLists.txt`](./CMakeLists.txt) only adds the shared
plugin target when `THEMIS_PLUGIN_IMAGE_ANALYSIS_ONNX` is enabled and
`onnxruntime` is found. OpenCV is optional at configure time. The current
implementation source does not invoke ONNX Runtime or OpenCV APIs; a successful
build or linked dependency is not evidence of model inference.

## Initialization

`initialize(config, backend)` takes the implementation mutex, reads the model
label and output dimension, replaces a non-positive dimension with `512`, and
resolves `BackendType::AUTO` to CPU. Other enum values are stored as the
selected backend; source code does not probe or execute those providers.
`max_batch_size` defaults to `16` for CPU and `64` for other selected backends,
then clamps to a minimum of `1`.

If both `model.path` and `model.expected_sha256` are non-empty, the path is
hashed with OpenSSL when built with `THEMIS_HAS_OPENSSL`. Without that define,
the process-wide `ModelHashFn` callback is used if installed. If no callback
exists, verification is skipped; hardened deployments must not rely on that
configuration. If the hash function cannot read the file or the digest differs,
initialization returns `false` before setting `ready`.

The top-level `enable_mmap_loading` flag attempts a read-only mapping of a
non-empty model path. Mapping failure is silently ignored. A successful mapping
is retained and cleaned up at shutdown/destruction, but the code explicitly
does not pass the mapped bytes to an inference session. Initialization can
therefore report ready without loading or parsing a model.

## Request path and state

Embedding methods snapshot `impl_` under `impl_swap_mtx_`, hold a `RequestGuard`
for in-flight accounting, and serialize state access using the implementation
mutex. Image/text vector generation itself is deterministic hash/PRNG work;
see [`MODELING.md`](./MODELING.md). A not-ready call returns a failed result.

Batch calls reserve and return one result per input, then process chunks in
sequence. `max_batch_size` affects chunk size only; no total input count or
maximum dimension is enforced. `shutdown()` cleans mapped resources and clears
the current ready flag.

## Reload publication and timeout

`reloadModel(new_config)` builds and validates a new implementation, including
optional digest verification, before publishing it. It rejects a reload when
the current plugin is not ready. Under the pointer-swap mutex it copies
cumulative counters and assigns `impl_ = new_impl`; only after publication does
it wait up to 30 seconds for requests holding the old snapshot to finish.

Consequently, a `false` return can mean either a pre-publication failure (the
old snapshot remains active) or a drain timeout (the new snapshot is already
active, while old requests may still be completing). The bool does not identify
which case occurred. Check readiness/statistics and apply an explicit reload
with the saved prior configuration if rollback is required. Do not assume a
timeout restored the old configuration.

`reloadModel()` reads backend as a string from the new config; `auto` and
`cpu` select CPU, recognized backend strings map to their enum, and unknown
strings fall back to CPU. Unlike initialization, reload does not attempt mmap
loading. Successful reloads carry cumulative counters forward.

## Failure handling and extension points

- Empty image/text payloads return specific failed `EmbeddingResult`s.
- Hash mismatch, hash read failure, or invalid lifecycle state returns `false`
  from initialize/reload without structured diagnostics beyond that bool.
- Mmap failure is not surfaced to the caller.
- There is no native-session error, image decode error, or provider fallback
  path in the current implementation.
- `setModelHashFn()` changes a process-wide callback under its own mutex; it is
  intended as a non-OpenSSL digest bridge, not as an inference extension point.

Before adding an actual ONNX provider, specify preprocessing and tokenizer
contracts, provider selection and errors, artifact trust, concurrency, model
reload semantics, and model-backed tests in the same change.

## Validation

Focused tests compile the plugin source directly when needed. See
[`tests/onnx_clip/README.md`](../../tests/onnx_clip/README.md) for existing
targets and CTest filters. The current focused suite validates the reference
contract; it does not prove that the linked ONNX Runtime package is exercised.
