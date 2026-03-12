<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            themisdb-release-timeline.php                      ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:20                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     384                                            ║
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
 * Plugin Name: ThemisDB Release Timeline Visualizer
 * Plugin URI: https://github.com/makr-code/wordpressPlugins
 * Description: Interactive release timeline visualization with Mermaid.js for ThemisDB versions, featuring GitHub API integration, CHANGELOG parsing, and multiple timeline views.
 * Version: 1.0.0
 * Author: ThemisDB Team
 * Author URI: https://github.com/makr-code/wordpressPlugins
 * License: MIT
 * Text Domain: themisdb-release-timeline
 */

// Prevent direct access
if (!defined('ABSPATH')) {
    exit;
}

// Plugin constants
define('THEMISDB_RT_VERSION', '1.0.0');
define('THEMISDB_RT_PLUGIN_DIR', plugin_dir_path(__FILE__));
define('THEMISDB_RT_PLUGIN_URL', plugin_dir_url(__FILE__));
define('THEMISDB_RT_PLUGIN_FILE', __FILE__);

// Load updater class
$themisdb_updater_local = THEMISDB_RT_PLUGIN_DIR . 'includes/class-themisdb-plugin-updater.php';
$themisdb_updater_shared = dirname(THEMISDB_RT_PLUGIN_DIR) . '/includes/class-themisdb-plugin-updater.php';

if (file_exists($themisdb_updater_local)) {
    require_once $themisdb_updater_local;
} elseif (file_exists($themisdb_updater_shared)) {
    require_once $themisdb_updater_shared;
}

// Initialize automatic updates
if (class_exists('ThemisDB_Plugin_Updater')) {
    new ThemisDB_Plugin_Updater(
        THEMISDB_RT_PLUGIN_FILE,
        'themisdb-release-timeline',
        THEMISDB_RT_VERSION
    );
}

/**
 * Enqueue scripts and styles
 */
function themisdb_rt_enqueue_scripts() {
    // Mermaid.js from CDN
    wp_enqueue_script(
        'mermaid-js',
        'https://cdn.jsdelivr.net/npm/mermaid@10.0.0/dist/mermaid.min.js',
        array(),
        '10.0.0',
        true
    );
    
    // Plugin CSS
    wp_enqueue_style(
        'themisdb-release-timeline-css',
        THEMISDB_RT_PLUGIN_URL . 'assets/css/release-timeline.css',
        array(),
        THEMISDB_RT_VERSION
    );
    
    // Plugin JS
    wp_enqueue_script(
        'themisdb-release-timeline-js',
        THEMISDB_RT_PLUGIN_URL . 'assets/js/release-timeline.js',
        array('jquery', 'mermaid-js'),
        THEMISDB_RT_VERSION,
        true
    );
    
    // Localize script with AJAX URL and nonce
    wp_localize_script('themisdb-release-timeline-js', 'themisdbRTData', array(
        'ajaxUrl' => admin_url('admin-ajax.php'),
        'nonce' => wp_create_nonce('themisdb_rt_nonce')
    ));
}
add_action('wp_enqueue_scripts', 'themisdb_rt_enqueue_scripts');

/**
 * Register shortcode
 */
function themisdb_rt_shortcode($atts) {
    $atts = shortcode_atts(array(
        'view' => 'chronological', // chronological, gantt, mindmap
        'theme' => 'neutral', // neutral, dark, forest
        'releases' => '10',
        'source' => 'github', // github, changelog, manual
        'show_breaking' => 'true',
        'show_features' => 'true',
        'interactive' => 'true'
    ), $atts);
    
    ob_start();
    include THEMISDB_RT_PLUGIN_DIR . 'templates/timeline.php';
    return ob_get_clean();
}
add_shortcode('themisdb_release_timeline', 'themisdb_rt_shortcode');

/**
 * AJAX handler: Load release data
 */
function themisdb_rt_ajax_load_data() {
    check_ajax_referer('themisdb_rt_nonce', 'nonce');
    
    $source = sanitize_text_field($_POST['source'] ?? 'github');
    $view = sanitize_text_field($_POST['view'] ?? 'chronological');
    $releases_count = intval($_POST['releases'] ?? 10);
    
    // Check transient cache
    $cache_key = 'themisdb_rt_' . $source . '_' . $releases_count;
    $cached_data = get_transient($cache_key);
    
    if ($cached_data !== false) {
        wp_send_json_success($cached_data);
        return;
    }
    
    $data = array();
    
    switch ($source) {
        case 'github':
            $data = themisdb_rt_fetch_github_releases($releases_count);
            break;
        case 'changelog':
            $data = themisdb_rt_parse_changelog($releases_count);
            break;
        case 'manual':
            $data = themisdb_rt_get_manual_releases($releases_count);
            break;
        default:
            $data = themisdb_rt_get_default_releases();
    }
    
    // Cache for 1 hour
    set_transient($cache_key, $data, HOUR_IN_SECONDS);
    
    wp_send_json_success($data);
}
add_action('wp_ajax_themisdb_rt_load_data', 'themisdb_rt_ajax_load_data');
add_action('wp_ajax_nopriv_themisdb_rt_load_data', 'themisdb_rt_ajax_load_data');

/**
 * Fetch releases from GitHub API
 */
function themisdb_rt_fetch_github_releases($count = 10) {
    $github_repo = get_option('themisdb_rt_github_repo', 'makr-code/wordpressPlugins');
    $api_url = "https://api.github.com/repos/{$github_repo}/releases?per_page={$count}";
    
    $response = wp_remote_get($api_url, array(
        'headers' => array(
            'Accept' => 'application/vnd.github.v3+json',
            'User-Agent' => 'ThemisDB-Release-Timeline'
        ),
        'timeout' => 15
    ));
    
    if (is_wp_error($response)) {
        return themisdb_rt_get_default_releases();
    }
    
    $body = wp_remote_retrieve_body($response);
    $releases = json_decode($body, true);
    
    if (!is_array($releases)) {
        return themisdb_rt_get_default_releases();
    }
    
    $formatted = array();
    foreach ($releases as $release) {
        $formatted[] = array(
            'version' => $release['tag_name'],
            'name' => $release['name'],
            'date' => date('Y-m-d', strtotime($release['published_at'])),
            'body' => $release['body'],
            'breaking' => stripos($release['body'], 'breaking') !== false,
            'features' => themisdb_rt_extract_features($release['body']),
            'url' => $release['html_url']
        );
    }
    
    return $formatted;
}

/**
 * Parse CHANGELOG.md file
 */
function themisdb_rt_parse_changelog($count = 10) {
    $changelog_path = get_option('themisdb_rt_changelog_path', '');
    
    if (empty($changelog_path) || !file_exists($changelog_path)) {
        return themisdb_rt_get_default_releases();
    }
    
    $content = file_get_contents($changelog_path);
    $releases = array();
    
    // Parse markdown changelog (simplified)
    preg_match_all('/##\s*\[([\d.]+)\]\s*-\s*(\d{4}-\d{2}-\d{2})(.*?)(?=##\s*\[|$)/s', $content, $matches, PREG_SET_ORDER);
    
    foreach (array_slice($matches, 0, $count) as $match) {
        $releases[] = array(
            'version' => $match[1],
            'name' => 'Version ' . $match[1],
            'date' => $match[2],
            'body' => trim($match[3]),
            'breaking' => stripos($match[3], 'breaking') !== false,
            'features' => themisdb_rt_extract_features($match[3]),
            'url' => ''
        );
    }
    
    return !empty($releases) ? $releases : themisdb_rt_get_default_releases();
}

/**
 * Get manual releases from settings
 */
function themisdb_rt_get_manual_releases($count = 10) {
    $manual_releases = get_option('themisdb_rt_manual_releases', '');
    
    if (empty($manual_releases)) {
        return themisdb_rt_get_default_releases();
    }
    
    $releases = json_decode($manual_releases, true);
    
    if (!is_array($releases)) {
        return themisdb_rt_get_default_releases();
    }
    
    return array_slice($releases, 0, $count);
}

/**
 * Extract features from release notes
 */
function themisdb_rt_extract_features($body) {
    $features = array();
    
    // Extract bullet points
    preg_match_all('/[-*]\s*(.+)$/m', $body, $matches);
    
    foreach ($matches[1] as $item) {
        $item = trim($item);
        if (!empty($item)) {
            $features[] = $item;
        }
    }
    
    return array_slice($features, 0, 5); // Limit to 5 features
}

/**
 * Get default/demo releases
 */
function themisdb_rt_get_default_releases() {
    return array(
        array(
            'version' => 'v1.0.0',
            'name' => 'ThemisDB 1.0.0 - Initial Release',
            'date' => '2024-01-15',
            'body' => 'Initial release with multi-model support, LLM integration, and RocksDB storage.',
            'breaking' => false,
            'features' => array(
                'Multi-model database (Relational, Graph, Vector, Document)',
                'llama.cpp integration for LLM queries',
                'RocksDB-based storage engine',
                'AQL query language',
                'REST API'
            ),
            'url' => '#'
        ),
        array(
            'version' => 'v1.1.0',
            'name' => 'ThemisDB 1.1.0 - Performance Update',
            'date' => '2024-03-20',
            'body' => 'Performance improvements and vector search enhancements.',
            'breaking' => false,
            'features' => array(
                'Improved vector search performance',
                'Query optimizer enhancements',
                'Better caching layer',
                'New aggregation functions'
            ),
            'url' => '#'
        ),
        array(
            'version' => 'v2.0.0',
            'name' => 'ThemisDB 2.0.0 - Major Update',
            'date' => '2024-06-10',
            'body' => 'Major update with breaking changes. New sharding architecture and ACID transactions.',
            'breaking' => true,
            'features' => array(
                'Sharding and RAID support',
                'ACID transactions',
                'New query planner',
                'Breaking: Changed AQL syntax for joins',
                'Breaking: New connection protocol'
            ),
            'url' => '#'
        ),
        array(
            'version' => 'v2.1.0',
            'name' => 'ThemisDB 2.1.0 - LLM Enhancements',
            'date' => '2024-09-05',
            'body' => 'Enhanced LLM integration with llama.cpp 2.0 and better prompt management.',
            'breaking' => false,
            'features' => array(
                'llama.cpp 2.0 integration',
                'Prompt template system',
                'Model management improvements',
                'CPU-only inference optimization'
            ),
            'url' => '#'
        ),
        array(
            'version' => 'v2.2.0',
            'name' => 'ThemisDB 2.2.0 - Wikipedia Support',
            'date' => '2024-12-01',
            'body' => 'Optimizations for knowledge graph use cases, including Wikipedia import tools.',
            'breaking' => false,
            'features' => array(
                'Wikipedia ingestion tools',
                'Knowledge graph optimizations',
                'Enhanced semantic search',
                'Category hierarchy support',
                'Multi-language embeddings'
            ),
            'url' => '#'
        )
    );
}

/**
 * Admin settings page
 */
function themisdb_rt_admin_menu() {
    add_options_page(
        'Release Timeline Settings',
        'Release Timeline',
        'manage_options',
        'themisdb-release-timeline',
        'themisdb_rt_settings_page'
    );
}
add_action('admin_menu', 'themisdb_rt_admin_menu');

/**
 * Settings page content
 */
function themisdb_rt_settings_page() {
    include THEMISDB_RT_PLUGIN_DIR . 'templates/admin-settings.php';
}

/**
 * Register settings
 */
function themisdb_rt_register_settings() {
    register_setting('themisdb_rt_settings', 'themisdb_rt_github_repo');
    register_setting('themisdb_rt_settings', 'themisdb_rt_changelog_path');
    register_setting('themisdb_rt_settings', 'themisdb_rt_manual_releases');
    register_setting('themisdb_rt_settings', 'themisdb_rt_default_view');
    register_setting('themisdb_rt_settings', 'themisdb_rt_default_theme');
    register_setting('themisdb_rt_settings', 'themisdb_rt_show_breaking');
    register_setting('themisdb_rt_settings', 'themisdb_rt_show_features');
}
add_action('admin_init', 'themisdb_rt_register_settings');
