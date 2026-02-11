<?php
/**
 * Plugin Name: ThemisDB Taxonomy Manager
 * Plugin URI: https://github.com/makr-code/ThemisDB
 * Description: Manage custom taxonomies for ThemisDB features, use cases, industries, and technical specs
 * Version: 1.0.0
 * Author: ThemisDB Team
 * Author URI: https://github.com/makr-code/ThemisDB
 * License: MIT
 * Text Domain: themisdb-taxonomy
 * Domain Path: /languages
 * Requires at least: 5.8
 * Requires PHP: 7.4
 */

if (!defined('ABSPATH')) {
    exit;
}

define('THEMISDB_TAXONOMY_VERSION', '1.0.0');
define('THEMISDB_TAXONOMY_DIR', plugin_dir_path(__FILE__));
define('THEMISDB_TAXONOMY_URL', plugin_dir_url(__FILE__));

require_once THEMISDB_TAXONOMY_DIR . 'includes/class-taxonomy-manager.php';
require_once THEMISDB_TAXONOMY_DIR . 'includes/class-tree-view.php';
require_once THEMISDB_TAXONOMY_DIR . 'includes/class-widget.php';
require_once THEMISDB_TAXONOMY_DIR . 'includes/class-term-meta.php';
require_once THEMISDB_TAXONOMY_DIR . 'includes/class-seo.php';

function themisdb_taxonomy_init() {
    load_plugin_textdomain('themisdb-taxonomy', false, dirname(plugin_basename(__FILE__)) . '/languages');
    
    $taxonomy_manager = new ThemisDB_Taxonomy_Manager();
    $taxonomy_manager->register_taxonomies();
    
    if (is_admin()) {
        new ThemisDB_Taxonomy_Tree_View();
    }
    
    new ThemisDB_Taxonomy_SEO();
    new ThemisDB_Term_Meta();
}
add_action('init', 'themisdb_taxonomy_init');

function themisdb_taxonomy_enqueue_styles() {
    wp_enqueue_style('themisdb-taxonomy', THEMISDB_TAXONOMY_URL . 'assets/css/taxonomy-manager.css', array(), THEMISDB_TAXONOMY_VERSION);
    wp_enqueue_style('themisdb-widget', THEMISDB_TAXONOMY_URL . 'assets/css/widget.css', array(), THEMISDB_TAXONOMY_VERSION);
}
add_action('wp_enqueue_scripts', 'themisdb_taxonomy_enqueue_styles');

function themisdb_taxonomy_register_widget() {
    register_widget('ThemisDB_Taxonomy_Widget');
}
add_action('widgets_init', 'themisdb_taxonomy_register_widget');

function themisdb_taxonomy_activate() {
    $taxonomy_manager = new ThemisDB_Taxonomy_Manager();
    $taxonomy_manager->register_taxonomies();
    $taxonomy_manager->insert_default_terms();
    flush_rewrite_rules();
}
register_activation_hook(__FILE__, 'themisdb_taxonomy_activate');

function themisdb_taxonomy_deactivate() {
    flush_rewrite_rules();
}
register_deactivation_hook(__FILE__, 'themisdb_taxonomy_deactivate');
