<?php
/**
 * Uninstall script for ThemisDB Benchmark Visualizer
 * 
 * This file is executed when the plugin is deleted via WordPress admin
 */

// Exit if not called by WordPress
if (!defined('WP_UNINSTALL_PLUGIN')) {
    exit;
}

// Delete plugin options
delete_option('themisdb_bv_data_source');
delete_option('themisdb_bv_github_data_url');
delete_option('themisdb_bv_default_comparison_dbs');
delete_option('themisdb_bv_chart_theme');
delete_option('themisdb_bv_auto_update_interval');
delete_option('themisdb_bv_default_metric');
delete_option('themisdb_bv_default_category');

// Delete transients
delete_transient('themisdb_bv_cached_data');

// Delete any cached data with dynamic keys
global $wpdb;
$wpdb->query("DELETE FROM {$wpdb->options} WHERE option_name LIKE 'themisdb_bv_data_%'");
$wpdb->query("DELETE FROM {$wpdb->options} WHERE option_name LIKE '_transient_themisdb_bv_%'");
$wpdb->query("DELETE FROM {$wpdb->options} WHERE option_name LIKE '_transient_timeout_themisdb_bv_%'");

// Clean up is complete
