# ThemisDB Taxonomy Manager - Architecture

## Plugin Structure

```
themisdb-taxonomy-manager/
├── themisdb-taxonomy-manager.php    # Main plugin file
├── README.md                         # User documentation
├── CHANGELOG.md                      # Version history
├── LICENSE                           # MIT license
├── TESTING.md                        # Testing guide
│
├── includes/                         # PHP classes
│   ├── class-taxonomy-manager.php   # Main manager (existing)
│   ├── class-category-hierarchy.php # Hierarchy manager (existing)
│   ├── class-taxonomy-extractor.php # Content extraction (existing)
│   ├── class-admin.php              # Admin settings (existing)
│   ├── class-custom-taxonomies.php  # Register 4 taxonomies ✅
│   ├── class-tree-view.php          # Tree admin UI ✅
│   ├── class-widget.php             # Frontend widget ✅
│   ├── class-metabox.php            # Enhanced meta boxes ✅
│   └── class-template-handler.php   # Archive templates ✅
│
├── assets/
│   ├── css/
│   │   ├── admin.css                # Existing admin styles
│   │   ├── taxonomy-admin.css       # Tree view styles ✅
│   │   └── taxonomy-widget.css      # Widget styles ✅
│   └── js/
│       ├── admin.js                 # Existing admin JS
│       └── tree-view.js             # Drag & drop JS ✅
│
└── templates/
    └── taxonomy-archive.php         # Archive template ✅
```

## Component Relationships

```
┌─────────────────────────────────────────────────────────┐
│           Main Plugin Class                             │
│  (ThemisDB_Taxonomy_Manager_Plugin)                     │
└──────────────────┬──────────────────────────────────────┘
                   │
                   ├── Initializes ──────┐
                   │                      │
        ┌──────────┴─────────┐    ┌──────┴────────────┐
        │  Custom Taxonomies  │    │  Taxonomy Manager │
        │  (Registration)     │    │  (Content Extract)│
        └─────────────────────┘    └───────────────────┘
                   │
                   ├── Admin Components
                   │
        ┌──────────┴─────────┬──────────┬────────────┐
        │                    │          │            │
   ┌────┴────┐      ┌────────┴───┐  ┌──┴──────┐  ┌─┴─────────┐
   │Tree View│      │Admin Settings│ │Meta Box │  │Template   │
   │  Admin  │      │   Panel      │ │Enhanced │  │ Handler   │
   └─────────┘      └──────────────┘ └─────────┘  └───────────┘
        │
        ├── Frontend Components
        │
   ┌────┴──────┬──────────┬────────────┐
   │           │          │            │
┌──┴─────┐ ┌──┴────┐  ┌──┴──────┐  ┌─┴──────┐
│Widget  │ │Short  │  │Archive  │  │REST    │
│Display │ │codes  │  │Template │  │API     │
└────────┘ └───────┘  └─────────┘  └────────┘
```

## Data Flow

### 1. Taxonomy Registration Flow
```
Plugin Activation
    ↓
class-custom-taxonomies.php
    ↓
register_taxonomy() for each taxonomy
    ↓
create_default_terms()
    ↓
Add term meta (icon, color)
```

### 2. Tree View Admin Flow
```
User visits ThemisDB → Taxonomy Tree
    ↓
class-tree-view.php::render_tree_page()
    ↓
Get terms with hierarchy
    ↓
Render HTML tree with icons/colors
    ↓
tree-view.js initializes:
    - Sortable (drag & drop)
    - Expand/collapse
    - Search/filter
    ↓
On drag: AJAX save term order
```

### 3. Enhanced Meta Box Flow
```
User edits post
    ↓
class-metabox.php::add_meta_boxes()
    ↓
Remove default meta boxes
    ↓
Add custom meta boxes:
    - Features (grouped with icons)
    - Use Cases (simple list)
    - Industries (simple list)
    - Tech Specs (tag input)
    ↓
User selects terms
    ↓
save_meta_box() on post save
    ↓
wp_set_post_terms()
```

### 4. Widget Display Flow
```
User adds widget to sidebar
    ↓
class-widget.php::widget()
    ↓
Get terms based on config
    ↓
Render based on style:
    - List: render_list()
    - Cloud: render_cloud()
    - Grid: render_grid()
    ↓
Output HTML with taxonomy-widget.css
```

### 5. Shortcode Flow
```
User adds [themisdb_taxonomy] to content
    ↓
taxonomy_shortcode() in main plugin
    ↓
Get terms based on attributes
    ↓
Render based on style:
    - List
    - Cloud  
    - Grid
    ↓
Return HTML output
```

### 6. Archive Template Flow
```
User visits taxonomy archive
    ↓
class-template-handler.php::taxonomy_template()
    ↓
Check for theme template
    ↓
If not found, use plugin template
    ↓
templates/taxonomy-archive.php
    ↓
Render:
    - Breadcrumbs
    - Header with gradient
    - Icon, name, description
    - Post list
```

### 7. REST API Flow
```
GET /wp-json/themisdb/v1/taxonomies
    ↓
rest_get_taxonomies()
    ↓
Return array of taxonomies

GET /wp-json/themisdb/v1/taxonomy/{taxonomy}
    ↓
rest_get_taxonomy_terms()
    ↓
Return terms with metadata

GET /wp-json/themisdb/v1/taxonomy/{taxonomy}/tree
    ↓
rest_get_taxonomy_tree()
    ↓
build_term_tree() recursively
    ↓
Return hierarchical structure
```

## Database Schema

### Terms Table (wp_terms)
```sql
- term_id (PK)
- name
- slug
- term_group
```

### Term Taxonomy (wp_term_taxonomy)
```sql
- term_taxonomy_id (PK)
- term_id (FK)
- taxonomy (themisdb_feature, themisdb_usecase, etc.)
- description
- parent
- count
```

### Term Meta (wp_termmeta)
```sql
- meta_id (PK)
- term_id (FK)
- meta_key (icon, color, term_order)
- meta_value
```

### Term Relationships (wp_term_relationships)
```sql
- object_id (post_id)
- term_taxonomy_id
- term_order
```

## Custom Taxonomies

### 1. themisdb_feature
- **Type**: Hierarchical
- **Meta**: icon (emoji), color (hex)
- **Parents**: Data Models, AI/ML, Performance, Compatibility
- **Slug**: /features/

### 2. themisdb_usecase
- **Type**: Hierarchical
- **Meta**: None
- **Slug**: /use-cases/

### 3. themisdb_industry
- **Type**: Hierarchical
- **Meta**: None
- **Slug**: /industries/

### 4. themisdb_techspec
- **Type**: Non-hierarchical (tags)
- **Meta**: None
- **Slug**: /tech-specs/

## Hooks & Filters

### Actions
```php
// Plugin initialization
'plugins_loaded' → load_textdomain()
'init' → register_shortcodes()
'init' → register_taxonomies()
'rest_api_init' → register_rest_api()

// Admin
'admin_menu' → add_menu_page()
'admin_init' → register_settings()
'admin_enqueue_scripts' → enqueue_scripts()
'add_meta_boxes' → add_meta_boxes()
'save_post' → save_meta_box()

// Frontend
'wp_head' → add_schema_markup()
'wp_enqueue_scripts' → enqueue_frontend_styles()
'themisdb_before_taxonomy_archive' → render_breadcrumbs()

// Widgets
'widgets_init' → register_widget()

// AJAX
'wp_ajax_themisdb_save_term_order'
'wp_ajax_themisdb_export_taxonomies'
'wp_ajax_themisdb_consolidate_categories'
'wp_ajax_themisdb_get_recommendations'
```

### Filters
```php
'template_include' → taxonomy_template()
'widget_title' → (standard WordPress)
```

## API Endpoints

### REST Routes
```
GET  /wp-json/themisdb/v1/taxonomies
GET  /wp-json/themisdb/v1/taxonomy/{taxonomy}
GET  /wp-json/themisdb/v1/taxonomy/{taxonomy}/tree
POST /wp-json/themisdb/v1/taxonomy/extract
POST /wp-json/themisdb/v1/taxonomy/consolidate
GET  /wp-json/themisdb/v1/taxonomy/recommendations
```

## Settings & Options

### WordPress Options
```php
themisdb_taxonomy_auto_extract          (1/0)
themisdb_taxonomy_auto_tags             (1/0)
themisdb_taxonomy_auto_categories       (1/0)
themisdb_taxonomy_max_category_depth    (1-5)
themisdb_taxonomy_consolidate_categories (1/0)
themisdb_taxonomy_show_in_rest          (1/0)
themisdb_taxonomy_enable_seo_schema     (1/0)
themisdb_taxonomy_default_icon          (emoji)
themisdb_taxonomy_default_color         (hex)
themisdb_taxonomy_breadcrumb_separator  (string)
themisdb_taxonomy_enable_custom_metabox (1/0)
themisdb_default_terms_created          (1/0)
```

## Security Measures

1. **Nonce Verification**: All AJAX requests verify nonces
2. **Capability Checks**: Admin actions check user capabilities
3. **Input Sanitization**: All inputs sanitized before use
4. **Output Escaping**: All outputs escaped before display
5. **SQL Safety**: Uses WordPress API, no direct SQL queries
6. **XSS Prevention**: All user input escaped
7. **CSRF Protection**: Nonces on all forms

## Performance Optimizations

1. **Lazy Loading**: Assets only loaded when needed
2. **Caching**: WordPress term cache utilized
3. **Conditional Loading**: Admin scripts only in admin
4. **Transients**: Could be added for API responses
5. **Query Optimization**: Uses efficient WordPress queries

## Future Enhancements

- [ ] Import from JSON UI
- [ ] Bulk term editing
- [ ] Term merging interface
- [ ] Analytics dashboard
- [ ] Export/import presets
- [ ] Term relationships graph
- [ ] Advanced search filters
- [ ] Term usage statistics
