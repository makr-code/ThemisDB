[docs](../../index.md) > [de](../index.md) > [core](./index.md) > [PRIMARY_SOURCES](./PRIMARY_SOURCES.md)
**Datum:** 2026-05-31
**Status:** current
**Primary (Quelle der Wahrheit):**
- `src/core/README.md`
- `src/core/ARCHITECTURE.md`
- `src/core/ROADMAP.md`
- `src/core/FUTURE_ENHANCEMENTS.md`
- `src/core/MODULE_GAPS.md`
- `src/core/SECURITY.md`
- `src/core/AUDIT.md`
- `src/core/PERFORMANCE_EXPECTATIONS.md`
- `src/core/PRODUCTION_REQUIREMENTS.md`
- `src/core/CHANGELOG.md`

**Bezug / Reference:**
- Inventory-Baseline: `ai_working/developer_docs_inventory_report.md`
- Alignment-Baseline: `ai_working/docs_module_alignment_report_2026-05-31.md`
- Kontext: Modulabgleich docs gegen Planung, wobei neuere Planungsdokumente hoeher gewichtet werden als alte Reports.

---

# Primary Sources - core

Diese Seite definiert, welche `src/core/`-Dokumente fuer Planung und Verhaltensabgleich massgeblich sind.

## Abgleichsregeln

- Neuere Dokumente sind relevanter als aeltere Dokumente.
- `FUTURE_ENHANCEMENTS.md` und `MODULE_GAPS.md` bilden den primaeren Workload-Input.
- Historische Implementierungsreports in `docs/` sind sekundaer, sofern sie nicht juenger und source-verifiziert sind.

## Massgebliche Planungsquellen

| Datei | Rolle |
|---|---|
| `src/core/FUTURE_ENHANCEMENTS.md` | Zielverhalten, Constraints und geplante Schnittstellen |
| `src/core/MODULE_GAPS.md` | aktueller Gap-Workload und Severity-Snapshot |
| `src/core/ROADMAP.md` | phasenbezogene Umsetzung und Milestone-Status |

## Massgebliche Runtime- und Governance-Quellen

| Datei | Rolle |
|---|---|
| `src/core/ARCHITECTURE.md` | Laufzeitarchitektur und Abhaengigkeitsgrenzen |
| `src/core/SECURITY.md` | Security-Kontrollen und Restriktionen |
| `src/core/AUDIT.md` | Verifikations- und Evidenzstatus |
| `src/core/PERFORMANCE_EXPECTATIONS.md` | Benchmark-Vertraege und Performance-Gates |
| `src/core/PRODUCTION_REQUIREMENTS.md` | Produktionsreife-Anforderungen |
| `src/core/CHANGELOG.md` | Aenderungs-Traceability |

## Sekundaere Referenzen (nicht fuehrend)

- `include/core/*`-Markdown-Dateien
- historische Dokumente unter `docs/` (z. B. archivierte Reports)

Diese Quellen sind Kontext, aber nicht die Fuehrungsquelle fuer den aktuellen Workload-Status.

---

*Aktualisiert im docs-vs-planungs Alignment-Sweep am 2026-05-31.*
