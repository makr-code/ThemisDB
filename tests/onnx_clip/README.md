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

Exact model backend not verifiable from final document text alone.

# ONNX CLIP focused tests

The focused tests verify the current `ONNXClipPlugin` reference contract. They use
generated image bytes and synthetic/mock model files; they do not load CLIP weights,
execute ONNX Runtime, or establish semantic embedding quality.

## Test coverage

| Source | Coverage |
|---|---|
| `test_onnx_clip_golden_embeddings_focused.cpp` | Deterministic image/text vectors, dimensions, normalization, repeatability, batching, and statistics |
| `test_onnx_clip_hot_swap_focused.cpp` | Reload lifecycle, request draining, and concurrent request/reload behavior |
| `test_onnx_clip_mmap_focused.cpp` | Mapping resource lifecycle, fallback behavior, and unchanged reference outputs |

`tests/onnx_clip/CMakeLists.txt` registers sources matching `test_*_focused.cpp`
when `THEMIS_PLUGIN_IMAGE_ANALYSIS_ONNX=ON`. It creates one CTest entry per source
file. The mmap tests do not demonstrate memory reduction: mapped bytes are not
passed to an inference runtime. A reload drain timeout returns `false` after the
replacement implementation has already been published.

## Build and run

Configure with the module tests enabled, then build and run the registered tests:

```bash
cmake --preset linux-release -DTHEMIS_PLUGIN_IMAGE_ANALYSIS_ONNX=ON
cmake --build --preset linux-release
ctest --preset linux-release -R 'onnx_clip' --output-on-failure
```

The focused CTest names are based on each source filename and end in
`_OnnxClipFocused`. To inspect the exact names available in a configured build,
run `ctest --preset linux-release -N -R 'onnx_clip'`.

The repository's Windows validation command is:

```powershell
ctest --preset windows-release -R 'onnx_clip' --output-on-failure
```

## Related documentation

- `src/onnx_clip/README.md` — current behavior and limitations
- `src/onnx_clip/ARCHITECTURE.md` — component boundaries and runtime flow
- `src/onnx_clip/PRODUCTION_REQUIREMENTS.md` — deployment constraints and acceptance gates
