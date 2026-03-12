<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            themisdb-architecture-diagrams.php                 ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:16                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     1275                                           ║
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
 * Plugin Name: ThemisDB Architecture Diagrams
 * Plugin URI: https://github.com/makr-code/wordpressPlugins
 * Description: Interactive architecture diagrams for ThemisDB. Visualize multi-model architecture, storage layer, LLM integration, and sharding with Mermaid.js. Use shortcode [themisdb_architecture] to embed.
 * Version: 1.1.0
 * Author: ThemisDB Team
 * Author URI: https://github.com/makr-code/wordpressPlugins
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
define('THEMISDB_AD_VERSION', '1.1.0');
define('THEMISDB_AD_PLUGIN_DIR', plugin_dir_path(__FILE__));
define('THEMISDB_AD_PLUGIN_URL', plugin_dir_url(__FILE__));
define('THEMISDB_AD_PLUGIN_FILE', __FILE__);
define('THEMISDB_AD_GITHUB_REPO', 'makr-code/wordpressPlugins');
define('THEMISDB_AD_GITHUB_PATH', 'tools/architecture-diagrams-wordpress');

// Load updater class
$themisdb_updater_local = THEMISDB_AD_PLUGIN_DIR . 'includes/class-themisdb-plugin-updater.php';
$themisdb_updater_shared = dirname(THEMISDB_AD_PLUGIN_DIR) . '/includes/class-themisdb-plugin-updater.php';

if (file_exists($themisdb_updater_local)) {
    require_once $themisdb_updater_local;
} elseif (file_exists($themisdb_updater_shared)) {
    require_once $themisdb_updater_shared;
}

// Initialize automatic updates
if (class_exists('ThemisDB_Plugin_Updater')) {
    new ThemisDB_Plugin_Updater(
        THEMISDB_AD_PLUGIN_FILE,
        'themisdb-architecture-diagrams',
        THEMISDB_AD_VERSION
    );
}

/**
 * Detect color scheme (light/dark)
 * 
 * @return string 'light' or 'dark'
 */
function themisdb_arch_get_color_scheme() {
    // Priority 1: User cookie preference
    if (isset($_COOKIE['themisdb_color_scheme'])) {
        return sanitize_text_field($_COOKIE['themisdb_color_scheme']);
    }
    
    // Priority 2: WordPress theme setting
    $theme_mod = get_theme_mod('color_scheme', 'light');
    if ($theme_mod === 'dark') {
        return 'dark';
    }
    
    // Priority 3: Check if common dark mode plugins are active
    if (function_exists('is_plugin_active')) {
        if (is_plugin_active('wp-dark-mode/wp-dark-mode.php') || 
            is_plugin_active('dark-mode/dark-mode.php')) {
            return 'dark';
        }
    }
    
    // Priority 4: Admin setting (not currently used for auto-detection)
    // Future: Could implement time-based or other auto-detection logic here
    
    return 'light';
}

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
        add_action('wp_head', array($this, 'add_preload'), 1);
        
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
            'theme' => 'themis',
            'interactive' => true,
            'enable_export' => true,
            'show_descriptions' => true,
            'default_zoom' => 100,
            'enable_dark_mode' => 1,
            'enable_lazy_loading' => 1,
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
        
        // Detect color scheme
        $color_scheme = themisdb_arch_get_color_scheme();
        
        // Mermaid.js ESM from CDN - load in header to ensure it's available before plugin script
        wp_enqueue_script(
            'mermaid-js',
            'https://cdn.jsdelivr.net/npm/mermaid@10.6.1/dist/mermaid.esm.min.mjs',
            array(),
            '10.6.1',
            false
        );
        
        // Plugin CSS
        wp_enqueue_style(
            'themisdb-ad-style',
            THEMISDB_AD_PLUGIN_URL . 'assets/css/architecture-diagrams.css',
            array(),
            THEMISDB_AD_VERSION
        );
        
        // Dark mode styles
        if ($color_scheme === 'dark') {
            wp_enqueue_style(
                'themisdb-ad-dark',
                THEMISDB_AD_PLUGIN_URL . 'assets/css/architecture-diagrams-dark.css',
                array('themisdb-ad-style'),
                THEMISDB_AD_VERSION
            );
        }
        
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
            'colorScheme' => $color_scheme,
            'settings' => array(
                'default_view' => get_option('themisdb_ad_default_view', 'high_level'),
                'theme' => get_option('themisdb_ad_theme', 'themis'),
                'interactive' => get_option('themisdb_ad_interactive', true),
                'enable_export' => get_option('themisdb_ad_enable_export', true),
                'show_descriptions' => get_option('themisdb_ad_show_descriptions', true),
                'enableLazyLoading' => get_option('themisdb_ad_enable_lazy_loading', 1),
            ),
        ));
    }
    
    /**
     * Add preload for Mermaid.js
     */
    public function add_preload() {
        global $post;
        
        if (is_a($post, 'WP_Post') && has_shortcode($post->post_content, 'themisdb_architecture')) {
            echo '<link rel="modulepreload" href="https://cdn.jsdelivr.net/npm/mermaid@10.6.1/dist/mermaid.esm.min.mjs">' . "\n";
        }
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
        
        // NEW: v1.1.0 Settings
        register_setting('themisdb_ad_settings', 'themisdb_ad_enable_dark_mode', array(
            'type' => 'boolean',
            'default' => 1,
            'sanitize_callback' => 'absint'
        ));
        
        register_setting('themisdb_ad_settings', 'themisdb_ad_enable_lazy_loading', array(
            'type' => 'boolean',
            'default' => 1,
            'sanitize_callback' => 'absint'
        ));
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
            'database_comparison' => $this->get_database_comparison_diagram(),
            'llm_comparison' => $this->get_llm_comparison_diagram(),
            'hardware_architecture' => $this->get_hardware_architecture_diagram(),
            'performance_comparison' => $this->get_performance_comparison_diagram(),
            'tco_comparison' => $this->get_tco_comparison_diagram(),
            'feature_matrix' => $this->get_feature_matrix_diagram(),
            'deployment_options' => $this->get_deployment_options_diagram(),
            'use_case_recommendations' => $this->get_use_case_recommendations_diagram(),
            'migration_paths' => $this->get_migration_paths_diagram(),
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
    
    subgraph Server[\"API & Server Layer\"]
        RESTAPI[HTTP/REST Server]
        GRPCAPI[gRPC Server]
        Auth[Authentication & Rate Limiting]
    end
    
    subgraph QueryLayer[\"Query Layer (Unified Programming Layer)\"]
        AQL[AQL Parser]
        OPT[Query Optimizer]
        EXEC[Execution Engine]
        FUNC[Function Libraries]
        LLM_FUNC[LLM Functions Optional]
    end
    
    subgraph Transaction[\"Transaction & Concurrency Layer\"]
        MVCC[MVCC]
        TXN[Transaction Manager]
        WAL[WAL Management]
    end
    
    subgraph Index[\"Index Layer\"]
        VECTOR[Vector Index HNSW]
        GRAPH[Graph Index]
        SECONDARY[Secondary Indexes]
        FULLTEXT[Fulltext Index]
    end
    
    subgraph Storage[\"Storage Layer\"]
        ROCKS[(RocksDB LSM-tree)]
        COMPRESS[Compression]
        SNAPSHOT[Snapshot Management]
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
    EXEC --> FUNC
    FUNC -.-> LLM_FUNC
    
    EXEC --> TXN
    TXN --> MVCC
    MVCC --> WAL
    
    EXEC --> VECTOR
    EXEC --> GRAPH
    EXEC --> SECONDARY
    EXEC --> FULLTEXT
    
    VECTOR --> ROCKS
    GRAPH --> ROCKS
    SECONDARY --> ROCKS
    FULLTEXT --> ROCKS
    
    ROCKS --> COMPRESS
    ROCKS --> SNAPSHOT
    
    style QueryLayer fill:#3498db
    style EXEC fill:#2ea44f
    style LLM_FUNC fill:#9b59b6
    style ROCKS fill:#2ea44f
    style VECTOR fill:#27ae60
    style GRAPH fill:#27ae60";
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
        return "graph TB
    subgraph Client[\"Client Applications\"]
        APP[Application]
    end
    
    subgraph Server[\"API & Server Layer\"]
        HTTP[HTTP/REST Server]
        GRPC[gRPC Server]
    end
    
    subgraph QueryLayer[\"Query Layer (Unified Programming Layer)\"]
        AQL[AQL Parser]
        EXEC[Execution Engine]
        FUNC[Function Libraries]
        LLM_FUNC[\"LLM Functions (Optional)<br/>llm_generate(), llm_embed()\"]
    end
    
    subgraph LLM[\"LLM Plugin (Optional - llama.cpp)\"]
        subgraph Models[\"Model Management\"]
            LOADER[Model Loader]
            CACHE[Model Cache]
            QUANT[Quantization Q4/Q5/Q8]
        end
        
        subgraph Inference[\"Inference Engine\"]
            PROMPT[Prompt Processing]
            TOKENS[Tokenization]
            GEN[Text Generation]
            SAMPLE[Sampling]
        end
        
        subgraph Optimization[\"Hardware Optimization\"]
            CUDA[CUDA GPU]
            METAL[Metal GPU]
            SIMD[CPU SIMD AVX2/512]
        end
    end
    
    subgraph Storage[\"Storage Layer\"]
        MODELS_DB[(Model Files<br/>GGUF Format)]
        EMBED_DB[(Embeddings Cache)]
        VECTOR_IDX[Vector Index HNSW]
        ROCKS[(RocksDB)]
    end
    
    APP --> HTTP
    APP --> GRPC
    HTTP --> AQL
    GRPC --> AQL
    
    AQL --> EXEC
    EXEC --> FUNC
    FUNC --> LLM_FUNC
    
    LLM_FUNC -.->|\"Optional Plugin\"| LOADER
    LLM_FUNC -.->|\"Optional Plugin\"| PROMPT
    
    LOADER --> CACHE
    CACHE --> QUANT
    QUANT --> MODELS_DB
    
    PROMPT --> TOKENS
    TOKENS --> GEN
    GEN --> SAMPLE
    
    GEN --> CUDA
    GEN --> METAL
    GEN --> SIMD
    
    LLM_FUNC --> EMBED_DB
    EMBED_DB --> VECTOR_IDX
    VECTOR_IDX --> ROCKS
    
    style QueryLayer fill:#3498db
    style LLM_FUNC fill:#9b59b6
    style LOADER fill:#9b59b6
    style PROMPT fill:#9b59b6
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
    
    /**
     * Database comparison diagram
     */
    private function get_database_comparison_diagram() {
        return "graph TB
    subgraph ThemisDB[\"ThemisDB Architecture\"]
        TDB_API[\"Unified API<br/>(REST, gRPC, PostgreSQL Wire)\"]
        TDB_MULTI[\"Multi-Model Engine<br/>(Relational, Graph, Vector, Document)\"]
        TDB_LLM[\"Embedded LLM<br/>(llama.cpp, No API Costs)\"]
        TDB_STORAGE[\"RocksDB + HNSW<br/>(ACID, Vector Search)\"]
        TDB_GPU[\"GPU Support<br/>(CUDA, Metal, Vulkan)\"]
    end
    
    subgraph PostgreSQL[\"PostgreSQL\"]
        PG_API[\"SQL API Only\"]
        PG_REL[\"Relational Only<br/>(+ pgvector extension)\"]
        PG_NO_LLM[\"No LLM<br/>(External API Required)\"]
        PG_STORAGE[\"B-Tree Storage<br/>(ACID)\"]
        PG_CPU[\"CPU Only\"]
    end
    
    subgraph MongoDB[\"MongoDB\"]
        MG_API[\"MongoDB Wire Protocol\"]
        MG_DOC[\"Document Model<br/>(+ Atlas Vector Search)\"]
        MG_NO_LLM[\"No LLM<br/>(External API Required)\"]
        MG_STORAGE[\"WiredTiger<br/>(Eventual Consistency)\"]
        MG_CPU[\"CPU Only\"]
    end
    
    subgraph Neo4j[\"Neo4j\"]
        NJ_API[\"Cypher API\"]
        NJ_GRAPH[\"Graph Only<br/>(+ Vector Plugin)\"]
        NJ_NO_LLM[\"No LLM<br/>(External API Required)\"]
        NJ_STORAGE[\"Native Graph Store<br/>(ACID)\"]
        NJ_CPU[\"CPU Only\"]
    end
    
    TDB_API -.->|\"Supports All\"| PG_API
    TDB_API -.->|\"Supports All\"| MG_API
    TDB_API -.->|\"Supports All\"| NJ_API
    
    TDB_MULTI -.->|\"Includes\"| PG_REL
    TDB_MULTI -.->|\"Includes\"| MG_DOC
    TDB_MULTI -.->|\"Includes\"| NJ_GRAPH
    
    TDB_LLM -.->|\"Built-in vs External\"| PG_NO_LLM
    TDB_LLM -.->|\"Built-in vs External\"| MG_NO_LLM
    TDB_LLM -.->|\"Built-in vs External\"| NJ_NO_LLM
    
    TDB_GPU -.->|\"GPU Accelerated\"| PG_CPU
    TDB_GPU -.->|\"GPU Accelerated\"| MG_CPU
    TDB_GPU -.->|\"GPU Accelerated\"| NJ_CPU
    
    style TDB_API fill:#2ea44f
    style TDB_MULTI fill:#2ea44f
    style TDB_LLM fill:#3498db
    style TDB_STORAGE fill:#2ea44f
    style TDB_GPU fill:#27ae60
    style PG_API fill:#cccccc
    style MG_API fill:#cccccc
    style NJ_API fill:#cccccc";
    }
    
    /**
     * LLM comparison diagram
     */
    private function get_llm_comparison_diagram() {
        return "graph TB
    subgraph ThemisDB_LLM[\"ThemisDB - Embedded LLM\"]
        TDB_EMBED[\"Embedded llama.cpp\"]
        TDB_LOCAL[\"Local Model Files<br/>(LLaMA, Mistral, Phi-3)\"]
        TDB_NO_API[\"No API Calls<br/>(Zero Latency)\"]
        TDB_QUANT[\"Quantization Support<br/>(Q4, Q5, Q8)\"]
        TDB_GPU_LLM[\"GPU Acceleration<br/>(CUDA, Metal)\"]
        TDB_COST[\"💰 Zero Runtime Cost\"]
        TDB_PRIVACY[\"🔒 Complete Privacy<br/>(Data Never Leaves Server)\"]
    end
    
    subgraph OpenAI[\"OpenAI API\"]
        OAI_API[\"REST API Calls\"]
        OAI_CLOUD[\"Cloud-Hosted Models<br/>(GPT-3.5, GPT-4)\"]
        OAI_LATENCY[\"Network Latency<br/>(100-500ms)\"]
        OAI_NO_QUANT[\"No Quantization<br/>(Fixed Model Size)\"]
        OAI_CLOUD_GPU[\"Cloud GPU<br/>(Abstracted)\"]
        OAI_COST[\"💰 Pay Per Token<br/>($0.002-$0.06/1K tokens)\"]
        OAI_DATA[\"⚠️ Data Sent to Cloud\"]
    end
    
    subgraph Anthropic[\"Anthropic Claude\"]
        ANT_API[\"REST API Calls\"]
        ANT_CLOUD[\"Cloud-Hosted Models<br/>(Claude 2, 3)\"]
        ANT_LATENCY[\"Network Latency<br/>(100-500ms)\"]
        ANT_NO_QUANT[\"No Quantization\"]
        ANT_CLOUD_GPU[\"Cloud GPU\"]
        ANT_COST[\"💰 Pay Per Token<br/>($0.003-$0.015/1K tokens)\"]
        ANT_DATA[\"⚠️ Data Sent to Cloud\"]
    end
    
    subgraph Ollama[\"Ollama (Local)\"]
        OLL_LOCAL[\"Local Server\"]
        OLL_MODELS[\"Local Models<br/>(Same as ThemisDB)\"]
        OLL_NO_API[\"Local API<br/>(Low Latency)\"]
        OLL_QUANT[\"Quantization Support\"]
        OLL_GPU[\"GPU Support\"]
        OLL_COST[\"💰 Zero Runtime Cost\"]
        OLL_PRIVACY[\"🔒 Local Privacy\"]
        OLL_SEPARATE[\"⚠️ Separate Service<br/>(Not Integrated)\"]
    end
    
    TDB_EMBED -.->|\"Integrated vs Separate\"| OAI_API
    TDB_EMBED -.->|\"Integrated vs Separate\"| ANT_API
    TDB_EMBED -.->|\"Integrated vs External\"| OLL_LOCAL
    
    TDB_NO_API -.->|\"0ms vs 100-500ms\"| OAI_LATENCY
    TDB_NO_API -.->|\"0ms vs 100-500ms\"| ANT_LATENCY
    
    TDB_COST -.->|\"Free vs Paid\"| OAI_COST
    TDB_COST -.->|\"Free vs Paid\"| ANT_COST
    
    TDB_PRIVACY -.->|\"Private vs Cloud\"| OAI_DATA
    TDB_PRIVACY -.->|\"Private vs Cloud\"| ANT_DATA
    
    style TDB_EMBED fill:#3498db
    style TDB_NO_API fill:#27ae60
    style TDB_COST fill:#2ea44f
    style TDB_PRIVACY fill:#2ea44f
    style TDB_GPU_LLM fill:#27ae60
    style OAI_API fill:#cccccc
    style ANT_API fill:#cccccc
    style OAI_COST fill:#e74c3c
    style ANT_COST fill:#e74c3c
    style OAI_DATA fill:#f39c12
    style ANT_DATA fill:#f39c12";
    }
    
    /**
     * Hardware architecture diagram
     */
    private function get_hardware_architecture_diagram() {
        return "graph TB
    subgraph Server[\"ThemisDB Server Hardware Stack\"]
        subgraph CPU_Layer[\"CPU Layer\"]
            CPU[\"CPU<br/>Intel Xeon / AMD EPYC<br/>20-128 Cores\"]
            CPU_CACHE[\"L1/L2/L3 Cache<br/>256KB-256MB\"]
            SIMD[\"SIMD Instructions<br/>AVX2, AVX-512\"]
        end
        
        subgraph GPU_Layer[\"GPU Layer (Optional)\"]
            GPU[\"GPU<br/>NVIDIA A100/H100<br/>RTX 4090\"]
            GPU_MEM[\"GPU Memory<br/>VRAM: 24-80GB<br/>Bandwidth: 2-3 TB/s\"]
            CUDA[\"CUDA Cores<br/>10K-18K Cores\"]
            TENSOR[\"Tensor Cores<br/>AI Acceleration\"]
        end
        
        subgraph Memory[\"System Memory\"]
            RAM[\"DDR4/DDR5 RAM<br/>64GB - 1TB\"]
            SWAP[\"Swap Space<br/>Optional\"]
            NUMA[\"NUMA Architecture<br/>Multi-Socket Systems\"]
        end
        
        subgraph Storage_HW[\"Storage Hardware\"]
            SSD[\"NVMe SSD<br/>1-10TB<br/>Read: 7GB/s\"]
            HDD[\"HDD (Archive)<br/>10-100TB<br/>Read: 200MB/s\"]
            RAID_HW[\"RAID Controller<br/>RAID 0/1/5/10\"]
        end
        
        subgraph Network[\"Network Interface\"]
            NIC[\"Network Card<br/>10/25/100 Gbps\"]
            RDMA[\"RDMA Support<br/>(Optional)\"]
        end
    end
    
    subgraph Software[\"ThemisDB Software Mapping\"]
        DB_ENGINE[\"Database Engine<br/>(CPU Intensive)\"]
        VECTOR_SEARCH[\"Vector Search<br/>(GPU Accelerated)\"]
        LLM_ENGINE[\"LLM Inference<br/>(GPU Accelerated)\"]
        STORAGE_ENGINE[\"Storage Engine<br/>(SSD Optimized)\"]
        REPLICATION[\"Replication<br/>(Network Intensive)\"]
    end
    
    CPU --> DB_ENGINE
    CPU_CACHE --> DB_ENGINE
    SIMD --> DB_ENGINE
    
    GPU --> VECTOR_SEARCH
    GPU --> LLM_ENGINE
    GPU_MEM --> VECTOR_SEARCH
    GPU_MEM --> LLM_ENGINE
    CUDA --> LLM_ENGINE
    TENSOR --> LLM_ENGINE
    
    RAM --> DB_ENGINE
    RAM --> VECTOR_SEARCH
    RAM --> LLM_ENGINE
    NUMA --> DB_ENGINE
    
    SSD --> STORAGE_ENGINE
    HDD --> STORAGE_ENGINE
    RAID_HW --> STORAGE_ENGINE
    
    NIC --> REPLICATION
    RDMA --> REPLICATION
    
    style CPU fill:#3498db
    style GPU fill:#27ae60
    style RAM fill:#9b59b6
    style SSD fill:#e67e22
    style NIC fill:#e74c3c
    style VECTOR_SEARCH fill:#27ae60
    style LLM_ENGINE fill:#27ae60
    style DB_ENGINE fill:#3498db
    style STORAGE_ENGINE fill:#2ea44f";
    }
    
    /**
     * Performance comparison with hardware considerations
     */
    private function get_performance_comparison_diagram() {
        return "graph TB
    subgraph Config1[\"Configuration 1: CPU Only\"]
        C1_HW[\"Hardware:<br/>Intel Xeon 32-Core<br/>128GB RAM<br/>NVMe SSD\"]
        C1_TDB[\"ThemisDB<br/>Vector Search: 10K qps<br/>LLM: 5 tokens/sec\"]
        C1_PG[\"PostgreSQL + pgvector<br/>Vector Search: 2K qps<br/>No LLM\"]
        C1_COST[\"💰 Cost: $500/month\"]
    end
    
    subgraph Config2[\"Configuration 2: CPU + Mid GPU\"]
        C2_HW[\"Hardware:<br/>Intel Xeon 32-Core<br/>128GB RAM + RTX 4090<br/>NVMe SSD\"]
        C2_TDB[\"ThemisDB<br/>Vector Search: 50K qps<br/>LLM: 50 tokens/sec\"]
        C2_PG[\"PostgreSQL + pgvector<br/>Vector Search: 2K qps<br/>No LLM Support\"]
        C2_COST[\"💰 Cost: $2,000/month\"]
    end
    
    subgraph Config3[\"Configuration 3: High-End GPU\"]
        C3_HW[\"Hardware:<br/>AMD EPYC 64-Core<br/>256GB RAM + A100 80GB<br/>NVMe SSD RAID\"]
        C3_TDB[\"ThemisDB<br/>Vector Search: 200K qps<br/>LLM: 150 tokens/sec\"]
        C3_PG[\"PostgreSQL + pgvector<br/>Vector Search: 5K qps<br/>No LLM Support\"]
        C3_COST[\"💰 Cost: $10,000/month\"]
    end
    
    subgraph Cloud[\"Cloud Alternative\"]
        CL_HW[\"Cloud Services:<br/>OpenAI API<br/>Pinecone Vector DB<br/>AWS RDS\"]
        CL_PERF[\"Performance:<br/>Vector Search: 10K qps<br/>LLM: 20 tokens/sec<br/>+ Network Latency\"]
        CL_COST[\"💰 Cost: $5,000-50,000/month<br/>(Depends on Usage)\"]
        CL_PRIVACY[\"⚠️ Data Leaves Premises\"]
    end
    
    C1_TDB -.->|\"5x Faster Vector\"| C1_PG
    C2_TDB -.->|\"25x Faster Vector<br/>+ Native LLM\"| C2_PG
    C3_TDB -.->|\"40x Faster Vector<br/>+ Fast LLM\"| C3_PG
    
    C1_COST -.->|\"vs\"| CL_COST
    C2_COST -.->|\"vs\"| CL_COST
    C3_COST -.->|\"vs\"| CL_COST
    
    style C1_TDB fill:#2ea44f
    style C2_TDB fill:#2ea44f
    style C3_TDB fill:#2ea44f
    style C1_PG fill:#cccccc
    style C2_PG fill:#cccccc
    style C3_PG fill:#cccccc
    style C1_COST fill:#3498db
    style C2_COST fill:#3498db
    style C3_COST fill:#3498db
    style CL_COST fill:#e74c3c
    style CL_PRIVACY fill:#f39c12";
    }
    
    /**
     * TCO (Total Cost of Ownership) comparison over time
     */
    private function get_tco_comparison_diagram() {
        return "graph LR
    subgraph Year1[\"Year 1 Costs\"]
        TDB_Y1[\"ThemisDB Self-Hosted<br/>Hardware: $20,000<br/>Monthly: $500<br/>Total: $26,000\"]
        PG_Y1[\"PostgreSQL + pgvector<br/>Hardware: $15,000<br/>Monthly: $500<br/>Total: $21,000\"]
        CLOUD_Y1[\"Cloud Services<br/>Setup: $5,000<br/>Monthly: $15,000<br/>Total: $185,000\"]
    end
    
    subgraph Year3[\"Year 3 Costs (Cumulative)\"]
        TDB_Y3[\"ThemisDB<br/>Initial: $20,000<br/>3yr Operating: $18,000<br/>Total: $38,000\"]
        PG_Y3[\"PostgreSQL<br/>Initial: $15,000<br/>3yr Operating: $18,000<br/>+ LLM API: $180,000<br/>Total: $213,000\"]
        CLOUD_Y3[\"Cloud Services<br/>Setup: $5,000<br/>3yr Monthly: $540,000<br/>Total: $545,000\"]
    end
    
    subgraph Year5[\"Year 5 Costs (Cumulative)\"]
        TDB_Y5[\"ThemisDB<br/>Initial: $20,000<br/>5yr Operating: $30,000<br/>Upgrade: $10,000<br/>Total: $60,000<br/>💰 Best ROI\"]
        PG_Y5[\"PostgreSQL<br/>Initial: $15,000<br/>5yr Operating: $30,000<br/>+ LLM API: $300,000<br/>Total: $345,000\"]
        CLOUD_Y5[\"Cloud Services<br/>Setup: $5,000<br/>5yr Monthly: $900,000<br/>Total: $905,000<br/>⚠️ Highest Cost\"]
    end
    
    TDB_Y1 --> TDB_Y3
    TDB_Y3 --> TDB_Y5
    PG_Y1 --> PG_Y3
    PG_Y3 --> PG_Y5
    CLOUD_Y1 --> CLOUD_Y3
    CLOUD_Y3 --> CLOUD_Y5
    
    style TDB_Y1 fill:#2ea44f
    style TDB_Y3 fill:#2ea44f
    style TDB_Y5 fill:#27ae60
    style PG_Y1 fill:#cccccc
    style PG_Y3 fill:#cccccc
    style PG_Y5 fill:#999999
    style CLOUD_Y1 fill:#f39c12
    style CLOUD_Y3 fill:#e67e22
    style CLOUD_Y5 fill:#e74c3c";
    }
    
    /**
     * Feature matrix comparison
     */
    private function get_feature_matrix_diagram() {
        return "graph TB
    subgraph Features[\"Feature Comparison Matrix\"]
        F1[\"Multi-Model Support\"]
        F2[\"Embedded LLM\"]
        F3[\"GPU Acceleration\"]
        F4[\"ACID Transactions\"]
        F5[\"Vector Search\"]
        F6[\"Graph Queries\"]
        F7[\"Time Series\"]
        F8[\"Full-Text Search\"]
        F9[\"Sharding/Clustering\"]
        F10[\"Real-time Sync\"]
    end
    
    subgraph ThemisDB[\"ThemisDB\"]
        T1[\"✅ Native All Models\"]
        T2[\"✅ llama.cpp Built-in\"]
        T3[\"✅ CUDA/Metal/Vulkan\"]
        T4[\"✅ Full ACID\"]
        T5[\"✅ HNSW + FAISS\"]
        T6[\"✅ Native Graph\"]
        T7[\"✅ Built-in\"]
        T8[\"✅ Integrated\"]
        T9[\"✅ RAID-style\"]
        T10[\"✅ CDC/Changefeed\"]
    end
    
    subgraph PostgreSQL[\"PostgreSQL\"]
        P1[\"⚠️ Relational Only\"]
        P2[\"❌ No LLM\"]
        P3[\"❌ CPU Only\"]
        P4[\"✅ Full ACID\"]
        P5[\"⚠️ pgvector ext\"]
        P6[\"❌ No Native\"]
        P7[\"⚠️ TimescaleDB ext\"]
        P8[\"✅ Built-in\"]
        P9[\"⚠️ Complex Setup\"]
        P10[\"⚠️ Logical Repl\"]
    end
    
    subgraph MongoDB[\"MongoDB\"]
        M1[\"⚠️ Document Only\"]
        M2[\"❌ No LLM\"]
        M3[\"❌ CPU Only\"]
        M4[\"⚠️ Eventual Consistency\"]
        M5[\"⚠️ Atlas Vector\"]
        M6[\"❌ No Native\"]
        M7[\"✅ Built-in\"]
        M8[\"✅ Text Search\"]
        M9[\"✅ Sharding\"]
        M10[\"✅ Change Streams\"]
    end
    
    subgraph Neo4j[\"Neo4j\"]
        N1[\"⚠️ Graph Only\"]
        N2[\"❌ No LLM\"]
        N3[\"❌ CPU Only\"]
        N4[\"✅ Full ACID\"]
        N5[\"⚠️ Plugin\"]
        N6[\"✅ Native Graph\"]
        N7[\"❌ No Native\"]
        N8[\"✅ Lucene\"]
        N9[\"✅ Clustering\"]
        N10[\"❌ Limited\"]
    end
    
    F1 --> T1 & P1 & M1 & N1
    F2 --> T2 & P2 & M2 & N2
    F3 --> T3 & P3 & M3 & N3
    F4 --> T4 & P4 & M4 & N4
    F5 --> T5 & P5 & M5 & N5
    F6 --> T6 & P6 & M6 & N6
    F7 --> T7 & P7 & M7 & N7
    F8 --> T8 & P8 & M8 & N8
    F9 --> T9 & P9 & M9 & N9
    F10 --> T10 & P10 & M10 & N10
    
    style T1 fill:#2ea44f
    style T2 fill:#2ea44f
    style T3 fill:#2ea44f
    style T4 fill:#2ea44f
    style T5 fill:#2ea44f
    style T6 fill:#2ea44f
    style T7 fill:#2ea44f
    style T8 fill:#2ea44f
    style T9 fill:#2ea44f
    style T10 fill:#2ea44f";
    }
    
    /**
     * Deployment options comparison
     */
    private function get_deployment_options_diagram() {
        return "graph TB
    subgraph OnPrem[\"On-Premise Deployment\"]
        OP_TDB[\"ThemisDB\"]
        OP_HW[\"Your Hardware<br/>Full Control\"]
        OP_DATA[\"🔒 Data On-Site<br/>Complete Privacy\"]
        OP_COST[\"💰 CapEx Model<br/>Predictable Costs\"]
        OP_PERF[\"⚡ No Network Latency<br/>Max Performance\"]
        OP_MAINT[\"🔧 Self-Managed<br/>Your Team\"]
    end
    
    subgraph Cloud[\"Cloud Deployment\"]
        CL_TDB[\"ThemisDB on Cloud VM\"]
        CL_HW[\"AWS/Azure/GCP<br/>Flexible Scaling\"]
        CL_DATA[\"🔒 Data in Cloud<br/>Your VPC/Region\"]
        CL_COST[\"💰 OpEx Model<br/>Pay-as-you-go\"]
        CL_PERF[\"⚡ Regional Latency<br/>Good Performance\"]
        CL_MAINT[\"🔧 Managed Infrastructure<br/>Cloud Provider\"]
    end
    
    subgraph Hybrid[\"Hybrid Deployment\"]
        HY_TDB[\"ThemisDB Distributed\"]
        HY_HW[\"On-Prem + Cloud<br/>Best of Both\"]
        HY_DATA[\"🔒 Sensitive Data On-Prem<br/>Archive in Cloud\"]
        HY_COST[\"💰 Mixed Model<br/>Optimized Costs\"]
        HY_PERF[\"⚡ Edge Processing<br/>Central Storage\"]
        HY_MAINT[\"🔧 Split Responsibility<br/>Shared Management\"]
    end
    
    subgraph SaaS[\"Cloud Services (Alternative)\"]
        SA_SERV[\"Multiple Services\"]
        SA_HW[\"Fully Managed<br/>No Control\"]
        SA_DATA[\"⚠️ Data with Vendor<br/>Limited Privacy\"]
        SA_COST[\"💰 Per-Use Billing<br/>Unpredictable\"]
        SA_PERF[\"⚡ Internet Latency<br/>Variable Performance\"]
        SA_MAINT[\"🔧 Vendor Managed<br/>Lock-in Risk\"]
    end
    
    OP_TDB --> OP_HW --> OP_DATA
    OP_DATA --> OP_COST --> OP_PERF --> OP_MAINT
    
    CL_TDB --> CL_HW --> CL_DATA
    CL_DATA --> CL_COST --> CL_PERF --> CL_MAINT
    
    HY_TDB --> HY_HW --> HY_DATA
    HY_DATA --> HY_COST --> HY_PERF --> HY_MAINT
    
    SA_SERV --> SA_HW --> SA_DATA
    SA_DATA --> SA_COST --> SA_PERF --> SA_MAINT
    
    style OP_TDB fill:#2ea44f
    style OP_DATA fill:#27ae60
    style CL_TDB fill:#3498db
    style CL_DATA fill:#3498db
    style HY_TDB fill:#9b59b6
    style HY_DATA fill:#8e44ad
    style SA_SERV fill:#e74c3c
    style SA_DATA fill:#c0392b";
    }
    
    /**
     * Use case recommendations
     */
    private function get_use_case_recommendations_diagram() {
        return "graph TB
    subgraph AI_ML[\"AI/ML Applications\"]
        AI_DESC[\"RAG, Embeddings,<br/>Semantic Search\"]
        AI_REC[\"✅ ThemisDB<br/>Embedded LLM + Vector Search<br/>Zero API Costs\"]
    end
    
    subgraph RealTime[\"Real-Time Analytics\"]
        RT_DESC[\"Dashboards, Monitoring,<br/>Time-Series Data\"]
        RT_REC[\"✅ ThemisDB<br/>Native Time-Series + Fast Queries<br/>Or: PostgreSQL + TimescaleDB\"]
    end
    
    subgraph GraphApp[\"Graph Applications\"]
        GR_DESC[\"Social Networks,<br/>Knowledge Graphs, Recommendations\"]
        GR_REC[\"✅ ThemisDB<br/>Native Graph + Multi-Model<br/>Or: Neo4j (Graph-only)\"]
    end
    
    subgraph MultiModel[\"Complex Data Models\"]
        MM_DESC[\"Mixed Relational, Graph,<br/>Vector, Document Data\"]
        MM_REC[\"✅ ThemisDB<br/>Single Database for All Models<br/>No Data Duplication\"]
    end
    
    subgraph Enterprise[\"Enterprise Applications\"]
        ENT_DESC[\"ERP, CRM, OLTP<br/>High Transaction Volume\"]
        ENT_REC[\"✅ ThemisDB or PostgreSQL<br/>Full ACID Compliance<br/>Proven Reliability\"]
    end
    
    subgraph ContentMgmt[\"Content Management\"]
        CM_DESC[\"CMS, Document Storage,<br/>Flexible Schemas\"]
        CM_REC[\"✅ ThemisDB or MongoDB<br/>Document Model<br/>Schema Flexibility\"]
    end
    
    subgraph IoT[\"IoT & Edge Computing\"]
        IOT_DESC[\"Sensor Data, Edge Analytics,<br/>Time-Series + AI\"]
        IOT_REC[\"✅ ThemisDB<br/>Time-Series + LLM + Compact<br/>Edge Deployment Ready\"]
    end
    
    subgraph Compliance[\"Privacy & Compliance\"]
        COMP_DESC[\"GDPR, HIPAA, Financial,<br/>Data Sovereignty\"]
        COMP_REC[\"✅ ThemisDB On-Premise<br/>Data Never Leaves Infrastructure<br/>Full Audit Trail\"]
    end
    
    AI_DESC --> AI_REC
    RT_DESC --> RT_REC
    GR_DESC --> GR_REC
    MM_DESC --> MM_REC
    ENT_DESC --> ENT_REC
    CM_DESC --> CM_REC
    IOT_DESC --> IOT_REC
    COMP_DESC --> COMP_REC
    
    style AI_REC fill:#2ea44f
    style RT_REC fill:#2ea44f
    style GR_REC fill:#2ea44f
    style MM_REC fill:#2ea44f
    style ENT_REC fill:#3498db
    style CM_REC fill:#3498db
    style IOT_REC fill:#2ea44f
    style COMP_REC fill:#27ae60";
    }
    
    /**
     * Migration paths from other databases
     */
    private function get_migration_paths_diagram() {
        return "graph TB
    subgraph FromPG[\"From PostgreSQL\"]
        PG_SRC[\"PostgreSQL Database\"]
        PG_EXPORT[\"pg_dump Export\"]
        PG_SCHEMA[\"Schema Mapping<br/>SQL → AQL\"]
        PG_IMPORT[\"ThemisDB Import\"]
        PG_BENEFIT[\"✅ Gain: LLM + Vector + Graph<br/>Keep: ACID + SQL Compatibility\"]
    end
    
    subgraph FromMongo[\"From MongoDB\"]
        MG_SRC[\"MongoDB Database\"]
        MG_EXPORT[\"mongoexport JSON\"]
        MG_SCHEMA[\"Document Mapping<br/>BSON → ThemisDB\"]
        MG_IMPORT[\"ThemisDB Import\"]
        MG_BENEFIT[\"✅ Gain: ACID + LLM + Multi-Model<br/>Keep: Document Flexibility\"]
    end
    
    subgraph FromNeo4j[\"From Neo4j\"]
        NJ_SRC[\"Neo4j Database\"]
        NJ_EXPORT[\"Cypher Export\"]
        NJ_SCHEMA[\"Graph Mapping<br/>Cypher → AQL\"]
        NJ_IMPORT[\"ThemisDB Import\"]
        NJ_BENEFIT[\"✅ Gain: LLM + Vector + Relational<br/>Keep: Graph Queries\"]
    end
    
    subgraph FromLegacy[\"From Legacy Systems\"]
        LEG_SRC[\"Oracle/MSSQL/MySQL\"]
        LEG_EXPORT[\"SQL Export\"]
        LEG_SCHEMA[\"Schema Conversion<br/>SQL → AQL\"]
        LEG_IMPORT[\"ThemisDB Import\"]
        LEG_BENEFIT[\"✅ Gain: Modern Features + Cost Savings<br/>Keep: Data Integrity\"]
    end
    
    subgraph ToThemis[\"ThemisDB Unified Platform\"]
        THEMIS[\"ThemisDB\"]
        THEMIS_FEAT[\"All Features Available:<br/>• Multi-Model (SQL, Graph, Document, Vector)<br/>• Embedded LLM (llama.cpp)<br/>• GPU Acceleration<br/>• Full ACID Transactions<br/>• Horizontal Sharding\"]
    end
    
    PG_SRC --> PG_EXPORT --> PG_SCHEMA --> PG_IMPORT --> PG_BENEFIT
    MG_SRC --> MG_EXPORT --> MG_SCHEMA --> MG_IMPORT --> MG_BENEFIT
    NJ_SRC --> NJ_EXPORT --> NJ_SCHEMA --> NJ_IMPORT --> NJ_BENEFIT
    LEG_SRC --> LEG_EXPORT --> LEG_SCHEMA --> LEG_IMPORT --> LEG_BENEFIT
    
    PG_BENEFIT --> THEMIS
    MG_BENEFIT --> THEMIS
    NJ_BENEFIT --> THEMIS
    LEG_BENEFIT --> THEMIS
    
    THEMIS --> THEMIS_FEAT
    
    style PG_IMPORT fill:#2ea44f
    style MG_IMPORT fill:#2ea44f
    style NJ_IMPORT fill:#2ea44f
    style LEG_IMPORT fill:#2ea44f
    style THEMIS fill:#27ae60
    style THEMIS_FEAT fill:#27ae60
    style PG_BENEFIT fill:#3498db
    style MG_BENEFIT fill:#3498db
    style NJ_BENEFIT fill:#3498db
    style LEG_BENEFIT fill:#3498db";
    }
}

// Initialize plugin
function themisdb_architecture_diagrams_init() {
    return ThemisDB_Architecture_Diagrams::get_instance();
}

add_action('plugins_loaded', 'themisdb_architecture_diagrams_init');
