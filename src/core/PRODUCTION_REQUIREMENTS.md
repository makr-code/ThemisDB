> **Status:** 2026-05-13 – mit aktuellem Core-Code (`production_mode.h`, `security_initialization.cpp`, `concerns_context.cpp`) abgeglichen.

# ThemisDB Core Module - Production Requirements

## Zweck und Geltungsbereich

Dieses Dokument ist der **kanonische Referenzpunkt für produktive Mindestanforderungen** des Core-Moduls.
Es definiert nur verbindliche Betriebs- und Sicherheitsanforderungen, keine Architektur- oder Roadmap-Planung.

## Dokumentabgrenzung (Canonical Split)

- **`src/core/PRODUCTION_REQUIREMENTS.md` (dieses Dokument):** verpflichtende Produktionsanforderungen (MUST/MUST NOT), Sicherheitsannahmen, Betriebsgrenzen.
- **`src/core/README.md`:** Funktionsübersicht, Architekturkontext, API- und Nutzungsbeispiele.
- **`src/core/ROADMAP.md`:** Lieferphasen, offene/abgeschlossene Features, Readiness-Planung.
- **`src/core/FUTURE_ENHANCEMENTS.md`:** mittelfristige/langfristige Erweiterungen und Forschungsfelder.

## Produktionsmodus (verbindlich)

Der Produktionsmodus gilt als aktiv, wenn mindestens eine Bedingung erfüllt ist:

- `THEMIS_PRODUCTION_MODE` ist einer von: `1`, `true`, `True`, `TRUE`, `yes`, `Yes`, `on`, `On`
- `THEMIS_ENVIRONMENT` ist einer von: `production`, `prod`

## Verbindliche Sicherheitsanforderungen

### 1) SecurityLayerBuilder

Bei aktivem Produktionsmodus gilt:

- **MUST:** `withKeyProvider()` mit `VAULT` oder `HSM` verwenden
- **MUST NOT:** `LOCAL`-Key-Provider verwenden
- **MUST:** JWT-Validierung via `withJWT(...)` konfigurieren
- **MUST:** fehlerhafte Vault/JWT-Konfigurationen als Fehler behandeln (Fail Closed)

Zusätzliche Betriebsbedingung:

- **HSM:** nur erlaubt, wenn `THEMIS_HSM_ENABLED=1` gesetzt ist

### 2) ConcernsContext

Bei aktivem Produktionsmodus gilt:

- **MUST:** Tracing aktiviert (`tracingEnabled=true`) **oder** produktiver Tracer-Adapter (`otel`, `jaeger`, `zipkin`)
- **MUST:** wenn `tracerAdapter` leer/Default ist, `tracingEnabled=true`; bei expliziter Adapterwahl muss `tracerAdapter` einer von `otel`, `jaeger`, `zipkin` sein
- **MUST:** wenn `metricsAdapter` leer/Default ist, `metricsEnabled=true`; bei expliziter Adapterwahl muss `metricsAdapter="prometheus"` sein
- **MUST NOT:** `ConcernsContext::createNoOp()` verwenden

## Betriebsgrenzen (aktuelles Core-Verhalten)

- `maxMetricCardinality` ist standardmäßig auf `1000` Label-Kombinationen pro Metrik begrenzt.
- Der In-Memory-Cache ist durch `cacheMaxSize` begrenzt; `cacheDefaultTTL=0` bedeutet „kein TTL-Ablauf".
- `ConcernsContext` ist prozesslokal; verteilte Koordination (z. B. global konsistente Cache-Invalidierung) benötigt externe Systeme.

## Minimaler Produktions-Check (Audit-fähig)

- [ ] Produktionsmodus ist in Deployment-Konfiguration explizit gesetzt
- [ ] SecurityLayerBuilder nutzt `VAULT` oder `HSM` (kein `LOCAL`)
- [ ] JWT-Validierung ist konfiguriert
- [ ] Tracing ist produktiv aktiviert (`otel`/`jaeger`/`zipkin`)
- [ ] Metrics sind produktiv aktiviert (`prometheus`)
- [ ] `createNoOp()` wird in Produktionspfaden nicht verwendet
- [ ] HSM-Einsatz ist mit `THEMIS_HSM_ENABLED=1` und PKCS#11-Setup abgesichert

## Review / Sourcecode-Audit-Nachweis

**Review- und Audit-Status (Issue: „[Docs][Module] core - PRODUCTION_REQUIREMENTS.md aktualisieren")**

- [x] Fachreview durchgeführt (Dokumentabgleich gegen aktuelle Modulquellen)
- [x] Sourcecode-/Dokumentationsaudit durchgeführt
- [x] Ergebnis verlinkt
- [x] Betroffene Dateien im Review festgehalten

### Review-/Audit-Ergebnis (verlinkte Nachweise)

- Dokumentationsreview-Leitlinie: [`docs/DOCUMENTATION_REVIEW_GUIDELINES.md`](../../docs/DOCUMENTATION_REVIEW_GUIDELINES.md)
- Systematischer Review-Plan: [`docs/SYSTEMATISCHER_REVIEWPLAN.md`](../../docs/SYSTEMATISCHER_REVIEWPLAN.md)
- Source-Code-Audit-Referenz: [`docs/de/development/SOURCE_CODE_AUDIT.md`](../../docs/de/development/SOURCE_CODE_AUDIT.md)
- Audit-Runbook: [`docs/audit-framework/AUDIT_RUNBOOK.md`](../../docs/audit-framework/AUDIT_RUNBOOK.md)
- Link-Validierung der Referenzen: `python3 scripts/link-check.py --internal-only src/core/PRODUCTION_REQUIREMENTS.md` (durchgeführt am 2026-05-13)

### Betroffene Dateien im Review

- `src/core/PRODUCTION_REQUIREMENTS.md`
- `src/core/README.md`
- `src/core/ROADMAP.md`
- `src/core/FUTURE_ENHANCEMENTS.md`
- `include/core/production_mode.h`
- `include/core/security_initialization.h`
- `src/core/security_initialization.cpp`
- `src/core/concerns/concerns_context.cpp`
