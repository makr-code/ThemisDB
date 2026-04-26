[docs](../../README.md) > [de](../README.md) > [whisper](./README.md) > [missing-implementations](./missing-implementations.md)
**Datum:** 2026-04-16
**Status:** review
**Primary (Quelle der Wahrheit):**
- `src/whisper/ROADMAP.md`
- `src/whisper/FUTURE_ENHANCEMENTS.md`
- `src/whisper/SECURITY.md`
- `src/whisper/whisper_plugin.cpp`
- `src/whisper/whisper_transcriber.cpp`

**Bezug / Reference:**
- Issue: `[MODULE] whisper`
- Kontext: Task 4 Missing-Implementations-Report für modulweise Doku-Migration

---

# Whisper-Modul — Missing Implementations Report

## 1) Streaming-Transkription (offen)

| Feld | Inhalt |
|---|---|
| **Impact** | Hoch — Lange Audiodateien bleiben blockierend ohne Token-Streaming |
| **Evidence** | `src/whisper/FUTURE_ENHANCEMENTS.md` fordert `transcribeStream(...)`; kein entsprechender API-Eintrag in `include/plugins/audio_backend_interface.h` / `include/whisper/whisper_transcriber.h` |
| **Priorität** | P1 |
| **Folge-Issue (Vorschlag)** | `feat(whisper): add streaming transcription API with callback contract` |

## 2) Speaker Diarisierung (offen)

| Feld | Inhalt |
|---|---|
| **Impact** | Mittel — Multi-Speaker-Use-Cases ohne Sprecherzuordnung |
| **Evidence** | Als geplant in `src/whisper/ROADMAP.md` und `src/whisper/FUTURE_ENHANCEMENTS.md`; keine `diarize`-Implementierung in `src/whisper/whisper_transcriber.cpp` |
| **Priorität** | P2 |
| **Folge-Issue (Vorschlag)** | `feat(whisper): implement optional diarization pipeline and result model` |

## 3) VAD Pre-Filter (offen)

| Feld | Inhalt |
|---|---|
| **Impact** | Mittel — Unnötige Inferenz auf Stille-/Rauschsegmenten |
| **Evidence** | ROADMAP/Future markieren VAD als geplant; kein `IVoiceActivityDetector`-Pfad in `include/whisper/` bzw. `src/whisper/` |
| **Priorität** | P2 |
| **Folge-Issue (Vorschlag)** | `feat(whisper): add VAD strategy and segment pre-filter before transcriber` |

## 4) Realmodell-CI-Validierung (offen)

| Feld | Inhalt |
|---|---|
| **Impact** | Mittel — Produktionspfad mit echtem Modell nicht CI-abgesichert |
| **Evidence** | `src/whisper/ROADMAP.md` und `src/whisper/AUDIT.md` nennen fehlende End-to-End-Validierung mit Modell |
| **Priorität** | P2 |
| **Folge-Issue (Vorschlag)** | `ci(whisper): add gated real-model integration test job` |

## 5) Modellintegritätsprüfung (offen)

| Feld | Inhalt |
|---|---|
| **Impact** | Mittel — Geladene Modellartefakte werden aktuell nicht gegen Digest verifiziert |
| **Evidence** | `src/whisper/SECURITY.md`: SHA-256-Check als geplanter Punkt; kein Digest-Check in `src/whisper/whisper_transcriber.cpp` |
| **Priorität** | P2 |
| **Folge-Issue (Vorschlag)** | `security(whisper): verify model digest before loading whisper model` |
