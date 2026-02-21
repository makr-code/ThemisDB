/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-shortcode.php                                ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-02-21 14:18:17                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     71                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
