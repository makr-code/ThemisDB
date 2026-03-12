# Installation Guide - ThemisDB Wiki Integration Plugin

## Inhaltsverzeichnis / Table of Contents

1. [Systemanforderungen](#systemanforderungen--system-requirements)
2. [Installationsmethoden](#installationsmethoden--installation-methods)
3. [Konfiguration](#konfiguration--configuration)
4. [Erste Schritte](#erste-schritte--first-steps)
5. [Troubleshooting](#troubleshooting)

---

## Systemanforderungen / System Requirements

### Mindestanforderungen / Minimum Requirements

- **WordPress**: Version 5.0 oder höher
- **PHP**: Version 7.4 oder höher
- **MySQL**: Version 5.6 oder höher (oder MariaDB 10.0+)
- **Speicherplatz**: 5 MB für Plugin-Dateien
- **Internet-Verbindung**: Für GitHub API-Zugriff erforderlich

### Empfohlene Anforderungen / Recommended Requirements

- **WordPress**: Version 6.0+
- **PHP**: Version 8.0+
- **HTTPS**: SSL-Zertifikat installiert
- **Caching**: Object Cache (Redis/Memcached) für bessere Performance

---

## Installationsmethoden / Installation Methods

### Methode 1: WordPress Admin-Panel (Empfohlen / Recommended)

1. **Plugin hochladen:**
   - Gehen Sie zu **WordPress Admin → Plugins → Installieren**
   - Klicken Sie auf **Plugin hochladen**
   - Wählen Sie die `themisdb-wiki-integration.zip` Datei
   - Klicken Sie auf **Jetzt installieren**

2. **Plugin aktivieren:**
   - Nach der Installation auf **Plugin aktivieren** klicken
   - Sie werden zur Plugin-Liste weitergeleitet

3. **Konfiguration öffnen:**
   - Gehen Sie zu **Einstellungen → ThemisDB Wiki**
   - Konfigurieren Sie die Einstellungen (siehe unten)

### Methode 2: FTP/SFTP Upload

1. **Dateien vorbereiten:**
   - Entpacken Sie `themisdb-wiki-integration.zip`
   - Sie erhalten den Ordner `themisdb-wiki-integration`

2. **Per FTP hochladen:**
   ```bash
   # Verbinden Sie sich mit Ihrem FTP-Client (z.B. FileZilla)
   # Navigieren Sie zu: /wp-content/plugins/
   # Laden Sie den Ordner themisdb-wiki-integration hoch
   ```

3. **Plugin aktivieren:**
   - Gehen Sie zu **WordPress Admin → Plugins**
   - Suchen Sie **ThemisDB Wiki Integration**
   - Klicken Sie auf **Aktivieren**

### Methode 3: Von GitHub (Entwickler)

```bash
# 1. Navigieren Sie zum WordPress-Plugin-Verzeichnis
cd /pfad/zu/wordpress/wp-content/plugins/

# 2. ThemisDB Repository klonen
git clone https://github.com/makr-code/wordpressPlugins.git themisdb-repo

# 3. Plugin-Ordner kopieren
cp -r themisdb-repo/tools/themisdb-wiki-integration ./

# 4. Repository-Clone löschen
rm -rf themisdb-repo

# 5. Berechtigungen setzen
chmod -R 755 themisdb-wiki-integration
```

### Methode 4: WP-CLI (Kommandozeile)

```bash
# 1. Plugin-Ordner ins Plugin-Verzeichnis kopieren
cp -r /pfad/zu/themisdb-wiki-integration /pfad/zu/wordpress/wp-content/plugins/

# 2. Plugin mit WP-CLI aktivieren
wp plugin activate themisdb-wiki-integration

# 3. Status überprüfen
wp plugin list
```

---

## Konfiguration / Configuration

### Schritt 1: Admin-Panel öffnen

1. Gehen Sie zu **WordPress Admin → Einstellungen → ThemisDB Wiki**
2. Sie sehen das Konfigurations-Panel

### Schritt 2: GitHub-Einstellungen

#### GitHub Repository
```
Standardwert: makr-code/wordpressPlugins
Format: besitzer/repository-name
Beispiele:
  - makr-code/wordpressPlugins
  - your-org/your-repo
```

#### Branch
```
Standardwert: main
Alternativen: develop, feature/docs, release/v1.4
```

#### Dokumentationspfad
```
Standardwert: docs
Beispiele:
  - docs
  - documentation
  - wiki
  - content/docs
```

#### Standard-Sprache
```
Optionen:
  - de (Deutsch)
  - en (English)
  - fr (Français)
```

### Schritt 3: GitHub Token (Optional)

**Warum ein Token?**
- Höhere API Rate Limits (5.000 statt 60 Requests/Stunde)
- Zugriff auf private Repositories
- Stabilere Performance

**Token erstellen:**

1. Gehen Sie zu https://github.com/settings/tokens
2. Klicken Sie auf **Generate new token** → **Generate new token (classic)**
3. **Name**: "ThemisDB Wiki WordPress Plugin"
4. **Scopes auswählen:**
   - ✅ `public_repo` (für öffentliche Repos)
   - ✅ `repo` (für private Repos)
5. Klicken Sie auf **Generate token**
6. **Kopieren Sie das Token** (wird nur einmal angezeigt!)
7. Fügen Sie es in WordPress unter **GitHub Token** ein

**Sicherheitshinweis:** Behandeln Sie GitHub Tokens wie Passwörter!

### Schritt 4: Auto-Sync konfigurieren

**Auto-Sync aktivieren:**
- ✅ Checkbox aktivieren
- Dokumentation wird automatisch stündlich aktualisiert
- Cache wird geleert und neu geladen

**Auto-Sync deaktivieren:**
- ❌ Checkbox deaktivieren
- Manuelle Synchronisierung über "Sync Now" Button

### Schritt 5: Einstellungen speichern

1. Klicken Sie auf **Änderungen speichern**
2. Sie sehen eine Erfolgsmeldung
3. Testen Sie die Konfiguration mit "Sync Now"

---

## Erste Schritte / First Steps

### Schritt 1: Testseite erstellen

1. **Neue Seite erstellen:**
   - Gehen Sie zu **Seiten → Erstellen**
   - Titel: "ThemisDB Dokumentation"

2. **Shortcode einfügen:**
   ```php
   [themisdb_wiki file="README.md" lang="de"]
   ```

3. **Seite veröffentlichen:**
   - Klicken Sie auf **Veröffentlichen**
   - Öffnen Sie die Seite im Frontend

4. **Ergebnis prüfen:**
   - Sie sollten die README.md aus GitHub sehen
   - Formatierung sollte korrekt sein

### Schritt 2: Dokumentations-Hub erstellen

```php
<!-- Seite: Dokumentation -->
<h1>ThemisDB Dokumentation</h1>
<p>Willkommen zur ThemisDB Dokumentation! Wählen Sie einen Bereich:</p>

[themisdb_docs lang="de" layout="grid"]
```

### Schritt 3: Feature-Seite erstellen

```php
<!-- Seite: Features -->
[themisdb_wiki file="features/FEATURES.md" lang="de" show_toc="yes"]
```

### Schritt 4: Installation Guide

```php
<!-- Seite: Installation -->
[themisdb_wiki file="guides/INSTALLATION.md" lang="en"]
```

### Schritt 5: Navigation anpassen

**Im Theme (functions.php) oder Custom HTML Widget:**

```php
<nav class="themisdb-docs-nav">
    <ul>
        <li><a href="/documentation/">Dokumentation</a></li>
        <li><a href="/features/">Features</a></li>
        <li><a href="/installation/">Installation</a></li>
        <li><a href="/api-reference/">API Reference</a></li>
    </ul>
</nav>
```

---

## Troubleshooting

### Problem 1: "GitHub API returned status code 404"

**Ursache:** Falscher Repository-Name, Branch oder Dateipfad

**Lösung:**
```
1. Überprüfen Sie Einstellungen → ThemisDB Wiki:
   - Repository: makr-code/wordpressPlugins ✅
   - Branch: main (nicht master!) ✅
   - Docs Path: docs ✅

2. Testen Sie manuell:
   https://github.com/makr-code/wordpressPlugins/blob/main/docs/README.md
   
3. Wenn Datei existiert, überprüfen Sie Schreibweise
```

### Problem 2: "Rate limit exceeded"

**Ursache:** Zu viele API-Requests ohne Token (max. 60/Stunde)

**Lösung:**
```
1. GitHub Personal Access Token erstellen (siehe oben)
2. Token in Plugin-Einstellungen einfügen
3. "Sync Now" klicken
4. Rate Limit erhöht sich auf 5.000/Stunde
```

### Problem 3: Dokumentation wird nicht aktualisiert

**Ursache:** Cache ist noch aktiv

**Lösung:**
```
1. Admin-Panel → ThemisDB Wiki öffnen
2. "Sync Now" Button klicken
3. Warten bis "Cache cleared successfully" erscheint
4. Seite im Frontend neu laden (Strg+F5)
```

### Problem 4: Styling-Probleme

**Ursache:** Theme-CSS überschreibt Plugin-Styles

**Lösung:**
```css
/* In Theme's custom CSS oder Child Theme */
.themisdb-wiki-container {
    all: initial; /* Reset all styles */
    display: block;
}

/* Dann Plugin-Styles neu anwenden */
.themisdb-wiki-container * {
    box-sizing: border-box;
}
```

### Problem 5: Shortcode wird als Text angezeigt

**Ursache:** Plugin nicht aktiviert oder Shortcode-Syntax falsch

**Lösung:**
```
1. Plugins → Installed Plugins
2. Prüfen ob "ThemisDB Wiki Integration" aktiviert ist
3. Shortcode-Syntax überprüfen:
   ✅ [themisdb_wiki file="README.md"]
   ❌ [themisdb-wiki file="README.md"]
   ❌ [themisdb_wiki file=README.md]
```

### Problem 6: "Permission denied" / "Unauthorized"

**Ursache:** Private Repository ohne Token oder falsches Token

**Lösung:**
```
1. Für private Repos: GitHub Token mit 'repo' Scope erstellen
2. Token in Plugin-Einstellungen einfügen
3. Testen mit öffentlichem Repo:
   Repository: torvalds/linux
   Branch: master
   Docs Path: Documentation
```

### Problem 7: Langsame Ladezeiten

**Ursache:** Kein Caching oder zu viele Requests

**Lösung:**
```
1. Auto-Sync aktivieren (reduziert API-Calls)
2. Object Cache aktivieren (Redis/Memcached)
3. CDN für WordPress einrichten
4. Lazy Loading für Assets aktivieren
```

---

## Nächste Schritte

### 1. Erweiterte Konfiguration
- Custom CSS für individuelle Anpassungen
- JavaScript-Events für Interaktivität
- Mehrsprachige Navigation

### 2. Performance-Optimierung
- Object Cache (Redis)
- CDN-Integration
- Lazy Loading

### 3. SEO-Optimierung
- Meta-Descriptions für Docs-Seiten
- Schema Markup für Dokumentation
- XML Sitemap für Docs

### 4. Integration mit anderen Plugins
- Rank Math SEO
- Syntax Highlighter Evolved
- Table of Contents Plus

---

## Support

**Fragen? Probleme?**
- GitHub Issues: https://github.com/makr-code/wordpressPlugins/issues
- Dokumentation: https://github.com/makr-code/wordpressPlugins/tree/main/docs
- Community: ThemisDB Discord/Slack

---

**Viel Erfolg mit der Installation! 🚀**
