<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            themisdb-feature-matrix.php                        ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:18                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     270                                            ║
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
 * Plugin Name: ThemisDB Feature Matrix
 * Plugin URI: https://github.com/makr-code/wordpressPlugins
 * Description: Interactive feature comparison matrix for ThemisDB vs PostgreSQL, MongoDB, Neo4j
 * Version: 1.0.0
 * Author: ThemisDB Team
 * Author URI: https://github.com/makr-code/wordpressPlugins
 * License: MIT
 * Text Domain: themisdb-feature-matrix
 * Domain Path: /languages
 * Requires at least: 5.8
 * Requires PHP: 7.4
 */

if (!defined('ABSPATH')) {
    exit;
}

// Define plugin constants
define('THEMISDB_FM_VERSION', '1.0.0');
define('THEMISDB_FM_PLUGIN_DIR', plugin_dir_path(__FILE__));
define('THEMISDB_FM_PLUGIN_URL', plugin_dir_url(__FILE__));
define('THEMISDB_FM_PLUGIN_FILE', __FILE__);

// Backwards compatibility - deprecated constants
define('THEMISDB_MATRIX_DIR', THEMISDB_FM_PLUGIN_DIR);
define('THEMISDB_MATRIX_URL', THEMISDB_FM_PLUGIN_URL);
define('THEMISDB_MATRIX_VERSION', THEMISDB_FM_VERSION);

/**
 * Safe require helper - loads files with error handling and logging
 *
 * @param string $file File path to require
 * @param bool $is_critical Whether this file is critical for plugin operation
 * @return bool True if file was loaded successfully, false otherwise
 */
function themisdb_fm_safe_require($file, $is_critical = true) {
    static $displayed_notices = array();
    
    if (file_exists($file)) {
        require_once $file;
        return true;
    }
    
    $error_message = sprintf(
        '[ThemisDB Feature Matrix] Missing file: %s',
        str_replace(THEMISDB_FM_PLUGIN_DIR, '', $file)
    );
    
    // Log to debug.log if WP_DEBUG_LOG is enabled
    if (defined('WP_DEBUG_LOG') && WP_DEBUG_LOG) {
        error_log($error_message);
    }
    
    // Add admin notice (only once per file)
    $notice_key = md5($file);
    if (!isset($displayed_notices[$notice_key])) {
        $displayed_notices[$notice_key] = true;
        add_action('admin_notices', function() use ($file, $is_critical) {
            $class = $is_critical ? 'notice-error' : 'notice-warning';
            $message = sprintf(
                '<strong>ThemisDB Feature Matrix:</strong> Missing file: %s',
                esc_html(basename($file))
            );
            if ($is_critical) {
                $message .= ' - Plugin functionality may be impaired.';
            }
            printf('<div class="notice %s"><p>%s</p></div>', esc_attr($class), $message);
        });
    }
    
    return false;
}

// Track if all critical files loaded successfully
$themisdb_fm_files_loaded = true;

// Load required files
// Note: Using bitwise AND (&=) instead of logical AND (&&=) to prevent short-circuit evaluation
// This ensures ALL files are checked and logged, not just until the first failure
$themisdb_fm_files_loaded &= themisdb_fm_safe_require(THEMISDB_FM_PLUGIN_DIR . 'includes/class-feature-matrix.php');
$themisdb_fm_files_loaded &= themisdb_fm_safe_require(THEMISDB_FM_PLUGIN_DIR . 'includes/class-shortcode.php');
$themisdb_fm_files_loaded &= themisdb_fm_safe_require(THEMISDB_FM_PLUGIN_DIR . 'includes/class-admin.php');

// Abort initialization if critical files are missing
if (!$themisdb_fm_files_loaded) {
    if (defined('WP_DEBUG_LOG') && WP_DEBUG_LOG) {
        error_log('[ThemisDB Feature Matrix] Plugin initialization aborted due to missing files');
    }
    return;
}

// Load updater class
$themisdb_updater_local = THEMISDB_FM_PLUGIN_DIR . 'includes/class-themisdb-plugin-updater.php';
$themisdb_updater_shared = dirname(THEMISDB_FM_PLUGIN_DIR) . '/includes/class-themisdb-plugin-updater.php';

if (file_exists($themisdb_updater_local)) {
    require_once $themisdb_updater_local;
} elseif (file_exists($themisdb_updater_shared)) {
    require_once $themisdb_updater_shared;
}

// Initialize automatic updates
if (class_exists('ThemisDB_Plugin_Updater')) {
    new ThemisDB_Plugin_Updater(
        THEMISDB_FM_PLUGIN_FILE,
        'themisdb-feature-matrix',
        THEMISDB_FM_VERSION
    );
}

/**
 * Initialize plugin
 */
function themisdb_matrix_init() {
    load_plugin_textdomain('themisdb-feature-matrix', false, dirname(plugin_basename(__FILE__)) . '/languages');
    
    // Initialize shortcode handler
    new ThemisDB_Matrix_Shortcode();
    
    // Initialize admin if in admin area
    if (is_admin()) {
        new ThemisDB_Feature_Matrix_Admin();
    }
}
add_action('plugins_loaded', 'themisdb_matrix_init');

/**
 * Enqueue assets
 */
function themisdb_matrix_enqueue_assets() {
    global $post;
    
    // Only load if shortcode is present
    if (!is_a($post, 'WP_Post') || !has_shortcode($post->post_content, 'themisdb_feature_matrix')) {
        return;
    }
    
    // Plugin CSS
    wp_enqueue_style(
        'themisdb-fm-style',
        THEMISDB_FM_PLUGIN_URL . 'assets/css/feature-matrix.css',
        array(),
        THEMISDB_FM_VERSION
    );
    
    // Dark mode CSS if enabled
    $color_scheme = themisdb_matrix_get_color_scheme();
    if ($color_scheme === 'dark') {
        wp_enqueue_style(
            'themisdb-fm-dark-style',
            THEMISDB_FM_PLUGIN_URL . 'assets/css/feature-matrix-dark.css',
            array('themisdb-fm-style'),
            THEMISDB_FM_VERSION
        );
    }
    
    // Plugin JS
    wp_enqueue_script(
        'themisdb-fm-script',
        THEMISDB_FM_PLUGIN_URL . 'assets/js/feature-matrix.js',
        array('jquery'),
        THEMISDB_FM_VERSION,
        true
    );
    
    // Localize script with AJAX URL and settings
    wp_localize_script('themisdb-fm-script', 'themisdbFM', array(
        'ajax_url' => admin_url('admin-ajax.php'),
        'nonce' => wp_create_nonce('themisdb_fm_nonce'),
        'plugin_url' => THEMISDB_FM_PLUGIN_URL,
        'settings' => array(
            'default_view' => get_option('themisdb_fm_default_view', 'all'),
            'default_style' => get_option('themisdb_fm_default_style', 'modern'),
            'enable_filters' => get_option('themisdb_fm_enable_filters', 'yes'),
            'enable_csv_export' => get_option('themisdb_fm_enable_csv_export', 'yes'),
            'show_themis_highlight' => get_option('themisdb_fm_show_themis_highlight', 'yes'),
            'sticky_header' => get_option('themisdb_fm_sticky_header', 'yes'),
            'enable_tooltips' => get_option('themisdb_fm_enable_tooltips', 'yes')
        ),
    ));
}
add_action('wp_enqueue_scripts', 'themisdb_matrix_enqueue_assets');

/**
 * Plugin activation
 */
function themisdb_matrix_activate() {
    // Set default options
    $defaults = array(
        'default_category' => 'all',
        'default_view' => 'all',
        'default_style' => 'modern',
        'show_legend' => 1,
        'enable_filtering' => 1,
        'enable_filters' => 'yes',
        'enable_sorting' => 1,
        'sticky_header' => 1,
        'highlight_themis' => 1,
        'show_themis_highlight' => 'yes',
        'enable_export' => 1,
        'enable_csv_export' => 'yes',
        'enable_tooltips' => 'yes',
        'export_prefix' => 'themisdb-comparison'
    );
    
    foreach ($defaults as $key => $value) {
        $option_key = 'themisdb_fm_' . $key;
        // Also try with matrix prefix for backwards compatibility
        $matrix_option_key = 'themisdb_matrix_' . $key;
        
        if (get_option($option_key) === false && get_option($matrix_option_key) === false) {
            add_option($option_key, $value);
        }
    }
    
    // Flush rewrite rules
    flush_rewrite_rules();
}
register_activation_hook(__FILE__, 'themisdb_matrix_activate');

/**
 * AJAX handler to get feature data
 */
function themisdb_matrix_ajax_get_features() {
    check_ajax_referer('themisdb_fm_nonce', 'nonce');
    
    $category = isset($_POST['category']) ? sanitize_text_field($_POST['category']) : 'all';
    
    // Get features from data class
    $features = ThemisDB_Feature_Matrix_Data::get_flat_features($category);
    $databases = ThemisDB_Feature_Matrix_Data::get_databases();
    
    wp_send_json_success(array(
        'features' => $features,
        'databases' => array_keys($databases),
        'database_info' => $databases,
        'category' => $category
    ));
}
add_action('wp_ajax_themisdb_fm_get_features', 'themisdb_matrix_ajax_get_features');
add_action('wp_ajax_nopriv_themisdb_fm_get_features', 'themisdb_matrix_ajax_get_features');

/**
 * Get color scheme preference
 *
 * @return string 'light' or 'dark'
 */
function themisdb_matrix_get_color_scheme() {
    if (isset($_COOKIE['themisdb_color_scheme'])) {
        return sanitize_text_field($_COOKIE['themisdb_color_scheme']);
    }
    return 'light';
}
