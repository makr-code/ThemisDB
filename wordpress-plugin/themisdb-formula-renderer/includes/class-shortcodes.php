/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-shortcodes.php                               ║
  Version:         0.0.20                                             ║
  Last Modified:   2026-02-21 19:15:02                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     113                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

<?php
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
