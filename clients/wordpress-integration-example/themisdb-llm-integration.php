/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            themisdb-llm-integration.php                       ║
  Version:         0.0.34                                             ║
  Last Modified:   2026-03-09 03:52:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     444                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 65b6fc41e  2026-02-24  fix: resolve remaining Python (34) and PHP (23) error-han... ║
    • a629043ab  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

<?php
/**
 * Plugin Name: ThemisDB LLM Integration
 * Plugin URI: https://github.com/makr-code/ThemisDB
 * Description: Erweitert WordPress mit LLM-Features via ThemisDB - Hybrid-Ansatz
 * Version: 1.0.0
 * Author: ThemisDB Team
 * Author URI: https://github.com/makr-code/ThemisDB
 * License: MIT
 * Text Domain: themisdb-llm
 * Requires at least: 5.0
 * Requires PHP: 7.4
 */

// Prevent direct access
if (!defined('ABSPATH')) {
    exit;
}

// Composer Autoloader (falls installiert)
if (file_exists(__DIR__ . '/vendor/autoload.php')) {
    require_once __DIR__ . '/vendor/autoload.php';
}

use ThemisDB\ThemisClient;

/**
 * Main Plugin Class - Hybrid WordPress + ThemisDB Integration
 * 
 * This plugin demonstrates how to use WordPress with MySQL for core functionality
 * while leveraging ThemisDB for advanced LLM/AI features:
 * - Semantic Search (Vector Embeddings)
 * - Content Recommendations (Graph Traversal)
 * - AI Chat (RAG - Retrieval-Augmented Generation)
 * - Automated Tagging (LLM Analysis)
 */
class ThemisDB_LLM_Integration {
    
    /**
     * @var ThemisClient
     */
    private $client;
    
    /**
     * @var string
     */
    private $version = '1.0.0';
    
    /**
     * Constructor
     */
    public function __construct() {
        // Initialize ThemisDB Client
        $this->init_client();
        
        // Register hooks
        $this->register_hooks();
    }
    
    /**
     * Initialize ThemisDB Client
     */
    private function init_client() {
        $endpoint = get_option('themisdb_endpoint', 'http://localhost:8080');
        $namespace = get_option('themisdb_namespace', 'wordpress');
        
        try {
            $this->client = new ThemisClient(
                [$endpoint],
                ['namespace' => $namespace]
            );
        } catch (Exception $e) {
            error_log("ThemisDB init error: " . $e->getMessage());
            add_action('admin_notices', function() use ($e) {
                echo '<div class="error"><p><strong>ThemisDB Error:</strong> ' . 
                     esc_html($e->getMessage()) . '</p></div>';
            });
        }
    }
    
    /**
     * Register WordPress Hooks
     */
    private function register_hooks() {
        // Admin
        add_action('admin_menu', [$this, 'add_admin_menu']);
        add_action('admin_init', [$this, 'register_settings']);
        
        // Post Synchronization
        add_action('save_post', [$this, 'sync_post_to_themis'], 10, 2);
        add_action('before_delete_post', [$this, 'delete_post_from_themis']);
        
        // AJAX Handlers
        add_action('wp_ajax_themis_semantic_search', [$this, 'ajax_semantic_search']);
        add_action('wp_ajax_nopriv_themis_semantic_search', [$this, 'ajax_semantic_search']);
        add_action('wp_ajax_themis_rag_query', [$this, 'ajax_rag_query']);
        add_action('wp_ajax_nopriv_themis_rag_query', [$this, 'ajax_rag_query']);
        
        // Shortcodes
        add_shortcode('themis_chat', [$this, 'render_chat_widget']);
        add_shortcode('themis_search', [$this, 'render_search_widget']);
        
        // Enqueue Scripts
        add_action('wp_enqueue_scripts', [$this, 'enqueue_frontend_scripts']);
    }
    
    /**
     * Add Admin Menu
     */
    public function add_admin_menu() {
        add_menu_page(
            'ThemisDB LLM',
            'ThemisDB',
            'manage_options',
            'themisdb-llm',
            [$this, 'render_admin_page'],
            'dashicons-database',
            30
        );
    }
    
    /**
     * Register Settings
     */
    public function register_settings() {
        register_setting('themisdb_options', 'themisdb_endpoint');
        register_setting('themisdb_options', 'themisdb_namespace');
        register_setting('themisdb_options', 'themisdb_llm_model');
        register_setting('themisdb_options', 'themisdb_auto_sync');
    }
    
    /**
     * Sync Post to ThemisDB
     */
    public function sync_post_to_themis($post_id, $post) {
        if (!get_option('themisdb_auto_sync', true) || !$this->client) {
            return;
        }
        
        if ($post->post_status !== 'publish' || wp_is_post_autosave($post_id) || wp_is_post_revision($post_id)) {
            return;
        }
        
        try {
            $data = [
                'post_id' => $post_id,
                'title' => $post->post_title,
                'content' => wp_strip_all_tags($post->post_content),
                'author' => get_the_author_meta('display_name', $post->post_author),
                'date' => $post->post_date,
                'categories' => wp_get_post_categories($post_id, ['fields' => 'names']),
                'tags' => wp_get_post_tags($post_id, ['fields' => 'names']),
                'url' => get_permalink($post_id)
            ];
            
            // Store in ThemisDB
            $this->client->put('document', 'wordpress_posts', (string)$post_id, $data);
            
            // Generate embedding
            $this->generate_embedding($post_id, $data);
            
        } catch (Exception $e) {
            error_log('ThemisDB Sync Error: ' . $e->getMessage());
        }
    }
    
    /**
     * Delete Post from ThemisDB
     */
    public function delete_post_from_themis($post_id) {
        if (!$this->client) {
            return;
        }
        
        try {
            $this->client->delete('document', 'wordpress_posts', (string)$post_id);
            $this->client->vectorDelete("post_{$post_id}");
        } catch (Exception $e) {
            error_log('ThemisDB Delete Error: ' . $e->getMessage());
        }
    }
    
    /**
     * Generate Embedding for Post
     */
    private function generate_embedding($post_id, $data) {
        try {
            $content = substr($data['title'] . "\n\n" . $data['content'], 0, 5000);
            
            $result = $this->client->query(
                'LLM EMBED @content USING MODEL "sentence-transformers"',
                ['params' => ['content' => $content]]
            );
            
            if (isset($result['embedding']) && is_array($result['embedding'])) {
                $this->client->vectorUpsert(
                    "post_{$post_id}",
                    $result['embedding'],
                    ['post_id' => $post_id, 'type' => 'wordpress_post', 'title' => $data['title']]
                );
            }
        } catch (Exception $e) {
            error_log('ThemisDB Embedding Error: ' . $e->getMessage());
        }
    }
    
    /**
     * AJAX: Semantic Search
     */
    public function ajax_semantic_search() {
        check_ajax_referer('themis_search', 'nonce');
        
        if (!$this->client) {
            wp_send_json_error('ThemisDB not available');
        }
        
        $query = sanitize_text_field($_POST['query'] ?? '');
        if (empty($query)) {
            wp_send_json_error('Query is required');
        }
        
        try {
            $result = $this->client->query(
                'LLM EMBED @query USING MODEL "sentence-transformers"',
                ['params' => ['query' => $query]]
            );
            
            if (!isset($result['embedding'])) {
                wp_send_json_error('Failed to generate embedding');
            }
            
            $search_results = $this->client->vectorSearch(
                $result['embedding'],
                10,
                ['type' => 'wordpress_post']
            );
            
            $posts = [];
            foreach ($search_results['results'] as $item) {
                $post_id = (int)str_replace('post_', '', $item['id']);
                $post = get_post($post_id);
                
                if ($post && $post->post_status === 'publish') {
                    $posts[] = [
                        'id' => $post_id,
                        'title' => $post->post_title,
                        'excerpt' => get_the_excerpt($post),
                        'url' => get_permalink($post),
                        'score' => $item['score']
                    ];
                }
            }
            
            wp_send_json_success($posts);
            
        } catch (Exception $e) {
            error_log("ThemisDB AJAX error: " . $e->getMessage());
            wp_send_json_error($e->getMessage());
        }
    }
    
    /**
     * AJAX: RAG Query
     */
    public function ajax_rag_query() {
        check_ajax_referer('themis_rag', 'nonce');
        
        if (!$this->client) {
            wp_send_json_error('ThemisDB not available');
        }
        
        $query = sanitize_text_field($_POST['query'] ?? '');
        $model = get_option('themisdb_llm_model', 'llama-2-7b');
        
        try {
            $result = $this->client->query(
                'LLM RAG @query FROM COLLECTION wordpress_posts TOP 5 USING MODEL @model',
                ['params' => ['query' => $query, 'model' => $model]]
            );
            
            wp_send_json_success([
                'answer' => $result['answer'] ?? 'No answer generated',
                'sources' => $result['sources'] ?? []
            ]);
            
        } catch (Exception $e) {
            error_log("ThemisDB RAG query error: " . $e->getMessage());
            wp_send_json_error($e->getMessage());
        }
    }
    
    /**
     * Render Chat Widget Shortcode
     */
    public function render_chat_widget($atts) {
        $nonce = wp_create_nonce('themis_rag');
        
        ob_start();
        ?>
        <div class="themis-chat-widget">
            <div id="themis-chat-messages"></div>
            <div class="themis-chat-input">
                <input type="text" id="themis-chat-text" placeholder="Ask a question...">
                <button id="themis-chat-send">Send</button>
            </div>
        </div>
        <?php
        return ob_get_clean();
    }
    
    /**
     * Render Search Widget Shortcode
     */
    public function render_search_widget($atts) {
        $nonce = wp_create_nonce('themis_search');
        
        ob_start();
        ?>
        <div class="themis-search-widget">
            <input type="text" id="themis-search-input" placeholder="Semantic search...">
            <button id="themis-search-button">Search</button>
            <div id="themis-search-results"></div>
        </div>
        <?php
        return ob_get_clean();
    }
    
    /**
     * Enqueue Frontend Scripts
     */
    public function enqueue_frontend_scripts() {
        wp_enqueue_style(
            'themisdb-llm-frontend',
            plugins_url('assets/css/frontend.css', __FILE__),
            [],
            $this->version
        );
        
        wp_enqueue_script(
            'themisdb-llm-frontend',
            plugins_url('assets/js/frontend.js', __FILE__),
            ['jquery'],
            $this->version,
            true
        );
        
        wp_localize_script('themisdb-llm-frontend', 'themisdb', [
            'ajax_url' => admin_url('admin-ajax.php'),
            'search_nonce' => wp_create_nonce('themis_search'),
            'rag_nonce' => wp_create_nonce('themis_rag')
        ]);
    }
    
    /**
     * Render Admin Page
     */
    public function render_admin_page() {
        ?>
        <div class="wrap">
            <h1>ThemisDB LLM Integration</h1>
            <p>WordPress + ThemisDB Hybrid Integration for LLM Features</p>
            
            <h2>Status</h2>
            <?php if ($this->client): ?>
                <p>✅ Connected to ThemisDB</p>
            <?php else: ?>
                <p>❌ Not connected. Please check settings.</p>
            <?php endif; ?>
            
            <h2>Settings</h2>
            <form method="post" action="options.php">
                <?php
                settings_fields('themisdb_options');
                do_settings_sections('themisdb_options');
                ?>
                <table class="form-table">
                    <tr>
                        <th>ThemisDB Endpoint</th>
                        <td>
                            <input type="text" name="themisdb_endpoint" 
                                   value="<?php echo esc_attr(get_option('themisdb_endpoint', 'http://localhost:8080')); ?>" 
                                   class="regular-text">
                        </td>
                    </tr>
                    <tr>
                        <th>Namespace</th>
                        <td>
                            <input type="text" name="themisdb_namespace" 
                                   value="<?php echo esc_attr(get_option('themisdb_namespace', 'wordpress')); ?>" 
                                   class="regular-text">
                        </td>
                    </tr>
                    <tr>
                        <th>LLM Model</th>
                        <td>
                            <input type="text" name="themisdb_llm_model" 
                                   value="<?php echo esc_attr(get_option('themisdb_llm_model', 'llama-2-7b')); ?>" 
                                   class="regular-text">
                        </td>
                    </tr>
                    <tr>
                        <th>Auto Sync Posts</th>
                        <td>
                            <input type="checkbox" name="themisdb_auto_sync" 
                                   <?php checked(get_option('themisdb_auto_sync', true)); ?>>
                            <label>Automatically sync published posts to ThemisDB</label>
                        </td>
                    </tr>
                </table>
                <?php submit_button(); ?>
            </form>
        </div>
        <?php
    }
}

// Initialize Plugin
function themisdb_llm_init() {
    new ThemisDB_LLM_Integration();
}
add_action('plugins_loaded', 'themisdb_llm_init');
