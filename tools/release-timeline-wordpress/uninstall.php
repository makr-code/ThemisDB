<?php
if (!defined('WP_UNINSTALL_PLUGIN')) {
    exit;
}

delete_option('themisdb_rt_data_source');
delete_option('themisdb_rt_cache_duration');
delete_option('themisdb_rt_github_token');
delete_option('themisdb_rt_display_style');

delete_transient('themisdb_rt_cached_releases');

global $wpdb;
$wpdb->query("DELETE FROM {$wpdb->options} WHERE option_name LIKE 'themisdb_rt_%'");
$wpdb->query("DELETE FROM {$wpdb->options} WHERE option_name LIKE '_transient_themisdb_rt_%'");
