<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            themisdb-downloads.php                             ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:17                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     203                                            ║
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
 * Plugin Name: ThemisDB Downloads
 * Plugin URI: https://github.com/makr-code/wordpressPlugins
 * Description: Automatisch die neuesten ThemisDB Packages von GitHub abrufen und als Download-Links mit SHA256-Checksums anzeigen. Extrahiert automatisch Schlagwörter und Kategorien aus Beitragsinhalten.
 * Version: 1.2.0
 * Author: ThemisDB Team
 * Author URI: https://github.com/makr-code/wordpressPlugins
 * License: MIT
 * Text Domain: themisdb-downloads
 * Domain Path: /languages
 * Requires at least: 5.0
 * Requires PHP: 7.2
 */

// Prevent direct access
if (!defined('ABSPATH')) {
    exit;
}

// Check PHP version
if (version_compare(PHP_VERSION, '7.2', '<')) {
    add_action('admin_notices', function() {
        echo '<div class="error"><p><strong>ThemisDB Downloads:</strong> Dieses Plugin benötigt PHP 7.2 oder höher. Sie verwenden PHP ' . PHP_VERSION . '</p></div>';
    });
    return;
}

// Plugin constants
define('THEMISDB_DOWNLOADS_VERSION', '1.2.0');
define('THEMISDB_DOWNLOADS_PLUGIN_DIR', plugin_dir_path(__FILE__));
define('THEMISDB_DOWNLOADS_PLUGIN_URL', plugin_dir_url(__FILE__));
define('THEMISDB_DOWNLOADS_PLUGIN_FILE', __FILE__);

// Load updater class
$themisdb_updater_local = THEMISDB_DOWNLOADS_PLUGIN_DIR . 'includes/class-themisdb-plugin-updater.php';
$themisdb_updater_shared = dirname(THEMISDB_DOWNLOADS_PLUGIN_DIR) . '/includes/class-themisdb-plugin-updater.php';

if (file_exists($themisdb_updater_local)) {
    require_once $themisdb_updater_local;
} elseif (file_exists($themisdb_updater_shared)) {
    require_once $themisdb_updater_shared;
}

// Initialize automatic updates
if (class_exists('ThemisDB_Plugin_Updater')) {
    new ThemisDB_Plugin_Updater(
        THEMISDB_DOWNLOADS_PLUGIN_FILE,
        'themisdb-downloads',
        THEMISDB_DOWNLOADS_VERSION
    );
}

// Include required files
require_once THEMISDB_DOWNLOADS_PLUGIN_DIR . 'includes/class-markdown-converter.php';
require_once THEMISDB_DOWNLOADS_PLUGIN_DIR . 'includes/class-github-api.php';
require_once THEMISDB_DOWNLOADS_PLUGIN_DIR . 'includes/class-admin.php';
require_once THEMISDB_DOWNLOADS_PLUGIN_DIR . 'includes/class-shortcodes.php';

// Keep legacy taxonomy manager for backward compatibility
// But prefer shared taxonomy manager if available
if (!function_exists('themisdb_get_taxonomy_manager')) {
    require_once THEMISDB_DOWNLOADS_PLUGIN_DIR . 'includes/class-taxonomy-manager.php';
}

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
    
    // Initialize taxonomy manager
    // Use shared taxonomy manager if available, otherwise use legacy
    if (!function_exists('themisdb_get_taxonomy_manager')) {
        new ThemisDB_Downloads_Taxonomy_Manager();
    } else {
        // Shared taxonomy manager is active, no need for legacy manager
        add_action('admin_notices', 'themisdb_downloads_show_shared_taxonomy_notice');
    }
    
    // Load text domain for translations
    load_plugin_textdomain('themisdb-downloads', false, dirname(plugin_basename(__FILE__)) . '/languages');
}
add_action('plugins_loaded', 'themisdb_downloads_init');

/**
 * Show notice when shared taxonomy manager is active
 */
function themisdb_downloads_show_shared_taxonomy_notice() {
    $screen = get_current_screen();
    if ($screen && $screen->id === 'plugins') {
        echo '<div class="notice notice-info"><p>';
        echo '<strong>ThemisDB Downloads:</strong> Using shared ThemisDB Taxonomy Manager for enhanced category and tag management.';
        echo '</p></div>';
    }
}

/**
 * Activation hook
 */
function themisdb_downloads_activate() {
    // Set default options
    if (!get_option('themisdb_github_repo')) {
        add_option('themisdb_github_repo', 'makr-code/wordpressPlugins');
    }
    if (!get_option('themisdb_cache_duration')) {
        add_option('themisdb_cache_duration', 3600); // 1 hour
    }
    if (!get_option('themisdb_github_token')) {
        add_option('themisdb_github_token', '');
    }
    // Auto-taxonomy options
    if (get_option('themisdb_auto_taxonomy') === false) {
        add_option('themisdb_auto_taxonomy', 1); // Enabled by default
    }
    if (get_option('themisdb_auto_tags') === false) {
        add_option('themisdb_auto_tags', 1); // Enabled by default
    }
    if (get_option('themisdb_auto_categories') === false) {
        add_option('themisdb_auto_categories', 1); // Enabled by default
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
    global $post;
    
    // Only load if shortcodes are present
    if (!is_a($post, 'WP_Post') || !(
        has_shortcode($post->post_content, 'themisdb_downloads') ||
        has_shortcode($post->post_content, 'themisdb_latest') ||
        has_shortcode($post->post_content, 'themisdb_verify') ||
        has_shortcode($post->post_content, 'themisdb_readme') ||
        has_shortcode($post->post_content, 'themisdb_changelog')
    )) {
        return;
    }
    
    wp_enqueue_style(
        'themisdb-downloads-style',
        THEMISDB_DOWNLOADS_PLUGIN_URL . 'assets/css/style.css',
        array(),
        THEMISDB_DOWNLOADS_VERSION
    );
    
    // Enqueue Mermaid.js for diagram rendering
    wp_enqueue_script(
        'mermaid-js',
        'https://cdn.jsdelivr.net/npm/mermaid@10/dist/mermaid.min.js',
        array(),
        '10.0.0',
        true
    );
    
    wp_enqueue_script(
        'themisdb-downloads-script',
        THEMISDB_DOWNLOADS_PLUGIN_URL . 'assets/js/script.js',
        array('jquery', 'mermaid-js'),
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
