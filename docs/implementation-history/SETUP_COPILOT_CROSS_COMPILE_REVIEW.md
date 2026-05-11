# SETUP: Copilot Cross-Compile Review System

**Deutsch:** Eine vollständig automatisierte Code-Review-Pipeline für Cross-Compile-Validierung

---

## 📦 Bereitgestellte Dateien

| Datei | Zweck | Aktivierung |
|-------|-------|-----------|
| `.copilot-cross-compile-prompt.md` | Copilot System Prompt für automatische Reviews | VSCode Copilot Settings |
| `.copilot-cross-compile-rules.json` | Regelwerk in JSON-Format | Programmatische Validierung |
| `scripts/cross-compile-reviewer.py` | Python CLI-Tool für Reviews | CI/CD, Pre-Commit, Manual |
| `.github/workflows/cross-compile-review.yml` | GitHub Actions Workflow | PR-Kommentare, automatische Checks |
| `scripts/pre-commit-cross-compile.py` | Git Pre-Commit Hook | Lokal vor jedem Commit |
| `CROSS_COMPILE_REQUIREMENTS.md` | Detaillierte Richtlinien und Fallbacks | Developer Reference |

---

## 🚀 INSTALLATION

### 1. Pre-Commit Hook lokal aktivieren

```bash
cd C:\VCC\themis

# Hook executable machen
git update-index --add --chmod=+x scripts/pre-commit-cross-compile.py

# Hook installieren
Copy-Item scripts/pre-commit-cross-compile.py -Destination .git/hooks/pre-commit

# Alternativ: Symlink (auf Linux/macOS)
ln -s ../../scripts/pre-commit-cross-compile.py .git/hooks/pre-commit
```

**Test:** Versuche einen neuen commit mit problematischem Code → Hook sollte blockieren

### 2. GitHub Actions aktivieren

```bash
# Die Workflow-Datei liegt bereits im korrekten Pfad:
.github/workflows/cross-compile-review.yml

# Sie werden automatisch bei PRs ausgelöst
# Keine weitere Aktivierung nötig!
```

### 3. VSCode Copilot konfigurieren

```json
// .vscode/settings.json (erstellen, falls nicht vorhanden)
{
  "copilot.inlineChat.systemPrompt": "file://.copilot-cross-compile-prompt.md"
}
```

Oder manuell konfigurieren:
- VSCode öffnen → Settings → "copilot"
- Suche: `copilot.inlineChat.systemPrompt`
- Wert: `.copilot-cross-compile-prompt.md`

---

## 🔧 VERWENDUNG

### A. Manuell vor dem Commit (Local)

```bash
# Alle problematischen Dateien prüfen
python3 scripts/cross-compile-reviewer.py --files src/my_new_file.cpp include/my_header.h

# Oder: Alle staged Files prüfen
python3 scripts/cross-compile-reviewer.py --staged-files

# JSON-Output für CI/CD
python3 scripts/cross-compile-reviewer.py --staged-files --output json --json-file report.json
```

### B. Automatisch bei Commit (Pre-Commit Hook)

```bash
git add src/new_feature.cpp
git commit -m "Add new feature"

# Hook läuft automatisch!
# Falls Violations: Commit wird blockiert
# Falls OK: Commit erfolgreich
```

### C. GitHub Actions (PR Validation)

```bash
# Einfach PR erstellen → GitHub läuft automatisch Review
# Kommentar mit Ergebnissen wird hinzugefügt
```

### D. VSCode Copilot Inline Chat

Beim Schreiben von Code:
```
1. Copilot Inline Chat öffnen (Ctrl+I / Cmd+I)
2. Fragen: "Is this code cross-compile safe?"
3. Copilot nutzt automatisch .copilot-cross-compile-prompt.md
4. Erhält sofort Feedback
```

---

## 📊 BEISPIEL-OUTPUTS

### Genehmigt ✅
```
✅ APPROVED - All cross-compile rules passed

📊 Summary:
  Critical Violations: 0
  High Violations: 0
  Total Violations: 0

📖 Reference: CROSS_COMPILE_REQUIREMENTS.md
```

### Abgelehnt ❌
```
❌ REJECTED - Cross-compile violations found

📊 Summary:
  Critical Violations: 2
  High Violations: 1
  Total Violations: 3

🔴 Critical Issues:
  [RULE_1_forbidden_headers] Line 42: Forbidden platform-specific header: windows.h
      Code: #include <windows.h>
      💡 Use cross-platform alternative from vcpkg or C++20 standard

  [RULE_4_hardcoded_paths] Line 87: Hardcoded path not cross-compile safe: Windows absolute path
      Code: std::string config = "C:\\Program Files\\ThemisDB\\config.ini";
      💡 Use std::filesystem::path or boost::filesystem with getenv()

⚠️ High Priority Issues:
  [RULE_6_compiler_pragmas] Line 15: MSVC-only pragma without GCC/Clang fallback
      Code: #pragma warning(disable: 4996)

📖 Reference: CROSS_COMPILE_REQUIREMENTS.md
```

### GitHub PR Kommentar
```
## 🔍 Cross-Compile Code Review

❌ Review Failed - Cross-compile issues found:

### Summary
- Critical Violations: 2
- High Priority Violations: 1
- Total Issues: 3

### Issues

**[RULE_1_forbidden_headers]** Line 42: Forbidden platform-specific header: windows.h
```c++
\#include <windows.h>
```
💡 Use cross-platform alternative from vcpkg or C++20 standard

---
📖 See CROSS_COMPILE_REQUIREMENTS.md for details
```

---

## 🎯 INTEGRATION ROADMAP

### Phase 1: Lokale Entwicklung ✅ (JETZT)
- Pre-Commit Hook für Developer
- VSCode Copilot Integration
- `cross-compile-reviewer.py` CLI

### Phase 2: CI/CD ✅ (JETZT)
- GitHub Actions Workflow
- PR Comments mit Ergebnissen
- Automatische Artifact-Speicherung

### Phase 3: Team-Training 📅 (Optional)
- Developer Workshop: Cross-Compile Anforderungen
- Code-Review-Guidelines in Pull-Request Template
- Violation Statistics Dashboard

### Phase 4: Enforcement 📅 (Optional)
- Branch Protection mit Cross-Compile Check
- Required Status Checks für PRs
- Automatische CODEOWNERS für Cross-Compile Violations

---

## 🔍 RULE ÜBERSICHT

| Regel | Typ | Prüfung |
|------|------|--------|
| **RULE_1** | CRITICAL | Keine platform-spezifischen Headers |
| **RULE_2** | INFO | Whitelist allowed headers |
| **RULE_3** | CRITICAL | Platform-Conditionals haben `#else` |
| **RULE_4** | CRITICAL | Keine hartcodierten Pfade |
| **RULE_5** | CRITICAL | Keine OS-spezifischen Funktionen |
| **RULE_6** | HIGH | Compiler-Pragmas haben Fallback |
| **RULE_7** | HIGH | Keine Architecture-Annahmen |
| **RULE_8** | CRITICAL | Externe Libs in vcpkg.json |
| **RULE_9** | HIGH | CMake behandelt alle Plattformen |

---

## 📞 SUPPORT & TROUBLESHOOTING

### Hook läuft nicht?

```bash
# Berechtigungen prüfen
ls -la .git/hooks/pre-commit

# Sollte sein: -rwxr-xr-x (755)
chmod +x .git/hooks/pre-commit

# Python-Path prüfen
which python3
# Falls nicht gefunden: Vollständigen Pfad in Hook verwenden
```

### GitHub Actions funktioniert nicht?

```bash
# Workflow manuell prüfen
cat .github/workflows/cross-compile-review.yml

# In Repository Settings → Actions → "Allow ... actions"
# Stelle sicher GitHub Actions aktiviert ist
```

### Copilot Prompt wird nicht geladen?

```bash
# VSCode Settings überprüfen
# Settings UI → "copilot" suchen
# Oder direkt in settings.json prüfen
code ~/.config/Code/User/settings.json  # Linux/macOS
code %APPDATA%\Code\User\settings.json  # Windows
```

### Zu viele False Positives?

1. Prüfe `.copilot-cross-compile-rules.json` auf über-aggressive Patterns
2. Füge Whitelist-Ausnahmen in `ALLOWED_HEADERS_PREFIXES` hinzu
3. Öffne Issue: "Cross-Compile-Reviewer: False Positive in Rule X"

---

## 📈 STATISTICS & METRICS

Hook generiert optional Statistiken:

```json
{
  "total_prs_reviewed": 42,
  "violations_by_rule": {
    "RULE_1_headers": 5,
    "RULE_4_paths": 3,
    "RULE_5_apis": 12,
    ...
  },
  "approval_rate": "85%",
  "most_common_violation": "RULE_5_forbidden_os_apis"
}
```

Speichern unter: `cross-compile-violations-stats.json`

---

## ✅ VERIFICATION CHECKLIST

Nach Setup sollte Folgendes funktionieren:

- [ ] `python3 scripts/cross-compile-reviewer.py --help` zeigt Hilfe
- [ ] `git commit` mit problematischem Code wird blockiert
- [ ] `git commit` mit gutem Code durchläuft Hook
- [ ] VSCode Copilot kann `.copilot-cross-compile-prompt.md` laden
- [ ] GitHub Actions triggert auf PR-Updates
- [ ] PR erhält Copilot-Review-Kommentar innerhalb 30 Sekunden

---

## 🎓 NEXT STEPS

1. **Developers**: Lies [CROSS_COMPILE_REQUIREMENTS.md](CROSS_COMPILE_REQUIREMENTS.md)
2. **Team Lead**: Führe [Setup-Checkliste](#-verification-checklist) durch
3. **CI/CD Engineer**: Verifiziere GitHub Actions Workflow
4. **Code Reviewer**: Nutze Copilot-Prompt für manual Reviews

---

**Erstellt:** 2026-01-15  
**Version:** 1.0  
**Maintainer:** ThemisDB Team
