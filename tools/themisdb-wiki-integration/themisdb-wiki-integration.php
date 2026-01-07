<?php
/**
 * Plugin Name: ThemisDB Wiki Integration
 * Plugin URI: https://github.com/makr-code/ThemisDB
 * Description: Automatically integrates ThemisDB documentation/wiki from GitHub into WordPress. Fetches markdown files and displays them with proper formatting.
 * Version: 1.0.0
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
define('THEMISDB_WIKI_VERSION', '1.0.0');
define('THEMISDB_WIKI_PLUGIN_DIR', plugin_dir_path(__FILE__));
define('THEMISDB_WIKI_PLUGIN_URL', plugin_dir_url(__FILE__));
define('THEMISDB_WIKI_CACHE_GROUP', 'themisdb_wiki');
define('THEMISDB_WIKI_CACHE_EXPIRATION', 3600); // 1 hour

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
        
        // AJAX handlers for auto-sync
        add_action('wp_ajax_themisdb_sync_docs', array($this, 'ajax_sync_docs'));
        add_action('wp_ajax_themisdb_clear_cache', array($this, 'ajax_clear_cache'));
    }
    
    /**
     * Plugin activation
     */
    public function activate() {
        // Set default options
        add_option('themisdb_wiki_github_repo', 'makr-code/ThemisDB');
        add_option('themisdb_wiki_github_branch', 'main');
        add_option('themisdb_wiki_docs_path', 'docs');
        add_option('themisdb_wiki_auto_sync', 'yes');
        add_option('themisdb_wiki_sync_interval', '3600');
        add_option('themisdb_wiki_default_lang', 'de');
        
        // Schedule auto-sync if enabled
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
        if (is_a($post, 'WP_Post') && (has_shortcode($post->post_content, 'themisdb_wiki') || has_shortcode($post->post_content, 'themisdb_docs'))) {
            wp_enqueue_style(
                'themisdb-wiki-style',
                THEMISDB_WIKI_PLUGIN_URL . 'assets/css/wiki-integration.css',
                array(),
                THEMISDB_WIKI_VERSION
            );
            
            wp_enqueue_script(
                'themisdb-wiki-script',
                THEMISDB_WIKI_PLUGIN_URL . 'assets/js/wiki-integration.js',
                array('jquery'),
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
        $repo = get_option('themisdb_wiki_github_repo', 'makr-code/ThemisDB');
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
     * Convert Markdown to HTML
     */
    private function markdown_to_html($markdown) {
        // Basic Markdown conversion with XSS protection
        // For production, consider using a library like Parsedown
        
        // Sanitize input first
        $markdown = wp_kses_post($markdown);
        
        $html = $markdown;
        
        // Headers
        $html = preg_replace('/^### (.*?)$/m', '<h3>$1</h3>', $html);
        $html = preg_replace('/^## (.*?)$/m', '<h2>$1</h2>', $html);
        $html = preg_replace('/^# (.*?)$/m', '<h1>$1</h1>', $html);
        
        // Bold and Italic
        $html = preg_replace('/\*\*\*(.*?)\*\*\*/s', '<strong><em>$1</em></strong>', $html);
        $html = preg_replace('/\*\*(.*?)\*\*/s', '<strong>$1</strong>', $html);
        $html = preg_replace('/\*(.*?)\*/s', '<em>$1</em>', $html);
        
        // Links (with URL validation)
        $html = preg_replace_callback('/\[(.*?)\]\((.*?)\)/', function($matches) {
            $text = esc_html($matches[1]);
            $url = esc_url($matches[2]);
            return '<a href="' . $url . '" target="_blank" rel="noopener noreferrer">' . $text . '</a>';
        }, $html);
        
        // Code blocks
        $html = preg_replace_callback('/```(.*?)```/s', function($matches) {
            return '<pre><code>' . esc_html($matches[1]) . '</code></pre>';
        }, $html);
        $html = preg_replace_callback('/`(.*?)`/', function($matches) {
            return '<code>' . esc_html($matches[1]) . '</code>';
        }, $html);
        
        // Lists - improved to handle multiple lists correctly
        $lines = explode("\n", $html);
        $in_list = false;
        $result = array();
        
        foreach ($lines as $line) {
            if (preg_match('/^[\*\-] (.*)$/', $line, $matches)) {
                if (!$in_list) {
                    $result[] = '<ul>';
                    $in_list = true;
                }
                $result[] = '<li>' . $matches[1] . '</li>';
            } else {
                if ($in_list) {
                    $result[] = '</ul>';
                    $in_list = false;
                }
                $result[] = $line;
            }
        }
        
        if ($in_list) {
            $result[] = '</ul>';
        }
        
        $html = implode("\n", $result);
        
        // Paragraphs
        $html = preg_replace('/\n\n/', '</p><p>', $html);
        $html = '<p>' . $html . '</p>';
        
        // Final sanitization
        $allowed_html = array(
            'h1' => array(), 'h2' => array(), 'h3' => array(),
            'p' => array(), 'br' => array(),
            'strong' => array(), 'em' => array(),
            'ul' => array(), 'ol' => array(), 'li' => array(),
            'a' => array('href' => array(), 'target' => array(), 'rel' => array()),
            'code' => array(), 'pre' => array(),
            'blockquote' => array()
        );
        
        return wp_kses($html, $allowed_html);
    }
    
    /**
     * List available documentation files
     */
    private function list_docs_files($lang = null) {
        $repo = get_option('themisdb_wiki_github_repo', 'makr-code/ThemisDB');
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
