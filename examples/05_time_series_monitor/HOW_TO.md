> **Aktueller Build-Flow:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Zeitreihen-Monitor - Bedienungsanleitung

## 🚀 Start

```bash
python main.py
```

## 📋 Hauptfunktionen

### Sensoren konfigurieren

1. Tab "Sensoren" → "Sensor hinzufügen"
2. Name, Typ, Einheit eingeben
3. Schwellwerte für Alarme setzen
4. Erfassungsintervall wählen
5. Speichern

### Live-Monitoring

**Dashboard**:
- Zeigt alle aktiven Sensoren
- Live-aktualisierte Charts
- Aktuelle Werte und Trends
- Alarm-Status

**Chart-Optionen**:
- Zeitfenster: 1min, 5min, 1h, 24h
- Aggregation: Roh, Durchschnitt, Min/Max
- Multi-Sensor-Ansicht

### Alarme verwalten

**Alarm erstellen**:
1. Sensor auswählen
2. "Alarm hinzufügen"
3. Bedingung: >, <, =, ≠
4. Schwellwert eingeben
5. Aktion wählen (Benachrichtigung, Email)

**Alarm-Historie**:
- Zeigt alle ausgelösten Alarme
- Filter nach Sensor, Zeitraum
- Export für Analyse

### Historische Daten

**Daten abrufen**:
1. Tab "Historie"
2. Zeitraum wählen
3. Sensoren auswählen
4. "Laden"

**Analyse**:
- Statistiken: Min, Max, Durchschnitt, Median
- Trendlinien
- Korrelationen zwischen Sensoren

## ⌨️ Tastenkombinationen

- `F5` - Aktualisieren
- `Space` - Pause/Resume
- `Ctrl+E` - Export
- `Ctrl+A` - Neuer Alarm

---

**Weitere Details**: Siehe MONITORING_GUIDE.md
