> **Aktueller Build-Flow:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Zeitreihen-Monitor - Monitoring Best Practices

## Übersicht

Dieser Guide beschreibt Best Practices für effektives Monitoring mit dem Zeitreihen-Monitor und ThemisDB.

## Monitoring-Grundlagen

### Die 4 Golden Signals

1. **Latenz**: Antwortzeiten
2. **Traffic**: Anfragerate
3. **Errors**: Fehlerrate
4. **Saturation**: Auslastung

### Monitoring-Pyramide

```
┌────────────────────┐
│  Business KPIs     │  ← Geschäftsziele
├────────────────────┤
│  Application       │  ← Service-Metriken
├────────────────────┤
│  Infrastructure    │  ← System-Metriken
└────────────────────┘
```

## Sensor-Konfiguration

### Sensor-Typen richtig wählen

**CPU-Sensor**:
```python
sensor = Sensor(
    name="app-server-cpu",
    type=SensorType.CPU,
    unit="%",
    warning_threshold=70.0,
    critical_threshold=90.0,
    check_interval=10  # Sekunden
)
```

**Memory-Sensor**:
```python
sensor = Sensor(
    name="app-server-memory",
    type=SensorType.MEMORY,
    unit="MB",
    warning_threshold=8192,  # 8 GB
    critical_threshold=12288,  # 12 GB
    check_interval=10
)
```

**Temperature-Sensor**:
```python
sensor = Sensor(
    name="server-room-temp",
    type=SensorType.TEMPERATURE,
    unit="°C",
    warning_threshold=25.0,
    critical_threshold=30.0,
    check_interval=30
)
```

### Schwellwerte festlegen

**Regel-basierte Schwellwerte**:
```python
# Static Thresholds
warning = baseline * 0.7
critical = baseline * 0.9

# Dynamic Thresholds (basierend auf Historie)
from statistics import mean, stdev

historical_values = get_last_n_hours(sensor_id, hours=24)
baseline = mean(historical_values)
deviation = stdev(historical_values)

warning = baseline + (2 * deviation)
critical = baseline + (3 * deviation)
```

## Datensammlung

### Sampling-Strategien

**Feste Intervalle** (empfohlen für die meisten Fälle):
```python
INTERVALS = {
    "high_frequency": 1,    # 1 Sekunde - für kritische Metriken
    "normal": 10,           # 10 Sekunden - Standard
    "low_frequency": 60     # 1 Minute - für stabile Metriken
}
```

**Adaptive Sampling** (bei Anomalien):
```python
def get_sample_interval(sensor):
    if sensor.status == "CRITICAL":
        return 1  # Erhöhte Frequenz
    elif sensor.status == "WARNING":
        return 5
    else:
        return sensor.default_interval
```

### Datenqualität sichern

**Outlier Detection**:
```python
def is_outlier(value, history, threshold=3):
    """Z-Score Methode"""
    if len(history) < 10:
        return False
    
    mean_val = sum(history) / len(history)
    std_val = (sum((x - mean_val) ** 2 for x in history) / len(history)) ** 0.5
    
    if std_val == 0:
        return False
    
    z_score = abs((value - mean_val) / std_val)
    return z_score > threshold
```

**Missing Data Handling**:
```python
def handle_missing_data(sensor_id, expected_count, actual_count):
    if actual_count < expected_count * 0.8:
        # Weniger als 80% der erwarteten Daten
        logger.warning(f"Missing data for {sensor_id}")
        send_alert(f"Sensor {sensor_id} may be offline")
```

## Visualisierung

### Chart-Konfiguration

**Zeitfenster wählen**:
```python
TIME_WINDOWS = {
    "realtime": 60,          # Letzte 60 Sekunden
    "short": 300,            # 5 Minuten
    "medium": 3600,          # 1 Stunde
    "long": 86400,           # 24 Stunden
    "week": 604800           # 7 Tage
}
```

**Y-Achsen-Skalierung**:
```python
def calculate_y_limits(data, padding=0.1):
    """Dynamische Y-Achsen-Grenzen"""
    if not data:
        return (0, 100)
    
    min_val = min(data)
    max_val = max(data)
    range_val = max_val - min_val
    
    padding_val = range_val * padding
    
    return (
        max(0, min_val - padding_val),
        max_val + padding_val
    )
```

**Farben für Status**:
```python
STATUS_COLORS = {
    "OK": "#28a745",        # Grün
    "WARNING": "#ffc107",   # Gelb
    "CRITICAL": "#dc3545"   # Rot
}
```

### Performance-Optimierung

**Datenpunkte reduzieren** (für große Zeitfenster):
```python
def downsample(data, target_points=100):
    """Reduziert Datenpunkte durch Aggregation"""
    if len(data) <= target_points:
        return data
    
    bucket_size = len(data) // target_points
    downsampled = []
    
    for i in range(0, len(data), bucket_size):
        bucket = data[i:i+bucket_size]
        # Durchschnitt oder Max/Min je nach Anforderung
        downsampled.append(sum(bucket) / len(bucket))
    
    return downsampled
```

## Alarme und Benachrichtigungen

### Alarm-Strategie

**Alert Levels**:
```python
class AlertSeverity(Enum):
    INFO = "info"           # Informational
    WARNING = "warning"     # Benötigt Aufmerksamkeit
    CRITICAL = "critical"   # Sofortige Aktion nötig
    EMERGENCY = "emergency" # System down
```

**Alert Rules**:
```python
def evaluate_alert(sensor, measurement):
    """Bestimmt Alert-Level"""
    value = measurement.value
    
    if value >= sensor.critical_threshold:
        return AlertSeverity.CRITICAL
    elif value >= sensor.warning_threshold:
        return AlertSeverity.WARNING
    elif value >= sensor.warning_threshold * 0.8:
        return AlertSeverity.INFO
    else:
        return None
```

### Alert Fatigue vermeiden

**Hysteresis** (Schwingungen vermeiden):
```python
class HysteresisAlert:
    def __init__(self, threshold, hysteresis_percent=10):
        self.threshold = threshold
        self.hysteresis = threshold * (hysteresis_percent / 100)
        self.state = "OK"
    
    def evaluate(self, value):
        if self.state == "OK" and value > self.threshold:
            self.state = "ALERT"
            return True
        elif self.state == "ALERT" and value < (self.threshold - self.hysteresis):
            self.state = "OK"
            return False
        return self.state == "ALERT"
```

**Rate Limiting**:
```python
from datetime import datetime, timedelta

class RateLimitedAlert:
    def __init__(self, cooldown_minutes=5):
        self.cooldown = timedelta(minutes=cooldown_minutes)
        self.last_alert = {}
    
    def should_alert(self, sensor_id):
        now = datetime.now()
        last = self.last_alert.get(sensor_id)
        
        if last is None or (now - last) > self.cooldown:
            self.last_alert[sensor_id] = now
            return True
        return False
```

**Aggregation** (mehrere Alerts zusammenfassen):
```python
def aggregate_alerts(alerts, window_seconds=60):
    """Fasst Alerts in Zeitfenster zusammen"""
    now = datetime.now()
    recent = [a for a in alerts if (now - a.timestamp).seconds < window_seconds]
    
    if len(recent) >= 3:
        # 3+ Alerts im Fenster → ein aggregierter Alert
        return Alert(
            sensor_id="multiple",
            message=f"{len(recent)} alerts in last {window_seconds}s",
            severity=max(a.severity for a in recent)
        )
    return None
```

## Datenaufbewahrung

### Retention-Policy

**Mehrstufige Retention**:
```python
RETENTION_POLICY = {
    "raw": {
        "duration": timedelta(days=7),
        "interval": 1  # Sekunden
    },
    "minutely": {
        "duration": timedelta(days=30),
        "interval": 60
    },
    "hourly": {
        "duration": timedelta(days=365),
        "interval": 3600
    },
    "daily": {
        "duration": timedelta(days=3650),  # 10 Jahre
        "interval": 86400
    }
}
```

**Automatische Aggregation**:
```python
def aggregate_to_minutely():
    """Aggregiert Sekunden-Daten zu Minuten"""
    cutoff = datetime.now() - timedelta(days=7)
    
    # Hole alte Raw-Daten
    raw_data = query_raw_data(before=cutoff)
    
    # Gruppiere nach Minute
    by_minute = {}
    for measurement in raw_data:
        minute_key = measurement.timestamp.replace(second=0, microsecond=0)
        if minute_key not in by_minute:
            by_minute[minute_key] = []
        by_minute[minute_key].append(measurement.value)
    
    # Erstelle aggregierte Einträge
    for minute, values in by_minute.items():
        save_aggregated_measurement({
            "timestamp": minute,
            "avg": sum(values) / len(values),
            "min": min(values),
            "max": max(values),
            "count": len(values)
        })
    
    # Lösche alte Raw-Daten
    delete_raw_data(before=cutoff)
```

## Performance-Tuning

### Database-Optimierung

**Batch Inserts**:
```python
def batch_insert_measurements(measurements, batch_size=1000):
    """Insertet Messungen in Batches"""
    for i in range(0, len(measurements), batch_size):
        batch = measurements[i:i+batch_size]
        client.bulk_create_measurements(batch)
```

**Indexes**:
```sql
-- ThemisDB Indizes für Time-Series
CREATE INDEX idx_measurements_sensor_time 
ON measurements(sensor_id, timestamp DESC);

CREATE INDEX idx_measurements_time 
ON measurements(timestamp DESC);
```

**Partitionierung**:
```sql
-- Monatliche Partitionen
CREATE TABLE measurements (
    ...
) PARTITION BY RANGE (timestamp) 
INTERVAL '1 month';
```

### Client-seitige Performance

**Connection Pooling**:
```python
from queue import Queue
import threading

class ClientPool:
    def __init__(self, size=5):
        self.pool = Queue(maxsize=size)
        for _ in range(size):
            self.pool.put(TimeSeriesClient())
    
    def get_client(self):
        return self.pool.get()
    
    def return_client(self, client):
        self.pool.put(client)
```

**Caching**:
```python
from functools import lru_cache
from datetime import datetime, timedelta

@lru_cache(maxsize=100)
def get_sensor_cached(sensor_id, cache_time):
    """Cached sensor lookup"""
    return client.get_sensor(sensor_id)

# Cache-Time alle 5 Minuten invalidieren
cache_time = datetime.now().replace(second=0, microsecond=0)
cache_time = cache_time.replace(minute=(cache_time.minute // 5) * 5)
sensor = get_sensor_cached("cpu-1", cache_time)
```

## Troubleshooting

### Häufige Probleme

**Problem: Hohe CPU-Last**
```python
# Lösung: Sampling-Intervall erhöhen
if cpu_usage > 80:
    for sensor in sensors:
        sensor.interval = max(sensor.interval * 2, 60)
```

**Problem: Speicher läuft voll**
```python
# Lösung: Zirkulärer Buffer
class CircularBuffer:
    def __init__(self, max_size=1000):
        self.buffer = []
        self.max_size = max_size
    
    def append(self, item):
        self.buffer.append(item)
        if len(self.buffer) > self.max_size:
            self.buffer.pop(0)
```

**Problem: Veraltete Daten**
```python
def check_data_freshness(sensor_id, max_age_seconds=60):
    """Prüft ob Daten aktuell sind"""
    latest = get_latest_measurement(sensor_id)
    if latest:
        age = (datetime.now() - latest.timestamp).total_seconds()
        if age > max_age_seconds:
            logger.warning(f"Stale data for {sensor_id}: {age}s old")
            return False
    return True
```

## Checkliste für Production

### Pre-Deployment

- [ ] Alle Sensoren konfiguriert und getestet
- [ ] Schwellwerte validiert
- [ ] Alerts eingerichtet und getestet
- [ ] Retention-Policy definiert
- [ ] Backup-Strategie implementiert
- [ ] Monitoring des Monitoring-Systems
- [ ] Dokumentation aktualisiert

### Post-Deployment

- [ ] Datenqualität überprüfen (erste 24h)
- [ ] Alert-Rate überprüfen (zu viele/wenige?)
- [ ] Performance-Metriken prüfen
- [ ] Speicherverbrauch monitoren
- [ ] Retention-Policy validieren
- [ ] Runbooks für häufige Probleme erstellen

## Best Practices Zusammenfassung

### ✅ DO

1. **Start simple**: Beginne mit wenigen kritischen Metriken
2. **Alert sparsam**: Nur für actionable items
3. **Dokumentiere Schwellwerte**: Warum diese Werte?
4. **Teste Alerts**: Simuliere Ausfälle
5. **Review regelmäßig**: Sind Schwellwerte noch aktuell?
6. **Aggregiere alte Daten**: Spare Speicherplatz
7. **Visualisiere**: Dashboards für schnellen Überblick
8. **Automatisiere**: Reaktionen auf bekannte Probleme

### ❌ DON'T

1. **Zu viele Metriken**: Focus on what matters
2. **Zu sensitive Alerts**: Alert fatigue vermeiden
3. **Rohdaten ewig speichern**: Retention-Policy nutzen
4. **Alerts ignorieren**: Wenn Alert, dann wichtig
5. **Keine Dokumentation**: Runbooks sind kritisch
6. **Blindes Kopieren**: Schwellwerte pro System anpassen
7. **Keine Tests**: Alert-System regelmäßig testen
8. **Vergessen zu monitoren**: Das Monitoring-System selbst!

## Weiterführende Ressourcen

- **Google SRE Book**: Monitoring Distributed Systems
- **Prometheus Best Practices**: Metric and label naming
- **The Art of Monitoring**: James Turnbull
- **Site Reliability Engineering**: Building Production Systems

---

**Letzte Aktualisierung**: 2025-12-22
