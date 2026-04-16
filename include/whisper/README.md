# include whisper module

Public headers for Whisper transcription plugin integration.

## Headers
- `whisper_plugin.h`
- `whisper_transcriber.h`
- `whisper_config.h`
- `audio_chunk_reader.h`

## Exposed API
- `WhisperPlugin` implementing `audio::IAudioBackend`
- Transcriber abstraction with real/stub/in-memory variants
- Audio chunk reader interface with WAV, FFmpeg and composite reader contracts
- JSON-backed whisper runtime configuration (`language_confidence_threshold` included)

## Installation

No separate installation step is required for headers; include them via the main project include path.

## Usage

- Include `whisper/whisper_plugin.h` to consume `WhisperPlugin`.
- Include `whisper/whisper_config.h` for JSON config contracts.
- Include `whisper/audio_chunk_reader.h` and `whisper/whisper_transcriber.h` for extension points.
