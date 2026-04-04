# Dokumentations-Validierungstools

## Übersicht

ThemisDB verwendet automatisierte Validierungstools, um die Qualität und Konsistenz der Dokumentation sicherzustellen. Diese Tools prüfen auf strukturelle Probleme, defekte Links und TOC-Konsistenz, bevor Code zusammengeführt wird.

## Validierungstools

### 1. Dokumentations-Linter (`docs-lint.py`)

Der Dokumentations-Linter prüft auf:

- **Überschriften-Hierarchie**: Stellt sicher, dass Überschriften ordnungsgemäß verschachtelt sind
- **Markdown-Syntax**: Validiert die korrekte Markdown-Formatierung
- **Link-Format**: Prüft auf leeren Link-Text oder URLs
- **Dateinamen**: Validiert Namenskonventionen (keine Leerzeichen, Kleinbuchstaben-Erweiterungen)
- **Erforderliche Abschnitte**: Stellt sicher, dass wichtige Dateien notwendige Abschnitte enthalten
- **Trailing Whitespace**: Erkennt unnötige nachgestellte Leerzeichen

#### Verwendung

```bash
# Alle Dokumentation linten
python3 scripts/docs-lint.py

# Bestimmte Pfade linten
python3 scripts/docs-lint.py docs/de compendium

# JSON-Report generieren
python3 scripts/docs-lint.py --format json --output lint-report.json

# Bei Warnungen fehlschlagen
python3 scripts/docs-lint.py --fail-on-warnings
```

#### Häufige Fehler und Lösungen

| Fehler | Lösung |
|--------|--------|
| Fehlender Abstand nach Überschriftsmarkierung | Abstand hinzufügen: `##Überschrift` → `## Überschrift` |
| Überschriftenebene übersprungen | Keine Ebenen überspringen: `# H1` → `### H3` sollte `# H1` → `## H2` sein |
| Leerer Link-Text | Beschreibenden Text bereitstellen: `[]()` → `[Link-Text](url)` |
| Dateiname enthält Leerzeichen | Bindestriche oder Unterstriche verwenden: `meine datei.md` → `meine-datei.md` |
| Trailing Whitespace | Leerzeichen am Zeilenende entfernen |

### 2. Link-Checker (`link-check.py`)

Der Link-Checker validiert:

- **Interne Links**: Überprüft, ob Dateien und Anker existieren
- **Anker-Links**: Prüft, ob Überschriften-Anker gültig sind
- **Externe Links**: Basis-Format-Validierung (vollständige HTTP-Prüfungen über GitHub Actions)
- **Relative Pfade**: Stellt korrekte Pfadauflösung sicher

#### Verwendung

```bash
# Alle Links prüfen
python3 scripts/link-check.py

# Bestimmte Pfade prüfen
python3 scripts/link-check.py docs compendium

# Nur interne Links prüfen
python3 scripts/link-check.py --internal-only

# JSON-Report generieren
python3 scripts/link-check.py --format json --output link-report.json
```

#### Häufige Fehler und Lösungen

| Fehler | Lösung |
|--------|--------|
| Defekter interner Link: Zieldatei nicht gefunden | Link auf korrekten Dateipfad aktualisieren |
| Anker in Zieldatei nicht gefunden | Prüfen, ob Überschrift existiert oder Ankernamen korrigieren |
| Externer Link enthält Leerzeichen | Leerzeichen kodieren: `meine url` → `meine%20url` |
| Ungültiges externes Link-Format | Korrekte URL sicherstellen: `www.example.com` → `https://www.example.com` |

### 3. TOC-Validator (`toc-check.py`)

Der TOC-Validator prüft:

- **Navigationsstruktur**: Validiert mkdocs.yml Navigationseinträge
- **Dateireferenzen**: Stellt sicher, dass alle Nav-Einträge auf existierende Dateien zeigen
- **Verwaiste Dateien**: Erkennt wichtige Dateien, die nicht in der Navigation sind
- **Doppelte Einträge**: Warnt vor Dateien, die mehrfach erscheinen
- **Kreuzverweise**: Validiert Konsistenz zwischen docs und compendium

#### Verwendung

```bash
# Alle mkdocs-Configs validieren
python3 scripts/toc-check.py

# Bestimmte Config validieren
python3 scripts/toc-check.py --configs mkdocs.yml

# JSON-Report generieren
python3 scripts/toc-check.py --format json --output toc-report.json
```

#### Häufige Fehler und Lösungen

| Fehler | Lösung |
|--------|--------|
| In Nav referenzierte Datei nicht gefunden | Eintrag entfernen oder fehlende Datei erstellen |
| Wichtige Datei nicht in Navigation | Datei zu mkdocs.yml nav-Abschnitt hinzufügen |
| Datei erscheint mehrfach in Navigation | Doppelte Referenzen entfernen |
| Fehlende Dateien | Dateien erstellen oder Navigation aktualisieren |

## GitHub Actions Integration

### Workflow: `documentation-validation.yml`

Der Validierungs-Workflow läuft automatisch bei:

- **Pull Requests**, die Dokumentation betreffen
- **Pushes** auf main, develop oder release-Branches
- **Manuellem Trigger** via workflow_dispatch

### Validierungs-Jobs

1. **docs-lint**: Führt Dokumentations-Linting durch
2. **link-check**: Validiert interne Links
3. **external-link-check**: Prüft externe Links (nur main/develop)
4. **toc-validation**: Validiert TOC-Struktur
5. **validation-summary**: Aggregiert Ergebnisse

### Erforderliche Status-Checks

Die folgenden Checks müssen vor dem Merge bestehen:

- ✅ Documentation Linting
- ✅ Link Validation
- ✅ TOC Validation

### Ergebnisse anzeigen

Ergebnisse sind an mehreren Stellen verfügbar:

1. **Job Summary**: In GitHub Actions Job-Zusammenfassung ansehen
2. **Artifacts**: JSON-Reports für detaillierte Analyse herunterladen
3. **PR Comments**: Automatische Kommentare bei Pull Requests (falls aktiviert)

## Lokale Entwicklung

### Pre-Commit-Checks

Validierung lokal vor dem Commit ausführen:

```bash
# Alle Checks ausführen
./scripts/validate-docs.sh

# Oder einzeln ausführen
python3 scripts/docs-lint.py
python3 scripts/link-check.py
python3 scripts/toc-check.py
```

### Abhängigkeiten installieren

```bash
# Python-Abhängigkeiten
pip install pyyaml

# Für externe Link-Prüfung (optional)
npm install -g markdown-link-check
```

## Konfiguration

### Linter-Konfiguration

Der Linter verwendet integrierte Regeln. Zum Anpassen:

1. `scripts/docs-lint.py` bearbeiten
2. Prüffunktionen modifizieren
3. Benutzerdefinierte Validierungsregeln hinzufügen

### Link-Checker-Konfiguration

Externe Link-Prüfung verwendet `.markdown-link-check.json`:

```json
{
  "ignorePatterns": [
    {"pattern": "^https://github.com/.*/issues/[0-9]+$"},
    {"pattern": "^https://github.com/.*/pull/[0-9]+$"}
  ],
  "timeout": "5s",
  "retryOn429": true,
  "aliveStatusCodes": [200, 206, 301, 302, 307, 308]
}
```

### Ausschlüsse

Standardmäßig sind diese Pfade ausgeschlossen:

- `ARCHIVED/`
- `archive/`
- `node_modules/`
- `.git/`
- `site/`
- `build/`

Um Ausschlüsse hinzuzufügen:

```bash
python3 scripts/docs-lint.py --exclude ARCHIVED archive temp
```

## Fehlerbehebung

### Validierung in CI fehlgeschlagen

1. **Job-Logs prüfen**: GitHub Actions Logs überprüfen
2. **Artifacts herunterladen**: Detaillierte JSON-Reports abrufen
3. **Lokal ausführen**: Problem mit gleichem Befehl reproduzieren
4. **Probleme beheben**: Fehler und Warnungen adressieren
5. **Fixes committen**: Änderungen pushen, um erneute Validierung auszulösen

### Falsch-Positive

Bei Falsch-Positiven:

1. **Regel überprüfen**: Prüfen, ob sie angemessen ist
2. **Ausschluss hinzufügen**: Bestimmte Muster bei Bedarf ausschließen
3. **Problem melden**: GitHub Issue für inkorrekte Validierung öffnen

### Performance-Probleme

Für große Dokumentations-Sets:

1. **Bereich begrenzen**: Nur geänderte Dateien validieren
2. **Ausschlüsse verwenden**: Unnötige Verzeichnisse überspringen
3. **Parallele Ausführung**: Checks aufteilen erwägen

## Fehlerreferenz

### Kritische Fehler (blockieren Merge)

- Defekte interne Links
- Fehlende Dateien in Navigation
- Ungültige Markdown-Syntax
- Dateikodierungsfehler

### Warnungen (informativ)

- Überschriften-Hierarchieprobleme
- Trailing Whitespace
- Verwaiste Dateien
- Ankerprobleme

## Best Practices

1. **Validierung lokal ausführen** vor dem Push
2. **Fehler sofort beheben** - keine Probleme ansammeln
3. **Warnungen überprüfen** - sie weisen oft auf echte Probleme hin
4. **TOC aktuell halten** beim Hinzufügen/Entfernen von Dateien
5. **Beschreibenden Link-Text verwenden** für Barrierefreiheit
6. **Namenskonventionen befolgen** für Konsistenz
7. **Anker testen** beim Verlinken auf bestimmte Abschnitte

## Support

Bei Fragen oder Problemen:

1. Diese Dokumentation prüfen
2. Fehlermeldungen sorgfältig lesen
3. GitHub Issue mit Details öffnen
4. Dokumentations-Team kontaktieren

## Versionshistorie

- **v1.0** (2026-01): Erste Implementierung
  - Dokumentations-Linting
  - Link-Validierung
  - TOC-Prüfung
  - GitHub Actions Integration
