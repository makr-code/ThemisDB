> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-07 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Whisper Plugin

> For reporting security vulnerabilities, see the project-level [SECURITY.md](../../SECURITY.md).

## Security Scope

This document covers the security posture of the Whisper audio transcription plugin:
`WhisperPlugin`, `WavAudioChunkReader`, `WhisperCppTranscriber`, `WhisperConfig`.

---

## Threat Model

| Threat | Attack Vector | Mitigation |
|--------|--------------|------------|
| Path traversal via audio file path | Attacker supplies `../../etc/passwd` as audio path | `WavAudioChunkReader` opens only regular files; path is not further interpreted; callers should normalise paths before passing |
| Malformed WAV file causing buffer overflow | Crafted WAV with oversized chunk size headers | Parser validates all chunk sizes against the remaining file size; no heap allocation proportional to untrusted header values |
| Model file tampering | Whisper model replaced with a crafted binary | SHA-256 digest check before model load via `WhisperConfig.model_sha256`; mismatch aborts initialization |
| Transcript exfiltration via log sink | Transcription text written to log files | Transcript text is never passed to any log sink; only metadata (model_id, duration_ms, success flag) is logged |
| Denial of service via large audio file | Attacker submits a multi-hour WAV file | `WavAudioChunkReader` reads the file into memory; callers should enforce a maximum file size limit before calling `transcribe()` |
| Inference resource exhaustion | Flood of `transcribe()` calls consuming all CPU/GPU | Rate limiting must be applied at the API layer; `WhisperPlugin` itself has no rate limiter in v2.0.0 |

---

## Security Controls

### Audio Input Validation
- RIFF magic bytes (`RIFF`, `WAVE`) validated before any data is read into memory.
- PCM format chunk validated: only 16-bit integer and 32-bit float samples accepted.
- File size is checked against the declared data-chunk length before allocation.

### Model Integrity
- SHA-256 digest verification of the model file is supported through `WhisperConfig.model_sha256`.
- Initialization fails when the computed digest does not match the expected value.

### Provenance
- Every `TranscriptionResult` carries `ingestion_source_type="WHISPER"` and `plugin_version`,
  allowing downstream audit trails to attribute transcripts to this plugin version.

---

## Security Checklist (v2.0.0)

- [x] Magic-byte / format validation in WAV parser
- [x] No transcript text in log output
- [x] Exception isolation: transcriber exceptions caught, not propagated to caller
- [x] Model file integrity check (SHA-256 gate in `WhisperCppTranscriber::initialize()`)
- [ ] Maximum audio file size enforcement (caller responsibility; to be documented in API layer)
- [ ] Rate limiting (caller / API layer responsibility)
- [x] Thread-safety audit
