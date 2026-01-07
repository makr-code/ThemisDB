# ThemisDB WordPress Plugins - Overview

This directory contains WordPress plugins specifically developed for ThemisDB to showcase performance benchmarks, feature comparisons, and other ThemisDB-specific data visualizations.

## 📦 Available Plugins

### 1. TCO Calculator ✅ (Existing)
**Location**: `/tools/tco-calculator-wordpress/`
**Status**: Production-ready
**Shortcode**: `[themisdb_tco_calculator]`

Calculate and compare Total Cost of Ownership between ThemisDB and competing database solutions.

**Features**:
- Interactive cost calculator
- Infrastructure, personnel, and license cost comparison
- AI/LLM cost analysis
- Export to PDF/CSV
- Chart.js visualizations

---

### 2. Benchmark Visualizer ✅ (New - Phase 1.1)
**Location**: `/tools/benchmark-visualizer-wordpress/`
**Status**: Production-ready
**Shortcode**: `[themisdb_benchmark_visualizer]`

Interactive visualization of ThemisDB performance benchmarks against PostgreSQL, MongoDB, Neo4j, and other databases.

**Features**:
- Performance metrics comparison (latency, throughput, memory)
- Category filtering (Vector Search, AQL, Graph, Documents, Transactions)
- Multiple chart types (bar, line, radar)
- Real-time data loading with caching
- Export to CSV/PDF
- Chart.js integration
- Responsive design
- Performance insights generation

**Installation**:
```bash
cd /path/to/wordpress/wp-content/plugins/
cp -r /path/to/ThemisDB/tools/benchmark-visualizer-wordpress ./themisdb-benchmark-visualizer
# Activate via WordPress Admin → Plugins
```

**Usage Examples**:
```php
// Basic usage
[themisdb_benchmark_visualizer]

// Filter by category
[themisdb_benchmark_visualizer category="vector_search"]

// Specify metric
[themisdb_benchmark_visualizer metric="latency"]

// Custom chart type
[themisdb_benchmark_visualizer chart_type="radar"]

// Combined
[themisdb_benchmark_visualizer category="vector_search" metric="throughput" chart_type="bar"]
```

---

### 3. Feature Matrix ✅ (New - Phase 1.2)
**Location**: `/tools/feature-matrix-wordpress/`
**Status**: Production-ready
**Shortcode**: `[themisdb_feature_matrix]`

Interactive feature comparison matrix showing ThemisDB capabilities compared to competing databases, with Mermaid.js diagram visualization.

**Features**:
- Feature availability matrix
- Category filtering (Architecture, AI/ML, Scalability, Security, Reliability, Usability)
- Status indicators (Available ✅, Partial ⚠️, Limited 🔧, Not Available ❌)
- Mermaid.js mind map diagrams
- Detailed and compact view modes
- Tooltips for feature descriptions
- Export to CSV/PDF
- Responsive design

**Installation**:
```bash
cd /path/to/wordpress/wp-content/plugins/
cp -r /path/to/ThemisDB/tools/feature-matrix-wordpress ./themisdb-feature-matrix
# Activate via WordPress Admin → Plugins
```

**Usage Examples**:
```php
// Basic usage
[themisdb_feature_matrix]

// Filter by category
[themisdb_feature_matrix category="ai_ml"]

// Compact view
[themisdb_feature_matrix view="compact"]

// Without diagram
[themisdb_feature_matrix show_diagram="false"]

// Combined
[themisdb_feature_matrix category="security" view="detailed" show_diagram="true"]
```

---

## 🏗️ Design Pattern

All plugins follow the **TCO Calculator** template pattern for consistency:

### Common Structure
```
themisdb-<plugin-name>/
├── themisdb-<plugin-name>.php    # Main plugin file
├── assets/
│   ├── css/
│   │   └── <plugin-name>.css     # Consistent styling
│   └── js/
│       └── <plugin-name>.js      # JavaScript logic
├── templates/
│   ├── <main-template>.php       # Display template
│   └── admin-settings.php        # Settings page
├── data/                         # (optional) Local data files
├── README.md
├── LICENSE                       # MIT
└── uninstall.php                # Cleanup on deletion
```

### Common Features
- ✅ WordPress Shortcode API integration
- ✅ Admin settings page (Settings → Plugin Name)
- ✅ AJAX data loading with caching
- ✅ Responsive design (mobile-first)
- ✅ Export functionality (CSV, PDF, Print)
- ✅ Consistent styling with CSS variables
- ✅ Loading states and error handling
- ✅ WordPress best practices (nonces, sanitization, escaping)
- ✅ Clean uninstallation

### Technology Stack

| Component | Technology | Version |
|-----------|-----------|---------|
| Visualization | Chart.js | 4.4.0 |
| Diagrams | Mermaid.js | 10.0.0 |
| JavaScript | jQuery + Vanilla JS | WP bundled |
| CSS | CSS3 with variables | - |
| PHP | WordPress Plugin API | 7.4+ |

---

## 🎨 Design System

### CSS Variables (Shared)
```css
--themisdb-primary: #2ea44f;
--themisdb-secondary: #3498db;
--themisdb-success: #27ae60;
--themisdb-warning: #f39c12;
--themisdb-danger: #e74c3c;
--themisdb-light-bg: #f6f8fa;
--themisdb-white: #ffffff;
--themisdb-text-primary: #24292f;
--themisdb-text-secondary: #57606a;
--themisdb-border: #d0d7de;
```

### Reusable CSS Classes
```css
.themisdb-<plugin>-wrapper       /* Main container */
.themisdb-section                /* Content sections */
.themisdb-btn-primary            /* Primary buttons */
.themisdb-btn-secondary          /* Secondary buttons */
.themisdb-select                 /* Dropdowns */
.themisdb-filter-group           /* Filter controls */
.themisdb-loading                /* Loading states */
.themisdb-spinner                /* Loading spinner */
.themisdb-footer                 /* Footer section */
```

### JavaScript Pattern
```javascript
window.ThemisDB<PluginName> = {
    init: function() { /* Initialize */ },
    loadData: function() { /* Load data via AJAX */ },
    render<Component>: function() { /* Render UI */ },
    export<Format>: function() { /* Export data */ }
};
```

---

## 📊 Phase 1 Implementation Summary

Based on **THEMISDB_WORDPRESS_PLUGINS_KONZEPT.md**, Phase 1 (Q1 2026) has been completed:

### ✅ Completed (Phase 1)

1. **Benchmark Visualizer** (40-60h estimated)
   - Interactive performance comparison
   - Chart.js integration
   - Multiple metrics and chart types
   - Export functionality
   - Fully documented

2. **Feature Matrix** (30-40h estimated)
   - Feature comparison table
   - Mermaid.js diagrams
   - Category filtering
   - Status indicators
   - Fully documented

### 📈 Key Benefits

- **Consistent Design**: All plugins follow the same design pattern
- **Reusable Code**: Common CSS classes and JavaScript patterns
- **Easy Maintenance**: Clear structure and documentation
- **Professional Quality**: Production-ready with error handling
- **WordPress Integration**: Follows WordPress coding standards

---

## 🚀 Installation Guide

### Prerequisites
- WordPress 5.0 or higher
- PHP 7.4 or higher
- Modern web browser with JavaScript enabled

### Step-by-Step Installation

1. **Clone ThemisDB Repository**
   ```bash
   git clone https://github.com/makr-code/ThemisDB.git
   cd ThemisDB
   ```

2. **Install Benchmark Visualizer**
   ```bash
   cd /path/to/wordpress/wp-content/plugins/
   cp -r /path/to/ThemisDB/tools/benchmark-visualizer-wordpress ./themisdb-benchmark-visualizer
   ```

3. **Install Feature Matrix**
   ```bash
   cd /path/to/wordpress/wp-content/plugins/
   cp -r /path/to/ThemisDB/tools/feature-matrix-wordpress ./themisdb-feature-matrix
   ```

4. **Activate Plugins**
   - Go to WordPress Admin → Plugins
   - Find "ThemisDB Benchmark Visualizer" and click "Activate"
   - Find "ThemisDB Feature Matrix" and click "Activate"

5. **Configure Settings**
   - Benchmark Visualizer: Settings → Benchmark Visualizer
   - Feature Matrix: Settings → Feature Matrix

6. **Use Shortcodes**
   - Add shortcodes to any page or post:
   ```php
   [themisdb_benchmark_visualizer]
   [themisdb_feature_matrix]
   ```

---

## 📖 Documentation Links

- **Benchmark Visualizer**: [README](benchmark-visualizer-wordpress/README.md)
- **Feature Matrix**: [README](feature-matrix-wordpress/README.md)
- **TCO Calculator**: [README](tco-calculator-wordpress/README.md)
- **WordPress Plugin Concept**: [THEMISDB_WORDPRESS_PLUGINS_KONZEPT.md](../docs/de/tools/THEMISDB_WORDPRESS_PLUGINS_KONZEPT.md)

---

## 🗺️ Future Roadmap (Phase 2 & 3)

As defined in the concept document:

### Phase 2: High-Value Features (Q2 2026)
- **Live Query Playground** (80-100h)
  - Interactive AQL editor
  - Live query execution
  - Demo database instance
- **Architecture Diagrams** (35-45h)
  - Interactive system diagrams
  - Multiple views (high-level, storage, LLM)
  - Mermaid.js integration

### Phase 3: Nice-to-Haves (Q3 2026)
- **Documentation Search** (50-70h)
  - Semantic search with vector embeddings
  - ThemisDB as search backend
- **Release Timeline** (25-35h)
  - Chronological release visualization
- **Test Coverage Dashboard** (20-30h)
  - CI/CD metrics display

---

## 🤝 Contributing

To add a new plugin following the established pattern:

1. Create plugin directory: `/tools/<plugin-name>-wordpress/`
2. Follow the standard structure (see Design Pattern above)
3. Use TCO Calculator as reference implementation
4. Reuse CSS variables and common classes
5. Implement WordPress best practices
6. Add comprehensive README
7. Include MIT LICENSE
8. Create uninstall.php for cleanup

---

## 📄 License

All plugins in this directory are licensed under the MIT License.

Copyright (c) 2026 ThemisDB Team

See individual plugin LICENSE files for details.

---

## 🔗 Resources

- **GitHub Repository**: [makr-code/ThemisDB](https://github.com/makr-code/ThemisDB)
- **WordPress Codex**: [Plugin Development](https://codex.wordpress.org/Plugin_API)
- **Chart.js Documentation**: [chartjs.org](https://www.chartjs.org/)
- **Mermaid.js Documentation**: [mermaid.js.org](https://mermaid.js.org/)

---

**Last Updated**: January 2026  
**Version**: 1.0.0  
**Status**: Phase 1 Complete ✅
