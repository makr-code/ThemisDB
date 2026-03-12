# Quick Start Guide

## Installation in 3 Schritten

### 1. Plugin hochladen

```bash
# Option A: ZIP hochladen
# - WordPress Admin → Plugins → Installieren → Plugin hochladen
# - Datei auswählen: themisdb-compendium-downloads-v1.0.0.zip
# - Installieren → Aktivieren

# Option B: Manuell via FTP
# - Entpacken Sie die ZIP-Datei
# - Laden Sie den Ordner nach /wp-content/plugins/ hoch
# - WordPress Admin → Plugins → Aktivieren
```

### 2. Einstellungen konfigurieren

```
WordPress Admin → Einstellungen → Kompendium Downloads

Empfohlene Einstellungen:
- GitHub Repository: makr-code/wordpressPlugins ✓
- Dateigrößen anzeigen: Ja ✓
- Cache-Dauer: 3600 Sekunden ✓
- Button-Stil: Modern ✓
```

### 3. Shortcode verwenden

```
Seite/Beitrag bearbeiten → Shortcode einfügen:

[themisdb_compendium_downloads]
```

## Beispiel-Seite erstellen

### Vollständige Downloads-Seite

```markdown
# ThemisDB Dokumentation

Willkommen zur ThemisDB Dokumentation! Hier finden Sie alle wichtigen Ressourcen.

## Kompendium Downloads

[themisdb_compendium_downloads style="modern" layout="cards"]

## Weitere Ressourcen

- [GitHub Repository](https://github.com/makr-code/wordpressPlugins)
- [Online Dokumentation](https://makr-code.github.io/ThemisDB/)
- [Docker Hub](https://hub.docker.com/r/themisdb/themisdb)
```

### Sidebar-Widget

```
WordPress Admin → Design → Widgets
→ "ThemisDB Kompendium Downloads" in Sidebar ziehen
→ Titel: "Dokumentation"
→ Stil: Minimal
→ Version anzeigen: Ja
→ Speichern
```

### Kompakte Anzeige

```
Für Footer oder schmale Bereiche:

[themisdb_compendium_downloads layout="compact" show_version="no"]
```

## Styling-Beispiele

### Modern (Standard)

```
[themisdb_compendium_downloads style="modern"]
```
→ Farbverläufe, Schatten, animierte Hover-Effekte

### Klassisch

```
[themisdb_compendium_downloads style="classic"]
```
→ Dezente Farben, klassisches Layout

### Minimal

```
[themisdb_compendium_downloads style="minimal"]
```
→ Reduziertes Design, keine Schatten

## Layouts

### Cards (Standard)

```
[themisdb_compendium_downloads layout="cards"]
```
→ Grid mit Karten-Design

### Liste

```
[themisdb_compendium_downloads layout="list"]
```
→ Vertikale Liste

### Kompakt

```
[themisdb_compendium_downloads layout="compact"]
```
→ Für Sidebars und schmale Bereiche

## Erweiterte Beispiele

### Download-Seite mit Filterung

```html
<div class="themisdb-docs-page">
    <h1>ThemisDB Downloads</h1>
    
    <div class="downloads-section">
        <h2>📚 Kompendium (PDF)</h2>
        [themisdb_compendium_downloads style="modern" layout="cards"]
    </div>
    
    <div class="other-resources">
        <h2>🐳 Docker Images</h2>
        <p>Docker Images finden Sie auf <a href="https://hub.docker.com/r/themisdb/themisdb">Docker Hub</a></p>
        
        <h2>📦 GitHub Releases</h2>
        <p>Binaries und Source Code auf <a href="https://github.com/makr-code/wordpressPlugins/releases">GitHub</a></p>
    </div>
</div>
```

### Landing Page

```html
<div class="hero-section">
    <h1>ThemisDB - High-Performance Multi-Model Database</h1>
    <p>Die komplette Dokumentation als PDF</p>
</div>

[themisdb_compendium_downloads style="modern" show_date="yes"]

<div class="features-grid">
    <div class="feature">
        <h3>🎓 Professional Edition</h3>
        <p>Vollständige technische Dokumentation</p>
    </div>
    <div class="feature">
        <h3>🖨️ Print Edition</h3>
        <p>Optimiert für den Druck</p>
    </div>
</div>
```

## Troubleshooting

### Keine Downloads sichtbar?

```
1. Cache leeren:
   Einstellungen → Kompendium Downloads → Cache leeren

2. Repository prüfen:
   Einstellungen → Kompendium Downloads → GitHub Repository
   → Soll sein: makr-code/wordpressPlugins

3. Browser-Cache leeren:
   Strg+Shift+R (Chrome/Firefox)
   Cmd+Shift+R (Mac)
```

### Styling stimmt nicht?

```css
/* Custom CSS hinzufügen: Design → Customizer → Zusätzliches CSS */

.themisdb-compendium-downloads {
    /* Ihre Anpassungen hier */
}
```

### Widget zeigt nicht korrekt an?

```
1. Widget-Area prüfen:
   Design → Widgets → Sidebar hat Platz?

2. Widget-Einstellungen:
   Layout: Compact
   Version anzeigen: Optional

3. Theme-Kompatibilität:
   Falls Probleme: style="minimal" verwenden
```

## Support

- **Dokumentation**: Siehe [README.md](README.md)
- **Installation**: Siehe [INSTALLATION.md](INSTALLATION.md)
- **Issues**: https://github.com/makr-code/wordpressPlugins/issues

---

**Entwickelt mit ❤️ vom ThemisDB Team**
