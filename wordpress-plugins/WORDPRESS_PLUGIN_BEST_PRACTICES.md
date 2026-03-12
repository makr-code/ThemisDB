# WordPress Plugin Best Practices für ThemisDB

**Version:** 1.0.0  
**Datum:** Januar 2026  
**Status:** Veröffentlicht  
**Zielgruppe:** Plugin-Entwickler, WordPress-Administratoren, ThemisDB Team

---

## Inhaltsverzeichnis

1. [Einleitung](#einleitung)
2. [Design-Standards](#design-standards)
3. [ThemisDB Branding](#themisdb-branding)
4. [Code-Standards](#code-standards)
5. [Sicherheit](#sicherheit)
6. [Performance](#performance)
7. [Barrierefreiheit](#barrierefreiheit)
8. [Testing](#testing)
9. [Dokumentation](#dokumentation)
10. [Plugin-Struktur](#plugin-struktur)
11. [WordPress-Integration](#wordpress-integration)
12. [Kompatibilität](#kompatibilität)

---

## 1. Einleitung

Dieses Dokument definiert Best Practices für die Entwicklung von WordPress-Plugins für das ThemisDB-Projekt. Es stellt sicher, dass alle Plugins ein konsistentes Design, hohe Qualität und optimale Benutzererfahrung bieten.

### Ziele

- **Konsistenz**: Einheitliches Look & Feel über alle Plugins
- **Qualität**: Hohe Code-Qualität und Wartbarkeit
- **Sicherheit**: Schutz vor Sicherheitslücken und Angriffen
- **Performance**: Optimierte Ladezeiten und Ressourcennutzung
- **Barrierefreiheit**: WCAG 2.1 AA Compliance
- **Komfortabilität**: Maximale Benutzerfreundlichkeit für Besucher

---

## 2. Design-Standards

### 2.1 Themis Brand Identity

Alle Plugins MÜSSEN die Themis Brand Identity einhalten:

#### Farbpalette (Primär)

```css
/* Themis Primary Colors */
--themis-primary: #2c3e50;        /* Dunkles Blau-Grau */
--themis-secondary: #3498db;      /* Helles Blau */
--themis-accent: #7c4dff;         /* Lila/Purple */
--themis-success: #27ae60;        /* Grün */
--themis-warning: #f39c12;        /* Orange */
--themis-error: #e74c3c;          /* Rot */
```

#### Farbpalette (Erweitert)

```css
/* Additional Theme Colors */
--themis-dark: #1a252f;           /* Dunklere Variante */
--themis-light: #ecf0f1;          /* Heller Hintergrund */
--themis-gray: #95a5a6;           /* Grau für Text */
--themis-text-primary: #2c3e50;   /* Haupt-Text */
--themis-text-secondary: #7f8c8d; /* Sekundär-Text */
```

#### Verwendungsrichtlinien

| Element | Farbe | Verwendung |
|---------|-------|-----------|
| **Buttons (Primary)** | `--themis-secondary` (#3498db) | Haupt-Aktionen |
| **Buttons (Secondary)** | `--themis-accent` (#7c4dff) | Sekundäre Aktionen |
| **Links** | `--themis-secondary` (#3498db) | Standard-Links |
| **Headers** | `--themis-primary` (#2c3e50) | Überschriften |
| **Success Messages** | `--themis-success` (#27ae60) | Erfolgsmeldungen |
| **Error Messages** | `--themis-error` (#e74c3c) | Fehlermeldungen |
| **Borders/Accents** | `--themis-accent` (#7c4dff) | Hervorhebungen |

### 2.2 Typografie

```css
/* Themis Typography */
--themis-font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, 
                      "Helvetica Neue", Arial, sans-serif;
--themis-font-size-base: 16px;
--themis-font-size-small: 14px;
--themis-font-size-large: 18px;
--themis-font-size-h1: 2.5rem;
--themis-font-size-h2: 2rem;
--themis-font-size-h3: 1.75rem;
--themis-line-height: 1.6;
```

**Schriftgewichte:**
- Regular: 400
- Medium: 500
- Semi-Bold: 600
- Bold: 700

### 2.3 Spacing & Layout

```css
/* Themis Spacing Scale */
--themis-spacing-xs: 0.25rem;   /* 4px */
--themis-spacing-sm: 0.5rem;    /* 8px */
--themis-spacing-md: 1rem;      /* 16px */
--themis-spacing-lg: 1.5rem;    /* 24px */
--themis-spacing-xl: 2rem;      /* 32px */
--themis-spacing-xxl: 3rem;     /* 48px */
```

**Container-Breiten:**
- Small: 600px
- Medium: 800px
- Large: 1200px
- Full: 100%

### 2.4 Border Radius & Shadows

```css
/* Border Radius */
--themis-radius-sm: 4px;
--themis-radius-md: 8px;
--themis-radius-lg: 12px;

/* Box Shadows */
--themis-shadow-sm: 0 1px 3px rgba(0, 0, 0, 0.1);
--themis-shadow-md: 0 4px 6px rgba(0, 0, 0, 0.1);
--themis-shadow-lg: 0 10px 15px rgba(0, 0, 0, 0.1);
--themis-shadow-xl: 0 20px 25px rgba(0, 0, 0, 0.15);
```

### 2.5 Buttons

**Standard Button Styles:**

```css
.themisdb-btn {
    display: inline-block;
    padding: 0.75rem 1.5rem;
    font-size: 1rem;
    font-weight: 600;
    text-align: center;
    text-decoration: none;
    border-radius: var(--themis-radius-md);
    border: none;
    cursor: pointer;
    transition: all 0.3s ease;
    font-family: var(--themis-font-family);
}

.themisdb-btn-primary {
    background: var(--themis-secondary);
    color: #ffffff;
}

.themisdb-btn-primary:hover {
    background: #2980b9;
    transform: translateY(-2px);
    box-shadow: var(--themis-shadow-md);
}

.themisdb-btn-secondary {
    background: var(--themis-accent);
    color: #ffffff;
}

.themisdb-btn-secondary:hover {
    background: #6a3de8;
}

.themisdb-btn-outline {
    background: transparent;
    color: var(--themis-secondary);
    border: 2px solid var(--themis-secondary);
}

.themisdb-btn-outline:hover {
    background: var(--themis-secondary);
    color: #ffffff;
}
```

### 2.6 Icons

**Icon-Standards:**
- **Icon-Set**: Font Awesome 6 oder Material Icons
- **Icon-Größe**: 16px (small), 20px (medium), 24px (large)
- **Icon-Farbe**: Themis Brand Colors

**Konsistente Icon-Verwendung:**

| Funktion | Icon (Font Awesome) | Farbe |
|----------|-------------------|-------|
| Download | `fa-download` | `--themis-secondary` |
| Externe Links | `fa-external-link` | `--themis-secondary` |
| Erfolg | `fa-check-circle` | `--themis-success` |
| Fehler | `fa-exclamation-circle` | `--themis-error` |
| Info | `fa-info-circle` | `--themis-secondary` |
| Warnung | `fa-warning` | `--themis-warning` |
| Dokumentation | `fa-file-alt` | `--themis-primary` |
| GitHub | `fa-github` | `--themis-primary` |

### 2.7 Responsive Design

**Mobile-First Approach:**

```css
/* Mobile (default) */
.themisdb-container {
    padding: 1rem;
}

/* Tablet (768px+) */
@media (min-width: 768px) {
    .themisdb-container {
        padding: 1.5rem;
    }
}

/* Desktop (1024px+) */
@media (min-width: 1024px) {
    .themisdb-container {
        padding: 2rem;
    }
}

/* Large Desktop (1440px+) */
@media (min-width: 1440px) {
    .themisdb-container {
        padding: 3rem;
    }
}
```

### 2.8 Dark Mode Support

Alle Plugins MÜSSEN Dark Mode unterstützen:

```css
/* Dark Mode Support */
@media (prefers-color-scheme: dark) {
    .themisdb-container {
        background: #1a1a1a;
        color: #e0e0e0;
    }
    
    .themisdb-btn-primary {
        background: var(--themis-secondary);
    }
    
    .themisdb-border {
        border-color: #404040;
    }
}
```

---

## 3. ThemisDB Branding

### 3.1 Plugin-Naming Convention

Alle ThemisDB-Plugins MÜSSEN dem Naming-Standard folgen:

**Format:** `themisdb-{funktionalität}`

**Beispiele:**
- ✅ `themisdb-formula-renderer`
- ✅ `themisdb-compendium-downloads`
- ✅ `themisdb-tco-calculator`
- ❌ `formula-renderer` (kein themisdb-Präfix)
- ❌ `ThemisDB_FormulaRenderer` (falsche Schreibweise)

### 3.2 CSS-Klassen Konvention

**BEM-Methodik mit themisdb-Namespace:**

```css
/* Block */
.themisdb-{block} { }

/* Element */
.themisdb-{block}__element { }

/* Modifier */
.themisdb-{block}--modifier { }
```

**Beispiele:**
```css
.themisdb-compendium { }
.themisdb-compendium__header { }
.themisdb-compendium__item { }
.themisdb-compendium__item--professional { }
.themisdb-compendium__button { }
.themisdb-compendium__button--primary { }
```

### 3.3 JavaScript-Namespace

```javascript
// Global namespace
window.ThemisDB = window.ThemisDB || {};

// Plugin-spezifischer Namespace
window.ThemisDB.FormulaRenderer = {
    init: function() { /* ... */ },
    render: function() { /* ... */ }
};

// Lokalisierte Daten
window.themisdbFormula = {
    autoRender: true,
    ajaxUrl: '/wp-admin/admin-ajax.php'
};
```

### 3.4 PHP-Namespace und Präfixe

```php
<?php
/**
 * Alle Funktionen mit themisdb_ präfixen
 */
function themisdb_formula_init() { }
function themisdb_compendium_shortcode() { }

/**
 * Konstanten in UPPERCASE
 */
define('THEMISDB_FORMULA_VERSION', '1.0.0');
define('THEMISDB_COMPENDIUM_PLUGIN_DIR', __DIR__);

/**
 * Klassen mit Namespace
 */
class ThemisDB_Formula_Renderer { }
class ThemisDB_Compendium_Admin { }
```

---

## 4. Code-Standards

### 4.1 PHP Standards

**WordPress Coding Standards befolgen:**

```php
<?php
/**
 * Beispiel für sauberen PHP-Code
 */

// Spaces statt Tabs (4 Spaces)
function themisdb_example_function( $param1, $param2 ) {
    // Yoda Conditions
    if ( 'value' === $param1 ) {
        // Code hier
    }
    
    // Array-Syntax
    $array = array(
        'key1' => 'value1',
        'key2' => 'value2',
    );
    
    // Sanitation
    $safe_input = sanitize_text_field( $param1 );
    
    // Escaping
    echo esc_html( $safe_input );
    
    return $result;
}
```

**Wichtige Regeln:**
- ✅ Immer Spaces verwenden (keine Tabs)
- ✅ Yoda Conditions (`'value' === $var`)
- ✅ Eingaben sanitizen
- ✅ Ausgaben escapen
- ✅ Funktionen dokumentieren (PHPDoc)
- ✅ Nonces verwenden für Sicherheit

### 4.2 JavaScript Standards

**Modern ES6+ JavaScript:**

```javascript
/**
 * Beispiel für modernen JavaScript-Code
 */

(function($) {
    'use strict';
    
    // Const/Let statt Var
    const PLUGIN_VERSION = '1.0.0';
    let isInitialized = false;
    
    // Arrow Functions
    const initialize = () => {
        if (isInitialized) return;
        
        // Event Delegation
        $(document).on('click', '.themisdb-btn', handleClick);
        
        isInitialized = true;
    };
    
    // Destructuring
    const handleClick = (event) => {
        const { target } = event;
        const data = $(target).data('info');
        
        processData(data);
    };
    
    // Template Literals
    const createMessage = (name) => {
        return `Willkommen, ${name}!`;
    };
    
    // Module Pattern
    window.ThemisDB = window.ThemisDB || {};
    window.ThemisDB.Plugin = {
        init: initialize,
        version: PLUGIN_VERSION
    };
    
    // Initialize on ready
    $(document).ready(initialize);
    
})(jQuery);
```

### 4.3 CSS Standards

**BEM + CSS Variables:**

```css
/**
 * ThemisDB Plugin Styles
 */

/* CSS Variables definieren */
:root {
    --plugin-primary: var(--themis-secondary);
    --plugin-spacing: var(--themis-spacing-md);
}

/* BEM Struktur */
.themisdb-plugin {
    padding: var(--plugin-spacing);
    background: var(--themis-light);
}

.themisdb-plugin__header {
    margin-bottom: var(--plugin-spacing);
    color: var(--themis-primary);
}

.themisdb-plugin__item {
    display: flex;
    gap: var(--plugin-spacing);
}

.themisdb-plugin__item--active {
    border-left: 4px solid var(--themis-accent);
}

/* Responsive */
@media (max-width: 768px) {
    .themisdb-plugin {
        padding: calc(var(--plugin-spacing) / 2);
    }
}

/* Dark Mode */
@media (prefers-color-scheme: dark) {
    .themisdb-plugin {
        background: #1a1a1a;
    }
}
```

---

## 5. Sicherheit

### 5.1 Input Validation & Sanitization

**IMMER Eingaben validieren und sanitizen:**

```php
<?php
// Sanitization Functions
$text = sanitize_text_field( $_POST['text'] );
$email = sanitize_email( $_POST['email'] );
$url = esc_url_raw( $_POST['url'] );
$textarea = sanitize_textarea_field( $_POST['content'] );
$integer = absint( $_POST['number'] );

// Custom Validation
function themisdb_validate_custom_input( $input ) {
    // Whitelist approach
    $allowed_values = array( 'option1', 'option2', 'option3' );
    
    if ( ! in_array( $input, $allowed_values, true ) ) {
        return 'option1'; // Default
    }
    
    return $input;
}
```

### 5.2 Output Escaping

**IMMER Ausgaben escapen:**

```php
<?php
// Escaping Functions
echo esc_html( $text );              // Text
echo esc_attr( $attribute );         // HTML-Attribute
echo esc_url( $url );                // URLs
echo esc_js( $javascript );          // JavaScript
echo esc_textarea( $textarea );      // Textarea

// Komplexere Daten
echo wp_kses_post( $html_content );  // Erlaubt Post-HTML
echo wp_kses( $content, $allowed );  // Custom erlaubte Tags
```

### 5.3 Nonces

**Immer Nonces verwenden:**

```php
<?php
// Nonce erstellen
wp_nonce_field( 'themisdb_action', 'themisdb_nonce' );

// Nonce prüfen
if ( ! isset( $_POST['themisdb_nonce'] ) || 
     ! wp_verify_nonce( $_POST['themisdb_nonce'], 'themisdb_action' ) ) {
    wp_die( 'Security check failed' );
}

// AJAX Nonce
wp_localize_script( 'my-script', 'themisdbAjax', array(
    'nonce' => wp_create_nonce( 'themisdb_ajax_nonce' )
));

// JavaScript
$.ajax({
    data: {
        nonce: themisdbAjax.nonce
    }
});
```

### 5.4 SQL Injection Prevention

**Prepared Statements verwenden:**

```php
<?php
global $wpdb;

// FALSCH ❌
$results = $wpdb->get_results( "SELECT * FROM table WHERE id = " . $_GET['id'] );

// RICHTIG ✅
$id = absint( $_GET['id'] );
$results = $wpdb->get_results( 
    $wpdb->prepare( "SELECT * FROM %i WHERE id = %d", $wpdb->prefix . 'table', $id )
);
```

### 5.5 XSS Prevention

```php
<?php
// Content Security Policy
function themisdb_add_csp_header() {
    header( "Content-Security-Policy: default-src 'self'; script-src 'self' 'unsafe-inline' https://cdn.jsdelivr.net; style-src 'self' 'unsafe-inline' https://cdn.jsdelivr.net;" );
}
add_action( 'send_headers', 'themisdb_add_csp_header' );
```

### 5.6 CSRF Protection

```php
<?php
// Capability Checks
if ( ! current_user_can( 'manage_options' ) ) {
    wp_die( 'Unauthorized access' );
}

// Referer Check
check_admin_referer( 'themisdb_action' );
```

---

## 6. Performance

### 6.1 Asset Loading

**Nur bei Bedarf laden:**

```php
<?php
function themisdb_enqueue_scripts() {
    // Nur laden wenn Shortcode vorhanden
    if ( ! has_shortcode( get_post()->post_content, 'themisdb_shortcode' ) ) {
        return;
    }
    
    // CSS
    wp_enqueue_style(
        'themisdb-style',
        plugin_dir_url( __FILE__ ) . 'assets/css/style.css',
        array(),
        THEMISDB_VERSION
    );
    
    // JavaScript
    wp_enqueue_script(
        'themisdb-script',
        plugin_dir_url( __FILE__ ) . 'assets/js/script.js',
        array( 'jquery' ),
        THEMISDB_VERSION,
        true // In footer laden
    );
    
    // Inline-Script minimieren
    wp_localize_script( 'themisdb-script', 'themisdbData', array(
        'ajaxUrl' => admin_url( 'admin-ajax.php' ),
        'nonce' => wp_create_nonce( 'themisdb_nonce' )
    ));
}
add_action( 'wp_enqueue_scripts', 'themisdb_enqueue_scripts' );
```

### 6.2 Caching

**Transients API verwenden:**

```php
<?php
function themisdb_get_data() {
    // Cache-Key
    $cache_key = 'themisdb_data_' . get_current_blog_id();
    
    // Aus Cache holen
    $data = get_transient( $cache_key );
    
    if ( false === $data ) {
        // Daten neu abrufen
        $data = themisdb_fetch_remote_data();
        
        // 1 Stunde cachen
        set_transient( $cache_key, $data, HOUR_IN_SECONDS );
    }
    
    return $data;
}

// Cache löschen
function themisdb_clear_cache() {
    delete_transient( 'themisdb_data_' . get_current_blog_id() );
}
```

### 6.3 Database Optimization

```php
<?php
// Effiziente Queries
function themisdb_get_posts_optimized() {
    $args = array(
        'post_type' => 'post',
        'posts_per_page' => 10,
        'no_found_rows' => true,          // Keine Pagination-Daten
        'update_post_meta_cache' => false, // Kein Meta-Cache
        'update_post_term_cache' => false, // Kein Term-Cache
        'fields' => 'ids'                  // Nur IDs holen
    );
    
    return new WP_Query( $args );
}
```

### 6.4 Lazy Loading

```javascript
// Lazy Load Images
document.addEventListener('DOMContentLoaded', function() {
    const images = document.querySelectorAll('img[data-src]');
    
    const imageObserver = new IntersectionObserver((entries, observer) => {
        entries.forEach(entry => {
            if (entry.isIntersecting) {
                const img = entry.target;
                img.src = img.dataset.src;
                img.removeAttribute('data-src');
                observer.unobserve(img);
            }
        });
    });
    
    images.forEach(img => imageObserver.observe(img));
});
```

---

## 7. Barrierefreiheit

### 7.1 WCAG 2.1 AA Compliance

**Mindestanforderungen:**
- ✅ Farbkontrast mindestens 4.5:1 (Text)
- ✅ Farbkontrast mindestens 3:1 (UI-Elemente)
- ✅ Tastaturnavigation vollständig möglich
- ✅ Screen Reader kompatibel
- ✅ Fokus-Indikatoren sichtbar
- ✅ Semantisches HTML

### 7.2 Semantic HTML

```html
<!-- Korrekte Struktur -->
<article class="themisdb-item" role="article">
    <header class="themisdb-item__header">
        <h2 class="themisdb-item__title">Titel</h2>
    </header>
    
    <div class="themisdb-item__content">
        <p>Inhalt...</p>
    </div>
    
    <footer class="themisdb-item__footer">
        <a href="#" 
           class="themisdb-btn themisdb-btn-primary"
           role="button"
           aria-label="Kompendium herunterladen">
            Download
        </a>
    </footer>
</article>
```

### 7.3 ARIA Labels

```html
<!-- Buttons mit ARIA -->
<button class="themisdb-btn" 
        aria-label="Kompendium Professional Version herunterladen">
    <i class="fa fa-download" aria-hidden="true"></i>
    Download
</button>

<!-- Navigation -->
<nav aria-label="Haupt-Navigation">
    <ul role="menubar">
        <li role="menuitem"><a href="#">Home</a></li>
        <li role="menuitem"><a href="#">Docs</a></li>
    </ul>
</nav>

<!-- Form Labels -->
<label for="themisdb-email">E-Mail-Adresse:</label>
<input type="email" 
       id="themisdb-email" 
       name="email"
       aria-required="true"
       aria-describedby="email-help">
<small id="email-help">Wir werden Ihre E-Mail nicht weitergeben.</small>
```

### 7.4 Keyboard Navigation

```css
/* Fokus-Indikatoren */
.themisdb-btn:focus,
.themisdb-link:focus {
    outline: 3px solid var(--themis-accent);
    outline-offset: 2px;
}

/* Nicht :focus-visible für bessere Kompatibilität entfernen */
.themisdb-btn:focus:not(:focus-visible) {
    /* Nur bei Maus-Klick outline ausblenden */
    outline: none;
}
```

```javascript
// Tab-Index Management
document.addEventListener('keydown', function(e) {
    if (e.key === 'Tab') {
        document.body.classList.add('keyboard-navigation');
    }
});

document.addEventListener('mousedown', function() {
    document.body.classList.remove('keyboard-navigation');
});
```

### 7.5 Skip Links

```html
<!-- Skip Link für Screen Reader -->
<a href="#main-content" class="skip-link screen-reader-text">
    Zum Hauptinhalt springen
</a>

<main id="main-content" role="main">
    <!-- Content -->
</main>
```

```css
.skip-link {
    position: absolute;
    top: -40px;
    left: 0;
    background: var(--themis-primary);
    color: white;
    padding: 8px;
    text-decoration: none;
}

.skip-link:focus {
    top: 0;
}
```

---

## 8. Testing

### 8.1 Browser Testing

**Mindest-Browser-Support:**
- ✅ Chrome (letzte 2 Versionen)
- ✅ Firefox (letzte 2 Versionen)
- ✅ Safari (letzte 2 Versionen)
- ✅ Edge (letzte 2 Versionen)
- ✅ Mobile Safari (iOS)
- ✅ Chrome Mobile (Android)

### 8.2 Responsive Testing

**Breakpoints testen:**
- 320px (Mobile klein)
- 375px (Mobile mittel)
- 768px (Tablet)
- 1024px (Desktop)
- 1440px (Large Desktop)

### 8.3 Accessibility Testing

**Tools:**
- WAVE (Browser Extension)
- axe DevTools
- Lighthouse Accessibility Score
- Screen Reader (NVDA/JAWS)
- Keyboard-Navigation Test

### 8.4 Performance Testing

**Metriken:**
- First Contentful Paint < 1.8s
- Largest Contentful Paint < 2.5s
- Total Blocking Time < 200ms
- Cumulative Layout Shift < 0.1
- Time to Interactive < 3.8s

**Tools:**
- Google PageSpeed Insights
- GTmetrix
- WebPageTest
- Chrome DevTools

### 8.5 Security Testing

```bash
# WP-CLI Security Check
wp plugin verify-checksums --all

# WordPress Plugin Check
wp plugin check themisdb-plugin-name

# PHP Security Scanner
phpcs --standard=WordPress-Extra plugin-file.php
```

---

## 9. Dokumentation

### 9.1 README.md

Jedes Plugin MUSS ein README.md enthalten:

```markdown
# ThemisDB Plugin Name

Kurzbeschreibung des Plugins.

## Features

- ✅ Feature 1
- ✅ Feature 2

## Installation

### Methode 1: Manual Upload
...

## Verwendung

### Shortcode
...

## Konfiguration

...

## Changelog

Siehe [CHANGELOG.md](CHANGELOG.md)
```

### 9.2 CHANGELOG.md

```markdown
# Changelog

## [1.0.1] - 2026-01-20

### Added
- Neue Feature X

### Changed
- Verbesserte Performance

### Fixed
- Bug Fix Y

### Security
- Sicherheitslücke Z geschlossen
```

### 9.3 Inline Documentation

```php
<?php
/**
 * Function description
 *
 * @since 1.0.0
 * @param string $param1 Description of param1
 * @param int    $param2 Description of param2
 * @return bool True on success, false on failure
 */
function themisdb_example_function( $param1, $param2 ) {
    // Implementation
}
```

---

## 10. Plugin-Struktur

### 10.1 Empfohlene Verzeichnisstruktur

```
themisdb-plugin-name/
├── assets/
│   ├── css/
│   │   ├── style.css
│   │   └── admin-style.css
│   ├── js/
│   │   ├── script.js
│   │   └── admin-script.js
│   └── images/
│       └── icon.svg
├── includes/
│   ├── class-plugin-name.php
│   ├── class-admin.php
│   └── class-widget.php
├── languages/
│   └── themisdb-plugin-name.pot
├── templates/
│   ├── shortcode.php
│   └── widget.php
├── CHANGELOG.md
├── LICENSE
├── README.md
├── package.sh
└── themisdb-plugin-name.php
```

### 10.2 Haupt-Plugin-Datei

```php
<?php
/**
 * Plugin Name: ThemisDB Plugin Name
 * Plugin URI: https://github.com/makr-code/wordpressPlugins
 * Description: Plugin description
 * Version: 1.0.0
 * Author: ThemisDB Team
 * Author URI: https://github.com/makr-code/wordpressPlugins
 * License: MIT
 * Text Domain: themisdb-plugin-name
 * Domain Path: /languages
 * Requires at least: 5.0
 * Requires PHP: 7.2
 */

// Prevent direct access
if ( ! defined( 'ABSPATH' ) ) {
    exit;
}

// Plugin constants
define( 'THEMISDB_PLUGIN_VERSION', '1.0.0' );
define( 'THEMISDB_PLUGIN_DIR', plugin_dir_path( __FILE__ ) );
define( 'THEMISDB_PLUGIN_URL', plugin_dir_url( __FILE__ ) );

// Include required files
require_once THEMISDB_PLUGIN_DIR . 'includes/class-plugin-name.php';

// Initialize
function themisdb_plugin_init() {
    new ThemisDB_Plugin_Name();
    load_plugin_textdomain( 
        'themisdb-plugin-name', 
        false, 
        dirname( plugin_basename( __FILE__ ) ) . '/languages' 
    );
}
add_action( 'plugins_loaded', 'themisdb_plugin_init' );

// Activation hook
function themisdb_plugin_activate() {
    // Set default options
}
register_activation_hook( __FILE__, 'themisdb_plugin_activate' );
```

---

## 11. WordPress-Integration

### 11.1 Hooks & Filters

```php
<?php
// Actions
add_action( 'init', 'themisdb_plugin_init' );
add_action( 'wp_enqueue_scripts', 'themisdb_enqueue_scripts' );
add_action( 'admin_menu', 'themisdb_add_admin_menu' );

// Filters
add_filter( 'the_content', 'themisdb_filter_content' );
add_filter( 'plugin_action_links_' . plugin_basename( __FILE__ ), 'themisdb_add_settings_link' );

// Custom Actions
do_action( 'themisdb_before_render' );
do_action( 'themisdb_after_render' );

// Custom Filters
$data = apply_filters( 'themisdb_data', $data );
```

### 11.2 Shortcodes

```php
<?php
function themisdb_shortcode_handler( $atts, $content = null ) {
    // Parse attributes
    $atts = shortcode_atts( array(
        'style' => 'modern',
        'layout' => 'cards',
        'show_version' => 'yes'
    ), $atts, 'themisdb_shortcode' );
    
    // Buffer output
    ob_start();
    
    // Include template
    include THEMISDB_PLUGIN_DIR . 'templates/shortcode.php';
    
    return ob_get_clean();
}
add_shortcode( 'themisdb_shortcode', 'themisdb_shortcode_handler' );
```

### 11.3 Widgets

```php
<?php
class ThemisDB_Widget extends WP_Widget {
    
    public function __construct() {
        parent::__construct(
            'themisdb_widget',
            __( 'ThemisDB Widget', 'themisdb-plugin' ),
            array( 'description' => __( 'Widget description', 'themisdb-plugin' ) )
        );
    }
    
    public function widget( $args, $instance ) {
        echo $args['before_widget'];
        // Widget output
        echo $args['after_widget'];
    }
    
    public function form( $instance ) {
        // Admin form
    }
    
    public function update( $new_instance, $old_instance ) {
        // Save widget settings
    }
}

// Register widget
function themisdb_register_widgets() {
    register_widget( 'ThemisDB_Widget' );
}
add_action( 'widgets_init', 'themisdb_register_widgets' );
```

---

## 12. Kompatibilität

### 12.1 WordPress Version

- **Minimum**: WordPress 5.0+
- **Empfohlen**: WordPress 6.0+
- **Tested up to**: Immer aktuelle Version testen

### 12.2 PHP Version

- **Minimum**: PHP 7.2+
- **Empfohlen**: PHP 8.0+
- **Maximum**: PHP 8.3+ (forward compatible)

### 12.3 Plugin-Kompatibilität

**Getestet mit:**
- WP Rocket (Caching)
- Rank Math SEO
- Wordfence Security
- Elementor Page Builder
- WooCommerce (falls relevant)

### 12.4 Theme-Kompatibilität

Alle Plugins MÜSSEN kompatibel sein mit:
- ✅ ThemisDB Theme (primary)
- ✅ Twenty Twenty-Four
- ✅ Astra
- ✅ GeneratePress
- ✅ Andere Standard WordPress Themes

---

## 13. Checkliste vor Release

### Pre-Release Checklist

- [ ] **Code Quality**
  - [ ] PHP Coding Standards eingehalten
  - [ ] JavaScript Code optimiert
  - [ ] CSS validiert
  - [ ] Keine console.log() im Production Code
  
- [ ] **Sicherheit**
  - [ ] Alle Eingaben validiert/sanitized
  - [ ] Alle Ausgaben escaped
  - [ ] Nonces implementiert
  - [ ] Security Scan durchgeführt
  
- [ ] **Performance**
  - [ ] Assets nur bei Bedarf geladen
  - [ ] Caching implementiert
  - [ ] Bilder optimiert
  - [ ] PageSpeed Score > 90
  
- [ ] **Design**
  - [ ] Themis Brand Colors verwendet
  - [ ] Responsive auf allen Breakpoints
  - [ ] Dark Mode funktioniert
  - [ ] Icons konsistent
  
- [ ] **Barrierefreiheit**
  - [ ] WCAG 2.1 AA konform
  - [ ] Tastaturnavigation funktioniert
  - [ ] Screen Reader getestet
  - [ ] Lighthouse Accessibility Score > 90
  
- [ ] **Testing**
  - [ ] Alle Browser getestet
  - [ ] Mobile Geräte getestet
  - [ ] Mit Caching Plugins getestet
  - [ ] Mit Page Builders getestet
  
- [ ] **Dokumentation**
  - [ ] README.md vollständig
  - [ ] CHANGELOG.md aktualisiert
  - [ ] Inline-Kommentare vorhanden
  - [ ] Admin-Hilfe implementiert
  
- [ ] **WordPress Standards**
  - [ ] Plugin Check Tool durchgelaufen
  - [ ] WP Coding Standards erfüllt
  - [ ] Translations Ready
  - [ ] Kompatibilität getestet

---

## 14. Support & Ressourcen

### Dokumentation
- [WordPress Plugin Handbook](https://developer.wordpress.org/plugins/)
- [WordPress Coding Standards](https://developer.wordpress.org/coding-standards/)
- [WordPress Theme Handbook](https://developer.wordpress.org/themes/)

### Tools
- [WordPress Plugin Check](https://wordpress.org/plugins/plugin-check/)
- [PHP CodeSniffer](https://github.com/squizlabs/PHP_CodeSniffer)
- [ESLint](https://eslint.org/)
- [Stylelint](https://stylelint.io/)

### ThemisDB Ressourcen
- [ThemisDB Repository](https://github.com/makr-code/wordpressPlugins)
- [ThemisDB Contributing Guidelines](../../CONTRIBUTING.md)
- [WordPress Theme Documentation](../docs/WORDPRESS_THEME_COMPLETE.md)

---

## 15. Kontakt

### Fragen zu diesem Dokument?

**GitHub Issues:** https://github.com/makr-code/wordpressPlugins/issues  
**Diskussionen:** https://github.com/makr-code/wordpressPlugins/discussions

---

**Dokument-Version:** 1.0.0  
**Letzte Aktualisierung:** Januar 2026  
**Maintainer:** ThemisDB Team  
**Lizenz:** MIT
