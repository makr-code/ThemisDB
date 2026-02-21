/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            uninstall.php                                      ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:48:05                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     62                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • fa8becd33  2026-02-16  WordPress plugins: Fix SQL injections, broken PHP, and st... ║
    • c6716ede7  2026-02-16  Add ThemisDB Order Request Plugin with shortcodes, AJAX h... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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
    $sites = get_sites(array('number' => 0));
    
    foreach ($sites as $site) {
        switch_to_blog($site->blog_id);
        
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
