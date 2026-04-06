<!-- Status: current | validated: 2026-04-06 -->

# Voice — Include Changelog

> Public header changes only. For implementation changes see [`../../src/voice/CHANGELOG.md`](../../src/voice/CHANGELOG.md).

## [Unreleased]

## [1.1.0] — 2026-03-12

### Added
- `voice_meeting_support.h` — multi-speaker diarization and meeting transcription
- `voice_telephony.h` — SIP/PSTN telephony integration header
- `voice_accessibility.h` — accessibility features (captions, amplification, slow-TTS)
- `voice_tts_customizer.h` — TTS voice style, rate, pitch, SSML customization
- `emotion_analyzer.h` — emotion classification from audio features

### Changed
- `voice_auth.h` — added `verify_liveness()` to public API; anti-spoofing integration
- `voice_browser_streaming.h` — added `set_auth_token()` and `validate_origin()` to API
- `wake_word_detector.h` — configurable sensitivity threshold added

### Fixed
- `voice_session_manager.h` — session token now uses CSPRNG-backed UUID generation

## [1.0.0] — 2026-01-15

### Added
- Initial release: `audio_preprocessing.h`, `voice_assistant.h`, `voice_audio_storage.h`,
  `voice_auth.h`, `voice_batch_processor.h`, `voice_browser_streaming.h`,
  `voice_error_handler.h`, `voice_intent_detector.h`, `voice_macro.h`,
  `voice_model_cache.h`, `voice_security.h`, `voice_session_manager.h`,
  `wake_word_detector.h`

See [`../../src/voice/CHANGELOG.md`](../../src/voice/CHANGELOG.md) for full history.
