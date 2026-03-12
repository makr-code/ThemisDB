<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-seo.php                                      ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:21                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     164                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 9d3ecaa0e  2026-02-28  Add ThemisDB Wiki Integration plugin with documentation i... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */


/**
 * SEO Integration
 * Adds Schema.org markup and breadcrumbs for taxonomies
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Taxonomy_SEO {
    
    /**
     * Constructor
     */
    public function __construct() {
        add_action('wp_head', array($this, 'add_schema_markup'));
        add_action('themisdb_taxonomy_breadcrumb', array($this, 'render_breadcrumb'));
    }
    
    /**
     * Add Schema.org markup to taxonomy pages
     */
    public function add_schema_markup() {
        if (!is_tax(array('themisdb_feature', 'themisdb_usecase', 'themisdb_industry', 'themisdb_techspec'))) {
            return;
        }
        
        $term = get_queried_object();
        
        if (!$term || is_wp_error($term)) {
            return;
        }
        
        $schema = array(
            '@context' => 'https://schema.org',
            '@type' => 'CollectionPage',
            'name' => $term->name,
            'description' => $term->description,
            'url' => get_term_link($term)
        );
        
        // Add breadcrumb schema
        $breadcrumb_schema = $this->get_breadcrumb_schema($term);
        if ($breadcrumb_schema) {
            $schema['breadcrumb'] = $breadcrumb_schema;
        }
        
        echo '<script type="application/ld+json">' . wp_json_encode($schema, JSON_UNESCAPED_SLASHES | JSON_PRETTY_PRINT) . '</script>' . "\n";
    }
    
    /**
     * Get breadcrumb Schema.org markup
     */
    private function get_breadcrumb_schema($term) {
        $breadcrumbs = array();
        $current = $term;
        $position = 1;
        
        // Build breadcrumb trail
        $trail = array();
        while ($current) {
            array_unshift($trail, $current);
            $current = ($current->parent) ? get_term($current->parent, $term->taxonomy) : null;
        }
        
        // Add home
        $breadcrumbs[] = array(
            '@type' => 'ListItem',
            'position' => $position++,
            'name' => 'Home',
            'item' => home_url('/')
        );
        
        // Add taxonomy archive
        $tax_obj = get_taxonomy($term->taxonomy);
        if ($tax_obj) {
            $breadcrumbs[] = array(
                '@type' => 'ListItem',
                'position' => $position++,
                'name' => $tax_obj->labels->name,
                'item' => home_url('/' . $tax_obj->rewrite['slug'])
            );
        }
        
        // Add term trail
        foreach ($trail as $trail_term) {
            $breadcrumbs[] = array(
                '@type' => 'ListItem',
                'position' => $position++,
                'name' => $trail_term->name,
                'item' => get_term_link($trail_term)
            );
        }
        
        return array(
            '@type' => 'BreadcrumbList',
            'itemListElement' => $breadcrumbs
        );
    }
    
    /**
     * Render breadcrumb navigation
     */
    public function render_breadcrumb($term = null) {
        if (!$term) {
            $term = get_queried_object();
        }
        
        if (!$term || is_wp_error($term)) {
            return;
        }
        
        $breadcrumbs = array();
        $current = $term;
        
        // Build trail
        while ($current) {
            array_unshift($breadcrumbs, sprintf(
                '<a href="%s">%s</a>',
                esc_url(get_term_link($current)),
                esc_html($current->name)
            ));
            
            $current = ($current->parent) ? get_term($current->parent, $term->taxonomy) : null;
        }
        
        if (!empty($breadcrumbs)) {
            echo '<nav class="themisdb-breadcrumb">';
            echo '<a href="' . esc_url(home_url('/')) . '">Home</a> › ';
            echo implode(' › ', $breadcrumbs);
            echo '</nav>';
        }
    }
}

/**
 * Helper function to display breadcrumbs
 */
function themisdb_taxonomy_breadcrumb($term = null) {
    do_action('themisdb_taxonomy_breadcrumb', $term);
}
