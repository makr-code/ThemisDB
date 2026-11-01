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

> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-10-31 | Primary: include/whisper/ -->
<!-- Links: ../../src/whisper/README.md · ../../src/whisper/ARCHITECTURE.md · ../../src/whisper/ROADMAP.md · ../../src/whisper/FUTURE_ENHANCEMENTS.md -->

# whisper — Public Headers

**Module Path:** `include/whisper/`
**Implementation Overview:** [`../../src/whisper/README.md`](../../src/whisper/README.md)
**Namespace:** `themis::whisper`
**Implementation version:** `2.3.0`; this identifies the implementation, not a production-readiness certification.

The `whisper` module provides audio transcription and language-detection capabilities
for ThemisDB via [whisper.cpp](https://github.com/ggerganov/whisper.cpp). A zero-dependency
stub transcriber is selected when whisper.cpp support is not compiled in; builds with the real
backend enabled require a valid model file.

---

## Header Surface (Public API)

| Header | Key Types | Purpose |
|---|---|---|
| `whisper_plugin.h` | `WhisperPlugin` | Top-level `IAudioBackend` implementation — entry point for all transcription and language-detection calls. |
| `whisper_config.h` | `WhisperConfig` | JSON-backed runtime configuration for the transcription engine. |
| `whisper_transcriber.h` | `IWhisperTranscriber`, `WhisperCppTranscriber` (conditional), `WhisperStubTranscriber`, `InMemoryWhisperTranscriber` | Strategy interface for the core inference engine plus optional production, stub, and test-double implementations. |
| `audio_chunk_reader.h` | `IAudioChunkReader`, `WavAudioChunkReader`, `FfmpegAudioChunkReader`, `CompositeAudioChunkReader` | Audio file I/O abstraction supporting WAV directly and MP3/OGG/FLAC/M4A/AAC/Opus/WMA/WebM through FFmpeg. |
| `voice_activity_detector.h` | `IVoiceActivityDetector`, `EnergyThresholdVad`, `VadConfig`, `SpeechSegment` | Voice Activity Detection strategy for skipping silent segments before inference. |
| `whisper_plugin_registrar.h` | `WhisperPluginAdapter`, `WhisperPluginRegistrar` | `IThemisPlugin` adapter and factory helper for integration with `plugins::PluginManager`, including hot-plug support. |

---

## Header Reference

### `whisper_plugin.h` — `WhisperPlugin`

Top-level audio-transcription plugin implementing `audio::IAudioBackend`.

```cpp
#include "whisper/whisper_plugin.h"
```

**Key methods:**

| Method | Description |
|---|---|
| `WhisperPlugin()` | Default constructor — selects `WhisperCppTranscriber` when `THEMIS_ENABLE_WHISPER` is defined, otherwise `WhisperStubTranscriber`. |
| `WhisperPlugin(transcriber, reader)` | Injection constructor for tests; both dependencies are transferred by unique_ptr. |
| `initialize(model_path, config_json)` | Load a model and apply JSON configuration. Returns `false` if initialization fails. |
| `transcribe(pcm_samples, sample_rate)` | Transcribe mono float32 PCM. Thread-safe. |
| `transcribeFile(path)` | Read and transcribe a supported audio file via the installed `IAudioChunkReader`. |
| `transcribeStream(pcm, rate, callback)` | Callback API. The default and current whisper.cpp transcriber emit the completed transcript as one token. Skips silent segments when VAD is installed. |
| `detectLanguage(pcm_samples, sample_rate)` | Returns the detected language and confidence. Applies `language_confidence_threshold` filter from `WhisperConfig`. |
| `transcribeWithDiarisation(pcm, rate, cfg)` | Calls the optional diarisation method; current whisper.cpp implementation returns one whole-transcript segment with one speaker ID. |
| `getStatistics()` | Returns a JSON object with `transcription_count`, `error_count`, `model_id`, and `last_error`. |
| `setVoiceActivityDetector(vad, cfg)` | Inject a `IVoiceActivityDetector`; pass `nullptr` to disable VAD. |

**Thread-safety:** calls into the transcriber are serialized through an internal mutex; file decoding in `transcribeFile()` occurs before that lock and may run concurrently. Counters are `std::atomic`.

---

### `whisper_config.h` — `WhisperConfig`

JSON-backed runtime configuration for the transcription engine.

```cpp
#include "whisper/whisper_config.h"
```

| Field | Type | Default | Description |
|---|---|---|---|
| `model_path` | `std::string` | `""` | Path to the `.bin` model file. An empty path does not select stub mode: it fails initialization with the real backend; the default stub is selected when whisper.cpp is not enabled. |
| `language` | `std::string` | `"auto"` | Passed to whisper.cpp; no format validation is performed by `WhisperConfig`. |
| `n_threads` | `int` | `4` | Number of inference threads. Values below `1` are clamped to `1`; there is no upper clamp. |
| `translate` | `bool` | `false` | Translate output to English when `true`. |
| `beam_size` | `int` | `5` | Beam search width. Values below `1` are clamped to `1`; there is no upper clamp. |
| `print_progress` | `bool` | `false` | Print progress to stderr during inference. |
| `quality_threshold` | `float` | `0.0f` | Serialized configuration field; no filtering behavior is implemented in the current transcriber. |
| `language_confidence_threshold` | `float` | `0.0f` | Minimum language-detection confidence to accept (0 = accept all). |
| `model_sha256` | `std::string` | `""` | Optional expected model digest; checked by the whisper.cpp backend before model load. |

**Static factory / serialization:**

```cpp
auto cfg = themis::whisper::WhisperConfig::fromJson(json_object);
nlohmann::json j = cfg.toJson();
```

---

### `whisper_transcriber.h` — Transcriber Strategy

Separates model inference from plugin lifecycle. Inject via the `WhisperPlugin` injection constructor.

```cpp
#include "whisper/whisper_transcriber.h"
```

**Implementations:**

| Class | When to use |
|---|---|
| `WhisperCppTranscriber` | Production (requires `THEMIS_ENABLE_WHISPER` compile flag and `libwhisper`). |
| `WhisperStubTranscriber` | Default CI/no-model fallback. Optionally accepts a `TranscribeFn` function for custom stub logic. |
| `InMemoryWhisperTranscriber` | Unit-test double. Pre-set results via `setNextResult()` / `setNextLanguage()` / `setStreamTokens()`. |

**Core interface:**

```cpp
class IWhisperTranscriber {
    virtual bool initialize(const WhisperConfig& cfg) = 0;
    virtual bool isInitialized() const = 0;
    virtual audio::TranscriptionResult    transcribe(pcm, rate) = 0;
    virtual audio::LanguageDetectionResult detectLanguage(pcm, rate) = 0;
    virtual audio::TranscriptionResult    transcribeStream(pcm, rate, callback);
    virtual std::string getModelId() const = 0;
};
```

`transcribeStream()` has a default implementation that calls `transcribe()` and emits the full text
as a single token. `WhisperCppTranscriber` does not override it; `InMemoryWhisperTranscriber`
supports a preloaded multi-token test path. The default implementation catches any callback
exception and returns a failed result. The plugin catches `std::exception` thrown by a transcriber;
non-standard exceptions escaping an overriding transcriber method may propagate.

---

### `audio_chunk_reader.h` — Audio File I/O

Extension point for audio file ingestion. `WhisperPlugin` uses `CompositeAudioChunkReader` by default.

```cpp
#include "whisper/audio_chunk_reader.h"
```

| Class | Supported formats | Notes |
|---|---|---|
| `WavAudioChunkReader` | RIFF/WAV (16-bit PCM, 32-bit float) | No external dependencies. Throws on malformed WAV. |
| `FfmpegAudioChunkReader` | MP3, OGG, FLAC, M4A, AAC, Opus, WMA, WebM (extension-based routing) | Requires `ffmpeg` on `PATH`; reads fail if unavailable. Max output: 500 MiB. |
| `CompositeAudioChunkReader` | Delegates by file extension | Tries registered readers in insertion order; throws if no reader accepts the file. |

**Core interface:**

```cpp
class IAudioChunkReader {
    virtual std::vector<float> readFile(const std::string& path, float& out_sample_rate) = 0;
    virtual bool canRead(const std::string& path) const = 0;
};
```

`readFile()` returns mono float32 PCM samples. WAV preserves the file's sample rate; FFmpeg output is
resampled to 16 kHz.

---

### `voice_activity_detector.h` — VAD Strategy

Detects speech-active segments in a PCM buffer before forwarding audio to the transcriber.

```cpp
#include "whisper/voice_activity_detector.h"
```

| Type | Description |
|---|---|
| `SpeechSegment` | Half-open sample range `[start_sample, end_sample)` identified as speech. |
| `VadConfig` | `energy_threshold` (default 0.01), `min_speech_ms` (default 50 ms), `frame_ms` (default 20 ms). |
| `IVoiceActivityDetector` | Strategy interface: `detect(pcm, sample_rate, cfg) -> vector<SpeechSegment>`. |
| `EnergyThresholdVad` | RMS-energy-threshold implementation. The header states a < 5 ms target per 1-second chunk at 16 kHz; no benchmark result is documented here. |

Inject into `WhisperPlugin` via `setVoiceActivityDetector()`. The current implementation applies
VAD from `transcribeStream()` and concatenates detected speech segments before inference;
`transcribe()` does not apply VAD. The comment on `setVoiceActivityDetector()` in
`whisper_plugin.h` also mentions a `transcribe()` VAD path, but that behavior is not implemented
by the current `WhisperPlugin::transcribe()`.

---

### `whisper_plugin_registrar.h` — Plugin Manager Integration

Adapts `WhisperPlugin` for use with the unified `plugins::PluginManager`.

```cpp
#include "whisper/whisper_plugin_registrar.h"
```

**`WhisperPluginAdapter`** (`IThemisPlugin`):

| Method | Description |
|---|---|
| `initialize(config_json)` | Parses JSON; calls `WhisperPlugin::initialize(model_path, config)`. |
| `shutdown()` | Resets the inner plugin to its default stub state. |
| `getInstance()` | Returns a raw pointer to the underlying `WhisperPlugin` (cast to `WhisperPlugin*`). |
| `getType()` | Returns `PluginType::AUDIO_PROCESSING`. |
| `getName()` / `getVersion()` | `"whisper"` / `"2.3.0"`. |

**`WhisperPluginRegistrar`** (all methods are static):

| Method | Description |
|---|---|
| `createPlugin(config)` | Creates a standalone `WhisperPlugin`. |
| `createAdapter(config)` | Creates a `WhisperPluginAdapter` wrapping a new `WhisperPlugin`. |
| `defaultReloadCallback()` | Returns a reload callback that re-initializes the plugin with `model_path` from config. |
| `enableHotPlug(manager, directory)` | Starts hot-plug monitoring for the given directory via `PluginManager`. |
| `disableHotPlug(manager)` | Stops hot-plug monitoring. |

---

## Configuration Options

### `WhisperConfig` Quick Reference

```json
{
  "model_path": "/models/ggml-base.bin",
  "language": "auto",
  "n_threads": 4,
  "translate": false,
  "beam_size": 5,
  "print_progress": false,
  "quality_threshold": 0.0,
  "language_confidence_threshold": 0.5
}
```

Set `language_confidence_threshold > 0` to label low-confidence `detectLanguage()` results as
`"unknown"`; the returned confidence remains the underlying confidence value.

### `VadConfig` Quick Reference

```cpp
themis::whisper::VadConfig vad_cfg;
vad_cfg.energy_threshold = 0.01f; // RMS threshold (0..1 normalised)
vad_cfg.min_speech_ms    = 50.0f; // discard segments shorter than this
vad_cfg.frame_ms         = 20.0f; // analysis frame length
```

---

## Runtime Behavior, Errors, and Limits

| Condition | Behavior |
|---|---|
| Plugin not initialized (`initialize()` not called or returned `false`) | Transcription methods return `success=false` with an error message. `detectLanguage()` returns its default result (empty language and zero confidence), which has no success/error fields. |
| File not found | `transcribeFile()` returns `success=false`, `error_message=<filesystem error>`. |
| Invalid WAV format (bad magic / truncated data) | `WavAudioChunkReader::readFile()` throws `std::runtime_error`; `WhisperPlugin` catches it and returns `success=false`. |
| `ffmpeg` not on PATH | `FfmpegAudioChunkReader::readFile()` throws `std::runtime_error("ffmpeg not available")`; caught by `WhisperPlugin`. |
| FFmpeg output exceeds 500 MiB | `FfmpegAudioChunkReader` throws `std::runtime_error("FfmpegAudioChunkReader: output exceeds maximum size limit")`. |
| FFmpeg path contains a NUL byte | `FfmpegAudioChunkReader::shellEscape()` throws `std::runtime_error`; other shell characters are quoted/escaped. |
| Transcriber throws `std::exception` during inference | `WhisperPlugin` catches, sets `success=false` and `error_message=<exception.what()>`, increments error counter. |
| All-silent PCM with VAD installed | `transcribeStream()` passes empty PCM to the transcriber; the result depends on the selected backend. |
| `CompositeAudioChunkReader` with no matching reader | `readFile()` throws `std::runtime_error`. |
| `language_confidence_threshold` filter triggered | `detectLanguage()` returns language `"unknown"` while preserving the measured confidence. |

**Provenance stamps:** successful plugin transcriptions receive these fields; error results carry
the source type and plugin version, but may not have a generation timestamp.

| Field | Value |
|---|---|
| `ingestion_source_type` | `"WHISPER"` |
| `plugin_version` | `"2.3.0"` |
| `generation_timestamp` | Unix epoch milliseconds |

**Limits:**

- Max FFmpeg output: 500 MiB (about 2 h 16 min at 16 kHz mono float32)
- `n_threads` and `beam_size` have a lower bound of `1` in JSON parsing; no upper bound is applied
- Concurrent `transcribe*()` / `detectLanguage()` calls serialize through `transcriber_mutex_`

---

## Usage Snippets

### Stub mode (no model required)

```cpp
#include "whisper/whisper_plugin.h"

auto plugin = std::make_unique<themis::whisper::WhisperPlugin>();
plugin->initialize("", {});

auto result = plugin->transcribeFile("recording.wav");
if (result.success) {
    std::cout << result.text << "\n";
}
```

### With whisper.cpp model

```cpp
#include "whisper/whisper_plugin.h"
#include "whisper/whisper_config.h"

themis::whisper::WhisperConfig cfg;
cfg.model_path = "/models/ggml-base.bin";
cfg.language   = "de";
cfg.n_threads  = 8;

auto plugin = std::make_unique<themis::whisper::WhisperPlugin>();
plugin->initialize(cfg.model_path, cfg.toJson());

auto result = plugin->transcribeFile("recording.wav");
```

### Test double injection

```cpp
#include "whisper/whisper_plugin.h"
#include "whisper/whisper_transcriber.h"

auto transcriber = std::make_unique<themis::whisper::InMemoryWhisperTranscriber>();
themis::audio::TranscriptionResult preset;
preset.text = "Hallo Welt";
preset.success = true;
transcriber->setNextResult(preset);

auto plugin = std::make_unique<themis::whisper::WhisperPlugin>(
    std::move(transcriber),
    std::make_unique<themis::whisper::WavAudioChunkReader>());
plugin->initialize("inmemory", {});
```

### Streaming transcription with VAD

```cpp
#include "whisper/whisper_plugin.h"
#include "whisper/voice_activity_detector.h"

auto plugin = std::make_unique<themis::whisper::WhisperPlugin>();
plugin->initialize("/models/ggml-base.bin", {});

themis::whisper::VadConfig vad_cfg;
vad_cfg.energy_threshold = 0.02f;
plugin->setVoiceActivityDetector(
    std::make_unique<themis::whisper::EnergyThresholdVad>(), vad_cfg);

std::vector<float> pcm = /* ... load PCM ... */;
auto result = plugin->transcribeStream(pcm, 16000.0f,
    [](const themis::audio::TranscriptionToken& tok) {
        std::cout << tok.text;
    });
```

### PluginManager integration

```cpp
#include "whisper/whisper_plugin_registrar.h"

// Create adapter for PluginManager
auto adapter = themis::whisper::WhisperPluginRegistrar::createAdapter(
    {{"model_path", "ggml-base.bin"}});

// Enable hot-plug monitoring
themis::whisper::WhisperPluginRegistrar::enableHotPlug(
    plugin_manager, "/plugins/whisper");
```

---

## Installation

No separate installation step is required for headers; include them via the main project include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

Then include headers using the module prefix:

```cpp
#include "whisper/whisper_plugin.h"
```

To enable the production whisper.cpp backend at compile time:

```cmake
cmake -B build -DTHEMIS_ENABLE_WHISPER=ON
cmake --build build --target themisdb
```

---

## Troubleshooting

**`transcribeFile()` returns `success=false` with "not initialized"**
→ Call `plugin->initialize(model_path, config)` before the first transcription call.

**`transcribeFile()` returns `success=false` with "ffmpeg not available"**
→ Install `ffmpeg` and ensure it is on `PATH`. WAV files bypass FFmpeg entirely.

**Empty transcription text in stub mode**
→ Expected. The stub transcriber (`WhisperStubTranscriber`) returns empty text, language `"unknown"`,
and confidence `0`; tests may inject a `TranscribeFn` using `setTranscribeFn()`.

**`detectLanguage()` returns `"unknown"` with confidence 0**
→ Either the audio is silent/too short, or `language_confidence_threshold` in `WhisperConfig` is set above the raw confidence. Lower the threshold or use longer audio.

**`CompositeAudioChunkReader` throws for an audio format**
→ Ensure `ffmpeg` is on PATH and that `FfmpegAudioChunkReader` was registered with `addReader()`.

**Concurrent transcription calls block each other**
→ By design: `WhisperPlugin` serializes all transcriber calls through `transcriber_mutex_` to protect the underlying `whisper_context*`. Use separate `WhisperPlugin` instances for true parallelism.

---

## Related Documentation

| Document | Description |
|---|---|
| [`../../src/whisper/README.md`](../../src/whisper/README.md) | Implementation overview, architecture diagram, quick-start, test suite reference |
| [`../../src/whisper/ARCHITECTURE.md`](../../src/whisper/ARCHITECTURE.md) | Component diagram, data flow, design principles |
| [`../../src/whisper/ROADMAP.md`](../../src/whisper/ROADMAP.md) | Feature roadmap, implementation phases, production readiness checklist |
| [`../../src/whisper/FUTURE_ENHANCEMENTS.md`](../../src/whisper/FUTURE_ENHANCEMENTS.md) | Planned enhancements (streaming, diarisation, VAD) with design constraints |
| [`../../src/whisper/SECURITY.md`](../../src/whisper/SECURITY.md) | Security considerations for model loading and FFmpeg subprocess handling |
| [`../../docs/de/whisper/README.md`](../../docs/de/whisper/README.md) | German-language module overview |
| [`../../docs/en/whisper/README.md`](../../docs/en/whisper/README.md) | English-language module overview |
