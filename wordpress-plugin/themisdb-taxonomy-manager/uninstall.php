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
$wpdb->query("DELETE FROM {$wpdb->termmeta} WHERE meta_key IN ('icon', 'color', 'extended_description', 'featured', 'term_order')");
