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

# Image Analysis Operations

## Purpose and operational boundary

This document defines routine deployment and monitoring practices for the
current plugin implementations. The module does not itself provide a request
queue, admission control, timeout, cache, or hard image-size limit. The
embedding/cache/throughput values in
[PERFORMANCE_EXPECTATIONS.md](PERFORMANCE_EXPECTATIONS.md) are targets and
must not be reported as measured service levels without fresh benchmark
evidence.

## Deployment readiness

Before enabling traffic, record which plugin targets are built, optional
dependency discovery results, OpenCV presence, model/tessdata versions, and
effective `PluginConfig`. Validate real inference using approved fixtures:

* Tesseract must load the requested language data and return an expected OCR
  transcript/word regions.
* YOLO must load the intended ONNX model, decode a real image through OpenCV,
  and return structurally valid normalized boxes for known test inputs.
* A successful initialization or health check is insufficient in a build
  without the native inference dependency.

Keep a known-good matching set of plugin binary, model, labels, and
configuration for rollback. Stage model/config changes independently before
production rollout.

## Signals and interpretation

Both plugin implementations expose `getStatistics()` JSON with inference
total, errors, average latency, and detections/words total (plus backend or
configuration details). These are per-plugin counters/averages, not a
Prometheus exporter or a P99 histogram. Sample at a controlled interval,
calculate deltas, and compare with deployment baselines. Do not expose
filesystem paths, OCR output, or raw image data in broadly accessible
telemetry.

At the caller boundary monitor request count, failure rate, latency
distribution, input byte/pixel dimensions, queue pressure, and process memory.
The plugin's own implementation does not enforce resource budgets. Alert on
repeated `success=false`, rising inference errors, unexpected latency or
resource growth, and a mismatch between configured and active backend.

## Routine change and recovery controls

1. Verify model/config provenance and compatibility before rollout; labels
   must correspond to model class indices and Tesseract data to configured
   language.
2. In staging, perform initialization plus real inference and compare known
   output, errors, latency, and memory with the established baseline.
3. Roll out gradually with caller-side concurrency, input-size, and timeout
   controls. These bounds are not supplied by the plugin.
4. If behavior regresses, stop new traffic to the backend and restore the
   last validated binary/model/labels/config set. Reinitialize and repeat the
   fixture check before resuming.
5. Preserve build configuration and artifact hashes/identities for
   investigation; escalate integrity or privacy concerns through
   [TRUST.md](TRUST.md) and the project
   [security reporting process](../../SECURITY.md).

## Readiness boundary

The module roadmap still calls for current release-profile benchmark and
fallback evidence. Do not declare GA, SLA compliance, or performance-target
attainment from this operational procedure or the stub-based tests. See
[ROADMAP.md](ROADMAP.md) and [RESEARCH.md](RESEARCH.md).
