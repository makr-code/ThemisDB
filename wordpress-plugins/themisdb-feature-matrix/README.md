# ThemisDB Feature Matrix WordPress Plugin

Interactive comparison matrix showcasing ThemisDB's superior features vs PostgreSQL, MongoDB, Neo4j with AI/ML capabilities, multi-model support, and comprehensive functionality.

## 📋 Overview

The ThemisDB Feature Matrix plugin provides a comprehensive, interactive feature comparison between ThemisDB and competing databases. It highlights ThemisDB's unique advantages, particularly in AI/ML integration, making it easy for users to understand why ThemisDB is the superior choice.

## ✨ Features

### 📊 Comprehensive Comparison
- ✅ **22+ Features** across 5 categories (expandable to 40+)
- ✅ **6 Data Models**: Relational SQL, Graph, Document Store, Vector/Embeddings, Time-Series, Key-Value
- ✅ **4 AI/ML Features**: Embedded LLM (exclusive), Vector Search, RAG Support (exclusive), GPU Acceleration (exclusive)
- ✅ **4 Performance Features**: Horizontal Scaling, Auto-Sharding, Replication, Built-in Caching
- ✅ **5 Compatibility Features**: SQL, MongoDB, Cypher, REST API, GraphQL API
- ✅ **3 Licensing Metrics**: License Type, Commercial Use, Cloud Vendor Lock-in

### 🎨 Interactive Features
- ✅ **Category Filtering** - Quick-access buttons and dropdown for filtering by feature category
- ✅ **Column Sorting** - Click column headers to sort by support level (full > limited > no)
- ✅ **CSV Export** - Download the entire comparison as a spreadsheet
- ✅ **Print Support** - Optimized print layout for documentation
- ✅ **Tooltips** - Hover info icons for detailed feature descriptions
- ✅ **Mobile Card View** - Responsive design switches to card layout on mobile devices

### 💎 ThemisDB Highlighting
- ✅ **Column Highlighting** - ThemisDB column with special background gradient
- ✅ **"⭐ Recommended" Badge** - Prominent badge on ThemisDB column header
- ✅ **Exclusive Feature Markers** - Stars (⭐) for ThemisDB-only features
- ✅ **Themis Brand Colors** - Consistent color scheme (#2c3e50, #3498db, #7c4dff)

### 📱 Responsive Design
- ✅ **Desktop** (>768px) - Full table with sticky header
- ✅ **Tablet** (768px-1024px) - Compact table view
- ✅ **Mobile** (<768px) - Card-based layout with swipeable categories
- ✅ **Touch-Optimized** - Large tap targets for mobile interaction

### ♿ Accessibility (WCAG 2.1 AA)
- ✅ **Semantic HTML** - Proper `<table>`, `<th>`, `<td>` structure with ARIA roles
- ✅ **ARIA Labels** - All interactive elements labeled for screen readers
- ✅ **Keyboard Navigation** - Full tab navigation through all controls
- ✅ **Screen Reader Support** - Table captions and aria-live regions
- ✅ **High Contrast Mode** - Increased borders and weights for visibility

### 🎭 Style Variants
- **Modern** (default) - Full-featured with shadows and gradients
- **Compact** - Reduced padding and font sizes
- **Minimal** - Clean design without borders/shadows

## 🚀 Installation

### Method 1: Manual Installation

1. **Upload Plugin**
   ```bash
   cd /path/to/wordpress/wp-content/plugins/
   cp -r /path/to/ThemisDB/wordpress-plugin/themisdb-feature-matrix-wordpress ./
   ```

2. **Activate**
   - Navigate to WordPress Admin → Plugins
   - Find "ThemisDB Feature Matrix"
   - Click "Activate"

3. **Configure** (Optional)
   - Go to Settings → Feature Matrix
   - Adjust default settings as needed

## 📖 Usage

### Basic Shortcode

```php
[themisdb_feature_matrix]
```

### Shortcode Parameters

#### Category Filtering
```php
[themisdb_feature_matrix category="all"]          // Show all features
[themisdb_feature_matrix category="data_models"]  // Data model features only
[themisdb_feature_matrix category="ai_ml"]        // AI/ML features only ⭐
[themisdb_feature_matrix category="performance"]  // Performance features
[themisdb_feature_matrix category="compatibility"] // Compatibility features
[themisdb_feature_matrix category="licensing"]    // Licensing information
```

#### Visual Style
```php
[themisdb_feature_matrix style="modern"]    // Full-featured (default)
[themisdb_feature_matrix style="compact"]   // Space-saving
[themisdb_feature_matrix style="minimal"]   // Clean and simple
```

#### Display Options
```php
[themisdb_feature_matrix highlight_themis="yes"]  // Highlight ThemisDB column (default)
[themisdb_feature_matrix highlight_themis="no"]   // No highlighting

[themisdb_feature_matrix sticky_header="yes"]     // Sticky header (default)
[themisdb_feature_matrix sticky_header="no"]      // Static header

[themisdb_feature_matrix filterable="yes"]        // Show category buttons (default)
[themisdb_feature_matrix filterable="no"]         // Hide filter buttons
```

#### Combined Example
```php
[themisdb_feature_matrix 
    category="ai_ml" 
    style="modern" 
    highlight_themis="yes"
    sticky_header="yes"
    filterable="yes"]
```

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
├── README.md                        # This file
├── CHANGELOG.md                     # Version history
├── LICENSE                          # MIT License
├── includes/
│   ├── class-feature-matrix.php    # Feature data class (22+ features)
│   └── class-admin.php             # Admin settings interface
├── assets/
│   ├── css/
│   │   └── feature-matrix.css      # Themis-branded styles with responsive design
│   ├── js/
│   │   └── feature-matrix.js       # Interactive features (sorting, CSV, cards)
│   └── images/                     # Database logos (placeholder)
└── templates/
    └── matrix.php                  # Main display template
```

### Technologies

- **PHP** 7.4+ - WordPress plugin development
- **JavaScript** ES5+ with jQuery - Interactive features
- **CSS3** - Modern responsive styling with Themis colors
- **WordPress APIs** - Settings API, Shortcode API, AJAX

### Status Values

- ✓ **full** - Fully supported natively (green)
- ◐ **limited** - Partially supported or requires extensions (orange)
- ✗ **no** - Not available (red)
- **Text** - Custom values for licensing fields

### Themis Brand Colors

```css
--matrix-themis-primary: #2c3e50;    /* Dark blue-gray */
--matrix-themis-secondary: #3498db;   /* Bright blue */
--matrix-themis-accent: #7c4dff;      /* Purple accent */
--matrix-status-full: #27ae60;        /* Green (full support) */
--matrix-status-limited: #f39c12;     /* Orange (limited) */
--matrix-status-no: #e74c3c;          /* Red (not available) */
```

## ⚙️ Admin Settings

Access via **Settings → Feature Matrix**

- **Default View** - Choose which category displays by default
- **Default Style** - Select modern, compact, or minimal
- **Enable Category Filters** - Show/hide filter buttons
- **Enable CSV Export** - Show/hide export button
- **Show ThemisDB Highlight** - Toggle column highlighting
- **Sticky Header** - Enable/disable sticky table header
- **Enable Tooltips** - Show/hide info tooltips

## 🎯 Key Highlights

### ThemisDB Exclusive Features (Full Support Only in ThemisDB)

1. **Embedded LLM** ⭐ - Run LLaMA models directly in the database
2. **RAG Support** ⭐ - Built-in Retrieval-Augmented Generation
3. **GPU Acceleration** ⭐ - Hardware acceleration for ML workloads

### Multi-Model Excellence

ThemisDB is the only database with **full support** for:
- Relational SQL
- Graph Database
- Document Store
- Vector/Embeddings
- Time-Series
- Key-Value

## 📊 Feature Count

- **Total Features**: 22+
- **Categories**: 5
- **Databases Compared**: 4 (ThemisDB, PostgreSQL, MongoDB, Neo4j)
- **ThemisDB Advantages**: 5+ exclusive "full" features

## 🔒 Security

- **Nonce Verification** - All AJAX requests use WordPress nonces
- **Capability Checks** - Admin functions require proper permissions
- **Input Sanitization** - All inputs sanitized with WordPress functions
- **Output Escaping** - All outputs properly escaped
- **XSS Protection** - HTML escaping in JavaScript

## 🌐 Browser Support

- Chrome/Edge 90+
- Firefox 88+
- Safari 14+
- Mobile browsers (iOS Safari, Chrome Mobile)

## 📄 License

MIT License - See LICENSE file for details

## 🔗 Links

- **GitHub**: [makr-code/wordpressPlugins](https://github.com/makr-code/wordpressPlugins)
- **Plugin Directory**: `wordpress-plugin/themisdb-feature-matrix-wordpress/`
- **Documentation**: This README

## 🗺️ Roadmap

- [ ] Gutenberg block support
- [ ] Custom feature definitions via UI
- [ ] Feature comparison export to PDF
- [ ] Multi-language support (i18n)
- [ ] Animation effects for column sorting
- [ ] Expandable feature details
- [ ] Comparison with additional databases

## 📝 Version History

See [CHANGELOG.md](CHANGELOG.md) for detailed version history.

### Version 1.0.0 (2026-02-11)
- Initial release with 22+ features
- Interactive filtering and sorting
- CSV export functionality
- Mobile card view
- ThemisDB highlighting
- WCAG 2.1 AA accessibility
- Themis brand colors

---

**Powered by [ThemisDB](https://github.com/makr-code/wordpressPlugins)** - The Multi-Model Database with AI/ML Integration
