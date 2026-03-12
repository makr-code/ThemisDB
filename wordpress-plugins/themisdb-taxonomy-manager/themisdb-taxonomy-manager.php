<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            themisdb-taxonomy-manager.php                      ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:21                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     717                                            ║
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
 * Plugin Name: ThemisDB Taxonomy Manager
 * Plugin URI: https://github.com/makr-code/wordpressPlugins
 * Description: Manage custom taxonomies for ThemisDB features, use-cases, and industries with visual tree view
 * Version: 1.0.0
 * Author: ThemisDB Team
 * Author URI: https://github.com/makr-code/wordpressPlugins
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
define('THEMISDB_TAXONOMY_FILE', __FILE__);
// Backward compatibility aliases
define('THEMISDB_TAXONOMY_PLUGIN_DIR', THEMISDB_TAXONOMY_DIR);
define('THEMISDB_TAXONOMY_PLUGIN_URL', THEMISDB_TAXONOMY_URL);

// Load updater class
$themisdb_updater_local = THEMISDB_TAXONOMY_PLUGIN_DIR . 'includes/class-themisdb-plugin-updater.php';
$themisdb_updater_shared = dirname(THEMISDB_TAXONOMY_PLUGIN_DIR) . '/includes/class-themisdb-plugin-updater.php';

if (file_exists($themisdb_updater_local)) {
    require_once $themisdb_updater_local;
} elseif (file_exists($themisdb_updater_shared)) {
    require_once $themisdb_updater_shared;
}

// Initialize automatic updates
if (class_exists('ThemisDB_Plugin_Updater')) {
    new ThemisDB_Plugin_Updater(
        THEMISDB_TAXONOMY_FILE,
        'themisdb-taxonomy-manager',
        THEMISDB_TAXONOMY_VERSION
    );
}

/**
 * Safe require helper - loads files with error handling and logging
 *
 * @param string $file File path to require
 * @param bool $is_critical Whether this file is critical for plugin operation
 * @return bool True if file was loaded successfully, false otherwise
 */
function themisdb_taxonomy_safe_require($file, $is_critical = true) {
    static $displayed_notices = array();
    
    if (file_exists($file)) {
        require_once $file;
        return true;
    }
    
    $error_message = sprintf(
        '[ThemisDB Taxonomy Manager] Missing file: %s',
        str_replace(THEMISDB_TAXONOMY_PLUGIN_DIR, '', $file)
    );
    
    // Log to debug.log if WP_DEBUG_LOG is enabled
    if (defined('WP_DEBUG_LOG') && WP_DEBUG_LOG) {
        error_log($error_message);
    }
    
    // Add admin notice (only once per file)
    $notice_key = md5($file);
    if (!isset($displayed_notices[$notice_key])) {
        $displayed_notices[$notice_key] = true;
        add_action('admin_notices', function() use ($file, $is_critical) {
            $class = $is_critical ? 'notice-error' : 'notice-warning';
            $message = sprintf(
                '<strong>ThemisDB Taxonomy Manager:</strong> Missing file: %s',
                esc_html(basename($file))
            );
            if ($is_critical) {
                $message .= ' - Plugin functionality may be impaired.';
            }
            printf('<div class="notice %s"><p>%s</p></div>', esc_attr($class), $message);
        });
    }
    
    return false;
}

// Track if all critical files loaded successfully
$themisdb_taxonomy_files_loaded = true;

// Include required files
// Note: Using bitwise AND (&=) instead of logical AND (&&=) to prevent short-circuit evaluation
// This ensures ALL files are checked and logged, not just until the first failure
$themisdb_taxonomy_files_loaded &= themisdb_taxonomy_safe_require(THEMISDB_TAXONOMY_PLUGIN_DIR . 'includes/class-tfidf.php');
$themisdb_taxonomy_files_loaded &= themisdb_taxonomy_safe_require(THEMISDB_TAXONOMY_PLUGIN_DIR . 'includes/class-analytics.php');
$themisdb_taxonomy_files_loaded &= themisdb_taxonomy_safe_require(THEMISDB_TAXONOMY_PLUGIN_DIR . 'includes/class-category-hierarchy.php');
$themisdb_taxonomy_files_loaded &= themisdb_taxonomy_safe_require(THEMISDB_TAXONOMY_PLUGIN_DIR . 'includes/class-taxonomy-extractor.php');
$themisdb_taxonomy_files_loaded &= themisdb_taxonomy_safe_require(THEMISDB_TAXONOMY_PLUGIN_DIR . 'includes/class-taxonomy-manager.php');
$themisdb_taxonomy_files_loaded &= themisdb_taxonomy_safe_require(THEMISDB_TAXONOMY_PLUGIN_DIR . 'includes/class-admin.php');
$themisdb_taxonomy_files_loaded &= themisdb_taxonomy_safe_require(THEMISDB_TAXONOMY_PLUGIN_DIR . 'includes/class-custom-taxonomies.php');
$themisdb_taxonomy_files_loaded &= themisdb_taxonomy_safe_require(THEMISDB_TAXONOMY_PLUGIN_DIR . 'includes/class-tree-view.php');
$themisdb_taxonomy_files_loaded &= themisdb_taxonomy_safe_require(THEMISDB_TAXONOMY_PLUGIN_DIR . 'includes/class-widget.php');
$themisdb_taxonomy_files_loaded &= themisdb_taxonomy_safe_require(THEMISDB_TAXONOMY_PLUGIN_DIR . 'includes/class-metabox.php');
$themisdb_taxonomy_files_loaded &= themisdb_taxonomy_safe_require(THEMISDB_TAXONOMY_PLUGIN_DIR . 'includes/class-template-handler.php');

// Abort initialization if critical files are missing
if (!$themisdb_taxonomy_files_loaded) {
    if (defined('WP_DEBUG_LOG') && WP_DEBUG_LOG) {
        error_log('[ThemisDB Taxonomy Manager] Plugin initialization aborted due to missing files');
    }
    return;
}

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
     * Custom Taxonomies instance
     */
    private $custom_taxonomies;
    
    /**
     * Tree View instance
     */
    private $tree_view;
    
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
        
        // Initialize custom taxonomies
        $this->custom_taxonomies = new ThemisDB_Custom_Taxonomies();
        
        // Initialize admin panel
        if (is_admin()) {
            new ThemisDB_Taxonomy_Admin();
            $this->tree_view = new ThemisDB_Tree_View();
        }
        
        // Register activation/deactivation hooks
        register_activation_hook(__FILE__, array($this, 'activate'));
        register_deactivation_hook(__FILE__, array($this, 'deactivate'));
        
        // Load text domain
        add_action('plugins_loaded', array($this, 'load_textdomain'));
        
        // Register shortcodes
        add_action('init', array($this, 'register_shortcodes'));
        
        // Register REST API routes
        add_action('rest_api_init', array($this, 'register_rest_api'));
        
        // Add schema.org markup
        add_action('wp_head', array($this, 'add_schema_markup'));
        
        // Add breadcrumbs
        add_action('themisdb_before_taxonomy_archive', array($this, 'render_breadcrumbs'));
    }
    
    /**
     * Plugin activation
     */
    public function activate() {
        // Set default options
        add_option('themisdb_taxonomy_auto_extract', 1);
        add_option('themisdb_taxonomy_auto_tags', 1);
        add_option('themisdb_taxonomy_auto_categories', 1);
        add_option('themisdb_taxonomy_max_categories', 5);
        add_option('themisdb_taxonomy_max_tags', 10);
        add_option('themisdb_taxonomy_min_tfidf_score', 0.5);
        add_option('themisdb_taxonomy_similarity_threshold', 0.8);
        add_option('themisdb_taxonomy_prefer_existing', 1);
        add_option('themisdb_taxonomy_max_category_depth', 3);
        add_option('themisdb_taxonomy_min_category_posts', 2);
        add_option('themisdb_taxonomy_consolidate_categories', 1);
        add_option('themisdb_taxonomy_show_in_rest', 1);
        add_option('themisdb_taxonomy_enable_seo_schema', 1);
        add_option('themisdb_taxonomy_default_icon', '📊');
        add_option('themisdb_taxonomy_default_color', '#3498db');
        add_option('themisdb_taxonomy_breadcrumb_separator', ' / ');
        add_option('themisdb_taxonomy_enable_custom_metabox', 1);
        
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
            'themisdb-taxonomy',
            false,
            dirname(plugin_basename(__FILE__)) . '/languages'
        );
    }
    
    /**
     * Register shortcodes
     */
    public function register_shortcodes() {
        add_shortcode('themisdb_taxonomy', array($this, 'taxonomy_shortcode'));
        add_shortcode('themisdb_taxonomy_info', array($this, 'taxonomy_info_shortcode'));
    }
    
    /**
     * Taxonomy shortcode
     * [themisdb_taxonomy taxonomy="themisdb_feature" style="list" show_count="yes" parent_only="no"]
     */
    public function taxonomy_shortcode($atts) {
        $atts = shortcode_atts(array(
            'taxonomy' => 'themisdb_feature',
            'style' => 'list',
            'show_count' => 'yes',
            'parent_only' => 'no',
            'min_size' => '0.8',
            'max_size' => '2'
        ), $atts);
        
        $term_args = array(
            'taxonomy' => $atts['taxonomy'],
            'hide_empty' => false,
            'orderby' => 'name',
            'order' => 'ASC'
        );
        
        if ($atts['parent_only'] === 'yes') {
            $term_args['parent'] = 0;
        }
        
        $terms = get_terms($term_args);
        
        if (empty($terms) || is_wp_error($terms)) {
            return '';
        }
        
        $show_count = ($atts['show_count'] === 'yes');
        
        ob_start();
        
        wp_enqueue_style(
            'themisdb-taxonomy-widget',
            THEMISDB_TAXONOMY_PLUGIN_URL . 'assets/css/taxonomy-widget.css',
            array(),
            THEMISDB_TAXONOMY_VERSION
        );
        
        switch ($atts['style']) {
            case 'cloud':
                $this->render_cloud_shortcode($terms, $show_count, $atts['min_size'], $atts['max_size']);
                break;
            case 'grid':
                $this->render_grid_shortcode($terms, $show_count);
                break;
            case 'list':
            default:
                $this->render_list_shortcode($terms, $show_count);
                break;
        }
        
        return ob_get_clean();
    }
    
    /**
     * Taxonomy info shortcode
     * [themisdb_taxonomy_info term_id="123"]
     */
    public function taxonomy_info_shortcode($atts) {
        $atts = shortcode_atts(array(
            'term_id' => 0
        ), $atts);
        
        $term = get_term($atts['term_id']);
        
        if (is_wp_error($term) || !$term) {
            return '';
        }
        
        $icon = get_term_meta($term->term_id, 'icon', true);
        $color = get_term_meta($term->term_id, 'color', true);
        
        ob_start();
        ?>
        <div class="themisdb-taxonomy-info" style="border-left-color: <?php echo esc_attr($color); ?>;">
            <?php if (!empty($icon)): ?>
                <span class="taxonomy-icon"><?php echo esc_html($icon); ?></span>
            <?php endif; ?>
            <h3><?php echo esc_html($term->name); ?></h3>
            <?php if (!empty($term->description)): ?>
                <p><?php echo esc_html($term->description); ?></p>
            <?php endif; ?>
            <span class="post-count"><?php echo $term->count; ?> posts</span>
        </div>
        <?php
        return ob_get_clean();
    }
    
    /**
     * Render list style for shortcode
     */
    private function render_list_shortcode($terms, $show_count) {
        echo '<ul class="themisdb-taxonomy-list">';
        foreach ($terms as $term) {
            $icon = get_term_meta($term->term_id, 'icon', true);
            echo '<li>';
            echo '<a href="' . esc_url(get_term_link($term)) . '">';
            if (!empty($icon)) {
                echo '<span class="icon">' . esc_html($icon) . '</span> ';
            }
            echo esc_html($term->name);
            if ($show_count) {
                echo ' <span class="count">(' . $term->count . ')</span>';
            }
            echo '</a>';
            echo '</li>';
        }
        echo '</ul>';
    }
    
    /**
     * Render cloud style for shortcode
     */
    private function render_cloud_shortcode($terms, $show_count, $min_size, $max_size) {
        $counts = wp_list_pluck($terms, 'count');
        $min_count = min($counts);
        $max_count = max($counts);
        $spread = $max_count - $min_count;
        if ($spread <= 0) {
            $spread = 1;
        }
        
        echo '<div class="themisdb-tag-cloud">';
        foreach ($terms as $term) {
            $color = get_term_meta($term->term_id, 'color', true);
            if (empty($color)) {
                $color = '#3498db';
            }
            
            $size = $min_size + (($term->count - $min_count) / $spread) * ($max_size - $min_size);
            
            echo '<a href="' . esc_url(get_term_link($term)) . '" ';
            echo 'style="font-size: ' . esc_attr($size) . 'em; color: ' . esc_attr($color) . ';">';
            echo esc_html($term->name);
            echo '</a> ';
        }
        echo '</div>';
    }
    
    /**
     * Render grid style for shortcode
     */
    private function render_grid_shortcode($terms, $show_count) {
        echo '<div class="themisdb-taxonomy-grid">';
        foreach ($terms as $term) {
            $icon = get_term_meta($term->term_id, 'icon', true);
            $color = get_term_meta($term->term_id, 'color', true);
            if (empty($color)) {
                $color = '#3498db';
            }
            
            echo '<div class="taxonomy-card" style="border-color: ' . esc_attr($color) . ';">';
            echo '<a href="' . esc_url(get_term_link($term)) . '">';
            if (!empty($icon)) {
                echo '<span class="card-icon">' . esc_html($icon) . '</span>';
            }
            echo '<h3>' . esc_html($term->name) . '</h3>';
            if ($show_count) {
                $taxonomy_obj = get_taxonomy($term->taxonomy);
                echo '<span class="card-count">' . $term->count . ' ' . strtolower($taxonomy_obj->labels->name) . '</span>';
            }
            echo '</a>';
            echo '</div>';
        }
        echo '</div>';
    }
    
    /**
     * Register REST API routes
     */
    public function register_rest_api() {
        // GET /wp-json/themisdb/v1/taxonomies
        register_rest_route('themisdb/v1', '/taxonomies', array(
            'methods' => 'GET',
            'callback' => array($this, 'rest_get_taxonomies'),
            'permission_callback' => '__return_true'
        ));
        
        // GET /wp-json/themisdb/v1/taxonomy/{taxonomy}
        register_rest_route('themisdb/v1', '/taxonomy/(?P<taxonomy>[a-z_]+)', array(
            'methods' => 'GET',
            'callback' => array($this, 'rest_get_taxonomy_terms'),
            'permission_callback' => '__return_true'
        ));
        
        // GET /wp-json/themisdb/v1/taxonomy/{taxonomy}/tree
        register_rest_route('themisdb/v1', '/taxonomy/(?P<taxonomy>[a-z_]+)/tree', array(
            'methods' => 'GET',
            'callback' => array($this, 'rest_get_taxonomy_tree'),
            'permission_callback' => '__return_true'
        ));
    }
    
    /**
     * REST API: Get all taxonomies
     */
    public function rest_get_taxonomies($request) {
        $taxonomies = array('themisdb_feature', 'themisdb_usecase', 'themisdb_industry', 'themisdb_techspec');
        $result = array();
        
        foreach ($taxonomies as $taxonomy) {
            $tax_object = get_taxonomy($taxonomy);
            if ($tax_object) {
                $result[] = array(
                    'name' => $taxonomy,
                    'label' => $tax_object->label,
                    'hierarchical' => $tax_object->hierarchical,
                    'public' => $tax_object->public,
                    'rewrite' => $tax_object->rewrite
                );
            }
        }
        
        return rest_ensure_response($result);
    }
    
    /**
     * REST API: Get taxonomy terms
     */
    public function rest_get_taxonomy_terms($request) {
        $taxonomy = $request->get_param('taxonomy');
        
        $terms = get_terms(array(
            'taxonomy' => $taxonomy,
            'hide_empty' => false
        ));
        
        if (is_wp_error($terms)) {
            return new WP_Error('invalid_taxonomy', 'Invalid taxonomy', array('status' => 404));
        }
        
        $result = array();
        foreach ($terms as $term) {
            $result[] = array(
                'term_id' => $term->term_id,
                'name' => $term->name,
                'slug' => $term->slug,
                'description' => $term->description,
                'parent' => $term->parent,
                'count' => $term->count,
                'link' => get_term_link($term),
                'meta' => array(
                    'icon' => get_term_meta($term->term_id, 'icon', true),
                    'color' => get_term_meta($term->term_id, 'color', true)
                )
            );
        }
        
        return rest_ensure_response($result);
    }
    
    /**
     * REST API: Get taxonomy tree
     */
    public function rest_get_taxonomy_tree($request) {
        $taxonomy = $request->get_param('taxonomy');
        
        $terms = get_terms(array(
            'taxonomy' => $taxonomy,
            'hide_empty' => false,
            'parent' => 0
        ));
        
        if (is_wp_error($terms)) {
            return new WP_Error('invalid_taxonomy', 'Invalid taxonomy', array('status' => 404));
        }
        
        $result = array();
        foreach ($terms as $term) {
            $result[] = $this->build_term_tree($term);
        }
        
        return rest_ensure_response($result);
    }
    
    /**
     * Build term tree recursively
     */
    private function build_term_tree($term) {
        $children = get_terms(array(
            'taxonomy' => $term->taxonomy,
            'hide_empty' => false,
            'parent' => $term->term_id
        ));
        
        $term_data = array(
            'term_id' => $term->term_id,
            'name' => $term->name,
            'slug' => $term->slug,
            'description' => $term->description,
            'count' => $term->count,
            'link' => get_term_link($term),
            'meta' => array(
                'icon' => get_term_meta($term->term_id, 'icon', true),
                'color' => get_term_meta($term->term_id, 'color', true)
            ),
            'children' => array()
        );
        
        if (!empty($children) && !is_wp_error($children)) {
            foreach ($children as $child) {
                $term_data['children'][] = $this->build_term_tree($child);
            }
        }
        
        return $term_data;
    }
    
    /**
     * Add Schema.org markup for taxonomy pages
     */
    public function add_schema_markup() {
        if (!is_tax(array('themisdb_feature', 'themisdb_usecase', 'themisdb_industry', 'themisdb_techspec'))) {
            return;
        }
        
        if (!get_option('themisdb_taxonomy_enable_seo_schema', 1)) {
            return;
        }
        
        $term = get_queried_object();
        if (!$term) {
            return;
        }
        
        ?>
        <script type="application/ld+json">
        {
            "@context": "https://schema.org",
            "@type": "CollectionPage",
            "name": "<?php echo esc_js($term->name); ?>",
            "description": "<?php echo esc_js($term->description); ?>",
            "url": "<?php echo esc_url(get_term_link($term)); ?>",
            "isPartOf": {
                "@type": "WebSite",
                "name": "<?php echo esc_js(get_bloginfo('name')); ?>",
                "url": "<?php echo esc_url(home_url('/')); ?>"
            },
            "numberOfItems": <?php echo intval($term->count); ?>
        }
        </script>
        <?php
    }
    
    /**
     * Render breadcrumbs for taxonomy pages
     */
    public function render_breadcrumbs() {
        if (!is_tax(array('themisdb_feature', 'themisdb_usecase', 'themisdb_industry', 'themisdb_techspec'))) {
            return;
        }
        
        $term = get_queried_object();
        if (!$term) {
            return;
        }
        
        $separator = get_option('themisdb_taxonomy_breadcrumb_separator', ' / ');
        $breadcrumbs = array();
        
        // Build breadcrumb trail
        $current_term = $term;
        while ($current_term) {
            array_unshift($breadcrumbs, $current_term);
            
            if ($current_term->parent > 0) {
                $current_term = get_term($current_term->parent, $term->taxonomy);
            } else {
                break;
            }
        }
        
        echo '<nav class="themisdb-breadcrumbs">';
        echo '<a href="' . esc_url(home_url('/')) . '">Home</a>' . $separator;
        
        $taxonomy_obj = get_taxonomy($term->taxonomy);
        if ($taxonomy_obj) {
            echo '<a href="' . esc_url(home_url('/' . $taxonomy_obj->rewrite['slug'] . '/')) . '">' . esc_html($taxonomy_obj->label) . '</a>' . $separator;
        }
        
        $count = count($breadcrumbs);
        foreach ($breadcrumbs as $index => $crumb) {
            if ($index === $count - 1) {
                echo '<strong>' . esc_html($crumb->name) . '</strong>';
            } else {
                echo '<a href="' . esc_url(get_term_link($crumb)) . '">' . esc_html($crumb->name) . '</a>' . $separator;
            }
        }
        
        echo '</nav>';
    }
    
    /**
     * Get taxonomy manager instance
     */
    public function get_taxonomy_manager() {
        return $this->taxonomy_manager;
    }
}

/**
 * Initialize plugin
 */
function themisdb_taxonomy_init() {
    ThemisDB_Taxonomy_Manager_Plugin::get_instance();
}
add_action('plugins_loaded', 'themisdb_taxonomy_init');

/**
 * Helper function to get taxonomy manager
 */
function themisdb_get_taxonomy_manager() {
    $plugin = ThemisDB_Taxonomy_Manager_Plugin::get_instance();
    return $plugin->get_taxonomy_manager();
}

function themisdb_taxonomy_enqueue_styles() {
    global $post;
    
    // Check if shortcodes are present
    $has_shortcode = false;
    if (is_a($post, 'WP_Post')) {
        $has_shortcode = has_shortcode($post->post_content, 'themisdb_taxonomy') || 
                        has_shortcode($post->post_content, 'themisdb_taxonomy_info');
    }
    
    // Check if widget is active
    $has_widget = is_active_widget(false, false, 'themisdb_taxonomy_widget', true);
    
    // Only load if shortcode or widget is present
    if (!$has_shortcode && !$has_widget) {
        return;
    }
    
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
