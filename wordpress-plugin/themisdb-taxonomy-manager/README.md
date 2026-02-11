# ThemisDB Taxonomy Manager v1.0.0

A comprehensive WordPress plugin for managing custom taxonomies for ThemisDB features, use cases, industries, and technical specifications.

## 📋 Overview

This plugin provides advanced taxonomy management for ThemisDB-related content with visual tree interface, icon/color support, SEO optimization, and flexible display widgets.

## 🎯 Features

### ✅ Custom Taxonomies
- **Database Features** (`themisdb_feature`) - Hierarchical
- **Use Cases** (`themisdb_usecase`) - Hierarchical  
- **Industries** (`themisdb_industry`) - Hierarchical
- **Technical Specs** (`themisdb_techspec`) - Non-hierarchical (tags)

### ✅ Visual Tree View
- Interactive tree interface (Tools → Taxonomy Tree)
- Drag & drop reordering with AJAX save
- Expand/collapse branches
- Post count display
- Quick edit links

### ✅ Icon & Color Support
- Emoji icons (📦, 🗄️, 🎯, etc.)
- Font Awesome support
- Color picker with Themis brand presets
- Extended descriptions
- Featured flag
- Custom ordering

### ✅ Display Options
- **Widget**: 3 styles (list, cloud, grid)
- **Shortcodes**: `[themisdb_taxonomy]` and `[themisdb_term_card]`
- **Template Functions**: For theme integration

### ✅ SEO Optimization
- Schema.org CollectionPage markup
- Breadcrumb schema (BreadcrumbList)
- Hierarchical breadcrumb display
- Meta descriptions

## 📦 Installation

### Method 1: WordPress Admin
1. Download the plugin as ZIP
2. Go to Plugins → Add New → Upload Plugin
3. Activate the plugin
4. Default terms will be created automatically

### Method 2: Manual
```bash
cd /path/to/wordpress/wp-content/plugins/
cp -r /path/to/themisdb-taxonomy-manager ./
```

Then activate via WordPress admin.

## 🎨 Custom Taxonomies

### Database Features (`themisdb_feature`)

Hierarchical taxonomy for database features:

```
Data Models
├── Relational SQL
├── Graph Database
├── Document Store
├── Vector Database
├── Time-Series
└── Key-Value Store

AI/ML
├── Embedded LLM
├── Vector Search
├── RAG Support
├── GPU Acceleration
└── Model Inference

Performance
├── Horizontal Scaling
├── Auto-Sharding
├── Replication
├── Caching
└── Query Optimization

Compatibility
├── SQL Protocol
├── MongoDB Protocol
├── Cypher (Graph)
├── REST API
├── GraphQL API
└── gRPC
```

### Use Cases (`themisdb_usecase`)

- AI & Machine Learning
- Real-Time Analytics
- Graph Analytics
- IoT Data Management
- Content Management
- E-Commerce
- Social Networks
- Recommendation Systems
- Knowledge Graphs
- Semantic Search

### Industries (`themisdb_industry`)

- Healthcare
- Finance
- E-Commerce
- Telecommunications
- Manufacturing
- Education
- Government
- Media & Entertainment
- Transportation
- Energy

### Technical Specs (`themisdb_techspec`)

Non-hierarchical tags:
- ACID, MVCC, C++, RocksDB, llama.cpp
- CUDA, OpenCL, Docker, Kubernetes
- High Availability, Disaster Recovery

## 🧩 Usage

### Tree View Admin

Navigate to **Tools → Taxonomy Tree** to:
- View hierarchical term structure
- Drag & drop to reorder terms
- Expand/collapse branches
- Quick edit terms
- See post counts

### Widget

Add the **ThemisDB Taxonomy** widget to your sidebar:

**Settings:**
- Title
- Taxonomy selection
- Display style (list/cloud/grid)
- Show icons (yes/no)
- Show count (yes/no)
- Limit (number of terms)

### Shortcodes

#### Taxonomy List

```php
[themisdb_taxonomy taxonomy="themisdb_feature" style="list" show_icons="yes" show_count="yes"]

// Parameters:
// - taxonomy: themisdb_feature|themisdb_usecase|themisdb_industry|themisdb_techspec
// - style: list|cloud|grid
// - show_icons: yes|no
// - show_count: yes|no
// - parent: term_id (show only children)
// - limit: number (default: -1 for all)
// - orderby: name|count|term_order
// - order: ASC|DESC
```

#### Term Card

```php
[themisdb_term_card term_id="123" show_description="yes" show_posts="yes"]
```

### Template Functions

```php
// Display breadcrumb
<?php themisdb_taxonomy_breadcrumb(); ?>

// Get terms with icons
<?php
$terms = get_terms(array('taxonomy' => 'themisdb_feature'));
foreach ($terms as $term) {
    $icon = get_term_meta($term->term_id, 'icon', true);
    $color = get_term_meta($term->term_id, 'color', true);
    echo '<span style="color: ' . esc_attr($color) . ';">' . esc_html($icon) . '</span> ';
    echo esc_html($term->name);
}
?>
```

## 🎨 Themis Brand Colors

```css
--themis-primary: #2c3e50
--themis-secondary: #3498db
--themis-accent: #7c4dff
--themis-success: #27ae60
--themis-warning: #f39c12
--themis-error: #e74c3c
```

## 🔍 SEO Integration

### Schema.org Markup

Automatically adds CollectionPage and BreadcrumbList schema to taxonomy archive pages.

### Breadcrumbs

Display hierarchical breadcrumbs:

```php
<?php if (is_tax(array('themisdb_feature', 'themisdb_usecase', 'themisdb_industry', 'themisdb_techspec'))): ?>
    <?php themisdb_taxonomy_breadcrumb(); ?>
<?php endif; ?>
```

## 📝 Term Meta Fields

Each term supports:
- **Icon**: Emoji or Font Awesome class
- **Color**: Hex color with picker
- **Extended Description**: Long-form content
- **Featured**: Flag for highlighting
- **Order**: Manual sort position

## ⚙️ Development

### Hooks & Filters

```php
// Add custom term meta
add_action('themisdb_feature_edit_form_fields', 'my_custom_fields', 20, 2);

// Modify breadcrumb output
add_filter('themisdb_taxonomy_breadcrumb_html', 'my_breadcrumb_filter');
```

## 🧪 Testing

The plugin has been tested with:
- WordPress 5.8+
- PHP 7.4+
- Modern browsers (Chrome, Firefox, Safari, Edge)
- Mobile responsive design

## 📄 License

MIT License - See LICENSE file for details

## 👥 Author

**ThemisDB Team**
- GitHub: https://github.com/makr-code/ThemisDB
- Website: https://themisdb.org

## 🆘 Support

- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Documentation: https://github.com/makr-code/ThemisDB/wiki

## 📝 Changelog

See [CHANGELOG.md](CHANGELOG.md) for version history.
