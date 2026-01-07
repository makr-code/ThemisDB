<?php
/**
 * Uninstall script for ThemisDB Test Dashboard Plugin
 * 
 * This file is executed when the plugin is uninstalled via WordPress admin.
 * It removes all plugin options, transients, and cached data from the database.
 */

if (!defined('WP_UNINSTALL_PLUGIN')) {
    exit;
}

// Delete plugin options
delete_option('themisdb_td_github_token');
delete_option('themisdb_td_repository');
delete_option('themisdb_td_cache_duration');
delete_option('themisdb_td_metrics_display');

// Delete transients
delete_transient('themisdb_td_cached_metrics');
delete_transient('themisdb_td_cached_ci_status');

// Clean up any remaining options and transients
global $wpdb;
$wpdb->query($wpdb->prepare(
    "DELETE FROM {$wpdb->options} WHERE option_name LIKE %s",
    $wpdb->esc_like('themisdb_td_') . '%'
));
$wpdb->query($wpdb->prepare(
    "DELETE FROM {$wpdb->options} WHERE option_name LIKE %s",
    $wpdb->esc_like('_transient_themisdb_td_') . '%'
));
$wpdb->query($wpdb->prepare(
    "DELETE FROM {$wpdb->options} WHERE option_name LIKE %s",
    $wpdb->esc_like('_transient_timeout_themisdb_td_') . '%'
));
