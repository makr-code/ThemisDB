<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-shortcodes.php                               ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:18                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     109                                            ║
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
 * Shortcodes Class
 * 
 * Handles shortcode registration and processing
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Formula_Shortcodes {
    
    /**
     * Constructor
     */
    public function __construct() {
        // Register shortcodes
        add_shortcode('themisdb_formula', array($this, 'render_formula_shortcode'));
        add_shortcode('formula', array($this, 'render_formula_shortcode'));
        add_shortcode('latex', array($this, 'render_formula_shortcode'));
        add_shortcode('math', array($this, 'render_formula_shortcode'));
    }
    
    /**
     * Render formula shortcode
     * 
     * @param array $atts Shortcode attributes
     * @param string $content Shortcode content
     * @return string Rendered HTML
     */
    public function render_formula_shortcode($atts, $content = null) {
        // Parse attributes
        $atts = shortcode_atts(array(
            'display' => 'block', // 'block' or 'inline'
            'class' => '',
        ), $atts, 'themisdb_formula');
        
        if (empty($content)) {
            return '';
        }
        
        // Sanitize content
        $content = trim($content);
        
        // Determine display mode
        $is_block = ($atts['display'] === 'block');
        $delimiter = $is_block ? '$$' : '$';
        
        // Add delimiters if not already present
        if (!$this->starts_with($content, '$')) {
            $content = $delimiter . $content . $delimiter;
        }
        
        // Build CSS classes
        $classes = array('themisdb-formula');
        if ($is_block) {
            $classes[] = 'themisdb-formula-block';
        } else {
            $classes[] = 'themisdb-formula-inline';
        }
        
        if (!empty($atts['class'])) {
            $classes[] = sanitize_html_class($atts['class']);
        }
        
        $class_attr = implode(' ', $classes);
        
        // Return wrapped content
        if ($is_block) {
            return '<div class="' . esc_attr($class_attr) . '">' . esc_html($content) . '</div>';
        } else {
            return '<span class="' . esc_attr($class_attr) . '">' . esc_html($content) . '</span>';
        }
    }
    
    /**
     * Check if string starts with substring
     * 
     * @param string $haystack The string to search in
     * @param string $needle The substring to search for
     * @return bool
     */
    private function starts_with($haystack, $needle) {
        return strpos($haystack, $needle) === 0;
    }
}
