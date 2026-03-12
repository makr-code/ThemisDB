# ThemisDB Gallery - WordPress Plugin

Ein WordPress-Plugin, das beim Erstellen von Artikeln hilft, relevante frei verfügbare thematisch passende Bilder im Internet zu finden, herunterzuladen und einzubinden - mit vollen Credits (Urheber usw.). Optional mit KI-Bildgenerator.

## 🎯 Features

- **Bildsuche aus mehreren Quellen**
  - Unsplash - Hochqualitative kostenlose Bilder
  - Pexels - Große Auswahl an Stock-Fotos
  - Pixabay - Vielfältige Bildsammlung
  - Alle Anbieter gleichzeitig durchsuchen

- **Automatische Attribution**
  - Automatisches Hinzufügen von Quellenangaben
  - Fotografen-Credits mit Links
  - Lizenzinformationen
  - Gespeichert in WordPress-Medien-Metadaten

- **WordPress-Integration**
  - **Tab im Medien-Dashboard** - Direkt in der WordPress-Mediathek verfügbar
  - Meta-Box im Post-Editor für einfache Suche
  - Direkte Einfügung in Posts und Pages
  - Shortcodes für flexible Anzeige
  - Gutenberg-Blöcke (geplant)

- **Optional: AI-Bildgenerierung**
  - Integration mit OpenAI DALL-E
  - Generierung von Bildern aus Textbeschreibungen
  - Automatische Attribution als "AI Generated"

- **Cache-System**
  - Reduziert API-Aufrufe
  - Konfigurierbare Cache-Dauer
  - Manuelle Cache-Löschung

## 📋 Voraussetzungen

- WordPress 5.0 oder höher
- PHP 7.2 oder höher
- HTTPS empfohlen (für externe API-Aufrufe)

## 🚀 Installation

### Methode 1: ZIP-Upload

1. Laden Sie das Plugin als ZIP herunter oder erstellen Sie ein ZIP:
   ```bash
   cd wordpress-plugin
   zip -r themisdb-gallery.zip themisdb-gallery/
   ```

2. In WordPress:
   - Gehen Sie zu **Plugins → Installieren**
   - Klicken Sie auf **Plugin hochladen**
   - Wählen Sie die ZIP-Datei
   - Klicken Sie auf **Jetzt installieren**
   - Aktivieren Sie das Plugin

### Methode 2: Manueller Upload

1. Laden Sie den Ordner `themisdb-gallery` hoch nach:
   ```
   /wp-content/plugins/themisdb-gallery/
   ```

2. In WordPress:
   - Gehen Sie zu **Plugins**
   - Finden Sie "ThemisDB Gallery"
   - Klicken Sie auf **Aktivieren**

## ⚙️ Konfiguration

### 1. API-Schlüssel einrichten

Nach der Aktivierung gehen Sie zu **Einstellungen → ThemisDB Gallery**

#### Unsplash API-Schlüssel

1. Besuchen Sie [https://unsplash.com/developers](https://unsplash.com/developers)
2. Registrieren Sie sich / Melden Sie sich an
3. Erstellen Sie eine neue Anwendung
4. Kopieren Sie den "Access Key"
5. Fügen Sie ihn in die Plugin-Einstellungen ein

#### Pexels API-Schlüssel

1. Besuchen Sie [https://www.pexels.com/api/](https://www.pexels.com/api/)
2. Registrieren Sie sich / Melden Sie sich an
3. Kopieren Sie Ihren API-Schlüssel
4. Fügen Sie ihn in die Plugin-Einstellungen ein

#### Pixabay API-Schlüssel

1. Besuchen Sie [https://pixabay.com/api/docs/](https://pixabay.com/api/docs/)
2. Registrieren Sie sich / Melden Sie sich an
3. Kopieren Sie Ihren API-Schlüssel
4. Fügen Sie ihn in die Plugin-Einstellungen ein

#### OpenAI API-Schlüssel (Optional)

Nur erforderlich für AI-Bildgenerierung:

1. Besuchen Sie [https://platform.openai.com/api-keys](https://platform.openai.com/api-keys)
2. Erstellen Sie einen neuen API-Schlüssel
3. Fügen Sie ihn in die Plugin-Einstellungen ein
4. **Hinweis**: OpenAI ist ein kostenpflichtiger Service

### 2. Allgemeine Einstellungen

- **Standard-Anbieter**: Wählen Sie den bevorzugten Bildanbieter
- **Bilder pro Seite**: Anzahl der Suchergebnisse (5-50)
- **Cache-Dauer**: Wie lange Suchergebnisse gecacht werden (in Sekunden)
- **Automatische Quellenangabe**: Automatisch Attributionen zu Bildunterschriften hinzufügen

## 📝 Verwendung

### In der Mediathek (Dashboard)

1. Klicken Sie im WordPress-Dashboard auf **Medien → Dateien hinzufügen**
2. Oder klicken Sie im Post-Editor auf **Medien hinzufügen**
3. Im Media-Upload-Dialog sehen Sie einen neuen Tab **"ThemisDB Gallery"**
4. Geben Sie einen Suchbegriff ein (z.B. "Natur", "Technologie", "Business")
5. Wählen Sie einen Anbieter oder "Alle Anbieter"
6. Klicken Sie auf **Suchen**
7. Klicken Sie auf **Bild einfügen** bei dem gewünschten Bild
8. Das Bild wird in Ihre Mediathek importiert und direkt in den Post eingefügt

### Im Post-Editor

1. Öffnen Sie einen Post oder eine Page zum Bearbeiten
2. In der Seitenleiste finden Sie die **"ThemisDB Gallery - Bildsuche"** Meta-Box
3. Geben Sie einen Suchbegriff ein (z.B. "Natur", "Technologie", "Business")
4. Wählen Sie einen Anbieter oder "Alle Anbieter"
5. Klicken Sie auf **Suchen**
6. Klicken Sie auf **Bild einfügen** bei dem gewünschten Bild
7. Das Bild wird in Ihre Mediathek importiert und in den Post eingefügt

### AI-Bildgenerierung (Optional)

Wenn Sie einen OpenAI API-Schlüssel konfiguriert haben:

1. Geben Sie eine Beschreibung in das AI-Feld ein
   - Beispiel: "Ein modernes Büro mit Computern und Pflanzen"
2. Klicken Sie auf **AI Generieren**
3. Das generierte Bild wird angezeigt
4. Klicken Sie auf **Bild einfügen** um es zu verwenden

### Shortcodes

#### Galerie anzeigen

```php
// Galerie mit spezifischen Bild-IDs
[themisdb_gallery ids="123,124,125"]

// Galerie aus Suchergebnissen
[themisdb_gallery search="nature" provider="unsplash" columns="3" limit="12"]

// Galerie ohne Quellenangaben
[themisdb_gallery search="technology" show_attribution="no"]
```

**Parameter:**
- `ids` - Komma-getrennte Liste von Attachment-IDs
- `search` - Suchbegriff
- `provider` - Anbieter (unsplash, pexels, pixabay, all)
- `columns` - Anzahl Spalten (1-4, Standard: 3)
- `limit` - Maximale Anzahl Bilder (Standard: 12)
- `show_attribution` - Quellenangaben anzeigen (yes/no, Standard: yes)

#### Bildsuche-Widget

```php
[themisdb_image_search]
```

Zeigt ein Frontend-Suchformular für Besucher an.

#### Attribution anzeigen

```php
[themisdb_image_attribution id="123"]
```

Zeigt die Quellenangabe für ein bestimmtes Bild an.

## 🎨 Styling

Das Plugin enthält grundlegende Styles. Sie können diese in Ihrem Theme überschreiben:

```css
/* Galerie-Grid anpassen */
.themisdb-gallery {
    gap: 30px; /* Abstand zwischen Bildern */
}

/* Bild-Items anpassen */
.themisdb-gallery-item {
    border-radius: 12px; /* Abgerundete Ecken */
    box-shadow: 0 4px 12px rgba(0,0,0,0.1); /* Schatten */
}

/* Attribution anpassen */
.themisdb-gallery-attribution {
    background: #f0f0f0;
    padding: 15px;
}
```

## 🔧 Entwicklung

### Dateistruktur

```
themisdb-gallery/
├── themisdb-gallery.php          # Haupt-Plugin-Datei
├── includes/
│   ├── class-image-api.php       # API-Handler für Bildsuche
│   ├── class-admin.php           # Admin-Interface
│   ├── class-media-handler.php   # Medien-Import
│   ├── class-shortcodes.php      # Shortcode-Handler
│   └── class-gutenberg-block.php # Gutenberg-Blöcke
├── assets/
│   ├── css/
│   │   ├── style.css             # Frontend-Styles
│   │   ├── admin.css             # Admin-Styles
│   │   ├── blocks.css            # Block-Styles
│   │   └── blocks-editor.css     # Block-Editor-Styles
│   └── js/
│       ├── script.js             # Frontend-JavaScript
│       ├── admin.js              # Admin-JavaScript
│       └── blocks.js             # Gutenberg-Blöcke
└── languages/                    # Übersetzungsdateien
```

### Hooks und Filter

```php
// Eigenen Bildanbieter hinzufügen
add_filter('themisdb_gallery_providers', function($providers) {
    $providers['custom'] = 'Mein Anbieter';
    return $providers;
});

// Attribution-Text anpassen
add_filter('themisdb_gallery_attribution_text', function($text, $image_data) {
    return 'Foto: ' . $image_data['author'];
}, 10, 2);
```

## ❓ FAQ

### Sind die API-Schlüssel kostenlos?

Ja, Unsplash, Pexels und Pixabay bieten kostenlose API-Schlüssel für nicht-kommerzielle und kommerzielle Nutzung. OpenAI (für AI-Generierung) ist kostenpflichtig.

### Werden die Bilder in WordPress gespeichert?

Ja, beim Einfügen werden Bilder heruntergeladen und in Ihre WordPress-Mediathek importiert. Sie gehören dann zu Ihrer Website.

### Muss ich Quellenangaben hinzufügen?

Ja, die Lizenzen von Unsplash, Pexels und Pixabay erfordern Attribution. Das Plugin fügt diese automatisch hinzu.

### Kann ich die Quellenangaben anpassen?

Ja, Sie können die automatischen Quellenangaben deaktivieren und eigene hinzufügen, oder den Text per Filter anpassen.

### Wie viele API-Anfragen sind erlaubt?

- **Unsplash**: 50 Anfragen/Stunde (kostenlos)
- **Pexels**: 200 Anfragen/Stunde (kostenlos)
- **Pixabay**: 5.000 Anfragen/Stunde (kostenlos)

Das Cache-System reduziert die Anzahl der API-Anfragen erheblich.

## 🐛 Fehlerbehebung

### "API-Schlüssel nicht konfiguriert"

Stellen Sie sicher, dass Sie API-Schlüssel in den Plugin-Einstellungen eingegeben haben.

### "Keine Bilder gefunden"

- Überprüfen Sie Ihre API-Schlüssel
- Versuchen Sie einen anderen Suchbegriff
- Überprüfen Sie die API-Rate-Limits
- Leeren Sie den Cache in den Plugin-Einstellungen

### Bilder werden nicht eingefügt

- Überprüfen Sie, ob Sie die Berechtigung zum Hochladen von Dateien haben
- Stellen Sie sicher, dass Ihr WordPress-Upload-Ordner beschreibbar ist
- Überprüfen Sie die PHP-Upload-Limits

## 📄 Lizenz

MIT License - Siehe [LICENSE](../../LICENSE) Datei

## 🤝 Support

- GitHub Issues: https://github.com/makr-code/wordpressPlugins/issues
- Dokumentation: https://github.com/makr-code/wordpressPlugins/tree/main/wordpress-plugin

## 📚 Credits

Dieses Plugin nutzt folgende Dienste:
- [Unsplash](https://unsplash.com/) - Kostenlose hochauflösende Bilder
- [Pexels](https://www.pexels.com/) - Kostenlose Stock-Fotos
- [Pixabay](https://pixabay.com/) - Kostenlose Bilder und Videos
- [OpenAI DALL-E](https://openai.com/dall-e) - AI-Bildgenerierung (optional)

## 🔄 Changelog

### Version 1.0.0 (2026-01-07)
- Erste Veröffentlichung
- Bildsuche mit Unsplash, Pexels, Pixabay
- Automatische Attribution
- Post-Editor Integration
- Shortcodes
- AI-Bildgenerierung (optional)
- Cache-System
