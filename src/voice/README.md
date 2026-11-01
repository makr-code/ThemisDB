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

> Exact model backend not verifiable from final document text alone.

# Voice Module

The voice module provides C++ interfaces and implementation for assistant
orchestration, audio preprocessing, wake-word/intent decisions, session
management, browser streaming, telephony, biometric helpers, security helpers,
storage, and related utilities. The actual work performed depends on the
enabled build features and injected application/back-end callbacks.

## Developer entry points

- Public types and contracts: [`../../include/voice/`](../../include/voice/)
- Implementation architecture and dataflow:
  [`ARCHITECTURE.md`](ARCHITECTURE.md)
- API summary and result semantics: [`API.md`](API.md)
- Integration sequence: [`MODULE_GUIDE.md`](MODULE_GUIDE.md)
- Session/stream limits and backend caveats:
  [`IMPLEMENTATION.md`](IMPLEMENTATION.md)
- Test and benchmark source registration:
  [`../../tests/voice/CMakeLists.txt`](../../tests/voice/CMakeLists.txt) and
  [`../../benchmarks/voice/CMakeLists.txt`](../../benchmarks/voice/CMakeLists.txt)

## Build boundaries

`THEMIS_ENABLE_VOICE_ASSISTANT` is an opt-in CMake feature and defaults to
`OFF`; it gates the server voice API handler. The voice implementation source
list in `cmake/CMakeLists.txt` is gated by `THEMIS_ENABLE_LLM`. These options
are distinct. `THEMIS_ENABLE_RNNOISE` also defaults to `OFF`; the preprocessing
implementation has a fallback when RNNoise is unavailable. Benchmarks require
`THEMIS_BUILD_BENCHMARKS`; source registration does not establish runtime
behavior or test/benchmark success.

## Important limitations

- `VoiceAssistant::Config` disables voice authentication and wake-word handling
  by default.
- Browser streaming uses placeholder transcript/TTS behavior without injected
  backends; do not present it as speech recognition/synthesis.
- In-process session, biometric, storage, and security helper state is not a
  durable or application authorization boundary.
- The module does not establish deployed transport, external backend,
  compliance, performance, or production-readiness guarantees.

See [`ROADMAP.md`](ROADMAP.md) for voice-specific evidence posture. Root
[`../../ROADMAP.md`](../../ROADMAP.md) remains authoritative for aggregate
project maturity and release blockers.
