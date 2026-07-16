> **Hinweis:** Inhalt mit aktuellem Modulcode und -stand abgleichen.

# Sensor-Simulation - IoT Sensornetzwerk

## 📋 Übersicht

Dieses Dokument beschreibt die Sensor-Simulation für das IoT-System, einschließlich Sensor-Typen, Datenformaten, MQTT-Integration und Fehlerszenarien.

## 🌡️ Sensor-Typen

### 1. Temperatursensor

**Eigenschaften:**
- Messbereich: -40°C bis +85°C
- Genauigkeit: ±0.5°C
- Abtastrate: 1-60 Sekunden
- Energieverbrauch: Niedrig

**Simulation:**
```python
import random
import math
from datetime import datetime

class TemperatureSensor:
    """Simuliert einen Temperatursensor mit realistischem Verhalten."""
    
    def __init__(self, sensor_id: str, location: dict):
        self.sensor_id = sensor_id
        self.location = location
        self.base_temperature = 20.0  # Basistemperatur
        self.noise_level = 0.5  # Rausch-Amplitude
        self.drift_rate = 0.001  # Langzeit-Drift pro Messung
        self.accumulated_drift = 0.0
    
    def read(self) -> dict:
        """Liest einen Temperaturwert."""
        # Tageszeit-Effekt (sinusoidal)
        hour = datetime.now().hour
        time_effect = 5 * math.sin((hour - 6) * math.pi / 12)
        
        # Zufälliges Rauschen
        noise = random.gauss(0, self.noise_level)
        
        # Langzeit-Drift
        self.accumulated_drift += random.gauss(0, self.drift_rate)
        
        # Endwert
        temperature = (
            self.base_temperature +
            time_effect +
            noise +
            self.accumulated_drift
        )
        
        return {
            "sensor_id": self.sensor_id,
            "type": "temperature",
            "value": round(temperature, 2),
            "unit": "°C",
            "timestamp": datetime.now().isoformat(),
            "quality": self._calculate_quality(noise),
            "location": self.location
        }
    
    def _calculate_quality(self, noise: float) -> float:
        """Berechnet Datenqualität basierend auf Rauschen."""
        # Qualität sinkt bei höherem Rauschen
        quality = 1.0 - abs(noise) / (3 * self.noise_level)
        return max(0.0, min(1.0, quality))
    
    def inject_fault(self, fault_type: str):
        """Injiziert einen Fehler für Testing."""
        if fault_type == "spike":
            self.base_temperature += random.uniform(10, 30)
        elif fault_type == "drift":
            self.accumulated_drift += random.uniform(5, 15)
        elif fault_type == "noise":
            self.noise_level *= 5
```

### 2. Feuchtigkeitssensor

**Eigenschaften:**
- Messbereich: 0% bis 100% relative Luftfeuchtigkeit
- Genauigkeit: ±2%
- Abtastrate: 5-60 Sekunden

**Simulation:**
```python
class HumiditySensor:
    """Simuliert einen Feuchtigkeitssensor."""
    
    def __init__(self, sensor_id: str, location: dict):
        self.sensor_id = sensor_id
        self.location = location
        self.base_humidity = 50.0  # Basis 50% RH
    
    def read(self) -> dict:
        """Liest einen Feuchtigkeitswert."""
        # Korrelation mit Temperatur (inverser Zusammenhang)
        hour = datetime.now().hour
        time_effect = -3 * math.sin((hour - 6) * math.pi / 12)
        
        # Zufällige Variation
        variation = random.gauss(0, 2.0)
        
        # Wetter-Einfluss (simuliert)
        weather_effect = random.choice([-5, 0, 5, 10])  # Regen erhöht Feuchtigkeit
        
        humidity = self.base_humidity + time_effect + variation + weather_effect
        humidity = max(0, min(100, humidity))  # Begrenze auf 0-100%
        
        return {
            "sensor_id": self.sensor_id,
            "type": "humidity",
            "value": round(humidity, 1),
            "unit": "%RH",
            "timestamp": datetime.now().isoformat(),
            "quality": 0.95,
            "location": self.location
        }
```

### 3. Drucksensor

**Eigenschaften:**
- Messbereich: 300 hPa bis 1100 hPa
- Genauigkeit: ±0.5 hPa
- Verwendung: Wetter-Vorhersage, Höhenmessung

**Simulation:**
```python
class PressureSensor:
    """Simuliert einen Luftdrucksensor."""
    
    def __init__(self, sensor_id: str, location: dict):
        self.sensor_id = sensor_id
        self.location = location
        self.base_pressure = 1013.25  # Standarddruck auf Meereshöhe
        self.altitude = location.get("altitude", 0)
    
    def read(self) -> dict:
        """Liest einen Druckwert."""
        # Höhen-Korrektur (ca. -12 hPa pro 100m)
        altitude_effect = -0.12 * self.altitude
        
        # Wetter-Variation (Hoch-/Tiefdruck)
        weather_variation = random.gauss(0, 5)
        
        # Tageszeit-Variation (klein)
        time_variation = random.gauss(0, 0.5)
        
        pressure = (
            self.base_pressure +
            altitude_effect +
            weather_variation +
            time_variation
        )
        
        return {
            "sensor_id": self.sensor_id,
            "type": "pressure",
            "value": round(pressure, 2),
            "unit": "hPa",
            "timestamp": datetime.now().isoformat(),
            "quality": 0.98,
            "location": self.location
        }
```

### 4. Bewegungssensor (PIR)

**Eigenschaften:**
- Binär: Bewegung erkannt / nicht erkannt
- Erkennungsbereich: 5-12 Meter
- Erfassungswinkel: 110°

**Simulation:**
```python
class MotionSensor:
    """Simuliert einen Bewegungssensor (PIR)."""
    
    def __init__(self, sensor_id: str, location: dict):
        self.sensor_id = sensor_id
        self.location = location
        self.last_motion = None
        self.motion_probability = 0.05  # 5% Chance pro Check
    
    def read(self) -> dict:
        """Liest Bewegungsstatus."""
        # Simuliere Bewegungserkennung
        motion_detected = random.random() < self.motion_probability
        
        if motion_detected:
            self.last_motion = datetime.now()
        
        return {
            "sensor_id": self.sensor_id,
            "type": "motion",
            "value": 1 if motion_detected else 0,
            "unit": "boolean",
            "timestamp": datetime.now().isoformat(),
            "last_motion": self.last_motion.isoformat() if self.last_motion else None,
            "quality": 1.0,
            "location": self.location
        }
```

## 📊 Datenformat-Spezifikationen

### Standard-Messungs-Format

Alle Sensoren verwenden ein einheitliches Format:

```json
{
    "sensor_id": "sensor_uuid",
    "type": "temperature",
    "value": 23.5,
    "unit": "°C",
    "timestamp": "2025-12-22T10:30:45.123Z",
    "quality": 0.98,
    "location": {
        "lat": 52.5200,
        "lon": 13.4050,
        "altitude": 34
    },
    "metadata": {
        "battery_level": 85,
        "signal_strength": -65,
        "firmware_version": "2.1.0"
    }
}
```

### Batch-Format

Mehrere Messungen in einem Request:

```json
{
    "sensor_id": "sensor_uuid",
    "readings": [
        {
            "type": "temperature",
            "value": 23.5,
            "unit": "°C",
            "timestamp": "2025-12-22T10:30:45Z"
        },
        {
            "type": "temperature",
            "value": 23.6,
            "unit": "°C",
            "timestamp": "2025-12-22T10:31:45Z"
        }
    ]
}
```

## 🔌 MQTT-Integration

### Broker-Konfiguration

```python
import paho.mqtt.client as mqtt

class MQTTSensorPublisher:
    """MQTT Publisher für Sensor-Daten."""
    
    def __init__(
        self,
        broker_host: str = "localhost",
        broker_port: int = 1883,
        username: str = None,
        password: str = None
    ):
        self.client = mqtt.Client()
        
        if username and password:
            self.client.username_pw_set(username, password)
        
        self.client.on_connect = self._on_connect
        self.client.on_disconnect = self._on_disconnect
        self.client.on_publish = self._on_publish
        
        self.broker_host = broker_host
        self.broker_port = broker_port
        self.connected = False
    
    def connect(self):
        """Verbindet mit MQTT Broker."""
        try:
            self.client.connect(self.broker_host, self.broker_port, 60)
            self.client.loop_start()
        except Exception as e:
            print(f"Connection failed: {e}")
    
    def _on_connect(self, client, userdata, flags, rc):
        """Callback bei erfolgreicher Verbindung."""
        if rc == 0:
            self.connected = True
            print("Connected to MQTT broker")
        else:
            print(f"Connection failed with code {rc}")
    
    def _on_disconnect(self, client, userdata, rc):
        """Callback bei Verbindungsabbruch."""
        self.connected = False
        print("Disconnected from MQTT broker")
    
    def _on_publish(self, client, userdata, mid):
        """Callback nach erfolgreicher Publikation."""
        pass
    
    def publish_reading(self, sensor_id: str, reading: dict):
        """Publiziert eine Sensor-Messung."""
        topic = f"sensors/{sensor_id}/{reading['type']}"
        payload = json.dumps(reading)
        
        if self.connected:
            result = self.client.publish(topic, payload, qos=1)
            return result.rc == mqtt.MQTT_ERR_SUCCESS
        return False
```

### Topic-Hierarchie

```
sensors/
├── {sensor_id}/
│   ├── temperature
│   ├── humidity
│   ├── pressure
│   └── status
├── alerts/
│   ├── critical
│   ├── warning
│   └── info
└── system/
    ├── heartbeat
    └── diagnostics
```

### QoS-Levels

- **QoS 0** (At most once): Normale Messwerte
- **QoS 1** (At least once): Wichtige Messwerte, Alarme
- **QoS 2** (Exactly once): Kritische Alarme, Systemevents

### Beispiel-Subscriber

```python
class MQTTSensorSubscriber:
    """MQTT Subscriber für Sensor-Daten."""
    
    def __init__(self, broker_host: str, themis_client):
        self.client = mqtt.Client()
        self.client.on_connect = self._on_connect
        self.client.on_message = self._on_message
        self.broker_host = broker_host
        self.themis_client = themis_client
    
    def _on_connect(self, client, userdata, flags, rc):
        """Subscribe zu allen Sensor-Topics."""
        if rc == 0:
            self.client.subscribe("sensors/+/+", qos=1)
            self.client.subscribe("alerts/#", qos=1)
    
    def _on_message(self, client, userdata, msg):
        """Verarbeitet eingehende Nachrichten."""
        try:
            reading = json.loads(msg.payload)
            
            # Speichere in ThemisDB
            asyncio.run(
                self.themis_client.create("measurements", reading)
            )
            
            # Prüfe auf Anomalien
            self._check_for_anomalies(reading)
            
        except Exception as e:
            print(f"Error processing message: {e}")
    
    def _check_for_anomalies(self, reading: dict):
        """Prüft auf Anomalien und triggert Alarme."""
        if reading["type"] == "temperature":
            if reading["value"] > 50 or reading["value"] < -10:
                self._send_alert("critical", f"Temperature out of range: {reading['value']}")
```

## ⚠️ Fehlerszenarien

### 1. Sensor-Ausfall

```python
class SensorFailureSimulator:
    """Simuliert verschiedene Sensor-Ausfälle."""
    
    @staticmethod
    def simulate_complete_failure(sensor):
        """Sensor sendet keine Daten mehr."""
        raise ConnectionError("Sensor not responding")
    
    @staticmethod
    def simulate_intermittent_failure(sensor):
        """Sensor sendet sporadisch Daten."""
        if random.random() < 0.3:  # 30% Ausfallrate
            raise TimeoutError("Sensor timeout")
        return sensor.read()
    
    @staticmethod
    def simulate_stuck_value(sensor):
        """Sensor liefert konstanten Wert (defekt)."""
        stuck_value = sensor.read()
        # Überschreibe read-Methode
        sensor.read = lambda: stuck_value
        return stuck_value
    
    @staticmethod
    def simulate_drift(sensor):
        """Sensor driftet graduell ab."""
        original_read = sensor.read
        drift = 0
        
        def drifted_read():
            nonlocal drift
            drift += random.uniform(0.1, 0.5)
            reading = original_read()
            reading["value"] += drift
            return reading
        
        sensor.read = drifted_read
```

### 2. Netzwerk-Probleme

```python
class NetworkFailureSimulator:
    """Simuliert Netzwerk-Probleme."""
    
    @staticmethod
    def simulate_packet_loss(publisher, loss_rate: float = 0.1):
        """Simuliert Paketverlust."""
        original_publish = publisher.publish_reading
        
        def lossy_publish(sensor_id, reading):
            if random.random() > loss_rate:
                return original_publish(sensor_id, reading)
            return False  # Paket verloren
        
        publisher.publish_reading = lossy_publish
    
    @staticmethod
    def simulate_high_latency(publisher, delay_ms: int = 1000):
        """Simuliert hohe Latenz."""
        import time
        original_publish = publisher.publish_reading
        
        def delayed_publish(sensor_id, reading):
            time.sleep(delay_ms / 1000.0)
            return original_publish(sensor_id, reading)
        
        publisher.publish_reading = delayed_publish
    
    @staticmethod
    def simulate_connection_drops(publisher, drop_probability: float = 0.05):
        """Simuliert Verbindungsabbrüche."""
        original_publish = publisher.publish_reading
        
        def unstable_publish(sensor_id, reading):
            if random.random() < drop_probability:
                publisher.connected = False
                raise ConnectionError("Connection dropped")
            return original_publish(sensor_id, reading)
        
        publisher.publish_reading = unstable_publish
```

### 3. Datenqualitäts-Probleme

```python
class DataQualitySimulator:
    """Simuliert Datenqualitäts-Probleme."""
    
    @staticmethod
    def simulate_noise(sensor, noise_multiplier: float = 5.0):
        """Erhöht Rauschen."""
        if hasattr(sensor, 'noise_level'):
            sensor.noise_level *= noise_multiplier
    
    @staticmethod
    def simulate_outliers(reading: dict, probability: float = 0.05):
        """Injiziert gelegentliche Ausreißer."""
        if random.random() < probability:
            reading["value"] *= random.uniform(2, 10)
            reading["quality"] *= 0.1
        return reading
    
    @staticmethod
    def simulate_missing_data(reading: dict, probability: float = 0.1):
        """Simuliert fehlende Datenpunkte."""
        if random.random() < probability:
            return None
        return reading
```

## 🧪 Test-Szenarien

### Szenario 1: Normalbetrieb

```python
def test_normal_operation():
    """Testet Normalbetrieb aller Sensoren."""
    sensors = [
        TemperatureSensor("temp_01", {"lat": 52.52, "lon": 13.40, "altitude": 34}),
        HumiditySensor("hum_01", {"lat": 52.52, "lon": 13.40, "altitude": 34}),
        PressureSensor("press_01", {"lat": 52.52, "lon": 13.40, "altitude": 34}),
        MotionSensor("motion_01", {"lat": 52.52, "lon": 13.40, "altitude": 34})
    ]
    
    publisher = MQTTSensorPublisher()
    publisher.connect()
    
    for _ in range(100):  # 100 Messungen
        for sensor in sensors:
            reading = sensor.read()
            publisher.publish_reading(sensor.sensor_id, reading)
        time.sleep(1)
```

### Szenario 2: Sensor mit Drift

```python
def test_sensor_drift():
    """Testet Erkennung von Sensor-Drift."""
    sensor = TemperatureSensor("temp_drift", {"lat": 52.52, "lon": 13.40})
    
    # Injiziere Drift
    SensorFailureSimulator.simulate_drift(sensor)
    
    readings = []
    for _ in range(100):
        reading = sensor.read()
        readings.append(reading["value"])
        time.sleep(0.1)
    
    # Prüfe ob Drift erkannt wird
    drift = readings[-1] - readings[0]
    assert abs(drift) > 10, "Drift sollte erkannt werden"
```

### Szenario 3: Netzwerk-Ausfall mit Recovery

```python
def test_network_failure_recovery():
    """Testet Recovery nach Netzwerk-Ausfall."""
    sensor = TemperatureSensor("temp_02", {"lat": 52.52, "lon": 13.40})
    publisher = MQTTSensorPublisher()
    
    # Buffer für fehlgeschlagene Messungen
    failed_readings = []
    
    for i in range(50):
        reading = sensor.read()
        
        try:
            success = publisher.publish_reading(sensor.sensor_id, reading)
            if not success:
                failed_readings.append(reading)
        except ConnectionError:
            failed_readings.append(reading)
        
        # Simuliere Reconnect nach 10 Versuchen
        if i == 10:
            publisher.connect()
            
            # Sende gepufferte Readings
            for buffered in failed_readings:
                publisher.publish_reading(sensor.sensor_id, buffered)
            failed_readings.clear()
        
        time.sleep(0.1)
```

## 📈 Monitoring & Metriken

### Sensor-Health-Metriken

```python
class SensorHealthMonitor:
    """Überwacht Sensor-Gesundheit."""
    
    def __init__(self, themis_client):
        self.client = themis_client
        self.metrics = {}
    
    async def calculate_health_score(self, sensor_id: str) -> float:
        """Berechnet Health-Score für Sensor."""
        # Hole letzte 100 Messungen
        readings = await self.client.query(
            "measurements",
            {
                "sensor_id": sensor_id,
                "limit": 100,
                "sort": "-timestamp"
            }
        )
        
        if not readings:
            return 0.0
        
        # Metriken
        avg_quality = sum(r.get("quality", 0) for r in readings) / len(readings)
        missing_rate = (100 - len(readings)) / 100  # Erwartete vs. tatsächliche
        
        # Prüfe auf stuck values
        values = [r["value"] for r in readings]
        is_stuck = len(set(values)) == 1
        
        # Score
        health_score = avg_quality * (1 - missing_rate)
        if is_stuck:
            health_score *= 0.1
        
        return health_score
```

## 🎓 Best Practices

1. **Daten-Validierung**
   - Prüfe Wertebereiche vor Speicherung
   - Verwerfe offensichtlich fehlerhafte Werte

2. **Buffering**
   - Puffere Daten lokal bei Netzwerk-Ausfällen
   - Sende gepufferte Daten nach Reconnect

3. **Zeitstempel**
   - Verwende UTC für alle Zeitstempel
   - Synchronisiere Sensor-Uhren regelmäßig

4. **Fehlerbehandlung**
   - Implementiere Retry-Logic mit Backoff
   - Logge alle Fehler für Debugging

5. **Performance**
   - Verwende Batch-Operationen wo möglich
   - Komprimiere Daten vor Übertragung

## 📚 Weitere Dokumentation

- [CEP_PATTERNS.md](CEP_PATTERNS.md) - Event Processing
- [ML_MODELS.md](ML_MODELS.md) - Anomalie-Erkennung
- [SCALING_GUIDE.md](SCALING_GUIDE.md) - Skalierung
