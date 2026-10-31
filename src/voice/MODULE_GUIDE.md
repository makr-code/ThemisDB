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

# Voice Module Integration Guide

> Exact model backend not verifiable from final document text alone.

**Audience:** Application developers wiring voice processing into a ThemisDB service. This guide describes a safe integration order; it does not provide a standalone command-line service or promise that every optional backend is present.

## Integration sequence

1. **Select the owned interface.** Use `VoiceAssistant` for combined command processing, `VoiceSessionManager` for lifecycle ownership, `VoiceStreamingManager` for browser-style chunks, or `VoiceTelephonyManager` for call integration. The public declarations are under [`include/voice`](../../include/voice).
2. **Establish caller authorization and consent first.** Authenticate the application user and apply the deployment's recording/transcription-consent policy before creating privileged or persistent work. `enable_voice_auth` defaults to `false`; do not assume biometrics gate sessions automatically.
3. **Validate and normalize input.** Confirm codec, sample rate, channel count, bit depth, and duration at the transport adapter. For `VoiceStreamingSession`, reject empty or frame-misaligned PCM and respect its per-frame limit. Keep validator, stream-frame, accumulated-buffer, and session-duration limits distinct; see [`IMPLEMENTATION.md`](IMPLEMENTATION.md).
4. **Create and own a session.** Supply a non-empty user ID to `VoiceSessionManager::createSession`; an empty returned session ID means failure. Record the ID only in logs/telemetry that are permitted for the deployment. Set explicit idle and maximum-duration values for the workload.
5. **Connect real STT/TTS dependencies.** Initialize the configured backend and verify a known fixture before accepting traffic. Browser streaming without an injected STT backend returns synthetic placeholder transcripts; its TTS callback also currently echoes placeholder transcript bytes. Do not expose this behavior as speech recognition or synthesized speech.
6. **Process only final text for commands.** Treat partial transcript output as provisional. Validate intent category/confidence; `UNKNOWN` or low confidence should not dispatch a privileged action. Perform normal authorization again at the command/application boundary.
7. **Close and verify cleanup.** End the stream, terminate the session on completion/error, and inspect manager state/diagnostic results. A timed-out or terminal session is not reusable; create a fresh session after resolving the cause.

## Configuration checks

- Browser streaming `max_frame_bytes` must be in `(0, 65536]`; `max_duration_s` must be in `(0, 3600]`. Defaults are 65,536 bytes and 600 seconds.
- Session manager defaults are five-minute idle timeout, 30-minute maximum duration, and 30-second cleanup interval. Its documented capacity is 1,000 per manager; verify deployment-specific memory/capacity independently.
- Browser stream manager defaults to 200 concurrent streams; creation at capacity returns an empty stream ID.
- Wake-word input is 16-bit little-endian PCM at the configured sample rate. Its detection callback is synchronous; keep it short.
- When `origin_allowlist` is empty, browser origin checks allow any origin. Configure a non-empty exact allowlist when origin restriction is required.

## Error handling and integration tests

Map empty IDs, `false`, `nullopt`, rejected chunks, callback errors, `UNKNOWN`, and backend failure markers to explicit application outcomes. Never retry authentication/authorization denials as if they were transient. Retry transient backend/network failures only under the caller's idempotency policy; session creation or command execution may have side effects.

Before rollout, test one valid request and each rejected boundary: empty audio, wrong frame alignment, maximum frame + 1, invalid sequence, expired/terminated session, backend unavailable, low-confidence intent, auth rejection, and teardown during active work. Relevant targets live in [`tests/voice/CMakeLists.txt`](../../tests/voice/CMakeLists.txt); no test pass is implied by registration alone.

See [`API.md`](API.md), [`TRUST.md`](TRUST.md), and [`RUNBOOK.md`](RUNBOOK.md) for contracts, trust assumptions, and recovery.
