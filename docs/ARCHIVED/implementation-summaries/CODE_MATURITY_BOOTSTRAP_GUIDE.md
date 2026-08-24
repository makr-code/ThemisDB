## Status: Stale – Archivierungskandidat
> **Hinweis (2026-08-12):** Diese Datei enthält TODO/FIXME/STALE/TBD/PLACEHOLDER-Marker und wird als Archivierungskandidat geführt. Inhalte wurden nicht gelöscht. Für den aktuellen Stand bitte kanonische Quellen und den [Root-Index](00_DOCUMENTATION_INDEX.md) konsultieren.
<!-- stale-marker: DOC-WEEKLY-2026-33 -->


> **⚠️ STATUS: STALE – Archivierungskandidat**
> Dieser Inhalt enthält veraltete TODO/FIXME/PLACEHOLDER-Marker und wird im nächsten Archiv-Run nach `docs/ARCHIVED/` verschoben.
> Bitte nicht als aktuelle Referenz nutzen. Inventar: [DOCS_INVENTORY_2026-Q3.md](Audit/DOCS_INVENTORY_2026-Q3.md)

---

# Code Maturity Workflow – Bootstrap-Anleitung

## ⚠️ Problem: Workflow läuft nicht

Der GitHub Actions Workflow `08-maintenance_code-maturity.yml`, der automatisch Source-Code-Header aktualisieren soll, wird nicht ausgeführt.

### Ursache

Die **initiale Report-Datei fehlt**: `docs/code_maturity_report.md`

Der Workflow braucht diese Datei, um:
- Die aktuelle Code-Reife zu evaluieren
- Drift zwischen aktueller und erzeugter Report zu erkennen
- Header-Updates zu verfolgen

**Ohne diese Datei:**
- Der Workflow startet zwar, gibt aber nur einen informativen Notice aus
- PR-basierte Trigger funktionieren nicht (Drift-Check schlägt fehl)
- Der wöchentliche Schedule (`Montag 3 AM UTC`) läuft im Hintergrund, ohne sichtbare Wirkung

## ✅ Lösung: Workflow Bootstrap

### Option 1: Mit PowerShell-Skript (empfohlen)

```powershell
# Starten Sie ein PowerShell-Terminal in der ThemisDB-Root

# Check-only Mode (keine Änderungen an Source-Dateien)
.\scriptsootstrap_code_maturity_workflow.ps1

# Mit Header-Updates (modifiziert Source-Dateien!)
.\scriptsootstrap_code_maturity_workflow.ps1 -UpdateHeaders

# Dry-Run (zeigt nur den Befehl an, führt nichts aus)
.\scriptsootstrap_code_maturity_workflow.ps1 -DryRun
```

### Option 2: Mit GitHub CLI

```bash
# Einfacher Trigger (Check-only, nur Report generieren)
gh workflow run 08-maintenance_code-maturity.yml --ref develop

# Mit Header-Updates aktiviert
gh workflow run 08-maintenance_code-maturity.yml --ref develop -f update_headers=true
```

### Option 3: Manuell über GitHub UI

1. Gehen Sie zu **Actions** → **Code Maturity Maintenance**
2. Klicken Sie **Run workflow**
3. Wählen Sie Branch: `develop`
4. (Optional) Setzen Sie `update_headers = true`
5. Klicken Sie **Run workflow**

## 📊 Was passiert nach dem Bootstrap?

### Unmittelbar nach dem Trigger:

1. **Ubuntu-Runner startet** (~1-2 Min)
2. **Python-Skript analysiert** Quellcode auf Reife-Indikatoren:
   - Stubs & Unimplemented-Pfade
   - Simulations & Mock-Code
   - TODO/FIXME Kommentare
   - Test-Abdeckung
   - Dokumentation

3. **Report wird generiert**:
   - `/tmp/code_maturity_report.md` (bei Check-only)
   - `.github/badges/code_maturity.json` (Mermaid-Badge)
   - `.github/version_tracking.json` (Versionsverfolgung)

### Nach dem erfolgreichen Run:

1. **Workflow Artifacts**:
   - Laden Sie `code-maturity-report-<run_number>` herunter
   - Enthält die generierte Report und Tracking-Dateien

2. **Erste Commit-Vorbereitung**:
   ```bash
   # Extrahieren Sie aus dem Artifact:
   cp /path/to/artifact/code_maturity_report.md docs/
   cp /path/to/artifact/.github/badges/*.json .github/badges/
   
   # Committen Sie:
   git add docs/code_maturity_report.md .github/badges/ .github/version_tracking.json
   git commit -m "docs: initialize code maturity report and badges"
   git push origin develop
   ```

3. **Danach läuft der Workflow automatisch**:
   - **Wöchentlich**: Montag 3 AM UTC (Check-only)
   - **Bei PR-Changes**: Wenn Sie `.github/scripts/analyze_code_maturity.py` oder die Workflow-Datei ändern
   - **Manuell**: Jederzeit über `workflow_dispatch`

## 🔍 Workflow-Auswahl-Modes

### Check-only (Standard)

```yaml
update_headers: false  (default)
```

**Verhalten:**
- Liest Sourcecode, änderniemands
- Generiert Report zu `/tmp/`
- Vergleicht mit committed `docs/code_maturity_report.md`
- Warnt bei Drift
- Sicher für PR-Trigger

### Rewrite (Nur manuell!)

```yaml
update_headers: true
```

**Verhalten:**
- Aktualisiert **ALLE** Source-Header mit Reife-Scores
- Schreibt `docs/code_maturity_report.md`
- Aktualisiert `.github/version_tracking.json`
- Generiert Badge-JSONs
- **WARNUNG**: Saubere Review der Artifacts vor Commit erforderlich!

## 🛠️ Troubleshooting

### "Workflow startet nicht"

1. **Überprüfe GitHub CLI**:
   ```powershell
   gh --version
   gh auth status  # Muss authentifiziert sein
   ```

2. **Überprüfe Branch**:
   ```bash
   git branch -v  # Sollte `develop` sein
   git push origin develop:develop  # Stelle sicher, dass der Branch existiert
   ```

3. **Überprüfe Dateiberechtigungen**:
   ```bash
   ls -la .github/scripts/analyze_code_maturity.py  # Muss vorhanden sein
   ls -la .github/workflows/08-maintenance_code-maturity.yml
   ```

### "Workflow schlägt mit Python-Fehler fehl"

1. **Lokale Syntax-Überprüfung**:
   ```bash
   python3 -m py_compile .github/scripts/analyze_code_maturity.py
   python3 .github/scripts/analyze_code_maturity.py --root . --no-headers
   ```

2. **Überprüfe Abhängigkeiten** (nur stdlib erforderlich, keine imports nötig)

3. **Überprüfe Permissions**:
   ```bash
   # Linux/Mac
   stat .github/scripts/analyze_code_maturity.py
   # Sollte lesbares Executable-Bit haben
   ```

### "artifacts sind leer"

1. **Überprüfe Job-Summary** im Workflow-Log
2. **Überprüfe /tmp/ Inhalte** im Upload-Step
3. **Überprüfe Git-Log**:
   ```bash
   git log --oneline -- .github/scripts/analyze_code_maturity.py | head -5
   ```

## 📚 Weiterführende Docs

- [WORKFLOW_REGISTRY.md](../.github/WORKFLOW_REGISTRY.md#aktiver-workflow-kern) – Überblick aller Workflows
- [CODE_MATURITY_HEADER_DOCUMENTATION.md](../ai_working/CODE_MATURITY_HEADER_DOCUMENTATION.md) – Detaillierte Header-Format-Doku
- [CODE_MATURITY_INTEGRATION_INDEX.md](../ai_working/CODE_MATURITY_INTEGRATION_INDEX.md) – Integration und Phase-Modell

## 🚀 Empfohlene Nächste Schritte

1. **Sofort:**
   ```powershell
   .\scriptsootstrap_code_maturity_workflow.ps1
   ```

2. **In ~2-5 Minuten:**
   ```bash
   gh run list --workflow="08-maintenance_code-maturity.yml" --limit 1
   ```

3. **Nach Completion:**
   - Review das heruntergeladene Artifact
   - Committen Sie `docs/code_maturity_report.md` + Badges
   - Pushen Sie

4. **Danach:**
   - Workflow läuft automatisch wöchentlich
   - PR-Trigger aktiviert sich bei Änderungen an Workflow/Script
   - Header bleiben automatisch up-to-date

---

**Fragen oder Probleme?** Überprüfen Sie das Job-Log unter:  
`https://github.com/makr-code/ThemisDB/actions/workflows/08-maintenance_code-maturity.yml`
