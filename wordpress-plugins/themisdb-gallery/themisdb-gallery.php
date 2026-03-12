<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            themisdb-gallery.php                               ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     206                                            ║
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
 * Plugin Name: ThemisDB Gallery
 * Plugin URI: https://github.com/makr-code/wordpressPlugins
 * Description: Hilft beim Artikel erstellen relevante frei verfügbare thematisch passende Bilder im Internet zu finden, herunterzuladen und einzubinden - mit vollen Credits (Urheber usw.)
 * Version: 1.0.1
 * Author: ThemisDB Team
 * Author URI: https://github.com/makr-code/wordpressPlugins
 * License: MIT
 * Text Domain: themisdb-gallery
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
        echo '<div class="error"><p><strong>ThemisDB Gallery:</strong> Dieses Plugin benötigt PHP 7.2 oder höher. Sie verwenden PHP ' . PHP_VERSION . '</p></div>';
    });
    return;
}

// Plugin constants
define('THEMISDB_GALLERY_VERSION', '1.0.1');
define('THEMISDB_GALLERY_PLUGIN_DIR', plugin_dir_path(__FILE__));
define('THEMISDB_GALLERY_PLUGIN_URL', plugin_dir_url(__FILE__));
define('THEMISDB_GALLERY_PLUGIN_BASENAME', plugin_basename(__FILE__));
define('THEMISDB_GALLERY_PLUGIN_FILE', __FILE__);

// Load updater class
$themisdb_updater_local = THEMISDB_GALLERY_PLUGIN_DIR . 'includes/class-themisdb-plugin-updater.php';
$themisdb_updater_shared = dirname(THEMISDB_GALLERY_PLUGIN_DIR) . '/includes/class-themisdb-plugin-updater.php';

if (file_exists($themisdb_updater_local)) {
    require_once $themisdb_updater_local;
} elseif (file_exists($themisdb_updater_shared)) {
    require_once $themisdb_updater_shared;
}

// Initialize automatic updates
if (class_exists('ThemisDB_Plugin_Updater')) {
    new ThemisDB_Plugin_Updater(
        THEMISDB_GALLERY_PLUGIN_FILE,
        'themisdb-gallery',
        THEMISDB_GALLERY_VERSION
    );
}

// Include required files
require_once THEMISDB_GALLERY_PLUGIN_DIR . 'includes/class-image-api.php';
require_once THEMISDB_GALLERY_PLUGIN_DIR . 'includes/class-admin.php';
require_once THEMISDB_GALLERY_PLUGIN_DIR . 'includes/class-media-handler.php';
require_once THEMISDB_GALLERY_PLUGIN_DIR . 'includes/class-shortcodes.php';
require_once THEMISDB_GALLERY_PLUGIN_DIR . 'includes/class-gutenberg-block.php';

/**
 * Initialize the plugin
 */
function themisdb_gallery_init() {
    // Initialize admin panel
    if (is_admin()) {
        new ThemisDB_Gallery_Admin();
    }
    
    // Initialize shortcodes
    new ThemisDB_Gallery_Shortcodes();
    
    // Initialize Gutenberg block
    new ThemisDB_Gallery_Gutenberg_Block();
    
    // Load text domain for translations
    load_plugin_textdomain('themisdb-gallery', false, dirname(plugin_basename(__FILE__)) . '/languages');
}
add_action('plugins_loaded', 'themisdb_gallery_init');

/**
 * Activation hook
 */
function themisdb_gallery_activate() {
    // Set default options
    if (!get_option('themisdb_gallery_unsplash_key')) {
        add_option('themisdb_gallery_unsplash_key', '');
    }
    if (!get_option('themisdb_gallery_pexels_key')) {
        add_option('themisdb_gallery_pexels_key', '');
    }
    if (!get_option('themisdb_gallery_pixabay_key')) {
        add_option('themisdb_gallery_pixabay_key', '');
    }
    if (!get_option('themisdb_gallery_openai_key')) {
        add_option('themisdb_gallery_openai_key', '');
    }
    if (!get_option('themisdb_gallery_cache_duration')) {
        add_option('themisdb_gallery_cache_duration', 3600); // 1 hour
    }
    if (!get_option('themisdb_gallery_default_provider')) {
        add_option('themisdb_gallery_default_provider', 'unsplash');
    }
    if (!get_option('themisdb_gallery_images_per_page')) {
        add_option('themisdb_gallery_images_per_page', 20);
    }
    if (!get_option('themisdb_gallery_auto_attribution')) {
        add_option('themisdb_gallery_auto_attribution', 'yes');
    }
}
register_activation_hook(__FILE__, 'themisdb_gallery_activate');

/**
 * Deactivation hook
 */
function themisdb_gallery_deactivate() {
    // Clear transients
    global $wpdb;
    $wpdb->query("DELETE FROM {$wpdb->options} WHERE option_name LIKE '_transient_themisdb_gallery_%'");
    $wpdb->query("DELETE FROM {$wpdb->options} WHERE option_name LIKE '_transient_timeout_themisdb_gallery_%'");
}
register_deactivation_hook(__FILE__, 'themisdb_gallery_deactivate');

/**
 * Enqueue frontend scripts and styles
 */
function themisdb_gallery_enqueue_scripts() {
    wp_enqueue_style(
        'themisdb-gallery-style',
        THEMISDB_GALLERY_PLUGIN_URL . 'assets/css/style.css',
        array(),
        THEMISDB_GALLERY_VERSION
    );
    
    wp_enqueue_script(
        'themisdb-gallery-script',
        THEMISDB_GALLERY_PLUGIN_URL . 'assets/js/script.js',
        array('jquery'),
        THEMISDB_GALLERY_VERSION,
        true
    );
    
    // Localize script for AJAX
    wp_localize_script('themisdb-gallery-script', 'themisdbGallery', array(
        'ajaxurl' => admin_url('admin-ajax.php'),
        'nonce' => wp_create_nonce('themisdb_gallery_nonce')
    ));
}
add_action('wp_enqueue_scripts', 'themisdb_gallery_enqueue_scripts');

/**
 * Enqueue admin scripts and styles
 */
function themisdb_gallery_admin_enqueue_scripts($hook) {
    // Only load on post editor and plugin settings page
    if (!in_array($hook, array('post.php', 'post-new.php', 'settings_page_themisdb-gallery'))) {
        return;
    }
    
    wp_enqueue_style(
        'themisdb-gallery-admin-style',
        THEMISDB_GALLERY_PLUGIN_URL . 'assets/css/admin.css',
        array(),
        THEMISDB_GALLERY_VERSION
    );
    
    wp_enqueue_script(
        'themisdb-gallery-admin-script',
        THEMISDB_GALLERY_PLUGIN_URL . 'assets/js/admin.js',
        array('jquery', 'jquery-ui-dialog'),
        THEMISDB_GALLERY_VERSION,
        true
    );
    
    // Enqueue WordPress media library
    wp_enqueue_media();
    
    // Localize script for AJAX
    wp_localize_script('themisdb-gallery-admin-script', 'themisdbGalleryAdmin', array(
        'ajaxurl' => admin_url('admin-ajax.php'),
        'nonce' => wp_create_nonce('themisdb_gallery_admin_nonce'),
        'searchPlaceholder' => __('Suche nach Bildern...', 'themisdb-gallery'),
        'searching' => __('Suche läuft...', 'themisdb-gallery'),
        'noResults' => __('Keine Bilder gefunden', 'themisdb-gallery'),
        'insertImage' => __('Bild einfügen', 'themisdb-gallery'),
        'downloading' => __('Lade herunter...', 'themisdb-gallery'),
        'error' => __('Fehler beim Laden', 'themisdb-gallery')
    ));
}
add_action('admin_enqueue_scripts', 'themisdb_gallery_admin_enqueue_scripts');
