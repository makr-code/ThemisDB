# ThemisDB Downloads WordPress Plugin

Ein WordPress Plugin, das automatisch die neuesten ThemisDB Packages von GitHub abruft und als Download-Links mit SHA256-Checksums auf einer WordPress-Seite anzeigt. Das Plugin analysiert beim Speichern von Artikeln automatisch den Inhalt und erstellt passende Schlagwörter (Tags) und Kategorien.

## Features

- ✅ **Automatischer Download von GitHub Releases**: Ruft die neuesten Releases von GitHub automatisch ab
- ✅ **SHA256-Checksums Anzeige**: Zeigt SHA256-Hashes für alle Download-Dateien an
- ✅ **Download-Verifizierung**: Integriertes Tool zur Überprüfung der Datei-Integrität
- ✅ **Automatische Schlagwörter und Kategorien**: Analysiert Artikelinhalte und erstellt automatisch relevante Tags und Kategorien beim Speichern
- ✅ **Intelligente Textanalyse**: Verwendet Häufigkeit, Relevanz und Phrase-Erkennung für beste Ergebnisse
- ✅ **Mehrere Anzeigestile**: Standard, Kompakt, Tabellen-Ansicht
- ✅ **Plattform-Filter**: Zeige nur Windows, Linux, Docker oder andere Plattformen
- ✅ **Cache-System**: Reduziert API-Aufrufe durch intelligentes Caching
- ✅ **Responsive Design**: Funktioniert auf allen Geräten
- ✅ **Einfache Integration**: Shortcodes für schnelle Einbindung

## Installation

### Methode 1: Upload über WordPress Admin

1. Laden Sie das Plugin als ZIP-Datei herunter
2. Gehen Sie zu WordPress Admin → Plugins → Installieren
3. Klicken Sie auf "Plugin hochladen"
4. Wählen Sie die ZIP-Datei aus und klicken Sie auf "Jetzt installieren"
5. Aktivieren Sie das Plugin

### Methode 2: Manuelle Installation

1. Entpacken Sie das Plugin-Verzeichnis
2. Laden Sie den Ordner `themisdb-downloads` in das Verzeichnis `/wp-content/plugins/` hoch
3. Aktivieren Sie das Plugin über das "Plugins"-Menü in WordPress

### Methode 3: FTP Upload

1. Entpacken Sie das Plugin
2. Laden Sie den Ordner per FTP in `/wp-content/plugins/` hoch
3. Aktivieren Sie das Plugin in WordPress

## Konfiguration

Nach der Aktivierung:

1. Gehen Sie zu **Einstellungen → ThemisDB Downloads**
2. Konfigurieren Sie die folgenden Optionen:

### Einstellungen

| Einstellung | Beschreibung | Standard |
|-------------|--------------|----------|
| **GitHub Repository** | Das GitHub Repository im Format `owner/repository` | `makr-code/wordpressPlugins` |
| **GitHub Token** | Optional: Personal Access Token für höhere API-Limits | - |
| **Cache Dauer** | Wie lange Release-Daten gecacht werden sollen (in Sekunden) | `3600` (1 Stunde) |
| **Anzahl Releases** | Wie viele Releases angezeigt werden sollen | `10` |
| **Pre-Releases** | Beta- und Alpha-Versionen anzeigen | `Aus` |
| **Automatische Taxonomien** | Automatische Zuweisung von Schlagwörtern und Kategorien aktivieren | `Ein` |
| **Automatische Tags** | Tags automatisch erstellen und zuweisen | `Ein` |
| **Automatische Kategorien** | Kategorien automatisch erstellen und zuweisen | `Ein` |

### GitHub Token (Optional)

Ein GitHub Personal Access Token erhöht das API Rate Limit von 60 auf 5000 Anfragen pro Stunde.

**Token erstellen:**
1. Gehen Sie zu [GitHub Settings → Developer Settings → Personal Access Tokens](https://github.com/settings/tokens)
2. Klicken Sie auf "Generate new token (classic)"
3. Geben Sie eine Beschreibung ein (z.B. "WordPress ThemisDB Plugin")
4. Wählen Sie keine Berechtigungen (öffentliche Daten benötigen keine Scopes)
5. Klicken Sie auf "Generate token"
6. Kopieren Sie den Token und fügen Sie ihn in den Plugin-Einstellungen ein

## Verwendung

### Shortcodes

#### 1. Neueste Version anzeigen (Standard)

```
[themisdb_downloads]
```

Zeigt die neueste Version mit allen Download-Links und SHA256-Checksums an.

#### 2. Alle Releases anzeigen

```
[themisdb_downloads show="all"]
```

Zeigt die letzten 10 Releases (konfigurierbar in den Einstellungen).

#### 3. Nur bestimmte Plattform anzeigen

```
[themisdb_downloads platform="windows"]
[themisdb_downloads platform="linux"]
[themisdb_downloads platform="docker"]
```

Filtert Downloads nach Plattform.

#### 4. Kompakte Ansicht

```
[themisdb_downloads style="compact"]
```

Zeigt eine kompakte Liste mit Download-Links.

#### 5. Tabellen-Ansicht

```
[themisdb_downloads style="table"]
```

Zeigt Downloads in einer übersichtlichen Tabelle.

#### 6. Kombination von Optionen

```
[themisdb_downloads show="all" platform="linux" style="table" limit="5"]
```

#### 7. Nur Versionsnummer anzeigen

```
[themisdb_latest]
```

Zeigt nur die neueste Versionsnummer (z.B. "v1.4.0").

#### 8. Verifizierungs-Tool

```
[themisdb_verify]
```

Zeigt ein interaktives Tool zur Überprüfung der Download-Integrität.

#### 9. README anzeigen

```
[themisdb_readme]
```

Zeigt die README.md Datei der neuesten Version an.

**Optionen:**
- `version="latest"` - Neueste Version (Standard)
- `version="v1.4.0"` - Spezifische Version
- `style="default"` - HTML-formatiert (Standard)
- `style="raw"` - Unformatierter Text

**Beispiele:**
```
[themisdb_readme]                           # Neueste README
[themisdb_readme version="v1.3.4"]         # README für v1.3.4
[themisdb_readme style="raw"]              # Rohtext-Ansicht
```

#### 10. CHANGELOG anzeigen

```
[themisdb_changelog]
```

Zeigt die CHANGELOG.md oder RELEASE_NOTES.md Datei der neuesten Version an.

**Optionen:**
- `version="latest"` - Neueste Version (Standard)
- `version="v1.4.0"` - Spezifische Version
- `style="default"` - HTML-formatiert (Standard)
- `style="raw"` - Unformatierter Text

**Beispiele:**
```
[themisdb_changelog]                        # Neuestes CHANGELOG
[themisdb_changelog version="v1.3.4"]      # CHANGELOG für v1.3.4
[themisdb_changelog style="raw"]           # Rohtext-Ansicht
```

### Markdown-Unterstützung

README und CHANGELOG Dateien werden automatisch von Markdown nach HTML konvertiert mit voller Unterstützung für:

**Formatierungen:**
- **Fett**, *kursiv*, ~~durchgestrichen~~
- `Inline-Code` und Code-Blöcke mit Syntax-Highlighting
- Links und Bilder
- Listen (nummeriert und unnummeriert, mit Verschachtelung)
- Tabellen mit Ausrichtung
- Blockzitate
- Überschriften (H1-H6)
- Horizontale Trennlinien

**Erweiterte Features:**
- **Mermaid-Diagramme**: Eingebettete Diagramme werden automatisch gerendert
  ```
  ```mermaid
  graph TD
    A[Start] --> B[Ende]
  ```
  ```
- Sichere HTML-Escaping für XSS-Schutz
- Responsive Design für alle Geräte

## Beispiel-Seite

Erstellen Sie eine neue Seite mit folgendem Inhalt:

```
<h2>ThemisDB Downloads</h2>

<p>Aktuelle Version: [themisdb_latest]</p>

<h3>Neueste Version herunterladen</h3>
[themisdb_downloads]

<h3>Was ist neu?</h3>
[themisdb_changelog]

<h3>Download-Verifizierung</h3>
<p>Überprüfen Sie die Integrität Ihrer heruntergeladenen Datei:</p>
[themisdb_verify]

<h3>Dokumentation</h3>
[themisdb_readme]

<h3>Alle Versionen</h3>
[themisdb_downloads show="all" style="table" limit="5"]
```

## Download-Verifizierung

### Browser-basierte Verifizierung

Das Plugin enthält ein JavaScript-Tool zur Verifizierung:

1. Verwenden Sie den Shortcode `[themisdb_verify]`
2. Wählen Sie die heruntergeladene Datei aus
3. Kopieren Sie den SHA256-Hash aus der Download-Liste
4. Klicken Sie auf "Verifizieren"

### Kommandozeilen-Verifizierung

**Windows (PowerShell):**
```powershell
Get-FileHash -Algorithm SHA256 themis-1.4.0-windows-x64.zip | Format-List
```

**Linux/macOS:**
```bash
sha256sum themis-1.4.0-linux-x64.tar.gz
```

Vergleichen Sie den berechneten Hash mit dem angezeigten SHA256-Checksum.

## Automatische Schlagwörter und Kategorien

Das Plugin kann automatisch relevante Schlagwörter (Tags) und Kategorien aus dem Inhalt von Beiträgen und Seiten extrahieren und zuweisen.

### Funktionsweise

Das Plugin analysiert **Titel und Textinhalt** von Beiträgen und Seiten beim Speichern und verwendet dabei fortgeschrittene Textanalyse-Techniken:

- **Wortfrequenz-Analyse**: Häufig vorkommende Wörter werden als relevanter eingestuft
- **Titel-Gewichtung**: Wörter aus dem Titel erhalten höhere Priorität (3x Gewichtung)
- **Wortlängen-Bonus**: Längere Wörter (>6 Zeichen) werden als bedeutsamer gewertet
- **Stop-Word-Filterung**: Füllwörter (der, die, das, the, is, are, etc.) werden ausgeschlossen
- **Phrase-Erkennung**: Zusammenhängende Begriffe (2-3 Wörter) werden für Kategorien verwendet

#### Automatische Tags

Tags werden extrahiert basierend auf:
- **Häufigkeit**: Wie oft ein Wort im Text vorkommt
- **Relevanz**: Position im Titel, Wortlänge, Kapitalisierung
- **Best Practice**: Bis zu 15 relevanteste Begriffe werden als Tags vergeben

**Beispiel für Beitrag "ThemisDB Version 1.4.0 - Neue Release mit Windows Support":**
- ThemisDB (hohe Frequenz + im Titel)
- Version (im Titel)
- Windows (im Titel + Inhalt)
- Release (im Titel)
- Support (im Titel + Inhalt)
- Database (im Inhalt)
- Installation (im Inhalt)
- Performance (im Inhalt)

#### Automatische Kategorien

Kategorien werden extrahiert basierend auf:
- **Phrase-Analyse**: 2-3 Wort-Kombinationen werden identifiziert
- **Kontext-Relevanz**: Phrasen aus dem Titel haben höhere Priorität
- **Best Practice**: Bis zu 5 relevanteste Phrasen werden als Kategorien vergeben

**Beispiel-Kategorien:**
- ThemisDB Version
- Windows Support
- Neue Release
- Database Management

### Aktivierung und Konfiguration

1. Gehen Sie zu **Einstellungen → ThemisDB Downloads**
2. Scrollen Sie zum Abschnitt **"Automatische Schlagwörter und Kategorien"**
3. Aktivieren Sie die gewünschten Optionen:
   - **Automatische Taxonomien aktivieren**: Haupt-Schalter für die Funktion
   - **Automatische Schlagwörter (Tags)**: Tags aus Inhalt extrahieren und zuweisen
   - **Automatische Kategorien**: Kategorien aus Phrasen extrahieren und zuweisen

### Wann werden Taxonomien zugewiesen?

Taxonomien werden automatisch zugewiesen:
- **Beim Speichern** eines Beitrags oder einer Seite
- Gilt für alle Beiträge und Seiten (nicht nur solche mit Shortcodes)

### Vorhandene Taxonomien

- Das Plugin **fügt** Tags und Kategorien **hinzu**, ohne vorhandene zu entfernen
- Wenn bereits ThemisDB-Tags vorhanden sind, werden keine neuen hinzugefügt
- Sie können jederzeit manuell Tags und Kategorien bearbeiten

### Deaktivierung

Um die automatische Taxonomie-Zuweisung zu deaktivieren:
1. Gehen Sie zu **Einstellungen → ThemisDB Downloads**
2. Deaktivieren Sie "Automatische Taxonomien aktivieren"
3. Klicken Sie auf "Einstellungen speichern"

Bereits erstellte Tags und Kategorien bleiben erhalten, aber es werden keine neuen mehr erstellt.

## API-Limits

### Ohne Token
- **60 Anfragen pro Stunde** pro IP-Adresse
- Geeignet für kleine bis mittelgroße Websites

### Mit Token
- **5000 Anfragen pro Stunde**
- Empfohlen für größere Websites oder häufige Updates

**Empfehlung:** Verwenden Sie einen Cache von mindestens 1 Stunde (3600 Sekunden).

## Cache-Verwaltung

### Automatisches Caching

Das Plugin cached Release-Daten automatisch für die konfigurierte Dauer (Standard: 1 Stunde).

### Manuelles Cache-Leeren

- Gehen Sie zu **Einstellungen → ThemisDB Downloads**
- Klicken Sie auf "Cache leeren"

### Cache bei Plugin-Deaktivierung

Der Cache wird automatisch gelöscht, wenn das Plugin deaktiviert wird.

## Styling-Anpassungen

### CSS-Klassen

Das Plugin verwendet folgende CSS-Klassen:

- `.themisdb-downloads-container` - Haupt-Container
- `.themisdb-release` - Einzelnes Release
- `.download-item` - Download-Element
- `.sha256-hash` - SHA256-Hash
- `.copy-hash-button` - Hash-Kopier-Button

### Eigenes CSS hinzufügen

Fügen Sie in Ihrem Theme-CSS eigene Styles hinzu:

```css
.themisdb-downloads-container {
    /* Ihre Styles */
}

.download-item {
    /* Ihre Styles */
}
```

## Fehlerbehebung

### Problem: Keine Releases werden angezeigt

**Lösung:**
1. Überprüfen Sie die GitHub Repository-Einstellung
2. Stellen Sie sicher, dass das Repository öffentlich ist
3. Leeren Sie den Cache in den Plugin-Einstellungen
4. Überprüfen Sie die API-Verbindung auf der Einstellungsseite

### Problem: API Rate Limit erreicht

**Lösung:**
1. Fügen Sie einen GitHub Personal Access Token hinzu
2. Erhöhen Sie die Cache-Dauer
3. Warten Sie, bis das Rate Limit zurückgesetzt wird (jede Stunde)

### Problem: SHA256-Checksums werden nicht angezeigt

**Lösung:**
1. Stellen Sie sicher, dass das GitHub Release eine `SHA256SUMS.txt` Datei enthält
2. Die Datei muss das Format `hash  filename` haben
3. Leeren Sie den Cache und laden Sie die Seite neu

### Problem: Browser-Verifizierung funktioniert nicht

**Lösung:**
1. Verwenden Sie einen modernen Browser (Chrome, Firefox, Edge, Safari)
2. Stellen Sie sicher, dass JavaScript aktiviert ist
3. Für ältere Browser: Verwenden Sie die Kommandozeilen-Verifizierung

## Entwicklung & Anpassungen

### Dateistruktur

```
themisdb-downloads/
├── themisdb-downloads.php       # Haupt-Plugin-Datei
├── README.md                     # Diese Datei
├── includes/
│   ├── class-github-api.php     # GitHub API Handler
│   ├── class-admin.php          # Admin Panel
│   ├── class-shortcodes.php     # Shortcode Handler
│   └── class-taxonomy-manager.php # Taxonomy Manager (Auto Tags/Kategorien)
├── assets/
│   ├── css/
│   │   ├── style.css            # Frontend Styles
│   │   └── admin.css            # Admin Styles
│   └── js/
│       ├── script.js            # Frontend JavaScript
│       └── admin.js             # Admin JavaScript
└── languages/                    # Übersetzungen (zukünftig)
```

### Hooks & Filter

#### Filter

```php
// Ändere Cache-Dauer programmatisch
add_filter('themisdb_cache_duration', function($duration) {
    return 7200; // 2 Stunden
});

// Ändere API-Endpunkt
add_filter('themisdb_api_endpoint', function($endpoint, $repo) {
    return "https://api.github.com/repos/{$repo}/releases";
}, 10, 2);
```

#### Actions

```php
// Nach Cache-Aktualisierung
add_action('themisdb_cache_updated', function() {
    // Deine Aktion
});
```

## Sicherheit

- ✅ Alle Eingaben werden sanitized
- ✅ Alle Ausgaben werden escaped
- ✅ Nonce-Verifikation für AJAX-Anfragen
- ✅ Capability-Checks für Admin-Funktionen
- ✅ Kein direkter Dateizugriff möglich

## Kompatibilität

- **WordPress:** 5.0 oder höher
- **PHP:** 7.2 oder höher
- **Browser:** Alle modernen Browser (Chrome, Firefox, Safari, Edge)

## Support & Beiträge

- **GitHub Repository:** [makr-code/wordpressPlugins](https://github.com/makr-code/wordpressPlugins)
- **Issues:** [GitHub Issues](https://github.com/makr-code/wordpressPlugins/issues)
- **Dokumentation:** [docs/de/deployment/](../../docs/de/deployment/)

## Lizenz

Dieses Plugin ist unter der MIT-Lizenz lizenziert. Siehe [LICENSE](../../LICENSE) für Details.

## Credits

Entwickelt für das ThemisDB-Projekt.

## Changelog

### Version 1.2.0 (Januar 2026)
- ✅ **NEU: Automatische Schlagwörter und Kategorien**
- ✅ Automatische Tag-Erstellung basierend auf Release-Daten
- ✅ Automatische Kategorien-Erstellung
- ✅ Konfigurierbare Taxonomie-Einstellungen im Admin-Panel
- ✅ Intelligente Taxonomie-Zuweisung beim Speichern von Beiträgen

### Version 1.1.0 (Januar 2026)
- ✅ README und CHANGELOG Shortcodes
- ✅ Verbesserte Markdown-Darstellung
- ✅ Erweiterte Dokumentation

### Version 1.0.0 (Januar 2026)
- ✅ Erste Veröffentlichung
- ✅ GitHub API Integration
- ✅ SHA256-Checksums Anzeige
- ✅ Browser-basierte Verifizierung
- ✅ Mehrere Anzeigestile
- ✅ Cache-System
- ✅ Admin-Panel
- ✅ Responsive Design
