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

# Voice Incident Runbook

> Exact model backend not verifiable from final document text alone.

**Audience:** ThemisDB operators and on-call maintainers. This is a code-level triage guide for voice session, stream, and backend failures. The module does not define a standalone voice admin CLI; use the owning service's authenticated diagnostics and logs.

## First response

1. Identify affected integration path: assistant command, session manager, browser stream, or telephony.
2. Record a correlation/session ID, UTC time window, error callback/log text, backend name, and deployment/build configuration. Do not copy raw audio, transcripts, biometric vectors, tokens, or caller identifiers into a general incident ticket.
3. Stop admitting new affected work if the failure is broad or a control is failing open. Preserve the existing audit/diagnostic evidence according to the service's retention policy.
4. Do not disable stream validation, authorization, consent checks, or liveness gates as an incident workaround.

## Symptom-to-diagnosis

| Symptom | Check | Safe response |
|---|---|---|
| Session creation returns an empty ID | Verify non-empty user ID, session manager capacity, persistence backend availability, and session creation error logs. `VoiceSessionManager` exposes `getAnalytics()` and `getSessionState()`. | Reject the request; correct identity/configuration or recover backend/capacity. Do not create a ghost session. |
| Stream creation returns an empty ID | Check `VoiceStreamingManager::activeSessionCount()` against its configured maximum. | Apply backpressure or reduce intake; close completed streams only after confirming ownership. |
| Chunks are rejected / stream ends unexpectedly | Check frame size, PCM alignment, sample rate/channel/bit depth, sequence continuity, stream age, and validator/manager error callback. | Correct the client encoding or start a fresh stream. Do not replay an unknown partial command. |
| Transcript is empty, synthetic, or unchanged by audio | Check whether a real transcription backend was injected and initialized. The browser path emits placeholder text if not; finalization is also placeholder-based. | Keep that path out of production traffic until a real backend is wired and verified. Do not diagnose placeholder output as an STT model-quality regression. |
| Intent is `UNKNOWN` or confidence is low | Check transcript quality, configured confidence threshold, timeout statistics, and entity/context inputs. | Ask for a repeat/clarification or return a non-privileged safe response; do not dispatch a destructive command. |
| Biometric authentication is denied | Check profile existence, sample format/duration/quality, liveness result, threshold, and audit callback outcome. | Deny access. Only enroll/retry after user and policy verification; never lower thresholds during an incident without security approval and evaluation evidence. |
| Teardown is slow or fails | Inspect `getSessionTeardownStatus()`, state, elapsed time, error code, and active counts. Use `terminateSessionWithTimeout()` / `terminateAllSessions()` only with a deliberate timeout. | Stop new work, attempt guarded termination, and escalate if resources remain. Do not reuse a closing/terminated session. |

## Recovery and rollback

- **Backend outage:** fail closed for operations that require speech or authentication. Restore/verify the configured STT/TTS service or revert the backend/configuration change; run a known non-sensitive fixture before reopening intake.
- **Malformed or oversized traffic:** reject at the boundary, retain bounded diagnostic metadata, and rate-limit abusive clients at the owning transport. Do not raise limits just to suppress rejections.
- **Resource pressure:** reduce admitted concurrency and drain sessions through their supported close/termination path. Restart only after capturing IDs, active counts, and teardown errors needed for investigation.
- **Regression after release:** roll back the voice component/configuration as a unit using the deployment's established release procedure. Reopen traffic gradually and verify session creation, stream rejection controls, transcript backend, and audit evidence.

Escalate security denials, suspected replay/biometric abuse, transcript exposure, audit failure, or any fail-open condition to the security/privacy owner. Escalate repeated backend failures or resource exhaustion to the voice service and ML/infrastructure owners.

## Observability and references

Use available signals from the module APIs: session analytics and teardown status; stream active count, bytes received, error callback; wake-word and intent statistics; biometric auth audit callback; security audit/denial records; and storage statistics. These APIs do not themselves guarantee external metric export or durable audit retention.

For deployment-specific service checks, consult [voice pipeline operations](../../docs/operability/RUNBOOK_VOICE_PIPELINE.md) and [incident triage](../../docs/operability/RUNBOOK_VOICE_INCIDENT_TRIAGE.md), verifying every command/config key against the deployed service before use. Source contracts: [`voice_session_manager.h`](../../include/voice/voice_session_manager.h), [`voice_browser_streaming.h`](../../include/voice/voice_browser_streaming.h), [`voice_auth.h`](../../include/voice/voice_auth.h).
