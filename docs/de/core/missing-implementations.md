# Core Module — Missing Implementations Report

**Validiert:** 2026-03-09  
**Geprüfte Revision:** `HEAD` (`copilot/inventory-check-acceleration-module`)  
**Geprüfte Pfade:** `src/core/`, `include/core/`  
**Methode:** Reality-Check (Doku ↔ Sourcecode); Suche nach `STUB`, `TODO`, `NOT_IMPLEMENTED`; Überprüfung aller ROADMAP-Claims gegen vorhandene Symbole

---

## Zusammenfassung

| Schweregrad | Anzahl |
|---------|--------|
| 🔴 Kritisch (Produktionsblocker) | 0 |
| 🟡 Mittel (Funktional eingeschränkt) | 1 |
| 🟢 Gering (Hardening / Optimierung) | 2 |

---

## Fehlende / Unvollständige Implementierungen

### 1. Plugin-basiertes Adapter-Laden (🟡 Mittel)

**Claim-Quelle:** `src/core/ROADMAP.md` → Phase 3, Issue: #1706  
**Datei:** `include/core/concerns/` (kein Plugin-Loader-Header vorhanden)

**Erwartet:** Laufzeit-Laden von Adaptern aus Shared Libraries ohne Rekompilierung (Phase 3, `[I]`).

**Beobachtet:** Kein Plugin-Loader für Core-Adaptern gefunden. Aktuell müssen alle Adapter zur Kompilierzeit ausgewählt und verlinkt werden. Die Konfigurationsauswahl zwischen `spdlog`/`noop` etc. erfolgt via `ConcernsContext::Config::loggerAdapter`-Strings, aber das eigentliche dynamische Laden (z.B. `dlopen`) ist nicht implementiert.

**Evidence:**
- `include/core/concerns/` enthält keine Plugin-Loader- oder `dlopen`-Wrapper
- `src/core/` enthält keine dynamische Lader-Logik
- ROADMAP Phase 3 korrekt als `[I]` (Issue: #1706) markiert ✅

**Issue-Titelvorschlag:** `feat(core): plugin-based adapter loading at runtime (Issue: #1706)`  
**Label-Vorschläge:** `module:core`, `kind:feature`, `priority:medium`

---

### 2. Dynamische Log-Level-Anpassung zur Laufzeit (🟢 Gering)

**Claim-Quelle:** `src/core/ROADMAP.md` → Planned Features Short-term, Issue: #1412  
**Datei:** `include/core/concerns/i_logger.h`, `include/core/concerns/spdlog_logger_adapter.h`

**Erwartet:** Log-Level kann zur Laufzeit ohne Neustart geändert werden.

**Beobachtet:** `ILogger` definiert kein `setLevel()`-Methode; `SpdlogLoggerAdapter` exponiert keine Methode zum dynamischen Setzen des Log-Levels. Die ROADMAP markiert dies korrekt als offen (`[I]`).

**Evidence:**
- `include/core/concerns/i_logger.h`: keine `setLevel()`-Methode in `ILogger`
- `include/core/concerns/spdlog_logger_adapter.h`: kein dynamisches Level-Setter
- ROADMAP Short-term: `[I]` für #1412 — korrekt ✅

**Issue-Titelvorschlag:** `feat(core): dynamic log level adjustment at runtime (Issue: #1412)`  
**Label-Vorschläge:** `module:core`, `kind:feature`, `priority:low`

---

### 3. Audit-Event-Interface für Compliance-Logging (🟢 Gering)

**Claim-Quelle:** `src/core/ROADMAP.md` → Planned Features Long-term, Issue: #1418  
**Datei:** `include/core/concerns/` (kein `i_audit.h` vorhanden)

**Erwartet:** `IAuditEvent`-Interface für Compliance-relevantes Event-Logging (Long-term, `[I]`).

**Beobachtet:** Kein `IAuditEvent`-Interface oder entsprechender Adapter in `include/core/concerns/`. Die ROADMAP markiert dies korrekt als offenes Long-term-Feature (`[I]`).

**Evidence:**
- `include/core/concerns/`: kein `i_audit.h` oder ähnliches
- ROADMAP Long-term: `[I]` für #1418 — korrekt ✅

**Issue-Titelvorschlag:** `feat(core): audit event interface for compliance logging (Issue: #1418)`  
**Label-Vorschläge:** `module:core`, `kind:feature`, `priority:low`

---

## Positiv verifizierte Claims

Die folgenden ROADMAP- und README-Claims wurden überprüft und als korrekt befunden:

| Claim | Evidence |
|-------|----------|
| `ConcernsContext` Factory-Methoden: `create()`, `create(Config)`, `createNoOp()`, `createCustom()` | `include/core/concerns/concerns_context.h` Zeilen 125, 126, 136–172 ✅ |
| `SpdlogLoggerAdapter` | `include/core/concerns/spdlog_logger_adapter.h`: `class SpdlogLoggerAdapter : public ILogger` ✅ |
| `OpenTelemetryTracerAdapter` | `include/core/concerns/otel_tracer_adapter.h`: `class OpenTelemetryTracerAdapter : public ITracer` ✅ |
| `JaegerTracerAdapter` | `include/core/concerns/jaeger_tracer_adapter.h` ✅ |
| `ZipkinTracerAdapter` | `include/core/concerns/zipkin_tracer_adapter.h` ✅ |
| `PrometheusMetricsAdapter` | `include/core/concerns/prometheus_metrics_adapter.h` ✅ |
| `InMemoryCacheImpl` | `include/core/concerns/inmemory_cache_impl.h` ✅ |
| `StrategicCacheImpl` | `include/core/concerns/strategic_cache_impl.h` ✅ |
| `IFeatureFlags` + `InMemoryFeatureFlags` | `include/core/concerns/i_feature_flags.h` ✅ |
| `ISecrets` Interface | `include/core/concerns/i_secrets.h` ✅ |
| `ProbeResult`/`HealthStatus` in `lifecycle.h` | `include/core/concerns/lifecycle.h` ✅ |
| `W3CTraceContextPropagator` | `include/core/concerns/w3c_trace_context_propagator.h` ✅ |
| `ICircuitBreaker` | `include/core/concerns/i_circuit_breaker.h` ✅ |
| Noop-Implementierungen | `include/core/concerns/noop_implementations.h` ✅ |
| `ConfigValidator` | `include/core/config_validator.h` ✅ |
| Production-Mode-Erkennung | `include/core/production_mode.h` ✅ |
| `ROADMAP` Current Status: Production Ready | `include/core/concerns/concerns_context.h` Header: `Status: ✅ Production Ready` ✅ |

---

## Behobene Dokumentationsdrift (in diesem PR)

| Drift | Beobachtet | Behoben |
|-------|-----------|---------|
| ROADMAP-Factory-Methoden-Namen | `createForProduction()` / `createForTesting()` im ROADMAP | Korrigiert auf `create()` / `createNoOp()` ✅ |
| ROADMAP Long-term: Secrets-Interface | `[I]` für #1417 obwohl bereits in Phase 3 `[x]` | Aus Long-term entfernt ✅ |
| ROADMAP Phase 3: Doppelter Eintrag | „Structured log correlation" doppelt eingetragen | Duplikat entfernt ✅ |
| ROADMAP Current Status | „Beta" obwohl Production Ready | Auf „Production Ready" korrigiert ✅ |
| README-Links | Broken Links auf nicht-existierende Docs-Pfade | Entfernt ✅ |
