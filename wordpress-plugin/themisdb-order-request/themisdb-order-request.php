/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            themisdb-order-request.php                         ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 11:05:44                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     179                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e7fde96aa  2026-02-18  Add automatic GitHub-based updates for WordPress plugins ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

<?php
/**
 * Plugin Name: ThemisDB Order Request & Contract Management
 * Plugin URI: https://github.com/makr-code/ThemisDB
 * Description: Dialog-basiertes Bestellanfrage-System für ThemisDB mit Vertragsrecht-CRUD, automatischer PDF-Generierung und E-Mail-Versand. Integriert mit epServer für Stammdaten.
 * Version: 1.0.0
 * Author: ThemisDB Team
 * Author URI: https://github.com/makr-code/ThemisDB
 * License: MIT
 * Text Domain: themisdb-order-request
 * Domain Path: /languages
 * Requires at least: 5.0
 * Requires PHP: 7.4
 */

// Prevent direct access
if (!defined('ABSPATH')) {
    exit;
}

// Check PHP version
if (version_compare(PHP_VERSION, '7.4', '<')) {
    add_action('admin_notices', function() {
        echo '<div class="error"><p><strong>ThemisDB Order Request:</strong> Dieses Plugin benötigt PHP 7.4 oder höher. Sie verwenden PHP ' . esc_html(PHP_VERSION) . '</p></div>';
    });
    return;
}

// Plugin constants
define('THEMISDB_ORDER_VERSION', '1.0.0');
define('THEMISDB_ORDER_PLUGIN_DIR', plugin_dir_path(__FILE__));
define('THEMISDB_ORDER_PLUGIN_URL', plugin_dir_url(__FILE__));
define('THEMISDB_ORDER_PLUGIN_FILE', __FILE__);

// Load updater class
require_once dirname(THEMISDB_ORDER_PLUGIN_DIR) . '/includes/class-themisdb-plugin-updater.php';

// Initialize automatic updates
if (class_exists('ThemisDB_Plugin_Updater')) {
    new ThemisDB_Plugin_Updater(
        THEMISDB_ORDER_PLUGIN_FILE,
        'themisdb-order-request',
        THEMISDB_ORDER_VERSION
    );
}

// Include required files
require_once THEMISDB_ORDER_PLUGIN_DIR . 'includes/class-database.php';
require_once THEMISDB_ORDER_PLUGIN_DIR . 'includes/class-order-manager.php';
require_once THEMISDB_ORDER_PLUGIN_DIR . 'includes/class-contract-manager.php';
require_once THEMISDB_ORDER_PLUGIN_DIR . 'includes/class-payment-manager.php';
require_once THEMISDB_ORDER_PLUGIN_DIR . 'includes/class-license-manager.php';
require_once THEMISDB_ORDER_PLUGIN_DIR . 'includes/class-pdf-generator.php';
require_once THEMISDB_ORDER_PLUGIN_DIR . 'includes/class-email-handler.php';
require_once THEMISDB_ORDER_PLUGIN_DIR . 'includes/class-epserver-api.php';
require_once THEMISDB_ORDER_PLUGIN_DIR . 'includes/class-admin.php';
require_once THEMISDB_ORDER_PLUGIN_DIR . 'includes/class-shortcodes.php';
require_once THEMISDB_ORDER_PLUGIN_DIR . 'includes/class-auth-system.php';

/**
 * Initialize the plugin
 */
function themisdb_order_request_init() {
    // Initialize database
    ThemisDB_Order_Database::init();
    
    // Initialize admin panel
    if (is_admin()) {
        new ThemisDB_Order_Admin();
    }
    
    // Initialize shortcodes
    new ThemisDB_Order_Shortcodes();
    
    // Load text domain for translations
    load_plugin_textdomain('themisdb-order-request', false, dirname(plugin_basename(__FILE__)) . '/languages');
}
add_action('plugins_loaded', 'themisdb_order_request_init');

/**
 * Activation hook
 */
function themisdb_order_request_activate() {
    // Create database tables
    ThemisDB_Order_Database::create_tables();
    
    // Set default options
    if (!get_option('themisdb_order_epserver_url')) {
        add_option('themisdb_order_epserver_url', 'https://service.themisdb.org:6734');
    }
    if (!get_option('themisdb_order_epserver_api_key')) {
        add_option('themisdb_order_epserver_api_key', '');
    }
    if (!get_option('themisdb_order_email_from')) {
        add_option('themisdb_order_email_from', get_option('admin_email'));
    }
    if (!get_option('themisdb_order_email_from_name')) {
        add_option('themisdb_order_email_from_name', get_option('blogname'));
    }
    if (!get_option('themisdb_order_pdf_storage')) {
        add_option('themisdb_order_pdf_storage', 'database'); // database or filesystem
    }
    if (!get_option('themisdb_order_legal_compliance')) {
        add_option('themisdb_order_legal_compliance', '1'); // Enable legal compliance checks
    }
    
    // Flush rewrite rules
    flush_rewrite_rules();
}
register_activation_hook(__FILE__, 'themisdb_order_request_activate');

/**
 * Deactivation hook
 */
function themisdb_order_request_deactivate() {
    // Flush rewrite rules
    flush_rewrite_rules();
}
register_deactivation_hook(__FILE__, 'themisdb_order_request_deactivate');

/**
 * Enqueue frontend scripts and styles
 */
function themisdb_order_request_enqueue_scripts() {
    wp_enqueue_style('themisdb-order-request-style', THEMISDB_ORDER_PLUGIN_URL . 'assets/css/order-request.css', array(), THEMISDB_ORDER_VERSION);
    wp_enqueue_script('themisdb-order-request-script', THEMISDB_ORDER_PLUGIN_URL . 'assets/js/order-request.js', array('jquery'), THEMISDB_ORDER_VERSION, true);
    
    // Localize script
    wp_localize_script('themisdb-order-request-script', 'themisdbOrder', array(
        'ajaxUrl' => admin_url('admin-ajax.php'),
        'nonce' => wp_create_nonce('themisdb_order_nonce'),
        'strings' => array(
            'loading' => __('Lädt...', 'themisdb-order-request'),
            'error' => __('Ein Fehler ist aufgetreten', 'themisdb-order-request'),
            'success' => __('Erfolgreich gespeichert', 'themisdb-order-request'),
        )
    ));
}
add_action('wp_enqueue_scripts', 'themisdb_order_request_enqueue_scripts');

/**
 * Enqueue admin scripts and styles
 */
function themisdb_order_request_admin_enqueue_scripts($hook) {
    // Only load on our plugin pages
    if (strpos($hook, 'themisdb-order') === false) {
        return;
    }
    
    wp_enqueue_style('themisdb-order-admin-style', THEMISDB_ORDER_PLUGIN_URL . 'assets/css/admin.css', array(), THEMISDB_ORDER_VERSION);
    wp_enqueue_script('themisdb-order-admin-script', THEMISDB_ORDER_PLUGIN_URL . 'assets/js/admin.js', array('jquery'), THEMISDB_ORDER_VERSION, true);
}
add_action('admin_enqueue_scripts', 'themisdb_order_request_admin_enqueue_scripts');
