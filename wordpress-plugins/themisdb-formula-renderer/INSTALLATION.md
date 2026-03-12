# Installation - ThemisDB Formula Renderer

Detaillierte Installationsanleitung für das ThemisDB Formula Renderer Plugin.

## Systemanforderungen

- **WordPress**: 5.0 oder höher
- **PHP**: 7.2 oder höher
- **Browser**: Moderner Browser mit JavaScript-Unterstützung

## Installationsmethoden

### Methode 1: WordPress Admin Panel (Empfohlen)

1. **Plugin ZIP erstellen**
   ```bash
   cd /pfad/zu/ThemisDB/wordpress-plugin
   zip -r themisdb-formula-renderer.zip themisdb-formula-renderer/
   ```

2. **In WordPress hochladen**
   - Melden Sie sich in Ihrem WordPress Admin an
   - Navigieren Sie zu **Plugins → Installieren**
   - Klicken Sie auf **Plugin hochladen**
   - Wählen Sie die `themisdb-formula-renderer.zip` Datei
   - Klicken Sie auf **Jetzt installieren**

3. **Plugin aktivieren**
   - Nach erfolgreicher Installation klicken Sie auf **Plugin aktivieren**
   - Sie sehen eine Bestätigungsnachricht

### Methode 2: Manueller Upload via FTP

1. **Plugin-Dateien vorbereiten**
   - Laden Sie das `themisdb-formula-renderer` Verzeichnis herunter
   
2. **Via FTP hochladen**
   - Verbinden Sie sich mit Ihrem Webserver via FTP
   - Navigieren Sie zu `/wp-content/plugins/`
   - Laden Sie das gesamte `themisdb-formula-renderer` Verzeichnis hoch
   
3. **Plugin aktivieren**
   - Gehen Sie zu WordPress Admin → **Plugins**
   - Suchen Sie nach "ThemisDB Formula Renderer"
   - Klicken Sie auf **Aktivieren**

### Methode 3: WP-CLI

Für fortgeschrittene Benutzer mit Kommandozeilen-Zugriff:

```bash
# Plugin-Verzeichnis kopieren
cp -r themisdb-formula-renderer /pfad/zu/wordpress/wp-content/plugins/

# Plugin aktivieren
wp plugin activate themisdb-formula-renderer
```

## Erste Schritte nach der Installation

### 1. Grundkonfiguration

Nach der Aktivierung gehen Sie zu **Einstellungen → Formula Renderer**:

- **Auto-Render Formulas**: Sollte aktiviert sein (Standard)
- **Inline Delimiter**: `$` (Standard) - für Inline-Formeln
- **Block Delimiter**: `$$` (Standard) - für Block-Formeln

Klicken Sie auf **Einstellungen speichern**.

### 2. Erste Testformel

Erstellen Sie einen neuen Beitrag oder eine neue Seite:

1. Fügen Sie folgenden Text ein:
   ```
   Die berühmte Formel von Einstein lautet $E = mc^2$.
   
   Eine komplexere Formel in Block-Darstellung:
   
   $$D = (t_n - t_{n-1}) - (t_{n-1} - t_{n-2})$$
   ```

2. Veröffentlichen oder Vorschau anzeigen

3. Die Formeln sollten nun schön gerendert erscheinen

### 3. Weitere Beispiele testen

Probieren Sie verschiedene Formeln aus der Einstellungsseite:

- Gehen Sie zu **Einstellungen → Formula Renderer**
- Scrollen Sie zu den **Beispielen**
- Kopieren Sie Beispiele und testen Sie sie in Ihren Beiträgen

## Erweiterte Konfiguration

### Custom CSS

Falls Sie das Aussehen anpassen möchten, fügen Sie Custom CSS hinzu:

1. Gehen Sie zu **Design → Customizer → Zusätzliches CSS**

2. Fügen Sie Ihre Styles hinzu:
   ```css
   /* Eigene Farben für Formeln */
   .themisdb-formula-block {
       background-color: #e8f4f8;
       border-left-color: #0077aa;
   }
   
   /* Größere Schrift für Formeln */
   .katex {
       font-size: 1.3em;
   }
   ```

3. Klicken Sie auf **Veröffentlichen**

### Theme-Integration

Für beste Ergebnisse fügen Sie im Theme folgendes hinzu:

```php
// In functions.php des Child Themes
add_theme_support('themisdb-formula-renderer');
```

### Performance-Optimierung

#### Mit Caching-Plugins

Das Plugin funktioniert mit allen gängigen Caching-Plugins:

- **WP Super Cache**: Keine Konfiguration nötig
- **W3 Total Cache**: JavaScript-Minifizierung kann deaktiviert werden für KaTeX
- **WP Rocket**: Funktioniert out-of-the-box

#### CDN-Konfiguration

KaTeX wird standardmäßig von jsDelivr CDN geladen. Falls gewünscht, können Sie einen anderen CDN verwenden:

Bearbeiten Sie `themisdb-formula-renderer.php` und ändern Sie:

```php
// Von:
wp_enqueue_style('katex-style', 'https://cdn.jsdelivr.net/npm/katex@0.16.9/dist/katex.min.css', ...);

// Zu (Beispiel: CDNJS):
wp_enqueue_style('katex-style', 'https://cdnjs.cloudflare.com/ajax/libs/KaTeX/0.16.9/katex.min.css', ...);
```

### Multisite-Installation

Für WordPress Multisite:

1. **Network-Aktivierung**
   - Gehen Sie zu **Network Admin → Plugins**
   - Aktivieren Sie "ThemisDB Formula Renderer" für das Netzwerk
   
2. **Site-spezifische Konfiguration**
   - Jede Site kann eigene Einstellungen haben
   - Gehen Sie auf jeder Site zu **Einstellungen → Formula Renderer**

## Überprüfung der Installation

### Checkliste

- [ ] Plugin ist aktiviert
- [ ] Einstellungsseite ist erreichbar unter **Einstellungen → Formula Renderer**
- [ ] KaTeX CSS und JS werden geladen (überprüfen Sie Seitenquelltext)
- [ ] Testformel wird korrekt gerendert
- [ ] Keine JavaScript-Fehler in Browser-Konsole

### Debugging

Falls Formeln nicht angezeigt werden:

1. **Browser-Konsole öffnen** (F12)
   - Suchen Sie nach JavaScript-Fehlern
   - KaTeX sollte geladen sein

2. **Plugin-Konflikte prüfen**
   - Deaktivieren Sie andere Plugins temporär
   - Testen Sie erneut

3. **Theme-Kompatibilität**
   - Wechseln Sie zu einem Standard-Theme (z.B. Twenty Twenty-Three)
   - Wenn es funktioniert, liegt ein Theme-Konflikt vor

4. **PHP-Fehler-Log prüfen**
   ```php
   // In wp-config.php
   define('WP_DEBUG', true);
   define('WP_DEBUG_LOG', true);
   ```
   - Überprüfen Sie `/wp-content/debug.log`

## Integration mit Page Builders

### Gutenberg (Block Editor)

Funktioniert sofort:

1. Fügen Sie einen **Absatz**-Block hinzu
2. Geben Sie Ihre Formel mit `$$...$$` ein
3. Die Formel wird beim Anzeigen automatisch gerendert

### Elementor

1. Fügen Sie ein **Text-Widget** hinzu
2. Wechseln Sie zur **Text-Ansicht**
3. Geben Sie Formeln mit `$$...$$` ein
4. Formeln werden auf der Live-Seite gerendert

### WPBakery Page Builder

1. Fügen Sie ein **Text-Block**-Element hinzu
2. Geben Sie Formeln mit `$$...$$` ein
3. Speichern und Vorschau anzeigen

### Divi Builder

1. Fügen Sie ein **Text-Modul** hinzu
2. Im Text-Editor, nutzen Sie `$$...$$` für Formeln
3. Formeln werden auf der Frontend-Seite gerendert

## Deinstallation

Falls Sie das Plugin entfernen möchten:

1. **Deaktivieren**
   - Gehen Sie zu **Plugins**
   - Klicken Sie bei "ThemisDB Formula Renderer" auf **Deaktivieren**

2. **Löschen**
   - Klicken Sie auf **Löschen**
   - Alle Plugin-Dateien und Einstellungen werden entfernt

**Hinweis**: Formeln in Ihren Beiträgen bleiben als Text erhalten (z.B. `$$E = mc^2$$`).

## Support

Bei Problemen:

1. Überprüfen Sie die [Fehlerbehebung](README.md#fehlerbehebung) im README
2. Konsultieren Sie die [KaTeX-Dokumentation](https://katex.org/docs/support_table.html)
3. Öffnen Sie ein Issue auf [GitHub](https://github.com/makr-code/wordpressPlugins/issues)

## Updates

Das Plugin wird über WordPress Auto-Updates aktualisiert. Sie können auch manuell aktualisieren:

1. Laden Sie die neueste Version herunter
2. Deaktivieren Sie das alte Plugin
3. Löschen Sie das alte Plugin
4. Installieren Sie die neue Version
5. Aktivieren Sie das Plugin wieder

**Ihre Einstellungen bleiben erhalten.**

---

Bei Fragen zur Installation wenden Sie sich an das ThemisDB Team.
