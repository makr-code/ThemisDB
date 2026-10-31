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

# Voice Trust Boundaries

> Exact model backend not verifiable from final document text alone.

**Audience:** Security reviewers, service integrators, and maintainers handling audio, transcripts, sessions, or speaker profiles. Audio-derived data is sensitive; these module APIs do not by themselves establish caller identity, consent, transport security, or durable compliance.

## Trust boundaries

1. **Client and transport → voice adapter:** Audio bytes, codec metadata, declared origin, session identifiers, and transcript callbacks are untrusted. Enforce authentication/authorization at the service boundary, validate format/size/ordering, and configure an origin allowlist. An empty browser-stream `origin_allowlist` allows any origin; a populated list uses exact, case-sensitive matching.
2. **Voice pipeline → STT/LLM/TTS backend:** Treat transcripts, intent results, generated responses, model output, and backend errors as untrusted. Validate intent/confidence before action dispatch, apply application authorization to every privileged operation, and avoid returning raw backend errors to clients.
3. **Biometric processing → profile store:** Audio, handcrafted speaker features, scores, and profile IDs are biometric/sensitive data. In-memory profile storage is not a durable or access-controlled identity provider. `VoiceAssistant::Config::enable_voice_auth` defaults to false, so integrations must not assume biometric gates run.
4. **Voice data → storage/audit:** `VoiceAudioStorage` stores audio bytes and metadata in process-owned maps. Its `encrypted` marker and key ID describe metadata; the header explicitly does not provide an actual cryptographic dependency. `VoiceSecurityManager` exposes consent, audit, redaction, and deletion operations, but its implementation state is in memory and does not prove durable retention/deletion.
5. **Operator diagnostics → incident records:** Session IDs and error details can themselves be sensitive. Minimize access and retention; do not place raw audio, full transcripts, biometric features, authentication tokens, or secrets in general logs or tickets.

## Risks and controls

| Risk | Required control / residual limit |
|---|---|
| Replay, synthesis, or speaker mismatch | Use the configured liveness plus verification checks for the relevant workflow; reject failed/ambiguous results. Current liveness is heuristic and is not a guarantee against modern spoofing. |
| Forged or oversized stream input | Enforce per-boundary format, size, sequence, duration, and session-state checks. A successful validation only establishes the implemented checks, not that content is benign. |
| Prompt/command injection through transcript | Treat speech transcript as user-controlled text; apply shared prompt-safety policy where LLM prompts are built, and independently authorize resulting actions. |
| Transcript/audio disclosure | Obtain and enforce recording/transcription/retention consent in the integrating service; restrict access and configure secure storage/transport outside the in-memory module helper. |
| False biometric decision | Do not use similarity or liveness alone as authorization. Calibrate on representative independent data and retain an alternate recovery/access path under policy. |
| Missing audit evidence | Route audit events to an approved durable, access-controlled sink. A successful in-process call is not proof of durable logging. |
| Browser origin misuse | Configure an explicit allowlist at the boundary; empty configuration is permissive. Also apply authenticated session binding and transport protections. |

## Incident handling

For suspected unauthorized access, replay, data exposure, or audit loss: stop affected intake without disabling other security controls; preserve only permitted diagnostic metadata; revoke impacted sessions/credentials through the owning service; follow the organization's privacy/security incident process; and assess whether recordings, transcripts, profiles, or backups require deletion or notification. Verify the service's durable evidence before reporting that deletion or containment completed. See [`RUNBOOK.md`](RUNBOOK.md).

## Source contracts

- [`voice_security.h`](../../include/voice/voice_security.h) defines security helper operations and default configuration.
- [`voice_auth.h`](../../include/voice/voice_auth.h) defines biometric features, thresholds, and result types.
- [`voice_audio_storage.h`](../../include/voice/voice_audio_storage.h) describes storage records and encryption metadata.
- [`voice_browser_streaming.h`](../../include/voice/voice_browser_streaming.h) defines stream inputs and origin matching behavior.
- [`SECURITY.md`](SECURITY.md) describes the related module risk controls; [`PRODUCTION_REQUIREMENTS.md`](PRODUCTION_REQUIREMENTS.md) remains the existing module production requirement record.
