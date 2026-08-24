# Gap Scanner Logging — Quick Reference

## 🚀 Schnelleinstieg

```bash
# Starten Sie die vollständige Pipeline mit verbessertem Logging
cd c:\Projects\ThemisDB
python tools/gap_audit_pipeline_v3.py --repo . --output ai_working
```

## 📊 Was Sie sehen werden

Während der Ausführung zeigt die Konsole:

```
[...] [STAGE 1] - Running gap_scanner_v3 on all modules
  [OK] Security — Scanning for security vulnerabilities
  [OK] Memory Safety — Detecting buffer overflows
  [OK] Reliability — Error propagation, assertions
  ... (weitere Scanner-Schritte)
  [OK] [STAGE 1] Complete (48.3s) - Scanned and categorized all gaps

[...] [STAGE 2] - Aggregating scanner results
  > [########################################] 64/64 100% (consolidating)
  [OK] [STAGE 2] Complete (2.1s)
  
  Gap Analysis Results
  ─────────────────────────────────────────────────────────
    Total Gaps Found                    27,990
    Critical Findings                    3,904
    High Findings                       11,008
```

## 🛠️ Anpassungsoptionen

### Logging deaktivieren (nur Print-Fallback):
Falls Sie nur die Standard-Ausgabe möchten, entfernen Sie `gap_scanner_logging.py`:
```bash
mv tools/gap_scanner_logging.py tools/gap_scanner_logging.py.bak
```

### Farben deaktivieren (für Log-Dateien):
```bash
# Ausgabe in Datei umleiten
python tools/gap_audit_pipeline_v3.py --repo . --output ai_working > scan.log 2>&1
```

## 📋 Log-Ausgabe-Struktur

```
Phase          Status    Details
─────────────────────────────────────────────────────────────
[STAGE 1]      [...]     Scanner-Ausführung im Gange
  [OK]         Task      Untershritt erfolgreich
  [OK]         Task      Untershritt erfolgreich
[OK]           STAGE 1   Complete (45.2s)

[STAGE 2]      [...]     Aggregation im Gange
  > [###---]   Progress  64/64 100%
[OK]           STAGE 2   Complete (2.1s)

...

Summary        Stats     Zusammenfassung pro Kategorie
Timing         Report    Gesamtausführungszeit
```

## 🔍 Log-Interpretation

| Symbol | Bedeutung | Aktion |
|--------|-----------|--------|
| `[OK]` | Erfolgreich | ✓ Weiter |
| `[...]` | In Progress | ⏳ Warten |
| `[FAIL]` | Fehler | ❌ Diagnostizieren |
| `[WARN]` | Warnung | ⚠️ Überprüfen |
| `[SKIP]` | Übersprungen | ⊘ OK |
| `> [###]` | Fortschrittsbalken | 📊 Anteil anzeigen |

## ⏱️ Timing verstehen

```
Pipeline Execution Timeline
─────────────────────────────────────────────────────────
  gap_scanner_v3.py execution              48.3s ← Scanner läuft
  Summary aggregation                       2.1s ← Daten zusammenfassen
  Module documentation generation           8.7s ← Docs generieren
  Header writer                           12.4s ← Header aktualisieren
─────────────────────────────────────────────────────────
  Total Elapsed                          1m 11.5s ← Gesamtdauer
```

## 🐛 Debugging

### Problem: Encoding-Fehler auf Windows
**Lösung**: Logger nutzt automatisch ASCII-Fallback. Falls noch Fehler:
```bash
# Explizit UTF-8 setzen
$env:PYTHONIOENCODING = 'utf-8'
python tools/gap_audit_pipeline_v3.py --repo . --output ai_working
```

### Problem: Keine Fortschrittsanzeige
**Mögliche Ursachen**:
- Ausgabe wird zu Datei umgeleitet (`> file.log`)
- Python läuft in nicht-interaktivem Terminal
- **Lösung**: Terminal-Ausgabe aktiv lassen

### Problem: Zu viel Ausgabe
**Lösung**: Ausgabe in Datei umleiten:
```bash
python tools/gap_audit_pipeline_v3.py --repo . --output ai_working 2>&1 | tee scan.log
```

## 📞 Support

Für weitere Details siehe: [SCANNER_LOGGING_ENHANCEMENT_2026-06-02.md](ai_working/SCANNER_LOGGING_ENHANCEMENT_2026-06-02.md)

---

**Last Updated**: 2026-06-02  
**Scanner Version**: v3.1 with Logging  
**Status**: ✅ Production Ready
