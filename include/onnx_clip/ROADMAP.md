---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "ThemisDB documentation orchestration assistant"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_Sources:
  - "src/onnx_clip/onnx_clip_plugin.h"
  - "src/onnx_clip/onnx_clip_plugin.cpp"
  - "src/onnx_clip/ROADMAP.md"
Related_Issue: "#7365"
---

Exact model backend not verifiable from final document text alone.

# ONNX CLIP documentation directory status

## Current state

`include/onnx_clip/` contains documentation, not C++ headers. The
`ONNXClipPlugin` declaration lives at
[`../../src/onnx_clip/onnx_clip_plugin.h`](../../src/onnx_clip/onnx_clip_plugin.h)
and uses namespace `themis::plugins::image`. It implements the shared
`IImageAnalysisBackend` contract. The implementation produces deterministic
reference vectors; it does not execute ONNX Runtime. See the canonical
[`src/onnx_clip/ROADMAP.md`](../../src/onnx_clip/ROADMAP.md) for runtime status.

## Proposed documentation work

The following items are retained from an older planning list. They are proposals,
not evidence of an approved schedule or implementation. Their former Q3/Q4 2026
target dates have passed and need maintainer confirmation before being reused:

- Add a contract index and usage examples for the source-local header.
- Document compiler/platform compatibility based on actual build coverage.
- Add compile checks for the declared header if maintainers want that as a CI
  contract.
- Review deprecation notices and ABI/versioning policy before changing the
  interface.

No C++ header under `include/onnx_clip/`, `themis::onnx_clip` namespace, or
Tensor/LLM layer association is established by the current declaration.

## References

- [`README.md`](./README.md) — source-local header and API summary
- [`../../src/onnx_clip/ROADMAP.md`](../../src/onnx_clip/ROADMAP.md) — canonical
  implementation roadmap
- [`../../src/onnx_clip/onnx_clip_plugin.h`](../../src/onnx_clip/onnx_clip_plugin.h)
  — current declaration
