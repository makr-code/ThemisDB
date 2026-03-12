# ThemisDB Taxonomy Manager v1.0.0

Manage custom taxonomies with visual tree view, drag & drop, and widgets.

## Features

✅ 4 Custom Taxonomies (Features, Use-Cases, Industries, Tech-Specs)
✅ Visual Tree-View Admin with Drag & Drop
✅ Custom Icons & Colors per Term
✅ Enhanced Meta-Box for Posts
✅ Taxonomy Widget (List, Cloud, Grid)
✅ Shortcodes for frontend display
✅ SEO Schema.org integration
✅ REST API endpoints
✅ Import/Export as JSON
✅ Intelligent taxonomy extraction from content
✅ Hierarchical category management

## Installation

### Method 1: WordPress Admin

1. Upload plugin folder to `/wp-content/plugins/themisdb-taxonomy-manager/`
2. Activate in WordPress Admin → Plugins
3. Go to ThemisDB → Taxonomy Tree to manage terms

### Method 2: Manual

```bash
cd /wp-content/plugins/
cp -r /path/to/themisdb-taxonomy-manager ./
```

## Custom Taxonomies

### 1. Database Features (themisdb_feature)
Hierarchical taxonomy with icon and color meta:

- **Data Models**: Relational SQL, Graph Database, Document Store, Vector Database, Time-Series, Key-Value Store
- **AI/ML**: Embedded LLM, Vector Search, RAG Support, GPU Acceleration, Model Inference
- **Performance**: Horizontal Scaling, Auto-Sharding, Replication, Caching, Query Optimization
- **Compatibility**: SQL Protocol, MongoDB Protocol, Cypher (Graph), REST API, GraphQL API, gRPC

### 2. Use Cases (themisdb_usecase)
Hierarchical taxonomy for common use cases:
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

### 3. Industries (themisdb_industry)
Hierarchical taxonomy for industry verticals:
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

### 4. Technical Specs (themisdb_techspec)
Non-hierarchical (tags style) for technical specifications:
- ACID, MVCC, C++, RocksDB, llama.cpp
- CUDA, OpenCL, Docker, Kubernetes
- High Availability, Disaster Recovery

## Tree View Admin

Navigate to **ThemisDB → Taxonomy Tree** to access the visual tree interface:

- Expandable/collapsible hierarchical view
- Drag & drop to reorder terms
- Search and filter terms
- Inline editing of term names
- Export taxonomies as JSON

## Widgets

Add taxonomy widgets via **Appearance → Widgets → "ThemisDB Taxonomy"**

### Widget Options:
- **Title**: Widget title
- **Taxonomy**: Choose from Features, Use Cases, Industries, or Tech Specs
- **Display Style**: List, Cloud, or Grid
- **Show Count**: Display post count
- **Parent Only**: Show only parent terms

## Shortcodes

### Taxonomy List
```php
[themisdb_taxonomy taxonomy="themisdb_feature" style="list" show_count="yes"]
```

### Taxonomy Cloud
```php
[themisdb_taxonomy taxonomy="themisdb_usecase" style="cloud" min_size="0.8" max_size="2"]
```

### Taxonomy Grid
```php
[themisdb_taxonomy taxonomy="themisdb_industry" style="grid" show_count="yes"]
```

### Single Taxonomy Info
```php
[themisdb_taxonomy_info term_id="123"]
```
Displays icon, name, description, and post count for a specific term.

## REST API

### Get All Taxonomies
```
GET /wp-json/themisdb/v1/taxonomies
```

### Get Terms in Taxonomy
```
GET /wp-json/themisdb/v1/taxonomy/themisdb_feature
```

### Get Hierarchical Tree
```
GET /wp-json/themisdb/v1/taxonomy/themisdb_feature/tree
```

## SEO & Schema.org

Automatically adds Schema.org CollectionPage markup to taxonomy archive pages. Enable/disable in **Settings → Taxonomy Manager**.

## Breadcrumbs

Breadcrumbs are automatically added to taxonomy archive pages showing the hierarchical path.

## Configuration

Go to **Settings → Taxonomy Manager** to configure:

| Setting | Description | Default |
|---------|-------------|---------|
| Enable Auto-Extraction | Auto-extract taxonomies on post save | ✅ On |
| Auto-Assign Categories | Auto-assign categories | ✅ On |
| Auto-Assign Tags | Auto-assign tags | ✅ On |
| Maximum Category Depth | Max hierarchy depth (1-5) | 3 |
| Consolidate Categories | Auto-consolidate similar categories | ✅ On |
| Enable Custom Meta Box | Use enhanced meta box | ✅ On |
| Default Icon | Default emoji for new terms | 📊 |
| Default Color | Default color for new terms | #3498db |
| Show in REST API | Make available via REST | ✅ On |
| Enable SEO Schema | Add Schema.org markup | ✅ On |
| Breadcrumb Separator | Separator for breadcrumbs | " / " |

## Developer Usage

### Programmatic Extraction

```php
// Get taxonomy manager
$manager = themisdb_get_taxonomy_manager();

// Extract from post
$post_id = 123;
$result = $manager->get_extractor()->extract_taxonomies($post_id);

// Assign with hierarchy
$manager->assign_categories_with_hierarchy($post_id, $result['categories']);
$manager->assign_tags($post_id, $result['tags']);
```

### Batch Processing

```php
$manager = themisdb_get_taxonomy_manager();

$post_ids = array(1, 2, 3, 4, 5);
$options = array(
    'extract_from_content' => true,
    'extract_from_metadata' => true,
    'max_categories' => 5,
    'max_tags' => 15
);

$stats = $manager->batch_assign_taxonomies($post_ids, $options);
```

### Custom Term Meta

```php
// Get term icon and color
$icon = get_term_meta($term_id, 'icon', true);
$color = get_term_meta($term_id, 'color', true);

// Set term icon and color
update_term_meta($term_id, 'icon', '🚀');
update_term_meta($term_id, 'color', '#e74c3c');
```

## Import/Export

### Export
Click "Export JSON" in the Taxonomy Tree admin to download all taxonomies as a JSON file.

### Import
Upload a JSON file in the Taxonomy Tree admin to import taxonomies.

## Troubleshooting

### Problem: Taxonomies not showing
**Solution**: Make sure plugin is activated and flush permalinks (Settings → Permalinks → Save).

### Problem: Tree view not loading
**Solution**: Check browser console for JavaScript errors. Ensure jQuery UI is loaded.

### Problem: Drag & drop not working
**Solution**: Ensure jQuery UI Sortable is loaded. Check for JavaScript conflicts.

## License

MIT - Part of the ThemisDB project

## Support

- GitHub Issues: https://github.com/makr-code/wordpressPlugins/issues
- Documentation: https://github.com/makr-code/wordpressPlugins/wiki

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
- GitHub: https://github.com/makr-code/wordpressPlugins
- Website: https://themisdb.org

## 🆘 Support

- GitHub Issues: https://github.com/makr-code/wordpressPlugins/issues
- Documentation: https://github.com/makr-code/wordpressPlugins/wiki

## 📝 Changelog

See [CHANGELOG.md](CHANGELOG.md) for version history.
