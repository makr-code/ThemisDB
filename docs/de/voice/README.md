# Voice-Modul

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: PRIMARY_SOURCES.md · ../../../src/voice/README.md -->

**Stand:** 6. April 2026  
**Version:** aktuell  
**Kategorie:** Sprach-/Audio-Integration  
**Status:** 🟢 Production-Ready

---

## Übersicht

Das Voice-Modul integriert Sprachverarbeitung in ThemisDB: Sprach-Authentifizierung, Absichtserkennung, Emotionsanalyse und sprachgesteuerte Datenbankabfragen.

**Primäre Quelle:** [`src/voice/`](../../../src/voice/) · [`include/voice/`](../../../include/voice/)

---

## Kernkomponenten

| Komponente | Header | Source | Beschreibung |
|------------|--------|--------|--------------|
| VoiceAssistant | `voice_assistant.h` | `voice_assistant.cpp` | Haupt-Sprachassistent-Fassade |
| VoiceAuth | `voice_auth.h` | `voice_authenticator.cpp` | Biometrische Sprach-Authentifizierung |
| AudioPreprocessing | `audio_preprocessing.h` | `audio_preprocessing.cpp` | Audio-Vorverarbeitung (Rauschunterdrückung, Normalisierung) |
| EmotionAnalyzer | `emotion_analyzer.h` | `emotion_analyzer.cpp` | Emotionserkennung aus Sprachsignalen |
| VoiceIntentDetector | `voice_intent_detector.h` | `voice_intent_detector.cpp` | Absichtserkennung für sprachgesteuerte Abfragen |
| VoiceSessionManager | `voice_session_manager.h` | `voice_session_manager.cpp` | Sprachsitzungs-Verwaltung |
| VoiceBatchProcessor | `voice_batch_processor.h` | `voice_batch_processor.cpp` | Batch-Verarbeitung von Sprachaufnahmen |
| VoiceBrowserStreaming | `voice_browser_streaming.h` | `voice_browser_streaming.cpp` | Browser-basiertes Audio-Streaming (WebSocket/WebRTC) |
| VoiceAudioStorage | `voice_audio_storage.h` | `voice_audio_storage.cpp` | Audio-Datenspeicherung in ThemisDB |
| VoiceErrorHandler | `voice_error_handler.h` | `voice_error_handler.cpp` | Fehlerbehandlung für Voice-Operationen |
| VoiceAccessibility | `voice_accessibility.h` | `voice_accessibility.cpp` | Untertitel-Generierung und barrierefreier Transkript-Export (VTT, SRT, HTML) |
| VoiceMeetingSupport | `voice_meeting_support.h` | `voice_meeting_support.cpp` | Meeting-Transkription, Aktionspunkterkennung und Zusammenfassung |
| VoiceModelCache | `voice_model_cache.h` | `voice_model_cache.cpp` | LRU-Modell-Cache für STT/TTS/LLM-Handles |
| VoiceSecurity | `voice_security.h` | `voice_security.cpp` | PII-Redaktion, Einwilligungsverwaltung und Sicherheitsmechanismen |
| VoiceMacroManager | `voice_macro.h` | `voice_macro_manager.cpp` | Benutzerdefinierte Sprachbefehls-Makros mit Trigger-Phrase-Erkennung |
| WakeWordDetector | `wake_word_detector.h` | `wake_word_detector.cpp` | Immer-aktive Schlüsselwort-Erkennung ("hey themis") mit VAD-Gating |
| TelephonyBridge | `voice_telephony.h` | `voice_telephony.cpp` | SIP/WebRTC-Telefonie-Bridge für Echtzeit-Transkription und IVR-Flows |
| VoiceTTSCustomizer | `voice_tts_customizer.h` | `voice_tts_customizer.cpp` | Benutzerdefinierte TTS-Sprachprofile, Prosodie und SSML-Unterstützung |

---

## Primäre Dokumentation

| Dokument | Beschreibung |
|----------|--------------|
| [`src/voice/README.md`](../../../src/voice/README.md) | Modulübersicht und Verwendungsbeispiele |
| [`include/voice/README.md`](../../../include/voice/README.md) | Public-API-Dokumentation aller 18 Header |
| [`src/voice/ROADMAP.md`](../../../src/voice/ROADMAP.md) | Abgeschlossene und geplante Funktionen |
| [`src/voice/FUTURE_ENHANCEMENTS.md`](../../../src/voice/FUTURE_ENHANCEMENTS.md) | Geplante Erweiterungen |
| [`src/voice/ARCHITECTURE.md`](../../../src/voice/ARCHITECTURE.md) | Architekturübersicht |
| [`docs/troubleshooting/voice_troubleshooting.md`](../../troubleshooting/voice_troubleshooting.md) | Häufige Probleme und Lösungen |
