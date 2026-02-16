<?php
/**
 * Uninstall script for ThemisDB Feature Matrix
 */

if (!defined('WP_UNINSTALL_PLUGIN')) {
    exit;
}

// Delete plugin options
delete_option('themisdb_fm_data_source');
delete_option('themisdb_fm_default_comparison_dbs');
delete_option('themisdb_fm_default_category');
delete_option('themisdb_fm_show_mermaid_diagrams');
delete_option('themisdb_fm_table_view');
delete_option('themisdb_fm_enable_tooltips');

// Delete transients
delete_transient('themisdb_fm_cached_features');

// Delete cached data
global $wpdb;
$wpdb->query("DELETE FROM {$wpdb->options} WHERE option_name LIKE 'themisdb_fm_%'");
$wpdb->query("DELETE FROM {$wpdb->options} WHERE option_name LIKE '_transient_themisdb_fm_%'");
$wpdb->query("DELETE FROM {$wpdb->options} WHERE option_name LIKE '_transient_timeout_themisdb_fm_%'");
