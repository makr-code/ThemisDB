---
status: current
doc_version: "1.5.0"
validated: "2026-03-09"
---

# Dokumentations-Metadaten & Drift-Konventionen

**Kategorie:** Development · Infra  
**Erstellt:** 2026-03-09  
**Zuständig:** Themis DevTeam

---

## Übersicht

Dieses Dokument legt einheitliche, minimale Metadaten für alle *Secondary Docs* in ThemisDB fest. Ziel ist es, „gewollte Drift" klar zu signalisieren und den aktuellen Stand jeder Dokumentationsdatei maschinenlesbar zu machen.

> **Secondary Docs** sind alle handverfassten Markdown-Dateien unter `docs/` (z. B. Guides, Konzepte, Entwicklungsdokumentation). Automatisch generierte Code-Kommentare und API-Referenzen sind ausgenommen.

---

## Pflicht-Felder (YAML Front Matter)

Jede Secondary Doc **muss** einen YAML-Front-Matter-Block am Dateianfang besitzen:

```yaml
---
status: current          # Pflicht – siehe Werte unten
doc_version: "1.5.0"    # Pflicht – Semver-Tag oder Branch-Bezeichner
validated: "2026-03-09" # Optional – Datum der letzten manuellen Prüfung (YYYY-MM-DD)
---
```

### Feld: `status`

| Wert | Bedeutung |
|------|-----------|
| `current` | Dokument ist aktuell und entspricht dem Code-Stand. |
| `drifting` | Dokument hinkt absichtlich hinterher (z. B. Feature noch in Entwicklung). |
| `stale` | Dokument ist bekanntermaßen veraltet; Update ausstehend. |
| `archived` | Dokument ist archiviert; keine Aktualisierungen vorgesehen. |

### Feld: `doc_version`

Semver-Version oder Git-Tag, für den das Dokument gilt. Muss **nicht** mit `VERSION` übereinstimmen, wenn bewusste Drift vorliegt.

Beispiele:
- `"1.5.0"` → gilt für Release 1.5.0
- `"1.4.x"` → gilt für den gesamten 1.4-Zweig
- `"develop"` → spiegelt den aktuellen Entwicklungsstand wider

### Feld: `validated` (optional)

Datum der letzten manuellen Überprüfung im Format `YYYY-MM-DD`. Hilft beim Erkennen von Dokumenten, die lange nicht geprüft wurden.

---

## Vollständiges Beispiel

```markdown
---
status: drifting
doc_version: "1.4.x"
validated: "2026-01-15"
---

# Mein Feature Guide

Inhalt …
```

---

## Gültige `status`-Werte auf einen Blick

```
current   – aktuell und vollständig
drifting  – absichtlich hinter dem Code-Stand
stale     – veraltet, Update benötigt
archived  – archiviert, kein Update geplant
```

---

## Scope: Welche Dateien sind betroffen?

Der automatische Metadaten-Check (`scripts/docs-lint.py --check-metadata`) prüft standardmäßig folgende Pfade:

- `docs/de/development/`
- `docs/de/features/`
- `docs/de/guides/`
- `docs/de/architecture/`
- `docs/de/security/`

Verzeichnisse, die explizit ausgeschlossen sind:

- `docs/ARCHIVED/`, `docs/archive/`, `docs/de/archive/`
- Automatisch generierte Dateien (erkennbar am Präfix `IMPLEMENTATION_SUMMARY`, etc.)

Pfade können über `--metadata-paths` angepasst werden:

```bash
python3 scripts/docs-lint.py --check-metadata --metadata-paths docs/de/development docs/de/features
```

---

## CI-Integration

Der Metadaten-Check läuft automatisch als Teil des `documentation-validation`-Workflows:

- **Job:** `metadata-check`
- **Trigger:** Pull Requests und Pushes nach `main`, `develop`, `release/**`
- **Fehlerbedingung:** Fehlt `status` oder `doc_version` in einem Secondary Doc, schlägt die CI fehl

---

## Verwandte Dokumente

- [Dokumentations-Validierung](../DOKUMENTATIONS_VALIDIERUNG.md)
- [Dokumentations-Benennungskonvention](DOCUMENTATION_NAMING_CONVENTION.md)
- [Validation Script](../../../scripts/docs-lint.py)
