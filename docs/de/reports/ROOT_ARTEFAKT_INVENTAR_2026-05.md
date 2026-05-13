# Root-Artefakt-Inventar und Bereinigungsregel (Issue: Root-Artefakte)

**Stand:** 13. Mai 2026
**Status:** ✅ Dokumentationsaudit abgeschlossen
**Scope:** Root-Dateien `build_*.txt`, `test_*.txt`, `tmp_*.md`, `sec_block.txt`, `scout_cves_*.sarif`

---

## 1) Inventar (kanonisch vs. temporaer)

### 1.1 Aktuelle Root-Funde (Ist-Stand)

Alle aktuell gefundenen Dateien im Root mit `tmp_*.md` sind **temporär** und **nicht-kanonisch**:

| Datei | Klassifikation | Entscheidung | Zielpfad |
|---|---|---|---|
| `tmp_acceleration_blueprint.md` | temporär | aus Root entfernen | `docs/archive/tmp-notes/` |
| `tmp_ai_blueprint_4931.md` | temporär | aus Root entfernen | `docs/archive/tmp-notes/` |
| `tmp_analytics_blueprint.md` | temporär | aus Root entfernen | `docs/archive/tmp-notes/` |
| `tmp_api_blueprint.md` | temporär | aus Root entfernen | `docs/archive/tmp-notes/` |
| `tmp_review_audit_block.md` | temporär | aus Root entfernen | `docs/archive/tmp-notes/` |
| `tmp_scraper_blueprint.md` | temporär | aus Root entfernen | `docs/archive/tmp-notes/` |
| `tmp_tensor_blueprint.md` | temporär | aus Root entfernen | `docs/archive/tmp-notes/` |
| `tmp_tracker_new_modules.md` | temporär | aus Root entfernen | `docs/archive/tmp-notes/` |
| `tmp_tracking_priorities_comment.md` | temporär | aus Root entfernen | `docs/archive/tmp-notes/` |
| `tmp_tracking_stub_priorities.md` | temporär | aus Root entfernen | `docs/archive/tmp-notes/` |

### 1.2 Scope-Muster ohne aktuellen Root-Treffer

Diese Muster sind weiterhin als **nicht-kanonische Root-Artefakte** definiert, aktuell aber nicht im Root vorhanden:

| Muster | Aktueller Root-Status | Klassifikation | Zielpfad |
|---|---|---|---|
| `build_*.txt` | 0 Treffer | temporär (Build-Output) | `logs/archive/` |
| `test_*.txt` | 0 Treffer | temporär (Test-Output) | `tests/outputs/` bzw. `tests/outputs/archive/` |
| `sec_block.txt` | 0 Treffer | temporär (Hilfsartefakt) | `docs/ARCHIVED/root-drafts/` |
| `scout_cves_*.sarif` | 0 Treffer | temporär (Scan-Rohdaten) | `docs/audit-framework/evidence/v<release-version>/scans/` (z. B. `.../v1.4.1/scans/`) |

---

## 2) Aufbewahrungs- und Archivierungsregel

- Root ist nur fuer stabile Leitdokumente/Navigation gedacht.
- Build-/Test-/Security-Rohoutputs gelten als Betriebsartefakte, nicht als redaktionelle Quelle.
- Temporäre Root-Artefakte werden zeitnah verschoben:
  - `tmp_*.md` -> `docs/archive/tmp-notes/`
  - Build-/Test-Outputs -> `logs/archive/` und `tests/outputs/`
  - Security-Scan-Rohdaten -> `docs/audit-framework/evidence/v<release-version>/scans/` (Schema laut Audit-Runbook)
- Archivierte Inhalte bleiben nachvollziehbar, werden aber nicht als aktuelle Produktdokumentation referenziert.

---

## 3) Review-/Audit-Nachweis (Pflicht)

Referenzdokumente:
- `docs/DOCUMENTATION_REVIEW_GUIDELINES.md`
- `docs/SYSTEMATISCHER_REVIEWPLAN.md`
- `docs/PR_DOCUMENTATION_CHECKLIST.md`
- `docs/de/development/SOURCE_CODE_AUDIT.md`
- `docs/audit-framework/AUDIT_RUNBOOK.md`

Nachweis:
- [x] Fachreview gegen Doku-/Review-Checklisten durchgeführt
- [x] Dokumentationsaudit durchgeführt (Root-Artefakte klassifiziert)
- [x] Ergebnis als verlinkbarer Report dokumentiert (dieses Dokument)
- [x] Relevante Bereiche festgehalten (`docs/`, Root-Artefakte, Archivziele)

Betroffene/überprüfte Pfade:
- `/home/runner/work/ThemisDB/ThemisDB/docs/DOCS_ORGANIZATION_PLAN.md`
- `/home/runner/work/ThemisDB/ThemisDB/docs/de/reports/ROOT_ARTEFAKT_INVENTAR_2026-05.md`
- `/home/runner/work/ThemisDB/ThemisDB/docs/archive/tmp-notes/README.md`
- `/home/runner/work/ThemisDB/ThemisDB/docs/audit-framework/AUDIT_RUNBOOK.md`
- `/home/runner/work/ThemisDB/ThemisDB/scripts/root-docs-hygiene.py`
