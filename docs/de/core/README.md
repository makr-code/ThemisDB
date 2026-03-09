# Core-Modul

**Stand:** 9. März 2026  
**Version:** 1.0  
**Kategorie:** Querschnittsfunktionen / Dependency Injection  
**Validated:** 2026-03-09 (Reality-Check gegen Sourcecode; siehe [missing-implementations.md](missing-implementations.md))

---

## Übersicht

Das Core-Modul stellt die grundlegende Cross-Cutting-Concerns-Infrastruktur für ThemisDB bereit. Es implementiert ein Dependency-Injection-Framework, das pluggable Implementierungen von Logging, Tracing, Metriken, Caching, Secrets und Feature-Flags im gesamten Datenbank-Engine ermöglicht.

**Wichtigste Eigenschaften:**

- `ConcernsContext` als zentraler DI-Hub für alle Querschnittsfunktionen
- Pluggable Adapter-Architektur — Laufzeit-Austausch ohne Rekompilierung (Konfiguration)
- Produktionssichere Standard-Implementierungen mit Umgebungserkennung
- Vollständiges W3C-TraceContext-Propagationsprotokoll
- Circuit-Breaker-Schutz für alle Adapter-Aufrufe

---

## Source-Code Referenz

### Implementierung (`src/core/`)

| Datei / Komponente | Rolle |
|---|---|
| `concerns/concerns_context.cpp` | Zentrale DI-Implementierung und Factory-Methoden |
| `adapters/otel_tracer.cpp` | OpenTelemetry-Tracer-Adapter (OTLP-Export) |
| `adapters/spdlog_logger.cpp` | Spdlog-Logger-Adapter |
| `adapters/prometheus_metrics.cpp` | Prometheus-Metriken-Adapter |
| `adapters/context_propagation.cpp` | W3C-TraceContext-Propagation |
| `security_initialization.cpp` | Kryptografische Initialisierungsroutinen |

### Öffentliche Header (`include/core/`)

| Header | Rolle |
|---|---|
| `concerns/concerns_context.h` | `ConcernsContext` — zentraler DI-Hub; Factory-Methoden |
| `concerns/i_logger.h` | Logging-Interface mit Severity-Stufen und strukturiertem Logging |
| `concerns/i_tracer.h` | Distributed-Tracing-Interface; Span-Management |
| `concerns/i_metrics.h` | Metriken-Interface (Counter, Gauge, Histogram) |
| `concerns/i_cache.h` | Cache-Interface mit pluggable Eviction-Strategien |
| `concerns/i_secrets.h` | Secrets-Interface für Credential-Injektion |
| `concerns/i_feature_flags.h` | Feature-Flag-Interface (inkl. `InMemoryFeatureFlags`) |
| `concerns/i_circuit_breaker.h` | Circuit-Breaker-Interface für Adapter-Resilienz |
| `concerns/lifecycle.h` | `ProbeResult`/`HealthStatus` für Health-Check-Aggregation |
| `concerns/spdlog_logger_adapter.h` | `SpdlogLoggerAdapter` — spdlog-Integration |
| `concerns/otel_tracer_adapter.h` | `OpenTelemetryTracerAdapter` — OTLP-Exporter |
| `concerns/jaeger_tracer_adapter.h` | `JaegerTracerAdapter` — Jaeger-Backend |
| `concerns/zipkin_tracer_adapter.h` | `ZipkinTracerAdapter` — Zipkin-Backend |
| `concerns/prometheus_metrics_adapter.h` | `PrometheusMetricsAdapter` |
| `concerns/inmemory_cache_impl.h` | `InMemoryCacheImpl` — lokaler In-Process-Cache |
| `concerns/strategic_cache_impl.h` | `StrategicCacheImpl` — Cache mit pluggable Eviction |
| `concerns/noop_implementations.h` | Noop-Implementierungen für Test-Pfade |
| `concerns/w3c_trace_context_propagator.h` | `W3CTraceContextPropagator` — `traceparent`/`tracestate` |
| `concerns/context_propagation.h` | IContext-basierte Header-Extraktion/-Injektion |
| `concerns/CACHE_STRATEGIES_README.md` | Detaillierte Beschreibung aller Eviction-Strategien |
| `config_validator.h` | Konfigurationsvalidierung für alle Adapter-Typen |
| `production_mode.h` | Umgebungserkennung (`THEMIS_PRODUCTION_MODE`) |
| `query_engine_builder.h` | Builder für Query-Engine mit DI-Kontext |
| `storage_initialization.h` | Storage-Initialisierung mit ConcernsContext |
| `index_initialization.h` | Index-Initialisierung mit ConcernsContext |
| `security_initialization.h` | Sicherheits-Initialisierung |

---

## Factory-Methoden

| Methode | Verwendungszweck |
|---|---|
| `ConcernsContext::create()` | Standard-Produktionskonfiguration |
| `ConcernsContext::create(const Config&)` | Konfigurationsgesteuerte Adapter-Auswahl |
| `ConcernsContext::createNoOp()` | Testfreundliche No-Op-Konfiguration |
| `ConcernsContext::createCustom(...)` | Benutzerdefinierte Konfiguration mit eigenen Adaptern |

---

## Adapter-Übersicht

| Concern | Produktions-Adapter | Noop-Adapter |
|---|---|---|
| Logging | `SpdlogLoggerAdapter` | `NoopLogger` |
| Tracing | `OpenTelemetryTracerAdapter`, `JaegerTracerAdapter`, `ZipkinTracerAdapter` | `NoopTracer` |
| Metriken | `PrometheusMetricsAdapter` | `NoopMetrics` |
| Caching | `InMemoryCacheImpl`, `StrategicCacheImpl` | `NoopCache` |
| Secrets | *(konfigurierbar)* | `NoopSecrets` |
| Feature-Flags | `InMemoryFeatureFlags` | `NoOpFeatureFlags` |

---

## Laufzeitverhalten

### Initialisierung

```cpp
// Produktions-Kontext mit Konfiguration
auto context = ConcernsContext::create(config);

// Testfreundlicher No-Op-Kontext
auto test_context = ConcernsContext::createNoOp();

// Vollständig benutzerdefinierter Kontext
auto context = ConcernsContext::createCustom(
    std::make_unique<SpdlogLoggerAdapter>(),
    std::make_unique<OpenTelemetryTracerAdapter>(otel_config),
    std::make_unique<PrometheusMetricsAdapter>(),
    std::make_unique<InMemoryCacheImpl>()
);
```

### Adapter-Zugriff

```cpp
context->logger().info("Datenbankstart");
auto span = context->tracer().startSpan("query_execution");
context->metrics().incrementCounter("requests_total");
if (auto cached = context->cache().get("query_result:123")) {
    return *cached;
}
```

### Health-Check

```cpp
auto health = context->healthCheck();    // alle Concerns
auto ready  = context->readinessCheck(); // Produktionsbereitschaft
```

### W3C-TraceContext-Propagation

```cpp
// Eingehende Anfragen (Extraktion)
auto span = context->tracer().startSpanFromHeaders(http_headers);

// Ausgehende Anfragen (Injektion)
context->tracer().injectContext(outgoing_headers);
```

---

## Build-Konfiguration

```cmake
# Minimal-Build (nur Noop-Implementierungen)
-DTHEMIS_BUILD_TYPE=MINIMAL

# Community-Build (spdlog, grundlegende Observability)
-DTHEMIS_BUILD_TYPE=COMMUNITY

# Enterprise-Build (vollständiges OTEL, Prometheus, verteiltes Tracing)
-DTHEMIS_BUILD_TYPE=ENTERPRISE
```

### Umgebungsvariablen

| Variable | Wert | Bedeutung |
|---|---|---|
| `THEMIS_PRODUCTION_MODE` | `1` | Produktionsmodus aktivieren |
| `THEMIS_ENVIRONMENT` | `production` | Alternative Produktionsmodus-Aktivierung |

---

## Performance-Ziele

| Metrik | Zielwert |
|---|---|
| `ConcernsContext::resolve<T>()` (32-Thread-Contention) | ≤ 1 µs Median, ≤ 10 µs p99 |
| Adapter Hot-Swap (Register + Drain + Replace) | ≤ 100 ms |
| Server-Start (32 Adapter) | DI-Kontext-Aufbau ≤ 50 ms |
| Circuit-Breaker `call()` (Closed-Zustand) | ≤ 200 ns |
| Noop-Adapter-Overhead | < 1 ns (Compiler-optimiert) |
| Spdlog-Adapter | ~50–100 ns pro Log-Aufruf (Async-Modus) |

---

## Sicherheit

- **Keine Credentials im Kontext**: `ConcernsContext` speichert keine Secrets oder Credentials direkt — `ISecrets`-Adapter abstrahiert Credential-Zugriff
- **RAII-Handles**: Kein roher Pointer-Austausch über Modulgrenzen; dangling-Adapter-Zugriff durch Design unmöglich
- **Circuit-Breaker**: Verhindert kaskadierende Fehler bei Adapter-Ausfällen
- **Produktionsmodus-Erkennung**: Verhindert unsichere Default-Implementierungen in der Produktion

---

## Verwandte Dokumentation

### Primärdokumentation (Quellcode)

- [README (src/core)](../../../src/core/README.md) — Modulübersicht und Entwicklerleitfaden
- [ARCHITECTURE (src/core)](../../../src/core/ARCHITECTURE.md) — Architektur-Leitfaden und Komponentendiagramm
- [ROADMAP (src/core)](../../../src/core/ROADMAP.md) — Entwicklungs-Roadmap und Produktionsreife-Checkliste
- [FUTURE_ENHANCEMENTS (src/core)](../../../src/core/FUTURE_ENHANCEMENTS.md) — Geplante Features mit Performance-Zielen
- [Cache Strategies (include/core)](../../../include/core/concerns/CACHE_STRATEGIES_README.md) — Detaillierte Eviction-Strategie-Dokumentation

### Reality-Check & Offene Implementierungen

- [missing-implementations.md](missing-implementations.md) — Reality-Check-Bericht: fehlende/unvollständige Implementierungen (Stand 2026-03-09)
- [missing-implementations.json](missing-implementations.json) — Maschinenlesbares Format des obigen Berichts
