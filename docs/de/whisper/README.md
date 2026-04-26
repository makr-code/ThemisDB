[docs](../../README.md) > [de](../README.md) > [whisper](./README.md) > [README](./README.md)
**Datum:** 2026-04-16
**Status:** review
**Primary (Quelle der Wahrheit):**
- `src/whisper/README.md`
- `src/whisper/ARCHITECTURE.md`
- `src/whisper/ROADMAP.md`
- `src/whisper/FUTURE_ENHANCEMENTS.md`
- `include/whisper/README.md`
- `include/whisper/ROADMAP.md`

**Bezug / Reference:**
- Issue: `[MODULE] whisper`
- Kontext: Modulweiser Reality-Check und Secondary-Doku-Migration (Primary → Secondary)

---

# Whisper-Modul

## TL;DR

Das `whisper`-Modul ist implementiert mit `WhisperPlugin`, WAV-Reader, FFmpeg-Adapter,
Composite-Reader, Thread-Safety (`transcriber_mutex_`) und 44 Unit-Tests (A–N).

## Installation

Modul-Build erfolgt über die bestehende CMake-Integration:

```bash
cmake -B build
cmake --build build --target test_whisper_plugin
```

Optional mit produktivem Backend:

```bash
cmake -B build -DTHEMIS_ENABLE_WHISPER=ON
```

## Usage

- Datei-Transkription über `WhisperPlugin::transcribeFile(path)`
- PCM-Transkription über `WhisperPlugin::transcribe(samples, sample_rate)`
- Sprachdetektion über `WhisperPlugin::detectLanguage(samples, sample_rate)`

## Task 1 — Reality-Check (Code vs. Primary-Doku)

Geprüfte Implementierungspfade:
- `src/whisper/whisper_plugin.cpp`
- `src/whisper/audio_chunk_reader.cpp`
- `src/whisper/whisper_config.cpp`
- `src/whisper/tests/test_whisper_plugin.cpp`

Abweichungen (in dieser Migration korrigiert):
- `src/whisper/README.md`: Reifegrad/Testanzahl/Quickstart waren veraltet bzw. inkonsistent.
- `src/whisper/ARCHITECTURE.md`: Thread-Safety-Abschnitt war nicht mehr aktuell.
- `src/whisper/AUDIT.md` und `src/whisper/ROADMAP.md`: Testanzahl war auf 36 statt 44.
- `include/whisper/ROADMAP.md`: Testanzahl war auf 36 statt 44.

## Task 2 — ROADMAP / FUTURE_ENHANCEMENTS Verifikation

- `src/whisper/ROADMAP.md`: Phasen 1–6 sind durch Codepfade belegbar; offene Punkte bleiben
  Streaming, Diarisierung, VAD und Realmodell-CI.
- `src/whisper/FUTURE_ENHANCEMENTS.md`: enthält klare, implementierbare Angaben
  (Inputs/Outputs/Constraints/Tests/Targets).
- `include/whisper/FUTURE_ENHANCEMENTS.md`: strukturell korrekt; Performanceziel bleibt bewusst
  allgemein und sollte bei Umsetzung konkretisiert werden.

## Task 3 — Research-Hinweise / Constraints

- FFmpeg-Pfad bleibt Subprozess-basiert (`FfmpegAudioChunkReader`) mit Shell-Escaping und
  500-MB-Output-Grenze.
- Stub/Produktionspfad ist compile-time-gesteuert über `THEMIS_ENABLE_WHISPER`.
- Modellintegritätsprüfung ist in Security-Doku als geplanter Schritt markiert.

## Weiterführende Secondary-Dokumente

- [PRIMARY_SOURCES.md](./PRIMARY_SOURCES.md)
- [missing-implementations.md](./missing-implementations.md)
