# ThemisDB Theme - Widgets und Slider Funktionalität

## Übersicht

Das ThemisDB WordPress Theme bietet jetzt erweiterte Funktionalität für Slidefolien und Widgets zum Hervorheben von Artikeln.

## Neue Widgets

### 1. ThemisDB: Featured Slider (Hervorgehobener Slider)

Ein moderner Slider zum Präsentieren von wichtigen Artikeln auf Ihrer Website.

**Funktionen:**
- Zeigt "Sticky Posts" (hervorgehobene Beiträge) in einem animierten Slider
- Automatisches Abspielen mit 5 Sekunden Intervall
- Navigation durch Pfeiltasten (Tastatur)
- Touch-Gesten für mobile Geräte (Swipe)
- Responsive Design
- Pause beim Hover

**Verwendung:**
1. Gehen Sie zu **Design > Widgets** in WordPress Admin
2. Fügen Sie das Widget "ThemisDB: Featured Slider" zu einem Widget-Bereich hinzu
3. Konfigurieren Sie:
   - **Titel**: Optional - Überschrift für den Slider
   - **Anzahl der Posts**: Wie viele Artikel angezeigt werden sollen (Standard: 3)

**Tipp:** Markieren Sie Beiträge als "Sticky" (Anheften), um sie im Slider anzuzeigen:
- Bearbeiten Sie einen Beitrag
- In den "Beitrag"-Einstellungen (rechte Sidebar) aktivieren Sie "An erster Stelle der Webseite halten"

### 2. ThemisDB: Recent Posts (Aktuelle Beiträge)

Zeigt aktuelle Beiträge mit optionalen Vorschaubildern.

**Funktionen:**
- Liste der neuesten Beiträge
- Optionale Featured Images (Beitragsbilder)
- Veröffentlichungsdatum
- Konfigurierbare Anzahl

**Verwendung:**
1. Widget hinzufügen: "ThemisDB: Recent Posts"
2. Konfigurieren:
   - **Titel**: z.B. "Neueste Artikel"
   - **Anzahl der Posts**: Standard 5
   - **Display thumbnails**: Häkchen setzen um Bilder anzuzeigen

### 3. ThemisDB: Category Highlights (Kategorie-Highlights)

Hebt Beiträge aus einer bestimmten Kategorie hervor.

**Funktionen:**
- Zeigt Posts aus einer ausgewählten Kategorie
- Vorschaubild und Textauszug
- Perfekt für spezielle Inhaltsbereiche

**Verwendung:**
1. Widget hinzufügen: "ThemisDB: Category Highlights"
2. Konfigurieren:
   - **Titel**: z.B. "Tutorials" oder "Neuigkeiten"
   - **Category**: Wählen Sie eine Kategorie aus
   - **Anzahl der Posts**: Standard 3

### 4. ThemisDB: Call to Action (Handlungsaufforderung)

Ein auffälliges Box-Widget für wichtige Mitteilungen oder Links.

**Funktionen:**
- Gradient-Hintergrund in verschiedenen Farben
- Titel, Inhalt und Button
- 4 Farbstile: Primary, Secondary, Accent, Success
- Perfekt für Downloads, wichtige Links, Ankündigungen

**Verwendung:**
1. Widget hinzufügen: "ThemisDB: Call to Action"
2. Konfigurieren:
   - **Titel**: z.B. "Download ThemisDB"
   - **Content**: Beschreibungstext
   - **Button Text**: z.B. "Jetzt herunterladen"
   - **Button URL**: Link-Ziel
   - **Style**: Wählen Sie eine Farbe (Primary/Secondary/Accent/Success)

## Homepage Featured Slider

Zusätzlich zu den Widgets wird auf der Startseite automatisch ein Featured Slider angezeigt, wenn Sie Sticky Posts haben.

**Aktivierung:**
1. Setzen Sie Ihre Homepage als Startseite:
   - **Einstellungen > Lesen**
   - Wählen Sie "Eine statische Seite" oder nutzen Sie die Blog-Seite
2. Markieren Sie Beiträge als "Sticky" (siehe oben)
3. Der Slider erscheint automatisch auf der Startseite

**Features:**
- Zeigt bis zu 5 hervorgehobene Artikel
- Große, ansprechende Präsentation
- Automatische Navigation
- Mobile-freundlich

## Widget-Bereiche

Das ThemisDB Theme bietet folgende Widget-Bereiche:

1. **Sidebar** - Rechte Seitenleiste (auf den meisten Seiten)
2. **Footer Widget 1** - Fußzeile, linke Spalte
3. **Footer Widget 2** - Fußzeile, mittlere Spalte
4. **Footer Widget 3** - Fußzeile, rechte Spalte

## Empfohlene Widget-Platzierungen

### Startseite Setup
- **Sidebar:**
  - ThemisDB: Call to Action (Download-Link)
  - ThemisDB: Category Highlights (Tutorials)
  - ThemisDB: Recent Posts

### Blog-Seite Setup
- **Sidebar:**
  - ThemisDB: Featured Slider
  - Search
  - Categories

### Footer Setup
- **Footer Widget 1:** About Text / Navigation
- **Footer Widget 2:** ThemisDB: Recent Posts (mit Thumbnails)
- **Footer Widget 3:** ThemisDB: Call to Action (Newsletter/Social)

## Technische Details

### Slider Steuerung

**Tastatur-Navigation:**
- Pfeil links: Vorheriger Slide
- Pfeil rechts: Nächster Slide

**Touch-Gesten:**
- Wischen nach links: Nächster Slide
- Wischen nach rechts: Vorheriger Slide

**Autoplay:**
- 5 Sekunden pro Slide
- Pausiert beim Hover
- Neustart nach manueller Navigation

### Dateien

Die neue Funktionalität befindet sich in:
- `/inc/widgets.php` - Widget-Klassen
- `/css/widgets.css` - Widget-Styles
- `/js/slider.js` - Slider-Funktionalität

### Anpassung

Die Widgets verwenden die ThemisDB Themis-Farben:
- Primary: `#2c3e50` (Dunkles Blau-Grau)
- Secondary: `#3498db` (Helles Blau)
- Accent Purple: `#7c4dff` (Lila)
- Success: `#27ae60` (Grün)

Diese können im **Design > Customizer > Theme Colors** angepasst werden.

## Browser-Kompatibilität

- Chrome/Edge (neueste Versionen)
- Firefox (neueste Versionen)
- Safari (neueste Versionen)
- Mobile Browser (iOS Safari, Chrome Mobile)

## Performance

- Vanilla JavaScript (kein jQuery)
- CSS3 Transitions
- Optimierte Bildgrößen
- Lazy Loading unterstützt

## Support

Bei Fragen oder Problemen:
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Dokumentation: `/wordpress-theme/README.md`

---

*Erstellt: Januar 2026*  
*ThemisDB Theme Version: 1.0.0*
