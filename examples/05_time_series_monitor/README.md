> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Zeitreihen-Monitor - Echtzeitdaten-Visualisierung

![Status](https://img.shields.io/badge/status-ready-brightgreen)
![Difficulty](https://img.shields.io/badge/difficulty-medium-orange)
![Duration](https://img.shields.io/badge/duration-30--40%20min-blue)

## 📝 Übersicht

Real-time Monitoring-System für Sensor-Daten mit Live-Diagrammen, Alarmen und historischer Analyse. Demonstriert Time-Series Features von ThemisDB.

## ✨ Features

- ✅ **Live-Datenerfassung** - CPU, Memory, Temperatur, Custom Sensors
- ✅ **Animierte Charts** - Echtzeit-Aktualisierung mit matplotlib
- ✅ **Alarm-System** - Schwellwert-basierte Benachrichtigungen
- ✅ **Aggregationen** - Min, Max, Durchschnitt über Zeiträume
- ✅ **Historische Analyse** - Daten-Replay und Trendanalyse
- ✅ **Multi-Sensor Support** - Mehrere Datenquellen gleichzeitig
- ✅ **Export** - Daten exportieren für externe Analyse

## 📊 Datenmodell

```python
{
    "id": "measurement_uuid",
    "sensor_id": "cpu_usage",
    "sensor_name": "CPU Auslastung",
    "value": 75.5,
    "unit": "%",
    "timestamp": "2025-12-22T10:30:45.123Z",
    "tags": {
        "host": "server-01",
        "location": "datacenter-1"
    }
}
```

## 🔧 Verwendung

```bash
cd examples/05_time_series_monitor
pip install -r requirements.txt
python main.py
```

Siehe [HOW_TO.md](HOW_TO.md) und [MONITORING_GUIDE.md](MONITORING_GUIDE.md) für Details.

## 📚 Was Sie lernen

- **Time-Series Model** - Effiziente Zeitreihenspeicherung
- **Continuous Aggregates** - Automatische Aggregationen
- **matplotlib Animation** - Live-Charts aktualisieren
- **Threading** - Parallele Datenerfassung
- **Alarm-Logik** - Schwellwert-Überwachung

---

**Status**: Geplant
