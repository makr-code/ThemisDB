> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# IoT-Sensornetzwerk - Echtzeit-Datenverarbeitung

![Status](https://img.shields.io/badge/status-planned-yellow)
![Difficulty](https://img.shields.io/badge/difficulty-complex-red)
![Duration](https://img.shields.io/badge/duration-60--90%20min-blue)

## 📝 Übersicht

Vollständiges IoT-System mit MQTT-Integration, Complex Event Processing (CEP), Anomalie-Erkennung und geografischer Visualisierung.

## ✨ Features

- ✅ **IoT-Sensor-Simulation** - Mehrere Sensor-Typen
- ✅ **MQTT Protocol** - Standard IoT-Kommunikation
- ✅ **Echtzeit-Verarbeitung** - Stream Processing
- ✅ **Complex Event Processing** - Pattern Matching
- ✅ **Anomalie-Erkennung** - ML-basiert (scikit-learn)
- ✅ **Geografische Karten** - Sensor-Standorte visualisieren
- ✅ **Alarmierung & Eskalation** - Multi-Level Alerts
- ✅ **Dashboard** - Multi-View Interface
- ✅ **Time-Series Optimierung** - Gorilla Compression

## 📊 Datenmodell

### Sensor
```python
{
    "id": "sensor_uuid",
    "type": "temperature",  # temperature, humidity, pressure, motion
    "name": "Sensor-Berlin-01",
    "location": {
        "lat": 52.5200,
        "lon": 13.4050,
        "altitude": 34
    },
    "config": {
        "interval": 60,  # Sekunden
        "thresholds": {
            "min": -20,
            "max": 50,
            "critical_min": -30,
            "critical_max": 60
        }
    },
    "status": "active"
}
```

### Messung (Time-Series)
```python
{
    "id": "measurement_uuid",
    "sensor_id": "sensor_uuid",
    "timestamp": "2025-12-22T10:30:45.123Z",
    "value": 23.5,
    "unit": "°C",
    "quality": 0.98,  # Datenqualität 0-1
    "anomaly_score": 0.05  # ML-Score
}
```

### Event (CEP)
```python
{
    "id": "event_uuid",
    "pattern": "high_temperature_sustained",
    "sensors": ["sensor1", "sensor2"],
    "triggered_at": "2025-12-22T10:35:00Z",
    "severity": "warning",  # info, warning, critical
    "description": "Temperatur über 45°C für 10 Minuten",
    "actions_taken": ["notification", "email"]
}
```

## 🔧 Installation

```bash
cd examples/09_iot_sensor_network
pip install -r requirements.txt
python main.py
```

## 📚 Dokumentation

- [README.md](README.md) - Diese Datei
- [HOW_TO.md](HOW_TO.md) - Bedienungsanleitung
- [SENSOR_SIMULATION.md](SENSOR_SIMULATION.md) - Sensor-Setup
- [CEP_PATTERNS.md](CEP_PATTERNS.md) - Event-Muster
- [ML_MODELS.md](ML_MODELS.md) - Anomalie-Erkennung
- [SCALING_GUIDE.md](SCALING_GUIDE.md) - Skalierung

## 📚 Was Sie lernen

- **Time-Series + CEP** - Kombination für Echtzeit
- **MQTT Integration** - IoT-Standard-Protokoll
- **Geo-Spatial Queries** - Standort-basierte Suche
- **ML Pipeline** - Training und Inference
- **Threading** - Parallele Sensor-Verarbeitung
- **Performance** - Optimierung für hohen Durchsatz

## 🎯 Use Cases

1. **Smart Building** - Gebäude-Monitoring
2. **Industrie 4.0** - Maschinen-Überwachung
3. **Smart City** - Städtische Infrastruktur
4. **Landwirtschaft** - Feld-Monitoring
5. **Umwelt-Monitoring** - Luft- und Wasserqualität

## 🧠 CEP-Pattern Beispiele

- **Schwellwert-Überschreitung** - Wert > Limit für X Sekunden
- **Trend-Erkennung** - Steigend/Fallend über Zeit
- **Korrelation** - Mehrere Sensoren gleichzeitig
- **Ausfall-Erkennung** - Sensor sendet nicht mehr
- **Anomalie-Pattern** - Ungewöhnliche Werte

---

**Status**: Geplant | Production-Ready IoT-System
