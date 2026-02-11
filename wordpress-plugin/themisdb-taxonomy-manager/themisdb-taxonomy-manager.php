<?php
/**
 * Plugin Name: ThemisDB Taxonomy Manager
 * Plugin URI: https://github.com/makr-code/ThemisDB
 * Description: Shared intelligent taxonomy (categories and tags) manager for ThemisDB WordPress plugins. Extracts from both content and structure, supports hierarchical categories up to 3 levels, and minimizes redundant categories.
 * Version: 1.0.0
 * Author: ThemisDB Team
 * Author URI: https://github.com/makr-code/ThemisDB
 * License: MIT
 * Text Domain: themisdb-taxonomy-manager
 * Domain Path: /languages
 * Requires at least: 5.0
 * Requires PHP: 7.2
 */

// Prevent direct access
if (!defined('ABSPATH')) {
    exit;
}

// Plugin constants
define('THEMISDB_TAXONOMY_VERSION', '1.0.0');
define('THEMISDB_TAXONOMY_PLUGIN_DIR', plugin_dir_path(__FILE__));
define('THEMISDB_TAXONOMY_PLUGIN_URL', plugin_dir_url(__FILE__));

// Include required files
require_once THEMISDB_TAXONOMY_PLUGIN_DIR . 'includes/class-category-hierarchy.php';
require_once THEMISDB_TAXONOMY_PLUGIN_DIR . 'includes/class-taxonomy-extractor.php';
require_once THEMISDB_TAXONOMY_PLUGIN_DIR . 'includes/class-taxonomy-manager.php';
require_once THEMISDB_TAXONOMY_PLUGIN_DIR . 'includes/class-admin.php';

/**
 * Main Plugin Class
 */
class ThemisDB_Taxonomy_Manager_Plugin {
    
    /**
     * Singleton instance
     */
    private static $instance = null;
    
    /**
     * Taxonomy Manager instance
     */
    private $taxonomy_manager;
    
    /**
     * Get singleton instance
     */
    public static function get_instance() {
        if (self::$instance === null) {
            self::$instance = new self();
        }
        return self::$instance;
    }
    
    /**
     * Constructor
     */
    private function __construct() {
        // Initialize taxonomy manager
        $this->taxonomy_manager = new ThemisDB_Taxonomy_Manager();
        
        // Initialize admin panel
        if (is_admin()) {
            new ThemisDB_Taxonomy_Admin();
        }
        
        // Register activation/deactivation hooks
        register_activation_hook(__FILE__, array($this, 'activate'));
        register_deactivation_hook(__FILE__, array($this, 'deactivate'));
        
        // Load text domain
        add_action('plugins_loaded', array($this, 'load_textdomain'));
    }
    
    /**
     * Plugin activation
     */
    public function activate() {
        // Set default options
        add_option('themisdb_taxonomy_auto_extract', 1);
        add_option('themisdb_taxonomy_auto_tags', 1);
        add_option('themisdb_taxonomy_auto_categories', 1);
        add_option('themisdb_taxonomy_max_category_depth', 3);
        add_option('themisdb_taxonomy_min_category_posts', 2);
        add_option('themisdb_taxonomy_consolidate_categories', 1);
        
        // Flush rewrite rules
        flush_rewrite_rules();
    }
    
    /**
     * Plugin deactivation
     */
    public function deactivate() {
        // Flush rewrite rules
        flush_rewrite_rules();
    }
    
    /**
     * Load text domain
     */
    public function load_textdomain() {
        load_plugin_textdomain(
            'themisdb-taxonomy-manager',
            false,
            dirname(plugin_basename(__FILE__)) . '/languages'
        );
    }
    
    /**
     * Get taxonomy manager instance
     */
    public function get_taxonomy_manager() {
        return $this->taxonomy_manager;
    }
}

// Initialize plugin
ThemisDB_Taxonomy_Manager_Plugin::get_instance();

/**
 * Helper function to get plugin instance
 */
function themisdb_taxonomy_manager() {
    return ThemisDB_Taxonomy_Manager_Plugin::get_instance();
}

/**
 * Helper function to get taxonomy manager
 */
function themisdb_get_taxonomy_manager() {
    return themisdb_taxonomy_manager()->get_taxonomy_manager();
}
