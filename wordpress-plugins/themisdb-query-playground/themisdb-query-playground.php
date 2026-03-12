<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            themisdb-query-playground.php                      ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:20                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     478                                            ║
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
 * Plugin Name: ThemisDB Query Playground
 * Plugin URI: https://github.com/makr-code/wordpressPlugins
 * Description: Interactive AQL query playground for ThemisDB. Execute queries, view results, and explore query execution plans. Use shortcode [themisdb_query_playground] to embed.
 * Version: 1.0.0
 * Author: ThemisDB Team
 * Author URI: https://github.com/makr-code/wordpressPlugins
 * License: MIT
 * License URI: https://opensource.org/licenses/MIT
 * Text Domain: themisdb-query-playground
 * Domain Path: /languages
 * Requires at least: 5.0
 * Requires PHP: 7.4
 */

// Exit if accessed directly
if (!defined('ABSPATH')) {
    exit;
}

// Define plugin constants
define('THEMISDB_QP_VERSION', '1.0.0');
define('THEMISDB_QP_PLUGIN_DIR', plugin_dir_path(__FILE__));
define('THEMISDB_QP_PLUGIN_URL', plugin_dir_url(__FILE__));
define('THEMISDB_QP_PLUGIN_FILE', __FILE__);
define('THEMISDB_QP_GITHUB_REPO', 'makr-code/wordpressPlugins');
define('THEMISDB_QP_GITHUB_PATH', 'tools/query-playground-wordpress');

// Load updater class
$themisdb_updater_local = THEMISDB_QP_PLUGIN_DIR . 'includes/class-themisdb-plugin-updater.php';
$themisdb_updater_shared = dirname(THEMISDB_QP_PLUGIN_DIR) . '/includes/class-themisdb-plugin-updater.php';

if (file_exists($themisdb_updater_local)) {
    require_once $themisdb_updater_local;
} elseif (file_exists($themisdb_updater_shared)) {
    require_once $themisdb_updater_shared;
}

// Initialize automatic updates
if (class_exists('ThemisDB_Plugin_Updater')) {
    new ThemisDB_Plugin_Updater(
        THEMISDB_QP_PLUGIN_FILE,
        'themisdb-query-playground',
        THEMISDB_QP_VERSION
    );
}

// Load ThemisDB PHP Client (if not already loaded via composer)
if (!class_exists('ThemisDB\\ThemisClient')) {
    // Provide path to ThemisDB PHP client
    $themisdb_client_path = get_option('themisdb_qp_client_path', '');
    if (!empty($themisdb_client_path) && file_exists($themisdb_client_path . '/vendor/autoload.php')) {
        require_once $themisdb_client_path . '/vendor/autoload.php';
    }
}

/**
 * Main Plugin Class
 */
class ThemisDB_Query_Playground {
    
    /**
     * Plugin instance
     */
    private static $instance = null;
    
    /**
     * ThemisDB client instance
     */
    private $client = null;
    
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
        add_shortcode('themisdb_query_playground', array($this, 'render_playground'));
        
        // Admin menu
        add_action('admin_menu', array($this, 'add_admin_menu'));
        add_action('admin_init', array($this, 'register_settings'));
        
        // Plugin action links
        add_filter('plugin_action_links_' . plugin_basename(THEMISDB_QP_PLUGIN_FILE), array($this, 'add_action_links'));
        
        // AJAX endpoints
        add_action('wp_ajax_themisdb_qp_execute_query', array($this, 'ajax_execute_query'));
        add_action('wp_ajax_nopriv_themisdb_qp_execute_query', array($this, 'ajax_execute_query'));
        add_action('wp_ajax_themisdb_qp_get_examples', array($this, 'ajax_get_examples'));
        add_action('wp_ajax_nopriv_themisdb_qp_get_examples', array($this, 'ajax_get_examples'));
    }
    
    /**
     * Plugin activation
     */
    public function activate() {
        // Set default options
        $defaults = array(
            'endpoint' => 'http://localhost:8080',
            'namespace' => 'default',
            'timeout' => 30,
            'enable_execution' => true,
            'max_results' => 100,
            'enable_examples' => true,
            'theme' => 'monokai',
            'client_path' => '',
            'read_only_mode' => true,
        );
        
        foreach ($defaults as $key => $value) {
            if (get_option('themisdb_qp_' . $key) === false) {
                add_option('themisdb_qp_' . $key, $value);
            }
        }
    }
    
    /**
     * Plugin deactivation
     */
    public function deactivate() {
        // Clean up transients
        delete_transient('themisdb_qp_cached_examples');
    }
    
    /**
     * Initialize plugin
     */
    public function init() {
        // Load text domain
        load_plugin_textdomain('themisdb-query-playground', false, dirname(plugin_basename(__FILE__)) . '/languages');
        
        // Initialize ThemisDB client
        $this->init_client();
    }
    
    /**
     * Initialize ThemisDB client
     */
    private function init_client() {
        if (!class_exists('ThemisDB\\ThemisClient')) {
            return;
        }
        
        try {
            $endpoint = get_option('themisdb_qp_endpoint', 'http://localhost:8080');
            $namespace = get_option('themisdb_qp_namespace', 'default');
            $timeout = (float) get_option('themisdb_qp_timeout', 30);
            
            $this->client = new \ThemisDB\ThemisClient(
                [$endpoint],
                [
                    'namespace' => $namespace,
                    'timeout' => $timeout,
                ]
            );
        } catch (Exception $e) {
            error_log('ThemisDB Query Playground: Failed to initialize client - ' . $e->getMessage());
        }
    }
    
    /**
     * Enqueue assets
     */
    public function enqueue_assets() {
        global $post;
        
        // Only load if shortcode is present
        if (!is_a($post, 'WP_Post') || !has_shortcode($post->post_content, 'themisdb_query_playground')) {
            return;
        }
        
        // CodeMirror from CDN for AQL editor
        wp_enqueue_style(
            'codemirror-css',
            'https://cdnjs.cloudflare.com/ajax/libs/codemirror/5.65.2/codemirror.min.css',
            array(),
            '5.65.2'
        );
        
        wp_enqueue_style(
            'codemirror-theme',
            'https://cdnjs.cloudflare.com/ajax/libs/codemirror/5.65.2/theme/monokai.min.css',
            array('codemirror-css'),
            '5.65.2'
        );
        
        wp_enqueue_script(
            'codemirror-js',
            'https://cdnjs.cloudflare.com/ajax/libs/codemirror/5.65.2/codemirror.min.js',
            array(),
            '5.65.2',
            true
        );
        
        wp_enqueue_script(
            'codemirror-sql',
            'https://cdnjs.cloudflare.com/ajax/libs/codemirror/5.65.2/mode/sql/sql.min.js',
            array('codemirror-js'),
            '5.65.2',
            true
        );
        
        // Plugin CSS
        wp_enqueue_style(
            'themisdb-qp-style',
            THEMISDB_QP_PLUGIN_URL . 'assets/css/query-playground.css',
            array(),
            THEMISDB_QP_VERSION
        );
        
        // Plugin JS
        wp_enqueue_script(
            'themisdb-qp-script',
            THEMISDB_QP_PLUGIN_URL . 'assets/js/query-playground.js',
            array('jquery', 'codemirror-js', 'codemirror-sql'),
            THEMISDB_QP_VERSION,
            true
        );
        
        // Localize script
        wp_localize_script('themisdb-qp-script', 'themisdbQP', array(
            'ajax_url' => admin_url('admin-ajax.php'),
            'nonce' => wp_create_nonce('themisdb_qp_nonce'),
            'plugin_url' => THEMISDB_QP_PLUGIN_URL,
            'settings' => array(
                'theme' => get_option('themisdb_qp_theme', 'monokai'),
                'enable_execution' => get_option('themisdb_qp_enable_execution', true),
                'read_only_mode' => get_option('themisdb_qp_read_only_mode', true),
            ),
        ));
    }
    
    /**
     * Render query playground
     */
    public function render_playground($atts) {
        $atts = shortcode_atts(array(
            'height' => '500px',
            'theme' => get_option('themisdb_qp_theme', 'monokai'),
            'default_query' => '',
        ), $atts, 'themisdb_query_playground');
        
        // Check if client is available
        $client_available = ($this->client !== null);
        
        // Load template
        ob_start();
        include THEMISDB_QP_PLUGIN_DIR . 'templates/playground.php';
        return ob_get_clean();
    }
    
    /**
     * Add admin menu
     */
    public function add_admin_menu() {
        add_options_page(
            __('Query Playground Settings', 'themisdb-query-playground'),
            __('Query Playground', 'themisdb-query-playground'),
            'manage_options',
            'themisdb-qp-settings',
            array($this, 'render_settings_page')
        );
    }
    
    /**
     * Register settings
     */
    public function register_settings() {
        register_setting('themisdb_qp_settings', 'themisdb_qp_endpoint');
        register_setting('themisdb_qp_settings', 'themisdb_qp_namespace');
        register_setting('themisdb_qp_settings', 'themisdb_qp_timeout');
        register_setting('themisdb_qp_settings', 'themisdb_qp_enable_execution');
        register_setting('themisdb_qp_settings', 'themisdb_qp_max_results');
        register_setting('themisdb_qp_settings', 'themisdb_qp_enable_examples');
        register_setting('themisdb_qp_settings', 'themisdb_qp_theme');
        register_setting('themisdb_qp_settings', 'themisdb_qp_client_path');
        register_setting('themisdb_qp_settings', 'themisdb_qp_read_only_mode');
    }
    
    /**
     * Render settings page
     */
    public function render_settings_page() {
        if (!current_user_can('manage_options')) {
            return;
        }
        
        include THEMISDB_QP_PLUGIN_DIR . 'templates/admin-settings.php';
    }
    
    /**
     * Add plugin action links
     */
    public function add_action_links($links) {
        $settings_link = '<a href="' . admin_url('options-general.php?page=themisdb-qp-settings') . '">' . __('Settings', 'themisdb-query-playground') . '</a>';
        array_unshift($links, $settings_link);
        return $links;
    }
    
    /**
     * AJAX handler to execute query
     */
    public function ajax_execute_query() {
        check_ajax_referer('themisdb_qp_nonce', 'nonce');
        
        // Check user capability - only editors and above can execute queries
        if (!current_user_can('edit_posts')) {
            wp_send_json_error(array('message' => 'Insufficient permissions to execute queries'));
            return;
        }
        
        if (!get_option('themisdb_qp_enable_execution', true)) {
            wp_send_json_error(array('message' => 'Query execution is disabled'));
            return;
        }
        
        if ($this->client === null) {
            wp_send_json_error(array('message' => 'ThemisDB client is not initialized'));
            return;
        }
        
        // Use wp_unslash instead of stripslashes for WordPress best practices
        // We don't sanitize the query as it needs to preserve SQL/AQL syntax
        // The database client should handle injection prevention
        $query = isset($_POST['query']) ? wp_unslash($_POST['query']) : '';
        
        if (empty($query)) {
            wp_send_json_error(array('message' => 'Query cannot be empty'));
            return;
        }
        
        // Check for read-only mode
        if (get_option('themisdb_qp_read_only_mode', true)) {
            if ($this->is_write_query($query)) {
                wp_send_json_error(array('message' => 'Write operations are disabled in read-only mode'));
                return;
            }
        }
        
        try {
            $start_time = microtime(true);
            $result = $this->client->query($query);
            $execution_time = (microtime(true) - $start_time) * 1000; // Convert to ms
            
            $max_results = (int) get_option('themisdb_qp_max_results', 100);
            $items = isset($result['items']) ? array_slice($result['items'], 0, $max_results) : [];
            
            wp_send_json_success(array(
                'items' => $items,
                'count' => count($items),
                'has_more' => isset($result['has_more']) ? $result['has_more'] : false,
                'execution_time' => round($execution_time, 2),
                'query' => $query,
            ));
        } catch (Exception $e) {
            wp_send_json_error(array(
                'message' => $e->getMessage(),
                'query' => $query,
            ));
        }
    }
    
    /**
     * AJAX handler to get example queries
     */
    public function ajax_get_examples() {
        check_ajax_referer('themisdb_qp_nonce', 'nonce');
        
        if (!get_option('themisdb_qp_enable_examples', true)) {
            wp_send_json_error(array('message' => 'Examples are disabled'));
            return;
        }
        
        $examples = $this->get_example_queries();
        wp_send_json_success($examples);
    }
    
    /**
     * Check if query is a write operation
     */
    private function is_write_query($query) {
        $write_keywords = array('INSERT', 'UPDATE', 'DELETE', 'DROP', 'CREATE', 'ALTER', 'TRUNCATE');
        $upper_query = strtoupper(trim($query));
        
        foreach ($write_keywords as $keyword) {
            if (strpos($upper_query, $keyword) === 0) {
                return true;
            }
        }
        
        return false;
    }
    
    /**
     * Get example queries
     */
    private function get_example_queries() {
        return array(
            array(
                'name' => 'Simple SELECT',
                'description' => 'Retrieve all users from the relational model',
                'query' => "SELECT * FROM urn:themis:relational:users LIMIT 10",
                'category' => 'basic',
            ),
            array(
                'name' => 'Vector Search',
                'description' => 'Find similar items using vector similarity',
                'query' => "SELECT * FROM urn:themis:vector:products\nWHERE VECTOR_SIMILARITY(embedding, [0.1, 0.2, 0.3]) > 0.8\nLIMIT 5",
                'category' => 'vector',
            ),
            array(
                'name' => 'Graph Traversal',
                'description' => 'Traverse relationships in graph model',
                'query' => "MATCH (user)-[:FRIEND_OF]->(friend)\nWHERE user.urn = 'urn:themis:graph:users:alice'\nRETURN friend",
                'category' => 'graph',
            ),
            array(
                'name' => 'Aggregation',
                'description' => 'Count and group results',
                'query' => "SELECT category, COUNT(*) as count\nFROM urn:themis:relational:products\nGROUP BY category\nORDER BY count DESC",
                'category' => 'analytics',
            ),
            array(
                'name' => 'JOIN Operation',
                'description' => 'Join data from multiple collections',
                'query' => "SELECT u.name, o.total\nFROM urn:themis:relational:users u\nJOIN urn:themis:relational:orders o ON u.uuid = o.user_id\nLIMIT 10",
                'category' => 'advanced',
            ),
            array(
                'name' => 'LLM Query',
                'description' => 'Use integrated LLM for natural language queries',
                'query' => "SELECT * FROM urn:themis:relational:documents\nWHERE LLM_SIMILARITY(content, 'artificial intelligence') > 0.7\nLIMIT 5",
                'category' => 'llm',
            ),
        );
    }
}

// Initialize plugin
function themisdb_query_playground_init() {
    return ThemisDB_Query_Playground::get_instance();
}

add_action('plugins_loaded', 'themisdb_query_playground_init');
