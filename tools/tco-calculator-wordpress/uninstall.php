<?php
/**
 * Uninstall script for ThemisDB TCO Calculator
 * 
 * This file is executed when the plugin is deleted via WordPress admin.
 * It removes all plugin data from the database.
 */

// Exit if uninstall not called from WordPress
if (!defined('WP_UNINSTALL_PLUGIN')) {
    exit;
}

// Delete plugin options
delete_option('themisdb_tco_enable_ai_features');
delete_option('themisdb_tco_default_requests_per_day');
delete_option('themisdb_tco_default_data_size');
delete_option('themisdb_tco_default_peak_load');
delete_option('themisdb_tco_default_availability');

// Delete transients
delete_transient('themisdb_tco_github_release');

// For multisite installations
if (is_multisite()) {
    global $wpdb;
    
    $blog_ids = $wpdb->get_col("SELECT blog_id FROM $wpdb->blogs");
    
    foreach ($blog_ids as $blog_id) {
        switch_to_blog($blog_id);
        
        // Delete options for each site
        delete_option('themisdb_tco_enable_ai_features');
        delete_option('themisdb_tco_default_requests_per_day');
        delete_option('themisdb_tco_default_data_size');
        delete_option('themisdb_tco_default_peak_load');
        delete_option('themisdb_tco_default_availability');
        
        // Delete transients
        delete_transient('themisdb_tco_github_release');
        
        restore_current_blog();
    }
}
