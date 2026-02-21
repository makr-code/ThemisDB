/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            uninstall.php                                      ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-02-21 14:08:30                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     89                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

<?php
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
