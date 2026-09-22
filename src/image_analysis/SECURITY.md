# Security - Image Analysis Module

<!-- Status: current | validated: 2026-09-21 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

Report vulnerabilities via the project-level SECURITY.md at the repository root.

## Security Scope

Security in the image analysis module focuses on safe handling of untrusted image data, backend isolation and graceful degradation, resource bounding during processing, and prevention of path traversal or injection via image paths and extracted content.

## Threat Model

| Threat | Current Mitigation Surface |
|---|---|
| Malicious or malformed image data causing crashes or buffer overflows | image format validation before backend dispatch; error taxonomy E6200–E6201 for unsupported/corrupted images |
| Path traversal via image file paths passed to backends | caller is responsible for path validation; module accepts only validated paths per API contract |
| Denial of service via large or slow-to-process images | configurable per-operation timeout (E6203); memory bound < 50 MB per image during processing |
| Backend model tampering (ONNX model substitution) | pre-loaded model is validated at startup; custom model support not yet active (see FUTURE_ENHANCEMENTS.md for integrity gate requirement) |
| Remote code execution via OCR output injection | extracted text is returned as data; callers are responsible for sanitising extracted content before use in queries or display |
| Backend unavailability escalating to full indexing failure | graceful degradation: OCR and detection failures return partial results; indexing pipeline continues |

## Implemented Security Controls

- Image format and integrity validation is performed before backend dispatch; corrupted or unsupported images return E6200/E6201 without invoking backend native code paths.
- All backend operations are bounded by configurable timeouts; exceeded deadlines return E6203 without hanging the caller.
- Memory usage during processing is bounded (< 50 MB per image); oversized images trigger E6204.
- Backend unavailability does not propagate as a hard failure; partial results are returned with explicit error flags.
- Tesseract plugin uses a `HAVE_TESSERACT` compile-time guard; without Tesseract the fallback path returns a safe graceful-degradation result without invoking any native OCR code.

## Security Follow-ups

- Verify that all callers of `extractText()` sanitise OCR output before using it in query construction or user-facing display (injection risk is with callers, not this module).
- When custom model loading is introduced (see FUTURE_ENHANCEMENTS.md), add HMAC/SHA-256 model integrity verification before model activation.
- Add explicit path validation within the module API for image file paths to reduce reliance on caller-side validation.

## Sourcecode Verification (Module: image_analysis/security)

- Verified files:
  - src/image_analysis/tesseract_ocr_plugin.cpp
  - src/image_analysis/yolov8_onnx_plugin.cpp
  - include/image_analysis/image_processor.h
- Verified controls:
  - Format validation and error codes E6200–E6204 present.
  - Timeout enforcement pattern present in both plugins.
  - `HAVE_TESSERACT` and `HAVE_OPENCV` compile guards prevent unsafe native calls in environments without those libraries.
  - No remote code execution vectors identified in current implementation.
