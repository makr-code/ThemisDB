<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-compendium-downloads.php                     ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:17                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     256                                            ║
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
 * ThemisDB Compendium Downloads Main Class
 * 
 * Handles shortcodes and download display
 */

// Prevent direct access
if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Compendium_Downloads {
    
    /**
     * Constructor
     */
    public function __construct() {
        // Register shortcodes
        add_shortcode('themisdb_compendium_downloads', array($this, 'render_downloads'));
        add_shortcode('themisdb_compendium', array($this, 'render_downloads')); // Alias
        
        // AJAX handlers for download tracking
        add_action('wp_ajax_themisdb_track_download', array($this, 'track_download'));
        add_action('wp_ajax_nopriv_themisdb_track_download', array($this, 'track_download'));
    }
    
    /**
     * Get release data from GitHub API or cache
     * 
     * @return array|false Release data or false on failure
     */
    public function get_release_data() {
        // Check cache first
        $cache_duration = get_option('themisdb_compendium_cache_duration', 3600);
        $cached_data = get_transient('themisdb_compendium_release_data');
        
        if ($cached_data !== false) {
            return $cached_data;
        }
        
        // Fetch from GitHub API
        $repo = get_option('themisdb_compendium_github_repo', 'makr-code/wordpressPlugins');
        $api_url = "https://api.github.com/repos/{$repo}/releases/latest";
        
        $response = wp_remote_get($api_url, array(
            'timeout' => 10,
            'headers' => array(
                'Accept' => 'application/vnd.github+json',
                'User-Agent' => 'ThemisDB-WordPress-Plugin'
            )
        ));
        
        if (is_wp_error($response)) {
            return false;
        }
        
        $body = wp_remote_retrieve_body($response);
        $data = json_decode($body, true);
        
        if (!$data || !isset($data['assets'])) {
            return false;
        }
        
        // Filter for compendium PDF files
        // Note: Search term is configurable via admin settings (default: 'kompendium')
        $search_term = get_option('themisdb_compendium_search_term', 'kompendium');
        $compendium_assets = array_filter($data['assets'], function($asset) use ($search_term) {
            $name = strtolower($asset['name']);
            $has_search_term = stripos($name, $search_term) !== false;
            $is_pdf = substr($name, -4) === '.pdf';
            return $has_search_term && $is_pdf;
        });
        
        $release_data = array(
            'version' => $data['tag_name'],
            'published_at' => $data['published_at'],
            'html_url' => $data['html_url'],
            'assets' => array_values($compendium_assets)
        );
        
        // Cache the data
        set_transient('themisdb_compendium_release_data', $release_data, $cache_duration);
        
        return $release_data;
    }
    
    /**
     * Format file size
     * 
     * @param int $bytes File size in bytes
     * @return string Formatted file size
     */
    private function format_file_size($bytes) {
        $units = array('B', 'KB', 'MB', 'GB');
        $bytes = max($bytes, 0);
        $pow = floor(($bytes > 0 ? log($bytes) : 0) / log(1024));
        $pow = min($pow, count($units) - 1);
        $bytes /= pow(1024, $pow);
        
        return round($bytes, 2) . ' ' . $units[$pow];
    }
    
    /**
     * Render downloads shortcode
     * 
     * @param array $atts Shortcode attributes
     * @return string HTML output
     */
    public function render_downloads($atts) {
        $atts = shortcode_atts(array(
            'style' => get_option('themisdb_compendium_button_style', 'modern'),
            'show_version' => 'yes',
            'show_date' => 'yes',
            'show_size' => get_option('themisdb_compendium_show_file_sizes', 1) ? 'yes' : 'no',
            'layout' => 'cards' // cards, list, compact
        ), $atts);
        
        $release_data = $this->get_release_data();
        
        if (!$release_data || empty($release_data['assets'])) {
            return '<div class="themisdb-compendium-error">' . 
                   '<p>' . __('Keine Kompendium-Downloads verfügbar.', 'themisdb-compendium-downloads') . '</p>' .
                   '</div>';
        }
        
        $output = '<div class="themisdb-compendium-downloads themisdb-style-' . esc_attr($atts['style']) . ' themisdb-layout-' . esc_attr($atts['layout']) . '">';
        
        // Header
        $output .= '<div class="themisdb-compendium-header">';
        $output .= '<h3 class="themisdb-compendium-title">' . 
                   '<span class="themisdb-icon">📚</span> ' .
                   __('ThemisDB Kompendium Downloads', 'themisdb-compendium-downloads') . 
                   '</h3>';
        
        if ($atts['show_version'] === 'yes') {
            $output .= '<p class="themisdb-compendium-version">' . 
                       __('Version:', 'themisdb-compendium-downloads') . ' ' . 
                       '<strong>' . esc_html($release_data['version']) . '</strong>';
            
            if ($atts['show_date'] === 'yes') {
                $date = date_i18n(get_option('date_format'), strtotime($release_data['published_at']));
                $output .= ' <span class="themisdb-date">(' . $date . ')</span>';
            }
            
            $output .= '</p>';
        }
        
        $output .= '</div>';
        
        // Download cards/list
        $output .= '<div class="themisdb-compendium-items">';
        
        foreach ($release_data['assets'] as $asset) {
            $name = $asset['name'];
            $url = $asset['browser_download_url'];
            $size = $asset['size'];
            
            // Determine variant (print or professional)
            $variant = 'default';
            $variant_label = __('Standard', 'themisdb-compendium-downloads');
            $variant_icon = '📄';
            
            if (stripos($name, 'professional') !== false) {
                $variant = 'professional';
                $variant_label = __('Professional', 'themisdb-compendium-downloads');
                $variant_icon = '🎓';
            } elseif (stripos($name, 'print') !== false) {
                $variant = 'print';
                $variant_label = __('Druckversion', 'themisdb-compendium-downloads');
                $variant_icon = '🖨️';
            }
            
            $output .= '<div class="themisdb-compendium-item themisdb-variant-' . esc_attr($variant) . '">';
            $output .= '<div class="themisdb-item-icon">' . $variant_icon . '</div>';
            $output .= '<div class="themisdb-item-content">';
            $output .= '<h4 class="themisdb-item-title">' . esc_html($variant_label) . '</h4>';
            
            if ($atts['show_size'] === 'yes') {
                $output .= '<p class="themisdb-item-size">' . $this->format_file_size($size) . '</p>';
            }
            
            $output .= '<a href="' . esc_url($url) . '" class="themisdb-download-button" data-asset="' . esc_attr($name) . '" target="_blank" rel="noopener">';
            $output .= '<span class="themisdb-button-icon">⬇️</span> ';
            $output .= __('PDF herunterladen', 'themisdb-compendium-downloads');
            $output .= '</a>';
            
            $output .= '</div>'; // item-content
            $output .= '</div>'; // compendium-item
        }
        
        $output .= '</div>'; // compendium-items
        
        // Footer with GitHub link
        $output .= '<div class="themisdb-compendium-footer">';
        $output .= '<p class="themisdb-github-link">';
        $output .= '<a href="' . esc_url($release_data['html_url']) . '" target="_blank" rel="noopener">';
        $output .= '<span class="themisdb-github-icon">🔗</span> ';
        $output .= __('Alle Versionen auf GitHub anzeigen', 'themisdb-compendium-downloads');
        $output .= '</a>';
        $output .= '</p>';
        $output .= '</div>';
        
        $output .= '</div>'; // themisdb-compendium-downloads
        
        return $output;
    }
    
    /**
     * Track download event (AJAX handler)
     */
    public function track_download() {
        check_ajax_referer('themisdb_compendium_download_track', 'nonce');
        
        $asset = isset($_POST['asset']) ? sanitize_text_field($_POST['asset']) : '';
        
        if (!empty($asset)) {
            // Update download counter
            $downloads = get_option('themisdb_compendium_download_stats', array());
            
            if (!isset($downloads[$asset])) {
                $downloads[$asset] = 0;
            }
            
            $downloads[$asset]++;
            
            update_option('themisdb_compendium_download_stats', $downloads);
            
            wp_send_json_success(array('downloads' => $downloads[$asset]));
        } else {
            wp_send_json_error();
        }
    }
}
