---
Author: "ThemisDB Team"
Created: "2026-08-08"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Voice Integration Readiness Notes

This is a non-normative engineering checklist, not a production approval,
security certification, SLA, or compliance statement. Deployment requirements
must be defined and verified by the service that integrates the voice module.

## Source-supported checks

- Enable the intended CMake feature explicitly. `THEMIS_ENABLE_VOICE_ASSISTANT`
  defaults to `OFF`; RNNoise is separately optional.
- Configure application authentication, authorization, consent, origin policy,
  transport protection, durable audit and data-retention controls outside the
  voice helper APIs.
- Confirm each session/stream/backend limit against the relevant public header.
  `VoiceSessionManager`, `VoiceStreamingSession` and `VoiceStreamValidator`
  define different contracts.
- Supply and exercise real STT/TTS callbacks/backends. Browser streaming uses
  placeholder transcript and TTS behavior when those callbacks are absent.
- Decide whether in-memory session, biometric, storage and security state is
  sufficient. These interfaces do not provide durable persistence or
  cryptographic protection by themselves.
- Treat biometric feature similarity and heuristic liveness as limited signals,
  not calibrated identity assurance or application authorization.
- Verify build, test and benchmark results in the target environment. CMake
  registration is not execution evidence; this documentation update did not
  run builds, tests, or benchmarks.

## Before accepting deployment claims

Record the service revision, enabled build options, backend identities,
configuration, test/CI artifacts, representative-hardware results and owner
approval. Do not copy historical pass rates, performance targets, error-code
tables, configuration keys, audit retention values or compliance claims from
older voice documents without validating them against the current source and
deployment.

## Canonical references

- [`../../include/voice/`](../../include/voice/) — API defaults and contracts
- [`ARCHITECTURE.md`](ARCHITECTURE.md) — source/dataflow boundaries
- [`SECURITY.md`](SECURITY.md) and [`TRUST.md`](TRUST.md) — integration risks
- [`ROADMAP.md`](ROADMAP.md) and root [`../../ROADMAP.md`](../../ROADMAP.md) — evidence posture
