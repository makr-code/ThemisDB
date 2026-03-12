<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            themisdb-docker-downloads.php                      ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:17                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     156                                            ║
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
 * Plugin Name: ThemisDB Docker Downloads
 * Plugin URI: https://github.com/makr-code/wordpressPlugins
 * Description: Automatisch die neuesten ThemisDB Docker Images von Docker Hub abrufen und als Download-Links mit SHA256-Digests anzeigen.
 * Version: 1.0.0
 * Author: ThemisDB Team
 * Author URI: https://github.com/makr-code/wordpressPlugins
 * License: MIT
 * Text Domain: themisdb-docker-downloads
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
        echo '<div class="error"><p><strong>ThemisDB Docker Downloads:</strong> Dieses Plugin benötigt PHP 7.2 oder höher. Sie verwenden PHP ' . esc_html(PHP_VERSION) . '</p></div>';
    });
    return;
}

// Plugin constants
define('THEMISDB_DOCKER_DOWNLOADS_VERSION', '1.0.0');
define('THEMISDB_DOCKER_DOWNLOADS_PLUGIN_DIR', plugin_dir_path(__FILE__));
define('THEMISDB_DOCKER_DOWNLOADS_PLUGIN_URL', plugin_dir_url(__FILE__));
define('THEMISDB_DOCKER_DOWNLOADS_PLUGIN_FILE', __FILE__);

// Load updater class
$themisdb_updater_local = THEMISDB_DOCKER_DOWNLOADS_PLUGIN_DIR . 'includes/class-themisdb-plugin-updater.php';
$themisdb_updater_shared = dirname(THEMISDB_DOCKER_DOWNLOADS_PLUGIN_DIR) . '/includes/class-themisdb-plugin-updater.php';

if (file_exists($themisdb_updater_local)) {
    require_once $themisdb_updater_local;
} elseif (file_exists($themisdb_updater_shared)) {
    require_once $themisdb_updater_shared;
}

// Initialize automatic updates
if (class_exists('ThemisDB_Plugin_Updater')) {
    new ThemisDB_Plugin_Updater(
        THEMISDB_DOCKER_DOWNLOADS_PLUGIN_FILE,
        'themisdb-docker-downloads',
        THEMISDB_DOCKER_DOWNLOADS_VERSION
    );
}

// Include required files
require_once THEMISDB_DOCKER_DOWNLOADS_PLUGIN_DIR . 'includes/class-dockerhub-api.php';
require_once THEMISDB_DOCKER_DOWNLOADS_PLUGIN_DIR . 'includes/class-admin.php';
require_once THEMISDB_DOCKER_DOWNLOADS_PLUGIN_DIR . 'includes/class-shortcodes.php';

/**
 * Initialize the plugin
 */
function themisdb_docker_downloads_init() {
    // Initialize admin panel
    if (is_admin()) {
        new ThemisDB_Docker_Downloads_Admin();
    }
    
    // Initialize shortcodes
    new ThemisDB_Docker_Downloads_Shortcodes();
    
    // Load text domain for translations
    load_plugin_textdomain('themisdb-docker-downloads', false, dirname(plugin_basename(__FILE__)) . '/languages');
}
add_action('plugins_loaded', 'themisdb_docker_downloads_init');

/**
 * Activation hook
 */
function themisdb_docker_downloads_activate() {
    // Set default options
    if (!get_option('themisdb_docker_namespace')) {
        add_option('themisdb_docker_namespace', 'themisdb');
    }
    if (!get_option('themisdb_docker_repository')) {
        add_option('themisdb_docker_repository', 'themisdb');
    }
    if (!get_option('themisdb_docker_cache_duration')) {
        add_option('themisdb_docker_cache_duration', 3600); // 1 hour
    }
    if (!get_option('themisdb_docker_token')) {
        add_option('themisdb_docker_token', '');
    }
}
register_activation_hook(__FILE__, 'themisdb_docker_downloads_activate');

/**
 * Deactivation hook
 */
function themisdb_docker_downloads_deactivate() {
    // Clear transients
    delete_transient('themisdb_docker_latest_tags');
    delete_transient('themisdb_docker_all_tags');
}
register_deactivation_hook(__FILE__, 'themisdb_docker_downloads_deactivate');

/**
 * Enqueue frontend scripts and styles
 */
function themisdb_docker_downloads_enqueue_scripts() {
    global $post;
    
    // Only load if shortcodes are present
    if (!is_a($post, 'WP_Post') || !(
        has_shortcode($post->post_content, 'themisdb_docker_tags') ||
        has_shortcode($post->post_content, 'themisdb_docker_latest')
    )) {
        return;
    }
    
    wp_enqueue_style(
        'themisdb-docker-downloads-style',
        THEMISDB_DOCKER_DOWNLOADS_PLUGIN_URL . 'assets/css/style.css',
        array(),
        THEMISDB_DOCKER_DOWNLOADS_VERSION
    );
    
    wp_enqueue_script(
        'themisdb-docker-downloads-script',
        THEMISDB_DOCKER_DOWNLOADS_PLUGIN_URL . 'assets/js/script.js',
        array('jquery'),
        THEMISDB_DOCKER_DOWNLOADS_VERSION,
        true
    );
    
    // Localize script for AJAX
    wp_localize_script('themisdb-docker-downloads-script', 'themisdbDockerDownloads', array(
        'ajaxurl' => admin_url('admin-ajax.php'),
        'nonce' => wp_create_nonce('themisdb_docker_downloads_nonce')
    ));
}
add_action('wp_enqueue_scripts', 'themisdb_docker_downloads_enqueue_scripts');
