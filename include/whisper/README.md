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
- Audio chunk reader interface with WAV reader implementation
- JSON-backed whisper runtime configuration