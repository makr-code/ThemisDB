# Telemetry Libraries for ThemisDB Instance Tracking

## Overview

For collecting metrics from ThemisDB instances worldwide, there are several established libraries and approaches available.

---

## Recommended Libraries

### 1. **OpenTelemetry** (RECOMMENDED) ⭐
- **Website**: https://opentelemetry.io/
- **Python Library**: `opentelemetry-api`, `opentelemetry-sdk`
- **Description**: Industry-standard observability framework
- **Pros**:
  - Vendor-neutral standard
  - Wide adoption (CNCF project)
  - Supports metrics, traces, and logs
  - Auto-instrumentation available
  - Works with multiple backends (Prometheus, Jaeger, etc.)
- **Cons**:
  - Can be complex for simple use cases
  - Requires backend infrastructure

**Installation:**
```bash
pip install opentelemetry-api opentelemetry-sdk opentelemetry-exporter-otlp
```

**Basic Usage:**
```python
from opentelemetry import metrics
from opentelemetry.sdk.metrics import MeterProvider
from opentelemetry.sdk.metrics.export import PeriodicExportingMetricReader

# Create meter
meter = metrics.get_meter("themisdb.instance")

# Create counter for instance count
instance_counter = meter.create_counter(
    "themisdb.instances.active",
    description="Number of active ThemisDB instances"
)

# Record metric
instance_counter.add(1, {"version": "1.0.0", "region": "eu-west"})
```

---

### 2. **Prometheus Client** ⭐
- **Website**: https://prometheus.io/
- **Python Library**: `prometheus-client`
- **Description**: Popular metrics collection and monitoring system
- **Pros**:
  - Simple to use
  - Wide industry adoption
  - Built-in HTTP exposition format
  - Time-series database
  - Great for aggregation
- **Cons**:
  - Pull-based (requires endpoints)
  - Separate infrastructure needed

**Installation:**
```bash
pip install prometheus-client
```

**Basic Usage:**
```python
from prometheus_client import Counter, Gauge, Histogram, start_http_server

# Define metrics
instances_total = Counter('themisdb_instances_total', 'Total ThemisDB instances')
active_nodes = Gauge('themisdb_active_nodes', 'Number of active nodes', ['instance_id'])
query_duration = Histogram('themisdb_query_duration_seconds', 'Query duration')

# Record metrics
instances_total.inc()
active_nodes.labels(instance_id='inst-123').set(5)
query_duration.observe(0.5)
```

---

### 3. **StatsD / Telegraf**
- **Python Library**: `statsd`, `python-statsd`
- **Description**: Simple daemon for stats aggregation
- **Pros**:
  - Very lightweight
  - UDP-based (fire-and-forget)
  - Low overhead
  - Easy to integrate
- **Cons**:
  - Limited built-in features
  - Requires separate infrastructure
  - UDP = potential data loss

**Installation:**
```bash
pip install statsd
```

---

### 4. **Custom HTTP REST API** (IMPLEMENTED) ⭐⭐⭐
- **Description**: Simple REST API for metric submission
- **Pros**:
  - Full control over data format
  - No external dependencies
  - Easy integration with existing FastAPI server
  - Works with firewalls (HTTPS)
  - Simple for clients to implement
- **Cons**:
  - Need to build backend yourself
  - Manual scaling considerations

**Our Implementation:**
```python
# ThemisDB instance sends metrics via HTTP POST
POST https://service.themisdb.org:6734/telemetry/heartbeat
{
  "license_key": "THEMIS-ENT-...",
  "instance_id": "unique-instance-id",
  "hostname": "prod-db-01",
  "version": "1.5.0",
  "metrics": {
    "nodes": 5,
    "total_cores": 80,
    "used_storage_tb": 2.5,
    "uptime_seconds": 86400,
    "query_count_24h": 1500000
  }
}
```

---

### 5. **Sentry** (For Error Tracking)
- **Website**: https://sentry.io/
- **Python Library**: `sentry-sdk`
- **Description**: Error tracking and performance monitoring
- **Pros**:
  - Excellent error tracking
  - Performance monitoring
  - Hosted or self-hosted
- **Cons**:
  - Focused on errors, not custom metrics
  - Can be expensive at scale

---

### 6. **InfluxDB / InfluxData**
- **Python Library**: `influxdb-client`
- **Description**: Time-series database optimized for metrics
- **Pros**:
  - Purpose-built for time-series data
  - SQL-like query language
  - Good performance
- **Cons**:
  - Separate infrastructure
  - Complexity

---

## Recommended Approach for ThemisDB

### **Hybrid Solution** (Best of Both Worlds)

1. **Primary: Custom REST API** (Already implementing)
   - Simple HTTP POST to `/telemetry/heartbeat`
   - Stores data in PostgreSQL
   - Minimal client-side complexity
   - Works through firewalls

2. **Optional: OpenTelemetry Export** (Future)
   - For customers who want advanced monitoring
   - Can export to their own Prometheus/Grafana
   - Standardized format

### Architecture

```
┌─────────────────────┐
│  ThemisDB Instance  │
│   (Client-side)     │
└──────────┬──────────┘
           │ HTTPS POST every 5 min
           ↓
┌─────────────────────────────┐
│  service.themisdb.org:6734  │
│  POST /telemetry/heartbeat  │
└──────────┬──────────────────┘
           │ Store in PostgreSQL
           ↓
┌─────────────────────┐
│  Telemetry Table    │
│  - instance_id      │
│  - metrics          │
│  - timestamps       │
└─────────────────────┘
           │
           ↓
┌─────────────────────┐
│  Analytics/Reports  │
│  - Active instances │
│  - Global usage     │
│  - Trends           │
└─────────────────────┘
```

---

## Implementation Details

### Client-Side (C++ for ThemisDB)

**Option 1: libcurl (Simple REST)**
```cpp
#include <curl/curl.h>

void send_telemetry() {
    CURL* curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://service.themisdb.org:6734/telemetry/heartbeat");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "{\"license_key\":\"...\",\"metrics\":{...}}");
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
}
```

**Option 2: C++ HTTP Client Libraries**
- **cpp-httplib** (header-only, simple)
- **Beast** (Boost.Asio-based)
- **cpr** (libcurl wrapper, modern C++)

### Server-Side (FastAPI - Already Implementing)

See `routers/telemetry.py` and `services/telemetry_service.py`

---

## Privacy & Security Considerations

1. **Minimal Data Collection**
   - Only essential metrics (version, node count)
   - No sensitive customer data
   - No query content or results

2. **HTTPS Only**
   - All telemetry over encrypted connection
   - Certificate validation required

3. **Rate Limiting**
   - Max 1 report per 5 minutes per instance
   - Prevent abuse/flooding

4. **Opt-Out Option**
   - Config flag: `telemetry_enabled=false`
   - Respect customer privacy

5. **Data Retention**
   - Keep only last 90 days
   - Aggregate older data
   - Regular cleanup

---

## Summary

**For ThemisDB, we recommend:**

✅ **Custom REST API** (Primary solution)
- Simple, reliable, works everywhere
- Full control over data and privacy
- Easy to implement in C++
- Already have FastAPI server

✅ **Future: OpenTelemetry export** (Optional)
- For advanced customers
- Standard format for external monitoring

**Client Libraries to Use:**
- **C++**: libcurl, cpp-httplib, or cpr
- **Python**: requests (built-in)
- **Go**: net/http (standard library)

**No external dependencies required!** The custom REST API approach is the cleanest solution for this use case.
