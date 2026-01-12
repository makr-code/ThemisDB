# LoRA Framework Dokumentations-Hub

**Version:** 1.0  
**Datum:** 2026-01-11  
**Status:** Vollständig

---

## Willkommen zur LoRA Framework Dokumentation

Dies ist der zentrale Hub für die gesamte Dokumentation des ThemisDB LoRA (Low-Rank Adaptation) Frameworks und seiner Anwendungen, insbesondere **themis_help_lora**, dem intelligenten Dokumentationsassistenten.

---

## 📚 Dokumentationsübersicht

### Für Endanwender

| Dokument | Beschreibung | Zielgruppe |
|----------|--------------|------------|
| [**themis_help_lora Benutzerhandbuch**](THEMIS_HELP_LORA_BENUTZERHANDBUCH.md) | Verwendung des Dokumentationsassistenten | Endanwender |
| [Schnellstart-Anleitung](#schnellstart) | Erste Schritte in 5 Minuten | Endanwender |
| [FAQ](#faq) | Häufig gestellte Fragen | Alle |

### Für Entwickler

| Dokument | Beschreibung | Zielgruppe |
|----------|--------------|------------|
| [**LoRA Framework Entwicklerhandbuch**](LORA_FRAMEWORK_ENTWICKLERHANDBUCH.md) | Vollständige API-Referenz und Architektur | Entwickler |
| [**Integrationsbeispiele**](LORA_INTEGRATIONSBEISPIELE.md) | Code-Beispiele in C++, Python, REST, AQL | Entwickler |
| [**Trainingsanleitung**](LORA_TRAININGSANLEITUNG.md) | Training und Feinabstimmung von Adaptern | ML-Ingenieure |

### Referenzdokumentation

| Dokument | Beschreibung | Zielgruppe |
|----------|--------------|------------|
| [AQL Funktionsreferenz](../../../LORA_AQL_REFERENCE.md) | Vollständige AQL-Funktionsreferenz | Entwickler |
| [Architekturübersicht](../../../LLM_LORA_UNIFIED_ARCHITECTURE.md) | Systemarchitektur und Design | Architekten |
| [Verwendungsbeispiele](../../../LORA_USAGE_EXAMPLES.md) | Praktische Beispiele und Muster | Entwickler |

---

## 🚀 Schnellstart

### Für Endanwender (Verwendung von themis_help_lora)

**5-Minuten-Setup:**

1. **Zugriff auf die Web-UI**
   - Navigieren Sie zur ThemisDB Admin-UI
   - Klicken Sie auf "Dokumentationsassistent" in der Seitenleiste

2. **Stellen Sie eine Frage**
   ```
   Beispiel: "Wie aktiviere ich Sharding in ThemisDB?"
   ```

3. **Erhalten Sie Ihre Antwort**
   - Lesen Sie die generierte Antwort
   - Kopieren Sie Codebeispiele, falls vorhanden

4. **Geben Sie Feedback** (Optional)
   - Klicken Sie auf 👍, wenn hilfreich
   - Klicken Sie auf 👎 und geben Sie eine Korrektur an, falls nicht

**Das war's!** Sie verwenden nun themis_help_lora.

### Für Entwickler (Verwendung des LoRA Frameworks)

**Schnelle Integration:**

```cpp
#include "llm/lora_framework/lora_orchestrator.h"

// Initialisierung
LoRAOrchestrator::Config config;
config.db = database_wrapper;
auto orchestrator = std::make_shared<LoRAOrchestrator>(config);

// Adapter erstellen
TrainingData data;
data.base_model = "llama-2-7b";
data.samples = {/* Ihre Trainingsdaten */};

LoRAHyperparameters params;
params.rank = 8;
params.alpha = 16;

std::string job_id = orchestrator->createAdapter(
    "mein_adapter", "llama-2-7b", data, params
);

// Fertig!
```

---

## 📖 Lernpfade

### Pfad 1: Endanwender → Power-User

1. Beginnen Sie mit dem [themis_help_lora Benutzerhandbuch](THEMIS_HELP_LORA_BENUTZERHANDBUCH.md)
2. Lernen Sie, effektives Feedback zu geben
3. Erkunden Sie AQL-Abfragen (Grundstufe)
4. Lesen Sie die FAQ für häufige Fragen

**Zeitaufwand:** 30 Minuten  
**Ergebnis:** Effiziente Nutzung des Dokumentationsassistenten

### Pfad 2: Entwickler → Integration

1. Lesen Sie das [Entwicklerhandbuch](LORA_FRAMEWORK_ENTWICKLERHANDBUCH.md) (Architektur & Kernkomponenten)
2. Folgen Sie den [Integrationsbeispielen](LORA_INTEGRATIONSBEISPIELE.md) (C++ oder REST API)
3. Testen Sie mit Beispielcode
4. Deployment in Ihrer Anwendung

**Zeitaufwand:** 2-4 Stunden  
**Ergebnis:** Integriertes LoRA Framework in Ihrer Anwendung

### Pfad 3: ML-Ingenieur → Trainingsexperte

1. Lesen Sie das [Entwicklerhandbuch](LORA_FRAMEWORK_ENTWICKLERHANDBUCH.md) (Vollständiges Dokument)
2. Studieren Sie die [Trainingsanleitung](LORA_TRAININGSANLEITUNG.md)
3. Experimentieren Sie mit Hyperparametern
4. Trainieren Sie Ihren ersten Adapter
5. Überwachen und optimieren Sie

**Zeitaufwand:** 4-8 Stunden  
**Ergebnis:** Können LoRA-Adapter trainieren und optimieren

---

## 🎯 Häufige Anwendungsfälle

### 1. Verwendung von themis_help_lora für Dokumentationsabfragen

**Was Sie benötigen:**
- Zugriff auf ThemisDB-Instanz
- JWT-Token für Authentifizierung

**Ressourcen:**
- [Benutzerhandbuch](THEMIS_HELP_LORA_BENUTZERHANDBUCH.md)
- [REST API Beispiele](LORA_INTEGRATIONSBEISPIELE.md#rest-api-beispiele)

### 2. Training eines benutzerdefinierten Dokumentationsassistenten

**Was Sie benötigen:**
- Trainingsdaten (Frage-Antwort-Paare)
- GPU mit 16+ GB VRAM (empfohlen)
- Grundlegendes Verständnis von ML-Konzepten

**Ressourcen:**
- [Trainingsanleitung](LORA_TRAININGSANLEITUNG.md)
- [Hyperparameter-Optimierung](LORA_TRAININGSANLEITUNG.md#hyperparameter-optimierung)

### 3. Integration von LoRA in Ihrer Anwendung

**Was Sie benötigen:**
- C++ Entwicklungsumgebung
- ThemisDB SDK
- Vertrautheit mit LLMs

**Ressourcen:**
- [Entwicklerhandbuch](LORA_FRAMEWORK_ENTWICKLERHANDBUCH.md)
- [Integrationsbeispiele](LORA_INTEGRATIONSBEISPIELE.md)

### 4. Aufbau eines domänenspezifischen Assistenten

**Was Sie benötigen:**
- Domänenwissen
- Strategie zur Sammlung von Trainingsdaten
- Evaluierungsmetriken

**Ressourcen:**
- [Trainingsanleitung](LORA_TRAININGSANLEITUNG.md#vorbereitung-der-trainingsdaten)
- [Best Practices](LORA_TRAININGSANLEITUNG.md#best-practices)

---

## 💡 Kernkonzepte

### Was ist LoRA?

**LoRA (Low-Rank Adaptation)** ist eine Technik zur effizienten Feinabstimmung großer Sprachmodelle:

✅ **Effizient**: Trainiert kleine Adapter-Layer statt des gesamten Modells  
✅ **Schnell**: Viel schneller als vollständige Feinabstimmung  
✅ **Portabel**: Adapter sind klein (10-100 MB vs GB für vollständige Modelle)  
✅ **Reversibel**: Kann einfach zwischen Adaptern wechseln

### Was ist themis_help_lora?

**themis_help_lora** ist die erste Anwendung des LoRA Frameworks:

- **Zweck**: Dokumentationsassistent für ThemisDB
- **Training**: Trainiert auf ThemisDB-Dokumentation
- **Lernen**: Verbessert sich durch Benutzerfeedback
- **Ziel**: Reduzierung von Halluzinationen und Verbesserung der Genauigkeit

### Architekturübersicht

```
┌─────────────────────────────────────────┐
│          Anwendungsschicht               │
│  (themis_help_lora, custom assistants)  │
├─────────────────────────────────────────┤
│          LoRA Framework                  │
│  (Orchestrator, Training, Storage)      │
├─────────────────────────────────────────┤
│          LLM-Infrastruktur               │
│  (llama.cpp, Modellverwaltung)          │
├─────────────────────────────────────────┤
│          ThemisDB Core                   │
│  (Datenbank, Storage, Sicherheit)       │
└─────────────────────────────────────────┘
```

---

## 🔧 API-Schnellreferenz

### REST API

```bash
# Dokumentationsassistent abfragen
POST /api/v1/llm/docs/query
{
  "question": "Wie aktiviere ich Sharding?"
}

# Feedback geben
POST /api/v1/llm/docs/feedback
{
  "question": "...",
  "answer": "...",
  "feedback_type": "positive"
}

# Adapter auflisten
GET /api/v1/llm/lora/list

# Adapter erstellen
POST /api/v1/llm/lora/create
{
  "adapter_id": "mein_adapter",
  "base_model": "llama-2-7b",
  "training_data": {...},
  "hyperparameters": {...}
}
```

### AQL-Funktionen

```aql
// Adapter trainieren
LORA_TRAIN(adapter_id, base_model, data, params)

// Mit Adapter abfragen
LORA_QUERY(base_model, adapter_id, question, options)

// Ähnliche Adapter finden
LORA_SIMILAR(adapter_id, limit, threshold)

// Adapter-Statistiken abrufen
LORA_STATS(adapter_id, metrics)

// Adapter-Abstammung abrufen
LORA_LINEAGE(adapter_id, depth)

// Besten Adapter empfehlen
LORA_RECOMMEND(query, base_model, category, criteria)

// Adaptionspfad finden
LORA_PATH(from_model, to_model, max_depth)
```

### C++ API

```cpp
// Orchestrator (Hauptschnittstelle)
auto orchestrator = std::make_shared<LoRAOrchestrator>(config);
orchestrator->createAdapter(id, model, data, params);
orchestrator->getAdapter(id);
orchestrator->updateAdapter(id, data);
orchestrator->deleteAdapter(id);

// Anwendung
auto assistant = std::make_shared<ThemisHelpLoRA>(config);
assistant->query(question, user_id);
assistant->addPositiveFeedback(q, a, user_id);
assistant->trainFromFeedback();
```

---

## 📊 Feature-Matrix

### LoRA Framework Funktionen

| Feature | Status | Beschreibung |
|---------|--------|--------------|
| **Training** | ✅ Vollständig | Adapter aus Daten trainieren |
| **Inkrementelles Training** | ✅ Vollständig | Nachtraining aus Feedback |
| **Multi-Modell-Unterstützung** | ✅ Vollständig | Mehrere Basismodelle unterstützen |
| **Storage-Backends** | ✅ Vollständig | Dateisystem, ThemisDB, S3 |
| **Versionsverwaltung** | ✅ Vollständig | Semantische Versionierung & Rollback |
| **Sicherheit** | ✅ Vollständig | Verschlüsselung, Signaturen, Audit-Logs |
| **REST API** | ✅ Vollständig | Vollständige HTTP-API |
| **AQL-Integration** | ✅ Vollständig | 7 AQL-Funktionen |
| **Graph-Unterstützung** | ✅ Vollständig | Adapter-Abstammungs-Tracking |
| **Vektorsuche** | ✅ Vollständig | Semantische Ähnlichkeit |

### themis_help_lora Features

| Feature | Status | Beschreibung |
|---------|--------|--------------|
| **Dokumentations-Q&A** | ✅ Vollständig | ThemisDB-Fragen beantworten |
| **Feedback-Sammlung** | ✅ Vollständig | Aus Korrekturen lernen |
| **Auto-Nachtraining** | ✅ Vollständig | Nachtraining bei Schwellenwert |
| **Versionsverwaltung** | ✅ Vollständig | Verbesserungen nachverfolgen |
| **Web-UI** | ✅ Vollständig | Admin-UI-Integration |
| **REST API** | ✅ Vollständig | Programmatischer Zugriff |
| **AQL-Funktionen** | ✅ Vollständig | Abfrage aus Datenbank |
| **Audit-Logging** | ✅ Vollständig | Vollständige Rückverfolgbarkeit |

---

## 🐛 Fehlerbehebung

### Häufige Probleme

| Problem | Lösung | Referenz |
|---------|--------|----------|
| Adapter nicht gefunden | Adapter-ID prüfen und verfügbare Adapter auflisten | [Benutzerhandbuch](THEMIS_HELP_LORA_BENUTZERHANDBUCH.md#fehlerbehebung) |
| Training schlägt fehl | Trainingsdaten validieren, Hyperparameter prüfen | [Trainingsanleitung](LORA_TRAININGSANLEITUNG.md#fehlerbehebung) |
| Speicher voll | Batch-Größe reduzieren oder Gradient Checkpointing aktivieren | [Trainingsanleitung](LORA_TRAININGSANLEITUNG.md#problem-speicher-voll) |
| Langsame Inferenz | Caching aktivieren, Quantisierung verwenden | [Entwicklerhandbuch](LORA_FRAMEWORK_ENTWICKLERHANDBUCH.md#fehlerbehebung) |
| Authentifizierung fehlgeschlagen | JWT-Token aktualisieren | [Benutzerhandbuch](THEMIS_HELP_LORA_BENUTZERHANDBUCH.md#problem-authentifizierung-fehlgeschlagen) |

---

## 📞 Hilfe erhalten

### Ressourcen

1. **Dokumentation**: Beginnen Sie hier - die meisten Fragen werden in den Anleitungen beantwortet
2. **Beispiele**: Siehe [Integrationsbeispiele](LORA_INTEGRATIONSBEISPIELE.md) für Code
3. **FAQ**: Siehe unten für häufige Fragen
4. **GitHub Issues**: Fehler melden oder Features anfordern
5. **Community-Forum**: Diskutieren Sie mit anderen Benutzern

### Support-Kanäle

- 📧 **E-Mail**: support@themisdb.io
- 💬 **Forum**: https://forum.themisdb.io
- 🐛 **Issues**: https://github.com/makr-code/ThemisDB/issues
- 📚 **Docs**: https://docs.themisdb.io

---

## ❓ FAQ

### Allgemein

**F: Was ist der Unterschied zwischen LoRA Framework und themis_help_lora?**  
A: Das LoRA Framework ist die Infrastruktur zum Erstellen und Verwalten von Adaptern. themis_help_lora ist die erste Anwendung, die auf diesem Framework aufbaut - ein Dokumentationsassistent.

**F: Muss ich maschinelles Lernen verstehen, um themis_help_lora zu verwenden?**  
A: Nein! Endanwender müssen nur Fragen stellen und Feedback geben. ML-Kenntnisse sind nur erforderlich, wenn Sie benutzerdefinierte Adapter trainieren möchten.

**F: Kann ich meine eigenen Adapter erstellen?**  
A: Ja! Folgen Sie der [Trainingsanleitung](LORA_TRAININGSANLEITUNG.md), um zu lernen wie.

### Technisch

**F: Welche Basismodelle werden unterstützt?**  
A: Derzeit Llama-2 (7B, 13B, 70B) und Mistral (7B). Weitere Modelle können hinzugefügt werden.

**F: Wie viel GPU-Speicher benötige ich für das Training?**  
A: Minimum 16 GB für 7B-Modelle, 24+ GB empfohlen. Details in der [Trainingsanleitung](LORA_TRAININGSANLEITUNG.md).

**F: Kann ich ohne GPU arbeiten?**  
A: Inferenz funktioniert auf CPU (langsamer). Training erfordert GPU.

**F: Wie lange dauert das Training?**  
A: Typischerweise 15-60 Minuten für 500-1000 Samples auf einer einzelnen GPU.

### Deployment

**F: Ist themis_help_lora produktionsreif?**  
A: Ja! Es umfasst Sicherheit, Audit-Logging, Versionsverwaltung und Rollback-Funktionen.

**F: Wie aktualisiere ich einen Adapter in der Produktion?**  
A: Verwenden Sie Canary-Deployment oder A/B-Testing. Siehe [Entwicklerhandbuch](LORA_FRAMEWORK_ENTWICKLERHANDBUCH.md#best-practices).

**F: Was ist die Rollback-Strategie?**  
A: Automatisches Rollback bei Qualitätsabfall oder manuelles Rollback zu jeder früheren Version.

---

## 🗺️ Roadmap

### Aktuelle Version (v1.0)

✅ Vollständiges LoRA Framework  
✅ themis_help_lora Anwendung  
✅ REST API & AQL-Funktionen  
✅ Training-Pipeline  
✅ Dokumentation vollständig

### Geplante Features (v1.1)

- [ ] Mehrsprachige Unterstützung (DE, FR, ES)
- [ ] Streaming-Antworten
- [ ] Erweiterte Caching-Strategien
- [ ] Mehr Basismodelle (GPT, Claude)

### Zukunft (v2.0)

- [ ] Multi-modale Unterstützung (Vision, Audio)
- [ ] Föderiertes Lernen
- [ ] AutoML für Hyperparameter-Optimierung
- [ ] Interaktive Training-UI

---

**Willkommen im ThemisDB LoRA-Ökosystem!**

Ob Sie ein Benutzer sind, der Dokumentationsfragen stellt, oder ein Entwickler, der benutzerdefinierte Adapter erstellt - wir hoffen, dass diese Anleitungen Ihnen helfen, das Beste aus dem Framework herauszuholen.

**Viel Erfolg beim Programmieren!** 🚀

---

**Letzte Aktualisierung**: 2026-01-11  
**Version**: 1.0
