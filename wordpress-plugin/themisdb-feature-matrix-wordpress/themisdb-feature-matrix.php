<?php
/**
 * Plugin Name: ThemisDB Feature Matrix
 * Plugin URI: https://github.com/makr-code/ThemisDB
 * Description: Interactive feature comparison matrix for ThemisDB vs PostgreSQL, MongoDB, Neo4j
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

// Load required files
require_once THEMISDB_FM_PLUGIN_DIR . 'includes/class-feature-matrix.php';
require_once THEMISDB_FM_PLUGIN_DIR . 'includes/class-admin.php';

/**
 * Main Plugin Class
 */
class ThemisDB_Feature_Matrix {
    
    /**
     * Plugin instance
     */
    private static $instance = null;
    
    /**
     * Admin instance
     */
    private $admin = null;
    
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
        
        // Initialize admin
        if (is_admin()) {
            $this->admin = new ThemisDB_Feature_Matrix_Admin();
        }
        
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
            'default_view' => 'all',
            'default_style' => 'modern',
            'enable_filters' => 'yes',
            'enable_csv_export' => 'yes',
            'show_themis_highlight' => 'yes',
            'sticky_header' => 'yes',
            'enable_tooltips' => 'yes'
        );
        
        foreach ($defaults as $key => $value) {
            if (get_option('themisdb_fm_' . $key) === false) {
                add_option('themisdb_fm_' . $key, $value);
            }
        }
        
        // Flush rewrite rules
        flush_rewrite_rules();
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
    
    /**
     * Render feature matrix
     */
    public function render_matrix($atts) {
        $atts = shortcode_atts(array(
            'category' => get_option('themisdb_fm_default_view', 'all'),
            'style' => get_option('themisdb_fm_default_style', 'modern'),
            'highlight_themis' => get_option('themisdb_fm_show_themis_highlight', 'yes'),
            'sticky_header' => get_option('themisdb_fm_sticky_header', 'yes'),
            'filterable' => get_option('themisdb_fm_enable_filters', 'yes')
        ), $atts, 'themisdb_feature_matrix');
        
        // Load template
        ob_start();
        include THEMISDB_FM_PLUGIN_DIR . 'templates/matrix.php';
        return ob_get_clean();
    }
    
    /**
     * Add plugin action links
     */
    public function add_action_links($links) {
        $settings_link = '<a href="' . admin_url('options-general.php?page=themisdb-feature-matrix') . '">' . __('Settings', 'themisdb-feature-matrix') . '</a>';
        array_unshift($links, $settings_link);
        return $links;
    }
    
    /**
     * AJAX handler to get feature data
     */
    public function ajax_get_features() {
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
}

// Initialize plugin
function themisdb_feature_matrix_init() {
    return ThemisDB_Feature_Matrix::get_instance();
}

add_action('plugins_loaded', 'themisdb_feature_matrix_init');
