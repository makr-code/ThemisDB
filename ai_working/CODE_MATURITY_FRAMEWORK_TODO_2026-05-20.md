# Code Maturity Framework - Konsolidierte TODO

Date: 2026-05-20
Owner: Code Quality / Tooling

## 1) Ist-Zustand (verifiziert)

### A. CI-basierter Maturity-Flow

- Workflow: .github/workflows/08-maintenance_code-maturity.yml
- Script: .github/scripts/analyze_code_maturity.py
- Output-Vertrag:
  - docs/code_maturity_report.md (Markdown report)
  - .github/version_tracking.json
  - .github/badges/*.json
- Trigger:
  - schedule (weekly)
  - workflow_dispatch (check-only vs rewrite)
  - PR nur auf enges Pfadset

### B. Lokaler Gap-Audit-Flow (v2)

- Pipeline: tools/complete_gap_audit.py -> tools/gap_audit_pipeline_v2.py
- Output-Vertrag:
  - ai_working/gap_scan_v2_*.json
  - ai_working/gap_scan_v2_summary.json
  - ai_working/module_gaps/*
- Header-Update via tools/file_header_updater.py

### C. Erweiterter Scanner-Flow (v3)

- Orchestrator: tools/gap_scanner_v3.py (+ phase scanners)
- Output-Vertrag:
  - ai_working/gap_scan_v3_*.json
  - issue templates / pipeline metrics

## 2) Hauptkonflikte

1. Doppeltes Header-System
- In Dateien sind teils beide Header sichtbar:
  - THEMIS_GAP_STATS (v2 tools)
  - ThemisDB box header (analyze_code_maturity.py)
- Beispiel: src/training/modality_parser.cpp

2. Zwei konkurrierende Report-Modelle
- CI erwartet docs/code_maturity_report.md.
- Team arbeitet praktisch mit ai_working/*.json.
- Ergebnis: "Report ersetzt"-Wahrnehmung und Drift.

3. Unterschiedliche Scopes
- analyze_code_maturity.py scannt .cpp/.c/.h/.hpp/.cs/.py/.php
- file_header_updater.py aktualisiert nur .cpp/.hpp
- Semantik und Abdeckung sind nicht deckungsgleich.

4. Dokumentationsdrift
- Mehrere ai_working-Dokus beschreiben jeweils unterschiedliche Wahrheiten.
- docs/ci-cd/.../code-maturity-analysis.md ist explizit als Legacy markiert.

5. ai_working-Rolle vs Nutzung
- ai_working/README.md definiert ai_working als Entwurfsraum.
- Tatsächlich liegen dort sehr viele langlebige JSON-Artefakte als Quasi-Quelle.

## 3) Zielbild (Single Source of Truth)

### Zielentscheidung Z1
Eine kanonische CI-Quelle festlegen:

Option A (bevorzugt kurzfristig):
- Beibehalten von docs/code_maturity_report.md als CI-kanonischem Report
- ai_working JSON bleibt Audit/Engineering-Detail

Option B (mittel/langfristig):
- CI kanonisiert JSON (z. B. .github/quality/code_maturity_summary.json)
- Markdown wird deterministisch daraus generiert

### Zielentscheidung Z2
Ein einziges Headerformat als Canonical wählen:
- Entweder box header (analyze_code_maturity)
- oder THEMIS_GAP_STATS (v2)
- keine parallele Koexistenz im selben File

### Zielentscheidung Z3
v2/v3 Rollen trennen:
- v2: kompakte quality/maturity baseline
- v3: tiefer Audit/issue-clustering scanner
- keine stillen Überschreibungen desselben Artefaktnamens

## 4) Priorisierte TODO-Liste

## P0 - Sofort (1-3 Tage)

- [ ] T001: Canonical Artifact Decision dokumentieren (Z1)
  - Output: docs/ci-cd/workflows/08-maintenance/code-maturity-analysis.md (aktualisiert, nicht legacy-only)
  - Done: eindeutiger Satz "canonical artifact is ..."

- [ ] T002: Header Canonical Decision dokumentieren (Z2)
  - Output: ai_working/CODE_MATURITY_HEADER_DOCUMENTATION.md + docs counterpart
  - Done: nur ein erlaubtes Headerformat, inkl. migration note

- [ ] T003: Guardrail im Workflow ergänzen
  - Datei: .github/workflows/08-maintenance_code-maturity.yml
  - Done: Job summary zeigt eindeutig, welches Artefakt als canonical gewertet wurde

## P1 - Konsolidierung (1 Woche)

- [ ] T101: Doppelte Header im Repository inventarisieren
  - Output: ai_working/header_conflicts_2026-05-20.json
  - Done: Liste aller Dateien mit beiden Header-Typen

- [ ] T102: One-time Migration Tool bauen
  - Zweck: Konfliktheaders vereinheitlichen
  - Output: tools/migrate_maturity_headers.py
  - Done: dry-run + apply-mode + idempotent

- [ ] T103: Score-Modelle angleichen
  - analyze_code_maturity vs gap_scanner_v2 Kategorien mappen
  - Output: ai_working/score_mapping_v1.md
  - Done: dokumentierte 1:1 oder 1:n Mapping-Tabelle

- [ ] T104: Artefakt-Namenskonvention fixieren
  - Output: docs/ci-cd/quality-artifact-contract.md
  - Done: keine kollidierenden Dateinamen für CI vs Audit

## P2 - Stabilisierung (2-3 Wochen)

- [ ] T201: CI-kompatibler JSON<->Markdown Bridge
  - Falls Option B gewählt: Generator für Markdown aus canonical JSON
  - Output: tools/render_code_maturity_report.py
  - Done: deterministisch, getestet, im Workflow integriert

- [ ] T202: v3 Scanner sauber in Roadmap verankern
  - Output: ai_working/SCANNER_TOOLSET_OVERVIEW.md + docs counterpart
  - Done: klare Abgrenzung "audit depth" vs "CI maturity"

- [ ] T203: Legacy-Dokus markieren und umleiten
  - Output: Hinweisblöcke + canonical links
  - Done: kein widersprüchlicher Einstiegspunkt mehr

## P3 - Governance (laufend)

- [ ] T301: Monthly Drift Review
  - Check: Workflow contract, tool outputs, docs consistency
  - Output: ai_working/monthly_maturity_drift_review_YYYY-MM.md

- [ ] T302: PR-Checklist erweitern
  - Frage: "ändert diese PR canonical maturity artifacts oder nur audit artifacts?"
  - Output: .github/pull_request_template.md (falls vorhanden)

## 5) Operating Model (ab sofort)

Bis zur Konsolidierung gilt:

1. CI-Maturity Status:
- maßgeblich über .github/workflows/08-maintenance_code-maturity.yml und analyze_code_maturity.py

2. Deep Gap Audit / Issue-Building:
- über tools/gap_scanner_v2.py, tools/gap_scanner_v3.py und ai_working JSON

3. Header-Änderungen:
- nur gezielt; keine gemischten bulk-runs aus beiden Systemen im selben PR

## 6) Nächster konkreter Schritt

Empfehlung für den nächsten PR (klein, risikoarm):

1. Entscheidung Z1 + Z2 schriftlich fixieren
2. T101 Inventar-Skript/Lauf ausführen
3. T102 Migrationsscript im Dry-Run erstellen
4. Danach erst Massenänderungen an Headers
