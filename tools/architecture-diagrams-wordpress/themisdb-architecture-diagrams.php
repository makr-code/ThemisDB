<?php
/**
 * Plugin Name: ThemisDB Architecture Diagrams
 * Plugin URI: https://github.com/makr-code/ThemisDB
 * Description: Interactive architecture diagrams for ThemisDB. Visualize multi-model architecture, storage layer, LLM integration, and sharding with Mermaid.js. Use shortcode [themisdb_architecture] to embed.
 * Version: 1.0.0
 * Author: ThemisDB Team
 * Author URI: https://github.com/makr-code/ThemisDB
 * License: MIT
 * License URI: https://opensource.org/licenses/MIT
 * Text Domain: themisdb-architecture-diagrams
 * Domain Path: /languages
 * Requires at least: 5.0
 * Requires PHP: 7.4
 */

// Exit if accessed directly
if (!defined('ABSPATH')) {
    exit;
}

// Define plugin constants
define('THEMISDB_AD_VERSION', '1.0.0');
define('THEMISDB_AD_PLUGIN_DIR', plugin_dir_path(__FILE__));
define('THEMISDB_AD_PLUGIN_URL', plugin_dir_url(__FILE__));
define('THEMISDB_AD_PLUGIN_FILE', __FILE__);
define('THEMISDB_AD_GITHUB_REPO', 'makr-code/ThemisDB');
define('THEMISDB_AD_GITHUB_PATH', 'tools/architecture-diagrams-wordpress');

/**
 * Main Plugin Class
 */
class ThemisDB_Architecture_Diagrams {
    
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
        add_shortcode('themisdb_architecture', array($this, 'render_diagram'));
        
        // Admin menu
        add_action('admin_menu', array($this, 'add_admin_menu'));
        add_action('admin_init', array($this, 'register_settings'));
        
        // Plugin action links
        add_filter('plugin_action_links_' . plugin_basename(THEMISDB_AD_PLUGIN_FILE), array($this, 'add_action_links'));
        
        // AJAX endpoints
        add_action('wp_ajax_themisdb_ad_get_diagram', array($this, 'ajax_get_diagram'));
        add_action('wp_ajax_nopriv_themisdb_ad_get_diagram', array($this, 'ajax_get_diagram'));
    }
    
    /**
     * Plugin activation
     */
    public function activate() {
        // Set default options
        $defaults = array(
            'default_view' => 'high_level',
            'theme' => 'neutral',
            'interactive' => true,
            'enable_export' => true,
            'show_descriptions' => true,
            'default_zoom' => 100,
        );
        
        foreach ($defaults as $key => $value) {
            if (get_option('themisdb_ad_' . $key) === false) {
                add_option('themisdb_ad_' . $key, $value);
            }
        }
    }
    
    /**
     * Plugin deactivation
     */
    public function deactivate() {
        // Clean up transients
        delete_transient('themisdb_ad_cached_diagrams');
    }
    
    /**
     * Initialize plugin
     */
    public function init() {
        // Load text domain
        load_plugin_textdomain('themisdb-architecture-diagrams', false, dirname(plugin_basename(__FILE__)) . '/languages');
    }
    
    /**
     * Enqueue assets
     */
    public function enqueue_assets() {
        global $post;
        
        // Only load if shortcode is present
        if (!is_a($post, 'WP_Post') || !has_shortcode($post->post_content, 'themisdb_architecture')) {
            return;
        }
        
        // Mermaid.js from CDN
        wp_enqueue_script(
            'mermaid-js',
            'https://cdn.jsdelivr.net/npm/mermaid@10/dist/mermaid.min.js',
            array(),
            '10.0.0',
            true
        );
        
        // Plugin CSS
        wp_enqueue_style(
            'themisdb-ad-style',
            THEMISDB_AD_PLUGIN_URL . 'assets/css/architecture-diagrams.css',
            array(),
            THEMISDB_AD_VERSION
        );
        
        // Plugin JS
        wp_enqueue_script(
            'themisdb-ad-script',
            THEMISDB_AD_PLUGIN_URL . 'assets/js/architecture-diagrams.js',
            array('jquery', 'mermaid-js'),
            THEMISDB_AD_VERSION,
            true
        );
        
        // Localize script with AJAX URL and settings
        wp_localize_script('themisdb-ad-script', 'themisdbAD', array(
            'ajax_url' => admin_url('admin-ajax.php'),
            'nonce' => wp_create_nonce('themisdb_ad_nonce'),
            'plugin_url' => THEMISDB_AD_PLUGIN_URL,
            'settings' => array(
                'default_view' => get_option('themisdb_ad_default_view', 'high_level'),
                'theme' => get_option('themisdb_ad_theme', 'neutral'),
                'interactive' => get_option('themisdb_ad_interactive', true),
                'enable_export' => get_option('themisdb_ad_enable_export', true),
                'show_descriptions' => get_option('themisdb_ad_show_descriptions', true),
            ),
        ));
    }
    
    /**
     * Render architecture diagram
     */
    public function render_diagram($atts) {
        $atts = shortcode_atts(array(
            'view' => get_option('themisdb_ad_default_view', 'high_level'),
            'theme' => get_option('themisdb_ad_theme', 'neutral'),
            'interactive' => get_option('themisdb_ad_interactive', true),
            'show_controls' => 'true',
        ), $atts, 'themisdb_architecture');
        
        // Load template
        ob_start();
        include THEMISDB_AD_PLUGIN_DIR . 'templates/diagram.php';
        return ob_get_clean();
    }
    
    /**
     * Add admin menu
     */
    public function add_admin_menu() {
        add_options_page(
            __('Architecture Diagrams Settings', 'themisdb-architecture-diagrams'),
            __('Architecture Diagrams', 'themisdb-architecture-diagrams'),
            'manage_options',
            'themisdb-ad-settings',
            array($this, 'render_settings_page')
        );
    }
    
    /**
     * Register settings
     */
    public function register_settings() {
        register_setting('themisdb_ad_settings', 'themisdb_ad_default_view');
        register_setting('themisdb_ad_settings', 'themisdb_ad_theme');
        register_setting('themisdb_ad_settings', 'themisdb_ad_interactive');
        register_setting('themisdb_ad_settings', 'themisdb_ad_enable_export');
        register_setting('themisdb_ad_settings', 'themisdb_ad_show_descriptions');
        register_setting('themisdb_ad_settings', 'themisdb_ad_default_zoom');
    }
    
    /**
     * Render settings page
     */
    public function render_settings_page() {
        if (!current_user_can('manage_options')) {
            return;
        }
        
        include THEMISDB_AD_PLUGIN_DIR . 'templates/admin-settings.php';
    }
    
    /**
     * Add plugin action links
     */
    public function add_action_links($links) {
        $settings_link = '<a href="' . admin_url('options-general.php?page=themisdb-ad-settings') . '">' . __('Settings', 'themisdb-architecture-diagrams') . '</a>';
        array_unshift($links, $settings_link);
        return $links;
    }
    
    /**
     * AJAX handler to get diagram code
     */
    public function ajax_get_diagram() {
        check_ajax_referer('themisdb_ad_nonce', 'nonce');
        
        $view = isset($_POST['view']) ? sanitize_text_field($_POST['view']) : 'high_level';
        
        // Get diagram code
        $diagram_code = $this->get_diagram_code($view);
        
        wp_send_json_success(array(
            'code' => $diagram_code,
            'view' => $view,
        ));
    }
    
    /**
     * Get Mermaid diagram code for specified view
     */
    private function get_diagram_code($view) {
        $diagrams = array(
            'high_level' => $this->get_high_level_diagram(),
            'storage_layer' => $this->get_storage_layer_diagram(),
            'llm_integration' => $this->get_llm_integration_diagram(),
            'sharding_raid' => $this->get_sharding_raid_diagram(),
        );
        
        return isset($diagrams[$view]) ? $diagrams[$view] : $diagrams['high_level'];
    }
    
    /**
     * High-level architecture diagram
     */
    private function get_high_level_diagram() {
        return "graph TB
    subgraph Client[\"Client Layer\"]
        CLI[CLI Client]
        REST[REST API Client]
        GRPC[gRPC Client]
        SDK[SDK Clients]
    end
    
    subgraph API[\"API Layer\"]
        RESTAPI[REST API Server]
        GRPCAPI[gRPC Server]
        Auth[Authentication]
    end
    
    subgraph Query[\"Query Engine\"]
        AQL[AQL Parser]
        OPT[Query Optimizer]
        EXEC[Execution Engine]
    end
    
    subgraph Storage[\"Storage Layer\"]
        ROCKS[(RocksDB)]
        VECTOR[Vector Index HNSW]
        GRAPH[Graph Store]
        DOC[Document Store]
    end
    
    subgraph AI[\"AI/LLM Layer\"]
        LLAMA[llama.cpp Engine]
        MODELS[LLM Models]
        EMBED[Embeddings]
    end
    
    CLI --> RESTAPI
    REST --> RESTAPI
    GRPC --> GRPCAPI
    SDK --> RESTAPI
    SDK --> GRPCAPI
    
    RESTAPI --> Auth
    GRPCAPI --> Auth
    Auth --> AQL
    
    AQL --> OPT
    OPT --> EXEC
    
    EXEC --> ROCKS
    EXEC --> VECTOR
    EXEC --> GRAPH
    EXEC --> DOC
    
    RESTAPI --> LLAMA
    GRPCAPI --> LLAMA
    LLAMA --> MODELS
    LLAMA --> EMBED
    
    style ROCKS fill:#2ea44f
    style VECTOR fill:#2ea44f
    style GRAPH fill:#2ea44f
    style DOC fill:#2ea44f
    style LLAMA fill:#3498db";
    }
    
    /**
     * Storage layer diagram
     */
    private function get_storage_layer_diagram() {
        return "graph TB
    subgraph Execution[\"Execution Layer\"]
        EXEC[Query Executor]
    end
    
    subgraph Storage[\"Storage Engine\"]
        ROCKS[(RocksDB Base)]
        
        subgraph Indexes[\"Index Layer\"]
            VECTOR[Vector Index HNSW]
            GRAPH[Graph Index]
            FULL[Full-Text Index]
            SPATIAL[Spatial Index]
        end
        
        subgraph Data[\"Data Layer\"]
            DOC[Document Store]
            KV[Key-Value Store]
            TS[Time Series]
            BLOB[Blob Storage]
        end
        
        subgraph Persistence[\"Persistence\"]
            WAL[Write-Ahead Log]
            SST[SST Files]
            MANIFEST[Manifest]
        end
    end
    
    EXEC --> VECTOR
    EXEC --> GRAPH
    EXEC --> FULL
    EXEC --> SPATIAL
    EXEC --> DOC
    EXEC --> KV
    EXEC --> TS
    EXEC --> BLOB
    
    VECTOR --> ROCKS
    GRAPH --> ROCKS
    FULL --> ROCKS
    SPATIAL --> ROCKS
    DOC --> ROCKS
    KV --> ROCKS
    TS --> ROCKS
    BLOB --> ROCKS
    
    ROCKS --> WAL
    ROCKS --> SST
    ROCKS --> MANIFEST
    
    style ROCKS fill:#2ea44f
    style WAL fill:#f39c12
    style SST fill:#f39c12
    style MANIFEST fill:#f39c12";
    }
    
    /**
     * LLM integration diagram
     */
    private function get_llm_integration_diagram() {
        return "graph LR
    subgraph Client[\"Client Applications\"]
        APP[Application]
    end
    
    subgraph API[\"ThemisDB API\"]
        REST[REST Endpoint]
        LLM_API[LLM API]
    end
    
    subgraph LLM[\"LLM Engine\"]
        LLAMA[llama.cpp]
        
        subgraph Models[\"Model Management\"]
            LOADER[Model Loader]
            CACHE[Model Cache]
            QUANT[Quantization]
        end
        
        subgraph Inference[\"Inference Engine\"]
            PROMPT[Prompt Processing]
            TOKENS[Tokenization]
            GEN[Generation]
            SAMPLE[Sampling]
        end
        
        subgraph Optimization[\"Optimization\"]
            CUDA[CUDA Support]
            METAL[Metal Support]
            SIMD[SIMD Vectorization]
        end
    end
    
    subgraph Storage[\"Storage\"]
        MODELS_DB[(Model Files)]
        EMBED_DB[(Embeddings)]
        VECTOR_IDX[Vector Index]
    end
    
    APP --> REST
    REST --> LLM_API
    
    LLM_API --> LLAMA
    LLAMA --> LOADER
    LOADER --> MODELS_DB
    LOADER --> CACHE
    CACHE --> QUANT
    
    LLAMA --> PROMPT
    PROMPT --> TOKENS
    TOKENS --> GEN
    GEN --> SAMPLE
    
    LLAMA --> CUDA
    LLAMA --> METAL
    LLAMA --> SIMD
    
    LLAMA --> EMBED_DB
    EMBED_DB --> VECTOR_IDX
    
    style LLAMA fill:#3498db
    style CUDA fill:#27ae60
    style METAL fill:#27ae60
    style VECTOR_IDX fill:#2ea44f";
    }
    
    /**
     * Sharding/RAID diagram
     */
    private function get_sharding_raid_diagram() {
        return "graph TB
    subgraph Client[\"Client Layer\"]
        APP[Application]
    end
    
    subgraph Coordinator[\"Coordination Layer\"]
        ROUTER[Query Router]
        SHARD_MAP[Shard Map]
        REPL_MGR[Replication Manager]
    end
    
    subgraph Shards[\"Distributed Shards\"]
        subgraph Shard1[\"Shard 1 RAID Group\"]
            S1P[Primary Node]
            S1R1[Replica 1]
            S1R2[Replica 2]
        end
        
        subgraph Shard2[\"Shard 2 RAID Group\"]
            S2P[Primary Node]
            S2R1[Replica 1]
            S2R2[Replica 2]
        end
        
        subgraph Shard3[\"Shard 3 RAID Group\"]
            S3P[Primary Node]
            S3R1[Replica 1]
            S3R2[Replica 2]
        end
    end
    
    subgraph Consensus[\"Consensus Layer\"]
        RAFT[Raft Protocol]
    end
    
    APP --> ROUTER
    ROUTER --> SHARD_MAP
    
    ROUTER --> S1P
    ROUTER --> S2P
    ROUTER --> S3P
    
    S1P --> S1R1
    S1P --> S1R2
    S2P --> S2R1
    S2P --> S2R2
    S3P --> S3R1
    S3P --> S3R2
    
    S1P --> RAFT
    S2P --> RAFT
    S3P --> RAFT
    
    REPL_MGR --> S1P
    REPL_MGR --> S2P
    REPL_MGR --> S3P
    
    style S1P fill:#2ea44f
    style S2P fill:#2ea44f
    style S3P fill:#2ea44f
    style RAFT fill:#e74c3c";
    }
}

// Initialize plugin
function themisdb_architecture_diagrams_init() {
    return ThemisDB_Architecture_Diagrams::get_instance();
}

add_action('plugins_loaded', 'themisdb_architecture_diagrams_init');
