> **Aktueller Build-Flow:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Drohnenbild-Analyse - Bedienungsanleitung

## 🚀 Schnellstart

```bash
# Installation
cd examples/10_drone_image_analysis
pip install -r requirements.txt
python download_models.py

# Starten
python main.py
```

## 📋 Hauptfunktionen

### Mission erstellen

1. **Neue Mission**:
   - Tab "Missionen" → "Neu"
   - Name, Beschreibung, Gebiet eingeben
   - Start/End-Koordinaten auf Karte markieren
   - Flughöhe und -geschwindigkeit setzen
   - Speichern

2. **Drohne konfigurieren**:
   - Kamera-Einstellungen
   - Aufnahme-Intervall
   - Überlappung (für Photogrammetrie)
   - GPS-Genauigkeit

### Bilder erfassen

**Simulation**:
1. Tab "Erfassung" → "Simulation starten"
2. Virtuelle Drohne fliegt Route
3. Bilder werden automatisch generiert
4. Echtzeit-Vorschau auf Karte

**Live-Drohne** (wenn verfügbar):
1. Drohne verbinden (USB/WiFi)
2. Kalibrierung durchführen
3. Mission hochladen
4. Start → Automatischer Flug

### Bildanalyse

**Automatische Analyse**:
- System analysiert Bilder automatisch
- Objekt-Erkennung läuft im Hintergrund
- LLM generiert Beschreibungen
- Embeddings für Suche erstellt

**Manuelle Analyse**:
1. Bild aus Galerie auswählen
2. "Analysieren" → Verschiedene Modi:
   - **Objekt-Erkennung**: Autos, Gebäude, Bäume
   - **Szenen-Klassifikation**: Urban, Rural, Forest
   - **LLM-Beschreibung**: Detaillierte Caption
   - **Ähnliche Bilder**: Vector Search

### Dashboard

**Karten-Ansicht**:
- Alle Bildpositionen als Marker
- Klick auf Marker → Thumbnail
- Farbcodierung nach Analyse-Status
- Zeitfilter für Animation

**Timeline**:
- Chronologische Bildabfolge
- Scrubbing durch Zeit
- Vergleichs-Modus (vorher/nachher)
- Auto-Play für Video-ähnliche Ansicht

**Statistiken**:
- Anzahl Bilder, Objekte, Events
- Abdeckungs-Heatmap
- Objekttyp-Verteilung
- Analyse-Performance

### Suche und Filter

**Textsuche**:
- Natürliche Sprache: "Zeige alle Bilder mit Autos"
- LLM-generierte Beschreibungen durchsuchen
- Fuzzy-Matching

**Bildsuche**:
- Bild hochladen → Ähnliche finden
- Vector-basierte Suche
- Threshold anpassbar

**Filter**:
- Zeitraum
- Geo-Bereich (Polygon auf Karte)
- Objekttypen
- Qualitäts-Score
- Wetterbedingungen

### Ereignis-Erkennung

**Automatische Events**:
System erkennt automatisch:
- **Veränderungen**: Neue Gebäude, Straßen
- **Anomalien**: Schäden, ungewöhnliche Objekte
- **Aktivitäten**: Bauarbeiten, Verkehr

**Event-Management**:
1. Tab "Ereignisse"
2. Liste aller erkannten Events
3. Event auswählen → Details und Bilder
4. Bestätigen oder Verwerfen
5. Export für Berichte

### LLM-Integration

**Bildbeschreibungen**:
- Automatisch bei Analyse
- Oder manuell: "Beschreibe dieses Bild"
- Detailgrad einstellbar
- Sprache wählbar (DE/EN)

**Interaktive Queries**:
1. Bild auswählen
2. Frage eingeben: "Was ist das für ein Gebäude?"
3. LLM antwortet basierend auf Bildinhalt

**Batch-Processing**:
- Mehrere Bilder gleichzeitig
- Fortschrittsanzeige
- Resultat-Export

### Export und Reports

**Bilder exportieren**:
- Original oder verarbeitet
- Mit/ohne Annotations
- Format: JPG, PNG, GeoTIFF

**Report generieren**:
1. Zeitraum und Gebiet wählen
2. Template auswählen:
   - Executive Summary
   - Detailbericht
   - Change Detection
3. Format: PDF, HTML, DOCX
4. Generieren → Download

## ⌨️ Tastenkombinationen

### Navigation
- `Tab` - Zwischen Panels wechseln
- `Ctrl+M` - Karte fokussieren
- `Ctrl+T` - Timeline fokussieren
- `Ctrl+G` - Galerie öffnen

### Aktionen
- `Space` - Play/Pause Timeline
- `←/→` - Vorheriges/Nächstes Bild
- `Ctrl+A` - Bild analysieren
- `Ctrl+S` - Suche öffnen
- `Ctrl+E` - Event erstellen
- `Ctrl+R` - Report generieren

### Ansicht
- `+/-` - Zoom Karte
- `F11` - Vollbild
- `Ctrl+1/2/3` - Panel-Layouts
- `H` - Hilfe anzeigen

## 💡 Best Practices

### Bilderfassung
1. **Überlappung**: 70-80% für Photogrammetrie
2. **Lichtverhältnisse**: Vormittags oder spätnachmittags
3. **Flughöhe**: Je nach Ziel (höher = Überblick, niedriger = Details)
4. **Geschwindigkeit**: Langsamer = schärfere Bilder
5. **Batterie**: Immer Reserve einplanen

### Analyse
1. **Modell wählen**: YOLO für Objekte, CLIP für Szenen
2. **Batch-Size**: Größer für GPU, kleiner für CPU
3. **Confidence-Threshold**: 0.5-0.7 für Balance
4. **LLM-Prompts**: Spezifisch formulieren
5. **Caching**: Aktivieren für wiederholte Analysen

### Performance
1. **GPU nutzen**: Deutlich schneller
2. **Bilder vorverarbeiten**: Einmal, cache dann
3. **Parallelisierung**: Thread-Count anpassen
4. **Speicher**: Thumbnails für UI, Originale für Analyse
5. **Indexes**: Spatial + Vector Indexes anlegen

### Sicherheit
1. **Zugriff beschränken**: Sensible Gebiete
2. **Verschlüsselung**: Bilder at-rest
3. **Anonymisierung**: Gesichter/Kennzeichen
4. **Audit**: Alle Zugriffe loggen
5. **Backup**: Regelmäßig Missionen sichern

## 🔍 Erweiterte Features

### Change Detection

Vergleicht Bilder über Zeit:

1. **Gebiet wählen**: Polygon auf Karte
2. **Zeitpunkte**: t1 und t2 auswählen
3. **Analyse starten**: System findet Unterschiede
4. **Visualisierung**: Overlay mit Änderungen
5. **Report**: Automatisch generiert

**Erkannte Änderungen**:
- Neue/entfernte Gebäude
- Vegetation-Veränderungen
- Straßen/Wege
- Temporäre Strukturen

### Photogrammetrie-Integration

Erstellt 3D-Modelle aus Bildern:

1. **Bildset auswählen**: Mit Überlappung
2. **Kalibrierung**: Kamera-Parameter
3. **Processing**: Externe Software (OpenDroneMap)
4. **Import**: 3D-Modell in System
5. **Visualisierung**: In separatem Viewer

### Machine Learning

**Eigene Modelle trainieren**:

1. **Daten annotieren**: Bounding Boxes zeichnen
2. **Dataset erstellen**: Export für Training
3. **Training**: Mit YOLO/TensorFlow
4. **Import**: Neues Modell in System
5. **Evaluation**: Auf Test-Set

### API-Integration

Externe Systeme anbinden:

```python
# REST API
POST /api/v1/images/analyze
{
  "image_id": "uuid",
  "models": ["yolo", "clip", "llm"],
  "options": {...}
}
```

**Webhooks**:
- Bei neuen Bildern
- Bei Events
- Bei Analyse-Completion

## 🐛 Troubleshooting

**Problem**: Analyse sehr langsam
- **Lösung**: GPU aktivieren, Batch-Size erhöhen

**Problem**: LLM antwortet nicht
- **Lösung**: ThemisDB mit LLM-Support starten

**Problem**: Bilder nicht georeferenziert
- **Lösung**: GPS-Daten in EXIF prüfen

**Problem**: Objekt-Erkennung ungenau
- **Lösung**: Confidence-Threshold anpassen

**Problem**: Speicher voll
- **Lösung**: Alte Missionen archivieren

---

**Weitere Dokumentation**:
- [ARCHITECTURE.md](ARCHITECTURE.md) - System-Design
- [LLM_INTEGRATION.md](LLM_INTEGRATION.md) - LLM-Setup
- [IMAGE_PROCESSING.md](IMAGE_PROCESSING.md) - CV-Pipeline
- [PERFORMANCE_TUNING.md](PERFORMANCE_TUNING.md) - Optimierung
- [TROUBLESHOOTING.md](TROUBLESHOOTING.md) - Ausführliche Fehlersuche

**Support**: [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
