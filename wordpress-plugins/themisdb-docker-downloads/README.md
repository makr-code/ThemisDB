# ThemisDB Docker Downloads WordPress Plugin

Ein WordPress Plugin, das automatisch die neuesten ThemisDB Docker Images von Docker Hub abruft und als Download-Links mit SHA256-Digests auf einer WordPress-Seite anzeigt.

## Features

- ✅ **Automatischer Download von Docker Hub**: Ruft die neuesten Docker Images automatisch ab
- ✅ **SHA256-Digests Anzeige**: Zeigt SHA256-Hashes für alle Image-Architekturen an
- ✅ **Multi-Architektur Support**: Unterstützt amd64, arm64, arm, i386 und weitere Architekturen
- ✅ **Mehrere Anzeigestile**: Standard, Kompakt, Tabellen-Ansicht
- ✅ **Architektur-Filter**: Zeige nur spezifische Architekturen (amd64, arm64, etc.)
- ✅ **Cache-System**: Reduziert API-Aufrufe durch intelligentes Caching
- ✅ **Responsive Design**: Funktioniert auf allen Geräten
- ✅ **Einfache Integration**: Shortcodes für schnelle Einbindung
- ✅ **One-Click Copy**: Docker Pull Commands mit einem Klick kopieren

## Installation

### Methode 1: Upload über WordPress Admin

1. Laden Sie das Plugin als ZIP-Datei herunter
2. Gehen Sie zu WordPress Admin → Plugins → Installieren
3. Klicken Sie auf "Plugin hochladen"
4. Wählen Sie die ZIP-Datei aus und klicken Sie auf "Jetzt installieren"
5. Aktivieren Sie das Plugin

### Methode 2: Manuelle Installation

1. Entpacken Sie das Plugin-Verzeichnis
2. Laden Sie den Ordner `themisdb-docker-downloads` in das Verzeichnis `/wp-content/plugins/` hoch
3. Aktivieren Sie das Plugin über das "Plugins"-Menü in WordPress

### Methode 3: FTP Upload

1. Entpacken Sie das Plugin
2. Laden Sie den Ordner per FTP in `/wp-content/plugins/` hoch
3. Aktivieren Sie das Plugin in WordPress

## Konfiguration

Nach der Aktivierung:

1. Gehen Sie zu **Einstellungen → ThemisDB Docker**
2. Konfigurieren Sie die folgenden Optionen:

### Einstellungen

| Einstellung | Beschreibung | Standard |
|-------------|--------------|----------|
| **Docker Hub Namespace** | Der Namespace auf Docker Hub | `themisdb` |
| **Docker Repository Name** | Der Repository-Name | `themisdb` |
| **Cache Dauer** | Wie lange Docker Hub Daten gecacht werden sollen (in Sekunden) | `3600` (1 Stunde) |
| **Anzahl Tags** | Wie viele Tags angezeigt werden sollen | `10` |
| **Docker Hub Token** | Optional: Personal Access Token für höhere API-Limits | - |

### Docker Hub Token (Optional)

Ein Docker Hub Personal Access Token erhöht das API Rate Limit.

**Token erstellen:**
1. Gehen Sie zu [Docker Hub Account Settings → Security](https://hub.docker.com/settings/security)
2. Klicken Sie auf "New Access Token"
3. Geben Sie eine Beschreibung ein (z.B. "WordPress ThemisDB Plugin")
4. Wählen Sie "Read-only" als Berechtigung
5. Klicken Sie auf "Generate"
6. Kopieren Sie den Token und fügen Sie ihn in den Plugin-Einstellungen ein

## Verwendung

### Shortcodes

#### 1. Neueste Tags anzeigen (Standard)

```
[themisdb_docker_tags]
```

Zeigt die neuesten Docker Tags mit allen Informationen an.

#### 2. Alle Tags anzeigen

```
[themisdb_docker_tags show="all"]
```

Zeigt die letzten 10 Tags (konfigurierbar in den Einstellungen).

#### 3. Nur bestimmte Architektur anzeigen

```
[themisdb_docker_tags architecture="amd64"]
[themisdb_docker_tags architecture="arm64"]
```

Filtert Tags nach Architektur.

#### 4. Kompakte Ansicht

```
[themisdb_docker_tags style="compact"]
```

Zeigt eine kompakte Liste mit Docker Pull Commands.

#### 5. Tabellen-Ansicht

```
[themisdb_docker_tags style="table"]
```

Zeigt Tags in einer übersichtlichen Tabelle.

#### 6. Kombination von Optionen

```
[themisdb_docker_tags show="all" architecture="amd64" style="table" limit="5"]
```

#### 7. Nur Tag-Name anzeigen

```
[themisdb_docker_latest]
```

Zeigt nur den neuesten Tag-Namen (z.B. "latest" oder "v1.4.0").

## Beispiel-Seite

Erstellen Sie eine neue Seite mit folgendem Inhalt:

```
<h2>ThemisDB Docker Images</h2>

<p>Neuester Tag: [themisdb_docker_latest]</p>

<h3>Docker Images herunterladen</h3>
[themisdb_docker_tags]

<h3>Alle verfügbaren Tags</h3>
[themisdb_docker_tags show="all" style="table" limit="5"]

<h3>ARM64 Images</h3>
[themisdb_docker_tags architecture="arm64" style="compact"]
```

## Docker Image Verifizierung

### Image Digest Verifizierung

Docker Images können über ihren SHA256-Digest verifiziert werden:

```bash
# Pull Image mit spezifischem Digest
docker pull themisdb/themisdb@sha256:abc123...

# Image Digest überprüfen
docker inspect themisdb/themisdb:latest --format='{{.RepoDigests}}'
```

### Docker Content Trust

Für zusätzliche Sicherheit kann Docker Content Trust aktiviert werden:

```bash
export DOCKER_CONTENT_TRUST=1
docker pull themisdb/themisdb:latest
```

## API-Limits

### Ohne Token
- **Anonyme Nutzung**: Begrenzte Anfragen
- Geeignet für kleine Websites

### Mit Token
- **Mit Token**: Höhere Rate Limits
- Empfohlen für größere Websites oder häufige Updates

**Empfehlung:** Verwenden Sie einen Cache von mindestens 1 Stunde (3600 Sekunden).

## Cache-Verwaltung

### Automatisches Caching

Das Plugin cached Docker Hub Daten automatisch für die konfigurierte Dauer (Standard: 1 Stunde).

### Manuelles Cache-Leeren

- Gehen Sie zu **Einstellungen → ThemisDB Docker**
- Klicken Sie auf "Cache leeren"

### Cache bei Plugin-Deaktivierung

Der Cache wird automatisch gelöscht, wenn das Plugin deaktiviert wird.

## Styling-Anpassungen

### CSS-Klassen

Das Plugin verwendet folgende CSS-Klassen:

- `.themisdb-docker-downloads-container` - Haupt-Container
- `.themisdb-docker-tag` - Einzelner Tag
- `.tag-header` - Tag-Header
- `.pull-command` - Pull Command Container
- `.docker-command` - Docker Command Code
- `.copy-command-btn` - Copy Button
- `.architecture-list` - Architektur-Liste
- `.digest-value` - SHA256 Digest

### Eigenes CSS hinzufügen

Fügen Sie in Ihrem Theme-CSS eigene Styles hinzu:

```css
.themisdb-docker-downloads-container {
    /* Ihre Styles */
}

.themisdb-docker-tag {
    /* Ihre Styles */
}
```

## Fehlerbehebung

### Problem: Keine Tags werden angezeigt

**Lösung:**
1. Überprüfen Sie die Docker Hub Namespace und Repository-Einstellungen
2. Stellen Sie sicher, dass das Repository öffentlich ist
3. Leeren Sie den Cache in den Plugin-Einstellungen
4. Überprüfen Sie die API-Verbindung auf der Einstellungsseite

### Problem: API Rate Limit erreicht

**Lösung:**
1. Fügen Sie einen Docker Hub Personal Access Token hinzu
2. Erhöhen Sie die Cache-Dauer
3. Reduzieren Sie die Anzahl der angezeigten Tags

### Problem: SHA256-Digests werden nicht angezeigt

**Lösung:**
1. Stellen Sie sicher, dass das Docker Image existiert
2. Leeren Sie den Cache und laden Sie die Seite neu
3. Überprüfen Sie die Docker Hub API-Verbindung

## Entwicklung & Anpassungen

### Dateistruktur

```
themisdb-docker-downloads/
├── themisdb-docker-downloads.php    # Haupt-Plugin-Datei
├── README.md                         # Diese Datei
├── includes/
│   ├── class-dockerhub-api.php      # Docker Hub API Handler
│   ├── class-admin.php              # Admin Panel
│   └── class-shortcodes.php         # Shortcode Handler
├── assets/
│   ├── css/
│   │   ├── style.css                # Frontend Styles
│   │   └── admin.css                # Admin Styles
│   └── js/
│       ├── script.js                # Frontend JavaScript
│       └── admin.js                 # Admin JavaScript
└── languages/                        # Übersetzungen (zukünftig)
```

### Hooks & Filter

#### Filter

```php
// Ändere Cache-Dauer programmatisch
add_filter('themisdb_docker_cache_duration', function($duration) {
    return 7200; // 2 Stunden
});
```

#### Actions

```php
// Nach Cache-Aktualisierung
add_action('themisdb_docker_cache_updated', function() {
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

Entwickelt für das ThemisDB-Projekt, analog zum GitHub Downloads Plugin.

## Changelog

### Version 1.0.0 (Januar 2026)
- ✅ Erste Veröffentlichung
- ✅ Docker Hub API Integration
- ✅ SHA256-Digests Anzeige
- ✅ Multi-Architektur Support
- ✅ Mehrere Anzeigestile
- ✅ Cache-System
- ✅ Admin-Panel
- ✅ Responsive Design
- ✅ One-Click Copy für Pull Commands
