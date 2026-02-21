/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            uninstall.php                                      ║
  Version:         0.0.19                                             ║
  Last Modified:   2026-02-21 19:00:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     62                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

<?php
/**
 * Uninstall Script
 * Clean up plugin data when plugin is deleted
 */

if (!defined('WP_UNINSTALL_PLUGIN')) {
    exit;
}

// Optional: Remove all terms and taxonomies
// Uncomment if you want complete cleanup on uninstall

/*
$taxonomies = array('themisdb_feature', 'themisdb_usecase', 'themisdb_industry', 'themisdb_techspec');

foreach ($taxonomies as $taxonomy) {
    $terms = get_terms(array(
        'taxonomy' => $taxonomy,
        'hide_empty' => false
    ));
    
    foreach ($terms as $term) {
        wp_delete_term($term->term_id, $taxonomy);
    }
}
*/

// Remove plugin options
delete_option('themisdb_taxonomy_settings');

// Clean up term meta
global $wpdb;
$meta_keys = array('icon', 'color', 'extended_description', 'featured', 'term_order');
$placeholders = implode(', ', array_fill(0, count($meta_keys), '%s'));
$wpdb->query($wpdb->prepare("DELETE FROM {$wpdb->termmeta} WHERE meta_key IN ($placeholders)", ...$meta_keys));
