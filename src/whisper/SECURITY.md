---
Author: "ThemisDB Team"
Created: "2026-04-07"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-10-31 -->
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
| Caller-controlled audio path | Untrusted path is passed to `transcribeFile()` | This module does not enforce an allowed-root policy; callers must authorize/normalize paths. WAV uses file I/O and FFmpeg uses a shell-quoted argument. |
| Malformed WAV | Crafted RIFF chunks or format fields | Parser checks RIFF/WAVE markers, accepted sample formats, and channel count (1–64); it reads the whole file into memory and has no WAV size cap. |
| Model file mismatch | Model contents differ from configured expected digest | Optional `model_sha256` is checked before load by `WhisperCppTranscriber`; no digest means no integrity check. |
| FFmpeg input/output abuse | Malicious or oversized media passed to the subprocess | Filename extension selects the reader; FFmpeg decodes content. Paths containing NUL are rejected, shell arguments are quoted, and raw output is capped at 500 MiB. No MIME/content allowlist is implemented. |
| Transcript exposure in logs | Transcript text is written to module logs | Current whisper module code does not log transcript text; deployments should review caller/API logging separately. |
| Resource exhaustion | Large WAV or many inference requests | WAV has no module-level file-size cap; FFmpeg output is capped. Calls through one plugin are serialized, but there is no rate limiter or request timeout here. |

---

## Security Controls

### Audio Input Validation
- RIFF/WAVE markers are checked after the file has been read into memory.
- PCM format accepts 16-bit integer and 32-bit float samples; channel count must be in `[1, 64]`.
- Data length is limited to the bytes remaining in the loaded file; this is not a bound on total WAV allocation.
- FFmpeg output is converted to 16 kHz mono float32 and limited to 500 MiB.

### Model Integrity
- SHA-256 verification is optional through `WhisperConfig.model_sha256` and is performed only by
  the whisper.cpp backend before model load.
- A digest mismatch or digest-read failure aborts model initialization. This does not establish
  publisher authenticity unless the expected digest is obtained through a trusted channel.

### Provenance
- Every `TranscriptionResult` carries `ingestion_source_type="WHISPER"` and `plugin_version`,
  allowing downstream audit trails to attribute transcripts to this plugin version.

---

## Current module controls and gaps

- [x] Magic-byte / format validation in WAV parser
- [x] FFmpeg path argument quoting, NUL rejection, and 500 MiB output limit
- [x] Optional expected model SHA-256 check in `WhisperCppTranscriber::initialize()`
- [x] `WhisperPlugin` converts `std::exception` failures from transcription/read paths to error results
- [ ] WAV file-size limit (not enforced by this module; caller responsibility)
- [ ] Request rate limiting and timeout (not implemented by this module; caller responsibility)
- [ ] Content-based format validation for FFmpeg inputs (routing is extension-based)
- [ ] Independent security audit/model supply-chain verification (not established by source inspection)
