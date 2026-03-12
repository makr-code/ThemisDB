<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-template-handler.php                         ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:21                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     101                                            ║
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
 * Template Handler
 * Handles custom templates for taxonomy archives
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Template_Handler {
    
    /**
     * Constructor
     */
    public function __construct() {
        // Add custom template for taxonomy archives
        add_filter('template_include', array($this, 'taxonomy_template'), 99);
        
        // Add archive header action
        add_action('themisdb_taxonomy_archive_header', array($this, 'render_archive_header'));
        
        // Enqueue frontend styles
        add_action('wp_enqueue_scripts', array($this, 'enqueue_frontend_styles'));
    }
    
    /**
     * Load custom taxonomy template
     */
    public function taxonomy_template($template) {
        if (is_tax(array('themisdb_feature', 'themisdb_usecase', 'themisdb_industry', 'themisdb_techspec'))) {
            // Check if theme has custom template
            $theme_template = locate_template(array(
                'taxonomy-' . get_query_var('taxonomy') . '.php',
                'taxonomy.php',
                'archive.php'
            ));
            
            // If theme doesn't have custom template, use plugin template
            if (!$theme_template) {
                $plugin_template = THEMISDB_TAXONOMY_PLUGIN_DIR . 'templates/taxonomy-archive.php';
                if (file_exists($plugin_template)) {
                    return $plugin_template;
                }
            }
            
            return $theme_template ?: $template;
        }
        
        return $template;
    }
    
    /**
     * Render archive header
     */
    public function render_archive_header() {
        $template = THEMISDB_TAXONOMY_PLUGIN_DIR . 'templates/taxonomy-archive.php';
        if (file_exists($template)) {
            include $template;
        }
    }
    
    /**
     * Enqueue frontend styles
     */
    public function enqueue_frontend_styles() {
        if (is_tax(array('themisdb_feature', 'themisdb_usecase', 'themisdb_industry', 'themisdb_techspec'))) {
            wp_enqueue_style(
                'themisdb-taxonomy-widget',
                THEMISDB_TAXONOMY_PLUGIN_URL . 'assets/css/taxonomy-widget.css',
                array(),
                THEMISDB_TAXONOMY_VERSION
            );
        }
    }
}

// Initialize
new ThemisDB_Template_Handler();
