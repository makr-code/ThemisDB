> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Drohnenbild-Analyse - KI-gestützte Echtzeit-Analyse mit LLM

![Status](https://img.shields.io/badge/status-planned-yellow)
![Difficulty](https://img.shields.io/badge/difficulty-very_complex-darkred)
![Duration](https://img.shields.io/badge/duration-90--120%20min-blue)

## 📝 Übersicht

Hochmodernes System für Echtzeit-Analyse von Drohnenbildern mit Computer Vision, LLM-Integration und geografischer Auswertung. Demonstriert alle fortgeschrittenen Features von ThemisDB.

## ✨ Features

- ✅ **Drohnen-Bilddaten** - Simulation und Live-Feed Support
- ✅ **Computer Vision** - OpenCV für Bildverarbeitung
- ✅ **Objekt-Erkennung** - YOLO/CLIP Integration
- ✅ **LLM-Integration** - ThemisDB Native LLM (llama.cpp)
- ✅ **Bildanalyse-Plugins** - Multi-Backend (llama.cpp Vision, ONNX CLIP, OpenCV DNN)
- ✅ **Bildbeschreibungen** - Automatische Caption-Generierung
- ✅ **Geo-Tagging** - GPS-Koordinaten und räumliche Queries
- ✅ **Zeitreihen-Analyse** - Veränderungen über Zeit tracken
- ✅ **Echtzeit-Streaming** - Server-Sent Events für Live-Updates
- ✅ **Vector Search** - Ähnliche Bilder finden
- ✅ **Komplexes Dashboard** - Multi-Panel Interface
- ✅ **Export & Reporting** - Analyse-Berichte generieren

## 📊 Datenmodell

### Drohnenbild
```python
{
    "id": "image_uuid",
    "drone_id": "drone_001",
    "timestamp": "2025-12-22T14:30:45.123Z",
    "location": {
        "lat": 52.5200,
        "lon": 13.4050,
        "altitude": 120,  # Meter
        "heading": 270,   # Grad
        "tilt": -45       # Grad
    },
    "image_path": "/data/images/img_001.jpg",
    "thumbnail_path": "/data/thumbnails/thumb_001.jpg",
    "metadata": {
        "resolution": "4096x3072",
        "format": "jpg",
        "size_mb": 8.5,
        "camera": "DJI Mavic 3",
        "iso": 100,
        "shutter_speed": "1/1000"
    }
}
```

### Bildanalyse (Vector + Metadata)
```python
{
    "id": "analysis_uuid",
    "image_id": "image_uuid",
    "embedding": [0.123, -0.456, ...],  # 512D CLIP embedding
    "detected_objects": [
        {
            "type": "vehicle",
            "confidence": 0.95,
            "bbox": [100, 150, 250, 300],  # x, y, w, h
            "classification": "car"
        },
        {
            "type": "building",
            "confidence": 0.88,
            "bbox": [300, 50, 500, 400]
        }
    ],
    "llm_description": "Luftaufnahme eines städtischen Gebiets mit mehreren Gebäuden...",
    "scene_classification": ["urban", "daytime", "clear_weather"],
    "quality_score": 0.92,
    "processing_time_ms": 2340
}
```

### Zeitreihen-Event
```python
{
    "id": "event_uuid",
    "location": {
        "lat": 52.5200,
        "lon": 13.4050
    },
    "event_type": "construction_progress",
    "description": "Neues Gebäude, Bauphase 3",
    "images": ["img_001", "img_045", "img_089"],
    "detected_at": "2025-12-22T14:30:00Z",
    "confidence": 0.87
}
```

## 🔧 Installation

```bash
cd examples/10_drone_image_analysis
pip install -r requirements.txt

# YOLO-Modell herunterladen (einmalig)
python download_models.py

# Optional: ThemisDB mit LLM-Support
# Siehe LLM_INTEGRATION.md
```

## 🚀 Start

```bash
python main.py
```

## 📚 Umfangreiche Dokumentation

### Benutzer-Dokumentation
- [README.md](README.md) - Übersicht und Features
- [HOW_TO.md](HOW_TO.md) - Schritt-für-Schritt-Anleitung
- [TUTORIAL.md](TUTORIAL.md) - Komplettes Tutorial

### Technische Dokumentation
- [ARCHITECTURE.md](ARCHITECTURE.md) - System-Design und Architektur
- [LLM_INTEGRATION.md](LLM_INTEGRATION.md) - LLM-Setup und Konfiguration
- [IMAGE_PROCESSING.md](IMAGE_PROCESSING.md) - CV-Pipeline Details
- [PERFORMANCE_TUNING.md](PERFORMANCE_TUNING.md) - Optimierung
- [DEPLOYMENT.md](DEPLOYMENT.md) - Production Deployment
- [TROUBLESHOOTING.md](TROUBLESHOOTING.md) - Fehlerbehebung

## 📚 Was Sie lernen

### ThemisDB Features
- **Multi-Model Integration** - Alle Features kombiniert
- **Vector + Geo + Time-Series** - Komplexe Queries
- **Native LLM** - llama.cpp Integration
- **Image Analysis Plugins** - Multi-Backend Support
- **Streaming** - Server-Sent Events
- **Performance** - GPU-Acceleration

### AI/ML Stack
- **Computer Vision** - OpenCV, PIL
- **Object Detection** - YOLO, CLIP
- **LLM Inference** - llama.cpp
- **Embeddings** - CLIP für Bilder
- **Transfer Learning** - Fine-tuning

### Software Engineering
- **Complex UI** - Multi-Panel Tkinter
- **Threading** - Parallele Verarbeitung
- **Streaming** - Real-time Updates
- **Error Handling** - Robuste Fehlerbehandlung
- **Logging** - Strukturiertes Logging

## 🎯 Use Cases

1. **Baufortschritt-Überwachung**
   - Automatische Erkennung von Änderungen
   - Zeitreihen-Vergleich
   - Report-Generierung

2. **Katastrophen-Management**
   - Schnelle Schadenserfassung
   - Priorisierung von Einsatzgebieten
   - Ressourcen-Planung

3. **Landwirtschaft**
   - Feld-Monitoring
   - Pflanzenwachstum tracken
   - Schädlings-Erkennung

4. **Infrastruktur-Inspektion**
   - Straßen, Brücken, Pipelines
   - Automatische Schadens-Erkennung
   - Wartungs-Planung

5. **Umwelt-Monitoring**
   - Wald-Monitoring
   - Gewässer-Überwachung
   - Wildtier-Tracking

## 🏗️ Architektur

```
┌─────────────────────────────────────────┐
│         Tkinter UI Dashboard            │
│  ┌───────────┐ ┌──────────┐ ┌────────┐ │
│  │ Map View  │ │ Timeline │ │ Images │ │
│  └───────────┘ └──────────┘ └────────┘ │
└────────────┬────────────────────────────┘
             │
┌────────────▼────────────────────────────┐
│      Application Layer (Python)         │
│  ┌──────────┐ ┌──────────┐ ┌─────────┐ │
│  │ CV Proc. │ │ LLM Infer│ │ Geo Ops │ │
│  └──────────┘ └──────────┘ └─────────┘ │
└────────────┬────────────────────────────┘
             │
┌────────────▼────────────────────────────┐
│         ThemisDB Multi-Model            │
│  ┌─────────┐ ┌────────┐ ┌────────────┐ │
│  │ Vector  │ │  Geo   │ │ Time-Series│ │
│  └─────────┘ └────────┘ └────────────┘ │
└─────────────────────────────────────────┘
```

## 🧠 AI-Pipeline

1. **Bilderfassung** - Drohne oder Simulation
2. **Vorverarbeitung** - Resize, Normalisierung
3. **Object Detection** - YOLO für Objekte
4. **Embedding-Generierung** - CLIP für Vektorsuche
5. **LLM-Beschreibung** - llama.cpp für Captions
6. **Geo-Tagging** - GPS-Koordinaten zuordnen
7. **Speicherung** - ThemisDB Multi-Model
8. **Indexierung** - Vector + Spatial Indexes

## ⚡ Performance

- **GPU-Beschleunigung** - CUDA für CV und LLM
- **Batch-Processing** - Mehrere Bilder parallel
- **Caching** - Embeddings und Predictions
- **Streaming** - Progressive Loading
- **Optimierte Queries** - Index-basiert

## 🔒 Sicherheit

- **Verschlüsselung** - Bilder at-rest und in-transit
- **Zugriffskontrolle** - RBAC für Missionen
- **Audit-Log** - Alle Zugriffe protokolliert
- **Anonymisierung** - Gesichter und Kennzeichen

---

**Status**: Geplant | Flagship-Beispiel für ThemisDB Capabilities

**Hinweis**: Dies ist das komplexeste Beispiel und benötigt:
- ThemisDB mit LLM-Support
- GPU (empfohlen für Performance)
- Min. 16GB RAM
- ~10GB Speicher für Modelle
