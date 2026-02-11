<?php
/**
 * Plugin Name: ThemisDB Wiki Integration
 * Plugin URI: https://github.com/makr-code/ThemisDB
 * Description: Native WordPress wiki with Markdown, [[WikiLinks]], version history, and GitHub Wiki sync
 * Version: 1.0.0
 * Author: ThemisDB Team
 * Author URI: https://github.com/makr-code
 * License: MIT
 * Text Domain: themisdb-wiki
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

// Include required files
require_once THEMISDB_WIKI_PLUGIN_DIR . 'includes/class-markdown-converter.php';
require_once THEMISDB_WIKI_PLUGIN_DIR . 'includes/class-wiki.php';
require_once THEMISDB_WIKI_PLUGIN_DIR . 'includes/class-wikilinks.php';
require_once THEMISDB_WIKI_PLUGIN_DIR . 'includes/class-version-manager.php';
require_once THEMISDB_WIKI_PLUGIN_DIR . 'includes/class-github-sync.php';
require_once THEMISDB_WIKI_PLUGIN_DIR . 'includes/class-search.php';
require_once THEMISDB_WIKI_PLUGIN_DIR . 'includes/class-admin.php';

/**
 * Main Plugin Class
 */
class ThemisDB_Wiki_Integration {
    
    private $wiki;
    private $wikilinks;
    private $version_manager;
    private $github_sync;
    private $search;
    private $admin;
    
    /**
     * Constructor
     */
    public function __construct() {
        // Activation/Deactivation hooks
        register_activation_hook(__FILE__, array($this, 'activate'));
        register_deactivation_hook(__FILE__, array($this, 'deactivate'));
        
        // Initialize plugin
        add_action('plugins_loaded', array($this, 'init'));
        add_action('wp_enqueue_scripts', array($this, 'enqueue_frontend_scripts'));
        add_action('admin_enqueue_scripts', array($this, 'enqueue_admin_scripts'));
        
        // Initialize components
        $this->wiki = new ThemisDB_Wiki();
        $this->wikilinks = new ThemisDB_WikiLinks();
        $this->version_manager = new ThemisDB_Wiki_Version_Manager();
        $this->github_sync = new ThemisDB_Wiki_GitHub_Sync();
        $this->search = new ThemisDB_Wiki_Search();
        $this->admin = new ThemisDB_Wiki_Admin();
        
        // Register shortcodes
        add_shortcode('themisdb_wiki_page', array($this, 'wiki_page_shortcode'));
        add_shortcode('themisdb_wiki_index', array($this, 'wiki_index_shortcode'));
        add_shortcode('themisdb_wiki_recent', array($this, 'wiki_recent_shortcode'));
        add_shortcode('themisdb_wiki_search', array($this, 'wiki_search_shortcode'));
        add_shortcode('themisdb_wiki_toc', array($this, 'wiki_toc_shortcode'));
    }
    
    /**
     * Plugin activation
     */
    public function activate() {
        // Initialize custom post type
        $this->wiki->register_post_type();
        
        // Flush rewrite rules
        flush_rewrite_rules();
        
        // Set default options
        add_option('themisdb_wiki_github_repo', 'makr-code/ThemisDB');
        add_option('themisdb_wiki_github_token', '');
        add_option('themisdb_wiki_github_branch', 'main');
        add_option('themisdb_wiki_sync_direction', 'manual');
        add_option('themisdb_wiki_auto_sync', 'no');
    }
    
    /**
     * Plugin deactivation
     */
    public function deactivate() {
        // Flush rewrite rules
        flush_rewrite_rules();
    }
    
    /**
     * Initialize plugin
     */
    public function init() {
        // Load text domain for translations
        load_plugin_textdomain('themisdb-wiki', false, dirname(plugin_basename(__FILE__)) . '/languages');
    }
    
    /**
     * Enqueue frontend scripts and styles
     */
    public function enqueue_frontend_scripts() {
        global $post;
        
        // Check if we're on a wiki page
        if (is_singular('themisdb_wiki') || is_post_type_archive('themisdb_wiki')) {
            wp_enqueue_style(
                'themisdb-wiki-frontend',
                THEMISDB_WIKI_PLUGIN_URL . 'assets/css/wiki-frontend.css',
                array(),
                THEMISDB_WIKI_VERSION
            );
            
            wp_enqueue_script(
                'themisdb-wiki-wikilinks',
                THEMISDB_WIKI_PLUGIN_URL . 'assets/js/wikilinks.js',
                array('jquery'),
                THEMISDB_WIKI_VERSION,
                true
            );
            
            wp_localize_script('themisdb-wiki-wikilinks', 'themisdbWiki', array(
                'ajaxurl' => admin_url('admin-ajax.php'),
                'adminUrl' => admin_url(),
                'nonce' => wp_create_nonce('themisdb_wiki_nonce')
            ));
        }
    }
    
    /**
     * Enqueue admin scripts and styles
     */
    public function enqueue_admin_scripts($hook) {
        global $post;
        
        // Only load on wiki edit pages
        if (($hook === 'post.php' || $hook === 'post-new.php') && 
            isset($post) && $post->post_type === 'themisdb_wiki') {
            
            // SimpleMDE CSS
            wp_enqueue_style(
                'simplemde',
                'https://cdn.jsdelivr.net/npm/simplemde@1.11.2/dist/simplemde.min.css',
                array(),
                '1.11.2'
            );
            
            wp_enqueue_style(
                'themisdb-wiki-editor',
                THEMISDB_WIKI_PLUGIN_URL . 'assets/css/wiki-editor.css',
                array('simplemde'),
                THEMISDB_WIKI_VERSION
            );
            
            // SimpleMDE JS
            wp_enqueue_script(
                'simplemde',
                'https://cdn.jsdelivr.net/npm/simplemde@1.11.2/dist/simplemde.min.js',
                array(),
                '1.11.2',
                true
            );
            
            wp_enqueue_script(
                'themisdb-markdown-editor',
                THEMISDB_WIKI_PLUGIN_URL . 'assets/js/markdown-editor.js',
                array('jquery', 'simplemde'),
                THEMISDB_WIKI_VERSION,
                true
            );
            
            wp_enqueue_script(
                'jsdiff',
                'https://cdn.jsdelivr.net/npm/diff@5.1.0/dist/diff.min.js',
                array(),
                '5.1.0',
                true
            );
            
            wp_enqueue_script(
                'themisdb-version-diff',
                THEMISDB_WIKI_PLUGIN_URL . 'assets/js/version-diff.js',
                array('jquery', 'jsdiff'),
                THEMISDB_WIKI_VERSION,
                true
            );
            
            wp_localize_script('themisdb-markdown-editor', 'themisdbWiki', array(
                'ajaxurl' => admin_url('admin-ajax.php'),
                'nonce' => wp_create_nonce('themisdb_wiki_nonce'),
                'postId' => isset($post->ID) ? $post->ID : 0,
                'adminUrl' => admin_url()
            ));
        }
        
        // Admin settings page
        if ($hook === 'settings_page_themisdb-wiki') {
            wp_enqueue_style(
                'themisdb-wiki-admin',
                THEMISDB_WIKI_PLUGIN_URL . 'assets/css/wiki-admin.css',
                array(),
                THEMISDB_WIKI_VERSION
            );
        }
    }
    
    /**
     * Wiki page shortcode
     */
    public function wiki_page_shortcode($atts) {
        $atts = shortcode_atts(array(
            'page' => ''
        ), $atts);
        
        if (empty($atts['page'])) {
            return '';
        }
        
        $post = get_page_by_path($atts['page'], OBJECT, 'themisdb_wiki');
        
        if (!$post) {
            return '<div class="wiki-error">Page not found</div>';
        }
        
        return do_shortcode($post->post_content);
    }
    
    /**
     * Wiki index shortcode
     */
    public function wiki_index_shortcode($atts) {
        $atts = shortcode_atts(array(
            'category' => ''
        ), $atts);
        
        $args = array(
            'post_type' => 'themisdb_wiki',
            'posts_per_page' => -1,
            'orderby' => 'title',
            'order' => 'ASC'
        );
        
        if (!empty($atts['category'])) {
            $args['tax_query'] = array(
                array(
                    'taxonomy' => 'wiki_category',
                    'field' => 'slug',
                    'terms' => $atts['category']
                )
            );
        }
        
        $query = new WP_Query($args);
        
        $output = '<div class="wiki-index"><ul>';
        
        while ($query->have_posts()) {
            $query->the_post();
            $output .= '<li><a href="' . get_permalink() . '">' . get_the_title() . '</a></li>';
        }
        
        $output .= '</ul></div>';
        
        wp_reset_postdata();
        
        return $output;
    }
    
    /**
     * Recent wiki changes shortcode
     */
    public function wiki_recent_shortcode($atts) {
        $atts = shortcode_atts(array(
            'limit' => 5
        ), $atts);
        
        $query = new WP_Query(array(
            'post_type' => 'themisdb_wiki',
            'posts_per_page' => intval($atts['limit']),
            'orderby' => 'modified',
            'order' => 'DESC'
        ));
        
        $output = '<div class="wiki-recent"><ul>';
        
        while ($query->have_posts()) {
            $query->the_post();
            $output .= '<li>';
            $output .= '<a href="' . get_permalink() . '">' . get_the_title() . '</a> ';
            $output .= '<span class="wiki-recent-date">' . get_the_modified_date() . '</span>';
            $output .= '</li>';
        }
        
        $output .= '</ul></div>';
        
        wp_reset_postdata();
        
        return $output;
    }
    
    /**
     * Wiki search shortcode
     */
    public function wiki_search_shortcode($atts) {
        return $this->search->get_search_form();
    }
    
    /**
     * Wiki TOC shortcode
     */
    public function wiki_toc_shortcode($atts) {
        $atts = shortcode_atts(array(
            'depth' => 3,
            'title' => 'Contents'
        ), $atts);
        
        return $this->wikilinks->generate_toc_html(get_the_content(), $atts['depth'], $atts['title']);
    }
}

// Initialize the plugin
new ThemisDB_Wiki_Integration();
