# ThemisDB Theme v2

**WordPress Block Theme (Full Site Editing)** für ThemisDB — inspiriert von [postgresql.org](https://www.postgresql.org/) und [Microsoft Azure](https://azure.microsoft.com/de-de/).

Version: **2.0.0** | Requires WordPress: **6.3+** | PHP: **7.4+**

---

## Inhalt

1. [Was ist dieses Theme?](#was-ist-dieses-theme)
2. [Theme aktivieren](#theme-aktivieren)
3. [Struktur](#struktur)
4. [Templates](#templates)
5. [Template Parts](#template-parts)
6. [Block Patterns](#block-patterns)
7. [Design Tokens](#design-tokens)
8. [Plugin-Empfehlungen (Free)](#plugin-empfehlungen-free)
9. [Plugin-Kompatibilität](#plugin-kompatibilität)
10. [Landingpage einrichten (Azure-Style)](#landingpage-einrichten-azure-style)
11. [Entwicklung](#entwicklung)

---

## Was ist dieses Theme?

`themisdb-theme-v2` ist ein **WordPress Block Theme** (Full Site Editing, FSE), das das bisherige klassische Theme `themisdb-theme` als Nachfolger ablöst — ohne es zu verändern. Es bietet:

- **Professionelles Azure/PostgreSQL-Style Design** für die ThemisDB Landingpage
- **Vollständige FSE-Unterstützung**: Site Editor, Global Styles, Template Editor
- **Zentralisiertes Design-Token-System** (`--themis-*` CSS Custom Properties)
- **Rückwärtskompatibilität** für alle existierenden ThemisDB-Plugins (CSS-Variablen-Aliases)
- **Wiederverwendbare Block Patterns**: Hero, Feature Cards, Stats Bar, Query Showcase, Testimonials
- **Docs-Layout** mit Sidebar-Navigation für technische Dokumentation

---

## Theme aktivieren

1. Stelle sicher, dass der Ordner `themisdb-theme-v2` unter `wp-content/themes/` (oder dem konfigurierten Theme-Verzeichnis) liegt.
2. Gehe in WordPress zu **Darstellung → Themes**.
3. Wähle **ThemisDB v2** und klicke **Aktivieren**.
4. Navigiere zu **Darstellung → Editor** (Site Editor), um Header, Footer und Startseite anzupassen.

> ⚠️ Das bestehende Theme `themisdb-theme` bleibt unverändert und kann jederzeit reaktiviert werden.

---

## Struktur

```
themisdb-theme-v2/
├── style.css                    # Theme-Header + globale CSS Custom Properties
├── theme.json                   # Design-Tokens: Palette, Typografie, Spacing, Layout
├── functions.php                # Theme-Setup, Pattern-Kategorien, Block-Styles, Sicherheit
│
├── templates/
│   ├── front-page.html          # ⭐ Azure-Style Landingpage
│   ├── index.html               # Blog-Index (3-Spalten Grid)
│   ├── page.html                # Standard-Seite
│   ├── single.html              # Einzelner Beitrag mit Meta + Navigation
│   ├── 404.html                 # 404-Fehlerseite
│   ├── page-docs.html           # Dokumentations-Layout mit Sidebar
│   └── page-full-width.html     # Vollbreite-Seite (kein Sidebar)
│
├── parts/
│   ├── header.html              # Dunkler Sticky-Header (Logo + Nav + CTA)
│   ├── header-docs.html         # Docs-Variante des Headers
│   ├── footer.html              # 4-Spalten Dark Footer
│   ├── announcement-bar.html    # Lila Release-Topbar
│   ├── page-hero.html           # Wiederverwendbarer Seiten-Titel-Block
│   ├── breadcrumbs.html         # Schema.org Breadcrumb-Navigation
│   ├── post-meta.html           # Autor + Datum + Lesezeit + Tags
│   ├── post-navigation.html     # Prev/Next Artikel-Navigation
│   ├── sidebar-docs.html        # Sticky Docs-Sidebar mit TOC
│   ├── cta-banner.html          # Gradient CTA-Strip (Download/Docs)
│   ├── search-bar.html          # Suchfeld-Komponente
│   └── 404-content.html         # 404-Inhalt mit Suche + Schnelllinks
│
├── patterns/
│   ├── hero-home.php            # Vollbreite Hero mit CTAs + Docker-Snippet
│   ├── feature-cards.php        # 3×2 Feature-Kacheln
│   ├── stats-bar.php            # 4 Statistik-Zahlen (500K+ Downloads etc.)
│   ├── cta-download.php         # Download-Optionen (Docker/Binary/Compendium)
│   ├── docs-grid.php            # 8 Docs/Ressourcen-Kacheln
│   ├── query-showcase.php       # Code-Demo mit SQL-Highlighting
│   └── testimonial.php          # 3 Testimonial-Cards
│
└── assets/
    ├── css/
    │   └── editor.css           # Editor-Styles (Gutenberg)
    └── js/
        └── navigation.js        # Mobile Menu, Copy-Buttons, Scroll-Header
```

---

## Templates

| Template | Beschreibung | Verwendung |
|---|---|---|
| `front-page.html` | Azure-Style Landingpage | Startseite |
| `index.html` | Blog-Grid (3 Spalten) | Blog-Übersicht, Archive |
| `page.html` | Standard-Seite + Kommentare | Seiten allgemein |
| `single.html` | Artikel mit Meta, Prev/Next | Blog-Beiträge |
| `404.html` | 404 mit Suche + Quicklinks | Fehlerseite |
| `page-docs.html` | Zwei-Spalten Docs-Layout | Dokumentation |
| `page-full-width.html` | Volle Breite ohne Sidebar | Landing-Unterseiten |

### Template einer Seite zuweisen
1. **Seite bearbeiten** → rechte Spalte → **Seitenattribute** → **Vorlage**
2. Oder im **Site Editor**: Templates → Neues Template

---

## Template Parts

| Part | Bereich | Beschreibung |
|---|---|---|
| `header` | Header | Dunkler Sticky-Header, Logo + Nav + CTA |
| `header-docs` | Header | Docs-Variante mit Docs-Suche |
| `footer` | Footer | 4-Spalten Footer: Products / Resources / Community / Legal |
| `announcement-bar` | – | Lila Topbar für aktuelle Releases |
| `page-hero` | – | Grauer Seiten-Titel-Block mit Breadcrumbs |
| `breadcrumbs` | – | Schema.org konform, Yoast-SEO-kompatibel |
| `post-meta` | – | Autorbild, Datum, Lesezeit, Tags |
| `post-navigation` | – | Prev/Next mit Titel-Preview |
| `sidebar-docs` | – | Sticky Docs-Navigation + Quick Links |
| `cta-banner` | – | Gradient CTA-Strip |
| `search-bar` | – | Suchfeld mit Quicklinks |
| `404-content` | – | 404-Inhalt mit Suche |

---

## Block Patterns

Alle Patterns sind im **Site Editor → Patterns** und im **Block-Inserter** unter **ThemisDB** verfügbar.

| Pattern | Slug | Einsatz |
|---|---|---|
| Hero – Home | `themisdb-v2/hero-home` | Startseiten-Hero |
| Feature Cards | `themisdb-v2/feature-cards` | Features-Übersicht |
| Stats Bar | `themisdb-v2/stats-bar` | Kennzahlen / Trust |
| CTA – Download | `themisdb-v2/cta-download` | Download-Sektion |
| Docs Grid | `themisdb-v2/docs-grid` | Ressourcen-Übersicht |
| Query Showcase | `themisdb-v2/query-showcase` | Code-Demo Sektion |
| Testimonial | `themisdb-v2/testimonial` | Social Proof |

---

## Design Tokens

Alle Design-Tokens sind als CSS Custom Properties definiert. Wichtigste:

```css
--themis-primary:        #2c3e50;   /* Dark Blue-Gray */
--themis-secondary:      #3498db;   /* Light Blue (Links, Buttons) */
--themis-accent:         #7c4dff;   /* Purple (Announcement, Badges) */
--themis-success:        #27ae60;
--themis-warning:        #f39c12;
--themis-error:          #e74c3c;

--themis-content-width:  760px;
--themis-wide-width:     1100px;

--themis-font-sans:      -apple-system, BlinkMacSystemFont, "Segoe UI", …
--themis-font-mono:      "SFMono-Regular", Consolas, …
```

**Global Styles** anpassen: **Darstellung → Editor → Stile** (das Pinsel-Icon oben rechts).

---

## Plugin-Empfehlungen (Free)

Diese kostenlosen Plugins ergänzen das Theme optimal für eine professionelle ThemisDB-Produktsite:

### 🔴 Must-Have

| Plugin | Slug | Funktion | Warum |
|---|---|---|---|
| **Yoast SEO** | `wordpress-seo` | Meta-Tags, XML-Sitemap, Breadcrumbs | SEO für Produktsite + Breadcrumb-Integration |
| **Kadence Blocks** | `kadence-blocks` | Tabs, Accordion, Icon Lists, Info Boxes | Essentiell für Docs-Seiten; füllt die Lücken von Core Blocks |
| **Easy Table of Contents** | `easy-table-of-contents` | Auto-TOC für lange Seiten | Kritisch für technische Dokumentation |
| **Relevanssi** | `relevanssi` | Erweiterte Suche (Volltext, gewichtet) | WP-Standard-Suche zu schwach für Docs-Content |

### 🟡 Strongly Recommended

| Plugin | Slug | Funktion | Warum |
|---|---|---|---|
| **Code Syntax Block** | `code-syntax-block` | Prism.js Syntax-Highlighting | SQL, JSON, Shell-Snippets professionell darstellen |
| **WP Super Cache** | `wp-super-cache` | Seiten-Caching | Performance für öffentliche Produktseite |
| **Smush** | `wp-smushit` | Bild-Optimierung (WebP) | PageSpeed für Hero-Images |
| **Complianz GDPR** | `complianz-gdpr` | Cookie-Banner (DSGVO-konform) | Rechtliche Absicherung |
| **Contact Form 7** | `contact-form-7` | Kontakt-/Support-Formulare | Nutzer-Feedback, Support-Requests |

### 🟢 Nice-to-Have

| Plugin | Slug | Funktion | Warum |
|---|---|---|---|
| **Google Site Kit** | `google-site-kit` | Analytics + Search Console | Traffic & Suchanfragen verstehen |
| **AddToAny** | `add-to-any` | Social Sharing Buttons | Blog-Posts teilen |
| **WP Revisions Control** | `wp-revisions-control` | Revisionen begrenzen | DB-Performance bei viel Content |
| **Broken Link Checker** | `broken-link-checker` | Kaputte Links finden | Qualitätssicherung |
| **Wordfence Security** | `wordfence` | Firewall + Malware Scan | Sicherheits-Basis |

### Warum **Kadence Blocks** für ThemisDB besonders wichtig ist

Kadence Blocks (free) liefert folgende Blöcke, die für ThemisDB-Docs fehlen:

- **Tabs** → Ideal für `[themisdb_feature_matrix]` Darstellung
- **Accordion** → FAQ, Changelog-Einträge, API-Parameter
- **Icon List** → Feature-Listen mit Icons statt Bullet Points
- **Info Box** → Callouts/Hinweis-Boxen für Docs
- **Table of Contents** (alternativ zu Easy TOC)
- **Advanced Gallery** → Integration mit `[themisdb_gallery]`

---

## Plugin-Kompatibilität

Das Theme styled folgende ThemisDB-Plugin-Ausgaben automatisch:

| Plugin | Shortcode | CSS-Klasse |
|---|---|---|
| Feature Matrix | `[themisdb_feature_matrix]` | `.themisdb-feature-wrapper` |
| Benchmark Visualizer | `[themisdb_benchmark_visualizer]` | `.themisdb-benchmark-wrapper` |
| Docker Downloads | `[themisdb_docker_tags]`, `[themisdb_docker_latest]` | `.themisdb-docker-wrapper` |
| Downloads | `[themisdb_downloads]`, `[themisdb_latest]` | `.themisdb-downloads-wrapper` |
| Compendium | `[themisdb_compendium_downloads]` | `.themisdb-downloads-wrapper` |
| Release Timeline | `[themisdb_release_timeline]` | `.themisdb-timeline-wrapper` |
| Order/License | `[themisdb_order_flow]`, `[themisdb_license_portal]` | `.themisdb-order-form` |
| Taxonomy | `[themisdb_taxonomy]`, `[themisdb_term_card]` | `.themisdb-taxonomy-list` |
| Gallery | `[themisdb_gallery]` | `.themisdb-gallery-grid` |
| Architecture | `[themisdb_architecture]` | `.themisdb-diagram-wrapper` |
| Query Playground | Shortcode | `.themisdb-playground-wrapper` |
| TCO Calculator | `[themisdb_tco_calculator]` | `.themisdb-tco-wrapper` |
| Formula Renderer | `[themisdb_formula]` | `.themisdb-formula-wrapper` |

---

## Landingpage einrichten (Azure-Style)

### Schritt 1: Startseite konfigurieren
1. **Einstellungen → Lesen** → „Eine statische Seite anzeigen"
2. Neue Seite „Home" erstellen
3. Dem Template `front-page` wird sie automatisch zugewiesen

### Schritt 2: Navigationsmenü anlegen
1. **Darstellung → Editor → Navigation**
2. Menü-Einträge: Features / Docs / Downloads / Benchmarks / Community
3. Das Theme ordnet dieses Menü dem Header automatisch zu

### Schritt 3: Announcement Bar anpassen
Datei `parts/announcement-bar.html` im Site Editor bearbeiten oder den Absatz direkt per Block Editor ändern.

### Schritt 4: Patterns einfügen
Im **Block-Inserter** (Seite bearbeiten) → Kategorie **ThemisDB – Landing Page** → gewünschtes Pattern einfügen.

### Schritt 5: Docs-Seiten mit Sidebar
Seiten, die das Template `page-docs` verwenden, erhalten automatisch:
- Sticky Docs-Sidebar (links)
- `header-docs` statt `header`
- "Was this helpful?"-Feedback-Box

---

## Entwicklung

### Anforderungen
- WordPress 6.3+
- PHP 7.4+
- Node.js nicht erforderlich (reines CSS/Block-Theme, keine Build-Tools)

### CSS-Anpassungen
Alle globalen Design-Tokens sind in `style.css` unter `:root {}` definiert. Einzelne Werte überschreiben:

```css
/* In einer Child-Theme style.css oder im Customizer */
:root {
    --themis-secondary: #0066cc; /* Eigene Akzentfarbe */
    --themis-wide-width: 1200px; /* Breiteres Layout */
}
```

### Global Styles (empfohlen)
**Darstellung → Editor → Stile** ermöglicht visuelle Anpassung der `theme.json`-Tokens ohne Code.

### Child-Theme
Ein Child-Theme ist nicht zwingend nötig, da das Theme auf `theme.json` Global Styles basiert. Für PHP-Anpassungen:

```php
// child-theme/functions.php
add_action( 'wp_enqueue_scripts', function() {
    wp_enqueue_style( 'child-style', get_stylesheet_uri(), array('themisdb-v2-style'), '1.0.0' );
} );
```

---

## Changelog

### v2.0.0 (2025)
- Initiales Release als vollständiges WordPress Block Theme (FSE)
- Azure/PostgreSQL-inspiriertes Design
- 7 Templates, 12 Template Parts, 7 Block Patterns
- Vollständige Plugin-Kompatibilität (alle ThemisDB-Plugins)
- Zentralisiertes `--themis-*` Design-Token-System
- Rückwärtskompatibilität für `--primary-color`, `--themisdb-*`, `--matrix-themis-*`

---

*ThemisDB v2 Theme — MIT License — https://github.com/makr-code/wordpressPlugins*
