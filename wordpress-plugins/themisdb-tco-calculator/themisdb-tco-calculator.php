<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            themisdb-tco-calculator.php                        ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:22                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     539                                            ║
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
 * Plugin Name: ThemisDB TCO Calculator
 * Plugin URI: https://github.com/makr-code/wordpressPlugins
 * Description: Total Cost of Ownership Calculator für ThemisDB - Vergleichen Sie die Gesamtbetriebskosten verschiedener Datenbanklösungen. Verwenden Sie den Shortcode [themisdb_tco_calculator] um den Rechner einzubinden.
 * Version: 1.0.0
 * Author: ThemisDB Team
 * Author URI: https://github.com/makr-code/wordpressPlugins
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
define('THEMISDB_TCO_PLUGIN_FILE', __FILE__);
define('THEMISDB_TCO_GITHUB_REPO', 'makr-code/wordpressPlugins');
define('THEMISDB_TCO_GITHUB_PATH', 'tools/tco-calculator-wordpress');

// Load updater class
$themisdb_updater_local = THEMISDB_TCO_PLUGIN_DIR . 'includes/class-themisdb-plugin-updater.php';
$themisdb_updater_shared = dirname(THEMISDB_TCO_PLUGIN_DIR) . '/includes/class-themisdb-plugin-updater.php';

if (file_exists($themisdb_updater_local)) {
    require_once $themisdb_updater_local;
} elseif (file_exists($themisdb_updater_shared)) {
    require_once $themisdb_updater_shared;
}

// Initialize automatic updates
if (class_exists('ThemisDB_Plugin_Updater')) {
    new ThemisDB_Plugin_Updater(
        THEMISDB_TCO_PLUGIN_FILE,
        'themisdb-tco-calculator',
        THEMISDB_TCO_VERSION
    );
}

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
        
        // Register shortcodes
        add_shortcode('themisdb_tco_calculator', array($this, 'render_calculator'));
        add_shortcode('themisdb_tco_workload', array($this, 'render_workload_section'));
        add_shortcode('themisdb_tco_infrastructure', array($this, 'render_infrastructure_section'));
        add_shortcode('themisdb_tco_personnel', array($this, 'render_personnel_section'));
        add_shortcode('themisdb_tco_operations', array($this, 'render_operations_section'));
        add_shortcode('themisdb_tco_ai', array($this, 'render_ai_section'));
        add_shortcode('themisdb_tco_results', array($this, 'render_results_section'));
        
        // Admin menu
        add_action('admin_menu', array($this, 'add_admin_menu'));
        add_action('admin_init', array($this, 'register_settings'));
        
        // Plugin action links
        add_filter('plugin_action_links_' . plugin_basename(THEMISDB_TCO_PLUGIN_FILE), array($this, 'add_action_links'));
        
        // GitHub updates
        add_filter('pre_set_site_transient_update_plugins', array($this, 'check_for_updates'));
        add_filter('plugins_api', array($this, 'plugin_info'), 20, 3);
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
        // Only load on pages with any TCO shortcode
        global $post;
        $has_tco_shortcode = false;
        
        if (is_a($post, 'WP_Post')) {
            $shortcodes = array(
                'themisdb_tco_calculator',
                'themisdb_tco_workload',
                'themisdb_tco_infrastructure',
                'themisdb_tco_personnel',
                'themisdb_tco_operations',
                'themisdb_tco_ai',
                'themisdb_tco_results'
            );
            
            foreach ($shortcodes as $shortcode) {
                if (has_shortcode($post->post_content, $shortcode)) {
                    $has_tco_shortcode = true;
                    break;
                }
            }
        }
        
        if ($has_tco_shortcode) {
            // Enqueue Chart.js from CDN
            wp_enqueue_script(
                'chartjs',
                'https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.js',
                array(),
                '4.4.0',
                true
            );
            
            // Enqueue Mermaid.js from CDN
            wp_enqueue_script(
                'mermaidjs',
                'https://cdn.jsdelivr.net/npm/mermaid@10.6.1/dist/mermaid.min.js',
                array(),
                '10.6.1',
                true
            );
            
            // Initialize Mermaid
            wp_add_inline_script(
                'mermaidjs',
                'if (typeof mermaid !== "undefined") { mermaid.initialize({ startOnLoad: false, theme: "default", securityLevel: "strict" }); }'
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
                array('chartjs', 'mermaidjs'),
                THEMISDB_TCO_VERSION,
                true
            );
            
            // Pass WordPress settings to JavaScript
            wp_localize_script(
                'themisdb-tco-calculator-script',
                'themisdbTCO',
                array(
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
     * Render workload section
     */
    public function render_workload_section($atts) {
        $atts = shortcode_atts(array(
            'scale' => '1',
            'animation' => 'fade-in',
            'delay' => '0',
        ), $atts);
        
        ob_start();
        include THEMISDB_TCO_PLUGIN_DIR . 'templates/sections/workload.php';
        return ob_get_clean();
    }
    
    /**
     * Render infrastructure section
     */
    public function render_infrastructure_section($atts) {
        $atts = shortcode_atts(array(
            'scale' => '1',
            'animation' => 'fade-in',
            'delay' => '0',
        ), $atts);
        
        ob_start();
        include THEMISDB_TCO_PLUGIN_DIR . 'templates/sections/infrastructure.php';
        return ob_get_clean();
    }
    
    /**
     * Render personnel section
     */
    public function render_personnel_section($atts) {
        $atts = shortcode_atts(array(
            'scale' => '1',
            'animation' => 'fade-in',
            'delay' => '0',
        ), $atts);
        
        ob_start();
        include THEMISDB_TCO_PLUGIN_DIR . 'templates/sections/personnel.php';
        return ob_get_clean();
    }
    
    /**
     * Render operations section
     */
    public function render_operations_section($atts) {
        $atts = shortcode_atts(array(
            'scale' => '1',
            'animation' => 'fade-in',
            'delay' => '0',
        ), $atts);
        
        ob_start();
        include THEMISDB_TCO_PLUGIN_DIR . 'templates/sections/operations.php';
        return ob_get_clean();
    }
    
    /**
     * Render AI section
     */
    public function render_ai_section($atts) {
        $atts = shortcode_atts(array(
            'scale' => '1',
            'animation' => 'fade-in',
            'delay' => '0',
        ), $atts);
        
        ob_start();
        include THEMISDB_TCO_PLUGIN_DIR . 'templates/sections/ai.php';
        return ob_get_clean();
    }
    
    /**
     * Render results section
     */
    public function render_results_section($atts) {
        $atts = shortcode_atts(array(
            'scale' => '1',
            'animation' => 'fade-in',
            'delay' => '0',
        ), $atts);
        
        ob_start();
        include THEMISDB_TCO_PLUGIN_DIR . 'templates/sections/results.php';
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
    
    /**
     * Add action links to plugin page
     */
    public function add_action_links($links) {
        $settings_link = '<a href="' . esc_url(admin_url('options-general.php?page=themisdb-tco-calculator')) . '">' . 
                        __('Einstellungen', 'themisdb-tco-calculator') . '</a>';
        array_unshift($links, $settings_link);
        return $links;
    }
    
    /**
     * Check for plugin updates from GitHub
     */
    public function check_for_updates($transient) {
        if (empty($transient->checked)) {
            return $transient;
        }
        
        $plugin_slug = plugin_basename(THEMISDB_TCO_PLUGIN_FILE);
        
        // Get the latest release info from GitHub
        $remote_version = $this->get_github_release_info();
        
        if ($remote_version && version_compare(THEMISDB_TCO_VERSION, $remote_version->tag_name, '<')) {
            $plugin_data = array(
                'slug' => dirname($plugin_slug),
                'plugin' => $plugin_slug,
                'new_version' => $remote_version->tag_name,
                'url' => $remote_version->html_url,
                'package' => $this->get_github_download_url($remote_version->tag_name),
                'tested' => '6.7',
                'requires_php' => '7.4',
            );
            
            $transient->response[$plugin_slug] = (object) $plugin_data;
        }
        
        return $transient;
    }
    
    /**
     * Get plugin info for update details
     */
    public function plugin_info($false, $action, $response) {
        $plugin_slug = dirname(plugin_basename(THEMISDB_TCO_PLUGIN_FILE));
        
        if ($action !== 'plugin_information' || $response->slug !== $plugin_slug) {
            return $false;
        }
        
        $remote_version = $this->get_github_release_info();
        
        if ($remote_version) {
            $response->name = 'ThemisDB TCO Calculator';
            $response->slug = $plugin_slug;
            $response->version = $remote_version->tag_name;
            $response->author = '<a href="https://github.com/makr-code">ThemisDB Team</a>';
            $response->homepage = 'https://github.com/' . THEMISDB_TCO_GITHUB_REPO;
            $response->download_link = $this->get_github_download_url($remote_version->tag_name);
            $response->requires = '5.0';
            $response->tested = '6.7';
            $response->requires_php = '7.4';
            $response->sections = array(
                'description' => 'Total Cost of Ownership Calculator für ThemisDB',
                'changelog' => isset($remote_version->body) ? $remote_version->body : 'Siehe GitHub für Details',
            );
            
            return $response;
        }
        
        return $false;
    }
    
    /**
     * Get latest release info from GitHub
     */
    private function get_github_release_info() {
        $cache_key = 'themisdb_tco_github_release';
        $cached = get_transient($cache_key);
        
        if ($cached !== false) {
            return $cached;
        }
        
        $api_url = 'https://api.github.com/repos/' . THEMISDB_TCO_GITHUB_REPO . '/releases/latest';
        
        $response = wp_remote_get($api_url, array(
            'timeout' => 10,
            'headers' => array(
                'Accept' => 'application/vnd.github+json',
                'X-GitHub-Api-Version' => '2022-11-28',
            ),
        ));
        
        if (is_wp_error($response)) {
            return false;
        }
        
        // Check HTTP status code
        $status_code = wp_remote_retrieve_response_code($response);
        if ($status_code !== 200) {
            return false;
        }
        
        $body = wp_remote_retrieve_body($response);
        
        // Validate JSON before decoding
        if (empty($body)) {
            return false;
        }
        
        $data = json_decode($body);
        
        // Check for JSON decoding errors
        if (json_last_error() !== JSON_ERROR_NONE) {
            return false;
        }
        
        if ($data && isset($data->tag_name)) {
            // Cache for 12 hours
            set_transient($cache_key, $data, 12 * HOUR_IN_SECONDS);
            return $data;
        }
        
        return false;
    }
    
    /**
     * Get GitHub download URL for specific version
     * Note: For production use, consider creating proper plugin ZIP releases
     * that contain only the plugin files in the correct structure.
     * Current implementation downloads the entire repository archive.
     */
    private function get_github_download_url($version) {
        // For a production plugin, you would want to create GitHub releases
        // with pre-packaged plugin ZIPs. This is a simplified implementation
        // that assumes the user will create proper release assets.
        // 
        // Alternative: Use release assets if available
        $release_info = get_transient('themisdb_tco_github_release');
        if ($release_info && isset($release_info->assets) && !empty($release_info->assets)) {
            // Look for a plugin ZIP in the release assets
            foreach ($release_info->assets as $asset) {
                if (strpos($asset->name, 'themisdb-tco-calculator') !== false && 
                    strpos($asset->name, '.zip') !== false) {
                    return $asset->browser_download_url;
                }
            }
        }
        
        // Fallback: repository archive (may require manual extraction)
        return 'https://github.com/' . THEMISDB_TCO_GITHUB_REPO . '/archive/refs/tags/' . $version . '.zip';
    }
}

// Initialize plugin
ThemisDB_TCO_Calculator::get_instance();
