# 🔄 Issue Migration Guide — 7-Phase Workflow Upgrade

## Overview

Dieser Guide erklärt, wie Sie bestehende GitHub Issues auf das neue **7-Phasen-Workflow-Modell** aktualisieren.

---

## 🎯 Motivation

**Alt (4-Phasen):**
- Nur Audit → Plan → Implement → Review
- Wenig Struktur für AI-Agenten
- Fehlende Checkpoints zwischen Phasen

**Neu (7-Phasen):**
- ✅ Phase 0: Validierung (verhindert Fehler vor Start)
- ✅ Phase 1: Audit
- ✅ Phase 2: Planung
- ✅ Phase 3: Implementierung (mit inkrementellen Checkpoints)
- ✅ Phase 4: Automatisierter Review (CI/CD)
- ✅ Phase 5: Menschlicher Review
- ✅ Phase 6: Dokumentation
- ✅ Phase 7: Merge
- ✅ Explizite Fehlerbehandlung & Rollback-Kriterien

---

## 📋 Schritt-für-Schritt Anleitung

### Schritt 1: Dry-Run (Vorschau anschauen)

**Was ändert sich bei den Issues?**

```bash
# Alle Issues anschauen (keine Änderung)
python tools/migrate_issues_to_7phase.py ai_working/gap_scan_v3_aggregate.json

# Oder nur ein spezifisches Modul
python tools/migrate_issues_to_7phase.py ai_working/gap_scan_v3_aggregate.json --module security
```

**Output beispiel:**
```
🔄 Migrating Existing Issues to 7-Phase Workflow
================================================================================
⚠️  [DRY-RUN MODE] — No actual GitHub changes will be made

📋 Fetching existing gap-remediation issues from GitHub...
Found 8 existing issues

📝 [DRY-RUN] Would update issue #123 (security)
================================================================================
# 🔴 SECURITY Module Gap Remediation — 7-Phase Workflow

**Status:** Ready for Implementation (7-Phase AI-Agent Model)
...
[truncated - sehen Sie die komplette neue Issue-Body]
```

---

### Schritt 2: GitHub Authentifizierung Setup

**Falls Sie GitHub CLI nutzen wollen (empfohlen):**

```bash
# GitHub CLI installieren (falls nicht vorhanden)
# Windows: choco install gh
# macOS: brew install gh
# Linux: sudo apt install gh

# Authentifizierung
gh auth login
# → Select GitHub.com
# → Select HTTPS
# → Authenticate with your GitHub credentials

# Verifizieren
gh auth status
```

**Oder: manuell Token setzen**

```powershell
# PowerShell (Windows)
$env:GH_TOKEN = "ghp_your_personal_access_token_here"

# Bash (macOS/Linux)
export GH_TOKEN="ghp_your_personal_access_token_here"
```

---

### Schritt 3: LIVE UPDATE (Alle Issues migrieren)

**⚠️ Warnung: Dies ändert alle bestehenden Issues auf GitHub!**

```bash
# LIVE UPDATE: Alle gap-remediation Issues
python tools/migrate_issues_to_7phase.py ai_working/gap_scan_v3_aggregate.json --github

# Oder nur ein spezifisches Modul
python tools/migrate_issues_to_7phase.py ai_working/gap_scan_v3_aggregate.json --github --module security
```

**Output:**
```
🔄 Migrating Existing Issues to 7-Phase Workflow
================================================================================

📋 Fetching existing gap-remediation issues from GitHub...
Found 8 existing issues

✅ Updated issue #123 (security)
✅ Updated issue #124 (memory)
✅ Updated issue #125 (reliability)
...

📊 Migration Summary
================================================================================
Total issues found:     8
Updated:                8 ✅
Failed:                 0 ❌
Skipped:                0 ⏭️

Mode:                   LIVE (changes applied)
```

---

## 📋 Was ändert sich bei den Issues?

### VORHER (Alt):
```
# 🔴 SECURITY Module Gap Analysis & Remediation Guide

**AI Agent Ready:** This issue contains detailed acceptance criteria...

## Executive Summary
[table]

## Gap Breakdown by Category
[category details]

[etc.]
```

### NACHHER (Neu):
```
# 🔴 SECURITY Module Gap Remediation — 7-Phase Workflow

**Status:** Ready for Implementation (7-Phase AI-Agent Model)

## 🚀 7-Phase Workflow Progress
[workflow diagram mit Checkboxen]

## ✅ Phase 0: Pre-Start Validation
[Validation-Tasks]

## 📋 Phase 1: Code Audit & Discovery
[Audit-Tasks mit Checkboxen]

## 📐 Phase 2: Implementation Planning
[Planning-Tasks]

## 💻 Phase 3: Code Implementation
[Implementation Loop mit inkrementellen Checkpoints]

## 🤖 Phase 4: Automated Code Review
[Automated-Check-Kriterien]

## 👤 Phase 5: Human Code Review
[Review-Checklist]

## 📚 Phase 6: Documentation
[Doc-Tasks]

## 🎉 Phase 7: Merge & Release
[Merge-Tasks]

## 🚨 Error Handling & Escalation
[Rollback-Kriterien für Build-Fehler, Test-Fehler, etc.]
```

---

## ✅ Checkliste für die Migration

- [ ] GitHub CLI installiert und authentifiziert (`gh auth status` OK)
- [ ] `GH_TOKEN` korrekt gesetzt (falls kein gh CLI)
- [ ] Aggregate-Datei vorhanden: `ai_working/gap_scan_v3_aggregate.json`
- [ ] Dry-run durchgeführt und Änderungen überprüft
- [ ] Alle Team-Mitglieder informiert über Issue-Format-Änderung
- [ ] LIVE UPDATE durchgeführt: `python tools/migrate_issues_to_7phase.py ... --github`
- [ ] Verifiziert: Mindestens 1-2 Issues auf GitHub öffnen und neues Format checken
- [ ] Alte Issue-Templates archiviert/gelöscht

---

## 🔍 Verifikation nach Migration

**Checken Sie auf GitHub:**

1. Issue #123 öffnen (security oder ein anderes Modul)
2. Neue Struktur sichtbar:
   - ✅ 7-Phase Workflow Progress Section
   - ✅ Phase 0-7 Sections mit Checkboxen
   - ✅ Error Handling & Escalation
3. Issue-Labels intakt:
   - ✅ `gap-scanner`
   - ✅ `security` (oder anderes Modul)
   - ✅ `ready-for-ai-agent` (neu)
   - ✅ `7phase-workflow` (neu)

---

## 🚀 Nächste Schritte nach Migration

### Für AI-Agenten:

```bash
# Issue abrufen und Work starten
gh issue view 123

# Feature-Branch erstellen
git checkout -b fix/security-gaps
git push origin fix/security-gaps

# Phase 0 Validierung starten
python tools/gap_scanner_v3.py . ai_working --module security
```

### Für Team-Mitglieder:

1. Ein paar migrierten Issues anschauen
2. Mit AI-Agent starten (Phase 0 Validierung)
3. Feedback geben über neue Struktur

---

## 📝 Rollback (Falls nötig)

Falls die Migration nicht gut läuft, können Sie die Issues manuell zurücksetzen:

```bash
# Alte Issue-Bodies aus Git-History wiederherstellen
git log --all --oneline -- "ai_working/enhanced_issues/"

# Oder: Issues manuell auf GitHub editieren
# (Einfach alte Version aus Ihrem Backup paste)
```

---

## 🎓 Ressourcen

- **7-Phase Workflow Template:** `tools/ISSUE_WORKFLOW_TEMPLATE.md`
- **Migration Tool:** `tools/migrate_issues_to_7phase.py`
- **Gap Scanner:** `tools/gap_scanner_v3.py`
- **Issue Generator (alt):** `tools/gap_issue_enhanced_template_generator.py`

---

## ❓ FAQ

**Q: Werden die bestehenden Issue-Comments und Reactions gelöscht?**  
A: Nein, nur der Issue-Body wird aktualisiert. Alle Comments bleiben bestehen.

**Q: Kann ich die Migration rückgängig machen?**  
A: Ja, Sie können die alte Version aus Git-History oder von GitHub API abrufen.

**Q: Was wenn ein Issue nicht geparst werden kann?**  
A: Das Tool überspringt es und meldet "Skipped". Sie können diese Issues manuell updaten.

**Q: Sollen die alten Issue-Templates archiviert werden?**  
A: Empfohlen: `ai_working/enhanced_issues/` → `ai_working/enhanced_issues_backup/`

---

## 📞 Kontakt

Bei Fragen zum neuen 7-Phasen-Modell:  
Siehe `ISSUE_WORKFLOW_TEMPLATE.md` für vollständige Dokumentation.

