# README und CHANGELOG Anzeige - Neue Features (v1.1.0)

## Übersicht

Das WordPress Plugin wurde um die Möglichkeit erweitert, README.md und CHANGELOG.md Dateien aus GitHub Releases anzuzeigen.

## Neue Shortcodes

### [themisdb_readme]

Zeigt die README.md Datei aus dem Release an.

**Parameter:**
- `version` - Welche Version anzeigen (Standard: "latest")
- `style` - Anzeigestil (Standard: "default", Alternative: "raw")

**Beispiele:**
```php
// Neueste README anzeigen
[themisdb_readme]

// README einer bestimmten Version
[themisdb_readme version="v1.3.4"]

// README im Rohtext-Format
[themisdb_readme style="raw"]
```

### [themisdb_changelog]

Zeigt die CHANGELOG.md oder RELEASE_NOTES.md Datei aus dem Release an.

**Parameter:**
- `version` - Welche Version anzeigen (Standard: "latest")
- `style` - Anzeigestil (Standard: "default", Alternative: "raw")

**Beispiele:**
```php
// Neuestes CHANGELOG anzeigen
[themisdb_changelog]

// CHANGELOG einer bestimmten Version
[themisdb_changelog version="v1.3.4"]

// CHANGELOG im Rohtext-Format
[themisdb_changelog style="raw"]
```

## Funktionsweise

### Automatische Erkennung

Das Plugin erkennt automatisch folgende Dateien in den Release-Assets:
- `README.md`, `README_v*.md`
- `CHANGELOG.md`, `CHANGELOG_v*.md`
- `RELEASE_NOTES.md`, `RELEASE_NOTES_v*.md`

### Markdown-zu-HTML-Konvertierung

Die Dateien werden automatisch von Markdown zu HTML konvertiert:

**Unterstützte Elemente:**
- ✅ Überschriften (#, ##, ###)
- ✅ Fett (**text**) und Kursiv (*text*)
- ✅ Links [text](url)
- ✅ Code-Blöcke (```)
- ✅ Inline-Code (`code`)
- ✅ Listen (- oder * item)
- ✅ Absätze

### Styling

Die angezeigten README/CHANGELOG Dateien erhalten automatisches Styling:
- 📄 Formatierte Überschriften
- 📝 Lesbare Schriftgröße und Zeilenabstand
- 💻 Code-Blöcke mit Syntax-Highlighting
- 🔗 Klickbare Links
- 📋 Formatierte Listen und Tabellen

## Beispiel-Seite

```html
<h1>ThemisDB Dokumentation</h1>

<h2>Downloads</h2>
[themisdb_downloads]

<h2>Was ist neu?</h2>
[themisdb_changelog]

<h2>Dokumentation</h2>
[themisdb_readme]

<h2>Ältere Versionen</h2>
<h3>Version 1.3.4</h3>
[themisdb_readme version="v1.3.4"]
[themisdb_changelog version="v1.3.4"]
```

## Technische Details

### API-Integration

Die README/CHANGELOG Dateien werden beim Abrufen der Releases automatisch heruntergeladen:

1. Plugin ruft GitHub API ab (`/repos/makr-code/ThemisDB/releases`)
2. Für jedes Release werden die Assets durchsucht
3. README*.md und CHANGELOG*.md Dateien werden identifiziert
4. Dateien werden heruntergeladen und gecacht
5. Markdown wird zu HTML konvertiert

### Caching

- README/CHANGELOG werden zusammen mit den Release-Daten gecacht
- Cache-Dauer: Konfigurierbar (Standard: 1 Stunde)
- Manuelles Cache-Leeren möglich im Admin-Panel

### Performance

- Dateien werden nur einmal beim ersten Abruf heruntergeladen
- Cached für schnelle Anzeige
- Markdown-Konvertierung erfolgt bei jeder Anzeige (leichtgewichtig)

## Voraussetzungen

**Für automatische Anzeige benötigt:**
- README.md oder CHANGELOG.md muss als Asset zum GitHub Release hinzugefügt werden
- Dateien müssen im Markdown-Format vorliegen
- Unterstützte Dateinamen: README*.md, CHANGELOG*.md, RELEASE_NOTES*.md

**Release-Vorbereitung:**
```bash
# Beim Erstellen eines GitHub Release
gh release create v1.4.0 \
  themisdb-1.4.0-windows-x64.zip \
  themisdb-1.4.0-linux-x64.tar.gz \
  SHA256SUMS.txt \
  README.md \
  CHANGELOG.md
```

## Fehlerfälle

**Wenn keine README/CHANGELOG gefunden wird:**
```
"Kein README für diese Version gefunden."
"Kein CHANGELOG für diese Version gefunden."
```

Dies kann passieren wenn:
- Die Datei nicht als Asset zum Release hinzugefügt wurde
- Der Dateiname nicht dem erwarteten Muster entspricht
- Das Release nicht existiert

**Lösung:**
1. Überprüfen Sie, ob die Datei als Asset vorhanden ist
2. Verwenden Sie die richtigen Dateinamen (README.md, CHANGELOG.md)
3. Leeren Sie den Cache im Admin-Panel

## Kompatibilität

- ✅ Abwärtskompatibel mit Version 1.0.0
- ✅ Keine Breaking Changes
- ✅ Neue Shortcodes sind optional
- ✅ Bestehende Shortcodes funktionieren weiterhin

## Version

- **Version:** 1.1.0
- **Datum:** 7. Januar 2026
- **Commit:** b1b406d
