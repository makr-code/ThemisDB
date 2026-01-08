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
            'database_comparison' => $this->get_database_comparison_diagram(),
            'llm_comparison' => $this->get_llm_comparison_diagram(),
            'hardware_architecture' => $this->get_hardware_architecture_diagram(),
            'performance_comparison' => $this->get_performance_comparison_diagram(),
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
}

// Initialize plugin
function themisdb_architecture_diagrams_init() {
    return ThemisDB_Architecture_Diagrams::get_instance();
}

add_action('plugins_loaded', 'themisdb_architecture_diagrams_init');
