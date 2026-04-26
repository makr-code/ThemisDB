> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-04-06 -->
# Changelog — Voice Module
Based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

## [1.1.0] — 2026-03-12
### Added
- Real-time meeting transcription with speaker diarization (PR #3434)
- Phone call transcription via telephony bridge (SIP/WebRTC)
- Voice biometric authentication (speaker verification)
- Real-time browser WebSocket streaming of transcription results
- Meeting protocol generation (structured summary from transcript)
- Telephony bridge: SIP trunk and WebRTC peer connection support

## [1.0.0] — 2024-06-01
### Added
- `VoiceAssistant` orchestrator with session management
- Whisper-based speech-to-text (STT) integration
- llama.cpp text-to-speech (TTS) and LLM response generation
- Voice command recognition and intent extraction
