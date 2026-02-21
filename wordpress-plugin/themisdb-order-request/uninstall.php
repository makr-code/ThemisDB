/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            uninstall.php                                      ║
  Version:         0.0.20                                             ║
  Last Modified:   2026-02-21 19:15:02                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     89                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
