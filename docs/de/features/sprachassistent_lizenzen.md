# Sprachassistent - Lizenzinformationen und On-Premise-Deployment

**Version:** 1.0  
**Status:** Enterprise-Feature  
**Datum:** Dezember 2025

---

## Übersicht

Dieses Dokument enthält umfassende Lizenzinformationen für alle im ThemisDB Sprachassistenten verwendeten Bibliotheken, mit speziellem Fokus auf Open-Source-Compliance und On-Premise-Deployment-Eignung.

---

## Kern-Bibliotheken

### 1. Whisper.cpp (Speech-to-Text)

| Attribut | Details |
|----------|---------|
| **Repository** | https://github.com/ggerganov/whisper.cpp |
| **Zweck** | Hochgenaue Sprachtranskription |
| **Lizenz** | MIT-Lizenz |
| **Copyright** | Copyright (c) 2022 Georgi Gerganov |
| **Open Source** | ✅ Ja |
| **Kommerzielle Nutzung** | ✅ Erlaubt |
| **On-Premise** | ✅ Vollständig unterstützt |
| **Attribution** | Erforderlich (MIT-Bedingungen) |

**Hauptmerkmale:**
- C++-Port von OpenAIs Whisper-Modell
- Keine externen API-Abhängigkeiten
- Läuft vollständig offline
- 100+ unterstützte Sprachen
- Mehrere Modellgrößen (tiny bis large)

**Compliance:**
- MIT-Lizenz ist eine der freizügigsten Open-Source-Lizenzen
- Erlaubt kommerzielle Nutzung, Modifikation und Verbreitung
- Keine Copyleft-Anforderungen
- Kann in proprietärer Software verwendet werden
- Erfordert nur Attribution in der Dokumentation

---

### 2. Piper TTS (Text-to-Speech)

| Attribut | Details |
|----------|---------|
| **Repository** | https://github.com/rhasspy/piper |
| **Zweck** | Neuronale Text-zu-Sprache-Synthese |
| **Lizenz** | MIT-Lizenz |
| **Copyright** | Copyright (c) 2023 Michael Hansen |
| **Open Source** | ✅ Ja |
| **Kommerzielle Nutzung** | ✅ Erlaubt |
| **On-Premise** | ✅ Vollständig unterstützt |
| **Attribution** | Erforderlich (MIT-Bedingungen) |

**Hauptmerkmale:**
- Neuronale TTS mit ONNX-Modellen
- Mehrere Stimmprofile
- Hochwertige Synthese
- Keine Cloud-Abhängigkeiten
- Leichtgewichtig und schnell

**Compliance:**
- MIT-Lizenz
- Geeignet für kommerzielle Bereitstellung
- Stimmenmodelle auch Open-Source (meist MIT/CC-BY)
- Keine Nutzungsbeschränkungen

---

### 3. llama.cpp (LLM-Inferenz)

| Attribut | Details |
|----------|---------|
| **Repository** | https://github.com/ggerganov/llama.cpp |
| **Zweck** | Large Language Model Inferenz |
| **Lizenz** | MIT-Lizenz |
| **Copyright** | Copyright (c) 2023 Georgi Gerganov |
| **Open Source** | ✅ Ja |
| **Kommerzielle Nutzung** | ✅ Erlaubt |
| **On-Premise** | ✅ Vollständig unterstützt |
| **Status** | Bereits in ThemisDB v1.3.0+ integriert |

**Hauptmerkmale:**
- Effiziente LLM-Inferenz in C++
- Unterstützung für LLaMA, Mistral, Phi-3 und mehr
- GGUF-Modellformat
- CPU- und GPU-Beschleunigung
- Keine externen API-Aufrufe

**Compliance:**
- MIT-Lizenz
- Bereits für ThemisDB-Nutzung genehmigt
- Kernbibliothek ist MIT (Modelle haben separate Lizenzen)

---

### 4. ONNX Runtime (Neuronales Netzwerk-Engine)

| Attribut | Details |
|----------|---------|
| **Repository** | https://github.com/microsoft/onnxruntime |
| **Zweck** | Neuronale Netzwerk-Inferenz-Engine (von Piper verwendet) |
| **Lizenz** | MIT-Lizenz |
| **Copyright** | Copyright (c) Microsoft Corporation |
| **Open Source** | ✅ Ja |
| **Kommerzielle Nutzung** | ✅ Erlaubt |
| **On-Premise** | ✅ Vollständig unterstützt |

---

## Lizenzzusammenfassung

### Alle Kernbibliotheken: MIT-Lizenz ✅

Die MIT-Lizenz erlaubt:
- ✅ Kommerzielle Nutzung
- ✅ Modifikation
- ✅ Verbreitung
- ✅ Private Nutzung
- ✅ Unterlizenzierung

Die MIT-Lizenz erfordert:
- ✅ Lizenz- und Copyright-Hinweis (Attribution)

Die MIT-Lizenz erfordert NICHT:
- ❌ Offenlegung des Quellcodes
- ❌ Copyleft (Teilen von Modifikationen)
- ❌ Markennutzung
- ❌ Patentgewährung
- ❌ Haftung oder Garantie

---

## On-Premise-Deployment

### ✅ Vollständig Unterstützt

Alle Bibliotheken sind für On-Premise-Deployment konzipiert und unterstützen dies:

1. **Keine Externen Abhängigkeiten**
   - Alle Verarbeitung erfolgt lokal
   - Keine API-Aufrufe an externe Dienste
   - Nach Modell-Download keine Internetverbindung erforderlich

2. **Datenschutz & Compliance**
   - Daten verlassen niemals Ihre Infrastruktur
   - DSGVO-konform
   - Geeignet für sensible Daten (Gesundheitswesen, Behörden, Finanzen)
   - Keine Datenverarbeitung durch Dritte

3. **Kostenvorteile**
   - Keine API-Gebühren pro Anfrage
   - Keine Nutzungsmessung
   - Einmaliger Modell-Download
   - Unbegrenzte Nutzung

4. **Kontrolle & Flexibilität**
   - Volle Kontrolle über Versionen
   - Modelle können angepasst und feinabgestimmt werden
   - Keine Vendor-Lock-in
   - Offline-Fähigkeit

5. **Performance**
   - Niedrige Latenz (keine Netzwerk-Aufrufe)
   - Skalierbar (mehr Hardware hinzufügen)
   - GPU-Beschleunigung unterstützt
   - Vorhersehbare Performance

---

## Enterprise-Eignung

### ✅ Genehmigt für Enterprise-Nutzung

| Kriterium | Status | Hinweise |
|-----------|--------|----------|
| **Open Source** | ✅ Ja | Alle MIT-lizenziert |
| **Kommerzielle Nutzung** | ✅ Erlaubt | Keine Einschränkungen |
| **On-Premise** | ✅ Unterstützt | Dafür konzipiert |
| **Datenschutz** | ✅ Konform | Keine externen Aufrufe |
| **Vendor Lock-in** | ✅ Keiner | Offene Standards |
| **Modifikationen** | ✅ Erlaubt | Anpassbar |
| **Verbreitung** | ✅ Erlaubt | Kann gebündelt werden |
| **Support** | ✅ Verfügbar | Aktive Communities |

---

## Modell-Lizenzen

**Wichtig:** Während die Inferenz-Bibliotheken MIT-lizenziert sind, können KI-Modelle separate Lizenzen haben.

### Whisper-Modelle

- **Quelle:** OpenAI / HuggingFace (ggerganov/whisper.cpp)
- **Lizenz:** Apache 2.0 / MIT
- **Kommerzielle Nutzung:** ✅ Erlaubt
- **Modelle:** tiny, base, small, medium, large (alle offen)

### Piper-Stimmenmodelle

- **Quelle:** rhasspy/piper-voices (HuggingFace)
- **Lizenz:** Meist MIT, einige CC-BY
- **Kommerzielle Nutzung:** ✅ Meiste erlauben kommerzielle Nutzung
- **Stimmen:** 50+ Sprachen, 100+ Stimmen

### LLM-Modelle (für llama.cpp)

Verschiedene Modelle haben verschiedene Lizenzen:

| Modell | Lizenz | Kommerzielle Nutzung |
|--------|--------|---------------------|
| LLaMA 2 | Meta-Lizenz | ✅ Erlaubt (<700M Nutzer) |
| Mistral | Apache 2.0 | ✅ Erlaubt |
| Phi-3 | MIT-Lizenz | ✅ Erlaubt |
| Gemma | Google ToS | ✅ Erlaubt |

**Empfehlung:** Verwenden Sie Mistral oder Phi-3 für uneingeschränkte kommerzielle Nutzung.

---

## Attributionsanforderungen

Gemäß MIT-Lizenzbedingungen ist folgende Attribution einzuschließen:

```
Diese Software verwendet folgende Open-Source-Bibliotheken:

Whisper.cpp - Spracherkennung
Copyright (c) 2022 Georgi Gerganov
MIT-Lizenz - https://github.com/ggerganov/whisper.cpp

Piper TTS - Text-zu-Sprache-Synthese
Copyright (c) 2023 Michael Hansen
MIT-Lizenz - https://github.com/rhasspy/piper

llama.cpp - LLM-Inferenz
Copyright (c) 2023 Georgi Gerganov
MIT-Lizenz - https://github.com/ggerganov/llama.cpp

ONNX Runtime - Neuronale Netzwerk-Inferenz
Copyright (c) Microsoft Corporation
MIT-Lizenz - https://github.com/microsoft/onnxruntime
```

---

## Compliance-Checkliste

Für On-Premise-Enterprise-Deployment:

- [x] Alle Bibliotheken sind Open-Source
- [x] Alle Bibliotheken verwenden permissive Lizenzen (MIT)
- [x] Kommerzielle Nutzung ist explizit erlaubt
- [x] On-Premise-Deployment wird unterstützt
- [x] Keine externen API-Abhängigkeiten
- [x] Keine Nutzungsbeschränkungen oder Messung
- [x] Quellcode-Modifikationen erlaubt
- [x] Kann mit proprietärer Software gebündelt werden
- [x] Attributionsanforderungen sind minimal
- [x] Keine Copyleft-Verpflichtungen
- [x] Keine Patent-Bedenken
- [x] DSGVO-konforme Architektur
- [x] Geeignet für sensible Datenverarbeitung
- [x] Kein Vendor Lock-in

---

## Rechtlicher Hinweis

Dieses Dokument stellt Informationen über die Lizenzen von Drittanbieter-Bibliotheken bereit, die im ThemisDB Sprachassistenten-Feature verwendet werden. Es dient nur zu Informationszwecken und stellt keine Rechtsberatung dar. Bei spezifischen rechtlichen Fragen konsultieren Sie bitte einen qualifizierten Anwalt.

**Letzte Aktualisierung:** Dezember 2025  
**Überprüfung:** Jährlich oder bei Aktualisierung der Bibliotheksversionen empfohlen

---

## Referenzen

- [MIT-Lizenz](https://opensource.org/licenses/MIT)
- [Whisper.cpp Repository](https://github.com/ggerganov/whisper.cpp)
- [Piper TTS Repository](https://github.com/rhasspy/piper)
- [llama.cpp Repository](https://github.com/ggerganov/llama.cpp)
- [ONNX Runtime Repository](https://github.com/microsoft/onnxruntime)
- [Open Source Initiative](https://opensource.org/)
- [SPDX License List](https://spdx.org/licenses/)

---

## Kontakt

Für Fragen zu Lizenzen oder Enterprise-Deployment:
- **Technisch:** GitHub Issues
- **Kommerziell:** sales@themisdb.com
- **Rechtlich:** legal@themisdb.com
