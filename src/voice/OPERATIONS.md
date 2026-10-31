---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Voice Operations Guide

> Exact model backend not verifiable from final document text alone.

**Audience:** Service owners sizing and operating a deployment that enables the Voice module. This page separates code-supported limits from deployment guarantees; actual availability, persistence, and performance depend on the integrating service and configured backends.

## Capacity and lifecycle controls

The following are source defaults, not SLOs. Configure and load-test them for the deployment:

| Resource | Current default / bound | Operational implication |
|---|---|---|
| Session idle timeout | 5 minutes | Inactive sessions should be cleaned up on the configured cleanup cycle |
| Session maximum duration | 30 minutes | Absolute session lifetime bounds long-lived state |
| Session cleanup interval | 30 seconds | Cleanup is periodic; it is not an instantaneous expiry guarantee |
| Session manager capacity | 1,000 per manager (documented) | Reject/observe creation failures rather than increasing without a capacity test |
| Browser stream manager capacity | 200 concurrent streams by default | Empty stream ID indicates capacity/creation failure |
| Browser frame size | 65,536 bytes by default; hard config maximum 65,536 | Apply client-side chunking and backpressure |
| Browser accumulated audio | 50 MiB per stream | Large/slow streams consume bounded memory and are rejected at the limit |
| Browser stream duration | 600 seconds by default; configuration maximum 3,600 | Close and recreate instead of extending an active stream indefinitely |

`VoiceStreamValidator` independently allows chunks up to 16 MiB. That is not the browser per-frame limit. Do not collapse these per-surface values into one global setting.

## Deployment readiness

Before enabling traffic, verify:

- The service explicitly owns authentication, authorization, origin policy, and recording/transcription consent.
- Real STT/TTS backends are initialized and exercised. Browser streaming's current built-in transcript and TTS paths are placeholders unless integrated; do not use them as a production capability.
- The compiled build options match the expected audio-conversion and denoising features. FFmpeg conversion is available only through the supplied/injected converter; RNNoise is compile-time conditional.
- Session persistence is appropriate. The default session backend is in-memory. The biometric profile maps and `VoiceSecurityManager` state are also in-process; process restart does not provide durable continuity.
- Storage, privacy, encryption/key management, audit retention, backups, and deletion are configured by the owning deployment. The audio storage header describes encryption metadata/key references, not an actual cryptographic implementation.
- The operator has access to the actual service's logs, metrics, and deployment-specific restart/rollback procedure. The module does not define a production CLI or guarantee external telemetry export.

## Routine checks

At service startup, run a non-sensitive fixture through the configured STT/TTS path and confirm a meaningful transcript/audio response. Inspect `VoiceSessionManager::getAnalytics()`, `VoiceStreamingManager::activeSessionCount()`, `VoiceIntentDetector::getStatistics()`, `WakeWordDetector::getStatistics()`, `VoiceBiometricAuthenticator::get_statistics()`, and storage/security statistics where those components are enabled.

During load tests, track active session/stream counts, rejected frame/sequence/format counts, auth denials, backend error/timeouts, teardown duration/errors, bytes buffered, queue growth, and p50/p95/p99 latency. Define alert thresholds from observed deployment baselines; the repository roadmap notes representative-hardware voice baselines remain pending.

## Change windows and rollback

For backend/model, threshold, frame-size, timeout, or concurrency changes, compare a canary against the current version using fixed fixtures and failure cases. Keep validation and auth fail-closed. Roll back configuration and backend versions together if error rates, false accepts, teardown failures, or resource use exceed approved limits. After rollback, verify new session creation, one complete non-sensitive audio round trip, rejection of a deliberately invalid frame, and cleanup of a completed session.

See [`RUNBOOK.md`](RUNBOOK.md) for incident response and [`PRODUCTION_REQUIREMENTS.md`](PRODUCTION_REQUIREMENTS.md) for the module's production requirement record. Treat the current source/header defaults as implementation facts; deployment policy and measured targets must be recorded by the owning service.
