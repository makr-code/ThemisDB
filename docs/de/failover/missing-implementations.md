[docs](../../index.md) > [de](../index.md) > [failover](./README.md) > [missing-implementations](./missing-implementations.md)
**Datum:** 2026-04-16
**Status:** review
**Primary (Quelle der Wahrheit):**
- `src/failover/ROADMAP.md`
- `include/failover/ROADMAP.md`
- `src/failover/FUTURE_ENHANCEMENTS.md`
- `include/failover/FUTURE_ENHANCEMENTS.md`
- `src/failover/auto_failover_manager.cpp`
- `src/failover/disaster_recovery_manager.cpp`

**Bezug / Reference:**
- Issue: `[MODULE] failover`
- Kontext: Verifizierter Missing-Implementations-Report nach Reality-Check des Failover-Moduls.

---

# Failover-Modul — Missing Implementations

## Zusammenfassung

| ID | Quelle | Lücke | Impact | Priorität |
|----|--------|-------|--------|-----------|
| FO-MI-01 | `src/failover/ROADMAP.md` | Cross-Region-Traffic-Manager-Integration offen | Erhöhtes manuelles Ops-Risiko bei regionalen Ausfällen | Hoch |
| FO-MI-02 | `include/failover/ROADMAP.md` | Include-boundary API/ABI-Kompatibilitätsprüfungen offen | Risiko unbemerkter Schnittstellen-/ABI-Regressionen | Mittel |
| FO-MI-03 | `src/failover/ROADMAP.md` | Multi-Region-Soak-Tests offen | Geringere Aussagekraft zur Langzeitstabilität unter Last | Mittel |
| FO-MI-04 | `src/failover/FUTURE_ENHANCEMENTS.md` | Pluggable Policy Evaluator für Promotion-Selektion fehlt | Begrenzte Steuerbarkeit komplexer Failover-Policies | Mittel |
| FO-MI-05 | `src/failover/FUTURE_ENHANCEMENTS.md` | Erweiterter Per-Step-Metrikexport fehlt | Eingeschränkte Ursachenanalyse bei Recovery-Latenz | Niedrig |

## Detailnachweise

### FO-MI-01 — Cross-Region-Traffic-Manager-Integration

- **Evidence**
  - Offener Roadmap-Punkt: `src/failover/ROADMAP.md` (`In Progress`)
  - Kein entsprechender Integrationscode in `src/failover/*.cpp` (nur Roadmap-Referenz auffindbar).
- **Impact**
  - Bei regionenübergreifenden Failovern bleibt Traffic-Umschaltung stärker von externer manueller Orchestrierung abhängig.
- **Folge-Issue (Vorschlag)**
  - Titel: `[failover] Integrate cross-region traffic manager into failover orchestration`
  - Labels: `area:failover`, `type:feature`, `priority:high`

### FO-MI-02 — Include-boundary API/ABI-Kompatibilitätsprüfungen

- **Evidence**
  - Offene Punkte in `include/failover/ROADMAP.md`:
    - „Add include-boundary API compatibility tests“
    - „ABI compatibility checks across compilers“
  - Keine dedizierten ABI/API-Kompatibilitätstests unter `tests/*failover*`.
- **Impact**
  - Header-Änderungen in `include/failover/*` können ABI-/Kompatibilitätsrisiken verursachen, die in regulären Funktions-Tests nicht zwingend auffallen.
- **Folge-Issue (Vorschlag)**
  - Titel: `[failover] Add include-boundary API and ABI compatibility checks`
  - Labels: `area:failover`, `type:test`, `priority:medium`

### FO-MI-03 — Multi-Region-Soak-Tests

- **Evidence**
  - Offener Readiness-Punkt: `src/failover/ROADMAP.md` („Extended multi-region failover soak tests“).
  - Vorhandene Tests decken funktionale/chaotische Szenarien ab, aber keinen dedizierten Multi-Region-Soak-Langlauftest.
- **Impact**
  - Performance-/Stabilitätsaussagen unter langlaufenden, regionenübergreifenden Störmustern bleiben begrenzt.
- **Folge-Issue (Vorschlag)**
  - Titel: `[failover] Add extended multi-region failover soak test suite`
  - Labels: `area:failover`, `type:test`, `priority:medium`

### FO-MI-04 — Pluggable Policy Evaluator

- **Evidence**
  - Als Implementierungsnotiz in `src/failover/FUTURE_ENHANCEMENTS.md` geführt.
  - Kein Policy-Evaluator-Hook in `include/failover/auto_failover_manager.h`.
- **Impact**
  - Begrenzte Erweiterbarkeit für domänenspezifische Promotion-Entscheidungen.
- **Folge-Issue (Vorschlag)**
  - Titel: `[failover] Add pluggable policy evaluator for promotion selection`
  - Labels: `area:failover`, `type:feature`, `priority:medium`

### FO-MI-05 — Per-Step-Metrikexport

- **Evidence**
  - Als offener Punkt in `src/failover/FUTURE_ENHANCEMENTS.md` genannt.
  - Vorhanden sind Basistelemetrie und Retry-Zähler (`AutoFailoverManager::Statistics`), aber kein dedizierter Export pro DR-Schritt.
- **Impact**
  - Forensik und Tuning von Recovery-Laufzeiten sind nur eingeschränkt granular.
- **Folge-Issue (Vorschlag)**
  - Titel: `[failover] Export per-step disaster-recovery timing and retry metrics`
  - Labels: `area:failover`, `type:observability`, `priority:low`
