<?php
/**
 * Plugin Name: ThemisDB Feature Matrix
 * Plugin URI: https://github.com/makr-code/ThemisDB
 * Description: Interactive feature comparison matrix for ThemisDB vs competitors (PostgreSQL, MongoDB, Neo4j)
 * Version: 1.0.0
 * Author: ThemisDB Team
 * Author URI: https://github.com/makr-code/ThemisDB
 * License: MIT
 * Text Domain: themisdb-feature-matrix
 * Domain Path: /languages
 * Requires at least: 5.8
 * Requires PHP: 7.4
 */

if (!defined('ABSPATH')) {
    exit;
}

// Plugin constants
define('THEMISDB_MATRIX_VERSION', '1.0.0');
define('THEMISDB_MATRIX_DIR', plugin_dir_path(__FILE__));
define('THEMISDB_MATRIX_URL', plugin_dir_url(__FILE__));

// Include files
require_once THEMISDB_MATRIX_DIR . 'includes/class-feature-matrix.php';
require_once THEMISDB_MATRIX_DIR . 'includes/class-shortcode.php';
require_once THEMISDB_MATRIX_DIR . 'includes/class-admin.php';

// Initialize
function themisdb_matrix_init() {
    load_plugin_textdomain('themisdb-feature-matrix', false, dirname(plugin_basename(__FILE__)) . '/languages');
    
    new ThemisDB_Matrix_Shortcode();
    
    if (is_admin()) {
        new ThemisDB_Matrix_Admin();
    }
}
add_action('plugins_loaded', 'themisdb_matrix_init');

// Activation
function themisdb_matrix_activate() {
    $defaults = array(
        'default_category' => 'all',
        'default_style' => 'modern',
        'show_legend' => 1,
        'enable_filtering' => 1,
        'enable_sorting' => 1,
        'sticky_header' => 1,
        'highlight_themis' => 1,
        'enable_export' => 1,
        'export_prefix' => 'themisdb-comparison'
    );
    
    foreach ($defaults as $key => $value) {
        if (get_option('themisdb_matrix_' . $key) === false) {
            add_option('themisdb_matrix_' . $key, $value);
        }
    }
}
register_activation_hook(__FILE__, 'themisdb_matrix_activate');

// Enqueue assets
function themisdb_matrix_enqueue_assets() {
    global $post;
    
    if (!is_a($post, 'WP_Post') || !has_shortcode($post->post_content, 'themisdb_feature_matrix')) {
        return;
    }
    
    wp_enqueue_style(
        'themisdb-matrix-style',
        THEMISDB_MATRIX_URL . 'assets/css/feature-matrix.css',
        array(),
        THEMISDB_MATRIX_VERSION
    );
    
    $color_scheme = themisdb_matrix_get_color_scheme();
    if ($color_scheme === 'dark') {
        wp_enqueue_style(
            'themisdb-matrix-dark',
            THEMISDB_MATRIX_URL . 'assets/css/feature-matrix-dark.css',
            array('themisdb-matrix-style'),
            THEMISDB_MATRIX_VERSION
        );
    }
    
    wp_enqueue_script(
        'themisdb-matrix-script',
        THEMISDB_MATRIX_URL . 'assets/js/feature-matrix.js',
        array('jquery'),
        THEMISDB_MATRIX_VERSION,
        true
    );
    
    wp_localize_script('themisdb-matrix-script', 'themisdbMatrix', array(
        'ajaxUrl' => admin_url('admin-ajax.php'),
        'nonce' => wp_create_nonce('themisdb_matrix_nonce')
    ));
}
add_action('wp_enqueue_scripts', 'themisdb_matrix_enqueue_assets');

function themisdb_matrix_get_color_scheme() {
    if (isset($_COOKIE['themisdb_color_scheme'])) {
        return sanitize_text_field($_COOKIE['themisdb_color_scheme']);
    }
    return 'light';
}
