<?php
if (!defined('WP_UNINSTALL_PLUGIN')) {
    exit;
}

delete_option('themisdb_td_github_token');
delete_option('themisdb_td_repository');
delete_option('themisdb_td_cache_duration');
delete_option('themisdb_td_metrics_display');

delete_transient('themisdb_td_cached_metrics');
delete_transient('themisdb_td_cached_ci_status');

global $wpdb;
$wpdb->query("DELETE FROM {$wpdb->options} WHERE option_name LIKE 'themisdb_td_%'");
$wpdb->query("DELETE FROM {$wpdb->options} WHERE option_name LIKE '_transient_themisdb_td_%'");
