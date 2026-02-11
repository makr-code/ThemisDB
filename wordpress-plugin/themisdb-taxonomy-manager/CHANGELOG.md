# Changelog

All notable changes to the ThemisDB Taxonomy Manager plugin will be documented in this file.

## [1.0.0] - 2024-02-11

### Added
- **Custom Taxonomies**: 4 custom taxonomies (Database Features, Use Cases, Industries, Technical Specs)
- **Hierarchical Support**: Full hierarchical category support with parent-child relationships
- **Default Terms**: Comprehensive set of default terms for all taxonomies
- **Visual Tree View**: Interactive tree-view admin interface in Tools → Taxonomy Tree
- **Drag & Drop**: Reorder terms using drag and drop with AJAX auto-save
- **Icon Support**: Add emoji or Font Awesome icons to terms
- **Color Support**: Assign custom colors to terms with color picker
- **Extended Meta**: Featured flag, custom ordering, and extended descriptions for terms
- **Taxonomy Widget**: Configurable widget with 3 display styles (list, cloud, grid)
- **Shortcodes**: `[themisdb_taxonomy]` and `[themisdb_term_card]` shortcodes
- **SEO Integration**: Schema.org CollectionPage and BreadcrumbList markup
- **Breadcrumb Function**: `themisdb_taxonomy_breadcrumb()` helper function
- **Themis Brand Colors**: Complete color palette integration
- **Responsive Design**: Mobile-friendly styles for all components

### Features
- Multiple taxonomy display styles (list, tag cloud, grid)
- Icon and color metadata for visual organization
- Tree-view admin with expand/collapse functionality
- AJAX-powered term reordering
- WordPress color picker integration
- Schema.org structured data for SEO
- Hierarchical breadcrumb navigation
- Widget with extensive customization options

### Technical
- WordPress 5.8+ compatibility
- PHP 7.4+ required
- REST API ready
- Internationalization ready (i18n)
- Clean uninstall process
