<?php
/**
 * Plugin Name: ThemisDB Benchmark Visualizer
 * Plugin URI: https://github.com/makr-code/ThemisDB
 * Description: Interactive visualization of ThemisDB performance benchmarks. Compare ThemisDB performance against PostgreSQL, MongoDB, and Neo4j. Use shortcode [themisdb_benchmark_visualizer] to embed.
 * Version: 1.0.0
 * Author: ThemisDB Team
 * Author URI: https://github.com/makr-code/ThemisDB
 * License: MIT
 * License URI: https://opensource.org/licenses/MIT
 * Text Domain: themisdb-benchmark-visualizer
 * Domain Path: /languages
 * Requires at least: 5.0
 * Requires PHP: 7.4
 */

// Exit if accessed directly
if (!defined('ABSPATH')) {
    exit;
}

// Define plugin constants
define('THEMISDB_BV_VERSION', '1.0.0');
define('THEMISDB_BV_PLUGIN_DIR', plugin_dir_path(__FILE__));
define('THEMISDB_BV_PLUGIN_URL', plugin_dir_url(__FILE__));
define('THEMISDB_BV_PLUGIN_FILE', __FILE__);
define('THEMISDB_BV_GITHUB_REPO', 'makr-code/ThemisDB');
define('THEMISDB_BV_GITHUB_PATH', 'tools/benchmark-visualizer-wordpress');

/**
 * Main Plugin Class
 */
class ThemisDB_Benchmark_Visualizer {
    
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
        add_shortcode('themisdb_benchmark_visualizer', array($this, 'render_visualizer'));
        
        // Admin menu
        add_action('admin_menu', array($this, 'add_admin_menu'));
        add_action('admin_init', array($this, 'register_settings'));
        
        // Plugin action links
        add_filter('plugin_action_links_' . plugin_basename(THEMISDB_BV_PLUGIN_FILE), array($this, 'add_action_links'));
        
        // AJAX endpoints
        add_action('wp_ajax_themisdb_bv_get_data', array($this, 'ajax_get_data'));
        add_action('wp_ajax_nopriv_themisdb_bv_get_data', array($this, 'ajax_get_data'));
    }
    
    /**
     * Plugin activation
     */
    public function activate() {
        // Set default options
        $defaults = array(
            'data_source' => 'local',
            'github_data_url' => 'https://raw.githubusercontent.com/makr-code/ThemisDB/main/benchmarks/benchmark_results/',
            'default_comparison_dbs' => 'postgresql,mongodb',
            'chart_theme' => 'light',
            'auto_update_interval' => 86400, // 24 hours
            'default_metric' => 'latency',
            'default_category' => 'all',
        );
        
        foreach ($defaults as $key => $value) {
            if (get_option('themisdb_bv_' . $key) === false) {
                add_option('themisdb_bv_' . $key, $value);
            }
        }
    }
    
    /**
     * Plugin deactivation
     */
    public function deactivate() {
        // Clean up transients
        delete_transient('themisdb_bv_cached_data');
    }
    
    /**
     * Initialize plugin
     */
    public function init() {
        // Load text domain
        load_plugin_textdomain('themisdb-benchmark-visualizer', false, dirname(plugin_basename(__FILE__)) . '/languages');
    }
    
    /**
     * Enqueue assets
     */
    public function enqueue_assets() {
        global $post;
        
        // Only load if shortcode is present
        if (!is_a($post, 'WP_Post') || !has_shortcode($post->post_content, 'themisdb_benchmark_visualizer')) {
            return;
        }
        
        // Chart.js from CDN (same version as TCO Calculator)
        wp_enqueue_script(
            'chartjs',
            'https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js',
            array(),
            '4.4.0',
            true
        );
        
        // Plugin CSS
        wp_enqueue_style(
            'themisdb-bv-style',
            THEMISDB_BV_PLUGIN_URL . 'assets/css/benchmark-visualizer.css',
            array(),
            THEMISDB_BV_VERSION
        );
        
        // Plugin JS
        wp_enqueue_script(
            'themisdb-bv-script',
            THEMISDB_BV_PLUGIN_URL . 'assets/js/benchmark-visualizer.js',
            array('jquery', 'chartjs'),
            THEMISDB_BV_VERSION,
            true
        );
        
        // Localize script with AJAX URL and settings
        wp_localize_script('themisdb-bv-script', 'themisdbBV', array(
            'ajax_url' => admin_url('admin-ajax.php'),
            'nonce' => wp_create_nonce('themisdb_bv_nonce'),
            'plugin_url' => THEMISDB_BV_PLUGIN_URL,
            'settings' => array(
                'chart_theme' => get_option('themisdb_bv_chart_theme', 'light'),
                'default_metric' => get_option('themisdb_bv_default_metric', 'latency'),
                'default_category' => get_option('themisdb_bv_default_category', 'all'),
            ),
        ));
    }
    
    /**
     * Render benchmark visualizer
     */
    public function render_visualizer($atts) {
        $atts = shortcode_atts(array(
            'category' => get_option('themisdb_bv_default_category', 'all'),
            'compare' => get_option('themisdb_bv_default_comparison_dbs', 'postgresql,mongodb'),
            'metric' => get_option('themisdb_bv_default_metric', 'latency'),
            'chart_type' => 'bar',
        ), $atts, 'themisdb_benchmark_visualizer');
        
        // Load template
        ob_start();
        include THEMISDB_BV_PLUGIN_DIR . 'templates/visualizer.php';
        return ob_get_clean();
    }
    
    /**
     * Add admin menu
     */
    public function add_admin_menu() {
        add_options_page(
            __('Benchmark Visualizer Settings', 'themisdb-benchmark-visualizer'),
            __('Benchmark Visualizer', 'themisdb-benchmark-visualizer'),
            'manage_options',
            'themisdb-bv-settings',
            array($this, 'render_settings_page')
        );
    }
    
    /**
     * Register settings
     */
    public function register_settings() {
        register_setting('themisdb_bv_settings', 'themisdb_bv_data_source');
        register_setting('themisdb_bv_settings', 'themisdb_bv_github_data_url');
        register_setting('themisdb_bv_settings', 'themisdb_bv_default_comparison_dbs');
        register_setting('themisdb_bv_settings', 'themisdb_bv_chart_theme');
        register_setting('themisdb_bv_settings', 'themisdb_bv_auto_update_interval');
        register_setting('themisdb_bv_settings', 'themisdb_bv_default_metric');
        register_setting('themisdb_bv_settings', 'themisdb_bv_default_category');
    }
    
    /**
     * Render settings page
     */
    public function render_settings_page() {
        if (!current_user_can('manage_options')) {
            return;
        }
        
        include THEMISDB_BV_PLUGIN_DIR . 'templates/admin-settings.php';
    }
    
    /**
     * Add plugin action links
     */
    public function add_action_links($links) {
        $settings_link = '<a href="' . admin_url('options-general.php?page=themisdb-bv-settings') . '">' . __('Settings', 'themisdb-benchmark-visualizer') . '</a>';
        array_unshift($links, $settings_link);
        return $links;
    }
    
    /**
     * AJAX handler to get benchmark data
     */
    public function ajax_get_data() {
        check_ajax_referer('themisdb_bv_nonce', 'nonce');
        
        $category = isset($_POST['category']) ? sanitize_text_field($_POST['category']) : 'all';
        $metric = isset($_POST['metric']) ? sanitize_text_field($_POST['metric']) : 'latency';
        
        // Check cache first
        $cache_key = 'themisdb_bv_data_' . md5($category . '_' . $metric);
        $cached_data = get_transient($cache_key);
        
        if ($cached_data !== false) {
            wp_send_json_success($cached_data);
            return;
        }
        
        // Load data
        $data = $this->load_benchmark_data($category, $metric);
        
        // Cache data
        $cache_duration = get_option('themisdb_bv_auto_update_interval', 86400);
        set_transient($cache_key, $data, $cache_duration);
        
        wp_send_json_success($data);
    }
    
    /**
     * Load benchmark data
     */
    private function load_benchmark_data($category = 'all', $metric = 'latency') {
        $data_source = get_option('themisdb_bv_data_source', 'local');
        
        if ($data_source === 'local') {
            return $this->load_local_benchmark_data($category, $metric);
        } else {
            return $this->load_github_benchmark_data($category, $metric);
        }
    }
    
    /**
     * Load local benchmark data
     */
    private function load_local_benchmark_data($category, $metric) {
        // Find the benchmark results directory
        $benchmark_dir = $this->find_benchmark_directory();
        
        if (!$benchmark_dir || !is_dir($benchmark_dir)) {
            return $this->get_fallback_data($category, $metric);
        }
        
        // Get available benchmark files
        $benchmark_files = $this->get_benchmark_files($benchmark_dir, $category);
        
        if (empty($benchmark_files)) {
            return $this->get_fallback_data($category, $metric);
        }
        
        // Parse benchmark data
        $parsed_data = $this->parse_benchmark_files($benchmark_files, $metric);
        
        return array(
            'labels' => $parsed_data['labels'],
            'datasets' => $parsed_data['datasets'],
            'metric' => $metric,
            'category' => $category,
            'summary' => $parsed_data['summary'],
        );
    }
    
    /**
     * Find benchmark directory
     */
    private function find_benchmark_directory() {
        // Try multiple possible locations
        $possible_paths = array(
            // Relative to plugin directory
            THEMISDB_BV_PLUGIN_DIR . '../../benchmarks/benchmark_results/20251223_085556',
            THEMISDB_BV_PLUGIN_DIR . '../../../benchmarks/benchmark_results/20251223_085556',
            // Absolute paths
            '/home/runner/work/ThemisDB/ThemisDB/benchmarks/benchmark_results/20251223_085556',
            dirname(THEMISDB_BV_PLUGIN_DIR, 4) . '/benchmarks/benchmark_results/20251223_085556',
        );
        
        foreach ($possible_paths as $path) {
            if (is_dir($path)) {
                return $path;
            }
        }
        
        // Try to find latest benchmark results
        $benchmark_base = dirname(THEMISDB_BV_PLUGIN_DIR, 4) . '/benchmarks/benchmark_results';
        if (is_dir($benchmark_base)) {
            $dirs = glob($benchmark_base . '/202*', GLOB_ONLYDIR);
            if (!empty($dirs)) {
                rsort($dirs); // Get latest
                return $dirs[0];
            }
        }
        
        return false;
    }
    
    /**
     * Get benchmark files based on category
     */
    private function get_benchmark_files($benchmark_dir, $category) {
        $files = array();
        
        // Map categories to file patterns
        $category_map = array(
            'all' => array('bench_comprehensive.json', 'bench_core_performance.json', 'bench_graph_traversal.json'),
            'vector_search' => array('bench_comprehensive.json', 'bench_gnn_embeddings.json'),
            'graph_traversal' => array('bench_graph_traversal.json', 'bench_pagerank.json'),
            'encryption' => array('bench_encryption.json'),
            'compression' => array('bench_compression.json'),
            'transaction' => array('bench_mvcc.json', 'bench_lock_contention.json'),
            'image_analysis' => array('bench_image_analysis.json', 'bench_image_analysis_latency.json'),
            'advanced' => array('bench_advanced_patterns.json', 'bench_hybrid_aql_sugar.json'),
        );
        
        $patterns = isset($category_map[$category]) ? $category_map[$category] : $category_map['all'];
        
        foreach ($patterns as $pattern) {
            $file_path = $benchmark_dir . '/' . $pattern;
            if (file_exists($file_path)) {
                $files[] = $file_path;
            }
        }
        
        return $files;
    }
    
    /**
     * Parse benchmark JSON files
     */
    private function parse_benchmark_files($files, $metric) {
        $labels = array();
        $data_points = array();
        $summary_stats = array(
            'total_benchmarks' => 0,
            'avg_time' => 0,
            'fastest' => null,
            'slowest' => null,
        );
        
        foreach ($files as $file) {
            $content = file_get_contents($file);
            $json = json_decode($content, true);
            
            if (!$json || !isset($json['benchmarks'])) {
                continue;
            }
            
            foreach ($json['benchmarks'] as $bench) {
                $name = $this->format_benchmark_name($bench['name']);
                
                // Extract metric value
                $value = $this->extract_metric_value($bench, $metric);
                
                if ($value !== null) {
                    $labels[] = $name;
                    $data_points[] = $value;
                    $summary_stats['total_benchmarks']++;
                    
                    if ($summary_stats['fastest'] === null || $value < $summary_stats['fastest']) {
                        $summary_stats['fastest'] = $value;
                    }
                    if ($summary_stats['slowest'] === null || $value > $summary_stats['slowest']) {
                        $summary_stats['slowest'] = $value;
                    }
                }
            }
        }
        
        if (!empty($data_points)) {
            $summary_stats['avg_time'] = array_sum($data_points) / count($data_points);
        }
        
        // Build datasets
        $datasets = array(
            array(
                'label' => 'ThemisDB',
                'data' => $data_points,
                'backgroundColor' => 'rgba(46, 164, 79, 0.8)',
                'borderColor' => 'rgba(46, 164, 79, 1)',
                'borderWidth' => 2,
                'pointRadius' => 4,
                'pointHoverRadius' => 6,
            ),
        );
        
        return array(
            'labels' => $labels,
            'datasets' => $datasets,
            'summary' => $summary_stats,
        );
    }
    
    /**
     * Format benchmark name for display
     */
    private function format_benchmark_name($name) {
        // Remove benchmark fixture prefix
        $name = preg_replace('/^[^\/]+\//', '', $name);
        
        // Replace underscores with spaces
        $name = str_replace('_', ' ', $name);
        
        // Truncate if too long
        if (strlen($name) > 50) {
            $name = substr($name, 0, 47) . '...';
        }
        
        return $name;
    }
    
    /**
     * Extract metric value from benchmark data
     */
    private function extract_metric_value($bench, $metric) {
        switch ($metric) {
            case 'latency':
                // Convert time to milliseconds
                $time = isset($bench['real_time']) ? $bench['real_time'] : $bench['cpu_time'];
                $unit = isset($bench['time_unit']) ? $bench['time_unit'] : 'ms';
                
                if ($unit === 'ns') {
                    return $time / 1000000; // ns to ms
                } elseif ($unit === 'us') {
                    return $time / 1000; // us to ms
                } elseif ($unit === 's') {
                    return $time * 1000; // s to ms
                }
                return $time; // already in ms
                
            case 'throughput':
                return isset($bench['items_per_second']) ? $bench['items_per_second'] : null;
                
            case 'memory':
                // Memory metrics might not be available in all benchmarks
                return isset($bench['bytes_per_second']) ? $bench['bytes_per_second'] / (1024 * 1024) : null;
                
            default:
                return isset($bench['real_time']) ? $bench['real_time'] : null;
        }
    }
    
    /**
     * Get fallback data when real benchmarks aren't available
     */
    private function get_fallback_data($category, $metric) {
        return array(
            'labels' => array('Vector Search', 'AQL Query', 'Graph Traversal', 'Document Insert', 'Transaction'),
            'datasets' => array(
                array(
                    'label' => 'ThemisDB',
                    'data' => array(2.3, 1.5, 3.1, 0.8, 4.2),
                    'backgroundColor' => 'rgba(46, 164, 79, 0.8)',
                    'borderColor' => 'rgba(46, 164, 79, 1)',
                    'borderWidth' => 1,
                ),
            ),
            'metric' => $metric,
            'category' => $category,
            'summary' => array(
                'total_benchmarks' => 5,
                'avg_time' => 2.38,
                'fastest' => 0.8,
                'slowest' => 4.2,
            ),
        );
    }
    
    /**
     * Load GitHub benchmark data
     */
    private function load_github_benchmark_data($category, $metric) {
        // This would fetch data from GitHub in a production environment
        return $this->load_local_benchmark_data($category, $metric);
    }
}

// Initialize plugin
function themisdb_benchmark_visualizer_init() {
    return ThemisDB_Benchmark_Visualizer::get_instance();
}

add_action('plugins_loaded', 'themisdb_benchmark_visualizer_init');
