<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            themisdb-wiki-integration.php                      ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     754                                            ║
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
 * Plugin Name: ThemisDB Wiki Integration
 * Plugin URI: https://github.com/makr-code/wordpressPlugins
 * Description: Integrates ThemisDB documentation/wiki from GitHub into WordPress. Fetches markdown files on-demand and displays them with proper formatting. Manual sync recommended.
 * Version: 1.0.1
 * Author: ThemisDB Team
 * Author URI: https://github.com/makr-code
 * License: MIT
 * Text Domain: themisdb-wiki-integration
 * Domain Path: /languages
 */

// Exit if accessed directly
if (!defined('ABSPATH')) {
    exit;
}

// Plugin constants
define('THEMISDB_WIKI_VERSION', '1.0.1');
define('THEMISDB_WIKI_PLUGIN_DIR', plugin_dir_path(__FILE__));
define('THEMISDB_WIKI_PLUGIN_URL', plugin_dir_url(__FILE__));
define('THEMISDB_WIKI_PLUGIN_FILE', __FILE__);
define('THEMISDB_WIKI_CACHE_GROUP', 'themisdb_wiki');
define('THEMISDB_WIKI_CACHE_EXPIRATION', 3600); // 1 hour

// Load updater class
$themisdb_updater_local = THEMISDB_WIKI_PLUGIN_DIR . 'includes/class-themisdb-plugin-updater.php';
$themisdb_updater_shared = dirname(THEMISDB_WIKI_PLUGIN_DIR) . '/includes/class-themisdb-plugin-updater.php';

if (file_exists($themisdb_updater_local)) {
    require_once $themisdb_updater_local;
} elseif (file_exists($themisdb_updater_shared)) {
    require_once $themisdb_updater_shared;
}

// Initialize automatic updates
if (class_exists('ThemisDB_Plugin_Updater')) {
    new ThemisDB_Plugin_Updater(
        THEMISDB_WIKI_PLUGIN_FILE,
        'themisdb-wiki-integration',
        THEMISDB_WIKI_VERSION
    );
}

// Include markdown converter
require_once THEMISDB_WIKI_PLUGIN_DIR . 'includes/class-markdown-converter.php';

/**
 * Main Plugin Class
 */
class ThemisDB_Wiki_Integration {
    
    /**
     * Constructor
     */
    public function __construct() {
        // Activation/Deactivation hooks
        register_activation_hook(__FILE__, array($this, 'activate'));
        register_deactivation_hook(__FILE__, array($this, 'deactivate'));
        
        // Initialize plugin
        add_action('plugins_loaded', array($this, 'init'));
        add_action('admin_menu', array($this, 'add_admin_menu'));
        add_action('admin_init', array($this, 'register_settings'));
        add_action('wp_enqueue_scripts', array($this, 'enqueue_scripts'));
        
        // Register shortcodes
        add_shortcode('themisdb_wiki', array($this, 'wiki_shortcode'));
        add_shortcode('themisdb_docs', array($this, 'docs_shortcode'));
        add_shortcode('themisdb_wiki_nav', array($this, 'wiki_nav_shortcode'));
        
        // Register widget
        add_action('widgets_init', array($this, 'register_widgets'));
        
        // AJAX handlers for auto-sync
        add_action('wp_ajax_themisdb_sync_docs', array($this, 'ajax_sync_docs'));
        add_action('wp_ajax_themisdb_clear_cache', array($this, 'ajax_clear_cache'));
    }
    
    /**
     * Plugin activation
     */
    public function activate() {
        // Set default options
        add_option('themisdb_wiki_github_repo', 'makr-code/wordpressPlugins');
        add_option('themisdb_wiki_github_branch', 'main');
        add_option('themisdb_wiki_docs_path', 'docs');
        add_option('themisdb_wiki_auto_sync', 'no'); // Changed to 'no' - sync only on-demand
        add_option('themisdb_wiki_sync_interval', '3600');
        add_option('themisdb_wiki_default_lang', 'de');
        
        // Schedule auto-sync if enabled (disabled by default)
        if (get_option('themisdb_wiki_auto_sync') === 'yes') {
            if (!wp_next_scheduled('themisdb_wiki_auto_sync_hook')) {
                wp_schedule_event(time(), 'hourly', 'themisdb_wiki_auto_sync_hook');
            }
        }
    }
    
    /**
     * Plugin deactivation
     */
    public function deactivate() {
        // Clear scheduled auto-sync
        wp_clear_scheduled_hook('themisdb_wiki_auto_sync_hook');
        
        // Clear cache
        $this->clear_all_cache();
    }
    
    /**
     * Initialize plugin
     */
    public function init() {
        // Load text domain for translations
        load_plugin_textdomain('themisdb-wiki-integration', false, dirname(plugin_basename(__FILE__)) . '/languages');
        
        // Hook auto-sync event
        add_action('themisdb_wiki_auto_sync_hook', array($this, 'auto_sync_docs'));
    }
    
    /**
     * Add admin menu
     */
    public function add_admin_menu() {
        add_options_page(
            __('ThemisDB Wiki Integration', 'themisdb-wiki-integration'),
            __('ThemisDB Wiki', 'themisdb-wiki-integration'),
            'manage_options',
            'themisdb-wiki-integration',
            array($this, 'admin_page')
        );
    }
    
    /**
     * Register settings
     */
    public function register_settings() {
        register_setting('themisdb_wiki_settings', 'themisdb_wiki_github_repo');
        register_setting('themisdb_wiki_settings', 'themisdb_wiki_github_branch');
        register_setting('themisdb_wiki_settings', 'themisdb_wiki_docs_path');
        register_setting('themisdb_wiki_settings', 'themisdb_wiki_auto_sync');
        register_setting('themisdb_wiki_settings', 'themisdb_wiki_sync_interval');
        register_setting('themisdb_wiki_settings', 'themisdb_wiki_default_lang');
        register_setting('themisdb_wiki_settings', 'themisdb_wiki_github_token');
    }
    
    /**
     * Enqueue scripts and styles
     */
    public function enqueue_scripts() {
        global $post;
        
        // Only load if shortcode is present
        if (is_a($post, 'WP_Post') && (
            has_shortcode($post->post_content, 'themisdb_wiki') || 
            has_shortcode($post->post_content, 'themisdb_docs') ||
            has_shortcode($post->post_content, 'themisdb_wiki_nav')
        )) {
            wp_enqueue_style(
                'themisdb-wiki-style',
                THEMISDB_WIKI_PLUGIN_URL . 'assets/css/wiki-integration.css',
                array(),
                THEMISDB_WIKI_VERSION
            );
            
            // Enqueue Mermaid.js for diagram rendering
            wp_enqueue_script(
                'mermaid-js',
                'https://cdn.jsdelivr.net/npm/mermaid@10/dist/mermaid.min.js',
                array(),
                '10.0.0',
                true
            );
            
            wp_enqueue_script(
                'themisdb-wiki-script',
                THEMISDB_WIKI_PLUGIN_URL . 'assets/js/wiki-integration.js',
                array('jquery', 'mermaid-js'),
                THEMISDB_WIKI_VERSION,
                true
            );
            
            // Localize script
            wp_localize_script('themisdb-wiki-script', 'themisdbWiki', array(
                'ajaxurl' => admin_url('admin-ajax.php'),
                'nonce' => wp_create_nonce('themisdb_wiki_nonce')
            ));
        }
    }
    
    /**
     * Admin page
     */
    public function admin_page() {
        include THEMISDB_WIKI_PLUGIN_DIR . 'templates/admin-settings.php';
    }
    
    /**
     * Fetch file from GitHub
     */
    private function fetch_github_file($file_path, $lang = null) {
        $repo = get_option('themisdb_wiki_github_repo', 'makr-code/wordpressPlugins');
        $branch = get_option('themisdb_wiki_github_branch', 'main');
        $docs_path = get_option('themisdb_wiki_docs_path', 'docs');
        $github_token = get_option('themisdb_wiki_github_token', '');
        
        // Build GitHub API URL
        if ($lang) {
            $full_path = $docs_path . '/' . $lang . '/' . ltrim($file_path, '/');
        } else {
            $full_path = $docs_path . '/' . ltrim($file_path, '/');
        }
        
        $api_url = "https://api.github.com/repos/{$repo}/contents/{$full_path}?ref={$branch}";
        
        // Check cache
        $cache_key = 'github_file_' . md5($api_url);
        $cached_content = get_transient($cache_key);
        
        if ($cached_content !== false) {
            return $cached_content;
        }
        
        // Prepare request headers
        $headers = array(
            'Accept' => 'application/vnd.github.v3.raw',
            'User-Agent' => 'ThemisDB-Wiki-Integration-WordPress-Plugin'
        );
        
        if (!empty($github_token)) {
            $headers['Authorization'] = 'Bearer ' . $github_token;
        }
        
        // Fetch from GitHub
        $response = wp_remote_get($api_url, array(
            'headers' => $headers,
            'timeout' => 30
        ));
        
        if (is_wp_error($response)) {
            return new WP_Error('github_fetch_error', $response->get_error_message());
        }
        
        $status_code = wp_remote_retrieve_response_code($response);
        
        if ($status_code !== 200) {
            return new WP_Error('github_api_error', sprintf(__('GitHub API returned status code %d', 'themisdb-wiki-integration'), $status_code));
        }
        
        $content = wp_remote_retrieve_body($response);
        
        // Cache the content
        set_transient($cache_key, $content, THEMISDB_WIKI_CACHE_EXPIRATION);
        
        return $content;
    }
    
    /**
     * Convert Markdown to HTML using shared converter
     */
    private function markdown_to_html($markdown) {
        return ThemisDB_Markdown_Converter::convert($markdown);
    }
    
    /**
     * List available documentation files
     */
    private function list_docs_files($lang = null) {
        $repo = get_option('themisdb_wiki_github_repo', 'makr-code/wordpressPlugins');
        $branch = get_option('themisdb_wiki_github_branch', 'main');
        $docs_path = get_option('themisdb_wiki_docs_path', 'docs');
        $github_token = get_option('themisdb_wiki_github_token', '');
        
        // Build path
        if ($lang) {
            $full_path = $docs_path . '/' . $lang;
        } else {
            $full_path = $docs_path;
        }
        
        $api_url = "https://api.github.com/repos/{$repo}/contents/{$full_path}?ref={$branch}";
        
        // Check cache
        $cache_key = 'github_list_' . md5($api_url);
        $cached_list = get_transient($cache_key);
        
        if ($cached_list !== false) {
            return $cached_list;
        }
        
        // Prepare request headers
        $headers = array(
            'Accept' => 'application/vnd.github.v3+json',
            'User-Agent' => 'ThemisDB-Wiki-Integration-WordPress-Plugin'
        );
        
        if (!empty($github_token)) {
            $headers['Authorization'] = 'Bearer ' . $github_token;
        }
        
        // Fetch from GitHub
        $response = wp_remote_get($api_url, array(
            'headers' => $headers,
            'timeout' => 30
        ));
        
        if (is_wp_error($response)) {
            return array();
        }
        
        $status_code = wp_remote_retrieve_response_code($response);
        
        if ($status_code !== 200) {
            return array();
        }
        
        $content = json_decode(wp_remote_retrieve_body($response), true);
        
        // Filter for directories and .md files
        $files = array();
        foreach ($content as $item) {
            if ($item['type'] === 'file' && substr($item['name'], -3) === '.md') {
                $files[] = array(
                    'name' => $item['name'],
                    'path' => $item['path'],
                    'type' => 'file'
                );
            } elseif ($item['type'] === 'dir') {
                $files[] = array(
                    'name' => $item['name'],
                    'path' => $item['path'],
                    'type' => 'dir'
                );
            }
        }
        
        // Cache the list
        set_transient($cache_key, $files, THEMISDB_WIKI_CACHE_EXPIRATION);
        
        return $files;
    }
    
    /**
     * Wiki shortcode
     */
    public function wiki_shortcode($atts) {
        $atts = shortcode_atts(array(
            'file' => 'README.md',
            'lang' => get_option('themisdb_wiki_default_lang', 'de'),
            'show_toc' => 'no'
        ), $atts);
        
        $content = $this->fetch_github_file($atts['file'], $atts['lang']);
        
        if (is_wp_error($content)) {
            return '<div class="themisdb-wiki-error">' . esc_html($content->get_error_message()) . '</div>';
        }
        
        $html = $this->markdown_to_html($content);
        
        $output = '<div class="themisdb-wiki-container">';
        
        if ($atts['show_toc'] === 'yes') {
            $output .= $this->generate_toc($html);
        }
        
        $output .= '<div class="themisdb-wiki-content">' . $html . '</div>';
        $output .= '</div>';
        
        return $output;
    }
    
    /**
     * Docs shortcode (lists available docs)
     */
    public function docs_shortcode($atts) {
        $atts = shortcode_atts(array(
            'lang' => get_option('themisdb_wiki_default_lang', 'de'),
            'category' => '',
            'layout' => 'list'
        ), $atts);
        
        $files = $this->list_docs_files($atts['lang']);
        
        if (empty($files)) {
            return '<div class="themisdb-docs-error">' . __('No documentation files found.', 'themisdb-wiki-integration') . '</div>';
        }
        
        $output = '<div class="themisdb-docs-list">';
        
        if ($atts['layout'] === 'grid') {
            $output .= '<div class="themisdb-docs-grid">';
            foreach ($files as $file) {
                $output .= '<div class="themisdb-doc-item">';
                $output .= '<h3>' . esc_html($file['name']) . '</h3>';
                $output .= '<p>' . esc_html($file['type']) . '</p>';
                $output .= '</div>';
            }
            $output .= '</div>';
        } else {
            $output .= '<ul>';
            foreach ($files as $file) {
                $icon = $file['type'] === 'dir' ? '📁' : '📄';
                $output .= '<li>' . $icon . ' ' . esc_html($file['name']) . '</li>';
            }
            $output .= '</ul>';
        }
        
        $output .= '</div>';
        
        return $output;
    }
    
    /**
     * Wiki Navigation shortcode (from _Sidebar.md)
     */
    public function wiki_nav_shortcode($atts) {
        $atts = shortcode_atts(array(
            'lang' => get_option('themisdb_wiki_default_lang', 'de'),
            'style' => 'sidebar' // sidebar, horizontal, accordion
        ), $atts);
        
        // Fetch _Sidebar.md file
        $sidebar_content = $this->fetch_github_file('_Sidebar.md', null); // Sidebar is in root docs/
        
        if (is_wp_error($sidebar_content)) {
            return '<div class="themisdb-wiki-nav-error">' . __('Navigation could not be loaded.', 'themisdb-wiki-integration') . '</div>';
        }
        
        // Parse the sidebar markdown to create navigation
        $nav_html = $this->parse_sidebar_to_nav($sidebar_content, $atts['lang'], $atts['style']);
        
        $output = '<nav class="themisdb-wiki-nav themisdb-wiki-nav-' . esc_attr($atts['style']) . '">';
        $output .= $nav_html;
        $output .= '</nav>';
        
        return $output;
    }
    
    /**
     * Parse _Sidebar.md content to navigation HTML
     */
    private function parse_sidebar_to_nav($markdown, $lang, $style) {
        $lines = explode("\n", $markdown);
        $html = '';
        $in_section = false;
        $in_subsection = false;
        $section_title = '';
        
        foreach ($lines as $line) {
            $line = trim($line);
            
            // Skip empty lines and horizontal rules
            if (empty($line) || $line === '---') {
                continue;
            }
            
            // Main heading (h3 - ###)
            if (preg_match('/^###\s+(.+)$/', $line, $matches)) {
                // Close previous subsection if open
                if ($in_subsection) {
                    $html .= '</ul></li>';
                    $in_subsection = false;
                }
                
                // Close previous section
                if ($in_section) {
                    $html .= '</ul></div>';
                }
                
                $section_title = strip_tags($matches[1]);
                $section_id = sanitize_title($section_title);
                
                if ($style === 'accordion') {
                    $html .= '<div class="themisdb-nav-section" data-section="' . esc_attr($section_id) . '">';
                    $html .= '<h3 class="themisdb-nav-section-title">' . esc_html($section_title) . ' <span class="themisdb-nav-toggle">▼</span></h3>';
                    $html .= '<ul class="themisdb-nav-section-items">';
                } else {
                    $html .= '<div class="themisdb-nav-section">';
                    $html .= '<h3 class="themisdb-nav-section-title">' . esc_html($section_title) . '</h3>';
                    $html .= '<ul class="themisdb-nav-section-items">';
                }
                
                $in_section = true;
            }
            // List items with links
            elseif (preg_match('/^-\s+\[(.+?)\]\((.+?)\)/', $line, $matches)) {
                if (!$in_section) {
                    $html .= '<div class="themisdb-nav-section"><ul class="themisdb-nav-section-items">';
                    $in_section = true;
                }
                
                $link_text = $matches[1];
                $link_url = $matches[2];
                
                // Build WordPress-friendly URL
                $wp_url = $this->build_nav_link_url($link_url, $lang);
                
                $html .= '<li class="themisdb-nav-item">';
                $html .= '<a href="' . esc_url($wp_url) . '">' . esc_html($link_text) . '</a>';
                $html .= '</li>';
            }
            // Sub-section heading (h4 - ####)
            elseif (preg_match('/^####\s+(.+)$/', $line, $matches)) {
                if ($in_section) {
                    // Close previous subsection if open
                    if ($in_subsection) {
                        $html .= '</ul></li>';
                    }
                    
                    $subsection_title = strip_tags($matches[1]);
                    $html .= '<li class="themisdb-nav-subsection">';
                    $html .= '<span class="themisdb-nav-subsection-title">' . esc_html($subsection_title) . '</span>';
                    $html .= '<ul class="themisdb-nav-subsection-items">';
                    $in_subsection = true;
                }
            }
        }
        
        // Close subsection if still open
        if ($in_subsection) {
            $html .= '</ul></li>';
        }
        
        // Close last section
        if ($in_section) {
            $html .= '</ul></div>';
        }
        
        return $html;
    }
    
    /**
     * Build WordPress-friendly URL for navigation links
     */
    private function build_nav_link_url($github_path, $lang) {
        // Get current page URL
        $current_url = get_permalink();
        $base_url = trailingslashit(dirname($current_url));
        
        // Clean the GitHub path
        $clean_path = ltrim($github_path, './');
        $clean_path = str_replace('../', '', $clean_path);
        
        // Check if this is an absolute URL
        if (strpos($clean_path, 'http://') === 0 || strpos($clean_path, 'https://') === 0) {
            return $clean_path;
        }
        
        // Build URL with themisdb_wiki shortcode parameter
        // We'll use a query parameter approach
        $slug = str_replace('.md', '', $clean_path);
        
        // Try to find a WordPress page with this shortcode
        // For now, we'll construct a relative URL
        return add_query_arg('doc', $slug, $current_url);
    }
    
    /**
     * Register widgets
     */
    public function register_widgets() {
        register_widget('ThemisDB_Wiki_Nav_Widget');
    }
    
    /**
     * Generate Table of Contents
     */
    private function generate_toc($html) {
        preg_match_all('/<h([2-3])>(.*?)<\/h[2-3]>/i', $html, $matches);
        
        if (empty($matches[0])) {
            return '';
        }
        
        $toc = '<div class="themisdb-wiki-toc"><h2>' . __('Table of Contents', 'themisdb-wiki-integration') . '</h2><ul>';
        
        foreach ($matches[2] as $index => $heading) {
            $level = $matches[1][$index];
            $anchor = sanitize_title($heading);
            $toc .= '<li class="toc-level-' . $level . '"><a href="#' . $anchor . '">' . esc_html($heading) . '</a></li>';
        }
        
        $toc .= '</ul></div>';
        
        return $toc;
    }
    
    /**
     * AJAX: Sync documentation
     */
    public function ajax_sync_docs() {
        check_ajax_referer('themisdb_wiki_nonce', 'nonce');
        
        if (!current_user_can('manage_options')) {
            wp_send_json_error(array('message' => __('Unauthorized', 'themisdb-wiki-integration')));
        }
        
        $this->clear_all_cache();
        
        wp_send_json_success(array('message' => __('Documentation cache cleared successfully.', 'themisdb-wiki-integration')));
    }
    
    /**
     * AJAX: Clear cache
     */
    public function ajax_clear_cache() {
        check_ajax_referer('themisdb_wiki_nonce', 'nonce');
        
        if (!current_user_can('manage_options')) {
            wp_send_json_error(array('message' => __('Unauthorized', 'themisdb-wiki-integration')));
        }
        
        $this->clear_all_cache();
        
        wp_send_json_success(array('message' => __('Cache cleared successfully.', 'themisdb-wiki-integration')));
    }
    
    /**
     * Auto-sync documentation
     */
    public function auto_sync_docs() {
        $this->clear_all_cache();
    }
    
    /**
     * Clear all cache
     */
    private function clear_all_cache() {
        global $wpdb;
        
        // Delete all transients starting with 'github_'
        $wpdb->query(
            "DELETE FROM {$wpdb->options} WHERE option_name LIKE '_transient_github_%' OR option_name LIKE '_transient_timeout_github_%'"
        );
    }
}

// Initialize the plugin
new ThemisDB_Wiki_Integration();

/**
 * ThemisDB Wiki Navigation Widget
 */
class ThemisDB_Wiki_Nav_Widget extends WP_Widget {
    
    /**
     * Constructor
     */
    public function __construct() {
        parent::__construct(
            'themisdb_wiki_nav_widget',
            __('ThemisDB Wiki Navigation', 'themisdb-wiki-integration'),
            array(
                'description' => __('Displays ThemisDB documentation navigation from GitHub wiki', 'themisdb-wiki-integration')
            )
        );
    }
    
    /**
     * Widget output
     */
    public function widget($args, $instance) {
        $title = !empty($instance['title']) ? $instance['title'] : __('Documentation', 'themisdb-wiki-integration');
        $style = !empty($instance['style']) ? $instance['style'] : 'sidebar';
        $lang = !empty($instance['lang']) ? $instance['lang'] : get_option('themisdb_wiki_default_lang', 'de');
        
        echo $args['before_widget'];
        
        if (!empty($title)) {
            echo $args['before_title'] . esc_html($title) . $args['after_title'];
        }
        
        // Use the shortcode to render navigation
        echo do_shortcode('[themisdb_wiki_nav lang="' . esc_attr($lang) . '" style="' . esc_attr($style) . '"]');
        
        echo $args['after_widget'];
    }
    
    /**
     * Widget admin form
     */
    public function form($instance) {
        $title = !empty($instance['title']) ? $instance['title'] : __('Documentation', 'themisdb-wiki-integration');
        $style = !empty($instance['style']) ? $instance['style'] : 'sidebar';
        $lang = !empty($instance['lang']) ? $instance['lang'] : 'de';
        ?>
        <p>
            <label for="<?php echo esc_attr($this->get_field_id('title')); ?>">
                <?php _e('Title:', 'themisdb-wiki-integration'); ?>
            </label>
            <input class="widefat" 
                   id="<?php echo esc_attr($this->get_field_id('title')); ?>" 
                   name="<?php echo esc_attr($this->get_field_name('title')); ?>" 
                   type="text" 
                   value="<?php echo esc_attr($title); ?>">
        </p>
        
        <p>
            <label for="<?php echo esc_attr($this->get_field_id('lang')); ?>">
                <?php _e('Language:', 'themisdb-wiki-integration'); ?>
            </label>
            <select class="widefat" 
                    id="<?php echo esc_attr($this->get_field_id('lang')); ?>" 
                    name="<?php echo esc_attr($this->get_field_name('lang')); ?>">
                <option value="de" <?php selected($lang, 'de'); ?>>Deutsch (DE)</option>
                <option value="en" <?php selected($lang, 'en'); ?>>English (EN)</option>
                <option value="fr" <?php selected($lang, 'fr'); ?>>Français (FR)</option>
            </select>
        </p>
        
        <p>
            <label for="<?php echo esc_attr($this->get_field_id('style')); ?>">
                <?php _e('Style:', 'themisdb-wiki-integration'); ?>
            </label>
            <select class="widefat" 
                    id="<?php echo esc_attr($this->get_field_id('style')); ?>" 
                    name="<?php echo esc_attr($this->get_field_name('style')); ?>">
                <option value="sidebar" <?php selected($style, 'sidebar'); ?>><?php _e('Sidebar', 'themisdb-wiki-integration'); ?></option>
                <option value="accordion" <?php selected($style, 'accordion'); ?>><?php _e('Accordion', 'themisdb-wiki-integration'); ?></option>
                <option value="horizontal" <?php selected($style, 'horizontal'); ?>><?php _e('Horizontal', 'themisdb-wiki-integration'); ?></option>
            </select>
        </p>
        <?php
    }
    
    /**
     * Update widget settings
     */
    public function update($new_instance, $old_instance) {
        $instance = array();
        $instance['title'] = (!empty($new_instance['title'])) ? sanitize_text_field($new_instance['title']) : '';
        $instance['style'] = (!empty($new_instance['style'])) ? sanitize_text_field($new_instance['style']) : 'sidebar';
        $instance['lang'] = (!empty($new_instance['lang'])) ? sanitize_text_field($new_instance['lang']) : 'de';
        return $instance;
    }
}
