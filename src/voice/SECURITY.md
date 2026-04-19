> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
# Security — Voice Module
> Report vulnerabilities via [SECURITY.md](../../../SECURITY.md).

## Threat Model

| Threat | Mitigation |
|--------|-----------|
| Audio eavesdropping | All audio streams encrypted in transit (TLS/SRTP) |
| Voice biometric spoofing (replay attack) | Liveness detection; freshness challenge in biometric auth flow |
| Transcript data exfiltration | Transcripts stored encrypted; access requires authentication |
| Unauthorized recording | Session creation requires authenticated user; explicit consent required |
| SIP/WebRTC injection attacks | SIP headers validated and sanitized; WebRTC ICE candidates validated |
| Sensitive content in transcripts | PII detector applied to transcripts before storage and logging |

## Security Controls
- All audio transport uses TLS 1.3 (WebSocket) or SRTP (WebRTC/SIP)
- Voice biometric templates stored as one-way features (not reversible to audio)
- Transcripts encrypted at rest via storage module encryption
- Meeting recordings require explicit participant consent flag
- Audit logging for all session creation and biometric authentication events

## Data Handling
- Audio buffers zeroed from memory after transcription
- Biometric templates stored separately from transcript data
- Retention policy for transcripts configurable; default 90 days

## Known Limitations
- Liveness detection requires client-side implementation for full anti-spoofing protection
