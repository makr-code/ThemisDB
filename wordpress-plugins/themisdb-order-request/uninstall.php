<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            uninstall.php                                      ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:20                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     85                                             ║
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
 * Uninstall script for ThemisDB Order Request Plugin
 * 
 * This file is executed when the plugin is deleted from WordPress
 */

// Exit if accessed directly
if (!defined('WP_UNINSTALL_PLUGIN')) {
    exit;
}

global $wpdb;

// Delete options
$options = array(
    'themisdb_order_epserver_url',
    'themisdb_order_epserver_api_key',
    'themisdb_order_email_from',
    'themisdb_order_email_from_name',
    'themisdb_order_pdf_storage',
    'themisdb_order_legal_compliance'
);

foreach ($options as $option) {
    delete_option($option);
}

// Ask user if they want to delete data
// In WordPress, this is typically handled via a settings page
// For now, we'll keep the data by default for safety

// If you want to delete all data on uninstall, uncomment the following:
/*
// Delete tables
$tables = array(
    $wpdb->prefix . 'themisdb_orders',
    $wpdb->prefix . 'themisdb_contracts',
    $wpdb->prefix . 'themisdb_contract_revisions',
    $wpdb->prefix . 'themisdb_products',
    $wpdb->prefix . 'themisdb_modules',
    $wpdb->prefix . 'themisdb_training_modules',
    $wpdb->prefix . 'themisdb_email_log'
);

foreach ($tables as $table) {
    $wpdb->query($wpdb->prepare("DROP TABLE IF EXISTS %i", $table));
}

// Delete uploaded PDF files
$upload_dir = wp_upload_dir();
$pdf_dir = $upload_dir['basedir'] . '/themisdb-contracts';

if (is_dir($pdf_dir)) {
    $files = glob($pdf_dir . '/*');
    foreach ($files as $file) {
        if (is_file($file)) {
            unlink($file);
        }
    }
    rmdir($pdf_dir);
}
*/
