> ⚠️ **Historische Messdaten** – Die in diesem Dokument enthaltenen Zahlen entstammen einem bestimmten Messzeitpunkt.
> Für reproduzierbare Ergebnisse: CMake-Presets und aktuellen Teststand verwenden.

# Scaling Guide - IoT Sensornetzwerk

## 📋 Übersicht

Strategien und Best Practices für die Skalierung eines IoT-Sensornetzwerks von einigen wenigen Sensoren bis zu Millionen von Geräten.

## 📊 Skalierungs-Dimensionen

### Daten-Volumen
- **Klein**: < 100 Sensoren, < 1 GB/Tag
- **Mittel**: 100-10.000 Sensoren, 1-100 GB/Tag
- **Groß**: 10.000-1M Sensoren, 100 GB - 10 TB/Tag
- **Sehr Groß**: > 1M Sensoren, > 10 TB/Tag

### Throughput
- **Niedrig**: < 1.000 Nachrichten/Sekunde
- **Mittel**: 1.000-100.000 Nachrichten/Sekunde
- **Hoch**: 100.000-1M Nachrichten/Sekunde
- **Sehr Hoch**: > 1M Nachrichten/Sekunde

### Latenz-Anforderungen
- **Real-time**: < 100ms
- **Near Real-time**: 100ms - 1s
- **Soft Real-time**: 1s - 10s
- **Batch**: > 10s

## 🔀 Horizontal vs. Vertical Scaling

### Vertical Scaling (Scale Up)

**Vorteile:**
- Einfacher zu implementieren
- Keine verteilte Komplexität
- Bessere Performance für Single-Node Operationen

**Nachteile:**
- Begrenzt durch Hardware-Limits
- Single Point of Failure
- Teurer pro Kapazität

**Wann verwenden:**
- Frühe Entwicklungsphase
- < 10.000 Sensoren
- Einfache Deployment-Anforderungen

```yaml
# Vertical Scaling Beispiel
# Von: 4 CPU, 8GB RAM
# Zu: 16 CPU, 64GB RAM

docker-compose.yml:
  themisdb:
    image: themisdb:latest
    deploy:
      resources:
        limits:
          cpus: '16'
          memory: 64G
        reservations:
          cpus: '8'
          memory: 32G
```

### Horizontal Scaling (Scale Out)

**Vorteile:**
- Unbegrenzte Skalierbarkeit
- Redundanz und Hochverfügbarkeit
- Kosteneffizient

**Nachteile:**
- Komplexere Architektur
- Datenverteilung notwendig
- Koordination zwischen Nodes

**Wann verwenden:**
- > 10.000 Sensoren
- Hohe Verfügbarkeitsanforderungen
- Globale Verteilung notwendig

## 🗂️ Sharding-Strategien

### 1. Sensor-Based Sharding

Verteilung basierend auf Sensor-ID:

```python
class SensorBasedSharding:
    """Sharding basierend auf Sensor-ID."""
    
    def __init__(self, num_shards: int):
        self.num_shards = num_shards
    
    def get_shard(self, sensor_id: str) -> int:
        """Berechnet Shard für Sensor."""
        # Consistent Hashing
        hash_value = hash(sensor_id)
        return hash_value % self.num_shards
    
    def route_message(self, sensor_id: str, message: dict) -> str:
        """Routet Nachricht zum richtigen Shard."""
        shard = self.get_shard(sensor_id)
        return f"shard_{shard}"

# Verwendung
sharding = SensorBasedSharding(num_shards=4)

sensor_id = "temp_sensor_001"
target_shard = sharding.route_message(sensor_id, {"value": 23.5})
print(f"Route to: {target_shard}")
```

### 2. Geographic Sharding

Verteilung basierend auf Standort:

```python
class GeographicSharding:
    """Sharding basierend auf geografischer Region."""
    
    def __init__(self):
        self.regions = {
            'europe': {'lat_min': 35, 'lat_max': 71, 'lon_min': -10, 'lon_max': 40},
            'north_america': {'lat_min': 15, 'lat_max': 72, 'lon_min': -168, 'lon_max': -52},
            'asia': {'lat_min': -10, 'lat_max': 55, 'lon_min': 60, 'lon_max': 150},
            'australia': {'lat_min': -47, 'lat_max': -10, 'lon_min': 113, 'lon_max': 153}
        }
    
    def get_region(self, lat: float, lon: float) -> str:
        """Bestimmt Region basierend auf Koordinaten."""
        for region_name, bounds in self.regions.items():
            if (bounds['lat_min'] <= lat <= bounds['lat_max'] and
                bounds['lon_min'] <= lon <= bounds['lon_max']):
                return region_name
        return 'other'
    
    def route_message(self, location: dict, message: dict) -> str:
        """Routet Nachricht zur regionalen Instanz."""
        region = self.get_region(location['lat'], location['lon'])
        return f"themisdb_{region}"

# Verwendung
geo_sharding = GeographicSharding()

location = {'lat': 52.52, 'lon': 13.40}  # Berlin
target = geo_sharding.route_message(location, {"value": 23.5})
print(f"Route to: {target}")  # themisdb_europe
```

### 3. Time-Based Sharding

Verteilung basierend auf Zeitstempel:

```python
from datetime import datetime, timedelta

class TimeBasedSharding:
    """Sharding basierend auf Zeit (für Time-Series)."""
    
    def __init__(self, shard_duration_days: int = 7):
        self.shard_duration = timedelta(days=shard_duration_days)
        self.epoch = datetime(2025, 1, 1)
    
    def get_shard(self, timestamp: datetime) -> str:
        """Berechnet Shard für Zeitstempel."""
        days_since_epoch = (timestamp - self.epoch).days
        shard_number = days_since_epoch // self.shard_duration.days
        return f"shard_{shard_number}"
    
    def get_shard_range(self, shard_name: str) -> tuple:
        """Gibt Zeitbereich für Shard zurück."""
        shard_number = int(shard_name.split('_')[1])
        start = self.epoch + (shard_number * self.shard_duration)
        end = start + self.shard_duration
        return start, end

# Verwendung
time_sharding = TimeBasedSharding(shard_duration_days=7)

timestamp = datetime.now()
shard = time_sharding.get_shard(timestamp)
print(f"Timestamp {timestamp} → {shard}")
```

## ⚖️ Load Balancing

### Round Robin

```python
class RoundRobinLoadBalancer:
    """Simple Round-Robin Load Balancing."""
    
    def __init__(self, backends: list):
        self.backends = backends
        self.current = 0
    
    def get_next(self) -> str:
        """Gibt nächsten Backend zurück."""
        backend = self.backends[self.current]
        self.current = (self.current + 1) % len(self.backends)
        return backend

# Verwendung
lb = RoundRobinLoadBalancer([
    "themisdb-1:8080",
    "themisdb-2:8080",
    "themisdb-3:8080"
])

for i in range(10):
    backend = lb.get_next()
    print(f"Request {i} → {backend}")
```

### Weighted Load Balancing

```python
import random

class WeightedLoadBalancer:
    """Gewichtetes Load Balancing basierend auf Kapazität."""
    
    def __init__(self, backends: dict):
        """
        backends: {"backend_name": weight}
        Höheres Gewicht = mehr Traffic
        """
        self.backends = backends
        self.total_weight = sum(backends.values())
    
    def get_next(self) -> str:
        """Wählt Backend basierend auf Gewichtung."""
        rand = random.uniform(0, self.total_weight)
        cumulative = 0
        
        for backend, weight in self.backends.items():
            cumulative += weight
            if rand <= cumulative:
                return backend
        
        return list(self.backends.keys())[-1]

# Verwendung (Server 1 ist doppelt so stark)
lb = WeightedLoadBalancer({
    "themisdb-1:8080": 2,
    "themisdb-2:8080": 1,
    "themisdb-3:8080": 1
})
```

### Least Connections

```python
class LeastConnectionsLoadBalancer:
    """Load Balancing basierend auf aktiven Verbindungen."""
    
    def __init__(self, backends: list):
        self.backends = {backend: 0 for backend in backends}
    
    def get_next(self) -> str:
        """Gibt Backend mit wenigsten Verbindungen zurück."""
        return min(self.backends, key=self.backends.get)
    
    def connection_opened(self, backend: str):
        """Registriert neue Verbindung."""
        self.backends[backend] += 1
    
    def connection_closed(self, backend: str):
        """Registriert geschlossene Verbindung."""
        self.backends[backend] = max(0, self.backends[backend] - 1)

# Verwendung
lb = LeastConnectionsLoadBalancer([
    "themisdb-1:8080",
    "themisdb-2:8080",
    "themisdb-3:8080"
])

backend = lb.get_next()
lb.connection_opened(backend)
# ... do work ...
lb.connection_closed(backend)
```

## 📦 Message Queue Integration

### MQTT → Kafka Bridge

Für hohen Throughput:

```python
from kafka import KafkaProducer
import paho.mqtt.client as mqtt
import json

class MQTTKafkaBridge:
    """Bridge von MQTT zu Kafka für Skalierung."""
    
    def __init__(
        self,
        mqtt_broker: str,
        kafka_brokers: list,
        kafka_topic: str
    ):
        # Kafka Producer
        self.kafka_producer = KafkaProducer(
            bootstrap_servers=kafka_brokers,
            value_serializer=lambda v: json.dumps(v).encode('utf-8'),
            compression_type='gzip',
            batch_size=16384,
            linger_ms=10
        )
        self.kafka_topic = kafka_topic
        
        # MQTT Client
        self.mqtt_client = mqtt.Client()
        self.mqtt_client.on_connect = self._on_connect
        self.mqtt_client.on_message = self._on_message
        self.mqtt_broker = mqtt_broker
        
        self.messages_processed = 0
    
    def start(self):
        """Startet Bridge."""
        self.mqtt_client.connect(self.mqtt_broker, 1883, 60)
        self.mqtt_client.loop_start()
    
    def _on_connect(self, client, userdata, flags, rc):
        """Subscribe zu allen Sensor-Topics."""
        client.subscribe("sensors/#", qos=1)
    
    def _on_message(self, client, userdata, msg):
        """Forwarded MQTT Message zu Kafka."""
        try:
            message = {
                'topic': msg.topic,
                'payload': json.loads(msg.payload),
                'timestamp': datetime.now().isoformat()
            }
            
            # Send to Kafka
            self.kafka_producer.send(self.kafka_topic, value=message)
            self.messages_processed += 1
            
            if self.messages_processed % 1000 == 0:
                print(f"Processed {self.messages_processed} messages")
        
        except Exception as e:
            print(f"Error processing message: {e}")

# Verwendung
bridge = MQTTKafkaBridge(
    mqtt_broker="localhost",
    kafka_brokers=["kafka-1:9092", "kafka-2:9092"],
    kafka_topic="sensor_data"
)
bridge.start()
```

### Kafka Consumer für ThemisDB

```python
from kafka import KafkaConsumer
import asyncio

class KafkaThemisDBConsumer:
    """Konsumiert von Kafka und schreibt zu ThemisDB."""
    
    def __init__(
        self,
        kafka_brokers: list,
        kafka_topic: str,
        themis_client,
        batch_size: int = 100
    ):
        self.consumer = KafkaConsumer(
            kafka_topic,
            bootstrap_servers=kafka_brokers,
            value_deserializer=lambda m: json.loads(m.decode('utf-8')),
            auto_offset_reset='earliest',
            enable_auto_commit=False,
            max_poll_records=batch_size
        )
        self.themis_client = themis_client
        self.batch_size = batch_size
    
    async def consume(self):
        """Konsumiert Messages und schreibt zu ThemisDB."""
        batch = []
        
        for message in self.consumer:
            batch.append(message.value['payload'])
            
            if len(batch) >= self.batch_size:
                await self._write_batch(batch)
                batch = []
                self.consumer.commit()
    
    async def _write_batch(self, batch: list):
        """Schreibt Batch zu ThemisDB."""
        try:
            await self.themis_client.batch_create("measurements", batch)
            print(f"Wrote batch of {len(batch)} measurements")
        except Exception as e:
            print(f"Error writing batch: {e}")
```

## 💾 Database Optimization

### Batch Inserts

```python
class BatchInserter:
    """Optimiert Inserts durch Batching."""
    
    def __init__(
        self,
        themis_client,
        batch_size: int = 1000,
        flush_interval_seconds: int = 5
    ):
        self.client = themis_client
        self.batch_size = batch_size
        self.flush_interval = flush_interval_seconds
        self.buffer = []
        self.last_flush = datetime.now()
    
    async def add(self, measurement: dict):
        """Fügt Messung zum Buffer hinzu."""
        self.buffer.append(measurement)
        
        # Flush wenn Batch voll oder Timeout
        should_flush = (
            len(self.buffer) >= self.batch_size or
            (datetime.now() - self.last_flush).total_seconds() >= self.flush_interval
        )
        
        if should_flush:
            await self.flush()
    
    async def flush(self):
        """Schreibt Buffer zu DB."""
        if not self.buffer:
            return
        
        try:
            await self.client.batch_create("measurements", self.buffer)
            print(f"Flushed {len(self.buffer)} measurements")
            self.buffer = []
            self.last_flush = datetime.now()
        except Exception as e:
            print(f"Error flushing batch: {e}")
```

### Indexing Strategy

```sql
-- Composite Index für häufige Queries
CREATE INDEX idx_sensor_time ON measurements(sensor_id, timestamp DESC);

-- Partial Index für Recent Data
CREATE INDEX idx_recent_measurements 
ON measurements(timestamp) 
WHERE timestamp > NOW() - INTERVAL '7 days';

-- Covering Index für Aggregationen
CREATE INDEX idx_sensor_aggregation 
ON measurements(sensor_id, timestamp, value);
```

### Time-Series Optimization

```python
class TimeSeriesOptimizer:
    """Optimierungen für Time-Series Daten."""
    
    @staticmethod
    async def setup_retention_policy(
        themis_client,
        collection: str,
        retention_days: int
    ):
        """Setzt Retention Policy."""
        await themis_client.execute(f"""
            CREATE RETENTION POLICY IF NOT EXISTS rp_{retention_days}d
            ON {collection}
            DURATION {retention_days}d
            REPLICATION 1
            DEFAULT
        """)
    
    @staticmethod
    async def setup_continuous_aggregation(
        themis_client,
        source_collection: str,
        target_collection: str,
        interval: str = '1h'
    ):
        """Erstellt kontinuierliche Aggregation."""
        await themis_client.execute(f"""
            CREATE CONTINUOUS QUERY cq_aggregate_{interval}
            ON {source_collection}
            BEGIN
                SELECT 
                    sensor_id,
                    MEAN(value) as avg_value,
                    MIN(value) as min_value,
                    MAX(value) as max_value,
                    COUNT(*) as count
                INTO {target_collection}
                FROM measurements
                GROUP BY time({interval}), sensor_id
            END
        """)
```

## 📊 Performance Benchmarks

### Throughput Benchmark

```python
import time
import asyncio
from concurrent.futures import ThreadPoolExecutor

class ThroughputBenchmark:
    """Misst Durchsatz des Systems."""
    
    def __init__(self, themis_client):
        self.client = themis_client
    
    async def run(
        self,
        num_messages: int = 10000,
        num_workers: int = 10
    ) -> dict:
        """Führt Throughput-Test aus."""
        start_time = time.time()
        
        # Erstelle Test-Daten
        messages = [
            {
                "sensor_id": f"sensor_{i % 100}",
                "value": random.uniform(20, 30),
                "timestamp": datetime.now().isoformat()
            }
            for i in range(num_messages)
        ]
        
        # Parallel schreiben
        chunk_size = num_messages // num_workers
        chunks = [
            messages[i:i + chunk_size]
            for i in range(0, num_messages, chunk_size)
        ]
        
        tasks = [
            self.client.batch_create("measurements", chunk)
            for chunk in chunks
        ]
        
        await asyncio.gather(*tasks)
        
        end_time = time.time()
        duration = end_time - start_time
        
        return {
            'total_messages': num_messages,
            'duration_seconds': duration,
            'messages_per_second': num_messages / duration,
            'num_workers': num_workers
        }

# Verwendung
benchmark = ThroughputBenchmark(themis_client)
results = asyncio.run(benchmark.run(num_messages=100000, num_workers=20))
print(f"Throughput: {results['messages_per_second']:.2f} msg/s")
```

### Latency Benchmark

```python
class LatencyBenchmark:
    """Misst Latenz des Systems."""
    
    def __init__(self, themis_client):
        self.client = themis_client
    
    async def run(self, num_samples: int = 1000) -> dict:
        """Misst Ende-zu-Ende Latenz."""
        latencies = []
        
        for i in range(num_samples):
            message = {
                "sensor_id": f"sensor_{i}",
                "value": random.uniform(20, 30),
                "timestamp": datetime.now().isoformat()
            }
            
            start = time.time()
            await self.client.create("measurements", message)
            end = time.time()
            
            latencies.append((end - start) * 1000)  # Convert to ms
        
        latencies = np.array(latencies)
        
        return {
            'mean_latency_ms': np.mean(latencies),
            'median_latency_ms': np.median(latencies),
            'p95_latency_ms': np.percentile(latencies, 95),
            'p99_latency_ms': np.percentile(latencies, 99),
            'max_latency_ms': np.max(latencies)
        }
```

## 🎯 Skalierungs-Roadmap

### Phase 1: Single Instance (0-1K Sensoren)
- Single ThemisDB Instance
- Lokaler MQTT Broker
- Direktes Schreiben zu DB

**Kapazität:** 1-5K Nachrichten/Sekunde

### Phase 2: Vertical Scaling (1K-10K Sensoren)
- Größere ThemisDB Instance (mehr CPU/RAM)
- MQTT Broker Cluster
- Connection Pooling

**Kapazität:** 5-50K Nachrichten/Sekunde

### Phase 3: Horizontal Scaling (10K-100K Sensoren)
- ThemisDB Cluster (3-5 Nodes)
- Kafka für Message Buffering
- Load Balancer
- Geographisches Sharding

**Kapazität:** 50-500K Nachrichten/Sekunde

### Phase 4: Geo-Distributed (100K-1M Sensoren)
- Multi-Region Deployment
- Edge Computing für Preprocessing
- CDN für Static Assets
- Advanced Caching

**Kapazität:** 500K-5M Nachrichten/Sekunde

### Phase 5: Massive Scale (> 1M Sensoren)
- Microservices Architektur
- Stream Processing (Apache Flink)
- Data Lake Integration
- Machine Learning Pipeline

**Kapazität:** > 5M Nachrichten/Sekunde

## 🎓 Best Practices

1. **Start Simple**
   - Beginne mit Single Instance
   - Scale nur wenn notwendig
   - Measure vor Optimization

2. **Monitoring ist kritisch**
   - Tracke alle relevanten Metriken
   - Alert bei Anomalien
   - Capacity Planning

3. **Datenkonsistenz**
   - Eventual Consistency akzeptieren
   - Idempotente Operationen
   - Deduplizierung implementieren

4. **Cost Optimization**
   - Right-size Instances
   - Auto-Scaling verwenden
   - Archive alte Daten

## 📚 Weitere Dokumentation

- [SENSOR_SIMULATION.md](SENSOR_SIMULATION.md) - Sensor-Setup
- [CEP_PATTERNS.md](CEP_PATTERNS.md) - Event Processing
- [ML_MODELS.md](ML_MODELS.md) - Anomalie-Erkennung
