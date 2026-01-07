<?php
/**
 * Plugin Name: ThemisDB Feature Matrix
 * Plugin URI: https://github.com/makr-code/ThemisDB
 * Description: Interactive feature comparison matrix for ThemisDB vs. competing databases. Visualize features, capabilities, and differences with Mermaid.js diagrams. Use shortcode [themisdb_feature_matrix] to embed.
 * Version: 1.0.0
 * Author: ThemisDB Team
 * Author URI: https://github.com/makr-code/ThemisDB
 * License: MIT
 * License URI: https://opensource.org/licenses/MIT
 * Text Domain: themisdb-feature-matrix
 * Domain Path: /languages
 * Requires at least: 5.0
 * Requires PHP: 7.4
 */

// Exit if accessed directly
if (!defined('ABSPATH')) {
    exit;
}

// Define plugin constants
define('THEMISDB_FM_VERSION', '1.0.0');
define('THEMISDB_FM_PLUGIN_DIR', plugin_dir_path(__FILE__));
define('THEMISDB_FM_PLUGIN_URL', plugin_dir_url(__FILE__));
define('THEMISDB_FM_PLUGIN_FILE', __FILE__);
define('THEMISDB_FM_GITHUB_REPO', 'makr-code/ThemisDB');
define('THEMISDB_FM_GITHUB_PATH', 'tools/feature-matrix-wordpress');

/**
 * Main Plugin Class
 */
class ThemisDB_Feature_Matrix {
    
    /**
     * Plugin instance
     */
    private static $instance = null;
    
    /**
     * Get plugin instance
     */
    public static function get_instance() {
        if (null === self::$instance) {
            self::$instance = new self();
        }
        return self::$instance;
    }
    
    /**
     * Constructor
     */
    private function __construct() {
        // Register activation and deactivation hooks
        register_activation_hook(__FILE__, array($this, 'activate'));
        register_deactivation_hook(__FILE__, array($this, 'deactivate'));
        
        // Initialize plugin
        add_action('init', array($this, 'init'));
        add_action('wp_enqueue_scripts', array($this, 'enqueue_assets'));
        
        // Register shortcode
        add_shortcode('themisdb_feature_matrix', array($this, 'render_matrix'));
        
        // Admin menu
        add_action('admin_menu', array($this, 'add_admin_menu'));
        add_action('admin_init', array($this, 'register_settings'));
        
        // Plugin action links
        add_filter('plugin_action_links_' . plugin_basename(THEMISDB_FM_PLUGIN_FILE), array($this, 'add_action_links'));
        
        // AJAX endpoints
        add_action('wp_ajax_themisdb_fm_get_features', array($this, 'ajax_get_features'));
        add_action('wp_ajax_nopriv_themisdb_fm_get_features', array($this, 'ajax_get_features'));
    }
    
    /**
     * Plugin activation
     */
    public function activate() {
        // Set default options
        $defaults = array(
            'data_source' => 'local',
            'default_comparison_dbs' => 'postgresql,mongodb,neo4j',
            'default_category' => 'all',
            'show_mermaid_diagrams' => true,
            'table_view' => 'detailed',
            'enable_tooltips' => true,
        );
        
        foreach ($defaults as $key => $value) {
            if (get_option('themisdb_fm_' . $key) === false) {
                add_option('themisdb_fm_' . $key, $value);
            }
        }
    }
    
    /**
     * Plugin deactivation
     */
    public function deactivate() {
        // Clean up transients
        delete_transient('themisdb_fm_cached_features');
    }
    
    /**
     * Initialize plugin
     */
    public function init() {
        // Load text domain
        load_plugin_textdomain('themisdb-feature-matrix', false, dirname(plugin_basename(__FILE__)) . '/languages');
    }
    
    /**
     * Enqueue assets
     */
    public function enqueue_assets() {
        global $post;
        
        // Only load if shortcode is present
        if (!is_a($post, 'WP_Post') || !has_shortcode($post->post_content, 'themisdb_feature_matrix')) {
            return;
        }
        
        // Mermaid.js from CDN (for diagrams)
        wp_enqueue_script(
            'mermaid-js',
            'https://cdn.jsdelivr.net/npm/mermaid@10/dist/mermaid.min.js',
            array(),
            '10.0.0',
            true
        );
        
        // Plugin CSS
        wp_enqueue_style(
            'themisdb-fm-style',
            THEMISDB_FM_PLUGIN_URL . 'assets/css/feature-matrix.css',
            array(),
            THEMISDB_FM_VERSION
        );
        
        // Plugin JS
        wp_enqueue_script(
            'themisdb-fm-script',
            THEMISDB_FM_PLUGIN_URL . 'assets/js/feature-matrix.js',
            array('jquery', 'mermaid-js'),
            THEMISDB_FM_VERSION,
            true
        );
        
        // Localize script with AJAX URL and settings
        wp_localize_script('themisdb-fm-script', 'themisdbFM', array(
            'ajax_url' => admin_url('admin-ajax.php'),
            'nonce' => wp_create_nonce('themisdb_fm_nonce'),
            'plugin_url' => THEMISDB_FM_PLUGIN_URL,
            'settings' => array(
                'default_category' => get_option('themisdb_fm_default_category', 'all'),
                'show_mermaid' => get_option('themisdb_fm_show_mermaid_diagrams', true),
                'table_view' => get_option('themisdb_fm_table_view', 'detailed'),
                'enable_tooltips' => get_option('themisdb_fm_enable_tooltips', true),
            ),
        ));
    }
    
    /**
     * Render feature matrix
     */
    public function render_matrix($atts) {
        $atts = shortcode_atts(array(
            'category' => get_option('themisdb_fm_default_category', 'all'),
            'compare' => get_option('themisdb_fm_default_comparison_dbs', 'postgresql,mongodb,neo4j'),
            'view' => get_option('themisdb_fm_table_view', 'detailed'),
            'show_diagram' => get_option('themisdb_fm_show_mermaid_diagrams', true),
        ), $atts, 'themisdb_feature_matrix');
        
        // Load template
        ob_start();
        include THEMISDB_FM_PLUGIN_DIR . 'templates/matrix.php';
        return ob_get_clean();
    }
    
    /**
     * Add admin menu
     */
    public function add_admin_menu() {
        add_options_page(
            __('Feature Matrix Settings', 'themisdb-feature-matrix'),
            __('Feature Matrix', 'themisdb-feature-matrix'),
            'manage_options',
            'themisdb-fm-settings',
            array($this, 'render_settings_page')
        );
    }
    
    /**
     * Register settings
     */
    public function register_settings() {
        register_setting('themisdb_fm_settings', 'themisdb_fm_data_source');
        register_setting('themisdb_fm_settings', 'themisdb_fm_default_comparison_dbs');
        register_setting('themisdb_fm_settings', 'themisdb_fm_default_category');
        register_setting('themisdb_fm_settings', 'themisdb_fm_show_mermaid_diagrams');
        register_setting('themisdb_fm_settings', 'themisdb_fm_table_view');
        register_setting('themisdb_fm_settings', 'themisdb_fm_enable_tooltips');
    }
    
    /**
     * Render settings page
     */
    public function render_settings_page() {
        if (!current_user_can('manage_options')) {
            return;
        }
        
        include THEMISDB_FM_PLUGIN_DIR . 'templates/admin-settings.php';
    }
    
    /**
     * Add plugin action links
     */
    public function add_action_links($links) {
        $settings_link = '<a href="' . admin_url('options-general.php?page=themisdb-fm-settings') . '">' . __('Settings', 'themisdb-feature-matrix') . '</a>';
        array_unshift($links, $settings_link);
        return $links;
    }
    
    /**
     * AJAX handler to get feature data
     */
    public function ajax_get_features() {
        check_ajax_referer('themisdb_fm_nonce', 'nonce');
        
        $category = isset($_POST['category']) ? sanitize_text_field($_POST['category']) : 'all';
        $compare = isset($_POST['compare']) ? sanitize_text_field($_POST['compare']) : '';
        
        // Check cache first
        $cache_key = 'themisdb_fm_features_' . md5($category . '_' . $compare);
        $cached_data = get_transient($cache_key);
        
        if ($cached_data !== false) {
            wp_send_json_success($cached_data);
            return;
        }
        
        // Load features
        $features = $this->load_features($category, $compare);
        
        // Cache data for 24 hours
        set_transient($cache_key, $features, 86400);
        
        wp_send_json_success($features);
    }
    
    /**
     * Load features data
     */
    private function load_features($category = 'all', $compare = '') {
        // Sample feature data structure
        // In production, this would load from JSON files or database
        $all_features = array(
            array(
                'name' => 'Multi-Model Support',
                'category' => 'architecture',
                'themisdb' => 'available',
                'postgresql' => 'limited',
                'mongodb' => 'partial',
                'neo4j' => 'not_available',
                'description' => 'Native support for document, graph, key-value, time-series, and vector data models in a single database',
            ),
            array(
                'name' => 'Native LLM Integration',
                'category' => 'ai_ml',
                'themisdb' => 'available',
                'postgresql' => 'not_available',
                'mongodb' => 'not_available',
                'neo4j' => 'not_available',
                'description' => 'Run LLaMA models directly in database with llama.cpp integration',
            ),
            array(
                'name' => 'Vector Search (HNSW)',
                'category' => 'ai_ml',
                'themisdb' => 'available',
                'postgresql' => 'available',
                'mongodb' => 'available',
                'neo4j' => 'limited',
                'description' => 'High-performance vector similarity search using HNSW algorithm',
            ),
            array(
                'name' => 'Graph Database',
                'category' => 'architecture',
                'themisdb' => 'available',
                'postgresql' => 'limited',
                'mongodb' => 'not_available',
                'neo4j' => 'available',
                'description' => 'Native graph data model with efficient traversal algorithms',
            ),
            array(
                'name' => 'ACID Transactions',
                'category' => 'reliability',
                'themisdb' => 'available',
                'postgresql' => 'available',
                'mongodb' => 'available',
                'neo4j' => 'available',
                'description' => 'Full ACID compliance for data consistency and integrity',
            ),
            array(
                'name' => 'Sharding & RAID',
                'category' => 'scalability',
                'themisdb' => 'available',
                'postgresql' => 'limited',
                'mongodb' => 'available',
                'neo4j' => 'available',
                'description' => 'Horizontal scaling with automatic sharding and RAID support',
            ),
            array(
                'name' => 'Field-Level Encryption',
                'category' => 'security',
                'themisdb' => 'available',
                'postgresql' => 'limited',
                'mongodb' => 'available',
                'neo4j' => 'limited',
                'description' => 'Encrypt sensitive data at the field level',
            ),
            array(
                'name' => 'AQL Query Language',
                'category' => 'usability',
                'themisdb' => 'available',
                'postgresql' => 'not_available',
                'mongodb' => 'not_available',
                'neo4j' => 'not_available',
                'description' => 'Powerful and intuitive Advanced Query Language',
            ),
        );
        
        // Filter by category if specified
        if ($category !== 'all') {
            $all_features = array_filter($all_features, function($feature) use ($category) {
                return $feature['category'] === $category;
            });
        }
        
        // Filter databases to compare
        $databases = array('themisdb');
        if (!empty($compare)) {
            $databases = array_merge($databases, explode(',', $compare));
        } else {
            $databases = array('themisdb', 'postgresql', 'mongodb', 'neo4j');
        }
        
        return array(
            'features' => array_values($all_features),
            'databases' => $databases,
            'category' => $category,
        );
    }
}

// Initialize plugin
function themisdb_feature_matrix_init() {
    return ThemisDB_Feature_Matrix::get_instance();
}

add_action('plugins_loaded', 'themisdb_feature_matrix_init');
