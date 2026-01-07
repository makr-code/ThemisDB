<?php
/**
 * Plugin Name: ThemisDB TCO Calculator
 * Plugin URI: https://github.com/makr-code/ThemisDB
 * Description: Total Cost of Ownership Calculator für ThemisDB - Vergleichen Sie die Gesamtbetriebskosten verschiedener Datenbanklösungen. Verwenden Sie den Shortcode [themisdb_tco_calculator] um den Rechner einzubinden.
 * Version: 1.0.0
 * Author: ThemisDB Team
 * Author URI: https://github.com/makr-code/ThemisDB
 * License: MIT
 * License URI: https://opensource.org/licenses/MIT
 * Text Domain: themisdb-tco-calculator
 * Domain Path: /languages
 * Requires at least: 5.0
 * Requires PHP: 7.4
 */

// Exit if accessed directly
if (!defined('ABSPATH')) {
    exit;
}

// Define plugin constants
define('THEMISDB_TCO_VERSION', '1.0.0');
define('THEMISDB_TCO_PLUGIN_DIR', plugin_dir_path(__FILE__));
define('THEMISDB_TCO_PLUGIN_URL', plugin_dir_url(__FILE__));

/**
 * Main Plugin Class
 */
class ThemisDB_TCO_Calculator {
    
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
        add_shortcode('themisdb_tco_calculator', array($this, 'render_calculator'));
        
        // Admin menu
        add_action('admin_menu', array($this, 'add_admin_menu'));
        add_action('admin_init', array($this, 'register_settings'));
    }
    
    /**
     * Plugin activation
     */
    public function activate() {
        // Set default options
        $defaults = array(
            'enable_ai_features' => true,
            'default_requests_per_day' => 1000000,
            'default_data_size' => 500,
            'default_peak_load' => 3,
            'default_availability' => 99.999,
        );
        
        foreach ($defaults as $key => $value) {
            if (get_option('themisdb_tco_' . $key) === false) {
                add_option('themisdb_tco_' . $key, $value);
            }
        }
    }
    
    /**
     * Plugin deactivation
     */
    public function deactivate() {
        // Cleanup if needed
    }
    
    /**
     * Initialize plugin
     */
    public function init() {
        // Load text domain for translations
        load_plugin_textdomain('themisdb-tco-calculator', false, dirname(plugin_basename(__FILE__)) . '/languages');
    }
    
    /**
     * Enqueue scripts and styles
     */
    public function enqueue_assets() {
        // Only load on pages with the shortcode
        global $post;
        if (is_a($post, 'WP_Post') && has_shortcode($post->post_content, 'themisdb_tco_calculator')) {
            // Enqueue Chart.js from CDN
            wp_enqueue_script(
                'chartjs',
                'https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.js',
                array(),
                '4.4.0',
                true
            );
            
            // Enqueue plugin CSS
            wp_enqueue_style(
                'themisdb-tco-calculator-styles',
                THEMISDB_TCO_PLUGIN_URL . 'assets/css/tco-calculator.css',
                array(),
                THEMISDB_TCO_VERSION
            );
            
            // Enqueue plugin JS
            wp_enqueue_script(
                'themisdb-tco-calculator-script',
                THEMISDB_TCO_PLUGIN_URL . 'assets/js/tco-calculator.js',
                array('chartjs'),
                THEMISDB_TCO_VERSION,
                true
            );
            
            // Pass WordPress settings to JavaScript
            wp_localize_script(
                'themisdb-tco-calculator-script',
                'themisdbTCO',
                array(
                    'ajaxUrl' => admin_url('admin-ajax.php'),
                    'nonce' => wp_create_nonce('themisdb_tco_nonce'),
                    'settings' => array(
                        'enableAI' => get_option('themisdb_tco_enable_ai_features', true),
                        'defaultRequestsPerDay' => get_option('themisdb_tco_default_requests_per_day', 1000000),
                        'defaultDataSize' => get_option('themisdb_tco_default_data_size', 500),
                        'defaultPeakLoad' => get_option('themisdb_tco_default_peak_load', 3),
                        'defaultAvailability' => get_option('themisdb_tco_default_availability', 99.999),
                    )
                )
            );
        }
    }
    
    /**
     * Render calculator HTML
     */
    public function render_calculator($atts) {
        // Parse shortcode attributes
        $atts = shortcode_atts(array(
            'title' => 'ThemisDB TCO-Rechner',
            'show_intro' => 'yes',
        ), $atts);
        
        // Start output buffering
        ob_start();
        
        // Include template
        include THEMISDB_TCO_PLUGIN_DIR . 'templates/calculator.php';
        
        // Return buffered content
        return ob_get_clean();
    }
    
    /**
     * Add admin menu
     */
    public function add_admin_menu() {
        add_options_page(
            __('ThemisDB TCO Calculator Einstellungen', 'themisdb-tco-calculator'),
            __('TCO Calculator', 'themisdb-tco-calculator'),
            'manage_options',
            'themisdb-tco-calculator',
            array($this, 'render_admin_page')
        );
    }
    
    /**
     * Register settings
     */
    public function register_settings() {
        register_setting('themisdb_tco_options', 'themisdb_tco_enable_ai_features');
        register_setting('themisdb_tco_options', 'themisdb_tco_default_requests_per_day');
        register_setting('themisdb_tco_options', 'themisdb_tco_default_data_size');
        register_setting('themisdb_tco_options', 'themisdb_tco_default_peak_load');
        register_setting('themisdb_tco_options', 'themisdb_tco_default_availability');
    }
    
    /**
     * Render admin page
     */
    public function render_admin_page() {
        if (!current_user_can('manage_options')) {
            return;
        }
        
        include THEMISDB_TCO_PLUGIN_DIR . 'templates/admin-settings.php';
    }
}

// Initialize plugin
ThemisDB_TCO_Calculator::get_instance();
