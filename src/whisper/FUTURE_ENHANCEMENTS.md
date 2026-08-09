> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-08-09 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Future Enhancements — Whisper Plugin

## Scope

Planned enhancements beyond v2.0.0. Core implementation is in `src/whisper/whisper_plugin.cpp`,
`src/whisper/whisper_transcriber.cpp`, and `src/whisper/audio_chunk_reader.cpp`.

---

## Design Constraints

- `IAudioBackend` interface must remain stable; any new capability must be added as an
  optional method with a default implementation.
- Provenance stamps (`ingestion_source_type`, `plugin_version`, `generation_timestamp`)
  must always be applied by `WhisperPlugin`, never by the transcriber.
- `WavAudioChunkReader` must not depend on `libsndfile`, FFmpeg, or any other audio library;
  additional format support must go through adapter classes implementing `IAudioChunkReader`.
- Stub mode (no model file) must continue to produce valid `TranscriptionResult` objects
  after any enhancement.

---

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `IAudioBackend::transcribeStream(stream, cfg, callback)` | Streaming transcription API | Token-by-token callback; both engines must implement it |
| `IAudioChunkReader::read(path)` | `WhisperPlugin` | Existing interface extended with MP3/OGG via `FfmpegAudioChunkReader` adapter |
| `IWhisperTranscriber::diarize(chunk, cfg)` | Speaker diarisation API | New optional method |

---

## Planned Features

### 1. Streaming Token Output (Target: Q3 2026)

**Problem:** Current `transcribe()` returns only after the entire audio is processed,
blocking the caller for long recordings.

**Solution:** Add `transcribeStream(stream, cfg, callback)` to `IAudioBackend`. The
`WhisperCppTranscriber` calls `whisper_full_with_state()` in segments and invokes the
callback after each word/segment token.

**Inputs:** Raw `AudioChunk` stream (iterator or callback), `TranscriptionConfig`.
**Outputs:** Token-by-token `TranscriptionToken` via callback; final `TranscriptionResult`.
**Constraints:** Callback must not block; the transcriber runs on a background thread.
**Errors:** Callback exception → `success=false`; partial result returned.
**Tests:** Unit test with `InMemoryWhisperTranscriber` emitting 3 tokens then completing.
**Perf target:** ≤ 50 ms first-token latency on 16-core build with stub.

---

### 2. [~] Speaker Diarisation (In Progress, Target: Q4 2026)

**Problem:** Transcripts from multi-speaker recordings are not attributed to individual speakers.

**Status update (v2.3.0):** Optional `diarize(pcm, sample_rate, cfg) -> DiarisationResult` is available in
`IWhisperTranscriber` and orchestrated by `WhisperPlugin::transcribeWithDiarisation()`.

**Next step:** `WhisperCppTranscriber` quality path integration with a dedicated speaker-embedding model
(e.g., pyannote-onnx) for improved multi-speaker attribution.

**Inputs:** `AudioChunk`, `DiarisationConfig { min_speakers, max_speakers }`.
**Outputs:** `DiarisationResult { segments: [{speaker_id, start_ms, end_ms, text}] }`.
**Constraints:** Optional feature; `DiarisationResult` is separate from `TranscriptionResult`.
**Errors:** Missing speaker model → feature disabled; transcription still succeeds.
**Tests:** 5 unit tests with stub returning preset diarisation fixtures.

---

### 3. MP3 / OGG Input Support ✅ Implemented in v2.1.0

**Status:** Implemented via `FfmpegAudioChunkReader : IAudioChunkReader` and `CompositeAudioChunkReader`.
`CompositeAudioChunkReader` dispatches by file extension (WAV → `WavAudioChunkReader`; other formats → `FfmpegAudioChunkReader`).
`ffmpeg` binary must be on `PATH`; its absence causes graceful degradation to WAV-only mode.

See `include/whisper/audio_chunk_reader.h` and `src/whisper/audio_chunk_reader.cpp`.

---

### 4. VAD Pre-filter (Target: Q3 2026)

**Problem:** Silent or noise-only segments waste inference time.

**Solution:** Add `IVoiceActivityDetector` strategy to `WhisperPlugin`. Default implementation
uses energy threshold; advanced implementation wraps silero-VAD ONNX model.

**Inputs:** `AudioChunk`, `VadConfig { energy_threshold, min_speech_ms }`.
**Outputs:** List of speech segments `{start_sample, end_sample}`.
**Perf target:** < 5 ms per 1-second chunk on CPU.
**Tests:** 3 unit tests; one all-silence, one all-speech, one mixed.

---

## Security / Reliability

- All new reader adapters must validate file extensions and MIME types before passing data
  to external binaries to prevent path traversal.
- Speaker-embedding models must be subject to the same integrity-verification policy as
  whisper.cpp models (digest check before load).
