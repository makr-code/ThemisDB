> **Hinweis:** Troubleshooting-Schritte gegen aktuellen Build/Test-Flow verifizieren.

# Zeitreihen-Monitor - Troubleshooting Guide

## Häufige Probleme und Lösungen

### Verbindungsprobleme

#### Problem: "Keine Verbindung zu ThemisDB"

**Symptome**:
```
ConnectionError: Could not connect to ThemisDB at localhost:8080
```

**Ursachen und Lösungen**:

1. **ThemisDB Server läuft nicht**
   ```bash
   # Prüfen
   docker ps | grep themisdb
   
   # Starten
   docker start themisdb
   # oder
   docker run -d -p 8080:8080 -p 18765:18765 themisdb/themisdb:latest
   ```

2. **Falscher Port**
   ```python
   # In config prüfen
   client = TimeSeriesClient(
       host="localhost",
       port=8080  # Standard-Port
   )
   ```

3. **Firewall blockiert**
   ```bash
   # Port freigeben (Linux)
   sudo ufw allow 8080/tcp
   
   # Testen
   curl http://localhost:8080/health
   ```

4. **Docker-Netzwerk-Probleme**
   ```bash
   # Netzwerk prüfen
   docker network inspect bridge
   
   # Mit Host-Netzwerk starten
   docker run --network host themisdb/themisdb:latest
   ```

#### Problem: "Connection timeout"

**Symptome**:
```
TimeoutError: Request timed out after 10 seconds
```

**Lösungen**:

1. **Timeout erhöhen**
   ```python
   client = TimeSeriesClient(
       host="localhost",
       port=8080,
       timeout=30  # Von 10 auf 30 Sekunden
   )
   ```

2. **Server-Last prüfen**
   ```bash
   # CPU/Memory Check
   docker stats themisdb
   ```

3. **Netzwerk-Latenz prüfen**
   ```bash
   ping localhost
   curl -w "@curl-format.txt" -o /dev/null -s http://localhost:8080/health
   ```

### Datenerfassungs-Probleme

#### Problem: Keine Messwerte werden gespeichert

**Diagnose-Schritte**:

1. **Logging aktivieren**
   ```python
   import logging
   logging.basicConfig(level=logging.DEBUG)
   ```

2. **Sensor-Status prüfen**
   ```python
   sensor = client.get_sensor("cpu-1")
   print(f"Sensor aktiv: {sensor.active}")
   print(f"Letzter Wert: {sensor.last_value}")
   print(f"Letzte Aktualisierung: {sensor.last_updated}")
   ```

3. **Measurements Query**
   ```python
   measurements = client.get_measurements(
       sensor_id="cpu-1",
       limit=10
   )
   print(f"Anzahl Messungen: {len(measurements)}")
   ```

**Häufige Ursachen**:

1. **Sensor inaktiv**
   ```python
   sensor.active = True
   client.update_sensor(sensor)
   ```

2. **Falscher Sensor-Typ**
   ```python
   # Simulation muss zum Typ passen
   if sensor.type == SensorType.CPU:
       value = simulate_cpu()  # Nicht simulate_temperature()
   ```

3. **Validierungs-Fehler**
   ```python
   # Wert außerhalb gültiger Grenzen
   if value < 0 or value > 100:
       logger.warning(f"Invalid value: {value}")
       value = max(0, min(100, value))
   ```

#### Problem: Messwerte sind unplausibel

**Beispiel**: CPU bei 500% oder Temperatur bei -273°C

**Lösungen**:

1. **Bounds Checking**
   ```python
   def validate_measurement(sensor_type, value):
       bounds = {
           SensorType.CPU: (0, 100),
           SensorType.MEMORY: (0, 100),
           SensorType.TEMPERATURE: (-50, 150)
       }
       min_val, max_val = bounds.get(sensor_type, (None, None))
       if min_val is not None and value < min_val:
           return min_val
       if max_val is not None and value > max_val:
           return max_val
       return value
   ```

2. **Outlier Detection**
   ```python
   def is_outlier(value, history):
       if len(history) < 10:
           return False
       mean = sum(history) / len(history)
       std = (sum((x-mean)**2 for x in history) / len(history)) ** 0.5
       z_score = abs((value - mean) / std) if std > 0 else 0
       return z_score > 3
   ```

3. **Rate-of-Change Limiting**
   ```python
   def smooth_value(new_value, last_value, max_change_percent=20):
       """Verhindert zu große Sprünge"""
       if last_value is None:
           return new_value
       
       max_change = last_value * (max_change_percent / 100)
       diff = abs(new_value - last_value)
       
       if diff > max_change:
           # Limitiere Änderung
           direction = 1 if new_value > last_value else -1
           return last_value + (max_change * direction)
       
       return new_value
   ```

### Visualisierungs-Probleme

#### Problem: "matplotlib not found"

**Symptome**:
```
ModuleNotFoundError: No module named 'matplotlib'
```

**Lösung**:
```bash
pip install matplotlib

# Oder mit requirements.txt
pip install -r requirements.txt
```

**Fallback ohne matplotlib**:
```python
try:
    import matplotlib.pyplot as plt
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("matplotlib nicht verfügbar - Textmodus aktiv")

if HAS_MATPLOTLIB:
    # Grafische Darstellung
    plot_chart(data)
else:
    # Text-basierte Darstellung
    print_ascii_chart(data)
```

#### Problem: Charts werden nicht aktualisiert

**Symptome**: UI zeigt alte oder eingefrorene Daten

**Lösungen**:

1. **Threading prüfen**
   ```python
   # Prüfe ob Thread läuft
   if not self.monitoring_thread or not self.monitoring_thread.is_alive():
       logger.error("Monitoring thread not running!")
       self.start_monitoring()
   ```

2. **UI-Updates im Main Thread**
   ```python
   # Falsch (Threading-Problem):
   def update_chart():
       plt.plot(data)
   
   # Richtig:
   def update_chart():
       self.root.after(0, lambda: self._update_chart_safe(data))
   ```

3. **Animation-Interval**
   ```python
   # Zu hohe Update-Rate
   ani = FuncAnimation(fig, update, interval=100)  # 100ms könnte zu schnell sein
   
   # Besser:
   ani = FuncAnimation(fig, update, interval=1000)  # 1 Sekunde
   ```

#### Problem: Speicherleck bei langen Laufzeiten

**Symptome**: RAM-Verbrauch steigt kontinuierlich

**Diagnose**:
```python
import sys
print(f"Liste Größe: {len(data)} Einträge")
print(f"Speicher: {sys.getsizeof(data) / 1024:.2f} KB")
```

**Lösung - Circular Buffer**:
```python
from collections import deque

class SensorDataManager:
    def __init__(self, max_points=1000):
        self.data = deque(maxlen=max_points)  # Auto-limit
    
    def add_measurement(self, value):
        self.data.append(value)
        # Älteste Werte werden automatisch entfernt
```

### Performance-Probleme

#### Problem: Hohe CPU-Last

**Diagnose**:
```bash
# CPU-Profiling
python -m cProfile -o profile.stats main.py

# Analysieren
python -m pstats profile.stats
>>> sort time
>>> stats 10
```

**Häufige Ursachen und Fixes**:

1. **Zu hohe Sampling-Rate**
   ```python
   # Vorher: 100ms Interval
   interval = 0.1
   
   # Nachher: 1s Interval
   interval = 1.0
   ```

2. **Ineffiziente Queries**
   ```python
   # Schlecht: Lädt alle Daten
   all_measurements = client.get_all_measurements(sensor_id)
   
   # Gut: Limitiert auf benötigte Anzahl
   recent = client.get_measurements(sensor_id, limit=100)
   ```

3. **Zu viele gleichzeitige Sensoren**
   ```python
   # Staggered Updates
   for i, sensor in enumerate(sensors):
       time.sleep(i * 0.1)  # 100ms Versatz
       update_sensor(sensor)
   ```

#### Problem: UI friert ein

**Ursache**: Lange Operationen im Main Thread

**Lösung - Hintergrund-Thread**:
```python
import threading

def long_running_operation():
    # Zeitaufwendige Operation
    data = fetch_large_dataset()
    process_data(data)

# Im Main Thread
thread = threading.Thread(target=long_running_operation, daemon=True)
thread.start()

# UI bleibt responsive
```

**Queue für Thread-Kommunikation**:
```python
from queue import Queue

class MonitorApp:
    def __init__(self):
        self.data_queue = Queue()
        self.start_background_thread()
    
    def start_background_thread(self):
        thread = threading.Thread(target=self._collect_data, daemon=True)
        thread.start()
    
    def _collect_data(self):
        while self.running:
            data = sensor.read()
            self.data_queue.put(data)
            time.sleep(1)
    
    def update_ui(self):
        """Wird im Main Thread aufgerufen"""
        while not self.data_queue.empty():
            data = self.data_queue.get()
            self.display_data(data)
        
        # Nächstes Update planen
        self.root.after(100, self.update_ui)
```

### Alarm-Probleme

#### Problem: Zu viele False Positives

**Symptom**: Ständige Alarme obwohl alles normal ist

**Lösungen**:

1. **Schwellwerte anpassen**
   ```python
   # Zu sensitiv
   sensor.warning_threshold = 60.0
   
   # Realistischer
   sensor.warning_threshold = 75.0
   ```

2. **Hysteresis einbauen**
   ```python
   class HysteresisChecker:
       def __init__(self, threshold, hysteresis=5):
           self.threshold = threshold
           self.hysteresis = hysteresis
           self.alarmed = False
       
       def check(self, value):
           if not self.alarmed and value > self.threshold:
               self.alarmed = True
               return True
           elif self.alarmed and value < (self.threshold - self.hysteresis):
               self.alarmed = False
           return False
   ```

3. **Sustained Condition**
   ```python
   def check_sustained_alert(values, threshold, duration=3):
       """Alarm nur wenn Schwellwert X-mal hintereinander überschritten"""
       if len(values) < duration:
           return False
       return all(v > threshold for v in values[-duration:])
   ```

#### Problem: Alarme kommen nicht an

**Prüfung**:
```python
def test_alert_system():
    alert = Alert(
        sensor_id="test",
        severity=AlertSeverity.CRITICAL,
        message="Test Alert"
    )
    
    success = alert_manager.send_alert(alert)
    print(f"Alert sent: {success}")
```

**Logging aktivieren**:
```python
import logging
logging.getLogger('alert_system').setLevel(logging.DEBUG)
```

### Daten-Inkonsistenzen

#### Problem: Lücken in Zeitreihen

**Diagnose**:
```python
def find_gaps(measurements, expected_interval_seconds=10):
    """Findet Lücken in Messungen"""
    gaps = []
    
    for i in range(len(measurements) - 1):
        current = measurements[i].timestamp
        next_m = measurements[i+1].timestamp
        gap = (next_m - current).total_seconds()
        
        if gap > expected_interval_seconds * 2:
            gaps.append({
                'start': current,
                'end': next_m,
                'duration': gap
            })
    
    return gaps

gaps = find_gaps(measurements)
print(f"Gefunden: {len(gaps)} Lücken")
for gap in gaps:
    print(f"  {gap['start']} - {gap['end']}: {gap['duration']}s")
```

**Interpolation für Lücken**:
```python
def interpolate_gaps(measurements):
    """Füllt Lücken durch Interpolation"""
    filled = []
    
    for i in range(len(measurements) - 1):
        filled.append(measurements[i])
        
        current = measurements[i]
        next_m = measurements[i+1]
        gap = (next_m.timestamp - current.timestamp).total_seconds()
        
        if gap > 20:  # Mehr als 2x das Normal-Interval
            # Interpolierte Werte einfügen
            interpolated_value = (current.value + next_m.value) / 2
            interpolated_time = current.timestamp + timedelta(seconds=gap/2)
            
            filled.append(Measurement(
                sensor_id=current.sensor_id,
                value=interpolated_value,
                timestamp=interpolated_time
            ))
    
    filled.append(measurements[-1])
    return filled
```

### System-Ressourcen

#### Problem: Disk Full

**Früherkennung**:
```python
import shutil

def check_disk_space(path="/", threshold_percent=90):
    """Prüft verfügbaren Speicherplatz"""
    stat = shutil.disk_usage(path)
    used_percent = (stat.used / stat.total) * 100
    
    if used_percent > threshold_percent:
        logger.critical(f"Disk usage at {used_percent:.1f}%!")
        return False
    return True
```

**Automatische Cleanup-Strategie**:
```python
def cleanup_old_data(days=30):
    """Löscht Daten älter als X Tage"""
    cutoff = datetime.now() - timedelta(days=days)
    
    deleted_count = client.delete_measurements(before=cutoff)
    logger.info(f"Deleted {deleted_count} old measurements")
    
    return deleted_count
```

#### Problem: Out of Memory

**Diagnose**:
```python
import psutil

def check_memory():
    memory = psutil.virtual_memory()
    print(f"Total: {memory.total / 1024**3:.2f} GB")
    print(f"Used: {memory.used / 1024**3:.2f} GB")
    print(f"Percent: {memory.percent}%")
```

**Memory-Optimierung**:
```python
# 1. Generators statt Listen
def get_measurements_generator(sensor_id):
    """Lazy Loading"""
    offset = 0
    batch_size = 1000
    
    while True:
        batch = client.get_measurements(
            sensor_id,
            offset=offset,
            limit=batch_size
        )
        
        if not batch:
            break
        
        for measurement in batch:
            yield measurement
        
        offset += batch_size

# 2. Daten-Aggregation
def aggregate_old_data():
    """Reduziert Datenvolumen durch Aggregation"""
    old_data = get_old_measurements()
    
    # Stündliche Durchschnitte statt Einzelwerte
    aggregated = {}
    for m in old_data:
        hour_key = m.timestamp.replace(minute=0, second=0)
        if hour_key not in aggregated:
            aggregated[hour_key] = []
        aggregated[hour_key].append(m.value)
    
    return {k: sum(v)/len(v) for k, v in aggregated.items()}
```

## Debug-Tipps

### Umfassendes Logging

```python
import logging
from datetime import datetime

# Logging-Setup
logging.basicConfig(
    level=logging.DEBUG,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler(f'monitor_{datetime.now():%Y%m%d}.log'),
        logging.StreamHandler()
    ]
)

logger = logging.getLogger(__name__)

# In Code verwenden
logger.debug("Starting data collection")
logger.info(f"Collected {count} measurements")
logger.warning(f"Sensor {id} above threshold")
logger.error(f"Failed to save: {error}")
logger.critical("Database connection lost!")
```

### Debug-Modus

```python
class MonitorApp:
    def __init__(self, debug=False):
        self.debug = debug
        
        if self.debug:
            # Aktiviere ausführliches Logging
            logging.getLogger().setLevel(logging.DEBUG)
            
            # Zeige alle Exceptions
            self.root.report_callback_exception = self._show_error
            
            # Kleinere Intervals für schnelleres Testing
            self.update_interval = 100  # ms
    
    def _show_error(self, exc_type, exc_value, exc_traceback):
        import traceback
        error = ''.join(traceback.format_exception(
            exc_type, exc_value, exc_traceback
        ))
        logger.error(f"GUI Error:\n{error}")
        messagebox.showerror("Error", str(exc_value))
```

### Health Check Endpoint

```python
def health_check():
    """Umfassender System-Health-Check"""
    status = {
        "timestamp": datetime.now().isoformat(),
        "database": "unknown",
        "sensors": {},
        "memory_mb": 0,
        "cpu_percent": 0
    }
    
    # Database
    try:
        client.ping()
        status["database"] = "ok"
    except:
        status["database"] = "error"
    
    # Sensors
    for sensor in sensors:
        age = (datetime.now() - sensor.last_updated).total_seconds()
        status["sensors"][sensor.id] = {
            "active": sensor.active,
            "data_age_seconds": age,
            "status": "ok" if age < 60 else "stale"
        }
    
    # Resources
    import psutil
    process = psutil.Process()
    status["memory_mb"] = process.memory_info().rss / 1024**2
    status["cpu_percent"] = process.cpu_percent()
    
    return status
```

## Support-Checkliste

Wenn Sie Hilfe benötigen, sammeln Sie diese Informationen:

### System-Info
- [ ] Betriebssystem und Version
- [ ] Python-Version: `python --version`
- [ ] ThemisDB-Version
- [ ] Installierte Pakete: `pip freeze > requirements_actual.txt`

### Logs
- [ ] Application Logs
- [ ] ThemisDB Logs: `docker logs themisdb`
- [ ] System Logs (bei Crashes)

### Reproduktion
- [ ] Schritte zur Reproduktion
- [ ] Erwartetes Verhalten
- [ ] Tatsächliches Verhalten
- [ ] Screenshots/Videos (bei UI-Problemen)

### Performance-Daten
- [ ] CPU-Usage während Problem
- [ ] Memory-Usage
- [ ] Disk I/O
- [ ] Network-Latency zu ThemisDB

---

**Bei weiteren Problemen**: GitHub Issues oder Community-Forum

**Letzte Aktualisierung**: 2025-12-22
