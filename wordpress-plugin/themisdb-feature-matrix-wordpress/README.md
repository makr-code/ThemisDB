# ThemisDB Feature Matrix - WordPress Plugin v1.0.0

Interactive feature comparison matrix for ThemisDB vs. competing databases (PostgreSQL, MongoDB, Neo4j).

## 📋 Overview

A WordPress plugin that provides a comprehensive, interactive feature comparison between ThemisDB and three major competing databases. Designed for marketing websites, documentation, and product pages to clearly demonstrate ThemisDB's unique capabilities.

### Key Features

- **Interactive Comparison Table** - Compare ThemisDB, PostgreSQL, MongoDB, and Neo4j side-by-side
- **Category Filtering** - Filter by Data Models, AI/ML, Performance, Compatibility, and Licensing
- **Column Sorting** - Sort by support level for any database
- **Hover Tooltips** - Detailed explanations for each feature
- **CSV Export** - Download comparison data with date-stamped filenames
- **Mobile Card View** - Automatic responsive layout for screens < 768px
- **Dark Mode** - Auto-detection via `prefers-color-scheme`
- **WCAG 2.1 AA Compliant** - Full accessibility support
- **Themis Branding** - Consistent color scheme and highlighting

## 🚀 Installation

### Method 1: Manual Installation

1. Download the plugin folder
2. Upload to `/wp-content/plugins/themisdb-feature-matrix-wordpress/`
3. Activate the plugin through WordPress Admin → Plugins
4. Configure settings at Settings → Feature Matrix

### Method 2: From Repository

```bash
cd /path/to/wordpress/wp-content/plugins/
git clone https://github.com/makr-code/ThemisDB.git
cp -r ThemisDB/wordpress-plugin/themisdb-feature-matrix-wordpress ./
```

Then activate via WordPress admin panel.

## 📖 Usage

### Basic Shortcode

```php
[themisdb_feature_matrix]
```

### Shortcode Parameters

| Parameter | Values | Default | Description |
|-----------|--------|---------|-------------|
| `category` | `all`, `data_models`, `ai_ml`, `performance`, `compatibility`, `pricing` | `all` | Filter features by category |
| `style` | `modern`, `minimal` | `modern` | Visual style |
| `show_legend` | `yes`, `no` | `yes` | Display status legend |
| `filterable` | `yes`, `no` | `yes` | Enable category filtering |
| `sticky_header` | `yes`, `no` | `yes` | Sticky table header on scroll |
| `highlight_themis` | `yes`, `no` | `yes` | Highlight ThemisDB column |

### Examples

**Show only AI/ML features:**
```php
[themisdb_feature_matrix category="ai_ml"]
```

**Minimal style without filtering:**
```php
[themisdb_feature_matrix style="minimal" filterable="no"]
```

**Performance comparison with all options:**
```php
[themisdb_feature_matrix 
    category="performance" 
    style="modern" 
    show_legend="yes" 
    sticky_header="yes"]
```

## 🎨 Feature Categories

### Data Models
- Relational (SQL)
- Graph Database
- Document (NoSQL)
- Vector/Embeddings ⭐ *Highlighted*
- Time-Series
- Key-Value Store

### AI/ML Features
- Embedded LLM ⭐ *Highlighted*
- Vector Similarity Search
- RAG (Retrieval-Augmented Generation) ⭐ *Highlighted*
- GPU Acceleration ⭐ *Highlighted*

### Performance & Scaling
- Horizontal Scaling
- Auto-Sharding
- Replication
- Built-in Caching

### Protocol Compatibility
- SQL Protocol
- MongoDB Protocol
- Cypher (Graph)
- REST API
- GraphQL API

### Licensing & Cost
- License Type
- Free for Commercial Use

## ⚙️ Admin Settings

Access settings at **Settings → Feature Matrix** in WordPress admin.

### Configuration Options

- **Default Category** - Category shown on initial load
- **Default Style** - Visual appearance (Modern/Minimal)
- **Enable Features**
  - Show Legend
  - Enable Filtering
  - Enable Sorting
  - Sticky Header
  - Highlight ThemisDB
- **Export Settings**
  - Enable CSV Export
  - Export Filename Prefix

## ♿ Accessibility (WCAG 2.1 AA)

The plugin is fully accessible with:

- **Semantic HTML** - Proper `<table>`, `<th>`, `<caption>` elements
- **ARIA Labels** - `role` and `aria-label` attributes
- **Keyboard Navigation** - Tab, Enter, Space key support
- **Focus Indicators** - 3px accent-colored outlines
- **Screen Reader Support** - `sr-only` class for hidden text
- **Color Contrast** - Minimum 4.5:1 ratio for all text
- **Alternative Text** - `aria-label` for status icons

## 📱 Responsive Design

### Desktop (≥ 768px)
- Full comparison table
- Column sorting
- Sticky header
- Hover tooltips

### Mobile (< 768px)
- Card-based layout
- One feature per card
- All database comparisons visible
- Touch-friendly buttons

## 🎨 Themis Brand Colors

```css
--themis-primary: #2c3e50;    /* Dark blue-gray */
--themis-secondary: #3498db;  /* Blue */
--themis-accent: #7c4dff;     /* Purple */
--themis-success: #27ae60;    /* Green (Full Support) */
--themis-warning: #f39c12;    /* Orange (Limited Support) */
--themis-error: #e74c3c;      /* Red (No Support) */
```

## 🔒 Security

- **Nonce Verification** - All form submissions
- **Capability Checks** - `manage_options` for admin
- **Input Sanitization** - `sanitize_text_field()` on all inputs
- **Output Escaping** - `esc_html()`, `esc_attr()` on outputs
- **XSS Prevention** - HTML escaping in JavaScript

## 📊 Feature Status Indicators

| Icon | Status | Meaning |
|------|--------|---------|
| ✓ | Full Support | Fully supported natively |
| ◐ | Limited Support | Available with limitations |
| ✗ | No Support | Feature not supported |

## 🛠️ Technical Requirements

- **PHP:** 7.4 or higher
- **WordPress:** 5.8 or higher
- **Browser:** Chrome 120+, Firefox 120+, Safari 17+, Edge 120+
- **JavaScript:** Enabled (for interactive features)

## 📂 File Structure

```
themisdb-feature-matrix-wordpress/
├── themisdb-feature-matrix.php      # Main plugin file
├── README.md                         # This file
├── CHANGELOG.md                      # Version history
├── LICENSE                           # MIT License
├── uninstall.php                     # Cleanup on uninstall
├── assets/
│   ├── css/
│   │   ├── feature-matrix.css       # Main styles
│   │   └── feature-matrix-dark.css  # Dark mode styles
│   ├── js/
│   │   └── feature-matrix.js        # Interactive functionality
│   └── images/                       # (Reserved for logos)
├── includes/
│   ├── class-feature-matrix.php     # Core feature data class
│   ├── class-shortcode.php          # Shortcode handler
│   └── class-admin.php              # Admin settings
└── templates/
    ├── matrix.php                   # Main table template
    └── admin-settings.php           # Admin page template
```

## 🧪 Testing

### Functional Tests
- ✅ Shortcode renders correctly
- ✅ All 4 databases displayed
- ✅ Filter buttons work
- ✅ Column sorting functional
- ✅ Tooltips display on hover
- ✅ CSV export downloads file
- ✅ Mobile view switches at 768px

### Accessibility Tests
- ✅ Screen reader compatibility
- ✅ Keyboard navigation
- ✅ Focus indicators visible
- ✅ Color contrast passes WCAG AA

### Browser Tests
- ✅ Chrome 120+
- ✅ Firefox 120+
- ✅ Safari 17+
- ✅ Edge 120+

## 📄 License

MIT License - See [LICENSE](LICENSE) file for details.

## 🔗 Links

- **GitHub Repository:** [makr-code/ThemisDB](https://github.com/makr-code/ThemisDB)
- **Plugin Path:** `/wordpress-plugin/themisdb-feature-matrix-wordpress/`
- **ThemisDB Documentation:** [docs/](../../docs/)

## 🐛 Troubleshooting

### Shortcode not rendering
- Verify plugin is activated
- Check for JavaScript errors in browser console
- Ensure theme supports shortcodes

### Table not displaying
- Check browser console for JavaScript errors
- Verify jQuery is loaded
- Clear browser cache

### CSS not loading
- Check file permissions (644 for files, 755 for directories)
- Clear WordPress cache
- Check for theme CSS conflicts

### Mobile view not switching
- Test with browser responsive mode
- Check window width is < 768px
- Verify JavaScript is enabled

## 📞 Support

For issues, questions, or contributions:
1. Check existing [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
2. Create a new issue with detailed information
3. Include WordPress version, PHP version, and browser details

## 🗺️ Roadmap

- [ ] Gutenberg block support
- [ ] Additional database comparisons (MySQL, Redis)
- [ ] Custom feature definitions via admin
- [ ] JSON import/export
- [ ] Multi-language support (i18n)
- [ ] Database logo SVGs
- [ ] Comparison permalink generation

---

**Version:** 1.0.0  
**Last Updated:** 2024-02-11  
**Author:** ThemisDB Team  
**License:** MIT
