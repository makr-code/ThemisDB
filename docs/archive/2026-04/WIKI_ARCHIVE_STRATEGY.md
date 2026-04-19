[docs](./index.md) > [WIKI_ARCHIVE_STRATEGY](WIKI_ARCHIVE_STRATEGY.md)
**Datum:** 2026-03-12
**Status:** stable
**Primary (Quelle der Wahrheit):**
- `src/README.md`
- `docs/CONTENT_MODEL.md`

**Bezug / Reference:**
- Issue: [META] Dokumentationssystem: Primary → Secondary → Compendium
- Kontext: Phase 5 — Strategie für Wiki und Archiv im ThemisDB-Dokumentationssystem.

---

## TL;DR

Das GitHub-Wiki dient als **Community-Spiegel** für ausgewählte Secondary Docs. Das `docs/ARCHIVED/`-Verzeichnis ist die einzige permanente Archivschicht. Veraltete Inhalte werden nicht gelöscht, sondern mit `**Status:** archived` versehen und in `docs/ARCHIVED/` verschoben.

---

## Kontext

- **Problem:** Dokumentation verteilt sich auf GitHub-Wiki, `docs/ARCHIVED/`, diverse Root-Level-Markdown-Dateien und Sub-Verzeichnisse ohne klare Ownership oder Lifecycle-Regeln.
- **Ziel:** Eindeutige Rollen für Wiki, Archiv und Community-Beiträge; automatisierte Mirror-Logik; klare Retention-Policy.
- **Nicht-Ziele:** Vollständige Migration aller historischen Inhalte; Löschung von Archiv-Dokumenten.

---

## Strategie: GitHub-Wiki (Mirror-Modus)

### Rolle des Wikis

Das GitHub-Wiki ist ein **read-only Mirror** ausgewählter Secondary Docs für Nutzer, die keine Pull Requests stellen können oder wollen. Es ist **nicht** die Source of Truth.

```
Secondary Docs (docs/de/**) ──publish──► GitHub Wiki (Read-Only Mirror)
                                          (automatisch via CI)
```

### Was ins Wiki gespiegelt wird

Nur stabile (`**Status:** stable`) Secondary Docs der folgenden `doc_kind`-Typen:
- `howto` — Schritt-für-Schritt-Anleitungen
- `faq` — Häufige Fragen
- `reference` — Kurzreferenzen / API-Cheatsheets

**Nicht** gespiegelt:
- `architecture` — zu technisch, gehört in die Hauptdokumentation
- `roadmap` — ändert sich zu häufig
- `draft`/`review`-Docs — noch nicht stabil
- Compendium-Kapitel — werden separat als PDF veröffentlicht

### Wiki-Automation

Das Skript `tools/publish_wiki.py` ist der Einstiegspunkt für automatisierte Wiki-Veröffentlichungen.

**Lokaler Test:**
```bash
python3 tools/publish_wiki.py --dry-run --source docs/de
```

**CI (Trigger: Push auf main, nur stabile Howtos/FAQs/References):**
```yaml
# Snippet aus .github/workflows/publish-wiki.yml (geplant)
- name: Sync Wiki
  run: python3 tools/publish_wiki.py --source docs/de --filter stable
  env:
    GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}
```

### Community-Beiträge zum Wiki

Nutzer können über den GitHub-Wiki-Editor Korrekturen vorschlagen. Diese werden **nicht automatisch** in die Primary/Secondary Docs übernommen. Der Prozess:

1. Community-Mitglied bearbeitet Wiki-Seite
2. Maintainer prüft die Änderung
3. Bei Übernahme: Maintainer öffnet PR gegen `docs/de/<modul>/` in der Hauptrepo
4. Nach Merge: Wiki wird beim nächsten CI-Lauf neu gespiegelt

---

## Strategie: Archiv

### Archiv-Policy

Ein Dokument wird archiviert, wenn:
- Es durch ein neueres Dokument ersetzt wurde **und**
- Es historischen Wert hat (z. B. Entscheidungsdokumentation, Meilensteinberichte) **oder**
- Es mind. 6 Monate alt und nicht mehr aktuell ist (Drift-Status `stale`)

**Retention:** Archivdokumente werden **niemals gelöscht**. Sie bleiben dauerhaft unter `docs/ARCHIVED/` erhalten.

### Archivierungs-Workflow

```
1. Status im Dokument auf "archived" setzen:
   **Status:** archived

2. Dokument verschieben nach:
   docs/ARCHIVED/<jahr>/<ursprünglicher-pfad>.md

3. Im ursprünglichen Speicherort (optional) eine Stub-Datei anlegen:
   "Dieses Dokument wurde archiviert. Siehe docs/ARCHIVED/..."

4. PR erstellen mit Label "docs/archive"
```

### Archiv-Verzeichnisstruktur

```
docs/ARCHIVED/
├── 2025/
│   ├── architecture/
│   ├── features/
│   └── guides/
├── 2026/
│   ├── architecture/
│   ├── features/
│   └── guides/
└── README.md           ← Erläutert die Archiv-Policy
```

### Bestehende archivierte Inhalte

Bereits vorhandene Archivdokumente in `docs/ARCHIVED/` und `docs/de/archive/` behalten ihre aktuelle Position. Eine rückwirkende Migration in die Jahresstruktur ist optional.

---

## Strategie: Root-Level-Markdown-Dateien

Viele historische Markdown-Dateien liegen direkt im Root-Verzeichnis (`roadmap.md`, `ARCHITECTURE.md`, etc.) oder in `docs/` ohne Sprach-Präfix. Diese folgen der folgenden Lifecycle-Regel:

| Typ | Aktion |
|-----|--------|
| `README.md`, `ARCHITECTURE.md`, `CHANGELOG.md` | Bleiben im Root als Primary Docs |
| Implementierungs-Summaries (z. B. `PHASE1_*.md`) | → `docs/ARCHIVED/<jahr>/` |
| Technische Guides ohne Sprach-Zuordnung | → `docs/de/<modul>/` oder `docs/en/<modul>/` |
| Duplikate / Obsolete | → `docs/ARCHIVED/<jahr>/` mit Status `archived` |

---

## CI-Integration (geplant)

### Geplante Workflow: `publish-wiki.yml`

```yaml
name: Publish Wiki Mirror

on:
  push:
    branches: [main]
    paths:
      - 'docs/de/**/*.md'

jobs:
  sync-wiki:
    name: Sync stable Secondary Docs to Wiki
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Publish to Wiki
        run: python3 tools/publish_wiki.py --source docs/de --filter stable
        env:
          GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}
```

**Status:** Geplant (Phase 5). Das Skript `tools/publish_wiki.py` existiert bereits.

### Geplanter Workflow: `archive-stale-docs.yml`

Ein Cron-basierter Workflow, der Docs mit Status `stale` (>180 Tage hinter Primary-Quelle) in GitHub Issues meldet, damit Maintainer über Archivierung entscheiden können.

**Status:** Geplant (Phase 5). Basis ist `scripts/drift-detector.py` + GitHub Issues API.

---

## Entscheidungen / Trade-offs

| Entscheidung | Begründung |
|---|---|
| Wiki = Read-only Mirror | Verhindert Divergenz zwischen Wiki und offizieller Doku. Community-Beiträge gehen immer durch den PR-Prozess. |
| Kein Wiki für Architecture-Docs | Architektur-Dokumentation ist zu technisch für Wiki-Leser und ändert sich mit Releases. |
| Archiv niemals löschen | Historische Entscheidungsdokumentation hat langfristigen Wert. Disk-Space ist vernachlässigbar. |
| Jahres-Ordner im Archiv | Vereinfacht die Navigation und macht das Alter von Inhalten sofort sichtbar. |
| `tools/publish_wiki.py` als Einstiegspunkt | Wiederverwendung des bestehenden Tools; keine neue Abhängigkeit. |

---

## Links

- [Inhaltsmodell](./CONTENT_MODEL.md)
- [Sprachstruktur](./LANGUAGE_STRUCTURE.md)
- [Drift-Erkennung](../scripts/drift-detector.py)
- [Wiki-Publish-Tool](../tools/publish_wiki.py)
- [docs/ARCHIVED/](./ARCHIVED/)
