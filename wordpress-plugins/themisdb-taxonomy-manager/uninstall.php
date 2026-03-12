<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            uninstall.php                                      ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:21                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     58                                             ║
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
