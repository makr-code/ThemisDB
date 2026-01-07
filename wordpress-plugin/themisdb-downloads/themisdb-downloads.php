<?php
/**
 * Plugin Name: ThemisDB Downloads
 * Plugin URI: https://github.com/makr-code/ThemisDB
 * Description: Automatisch die neuesten ThemisDB Packages von GitHub abrufen und als Download-Links mit SHA256-Checksums anzeigen
 * Version: 1.0.0
 * Author: ThemisDB Team
 * Author URI: https://github.com/makr-code/ThemisDB
 * License: MIT
 * Text Domain: themisdb-downloads
 * Domain Path: /languages
 */

// Prevent direct access
if (!defined('ABSPATH')) {
    exit;
}

// Plugin constants
define('THEMISDB_DOWNLOADS_VERSION', '1.0.0');
define('THEMISDB_DOWNLOADS_PLUGIN_DIR', plugin_dir_path(__FILE__));
define('THEMISDB_DOWNLOADS_PLUGIN_URL', plugin_dir_url(__FILE__));

// Include required files
require_once THEMISDB_DOWNLOADS_PLUGIN_DIR . 'includes/class-github-api.php';
require_once THEMISDB_DOWNLOADS_PLUGIN_DIR . 'includes/class-admin.php';
require_once THEMISDB_DOWNLOADS_PLUGIN_DIR . 'includes/class-shortcodes.php';

/**
 * Initialize the plugin
 */
function themisdb_downloads_init() {
    // Initialize admin panel
    if (is_admin()) {
        new ThemisDB_Downloads_Admin();
    }
    
    // Initialize shortcodes
    new ThemisDB_Downloads_Shortcodes();
    
    // Load text domain for translations
    load_plugin_textdomain('themisdb-downloads', false, dirname(plugin_basename(__FILE__)) . '/languages');
}
add_action('plugins_loaded', 'themisdb_downloads_init');

/**
 * Activation hook
 */
function themisdb_downloads_activate() {
    // Set default options
    if (!get_option('themisdb_github_repo')) {
        add_option('themisdb_github_repo', 'makr-code/ThemisDB');
    }
    if (!get_option('themisdb_cache_duration')) {
        add_option('themisdb_cache_duration', 3600); // 1 hour
    }
    if (!get_option('themisdb_github_token')) {
        add_option('themisdb_github_token', '');
    }
}
register_activation_hook(__FILE__, 'themisdb_downloads_activate');

/**
 * Deactivation hook
 */
function themisdb_downloads_deactivate() {
    // Clear transients
    delete_transient('themisdb_latest_release');
    delete_transient('themisdb_all_releases');
}
register_deactivation_hook(__FILE__, 'themisdb_downloads_deactivate');

/**
 * Enqueue frontend scripts and styles
 */
function themisdb_downloads_enqueue_scripts() {
    wp_enqueue_style(
        'themisdb-downloads-style',
        THEMISDB_DOWNLOADS_PLUGIN_URL . 'assets/css/style.css',
        array(),
        THEMISDB_DOWNLOADS_VERSION
    );
    
    wp_enqueue_script(
        'themisdb-downloads-script',
        THEMISDB_DOWNLOADS_PLUGIN_URL . 'assets/js/script.js',
        array('jquery'),
        THEMISDB_DOWNLOADS_VERSION,
        true
    );
    
    // Localize script for AJAX
    wp_localize_script('themisdb-downloads-script', 'themisdbDownloads', array(
        'ajaxurl' => admin_url('admin-ajax.php'),
        'nonce' => wp_create_nonce('themisdb_downloads_nonce')
    ));
}
add_action('wp_enqueue_scripts', 'themisdb_downloads_enqueue_scripts');
