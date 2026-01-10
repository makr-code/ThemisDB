<?php
/**
 * Uninstall script for ThemisDB Architecture Diagrams
 */

if (!defined('WP_UNINSTALL_PLUGIN')) {
    exit;
}

// Delete plugin options
delete_option('themisdb_ad_default_view');
delete_option('themisdb_ad_theme');
delete_option('themisdb_ad_interactive');
delete_option('themisdb_ad_enable_export');
delete_option('themisdb_ad_show_descriptions');
delete_option('themisdb_ad_default_zoom');

// Delete transients
delete_transient('themisdb_ad_cached_diagrams');

// Delete cached data
global $wpdb;
$wpdb->query("DELETE FROM {$wpdb->options} WHERE option_name LIKE 'themisdb_ad_%'");
$wpdb->query("DELETE FROM {$wpdb->options} WHERE option_name LIKE '_transient_themisdb_ad_%'");
$wpdb->query("DELETE FROM {$wpdb->options} WHERE option_name LIKE '_transient_timeout_themisdb_ad_%'");
