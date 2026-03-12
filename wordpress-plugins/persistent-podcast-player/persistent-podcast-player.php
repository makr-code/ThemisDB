<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            persistent-podcast-player.php                      ║
  Version:         0.0.34                                             ║
  Last Modified:   2026-03-09 04:08:16                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     361                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */
/**
 * Plugin Name: Persistent Podcast Player
 * Plugin URI: https://github.com/makr-code/wordpressPlugins
 * Description: A persistent podcast player with episode excerpts and related post links
 * Version: 1.0.0
 * Author: ThemisDB Team
 * Author URI: https://github.com/makr-code/wordpressPlugins
 * License: MIT
 * License URI: https://opensource.org/licenses/MIT
 * Text Domain: persistent-podcast-player
 * Domain Path: /languages
 * Requires at least: 5.0
 * Requires PHP: 7.4
 */

// Exit if accessed directly
if (!defined('ABSPATH')) {
    exit;
}

// Define plugin constants
define('PPP_VERSION', '1.0.0');
define('PPP_PLUGIN_DIR', plugin_dir_path(__FILE__));
define('PPP_PLUGIN_URL', plugin_dir_url(__FILE__));
define('PPP_PLUGIN_FILE', __FILE__);

// Load updater class (prefer local copy for standalone ZIP distribution)
$themisdb_updater_local = PPP_PLUGIN_DIR . 'includes/class-themisdb-plugin-updater.php';
$themisdb_updater_shared = dirname(PPP_PLUGIN_DIR) . '/includes/class-themisdb-plugin-updater.php';

if (file_exists($themisdb_updater_local)) {
    require_once $themisdb_updater_local;
} elseif (file_exists($themisdb_updater_shared)) {
    require_once $themisdb_updater_shared;
}

// Initialize automatic updates
if (class_exists('ThemisDB_Plugin_Updater')) {
    new ThemisDB_Plugin_Updater(
        PPP_PLUGIN_FILE,
        'persistent-podcast-player',
        PPP_VERSION
    );
}

/**
 * Main Plugin Class
 */
class Persistent_Podcast_Player {
    
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
        add_action('init', array($this, 'register_post_type'));
        add_action('rest_api_init', array($this, 'register_rest_routes'));
        add_action('wp_enqueue_scripts', array($this, 'enqueue_assets'));
        add_action('wp_body_open', array($this, 'render_player'));
    }
    
    /**
     * Plugin activation
     */
    public function activate() {
        $this->register_post_type();
        flush_rewrite_rules();
    }
    
    /**
     * Plugin deactivation
     */
    public function deactivate() {
        flush_rewrite_rules();
    }
    
    /**
     * Register custom post type
     */
    public function register_post_type() {
        $args = array(
            'label' => __('Podcast Episodes', 'persistent-podcast-player'),
            'public' => true,
            'show_in_rest' => true,
            'rest_base' => 'pod_episodes',
            'supports' => array('title', 'editor', 'custom-fields', 'thumbnail'),
            'menu_icon' => 'dashicons-microphone',
            'has_archive' => true,
            'capability_type' => 'post',
            'labels' => array(
                'name' => __('Podcast Episodes', 'persistent-podcast-player'),
                'singular_name' => __('Podcast Episode', 'persistent-podcast-player'),
                'add_new' => __('Add New Episode', 'persistent-podcast-player'),
                'add_new_item' => __('Add New Episode', 'persistent-podcast-player'),
                'edit_item' => __('Edit Episode', 'persistent-podcast-player'),
                'new_item' => __('New Episode', 'persistent-podcast-player'),
                'view_item' => __('View Episode', 'persistent-podcast-player'),
                'search_items' => __('Search Episodes', 'persistent-podcast-player'),
                'not_found' => __('No episodes found', 'persistent-podcast-player'),
                'not_found_in_trash' => __('No episodes found in Trash', 'persistent-podcast-player'),
            ),
        );
        
        register_post_type('pod_episode', $args);
        
        // Register meta fields
        register_post_meta('pod_episode', 'audio_url', array(
            'type' => 'string',
            'single' => true,
            'show_in_rest' => true,
        ));
        
        register_post_meta('pod_episode', 'related_post_id', array(
            'type' => 'integer',
            'single' => true,
            'show_in_rest' => true,
        ));
    }
    
    /**
     * Register REST routes
     */
    public function register_rest_routes() {
        register_rest_route('persistent-player/v1', '/episodes', array(
            'methods' => 'GET',
            'callback' => array($this, 'get_episodes'),
            'permission_callback' => '__return_true',
        ));
    }
    
    /**
     * Get episodes endpoint callback
     */
    public function get_episodes($request) {
        $args = array(
            'post_type' => 'pod_episode',
            'post_status' => 'publish',
            'posts_per_page' => 50,
            'orderby' => 'date',
            'order' => 'DESC',
        );
        
        $query = new WP_Query($args);
        $episodes = array();
        
        if ($query->have_posts()) {
            while ($query->have_posts()) {
                $query->the_post();
                $post_id = get_the_ID();
                
                // Get custom fields
                $audio_url = get_post_meta($post_id, 'audio_url', true);
                $related_post_id = get_post_meta($post_id, 'related_post_id', true);
                
                // Get excerpt and permalink from related post
                $excerpt = '';
                $permalink = '';
                
                if ($related_post_id && get_post_status($related_post_id) === 'publish') {
                    $related_post = get_post($related_post_id);
                    if ($related_post) {
                        $excerpt = $related_post->post_excerpt 
                            ? $related_post->post_excerpt 
                            : wp_trim_words(strip_tags($related_post->post_content), apply_filters('ppp_excerpt_length', 30));
                        $permalink = get_permalink($related_post_id);
                    }
                }
                
                // Get thumbnail
                $thumbnail = array(
                    'full' => '',
                    'medium' => '',
                    'thumbnail' => '',
                );
                
                if (has_post_thumbnail($post_id)) {
                    $thumbnail_id = get_post_thumbnail_id($post_id);
                    $thumbnail['full'] = wp_get_attachment_image_url($thumbnail_id, 'full');
                    $thumbnail['medium'] = wp_get_attachment_image_url($thumbnail_id, 'medium');
                    $thumbnail['thumbnail'] = wp_get_attachment_image_url($thumbnail_id, 'thumbnail');
                }
                
                $episodes[] = array(
                    'id' => $post_id,
                    'title' => get_the_title(),
                    'audio' => $audio_url ? $audio_url : '',
                    'desc' => strip_tags(get_the_content()),
                    'excerpt' => $excerpt,
                    'permalink' => $permalink,
                    'thumbnail' => $thumbnail,
                );
            }
            wp_reset_postdata();
        }
        
        return rest_ensure_response($episodes);
    }
    
    /**
     * Enqueue assets
     */
    public function enqueue_assets() {
        // Plugin CSS
        wp_enqueue_style(
            'ppp-player-style',
            PPP_PLUGIN_URL . 'assets/css/player.css',
            array(),
            PPP_VERSION
        );
        
        // Plugin JS
        wp_enqueue_script(
            'ppp-player-script',
            PPP_PLUGIN_URL . 'assets/js/player.js',
            array('jquery'),
            PPP_VERSION,
            true
        );
        
        // Localize script with REST URL
        wp_localize_script(
            'ppp-player-script',
            'pppData',
            array(
                'restUrl' => rest_url('persistent-player/v1/episodes'),
                'nonce' => wp_create_nonce('wp_rest'),
            )
        );
    }
    
    /**
     * Render player HTML
     */
    public function render_player() {
        ?>
        <div id="ppp-player" class="ppp-player">
            <div class="ppp-player-container">
                <div class="ppp-controls">
                    <button id="ppp-prev" class="ppp-btn" title="Previous Episode" aria-label="Previous Episode">
                        <span>&#9664;</span>
                    </button>
                    <button id="ppp-skip-backward" class="ppp-btn ppp-btn-skip" title="Skip Backward 15s" aria-label="Skip Backward 15 seconds">
                        <span>&#8634; 15</span>
                    </button>
                    <button id="ppp-play-pause" class="ppp-btn ppp-btn-play" title="Play/Pause" aria-label="Play">
                        <span class="ppp-play-icon">&#9654;</span>
                        <span class="ppp-pause-icon">&#10074;&#10074;</span>
                    </button>
                    <button id="ppp-skip-forward" class="ppp-btn ppp-btn-skip" title="Skip Forward 30s" aria-label="Skip Forward 30 seconds">
                        <span>30 &#8635;</span>
                    </button>
                    <button id="ppp-next" class="ppp-btn" title="Next Episode" aria-label="Next Episode">
                        <span>&#9654;</span>
                    </button>
                </div>
                
                <div class="ppp-time-display">
                    <span id="ppp-current-time">0:00</span>
                </div>
                
                <div class="ppp-progress-container">
                    <div class="ppp-progress-bar" id="ppp-progress-bar" role="slider" aria-label="Seek" aria-valuemin="0" aria-valuemax="100" aria-valuenow="0" tabindex="0">
                        <div class="ppp-progress-fill" id="ppp-progress-fill"></div>
                        <div class="ppp-progress-buffer" id="ppp-progress-buffer"></div>
                    </div>
                </div>
                
                <div class="ppp-time-display">
                    <span id="ppp-total-time">0:00</span>
                </div>
                
                <div class="ppp-volume-container">
                    <button id="ppp-volume-btn" class="ppp-btn ppp-btn-small" title="Mute/Unmute" aria-label="Volume">
                        <span class="ppp-volume-icon ppp-volume-on">&#128266;</span>
                        <span class="ppp-volume-icon ppp-volume-off">&#128263;</span>
                    </button>
                    <div class="ppp-volume-slider-container">
                        <input type="range" id="ppp-volume-slider" class="ppp-volume-slider" min="0" max="100" value="100" aria-label="Volume Level">
                    </div>
                </div>
                
                <div class="ppp-speed-container">
                    <button id="ppp-speed-btn" class="ppp-btn ppp-btn-small" title="Playback Speed" aria-label="Playback Speed">
                        <span id="ppp-speed-label">1x</span>
                    </button>
                    <div id="ppp-speed-menu" class="ppp-speed-menu" style="display: none;">
                        <button class="ppp-speed-option" data-speed="0.5">0.5x</button>
                        <button class="ppp-speed-option" data-speed="0.75">0.75x</button>
                        <button class="ppp-speed-option ppp-speed-active" data-speed="1">1x</button>
                        <button class="ppp-speed-option" data-speed="1.25">1.25x</button>
                        <button class="ppp-speed-option" data-speed="1.5">1.5x</button>
                        <button class="ppp-speed-option" data-speed="1.75">1.75x</button>
                        <button class="ppp-speed-option" data-speed="2">2x</button>
                    </div>
                </div>
                
                <div class="ppp-info">
                    <div class="ppp-title" id="ppp-title">Select an episode</div>
                    <div class="ppp-excerpt" id="ppp-excerpt"></div>
                </div>
                
                <div class="ppp-link-container">
                    <a href="#" id="ppp-link" class="ppp-link" target="_blank" rel="noopener noreferrer" aria-label="Zum Artikel (öffnet in neuem Tab)" style="display: none;">Zum Artikel</a>
                </div>
                
                <div class="ppp-options">
                    <label class="ppp-checkbox-label" title="Automatically play next episode">
                        <input type="checkbox" id="ppp-continuous-play" checked>
                        <span>Continuous Play</span>
                    </label>
                </div>
                
                <div class="ppp-playlist-toggle">
                    <button id="ppp-toggle-playlist" class="ppp-btn-toggle" aria-label="Toggle Playlist">Playlist</button>
                </div>
            </div>
            
            <div id="ppp-loading" class="ppp-loading" style="display: none;">
                <div class="ppp-spinner"></div>
            </div>
            
            <div id="ppp-error" class="ppp-error" style="display: none;">
                <span class="ppp-error-icon">⚠</span>
                <span class="ppp-error-message" id="ppp-error-message">Error loading audio</span>
                <button id="ppp-error-retry" class="ppp-btn-retry">Retry</button>
            </div>
            
            <div id="ppp-playlist" class="ppp-playlist" style="display: none;">
                <div class="ppp-playlist-items" id="ppp-playlist-items">
                    <!-- Playlist items will be populated by JS -->
                </div>
            </div>
            
            <audio id="ppp-audio" preload="metadata"></audio>
        </div>
        <?php
    }
}

// Initialize plugin
Persistent_Podcast_Player::get_instance();
