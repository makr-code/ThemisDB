> **Status:** 2026-06-01 – mit aktuellem Voice-Code (`voice_authenticator.cpp`, `voice_security.cpp`, `voice_session_manager.cpp`, `voice_telephony.cpp`) abgeglichen.

# ThemisDB Voice Module - Production Requirements

## Zweck und Geltungsbereich

Dieses Dokument ist der **kanonische Referenzpunkt für produktive Mindestanforderungen** des Voice-Moduls.
Es definiert verbindliche Anforderungen für Voice-Authentifizierung, Session-Lifecycle, Streaming-Limits, Transcript-Schutz und Telephony-Härtung.

## Dokumentabgrenzung (Canonical Split)

- **`src/voice/PRODUCTION_REQUIREMENTS.md` (dieses Dokument):** verpflichtende Produktionsanforderungen (MUST/MUST NOT), Sicherheitsannahmen, Betriebsgrenzen.
- **`src/voice/README.md`:** Funktionsübersicht, Architekturkontext, API- und Nutzungsbeispiele.
- **`src/voice/ROADMAP.md`:** Lieferphasen, offene/abgeschlossene Features, Readiness-Planung.
- **`src/voice/FUTURE_ENHANCEMENTS.md`:** mittelfristige/langfristige Erweiterungen und Forschungsfelder.

## Verbindliche Voice-Sicherheitsanforderungen

### 1) Authentifizierung und Session-Guard

- **MUST:** `voice_authenticator.cpp` aktiv vor Zugang zu Voice-Pipeline; unauthentifizierte Session-Initiierungen werden abgewiesen.
- **MUST:** `voice_security.cpp` Guards aktiv in allen sensiblen Voice-Pfaden.
- **MUST:** `voice_session_manager.cpp` mit bounded Session-Lifecycle konfiguriert; Session-Timeouts explizit gesetzt.
- **MUST NOT:** Voice-Sessions ohne Authentication-Guard initiieren.

### 2) Streaming-Limits und Input-Validierung

- **MUST:** Oversized/malformed Streaming-Input (`voice_browser_streaming.cpp`) wird mit explizitem Fehlercode abgewiesen.
- **MUST:** Audio-Preprocessing (`audio_preprocessing.cpp`) mit definierten Input-Size-Limits; unbegrenzte Audio-Puffer sind nicht zulässig.
- **MUST NOT:** Unvalidierte Audio-Streams in Emotion-Analyzer oder LLM-Pfade weitergeben.

### 3) Transcript-Schutz

- **MUST:** Transcript-Storage (`voice_audio_storage.cpp`) mit Zugriffskontrolle; kein öffentlicher Transcript-Zugriff ohne Autorisierung.
- **MUST NOT:** Sensitive Transcript-Inhalte in Logs ohne Maskierung persistieren.

### 4) Telephony-Härtung

- **MUST:** `voice_telephony.cpp` Eingabe-Validierung aktiv; Browser- und Telephony-Injection-Vektoren werden geprüft.
- **MUST:** Anti-Spoofing-Konfiguration aktiv; Deployment-Konfiguration muss Anti-Spoofing-Modell-Profil spezifizieren.

## Betriebsgrenzen (aktuelles Voice-Verhalten)

- Anti-Spoofing-Effektivität abhängig von konfiguriertem Modell-Profil; regelmäßige Kalibrierung empfohlen.
- Telephony/Browser-Threat-Handling benötigt kontinuierliche Regressionscoverage unter neuen Angriffsvarianten.
- `emotion_analyzer.cpp` ist inferenz-basiert; Produktionseinsatz erfordert kalibriertes Modell und definierten Confidence-Threshold.

## Minimaler Produktions-Check (Audit-fähig)

- [ ] Voice-Authenticator aktiv vor Voice-Pipeline
- [ ] Session-Lifecycle bounded (Timeout konfiguriert)
- [ ] Streaming-Input-Limits aktiv
- [ ] Transcript-Zugriffskontrolle aktiv
- [ ] Transcript-Logs maskiert
- [ ] Telephony-Input-Validierung aktiv
- [ ] Anti-Spoofing-Modell-Profil konfiguriert
- [ ] Produktionsmodus via `THEMIS_PRODUCTION_MODE` oder `THEMIS_ENVIRONMENT` gesetzt

## Review / Sourcecode-Audit-Nachweis

### Betroffene Dateien im Review

- `src/voice/PRODUCTION_REQUIREMENTS.md`
- `src/voice/voice_authenticator.cpp`
- `src/voice/voice_security.cpp`
- `src/voice/voice_session_manager.cpp`
- `src/voice/voice_browser_streaming.cpp`
- `src/voice/voice_telephony.cpp`
- `src/voice/audio_preprocessing.cpp`
- `src/voice/voice_audio_storage.cpp`
- `src/voice/voice_error_handler.cpp`
