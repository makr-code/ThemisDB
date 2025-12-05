# Observability Documentation

**Stand:** 5. Dezember 2025  
**Version:** 1.0.0  
**Kategorie:** Observability

---

## Übersicht

ThemisDB bietet umfassende Observability mit Metrics, Tracing und Logging.

## Features

| Feature | Implementierung | Status |
|---------|-----------------|--------|
| **Metrics** | Prometheus | ✅ Production |
| **Tracing** | OpenTelemetry | ✅ Production |
| **Logging** | spdlog | ✅ Production |
| **Health Checks** | `/health` Endpoint | ✅ Production |

## Prometheus Metrics

```
# HELP themisdb_requests_total Total number of requests
# TYPE themisdb_requests_total counter
themisdb_requests_total{method="GET",endpoint="/api/query"} 12345

# HELP themisdb_request_duration_seconds Request duration
# TYPE themisdb_request_duration_seconds histogram
themisdb_request_duration_seconds_bucket{le="0.1"} 9876
```

## OpenTelemetry Tracing

```cpp
#include "utils/tracing.h"

auto span = tracer->StartSpan("query_execute");
span->SetAttribute("aql.query", query);
// ... execute query ...
span->End();
```

## Source-Code Referenz

| Komponente | Header | Source |
|------------|--------|--------|
| Metrics | `include/utils/metrics.h` | `src/utils/metrics.cpp` |
| Tracing | `include/utils/tracing.h` | `src/utils/tracing.cpp` |
| Logger | `include/utils/logger.h` | `src/utils/logger.cpp` |

## Dokumentation in diesem Ordner

| Datei | Beschreibung |
|-------|--------------|
| [observability_metrics.md](observability_metrics.md) | Metrics Overview |
| [observability_prometheus.md](observability_prometheus.md) | Prometheus Integration |
| [observability_tracing.md](observability_tracing.md) | OpenTelemetry Tracing |
| [observability_phase6_complete.md](observability_phase6_complete.md) | Phase 6 Status |

## Verwandte Dokumentation

- [Server Module](../server/README.md) - HTTP Server
- [Enterprise Features](../enterprise/README.md) - Enterprise Monitoring
