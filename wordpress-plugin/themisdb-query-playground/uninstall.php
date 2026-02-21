/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            uninstall.php                                      ║
  Version:         0.0.23                                             ║
  Last Modified:   2026-02-21 19:43:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     54                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

<?php
/**
 * Uninstall script for ThemisDB Query Playground
 */

if (!defined('WP_UNINSTALL_PLUGIN')) {
    exit;
}

// Delete plugin options
delete_option('themisdb_qp_endpoint');
delete_option('themisdb_qp_namespace');
delete_option('themisdb_qp_timeout');
delete_option('themisdb_qp_enable_execution');
delete_option('themisdb_qp_max_results');
delete_option('themisdb_qp_enable_examples');
delete_option('themisdb_qp_theme');
delete_option('themisdb_qp_client_path');
delete_option('themisdb_qp_read_only_mode');

// Delete transients
delete_transient('themisdb_qp_cached_examples');

// Delete cached data
global $wpdb;
$wpdb->query($wpdb->prepare("DELETE FROM {$wpdb->options} WHERE option_name LIKE %s", 'themisdb_qp_%'));
$wpdb->query($wpdb->prepare("DELETE FROM {$wpdb->options} WHERE option_name LIKE %s", '_transient_themisdb_qp_%'));
$wpdb->query($wpdb->prepare("DELETE FROM {$wpdb->options} WHERE option_name LIKE %s", '_transient_timeout_themisdb_qp_%'));
