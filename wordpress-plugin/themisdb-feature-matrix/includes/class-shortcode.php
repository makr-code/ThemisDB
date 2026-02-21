/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-shortcode.php                                ║
  Version:         0.0.23                                             ║
  Last Modified:   2026-02-21 19:43:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     71                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

<?php
/**
 * Shortcode Handler Class
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Matrix_Shortcode {
    
    /**
     * Constructor
     */
    public function __construct() {
        add_shortcode('themisdb_feature_matrix', array($this, 'render_shortcode'));
    }
    
    /**
     * Render the shortcode
     * 
     * @param array $atts Shortcode attributes
     * @return string HTML output
     */
    public function render_shortcode($atts) {
        $atts = shortcode_atts(array(
            'category' => get_option('themisdb_matrix_default_category', 'all'),
            'style' => get_option('themisdb_matrix_default_style', 'modern'),
            'show_legend' => get_option('themisdb_matrix_show_legend', 1),
            'filterable' => get_option('themisdb_matrix_enable_filtering', 1),
            'sticky_header' => get_option('themisdb_matrix_sticky_header', 1),
            'highlight_themis' => get_option('themisdb_matrix_highlight_themis', 1),
        ), $atts, 'themisdb_feature_matrix');
        
        // Convert string 'yes'/'no' to boolean
        $atts['show_legend'] = ($atts['show_legend'] === 'yes' || $atts['show_legend'] == 1);
        $atts['filterable'] = ($atts['filterable'] === 'yes' || $atts['filterable'] == 1);
        $atts['sticky_header'] = ($atts['sticky_header'] === 'yes' || $atts['sticky_header'] == 1);
        $atts['highlight_themis'] = ($atts['highlight_themis'] === 'yes' || $atts['highlight_themis'] == 1);
        
        ob_start();
        include THEMISDB_MATRIX_DIR . 'templates/matrix.php';
        return ob_get_clean();
    }
}
