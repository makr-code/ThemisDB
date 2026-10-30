---
Author: ThemisDB Team
Created: 2026-10-30
Last Updated: 2026-10-30
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

Exact model backend not verifiable from final document text alone.

# Image Analysis Runbook

## Scope and first response

This runbook is for an operator or on-call engineer investigating OCR or
detection failures. Preserve the request/result error, plugin name/version,
build profile, configuration key names (redact sensitive filesystem details
as needed), model/data identity, and recent deployment changes. Do not log raw
images or OCR contents into an incident ticket unless approved by the data
owner.

## Triage sequence

1. **Check result before plugin status.** Record `success`, `error_message`,
   `model_name`, and `inference_time_ms`. A plugin may initialize and report
   ready even when compiled without its runtime; readiness alone is not an
   inference health check.
2. **Confirm build mode.** Inspect configure output and compile configuration
   for `THEMIS_PLUGIN_IMAGE_YOLOV8` / `THEMIS_PLUGIN_IMAGE_TESSERACT`, and
   whether `HAVE_ONNXRUNTIME` / `HAVE_TESSERACT` was enabled by package
   discovery. A target built without the native dependency deliberately
   returns an explicit inference error.
3. **Check image decoding mode.** Was OpenCV found and `HAVE_OPENCV` compiled?
   Without OpenCV, Tesseract treats bytes as raw grayscale pixels; YOLO
   supplies an all-zero tensor. Encoded image inputs in those paths are not
   validly decoded. Do not interpret resulting zero detections as a successful
   image analysis.
4. **Check runtime assets.** For Tesseract, verify the language and matching
   tessdata directory can initialize. For YOLO, verify the configured ONNX
   file is readable and compatible with the expected YOLOv8 output and label
   ordering. Avoid copying untrusted model files into a live service as a
   diagnostic.
5. **Reproduce with a known, approved fixture** in a non-production environment.
   The focused test is useful for API/stub behavior; it does not prove real
   inference. The soak/stress tests use stubs and are not diagnostic evidence
   for native model stability.

## Symptom-to-action

| Symptom | Likely cause / check | Safe action |
|---|---|---|
| `built without ... support` | Optional native runtime was not compiled in | Rebuild with dependency available and intended feature enabled; otherwise disable routing to this backend and surface unavailable status |
| Tesseract initialize fails | Missing/incorrect language data, path, or language | Validate `tessdata_path` and language in staging; restore previous known-good configuration |
| YOLO initialize fails with ONNX enabled | Missing/invalid model path or ONNX session creation error | Verify model provenance, readability, export compatibility, and provider setup; restore known-good model/config |
| Decode/preprocessing failure | Invalid bytes, unsupported format, or missing OpenCV decode path | Validate input contract upstream; do not silently retry with zero/raw pixels |
| Inference latency/error rate increases | Input size, provider, model, or resource pressure changed | Compare counters/average latency and deployment baseline; shed or pause image work at the caller boundary |
| `ready=true` but all operations fail | Stub build or actual runtime failure | Treat as unavailable; verify actual operation result, not readiness flag |

## Recovery and rollback

* Stop or route away image-analysis requests if results are unreliable or
  resource use is not bounded by the caller.
* Restore the previously approved plugin build, model/data asset, and
  configuration as one compatible set. Do not mix a previous labels file with
  a different detector model.
* Restart/reinitialize under the normal controlled deployment procedure and
  validate with an approved fixture before returning traffic.
* Preserve failed artifact identities and result metadata for investigation.
  Do not disable error reporting or treat fallback/stub output as valid work.
* Escalate repeated native crashes, suspected malicious model/image inputs,
  data disclosure, or unexplained false detections to module maintainers and
  the project security process ([root SECURITY.md](../../SECURITY.md)).

For recurring operational controls and readiness checks, see
[OPERATIONS.md](OPERATIONS.md); for security boundaries see
[TRUST.md](TRUST.md).
