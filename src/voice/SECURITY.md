---
Author: "ThemisDB Team"
Created: "2026-05-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Security Boundaries — Voice

This page describes in-tree security-related surfaces, not a security
certification, compliance assertion, or deployment policy. The integrating
service remains responsible for authorization, consent, transport protection,
durable audit, retention, and key management.

## Source-backed surfaces

- `VoiceAssistant::Config::enable_voice_auth` defaults to `false`; biometric
  checks do not automatically gate all session or command paths.
- `VoiceBiometricAuthenticator` compares handcrafted acoustic features and
  includes a separate heuristic liveness path. Neither is proof of identity or
  authorization, and no calibrated accuracy is established here.
- `VoiceSecurityManager` exposes helper operations for redaction, consent,
  audit, rate limiting, data export/deletion and statistics. Its in-memory
  state is not a durable compliance ledger.
- `VoiceAudioStorage` stores records in process-owned state; metadata fields
  such as `encrypted`/key references do not provide cryptographic encryption.
- Browser streaming validates its own frame/session inputs. An empty origin
  allowlist is permissive; a configured allowlist uses exact matching.
- Telephony/streaming interfaces and their tests do not alone prove deployed
  SIP/WebRTC connectivity or protocol conformance.

## Integration requirements

Treat audio, transcripts, origin values, IDs and backend output as untrusted.
Bind sessions to an authenticated application identity, apply authorization
again before privileged actions, enforce consent and retention externally, and
avoid logging raw audio, transcripts, biometric features, credentials or
tokens. Route audit records to an approved durable sink if durable evidence is
required.

For code-level controls and limits see
[`../../include/voice/voice_auth.h`](../../include/voice/voice_auth.h),
[`../../include/voice/voice_security.h`](../../include/voice/voice_security.h),
[`../../include/voice/voice_audio_storage.h`](../../include/voice/voice_audio_storage.h),
[`../../include/voice/voice_browser_streaming.h`](../../include/voice/voice_browser_streaming.h),
and [`TRUST.md`](TRUST.md). Report vulnerabilities through the repository's
root [`../../SECURITY.md`](../../SECURITY.md).
