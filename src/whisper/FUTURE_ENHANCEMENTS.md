---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Future Enhancements — Whisper Plugin

## Scope

Open implementation gaps beyond the current `2.3.0` API. Core implementation is in `src/whisper/whisper_plugin.cpp`,
`src/whisper/whisper_transcriber.cpp`, and `src/whisper/audio_chunk_reader.cpp`.

---

## Design Constraints

- `IAudioBackend` interface must remain stable; any new capability must be added as an
  optional method with a default implementation.
- `WhisperPlugin` owns provenance: it sets source/version fields on results and generation timestamps
  on successful results; existing error paths may omit the timestamp.
- `WavAudioChunkReader` must not depend on `libsndfile`, FFmpeg, or any other audio library;
  additional format support must go through adapter classes implementing `IAudioChunkReader`.
- Stub mode (no model file) must continue to produce valid `TranscriptionResult` objects
  after any enhancement.

---

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `IAudioBackend::transcribeStream(pcm, sample_rate, callback)` | Streaming transcription API | API exists; current whisper.cpp backend inherits the one-completed-text-token fallback |
| `IAudioChunkReader::readFile(path, out_sample_rate)` | `WhisperPlugin` | Implemented by WAV and FFmpeg readers; composite reader dispatches by extension |
| `IWhisperTranscriber::diarize(pcm, sample_rate, cfg)` | Speaker diarisation API | Optional method exists; current real backend does not distinguish speakers |

---

## Planned Features

### 1. Genuine Incremental Token Output (Target: TODO)

**Current state:** `transcribeStream()` and its callback are public, but the default
`IWhisperTranscriber` implementation calls `transcribe()` and emits the completed text as one
token. `WhisperCppTranscriber` does not override this method. The interface therefore does not
currently provide incremental whisper.cpp inference.

**Required change:** Implement a real whisper.cpp token/segment callback path without claiming
background-thread execution unless the implementation creates and manages that worker.

**Inputs/outputs:** Existing PCM vector, sample rate, `StreamCallback`, and final
`TranscriptionResult`; preserve the existing public signature.
**Errors:** The base `IWhisperTranscriber::transcribeStream()` catches any callback exception and
returns `success=false`; `WhisperPlugin` catches `std::exception` escaping a transcriber call.
Non-standard exceptions escaping an override may propagate. Document whether partial output is
retained.
**Tests:** Keep injected multi-token callback tests and add a real-model integration case.
**Performance:** TODO: define and measure first-token latency on named hardware and model.

---

### 2. Speaker Diarisation Quality (Target: TODO)

**Problem:** Transcripts from multi-speaker recordings are not attributed to individual speakers.

**Current state:** `DiarisationResult`, `DiarisationConfig`, and
`WhisperPlugin::transcribeWithDiarisation()` exist. The base method returns an empty result;
`WhisperCppTranscriber::diarize()` currently transcribes the audio and returns a single segment
assigned to one speaker. This is not multi-speaker diarisation.

**Required change:** Select and integrate a supported speaker-separation backend; do not assume a
specific external model/runtime until dependency and licensing choices are approved.

**Inputs:** mono float32 PCM samples, sample rate, and
`DiarisationConfig { min_speakers, max_speakers }`.
**Outputs:** `DiarisationResult { segments: [{speaker_id, start_ms, end_ms, text}] }`.
**Constraints:** Optional feature; `DiarisationResult` is separate from `TranscriptionResult`.
**Errors:** Define explicit behavior for unavailable or failed diarisation while preserving
transcription results.
**Tests:** Unit tests for configuration/error behavior plus real-model tests with multi-speaker
fixtures. TODO: establish quality metric and acceptance threshold.

---

### 3. Additional Audio Formats

**Current state:** `FfmpegAudioChunkReader` recognizes MP3, OGG, FLAC, M4A, AAC, Opus, WMA, and
WebM filename extensions and invokes the `ffmpeg` executable to produce 16 kHz mono float32 PCM.
The reader is optional at runtime; a missing executable causes a read error for those formats,
not automatic fallback to WAV.

TODO: add content-based format validation and integration tests for supported formats; current
routing is based on the filename extension.

---

### 4. VAD Validation and Behavior

**Current state:** `EnergyThresholdVad` implements frame-based RMS thresholding, and
`WhisperPlugin::transcribeStream()` applies it when configured. `transcribe()` does not apply VAD.

TODO: define and benchmark VAD accuracy/latency on representative noisy audio before adding an
alternate model-backed detector. Current unit tests cover silence, speech, mixed frames, and
plugin streaming integration; they do not establish production accuracy.

---

## Security / Reliability

- New reader adapters must validate input handling and subprocess arguments; extension checks
  alone do not validate file content.
- If an additional diarisation model is adopted, define its provenance, integrity, distribution,
  and resource limits before implementation.
