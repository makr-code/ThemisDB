# Kapitel 19: Monitoring & Observability

## Einführung

Monitoring und Observability sind kritische Aspekte für den produktiven Betrieb von ThemisDB. Dieses Kapitel behandelt die Überwachung von Systemmetriken, Application Performance Monitoring (APM), Logging-Strategien, Distributed Tracing und Alerting-Mechanismen.

## 18.1 Monitoring-Architektur

### 18.1.1 Übersicht

Eine umfassende Monitoring-Lösung für ThemisDB umfasst:

- **Metriken-Sammlung**: Prometheus, StatsD, InfluxDB
- **Visualisierung**: Grafana, Kibana
- **Logging**: ELK Stack (Elasticsearch, Logstash, Kibana), Loki
- **Distributed Tracing**: Jaeger, Zipkin
- **Alerting**: Prometheus Alertmanager, PagerDuty

### 18.1.2 Metriken-Typen

**System-Metriken:**
- CPU-Auslastung
- Speichernutzung (RAM, Swap)
- Disk I/O (IOPS, Latenz, Durchsatz)
- Netzwerk-Traffic (Bandbreite, Pakete)

**ThemisDB-Metriken:**
- Query-Performance (Latenz, Durchsatz)
- Connection Pool (aktive Connections, Wartezeit)
- Transaction-Statistiken (Commits, Rollbacks)
- Cache-Effizienz (Hit Rate, Miss Rate)
- Index-Performance
- Replikations-Lag

## 18.2 Prometheus Integration

### 18.2.1 Prometheus Setup

**Installation:**

```bash
# Prometheus herunterladen
wget https://github.com/prometheus/prometheus/releases/download/v2.45.0/prometheus-2.45.0.linux-amd64.tar.gz
tar xvfz prometheus-2.45.0.linux-amd64.tar.gz
cd prometheus-2.45.0.linux-amd64

# Konfiguration erstellen
cat > prometheus.yml <<EOF
global:
  scrape_interval: 15s
  evaluation_interval: 15s

scrape_configs:
  - job_name: 'themisdb'
    static_configs:
      - targets: ['localhost:8529']
    metrics_path: '/metrics'
EOF

# Prometheus starten
./prometheus --config.file=prometheus.yml
```

### 18.2.2 ThemisDB Exporter

**Metriken-Endpoint aktivieren:**

```python
# themisdb_exporter.py
from prometheus_client import start_http_server, Counter, Gauge, Histogram
from themisdb import ThemisDB
import time

# Metriken definieren
query_counter = Counter('themisdb_queries_total', 'Total number of queries', ['type'])
query_duration = Histogram('themisdb_query_duration_seconds', 'Query duration', ['type'])
active_connections = Gauge('themisdb_active_connections', 'Number of active connections')
cache_hit_rate = Gauge('themisdb_cache_hit_rate', 'Cache hit rate')

db = ThemisDB(host='localhost', port=8529)

def collect_metrics():
    """Sammelt ThemisDB Metriken."""
    while True:
        # Connection-Metriken
        stats = db.statistics()
        active_connections.set(stats['connections']['active'])
        
        # Cache-Metriken
        cache_stats = db.cache_statistics()
        hit_rate = cache_stats['hits'] / (cache_stats['hits'] + cache_stats['misses'])
        cache_hit_rate.set(hit_rate)
        
        # Query-Metriken
        query_stats = db.query_statistics()
        for query_type, count in query_stats.items():
            query_counter.labels(type=query_type).inc(count)
        
        time.sleep(15)

if __name__ == '__main__':
    # Metrics-Server starten
    start_http_server(9090)
    collect_metrics()
```

### 18.2.3 Wichtige Metriken

**Query Performance:**

```yaml
# Query Latenz Histogram
themisdb_query_duration_seconds_bucket{type="SELECT",le="0.1"} 1200
themisdb_query_duration_seconds_bucket{type="SELECT",le="0.5"} 1450
themisdb_query_duration_seconds_bucket{type="SELECT",le="1.0"} 1480
themisdb_query_duration_seconds_bucket{type="SELECT",le="5.0"} 1500

# Query Counter
themisdb_queries_total{type="SELECT"} 1500
themisdb_queries_total{type="INSERT"} 500
themisdb_queries_total{type="UPDATE"} 200
```

**Connection Pool:**

```yaml
# Aktive Connections
themisdb_active_connections 45

# Connection Pool Größe
themisdb_connection_pool_size 100
themisdb_connection_pool_available 55
```

## 18.3 Grafana Dashboards

### 18.3.1 Dashboard Setup

**Grafana Installation:**

```bash
# Grafana installieren
sudo apt-get install -y software-properties-common
sudo add-apt-repository "deb https://packages.grafana.com/oss/deb stable main"
wget -q -O - https://packages.grafana.com/gpg.key | sudo apt-key add -
sudo apt-get update
sudo apt-get install grafana

# Grafana starten
sudo systemctl start grafana-server
sudo systemctl enable grafana-server

# Zugriff: http://localhost:3000 (admin/admin)
```

### 18.3.2 ThemisDB Dashboard

**Dashboard-Definition (JSON):**

```json
{
  "dashboard": {
    "title": "ThemisDB Performance",
    "panels": [
      {
        "title": "Query Latency (p95)",
        "targets": [
          {
            "expr": "histogram_quantile(0.95, themisdb_query_duration_seconds_bucket)"
          }
        ],
        "type": "graph"
      },
      {
        "title": "Queries per Second",
        "targets": [
          {
            "expr": "rate(themisdb_queries_total[1m])"
          }
        ],
        "type": "graph"
      },
      {
        "title": "Active Connections",
        "targets": [
          {
            "expr": "themisdb_active_connections"
          }
        ],
        "type": "graph"
      },
      {
        "title": "Cache Hit Rate",
        "targets": [
          {
            "expr": "themisdb_cache_hit_rate"
          }
        ],
        "type": "gauge"
      }
    ]
  }
}
```

### 18.3.3 Dashboard-Beispiele

**System-Übersicht Dashboard:**
- CPU-Auslastung (pro Core)
- Speichernutzung (RAM, Swap, Cache)
- Disk I/O (Read/Write IOPS, Latenz)
- Netzwerk-Traffic (In/Out Bandwidth)

**Query-Performance Dashboard:**
- Query Latenz (p50, p95, p99)
- Queries per Second (nach Typ)
- Slow Queries (>1s)
- Query Error Rate

**Cluster-Status Dashboard:**
- Node Health Status
- Replikations-Lag
- Cluster-Topology
- Failover-Events

## 18.4 Logging

### 18.4.1 Logging-Strategie

**Log-Levels:**

```python
import logging

# Logging konfigurieren
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler('/var/log/themisdb/themisdb.log'),
        logging.StreamHandler()
    ]
)

logger = logging.getLogger('themisdb')

# Log-Levels verwenden
logger.debug("Connection pool initialized")       # DEBUG
logger.info("Query executed successfully")        # INFO
logger.warning("High memory usage detected")      # WARNING
logger.error("Query execution failed")            # ERROR
logger.critical("Database connection lost")       # CRITICAL
```

### 18.4.2 Strukturiertes Logging

**JSON-Logging:**

```python
import json
import logging

class JSONFormatter(logging.Formatter):
    def format(self, record):
        log_data = {
            'timestamp': self.formatTime(record),
            'level': record.levelname,
            'logger': record.name,
            'message': record.getMessage(),
            'module': record.module,
            'function': record.funcName,
            'line': record.lineno
        }
        
        # Zusätzliche Felder
        if hasattr(record, 'query_id'):
            log_data['query_id'] = record.query_id
        if hasattr(record, 'user_id'):
            log_data['user_id'] = record.user_id
        if hasattr(record, 'duration_ms'):
            log_data['duration_ms'] = record.duration_ms
        
        return json.dumps(log_data)

# Handler konfigurieren
handler = logging.FileHandler('/var/log/themisdb/themisdb.json')
handler.setFormatter(JSONFormatter())
logger.addHandler(handler)

# Logging mit Context
logger.info(
    "Query executed",
    extra={
        'query_id': 'q123',
        'user_id': 'u456',
        'duration_ms': 45.2
    }
)
```

### 18.4.3 ELK Stack Integration

**Filebeat Konfiguration:**

```yaml
# filebeat.yml
filebeat.inputs:
  - type: log
    enabled: true
    paths:
      - /var/log/themisdb/*.log
    json.keys_under_root: true
    json.add_error_key: true

output.elasticsearch:
  hosts: ["localhost:9200"]
  index: "themisdb-logs-%{+yyyy.MM.dd}"

setup.kibana:
  host: "localhost:5601"
```

**Logstash Pipeline:**

```conf
# logstash.conf
input {
  beats {
    port => 5044
  }
}

filter {
  if [type] == "themisdb" {
    json {
      source => "message"
    }
    
    # Query-Latenz berechnen
    if [duration_ms] {
      ruby {
        code => "event.set('duration_s', event.get('duration_ms') / 1000.0)"
      }
    }
    
    # Geo-IP Enrichment
    geoip {
      source => "client_ip"
      target => "geoip"
    }
  }
}

output {
  elasticsearch {
    hosts => ["localhost:9200"]
    index => "themisdb-logs-%{+YYYY.MM.dd}"
  }
}
```

## 18.5 Distributed Tracing

### 18.5.1 Jaeger Integration

**Jaeger Setup:**

```bash
# Jaeger All-in-One starten
docker run -d \
  -e COLLECTOR_ZIPKIN_HTTP_PORT=9411 \
  -p 5775:5775/udp \
  -p 6831:6831/udp \
  -p 6832:6832/udp \
  -p 5778:5778 \
  -p 16686:16686 \
  -p 14268:14268 \
  -p 9411:9411 \
  jaegertracing/all-in-one:latest

# Zugriff: http://localhost:16686
```

### 18.5.2 OpenTelemetry Integration

**Tracing-Code:**

```python
from opentelemetry import trace
from opentelemetry.sdk.trace import TracerProvider
from opentelemetry.sdk.trace.export import BatchSpanProcessor
from opentelemetry.exporter.jaeger.thrift import JaegerExporter
from opentelemetry.instrumentation.requests import RequestsInstrumentor

# Tracer konfigurieren
trace.set_tracer_provider(TracerProvider())
tracer = trace.get_tracer(__name__)

jaeger_exporter = JaegerExporter(
    agent_host_name='localhost',
    agent_port=6831,
)

trace.get_tracer_provider().add_span_processor(
    BatchSpanProcessor(jaeger_exporter)
)

# Requests instrumentieren
RequestsInstrumentor().instrument()

# Query mit Tracing
@tracer.start_as_current_span("execute_query")
def execute_query(query):
    span = trace.get_current_span()
    span.set_attribute("query.type", "SELECT")
    span.set_attribute("query.text", query)
    
    # Query ausführen
    with tracer.start_as_current_span("database_query"):
        result = db.execute(query)
    
    span.set_attribute("result.count", len(result))
    return result

# Multi-Service Tracing
@tracer.start_as_current_span("process_order")
def process_order(order_id):
    # Span 1: Order validieren
    with tracer.start_as_current_span("validate_order"):
        order = db.query(f"SELECT * FROM orders WHERE id = {order_id}")
    
    # Span 2: Inventory prüfen
    with tracer.start_as_current_span("check_inventory"):
        inventory = db.query(f"SELECT * FROM inventory WHERE product_id = {order['product_id']}")
    
    # Span 3: Payment verarbeiten
    with tracer.start_as_current_span("process_payment"):
        payment = process_payment_service(order)
    
    return {"status": "success"}
```

## 18.6 Alerting

### 18.6.1 Prometheus Alertmanager

**Alert-Regeln:**

```yaml
# alerts.yml
groups:
  - name: themisdb
    interval: 30s
    rules:
      # High Query Latency
      - alert: HighQueryLatency
        expr: histogram_quantile(0.95, themisdb_query_duration_seconds_bucket) > 1.0
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "High query latency detected"
          description: "p95 query latency is {{ $value }}s"
      
      # Low Cache Hit Rate
      - alert: LowCacheHitRate
        expr: themisdb_cache_hit_rate < 0.7
        for: 10m
        labels:
          severity: warning
        annotations:
          summary: "Low cache hit rate"
          description: "Cache hit rate is {{ $value }}"
      
      # High Connection Usage
      - alert: HighConnectionUsage
        expr: (themisdb_active_connections / themisdb_connection_pool_size) > 0.8
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "Connection pool nearly exhausted"
          description: "{{ $value | humanizePercentage }} of connections in use"
      
      # Database Down
      - alert: DatabaseDown
        expr: up{job="themisdb"} == 0
        for: 1m
        labels:
          severity: critical
        annotations:
          summary: "ThemisDB instance is down"
          description: "Instance {{ $labels.instance }} is unreachable"
      
      # Replication Lag
      - alert: HighReplicationLag
        expr: themisdb_replication_lag_seconds > 10
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "High replication lag"
          description: "Replication lag is {{ $value }}s"
```

### 18.6.2 Alertmanager Konfiguration

**Alertmanager Config:**

```yaml
# alertmanager.yml
global:
  resolve_timeout: 5m

route:
  group_by: ['alertname', 'cluster']
  group_wait: 10s
  group_interval: 10s
  repeat_interval: 12h
  receiver: 'team-db'
  
  routes:
    - match:
        severity: critical
      receiver: 'pagerduty'
    
    - match:
        severity: warning
      receiver: 'slack'

receivers:
  - name: 'team-db'
    email_configs:
      - to: 'db-team@company.com'
        from: 'alertmanager@company.com'
        smarthost: 'smtp.company.com:587'
  
  - name: 'pagerduty'
    pagerduty_configs:
      - service_key: 'YOUR_PAGERDUTY_KEY'
  
  - name: 'slack'
    slack_configs:
      - api_url: 'https://hooks.slack.com/services/YOUR/SLACK/WEBHOOK'
        channel: '#db-alerts'
        text: '{{ range .Alerts }}{{ .Annotations.summary }}\n{{ end }}'
```

## 18.7 Application Performance Monitoring (APM)

### 18.7.1 Query Performance Tracking

**Slow Query Logging:**

```python
import time
from functools import wraps

SLOW_QUERY_THRESHOLD = 1.0  # Sekunden

def track_query_performance(func):
    @wraps(func)
    def wrapper(*args, **kwargs):
        start_time = time.time()
        query = args[0] if args else kwargs.get('query')
        
        try:
            result = func(*args, **kwargs)
            duration = time.time() - start_time
            
            # Slow Query loggen
            if duration > SLOW_QUERY_THRESHOLD:
                logger.warning(
                    "Slow query detected",
                    extra={
                        'query': query,
                        'duration_ms': duration * 1000,
                        'threshold_ms': SLOW_QUERY_THRESHOLD * 1000
                    }
                )
            
            # Metriken updaten
            query_duration.labels(type=get_query_type(query)).observe(duration)
            
            return result
        except Exception as e:
            duration = time.time() - start_time
            logger.error(
                "Query execution failed",
                extra={
                    'query': query,
                    'duration_ms': duration * 1000,
                    'error': str(e)
                }
            )
            raise
    
    return wrapper

@track_query_performance
def execute_query(query):
    return db.execute(query)
```

### 18.7.2 Transaction Monitoring

**Transaction Tracking:**

```python
class TransactionMonitor:
    def __init__(self):
        self.active_transactions = {}
        self.transaction_counter = Counter('themisdb_transactions_total', 
                                          'Total transactions', 
                                          ['status'])
        self.transaction_duration = Histogram('themisdb_transaction_duration_seconds',
                                             'Transaction duration')
    
    def begin_transaction(self, txn_id):
        self.active_transactions[txn_id] = {
            'start_time': time.time(),
            'queries': []
        }
    
    def add_query(self, txn_id, query):
        if txn_id in self.active_transactions:
            self.active_transactions[txn_id]['queries'].append(query)
    
    def commit_transaction(self, txn_id):
        if txn_id in self.active_transactions:
            duration = time.time() - self.active_transactions[txn_id]['start_time']
            query_count = len(self.active_transactions[txn_id]['queries'])
            
            self.transaction_counter.labels(status='committed').inc()
            self.transaction_duration.observe(duration)
            
            logger.info(
                "Transaction committed",
                extra={
                    'txn_id': txn_id,
                    'duration_ms': duration * 1000,
                    'query_count': query_count
                }
            )
            
            del self.active_transactions[txn_id]
    
    def rollback_transaction(self, txn_id):
        if txn_id in self.active_transactions:
            duration = time.time() - self.active_transactions[txn_id]['start_time']
            
            self.transaction_counter.labels(status='rolled_back').inc()
            
            logger.warning(
                "Transaction rolled back",
                extra={
                    'txn_id': txn_id,
                    'duration_ms': duration * 1000
                }
            )
            
            del self.active_transactions[txn_id]
```

## 18.8 Health Checks

### 18.8.1 Liveness & Readiness Probes

**Health Check Endpoints:**

```python
from flask import Flask, jsonify
import time

app = Flask(__name__)

@app.route('/health/live')
def liveness():
    """Prüft ob die Anwendung läuft."""
    return jsonify({'status': 'ok'}), 200

@app.route('/health/ready')
def readiness():
    """Prüft ob die Anwendung Requests bearbeiten kann."""
    try:
        # Datenbankverbindung testen
        db.execute("SELECT 1")
        
        # Connection Pool prüfen
        stats = db.statistics()
        if stats['connections']['available'] == 0:
            return jsonify({
                'status': 'not_ready',
                'reason': 'Connection pool exhausted'
            }), 503
        
        # Replikations-Lag prüfen
        if stats.get('replication_lag', 0) > 30:
            return jsonify({
                'status': 'not_ready',
                'reason': 'High replication lag'
            }), 503
        
        return jsonify({'status': 'ready'}), 200
    
    except Exception as e:
        return jsonify({
            'status': 'not_ready',
            'reason': str(e)
        }), 503

@app.route('/health/startup')
def startup():
    """Prüft ob die Anwendung vollständig gestartet ist."""
    # Initialisierungs-Checks
    checks = {
        'database_connected': check_db_connection(),
        'cache_initialized': check_cache(),
        'migrations_applied': check_migrations()
    }
    
    if all(checks.values()):
        return jsonify({'status': 'started', 'checks': checks}), 200
    else:
        return jsonify({'status': 'starting', 'checks': checks}), 503
```

### 18.8.2 Kubernetes Probes

**Pod-Konfiguration:**

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: themisdb
spec:
  containers:
  - name: themisdb
    image: themisdb:latest
    ports:
    - containerPort: 8529
    
    livenessProbe:
      httpGet:
        path: /health/live
        port: 8529
      initialDelaySeconds: 30
      periodSeconds: 10
      timeoutSeconds: 5
      failureThreshold: 3
    
    readinessProbe:
      httpGet:
        path: /health/ready
        port: 8529
      initialDelaySeconds: 10
      periodSeconds: 5
      timeoutSeconds: 3
      failureThreshold: 3
    
    startupProbe:
      httpGet:
        path: /health/startup
        port: 8529
      initialDelaySeconds: 0
      periodSeconds: 10
      timeoutSeconds: 3
      failureThreshold: 30
```

## 18.9 Performance Profiling

### 18.9.1 Query Profiling

**EXPLAIN ANALYZE:**

```aql
-- Query Plan analysieren
EXPLAIN ANALYZE
SELECT c.name, COUNT(o.id) as order_count
FROM customers c
LEFT JOIN orders o ON c.id = o.customer_id
WHERE c.country = 'DE'
GROUP BY c.name
ORDER BY order_count DESC
LIMIT 10;

-- Output:
-- -> Limit (cost=1245.34..1245.36 rows=10)
--    -> Sort (cost=1245.34..1276.42 rows=12432)
--          Sort Key: order_count DESC
--          -> HashAggregate (cost=956.23..1080.55 rows=12432)
--                -> Hash Left Join (cost=345.67..876.12 rows=16040)
--                      Hash Cond: (c.id = o.customer_id)
--                      -> Index Scan using idx_customers_country on customers c
--                           Index Cond: (country = 'DE')
--                      -> Hash (cost=234.56..234.56 rows=8884)
--                           -> Seq Scan on orders o
```

### 18.9.2 Python Profiling

**cProfile Integration:**

```python
import cProfile
import pstats
from pstats import SortKey

def profile_query(query):
    profiler = cProfile.Profile()
    profiler.enable()
    
    # Query ausführen
    result = db.execute(query)
    
    profiler.disable()
    
    # Statistiken ausgeben
    stats = pstats.Stats(profiler)
    stats.sort_stats(SortKey.CUMULATIVE)
    stats.print_stats(10)
    
    return result

# Line Profiler
from line_profiler import LineProfiler

@profile
def complex_operation():
    # Schritt 1
    data = db.query("SELECT * FROM large_table")
    
    # Schritt 2
    processed = [process_row(row) for row in data]
    
    # Schritt 3
    db.bulk_insert("results", processed)

# Memory Profiler
from memory_profiler import profile as memory_profile

@memory_profile
def memory_intensive_operation():
    large_dataset = db.query("SELECT * FROM huge_table")
    return process_dataset(large_dataset)
```

## 18.10 Best Practices

### 18.10.1 Monitoring-Strategie

**Golden Signals:**

1. **Latency**: Zeit für Request-Bearbeitung
2. **Traffic**: Anzahl Requests pro Zeiteinheit
3. **Errors**: Rate fehlgeschlagener Requests
4. **Saturation**: Auslastung der Ressourcen

**RED Method (für Services):**
- **Rate**: Requests pro Sekunde
- **Errors**: Fehlerrate
- **Duration**: Latenz-Verteilung

**USE Method (für Ressourcen):**
- **Utilization**: Prozentuale Auslastung
- **Saturation**: Wartezeit/Queueing
- **Errors**: Fehlerzähler

### 18.10.2 Retention Policies

**Datenaufbewahrung:**

```yaml
# Prometheus Retention
prometheus:
  retention:
    time: 15d
    size: 50GB

# Aggregierte Metriken
recording_rules:
  - name: themisdb_5m
    interval: 5m
    rules:
      - record: themisdb:query_rate:5m
        expr: rate(themisdb_queries_total[5m])
      
      - record: themisdb:query_latency_p95:5m
        expr: histogram_quantile(0.95, themisdb_query_duration_seconds_bucket[5m])
```

**Log-Rotation:**

```conf
# /etc/logrotate.d/themisdb
/var/log/themisdb/*.log {
    daily
    rotate 7
    compress
    delaycompress
    missingok
    notifempty
    create 0640 themisdb themisdb
    sharedscripts
    postrotate
        systemctl reload themisdb
    endscript
}
```

### 18.10.3 Dashboard-Design

**Effektive Dashboards:**

1. **Übersichts-Dashboard**: Hochlevel-Metriken, Status-Indikatoren
2. **Detail-Dashboards**: Tiefere Einblicke für spezifische Bereiche
3. **Drill-Down**: Von Übersicht zu Details navigieren
4. **Zeitfenster**: Flexible Zeitbereichs-Auswahl
5. **Annotations**: Deployment-Marker, Incidents

**Dashboard-Struktur:**
- **Oberste Zeile**: SLI/SLO Status, Alerts
- **Zweite Zeile**: Golden Signals (Latency, Traffic, Errors, Saturation)
- **Weitere Zeilen**: Spezifische Metriken nach Kategorie

## Zusammenfassung

Monitoring und Observability sind essentiell für den Betrieb von ThemisDB:

- **Metriken**: Prometheus für Sammlung und Speicherung
- **Visualisierung**: Grafana für Dashboards
- **Logging**: ELK Stack für Log-Aggregation und Analyse
- **Tracing**: Jaeger/OpenTelemetry für Distributed Tracing
- **Alerting**: Proaktive Benachrichtigung bei Problemen
- **Health Checks**: Kubernetes-Integration für Orchestrierung
- **Profiling**: Performance-Analyse und Optimierung

Mit diesen Tools und Praktiken können Sie ThemisDB effektiv überwachen und Probleme frühzeitig erkennen.

---

## 19.10 Phase-3-Sync: Weiterführende Referenzen (docs/de/) {#chapter19_10_cross-references}

> Detaillierte Implementierungsdokumentation zu den behandelten Monitoring- und Observability-Themen:

| Thema | Referenz (docs/de/) |
|---|---|
| Observability Metriken | [`docs/de/observability/observability_metrics.md`](../../de/observability/observability_metrics.md) |
| Alerting Konfiguration | [`docs/de/observability/observability_alerting.md`](../../de/observability/observability_alerting.md) |
| OpenTelemetry Integration | [`docs/de/observability/observability_opentelemetry.md`](../../de/observability/observability_opentelemetry.md) |
| Primärquellen-Index | [`docs/de/observability/PRIMARY_SOURCES.md`](../../de/observability/PRIMARY_SOURCES.md) |

**→ Zurück:** [Kapitel 18: HA & Cluster](chapter_18_ha.md)  
**→ Weiter:** [Kapitel 19b: Observability](chapter_19_monitoring_observability.md)
