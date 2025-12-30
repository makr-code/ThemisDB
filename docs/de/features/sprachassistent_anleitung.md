# ThemisDB Sprachassistent - Vollständige Anleitung

**Version:** 1.0  
**Status:** Enterprise-Feature  
**Autor:** ThemisDB Team  
**Datum:** Dezember 2025

---

## Überblick

Der ThemisDB Sprachassistent bietet natürlichsprachliche Sprachinteraktionsfähigkeiten ähnlich wie Alexa oder Siri, direkt in die Datenbank integriert. Er kombiniert Speech-to-Text (STT), Text-to-Speech (TTS) und Large Language Models (LLM) um folgende Funktionen zu ermöglichen:

- **Sprachbefehle** - Datenbankabfragen und -steuerung über natürliche Sprache
- **Telefonaufzeichnung** - Automatische Transkription und Speicherung von Telefonaten
- **Besprechungsprotokolle** - KI-gestützte Protokollerstellung und Aktionspunkte
- **Sprachassistenten-Konversationen** - Interaktive sprachbasierte Assistenz

Alle Aufzeichnungen und Transkriptionen werden sicher in ThemisDB mit voller Revisionskontrolle und Audit-Trails gespeichert (Enterprise-Feature).

---

## Architektur

```
┌─────────────────────────────────────────────────────────┐
│                  Sprachassistent                         │
│  ┌───────────┐   ┌───────────┐   ┌─────────────┐       │
│  │    STT    │   │    TTS    │   │     LLM     │       │
│  │ (Whisper) │   │  (Piper)  │   │ (llama.cpp) │       │
│  └───────────┘   └───────────┘   └─────────────┘       │
└────────────────────┬────────────────────────────────────┘
                     │
         ┌───────────┴───────────┐
         │                       │
    ┌────▼────┐            ┌────▼────┐
    │   API   │            │   WS    │
    │/api/v1/ │            │  /ws/   │
    │ voice   │            │ voice   │
    └─────────┘            └─────────┘
         │                       │
         └───────────┬───────────┘
                     │
         ┌───────────▼───────────┐
         │  ThemisDB Speicher    │
         │  - Base Entities      │
         │  - Revisionskontrolle │
         │  - Audit-Logs         │
         └───────────────────────┘
```

---

## Features

### 1. Speech-to-Text (STT)

Basierend auf **Whisper.cpp** für hochgenaue Transkription:

- Mehrsprachenunterstützung (100+ Sprachen mit Auto-Erkennung)
- Zeitstempel-Generierung für Segmente
- Sprecher-Diarisierung (Identifikation verschiedener Sprecher)
- Konfidenzwerte auf Wortebene
- Echtzeit-Streaming-Transkription

**Unterstützte Audio-Formate:**
- MP3, WAV, OGG, FLAC, AAC, M4A, Opus, WMA

**Modellgrößen:**
- `tiny` - 39M Parameter, schnell, gut für Echtzeit
- `base` - 74M Parameter, ausgewogen (Standard)
- `small` - 244M Parameter, bessere Genauigkeit
- `medium` - 769M Parameter, hohe Genauigkeit
- `large` - 1550M Parameter, beste Genauigkeit

### 2. Text-to-Speech (TTS)

Basierend auf **Piper TTS** für natürlich klingende Sprachsynthese:

- Mehrere Stimmprofile (männlich/weiblich, verschiedene Akzente)
- Anpassbare Geschwindigkeit und Tonhöhe
- Mehrere Ausgabeformate (WAV, MP3, OGG)
- Hochwertige neuronale Synthese
- Echtzeit-Streaming-Synthese

**Verfügbare Stimmen:**
- Englisch (US, UK, Australisch)
- Deutsch
- Spanisch
- Französisch
- Und weitere...

### 3. LLM-Integration

Verwendet **llama.cpp** für Verständnis natürlicher Sprache:

- Konversationskontext-Verwaltung
- Besprechungszusammenfassungen
- Extraktion von Kernpunkten
- Identifikation von Aktionspunkten
- Verarbeitung natürlichsprachlicher Abfragen

---

## Schnellstart

### 1. Sprachassistent aktivieren

Bearbeiten Sie `config/voice_assistant.yaml`:

```yaml
voice_assistant:
  enabled: true
  
  stt:
    model_path: "./models/ggml-base.bin"
    model_size: "base"
    language: "auto"
  
  tts:
    model_path: "./models/tts-model.bin"
    voice: "default"
  
  llm:
    model_path: "./models/llama-2-7b-chat.gguf"
    n_ctx: 4096
```

### 2. ThemisDB Server starten

```bash
./themis_server --config config.yaml --enable-voice-assistant
```

### 3. Sprachbefehl testen

```bash
curl -X POST http://localhost:8080/api/v1/voice/command \
  -H "Authorization: Bearer IHR_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "text": "Was ist der Gesamtumsatz diesen Monat?",
    "session_id": "benutzer123"
  }'
```

---

## Anwendungsfälle

### 1. Telefonaufzeichnungssystem

Kundenservice-Anrufe automatisch aufzeichnen und transkribieren:

```python
import requests
import base64

# Audio-Datei lesen
with open("anruf.mp3", "rb") as f:
    audio_data = f.read()
    audio_base64 = base64.b64encode(audio_data).decode()

# Anruf aufzeichnen
response = requests.post(
    "http://localhost:8080/api/v1/voice/call/record",
    headers={"Authorization": "Bearer IHR_TOKEN"},
    json={
        "audio_base64": audio_base64,
        "call_id": "anruf-12345",
        "caller": "+49123456789",
        "callee": "+49987654321",
        "call_type": "inbound"
    }
)

result = response.json()
print(f"Transkript: {result['transcript']}")
print(f"Zusammenfassung: {result['summary']}")
print(f"Dokument-ID: {result['document_id']}")
```

### 2. Besprechungsprotokolle generieren

Automatische Erstellung von Besprechungsprotokollen:

```python
import requests
import base64

# Besprechungsaufnahme lesen
with open("besprechung.wav", "rb") as f:
    audio_data = f.read()
    audio_base64 = base64.b64encode(audio_data).decode()

# Protokoll generieren
response = requests.post(
    "http://localhost:8080/api/v1/voice/meeting/protocol",
    headers={"Authorization": "Bearer IHR_TOKEN"},
    json={
        "audio_base64": audio_base64,
        "meeting_id": "besprechung-789",
        "title": "Sprint-Planung",
        "participants": [
            "alice@firma.de",
            "bob@firma.de"
        ]
    }
)

result = response.json()
print(f"Zusammenfassung: {result['summary']}")
print(f"Kernpunkte: {result['key_points']}")
print(f"Aktionspunkte: {result['action_items']}")
```

### 3. Sprachgesteuerte Datenbankabfragen

Datenbank mit natürlicher Sprache abfragen:

```python
import requests

response = requests.post(
    "http://localhost:8080/api/v1/voice/command",
    headers={"Authorization": "Bearer IHR_TOKEN"},
    json={
        "text": "Zeige mir die Gesamtumsätze vom letzten Monat",
        "session_id": "benutzer123"
    }
)

result = response.json()
print(f"Antwort: {result['response']}")
```

---

## Verwendungsszenario: Notizen aus Telefongesprächen

Der Hauptzweck dieses Features ist die Umwandlung von Telefonaten in strukturierte Notizen und Gesprächsprotokolle. Hier ein vollständiges Beispiel:

### Beispiel: Kundensupport-Anruf

```python
import requests
import base64
from datetime import datetime

# 1. Telefongespräch aufzeichnen
with open("support_anruf.mp3", "rb") as f:
    audio_data = f.read()
    audio_base64 = base64.b64encode(audio_data).decode()

# 2. In ThemisDB speichern mit Metadaten
response = requests.post(
    "http://localhost:8080/api/v1/voice/call/record",
    headers={"Authorization": "Bearer IHR_TOKEN"},
    json={
        "audio_base64": audio_base64,
        "call_id": f"call-{datetime.now().strftime('%Y%m%d-%H%M%S')}",
        "caller": "+49123456789",
        "callee": "+49800-SERVICE",
        "start_time": int(datetime.now().timestamp() * 1000),
        "end_time": int(datetime.now().timestamp() * 1000) + 180000,
        "call_type": "inbound",
        "custom_fields": {
            "abteilung": "Kundenservice",
            "kategorie": "Technischer Support",
            "priorität": "hoch",
            "kunde_id": "KUNDE-12345"
        }
    }
)

result = response.json()

# 3. Ergebnis verarbeiten
print("=" * 60)
print("GESPRÄCHSNOTIZ")
print("=" * 60)
print(f"\nAnruf-ID: {result['call_id']}")
print(f"Dauer: {result['duration_ms'] / 1000 / 60:.1f} Minuten")
print(f"Sprache: {result['language']}")
print(f"Genauigkeit: {result['confidence'] * 100:.1f}%")
print(f"\nTranskript:\n{result['transcript']}")
print(f"\nZusammenfassung:\n{result['summary']}")
print(f"\nGespeichert in ThemisDB: {result['document_id']}")

# 4. Audio automatisch als OGG komprimiert und gespeichert
# 5. Vollständige Revisionskontrolle aktiviert
# 6. Audit-Log-Eintrag erstellt
```

**Resultat:**
```
============================================================
GESPRÄCHSNOTIZ
============================================================

Anruf-ID: call-20251230-143022
Dauer: 3.0 Minuten
Sprache: de
Genauigkeit: 95.2%

Transkript:
Kunde: Guten Tag, ich habe ein Problem mit meinem Drucker.
Agent: Guten Tag, gerne helfe ich Ihnen. Können Sie mir das 
       Problem genauer beschreiben?
Kunde: Der Drucker druckt nur noch weiße Seiten...
[...]

Zusammenfassung:
Kunde meldete Problem mit Drucker HP LaserJet Pro. Gerät 
druckt nur weiße Seiten. Agent diagnostizierte leere Toner-
kartusche. Ersatzkartusche wird per Express gesendet.
Kunde zufrieden mit Lösung.

Gespeichert in ThemisDB: recording:a1b2c3d4e5
```

---

## Speicherung und Revisionskontrolle

Alle Aufzeichnungen werden in ThemisDB gespeichert mit:

- **Revisionskontrolle** - Änderungen über Zeit nachverfolgen
- **Audit-Logs** - Wer hat wann auf was zugegriffen/geändert
- **Verschlüsselung** - Verschlüsselung ruhender Daten
- **Komprimierung** - Automatische Audio-Komprimierung (OGG/MP3)
- **Metadaten** - Umfangreiche Metadaten für Suche und Abruf

---

## Sicherheit

### Datenschutz

- PII-Erkennung und optionale Schwärzung
- Konfigurierbare Datenspeicherungsrichtlinien
- Automatische Bereinigung alter Aufzeichnungen
- DSGVO-konforme Datenverarbeitung

### Audit-Logging

Alle Sprachoperationen werden protokolliert:
- Wer hat die Anfrage initiiert
- Welche Operation wurde durchgeführt
- Wann ist es passiert
- Auf welche Daten wurde zugegriffen/diese geändert

---

## Performance

### STT-Performance

| Modell | Geschwindigkeit | Genauigkeit | Speicher |
|--------|----------------|-------------|----------|
| tiny   | 4x RT          | Gut         | ~1 GB    |
| base   | 1x RT          | Besser      | ~1 GB    |
| small  | 0.5x RT        | Hoch        | ~2 GB    |
| medium | 0.3x RT        | Sehr hoch   | ~5 GB    |
| large  | 0.2x RT        | Beste       | ~10 GB   |

*RT = Echtzeit (1x RT bedeutet 1 Minute Audio = 1 Minute Verarbeitung)*

---

## Enterprise-Features

- **Horizontale Skalierung** - Sprachverarbeitung auf mehrere Knoten verteilen
- **Hochverfügbarkeit** - Redundante Sprachassistenten
- **Erweiterte Analytik** - Anruf-Analytik, Sentiment-Analyse
- **Benutzerdefiniertes Stimmentraining** - Trainieren Sie benutzerdefinierte Stimmen für Ihre Marke
- **Integration** - Integration mit PBX-Systemen, CRM, etc.

---

## Support

Bei Fragen oder Problemen:
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Dokumentation: https://makr-code.github.io/ThemisDB/
- Enterprise Support: sales@themisdb.com

---

## Lizenz

Der Sprachassistent ist ein **Enterprise-Feature** von ThemisDB.

- Community Edition: Beschränkt auf grundlegende STT/TTS-Funktionalität
- Enterprise Edition: Vollständige Features einschließlich Telefonaufzeichnung, Besprechungsprotokolle und erweiterte LLM-Integration

Siehe [LICENSE](LICENSE) für Details.
