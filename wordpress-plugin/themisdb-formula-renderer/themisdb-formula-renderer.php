<?php
/**
 * Plugin Name: ThemisDB Formula Renderer
 * Plugin URI: https://github.com/makr-code/ThemisDB
 * Description: Rendert mathematische Formeln in LaTeX-Notation ($$...$$) in anzeigbare Formeln mit KaTeX. Unterstützt sowohl Inline- als auch Block-Formeln.
 * Version: 1.0.0
 * Author: ThemisDB Team
 * Author URI: https://github.com/makr-code/ThemisDB
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
        echo '<div class="error"><p><strong>ThemisDB Formula Renderer:</strong> Dieses Plugin benötigt PHP 7.2 oder höher. Sie verwenden PHP ' . PHP_VERSION . '</p></div>';
    });
    return;
}

// Plugin constants
define('THEMISDB_FORMULA_VERSION', '1.0.0');
define('THEMISDB_FORMULA_PLUGIN_DIR', plugin_dir_path(__FILE__));
define('THEMISDB_FORMULA_PLUGIN_URL', plugin_dir_url(__FILE__));

// Include required files
require_once THEMISDB_FORMULA_PLUGIN_DIR . 'includes/class-formula-renderer.php';
require_once THEMISDB_FORMULA_PLUGIN_DIR . 'includes/class-shortcodes.php';

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
 * Enqueue frontend scripts and styles
 */
function themisdb_formula_enqueue_scripts() {
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
    
    // Enqueue KaTeX Auto-render extension
    wp_enqueue_script(
        'katex-auto-render',
        'https://cdn.jsdelivr.net/npm/katex@0.16.9/dist/contrib/auto-render.min.js',
        array('katex-js'),
        '0.16.9',
        true
    );
    
    // Enqueue plugin custom script
    wp_enqueue_script(
        'themisdb-formula-script',
        THEMISDB_FORMULA_PLUGIN_URL . 'assets/js/script.js',
        array('jquery', 'katex-auto-render'),
        THEMISDB_FORMULA_VERSION,
        true
    );
    
    // Pass options to JavaScript
    $auto_render = get_option('themisdb_formula_auto_render', 1);
    wp_localize_script('themisdb-formula-script', 'themisdbFormula', array(
        'autoRender' => (bool) $auto_render,
        'inlineDelimiter' => get_option('themisdb_formula_inline_delimiter', '$'),
        'blockDelimiter' => get_option('themisdb_formula_block_delimiter', '$$')
    ));
}
add_action('wp_enqueue_scripts', 'themisdb_formula_enqueue_scripts');

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
