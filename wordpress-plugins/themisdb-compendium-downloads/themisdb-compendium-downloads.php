<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            themisdb-compendium-downloads.php                  ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:17                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     189                                            ║
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
 * Plugin Name: ThemisDB Compendium Downloads
 * Plugin URI: https://github.com/makr-code/wordpressPlugins
 * Description: Bietet ThemisDB Kompendium PDF-Versionen als Downloads auf der Website an, analog zu Docker und GitHub Releases.
 * Version: 1.0.0
 * Author: ThemisDB Team
 * Author URI: https://github.com/makr-code/wordpressPlugins
 * License: MIT
 * Text Domain: themisdb-compendium-downloads
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
        echo '<div class="error"><p><strong>ThemisDB Compendium Downloads:</strong> Dieses Plugin benötigt PHP 7.2 oder höher. Sie verwenden PHP ' . esc_html(PHP_VERSION) . '</p></div>';
    });
    return;
}

// Plugin constants
define('THEMISDB_COMPENDIUM_VERSION', '1.0.0');
define('THEMISDB_COMPENDIUM_PLUGIN_DIR', plugin_dir_path(__FILE__));
define('THEMISDB_COMPENDIUM_PLUGIN_URL', plugin_dir_url(__FILE__));
define('THEMISDB_COMPENDIUM_PLUGIN_FILE', __FILE__);

// Load updater class
$themisdb_updater_local = THEMISDB_COMPENDIUM_PLUGIN_DIR . 'includes/class-themisdb-plugin-updater.php';
$themisdb_updater_shared = dirname(THEMISDB_COMPENDIUM_PLUGIN_DIR) . '/includes/class-themisdb-plugin-updater.php';

if (file_exists($themisdb_updater_local)) {
    require_once $themisdb_updater_local;
} elseif (file_exists($themisdb_updater_shared)) {
    require_once $themisdb_updater_shared;
}

// Initialize automatic updates
if (class_exists('ThemisDB_Plugin_Updater')) {
    new ThemisDB_Plugin_Updater(
        THEMISDB_COMPENDIUM_PLUGIN_FILE,
        'themisdb-compendium-downloads',
        THEMISDB_COMPENDIUM_VERSION
    );
}

// Include required files
require_once THEMISDB_COMPENDIUM_PLUGIN_DIR . 'includes/class-compendium-downloads.php';
require_once THEMISDB_COMPENDIUM_PLUGIN_DIR . 'includes/class-compendium-widget.php';
require_once THEMISDB_COMPENDIUM_PLUGIN_DIR . 'includes/class-compendium-admin.php';

/**
 * Initialize the plugin
 */
function themisdb_compendium_init() {
    // Initialize compendium downloads
    new ThemisDB_Compendium_Downloads();
    
    // Initialize admin settings
    if (is_admin()) {
        new ThemisDB_Compendium_Admin();
    }
    
    // Load text domain for translations
    load_plugin_textdomain('themisdb-compendium-downloads', false, dirname(plugin_basename(__FILE__)) . '/languages');
}
add_action('plugins_loaded', 'themisdb_compendium_init');

/**
 * Register widgets
 */
function themisdb_compendium_register_widgets() {
    register_widget('ThemisDB_Compendium_Widget');
}
add_action('widgets_init', 'themisdb_compendium_register_widgets');

/**
 * Activation hook
 */
function themisdb_compendium_activate() {
    // Set default options
    if (get_option('themisdb_compendium_github_repo') === false) {
        add_option('themisdb_compendium_github_repo', 'makr-code/wordpressPlugins');
    }
    if (get_option('themisdb_compendium_show_file_sizes') === false) {
        add_option('themisdb_compendium_show_file_sizes', 1);
    }
    if (get_option('themisdb_compendium_cache_duration') === false) {
        add_option('themisdb_compendium_cache_duration', 3600); // 1 hour default
    }
    if (get_option('themisdb_compendium_button_style') === false) {
        add_option('themisdb_compendium_button_style', 'modern');
    }
    if (get_option('themisdb_compendium_search_term') === false) {
        add_option('themisdb_compendium_search_term', 'kompendium');
    }
}
register_activation_hook(__FILE__, 'themisdb_compendium_activate');

/**
 * Deactivation hook
 */
function themisdb_compendium_deactivate() {
    // Clear cached data
    delete_transient('themisdb_compendium_release_data');
}
register_deactivation_hook(__FILE__, 'themisdb_compendium_deactivate');

/**
 * Enqueue frontend scripts and styles
 */
function themisdb_compendium_enqueue_scripts() {
    global $post;
    
    // Only load if shortcodes are present
    if (!is_a($post, 'WP_Post') || !(
        has_shortcode($post->post_content, 'themisdb_compendium_downloads') ||
        has_shortcode($post->post_content, 'themisdb_compendium')
    )) {
        return;
    }
    
    wp_enqueue_style(
        'themisdb-compendium-style',
        THEMISDB_COMPENDIUM_PLUGIN_URL . 'assets/css/style.css',
        array(),
        THEMISDB_COMPENDIUM_VERSION
    );
    
    wp_enqueue_script(
        'themisdb-compendium-script',
        THEMISDB_COMPENDIUM_PLUGIN_URL . 'assets/js/script.js',
        array('jquery'),
        THEMISDB_COMPENDIUM_VERSION,
        true
    );
    
    // Pass settings to JavaScript
    wp_localize_script('themisdb-compendium-script', 'themisdbCompendium', array(
        'ajaxUrl' => admin_url('admin-ajax.php'),
        'nonce' => wp_create_nonce('themisdb_compendium_download_track')
    ));
    
    // Add debug flag if WP_DEBUG is enabled
    if (defined('WP_DEBUG') && WP_DEBUG) {
        wp_add_inline_script('themisdb-compendium-script', 'window.themisdbDebug = true;', 'before');
    }
}
add_action('wp_enqueue_scripts', 'themisdb_compendium_enqueue_scripts');

/**
 * Enqueue admin scripts and styles
 */
function themisdb_compendium_admin_enqueue_scripts($hook) {
    // Only load on plugin settings page
    if ($hook !== 'settings_page_themisdb-compendium-downloads') {
        return;
    }
    
    wp_enqueue_style(
        'themisdb-compendium-admin-style',
        THEMISDB_COMPENDIUM_PLUGIN_URL . 'assets/css/admin-style.css',
        array(),
        THEMISDB_COMPENDIUM_VERSION
    );
}
add_action('admin_enqueue_scripts', 'themisdb_compendium_admin_enqueue_scripts');
