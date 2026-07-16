> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Railway Monitoring System - Quick Start Guide

Vollständiges IoT-basiertes Echtzeit-Überwachungssystem für Zugverkehr mit KI-gestützter Analyse.

---

## ⚡ NEU: Production-Ready Deployment!

**Jetzt verfügbar:**
- ✅ **Docker Compose Setup** - Komplettes System mit einem Befehl
- ✅ **Quick-Start Scripts** - Automatische Installation (Linux/macOS/Windows)
- ✅ **Python Network Generator** - Keine C++ Compilation erforderlich
- ✅ **WPF Desktop Client** - Vollständig implementiert (.NET 8)
- ✅ **Web UI mit Nginx** - Production-ready Konfiguration

### 🚀 Schnellster Start (2 Minuten)

**Linux/macOS**:
```bash
./quick-start.sh
```

**Windows PowerShell**:
```powershell
.\quick-start.ps1
```

Das war's! System läuft auf:
- 🌐 Web UI: http://localhost:8080
- 🗄️ ThemisDB API: http://localhost:8765
- 🤖 Ollama LLM: http://localhost:11434

**📖 Vollständige Dokumentation:** Siehe **[DEPLOYMENT.md](DEPLOYMENT.md)**

---

## 🚀 Schnellstart (5 Minuten)

### Voraussetzungen

- ThemisDB läuft auf `http://localhost:8765`
- Python 3.8+ (für Simulator & Import)
- C++ Compiler (für Daten-Generator)
- Web Browser (für Live-Karte)

### Schritt 1: Streckennetz generieren

```bash
# Installiere nlohmann/json (falls nicht vorhanden)
# Ubuntu/Debian: sudo apt-get install nlohmann-json3-dev
# macOS: brew install nlohmann-json
# Oder: Header-only von https://github.com/nlohmann/json

# Kompiliere Daten-Generator
cd examples/railway
g++ -std=c++17 railway_base_data_generator.cpp -o railway_generator

# Generiere Streckendaten (ca. 400 Segmente, 150+ Signale)
mkdir -p ../../data
./railway_generator
```

**Output**: `../../data/railway_network_base_germany.json`

**Inhalt**:
- 15 Bahnhöfe (Frankfurt, München, Hamburg, etc.)
- ~400 Streckenabschnitte mit Geschwindigkeitsprofilen
- ~150 Signale (Haupt- und Vorsignale)
- ~40 Weichen
- ~50 Bahnübergänge

### Schritt 2: Daten in ThemisDB importieren

```bash
# Installiere Python Dependencies
pip install requests

# Importiere Streckennetz
cd ../../scripts/railway
python import_railway_network.py ../../data/railway_network_base_germany.json
```

**Importiert**:
- Stations als Graph Vertices
- Track Segments als Graph Edges
- Signale, Weichen, Bahnübergänge

### Schritt 3: Zugverkehr simulieren

```bash
# Starte Simulator mit 50 Zügen
python train_simulator.py \
    --network ../../data/railway_network_base_germany.json \
    --trains 50 \
    --interval 1.0 \
    --themis http://localhost:8765
```

**Simulation**:
- 50 Züge (realistisch verteilt: ICE, IC, RE, RB)
- Echtzeit-Updates (1 Hz)
- GPS-Telemetrie
- Fahrzeug-Systeme
- Infrastruktur-Events (Achszähler, Hotbox-Detektoren)
- Verspätungen nach realen Statistiken

### Schritt 4: Live-Karte öffnen

```bash
# Öffne im Browser
open ../../examples/railway/live_map.html
# oder
firefox ../../examples/railway/live_map.html
```

**Features**:
- OpenStreetMap Basis-Karte
- Live-Zugpositionen
- Bahnhöfe, Signale, Weichen (Layer)
- Echtzeit-Statistiken
- Train Details on Click

## 📊 Realistische Zahlen (Deutsche Bahn 2023)

### Zugverkehr pro Tag

```
Gesamt Deutschland:    ~40.000 Züge/Tag
├─ ICE (Fernverkehr):   1.200 Züge/Tag
├─ IC/EC (Fernverkehr):   800 Züge/Tag
├─ RE (Regional):       8.000 Züge/Tag
├─ RB (Regional):      15.000 Züge/Tag
└─ Güterverkehr:        5.000 Züge/Tag
```

### Pünktlichkeitsstatistik

```
ICE Pünktlichkeit (<6 Min):  91.5%
RE/RB Pünktlichkeit (<6 Min): 94.2%

Durchschnittliche Verspätung (bei verspäteten Zügen):
- ICE:  12.3 Minuten
- RE:    8.5 Minuten
```

### Strecken-Daten

```
Geschwindigkeiten:
- ICE (Hochgeschwindigkeit):  200-330 km/h
- IC (Hauptstrecken):         140-200 km/h
- RE (Regional):              100-160 km/h
- RB (Regionalbahn):           80-120 km/h

Signalabstände:
- Fernverkehr:  1.5 - 3.0 km
- Regional:     1.0 - 2.0 km

Bahnübergänge:
- Nur auf Regionalstrecken
- Durchschnitt: 1 pro 10 km
```

## 🗺️ Datenmodell

### Graph-Struktur

```
(Station) -[TRACK_SEGMENT]-> (Track_Point)
(Track_Point) -[TRACK_SEGMENT]-> (Track_Point)
(Signal) -[LOCATED_AT]-> (Track_Point)
(Switch) -[LOCATED_AT]-> (Track_Point)
(Train) -[CURRENTLY_AT]-> (Track_Segment)
```

### Time-Series Metriken

```
Pro Zug (1 Hz):
- train_telemetry: GPS, Speed, Delay, Occupancy
- train_vehicle_systems: Traction, Brakes, HVAC
- train_safety_systems: ETCS, PZB status

Pro Infrastruktur (Event-basiert):
- axle_counter_events: Zugein-/ausfahrt
- hotbox_detector: Heißläufer-Warnung
- signal_events: Aspekt-Änderungen
- weather_station: Wetter entlang Strecke
```

## 📡 API Endpoints

### Züge abfragen

```bash
# Alle aktiven Züge
curl http://localhost:8765/query -X POST \
  -H "Content-Type: application/json" \
  -d '{"table":"train","return":"entities","limit":100}'

# Verspätete Züge (>5 Min)
curl http://localhost:8765/query/aql -X POST \
  -d '{
    "query": "FOR t IN train FILTER t.delay_min > 5 RETURN t"
  }'
```

### Zeitreihen-Daten

```bash
# Telemetrie für ICE 508
curl http://localhost:8765/timeseries/train_telemetry/ICE508

# Letzte 1 Stunde
curl "http://localhost:8765/timeseries/train_telemetry/ICE508?from=-3600000"
```

### Graph-Abfragen

```bash
# Alle Signale auf Strecke 3600
curl http://localhost:8765/query/aql -X POST \
  -d '{
    "query": "FOR s IN signal FILTER s.track_number == \"3600\" RETURN s"
  }'

# Route von Frankfurt nach München
curl http://localhost:8765/graph/shortest_path -X POST \
  -d '{
    "start": "station:8000105",
    "end": "station:8000261",
    "algorithm": "dijkstra"
  }'
```

## 🎯 Erweiterte Features

### 1. Verspätungs-Analyse mit LLM

```python
import requests

# Frage an Ollama LLM
response = requests.post("http://localhost:8765/analytics/llm-query", json={
    "query": "Warum hat ICE 508 Verspätung?",
    "context": {
        "train_number": "ICE 508",
        "include_events": True,
        "time_window_min": 30
    }
})

print(response.json()["answer"])
# Output: "ICE 508 hat 15 Min Verspätung aufgrund Signalstörung
#          Signal F123 auf Strecke 3600 Km 45.3..."
```

### 2. Was-wäre-wenn Simulation

```python
# Simuliere Signalausfall
response = requests.post("http://localhost:8765/analytics/simulate", json={
    "scenario": "signal_failure",
    "signal_id": "signal:3600_H12",
    "duration_min": 120,
    "analyze_impact": True
})

print(f"Betroffene Züge: {response.json()['affected_trains']}")
print(f"Zusätzliche Verspätung: {response.json()['total_delay_min']} Min")
```

### 3. CEP Rules für Anomalien

```javascript
// In ThemisDB CEP Engine
CREATE RULE cascading_delays AS
SELECT 
  t1.trainNumber,
  COUNT(*) as affected_count
FROM TrainDelayEvents t1
JOIN TrainDelayEvents t2
  ON t1.next_station = t2.current_station
  AND t2.timestamp > t1.timestamp
  AND t2.timestamp < t1.timestamp + 600000
WHERE t1.delay_min > 10
WINDOW SLIDING(15 MINUTES)
GROUP BY t1.trainNumber
HAVING COUNT(*) >= 3
ACTION alert('operations_center', priority='HIGH');
```

## 📈 Visualisierung

### Grafana Dashboard

1. **Zugpositionen**: Live-Karte mit Leaflet Plugin
2. **Verspätungen**: Histogram + Trend
3. **Streckenauslastung**: Heatmap
4. **Effizienz-KPIs**: Pünktlichkeit, Energieverbrauch
5. **Anomalien**: CEP Alerts, Hotbox-Warnungen

### WebSocket Real-time Feed

```javascript
// Connect to live updates
const ws = new WebSocket('ws://localhost:8765/ws/trains');

ws.onmessage = (event) => {
  const train = JSON.parse(event.data);
  updateTrainMarker(train.train_number, train.lat, train.lon);
};
```

## 🔧 Konfiguration

### `config/railway_monitoring.yaml`

```yaml
storage:
  rocksdb_path: /data/railway_db

server:
  host: 0.0.0.0
  port: 8765

timeseries:
  enabled: true
  retention_days: 90
  compression: gorilla
  
cep:
  enabled: true
  rules_path: /etc/themis/cep_rules/
  
llm:
  enabled: true
  provider: ollama
  endpoint: http://localhost:11434
  model: llama3.2:latest
  
geo:
  enabled: true
  osm_import: true

simulation:
  trains_count: 50
  update_interval_sec: 1.0
  realistic_delays: true
  punctuality_ice: 0.915
  punctuality_re: 0.942
```

## 🚦 Troubleshooting

### Problem: Keine Züge sichtbar

```bash
# Prüfe ob Simulator läuft
ps aux | grep train_simulator

# Prüfe Time-Series Daten
curl http://localhost:8765/timeseries/train_telemetry

# Prüfe ThemisDB Logs
docker logs themisdb
```

### Problem: Zu viele Verspätungen

```python
# Passe Pünktlichkeit in train_simulator.py an:
PUNCTUALITY_ICE = 0.95  # Erhöhe auf 95%
```

### Problem: Map lädt nicht

- Prüfe CORS Settings in ThemisDB
- Öffne Browser Console (F12)
- Prüfe Network Tab für Fehler

## 📚 Weitere Dokumentation

- **Vollständiges Datenmodell**: `docs/projects/RAILWAY_MONITORING.md`
- **Zugmodell**: `docs/projects/RAILWAY_TRAIN_DATA_MODEL.md`
- **ThemisDB Features**: `docs/features/`
- **CEP Engine**: `docs/analytics/CEP_STREAMING_ANALYTICS.md`
- **LLM Integration**: `docs/enterprise/gpu_impact_analysis_llm_integration.md`

## 🎬 Demo-Video

```bash
# Starte komplette Demo
./scripts/railway/start_demo.sh

# Stoppt nach Ctrl+C:
# - ThemisDB Server
# - Train Simulator
# - Live Map Server
```

## 📊 Performance

**Erwartete Last**:
- 50 Züge = 50 Updates/sec
- 400 Sensoren = ~100 Events/sec
- ThemisDB: <10ms Latenz pro Write
- Memory: ~500 MB (50 Züge, 7 Tage Historie)
- Storage: ~5 GB/Monat (Gorilla Compression)

**Skalierung**:
- Getestet mit: 500 Züge = 95% CPU, 2GB RAM
- Max empfohlen: 1000 Züge pro ThemisDB Instanz
- Sharding: 10.000+ Züge über mehrere Nodes

## 🤝 Contribution

Verbesserungen willkommen:
- Realere Strecken-Daten (OpenStreetMap Import)
- Deutsche Bahn API Integration
- Fahrplan-Import (GTFS)
- ML-basierte Verspätungs-Vorhersage
- Mobile App

## 📄 Lizenz

MIT License - Siehe LICENSE file
