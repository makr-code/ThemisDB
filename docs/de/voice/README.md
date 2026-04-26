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
| VoiceBrowserStreaming | `voice_browser_streaming.h` | `voice_browser_streaming.cpp` | Browser-basiertes Audio-Streaming (WebRTC) |
| VoiceAudioStorage | `voice_audio_storage.h` | `voice_audio_storage.cpp` | Audio-Datenspeicherung in ThemisDB |
| VoiceErrorHandler | `voice_error_handler.h` | `voice_error_handler.cpp` | Fehlerbehandlung für Voice-Operationen |
| VoiceAccessibility | `voice_accessibility.h` | `voice_accessibility.cpp` | Barrierefreiheitsfunktionen |
| VoiceMeetingSupport | `voice_meeting_support.h` | *(impl. in voice_assistant_llm)* | Meeting-Transkription und -Zusammenfassung |
| VoiceModelCache | `voice_model_cache.h` | *(impl. in voice_assistant)* | Modell-Cache für schnelle Inference |
| VoiceSecurity | `voice_security.h` | `voice_authenticator.cpp` | Sicherheitsmechanismen für Voice-Daten |
| VoiceMacro | `voice_macro.h` | *(impl. in voice_assistant)* | Makro-Definitionen für wiederholbare Sprachbefehle |

---

## Primäre Dokumentation

| Dokument | Beschreibung |
|----------|--------------|
| [`src/voice/README.md`](../../../src/voice/README.md) | Modulübersicht und Verwendungsbeispiele |
| [`src/voice/FUTURE_ENHANCEMENTS.md`](../../../src/voice/FUTURE_ENHANCEMENTS.md) | Geplante Erweiterungen |
