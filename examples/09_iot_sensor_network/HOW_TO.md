> **Aktueller Build-Flow:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# IoT-Sensornetzwerk - Bedienungsanleitung

> **Historischer Stand:** 2026-01-31 — Inhalte nicht gegen aktuelle Quellen geprüft.

## 🚀 Start

```bash
pip install themisdb-client
python main.py
```
<!-- TODO: verify against current source -->

## 📋 Hauptfunktionen

### Sensoren konfigurieren

**Sensor hinzufügen**:
1. Tab "Sensoren" → "Neuer Sensor"
2. Typ wählen (Temperatur, Feuchtigkeit, etc.)
3. Standort auf Karte setzen
4. Schwellwerte konfigurieren
5. Erfassungsintervall setzen
6. Speichern

**Sensor starten**:
1. Sensor auswählen
2. "Starten" → Simulation beginnt
3. Daten werden gesammelt

### Dashboard

**Live-Ansicht**:
- Karte mit allen Sensoren
- Farbcodierung nach Status
- Live-Werte bei Sensor-Hover
- Aktuelle Alarme

**Charts**:
- Multi-Sensor-Zeitreihen
- Aggregierte Ansichten
- Zoom und Pan
- Export-Funktion

### CEP-Regeln

**Regel erstellen**:
1. Tab "CEP" → "Neue Regel"
2. Pattern wählen:
   - Schwellwert
   - Trend
   - Korrelation
   - Anomalie
3. Parameter setzen
4. Aktion definieren (Email, SMS, Webhook)
5. Aktivieren

**Pattern-Beispiele**:

**Schwellwert**:
```
IF sensor.value > 50 FOR 300 seconds
THEN trigger alert
```

**Trend**:
```
IF sensor.value INCREASING FOR 600 seconds
AND slope > 0.5
THEN trigger warning
```

**Korrelation**:
```
IF sensor1.value > 40 AND sensor2.value > 40
WITHIN 60 seconds
THEN trigger critical
```

### Anomalie-Erkennung

**ML-Modell trainieren**:
1. Tab "ML" → "Training"
2. Historische Daten wählen (min. 1000 Samples)
3. Algorithmus wählen (Isolation Forest, LOF, OCSVM)
4. "Training starten"
5. Modell-Performance prüfen

**Anomalien anzeigen**:
- Automatisch erkannte Anomalien
- Anomalie-Score
- Visualisierung in Charts
- Historie aller Anomalien

### Geografische Ansicht

**Karte nutzen**:
- Sensoren als Marker
- Farbe = Status (Grün/Gelb/Rot)
- Größe = Datenrate
- Click für Details

**Geo-Queries**:
- "Sensoren in 5km Radius"
- "Hotspots finden"
- "Regionale Statistiken"

### Alarmierung

**Alarm-Stufen**:
- **Info** (Blau): Informativ
- **Warning** (Gelb): Achtung erforderlich
- **Critical** (Rot): Sofortiges Handeln

**Eskalation**:
1. Warning → 5 Min → Email
2. Critical → Sofort → SMS + Email
3. Nicht bestätigt → 15 Min → Telefon

## ⌨️ Tastenkombinationen

- `F5` - Dashboard aktualisieren
- `Ctrl+S` - Sensor hinzufügen
- `Ctrl+R` - CEP-Regel erstellen
- `Ctrl+M` - Karte fokussieren
- `Space` - Pause/Resume Simulation

## 💡 Best Practices

1. **Realistische Schwellwerte**: Basierend auf historischen Daten
2. **CEP-Regeln testen**: Vor Produktiv-Einsatz
3. **ML-Modell regelmäßig trainieren**: Bei sich ändernden Bedingungen
4. **Alarms richtig priorisieren**: Alarm-Fatigue vermeiden
5. **Daten-Retention**: Alte Daten archivieren

## 🔧 Simulation

**Sensoren simulieren**:
- Realistische Werte mit Rauschen
- Tageszeit-Schwankungen
- Anomalie-Injection für Tests
- Verschiedene Sensor-Typen

**Szenarien**:
- Normal-Betrieb
- Graduelle Verschlechterung
- Plötzlicher Ausfall
- Extreme Werte

---

**Weitere Guides**: SENSOR_SIMULATION.md, CEP_PATTERNS.md, ML_MODELS.md
