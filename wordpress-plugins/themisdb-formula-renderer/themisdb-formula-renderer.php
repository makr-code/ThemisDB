<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            themisdb-formula-renderer.php                      ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:18                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     253                                            ║
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
 * Plugin Name: ThemisDB Formula Renderer
 * Plugin URI: https://github.com/makr-code/wordpressPlugins
 * Description: Rendert mathematische Formeln in LaTeX-Notation ($$...$$) in anzeigbare Formeln mit KaTeX. Unterstützt sowohl Inline- als auch Block-Formeln.
 * Version: 1.1.0
 * Author: ThemisDB Team
 * Author URI: https://github.com/makr-code/wordpressPlugins
 * License: MIT
 * Text Domain: themisdb-formula-renderer
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
        echo '<div class="error"><p><strong>ThemisDB Formula Renderer:</strong> Dieses Plugin benötigt PHP 7.2 oder höher. Sie verwenden PHP ' . esc_html(PHP_VERSION) . '</p></div>';
    });
    return;
}

// Plugin constants
define('THEMISDB_FORMULA_VERSION', '1.1.0');
define('THEMISDB_FORMULA_PLUGIN_DIR', plugin_dir_path(__FILE__));
define('THEMISDB_FORMULA_PLUGIN_URL', plugin_dir_url(__FILE__));
define('THEMISDB_FORMULA_PLUGIN_FILE', __FILE__);

// Load updater class
$themisdb_updater_local = THEMISDB_FORMULA_PLUGIN_DIR . 'includes/class-themisdb-plugin-updater.php';
$themisdb_updater_shared = dirname(THEMISDB_FORMULA_PLUGIN_DIR) . '/includes/class-themisdb-plugin-updater.php';

if (file_exists($themisdb_updater_local)) {
    require_once $themisdb_updater_local;
} elseif (file_exists($themisdb_updater_shared)) {
    require_once $themisdb_updater_shared;
}

// Initialize automatic updates
if (class_exists('ThemisDB_Plugin_Updater')) {
    new ThemisDB_Plugin_Updater(
        THEMISDB_FORMULA_PLUGIN_FILE,
        'themisdb-formula-renderer',
        THEMISDB_FORMULA_VERSION
    );
}

// Include required files
require_once THEMISDB_FORMULA_PLUGIN_DIR . 'includes/class-formula-renderer.php';
require_once THEMISDB_FORMULA_PLUGIN_DIR . 'includes/class-shortcodes.php';
require_once THEMISDB_FORMULA_PLUGIN_DIR . 'includes/class-formula-library.php';

/**
 * Initialize the plugin
 */
function themisdb_formula_init() {
    // Initialize formula renderer
    new ThemisDB_Formula_Renderer();
    
    // Initialize shortcodes
    new ThemisDB_Formula_Shortcodes();
    
    // Load text domain for translations
    load_plugin_textdomain('themisdb-formula-renderer', false, dirname(plugin_basename(__FILE__)) . '/languages');
}
add_action('plugins_loaded', 'themisdb_formula_init');

/**
 * Activation hook
 */
function themisdb_formula_activate() {
    // Set default options
    if (get_option('themisdb_formula_auto_render') === false) {
        add_option('themisdb_formula_auto_render', 1); // Enabled by default
    }
    if (get_option('themisdb_formula_inline_delimiter') === false) {
        add_option('themisdb_formula_inline_delimiter', '$'); // Single $ for inline
    }
    if (get_option('themisdb_formula_block_delimiter') === false) {
        add_option('themisdb_formula_block_delimiter', '$$'); // Double $$ for block
    }
}
register_activation_hook(__FILE__, 'themisdb_formula_activate');

/**
 * Conditional script loading - only if formulas present
 */
function themisdb_formula_enqueue_scripts() {
    global $post;
    
    // Only load if formulas are present
    if (!is_a($post, 'WP_Post')) {
        return;
    }
    
    $has_formula = (
        // Check shortcodes
        has_shortcode($post->post_content, 'themisdb_formula') ||
        has_shortcode($post->post_content, 'formula') ||
        has_shortcode($post->post_content, 'latex') ||
        has_shortcode($post->post_content, 'math') ||
        // Check delimiters
        strpos($post->post_content, '$$') !== false ||
        (strpos($post->post_content, '$') !== false && preg_match('/\$[^\$]+\$/', $post->post_content))
    );
    
    if (!$has_formula) {
        return;
    }
    
    // Enqueue KaTeX CSS
    wp_enqueue_style(
        'katex-style',
        'https://cdn.jsdelivr.net/npm/katex@0.16.9/dist/katex.min.css',
        array(),
        '0.16.9'
    );
    
    // Enqueue plugin custom styles
    wp_enqueue_style(
        'themisdb-formula-style',
        THEMISDB_FORMULA_PLUGIN_URL . 'assets/css/style.css',
        array('katex-style'),
        THEMISDB_FORMULA_VERSION
    );
    
    // Enqueue KaTeX JS
    wp_enqueue_script(
        'katex-js',
        'https://cdn.jsdelivr.net/npm/katex@0.16.9/dist/katex.min.js',
        array(),
        '0.16.9',
        true
    );
    
    // Enqueue KaTeX auto-render
    wp_enqueue_script(
        'katex-auto-render',
        'https://cdn.jsdelivr.net/npm/katex@0.16.9/dist/contrib/auto-render.min.js',
        array('katex-js'),
        '0.16.9',
        true
    );
    
    // Enqueue plugin JavaScript
    wp_enqueue_script(
        'themisdb-formula-script',
        THEMISDB_FORMULA_PLUGIN_URL . 'assets/js/script.js',
        array('jquery', 'katex-auto-render'),
        THEMISDB_FORMULA_VERSION,
        true
    );
    
    // Localize script
    wp_localize_script('themisdb-formula-script', 'themisdbFormula', array(
        'autoRender' => get_option('themisdb_formula_auto_render', 1),
        'inlineDelimiter' => get_option('themisdb_formula_inline_delimiter', '$'),
        'blockDelimiter' => get_option('themisdb_formula_block_delimiter', '$$'),
    ));
}
add_action('wp_enqueue_scripts', 'themisdb_formula_enqueue_scripts');

/**
 * Add preload for KaTeX
 */
function themisdb_formula_add_preload() {
    global $post;
    
    if (!is_a($post, 'WP_Post')) {
        return;
    }
    
    $has_formula = (
        has_shortcode($post->post_content, 'themisdb_formula') ||
        strpos($post->post_content, '$$') !== false
    );
    
    if ($has_formula) {
        echo '<link rel="preload" href="' . esc_url('https://cdn.jsdelivr.net/npm/katex@0.16.9/dist/katex.min.css') . '" as="style">';
        echo '<link rel="preload" href="' . esc_url('https://cdn.jsdelivr.net/npm/katex@0.16.9/dist/katex.min.js') . '" as="script">';
    }
}
add_action('wp_head', 'themisdb_formula_add_preload', 1);

/**
 * Enqueue admin scripts and styles
 */
function themisdb_formula_admin_enqueue_scripts($hook) {
    // Only load on plugin settings page
    if ($hook !== 'settings_page_themisdb-formula-renderer') {
        return;
    }
    
    wp_enqueue_style(
        'katex-style',
        'https://cdn.jsdelivr.net/npm/katex@0.16.9/dist/katex.min.css',
        array(),
        '0.16.9'
    );
    
    wp_enqueue_script(
        'katex-js',
        'https://cdn.jsdelivr.net/npm/katex@0.16.9/dist/katex.min.js',
        array(),
        '0.16.9',
        true
    );
    
    wp_enqueue_script(
        'katex-auto-render',
        'https://cdn.jsdelivr.net/npm/katex@0.16.9/dist/contrib/auto-render.min.js',
        array('katex-js'),
        '0.16.9',
        true
    );
}
add_action('admin_enqueue_scripts', 'themisdb_formula_admin_enqueue_scripts');

/**
 * Add formula library admin menu
 */
function themisdb_formula_library_menu() {
    add_submenu_page(
        'options-general.php',
        __('Formula Library', 'themisdb-formula-renderer'),
        __('Formula Library', 'themisdb-formula-renderer'),
        'manage_options',
        'themisdb-formula-library',
        array(new ThemisDB_Formula_Library(), 'render_admin_page')
    );
}
add_action('admin_menu', 'themisdb_formula_library_menu');
