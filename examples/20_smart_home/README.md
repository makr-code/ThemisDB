> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Smart Home Dashboard - IoT Automation mit ThemisDB

![Status](https://img.shields.io/badge/status-ready-brightgreen)
![Difficulty](https://img.shields.io/badge/difficulty-complex-red)
![Duration](https://img.shields.io/badge/duration-90--120%20min-blue)

## 📝 Übersicht

Das Smart Home Dashboard zeigt IoT-Automatisierung mit ThemisDB. Sie lernen:
- IoT Device Management
- Automation Rules (CEP)
- Energy Monitoring
- Weather Integration
- Scene Management
- Voice Control Integration

## ✨ Features

- ✅ **Device Management** - Alle IoT-Geräte
- ✅ **Automation Rules** - Wenn-Dann-Regeln
- ✅ **Energy Monitoring** - Verbrauch tracking
- ✅ **Weather Integration** - Externe Daten
- ✅ **Scenes** - Vordefinierte Zustände
- ✅ **Voice Control** - Alexa/Google Integration
- ✅ **Scheduling** - Zeitgesteuerte Aktionen
- ✅ **Notifications** - Alerts und Warnings

## 📊 Datenmodell

### Device

```json
{
  "id": "device_uuid",
  "name": "Living Room Light",
  "type": "light",
  "location": "living_room",
  "manufacturer": "Philips Hue",
  "status": "online",
  "state": {
    "power": "on",
    "brightness": 80,
    "color": "#FFFFFF"
  },
  "capabilities": ["dimming", "color"],
  "energy_usage": 12.5
}
```

### Automation Rule

```json
{
  "id": "rule_uuid",
  "name": "Morning Routine",
  "trigger": {
    "type": "time",
    "value": "07:00"
  },
  "conditions": [
    {"device": "motion_sensor", "state": "active"}
  ],
  "actions": [
    {"device": "lights", "action": "turn_on", "brightness": 50},
    {"device": "thermostat", "action": "set_temp", "value": 22}
  ],
  "enabled": true
}
```

### Scene

```json
{
  "id": "scene_uuid",
  "name": "Movie Night",
  "devices": [
    {"id": "tv", "state": {"power": "on"}},
    {"id": "lights", "state": {"brightness": 10}},
    {"id": "blinds", "state": {"position": "closed"}}
  ]
}
```

## 🛠️ ThemisDB Features

- **Time-Series** für Sensor Data
- **CEP** für Automation Rules
- **Graph** für Device Dependencies
- **Real-Time** für Status Updates

## 🔗 Navigation

- ⬅️ [19 - Recommendation Engine](../19_recommendation_engine/)
- 🏠 [Übersicht](../README.md)

---

**Status**: Ready | **Letzte Aktualisierung**: 2025-12-22
