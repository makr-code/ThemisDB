<?php
/**
 * Plugin Name: ThemisDB Graph Pattern Visualizer
 * Plugin URI: https://github.com/makr-code/ThemisDB
 * Description: Interactive graph pattern visualization with filtering, searching, and color-coded node groups. Inspired by Neo4j Bloom. Use shortcode [themisdb_graph] to embed.
 * Version: 1.0.0
 * Author: ThemisDB Team
 * Author URI: https://github.com/makr-code/ThemisDB
 * License: MIT
 * License URI: https://opensource.org/licenses/MIT
 * Text Domain: themisdb-graph-pattern
 * Domain Path: /languages
 * Requires at least: 5.0
 * Requires PHP: 7.4
 */

// Exit if accessed directly
if (!defined('ABSPATH')) {
    exit;
}

// Define plugin constants
define('THEMISDB_GP_VERSION', '1.0.0');
define('THEMISDB_GP_PLUGIN_DIR', plugin_dir_path(__FILE__));
define('THEMISDB_GP_PLUGIN_URL', plugin_dir_url(__FILE__));
define('THEMISDB_GP_PLUGIN_FILE', __FILE__);
define('THEMISDB_GP_GITHUB_REPO', 'makr-code/ThemisDB');

/**
 * Main Plugin Class
 */
class ThemisDB_Graph_Pattern {
    
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
        add_shortcode('themisdb_graph', array($this, 'render_graph'));
        
        // Admin menu
        add_action('admin_menu', array($this, 'add_admin_menu'));
        add_action('admin_init', array($this, 'register_settings'));
        add_action('admin_enqueue_scripts', array($this, 'enqueue_admin_assets'));
        
        // Plugin action links
        add_filter('plugin_action_links_' . plugin_basename(THEMISDB_GP_PLUGIN_FILE), array($this, 'add_action_links'));
        
        // AJAX endpoints
        add_action('wp_ajax_themisdb_gp_get_graph_data', array($this, 'ajax_get_graph_data'));
        add_action('wp_ajax_nopriv_themisdb_gp_get_graph_data', array($this, 'ajax_get_graph_data'));
        add_action('wp_ajax_themisdb_gp_save_layout', array($this, 'ajax_save_layout'));
    }
    
    /**
     * Plugin activation
     */
    public function activate() {
        // Set default options
        $defaults = array(
            'default_layout' => 'force_directed',
            'node_color_scheme' => 'category',
            'enable_physics' => true,
            'show_labels' => true,
            'edge_smooth' => true,
            'max_nodes' => 500,
            'enable_search' => true,
            'enable_filters' => true,
            'enable_export' => true,
        );
        
        foreach ($defaults as $key => $value) {
            if (get_option('themisdb_gp_' . $key) === false) {
                add_option('themisdb_gp_' . $key, $value);
            }
        }
    }
    
    /**
     * Plugin deactivation
     */
    public function deactivate() {
        // Clean up transients
        delete_transient('themisdb_gp_cached_graph_data');
    }
    
    /**
     * Initialize plugin
     */
    public function init() {
        // Load text domain
        load_plugin_textdomain('themisdb-graph-pattern', false, dirname(plugin_basename(__FILE__)) . '/languages');
    }
    
    /**
     * Enqueue assets
     */
    public function enqueue_assets() {
        global $post;
        
        // Only load if shortcode is present
        if (!is_a($post, 'WP_Post') || !has_shortcode($post->post_content, 'themisdb_graph')) {
            return;
        }
        
        // Vis-network.js from CDN
        wp_enqueue_script(
            'vis-network-js',
            'https://unpkg.com/vis-network@9.1.2/standalone/umd/vis-network.min.js',
            array(),
            '9.1.2',
            true
        );
        
        // Plugin CSS
        wp_enqueue_style(
            'themisdb-gp-style',
            THEMISDB_GP_PLUGIN_URL . 'assets/css/graph-pattern.css',
            array(),
            THEMISDB_GP_VERSION
        );
        
        // Plugin JS
        wp_enqueue_script(
            'themisdb-gp-script',
            THEMISDB_GP_PLUGIN_URL . 'assets/js/graph-pattern.js',
            array('jquery', 'vis-network-js'),
            THEMISDB_GP_VERSION,
            true
        );
        
        // Localize script with AJAX URL and settings
        wp_localize_script('themisdb-gp-script', 'themisdbGP', array(
            'ajax_url' => admin_url('admin-ajax.php'),
            'nonce' => wp_create_nonce('themisdb_gp_nonce'),
            'plugin_url' => THEMISDB_GP_PLUGIN_URL,
            'settings' => array(
                'default_layout' => get_option('themisdb_gp_default_layout', 'force_directed'),
                'node_color_scheme' => get_option('themisdb_gp_node_color_scheme', 'category'),
                'enable_physics' => get_option('themisdb_gp_enable_physics', true),
                'show_labels' => get_option('themisdb_gp_show_labels', true),
                'edge_smooth' => get_option('themisdb_gp_edge_smooth', true),
                'max_nodes' => get_option('themisdb_gp_max_nodes', 500),
                'enable_search' => get_option('themisdb_gp_enable_search', true),
                'enable_filters' => get_option('themisdb_gp_enable_filters', true),
                'enable_export' => get_option('themisdb_gp_enable_export', true),
            ),
        ));
    }
    
    /**
     * Enqueue admin assets
     */
    public function enqueue_admin_assets($hook) {
        // Only load on our settings page
        if ('settings_page_themisdb-gp-settings' !== $hook) {
            return;
        }
        
        wp_enqueue_style(
            'themisdb-gp-admin-style',
            THEMISDB_GP_PLUGIN_URL . 'assets/css/admin.css',
            array(),
            THEMISDB_GP_VERSION
        );
    }
    
    /**
     * Render graph visualization
     */
    public function render_graph($atts) {
        $atts = shortcode_atts(array(
            'data_source' => 'default',
            'layout' => get_option('themisdb_gp_default_layout', 'force_directed'),
            'height' => '600px',
            'show_controls' => 'true',
            'show_overlay' => 'true',
        ), $atts, 'themisdb_graph');
        
        // Load template
        ob_start();
        include THEMISDB_GP_PLUGIN_DIR . 'templates/graph.php';
        return ob_get_clean();
    }
    
    /**
     * Add admin menu
     */
    public function add_admin_menu() {
        add_options_page(
            __('Graph Pattern Settings', 'themisdb-graph-pattern'),
            __('Graph Pattern', 'themisdb-graph-pattern'),
            'manage_options',
            'themisdb-gp-settings',
            array($this, 'render_settings_page')
        );
    }
    
    /**
     * Register settings
     */
    public function register_settings() {
        register_setting('themisdb_gp_settings', 'themisdb_gp_default_layout');
        register_setting('themisdb_gp_settings', 'themisdb_gp_node_color_scheme');
        register_setting('themisdb_gp_settings', 'themisdb_gp_enable_physics');
        register_setting('themisdb_gp_settings', 'themisdb_gp_show_labels');
        register_setting('themisdb_gp_settings', 'themisdb_gp_edge_smooth');
        register_setting('themisdb_gp_settings', 'themisdb_gp_max_nodes');
        register_setting('themisdb_gp_settings', 'themisdb_gp_enable_search');
        register_setting('themisdb_gp_settings', 'themisdb_gp_enable_filters');
        register_setting('themisdb_gp_settings', 'themisdb_gp_enable_export');
    }
    
    /**
     * Render settings page
     */
    public function render_settings_page() {
        if (!current_user_can('manage_options')) {
            return;
        }
        
        include THEMISDB_GP_PLUGIN_DIR . 'templates/admin-settings.php';
    }
    
    /**
     * Add plugin action links
     */
    public function add_action_links($links) {
        $settings_link = '<a href="' . admin_url('options-general.php?page=themisdb-gp-settings') . '">' . __('Settings', 'themisdb-graph-pattern') . '</a>';
        array_unshift($links, $settings_link);
        return $links;
    }
    
    /**
     * AJAX handler to get graph data
     */
    public function ajax_get_graph_data() {
        check_ajax_referer('themisdb_gp_nonce', 'nonce');
        
        $data_source = isset($_POST['data_source']) ? sanitize_text_field($_POST['data_source']) : 'default';
        
        // Get graph data
        $graph_data = $this->get_graph_data($data_source);
        
        wp_send_json_success(array(
            'nodes' => $graph_data['nodes'],
            'edges' => $graph_data['edges'],
            'groups' => $graph_data['groups'],
        ));
    }
    
    /**
     * AJAX handler to save custom layout
     */
    public function ajax_save_layout() {
        check_ajax_referer('themisdb_gp_nonce', 'nonce');
        
        if (!current_user_can('edit_posts')) {
            wp_send_json_error(array('message' => 'Insufficient permissions'));
            return;
        }
        
        $layout_data = isset($_POST['layout_data']) ? json_decode(stripslashes($_POST['layout_data']), true) : array();
        $user_id = get_current_user_id();
        
        // Save layout per user
        update_user_meta($user_id, 'themisdb_gp_custom_layout', $layout_data);
        
        wp_send_json_success(array('message' => 'Layout saved successfully'));
    }
    
    /**
     * Get graph data for visualization
     */
    private function get_graph_data($data_source = 'default') {
        // For now, return sample ThemisDB architecture graph
        // In production, this could load from various sources
        
        $nodes = array(
            array('id' => 1, 'label' => 'Client Layer', 'group' => 'client', 'level' => 1),
            array('id' => 2, 'label' => 'REST API', 'group' => 'api', 'level' => 1),
            array('id' => 3, 'label' => 'gRPC API', 'group' => 'api', 'level' => 1),
            array('id' => 4, 'label' => 'CLI', 'group' => 'client', 'level' => 1),
            
            array('id' => 10, 'label' => 'Query Layer', 'group' => 'query', 'level' => 2, 'size' => 30),
            array('id' => 11, 'label' => 'AQL Parser', 'group' => 'query', 'level' => 2),
            array('id' => 12, 'label' => 'Optimizer', 'group' => 'query', 'level' => 2),
            array('id' => 13, 'label' => 'Executor', 'group' => 'query', 'level' => 2),
            array('id' => 14, 'label' => 'LLM Functions', 'group' => 'llm', 'level' => 2),
            
            array('id' => 20, 'label' => 'Transaction Layer', 'group' => 'transaction', 'level' => 3),
            array('id' => 21, 'label' => 'MVCC', 'group' => 'transaction', 'level' => 3),
            array('id' => 22, 'label' => 'WAL', 'group' => 'transaction', 'level' => 3),
            
            array('id' => 30, 'label' => 'Index Layer', 'group' => 'index', 'level' => 4),
            array('id' => 31, 'label' => 'Vector Index', 'group' => 'index', 'level' => 4),
            array('id' => 32, 'label' => 'Graph Index', 'group' => 'index', 'level' => 4),
            array('id' => 33, 'label' => 'Full-Text Index', 'group' => 'index', 'level' => 4),
            
            array('id' => 40, 'label' => 'Storage Layer', 'group' => 'storage', 'level' => 5, 'size' => 30),
            array('id' => 41, 'label' => 'RocksDB', 'group' => 'storage', 'level' => 5),
            array('id' => 42, 'label' => 'Compression', 'group' => 'storage', 'level' => 5),
            array('id' => 43, 'label' => 'Snapshot', 'group' => 'storage', 'level' => 5),
        );
        
        $edges = array(
            array('from' => 1, 'to' => 2, 'label' => 'HTTP'),
            array('from' => 1, 'to' => 3, 'label' => 'gRPC'),
            array('from' => 4, 'to' => 2, 'label' => 'CLI'),
            
            array('from' => 2, 'to' => 10),
            array('from' => 3, 'to' => 10),
            array('from' => 10, 'to' => 11),
            array('from' => 11, 'to' => 12),
            array('from' => 12, 'to' => 13),
            array('from' => 13, 'to' => 14, 'dashes' => true, 'label' => 'optional'),
            
            array('from' => 13, 'to' => 20),
            array('from' => 20, 'to' => 21),
            array('from' => 21, 'to' => 22),
            
            array('from' => 13, 'to' => 30),
            array('from' => 30, 'to' => 31),
            array('from' => 30, 'to' => 32),
            array('from' => 30, 'to' => 33),
            
            array('from' => 31, 'to' => 40),
            array('from' => 32, 'to' => 40),
            array('from' => 33, 'to' => 40),
            array('from' => 40, 'to' => 41),
            array('from' => 41, 'to' => 42),
            array('from' => 41, 'to' => 43),
        );
        
        $groups = array(
            array('id' => 'client', 'label' => 'Client Components', 'color' => '#3498db', 'visible' => true),
            array('id' => 'api', 'label' => 'API Layer', 'color' => '#9b59b6', 'visible' => true),
            array('id' => 'query', 'label' => 'Query Layer', 'color' => '#2ea44f', 'visible' => true),
            array('id' => 'llm', 'label' => 'LLM Components', 'color' => '#e67e22', 'visible' => true),
            array('id' => 'transaction', 'label' => 'Transaction Layer', 'color' => '#e74c3c', 'visible' => true),
            array('id' => 'index', 'label' => 'Index Layer', 'color' => '#27ae60', 'visible' => true),
            array('id' => 'storage', 'label' => 'Storage Layer', 'color' => '#34495e', 'visible' => true),
        );
        
        return array(
            'nodes' => $nodes,
            'edges' => $edges,
            'groups' => $groups,
        );
    }
}

// Initialize plugin
function themisdb_graph_pattern_init() {
    return ThemisDB_Graph_Pattern::get_instance();
}

add_action('plugins_loaded', 'themisdb_graph_pattern_init');
