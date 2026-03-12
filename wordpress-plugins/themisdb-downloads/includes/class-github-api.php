<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-github-api.php                               ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:17                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     295                                            ║
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
 * GitHub API Handler
 * Handles communication with GitHub API to fetch releases
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Downloads_GitHub_API {
    
    private $repo;
    private $token;
    private $cache_duration;
    
    public function __construct() {
        $this->repo = get_option('themisdb_github_repo', 'makr-code/wordpressPlugins');
        $this->token = get_option('themisdb_github_token', '');
        $this->cache_duration = get_option('themisdb_cache_duration', 3600);
    }
    
    /**
     * Get the latest release from GitHub
     * 
     * @return array|WP_Error Release data or error
     */
    public function get_latest_release() {
        // Check cache first
        $cached = get_transient('themisdb_latest_release');
        if ($cached !== false) {
            return $cached;
        }
        
        $url = "https://api.github.com/repos/{$this->repo}/releases/latest";
        $response = $this->make_request($url);
        
        if (is_wp_error($response)) {
            return $response;
        }
        
        // Parse release data
        $release = $this->parse_release($response);
        
        // Cache the result
        set_transient('themisdb_latest_release', $release, $this->cache_duration);
        
        return $release;
    }
    
    /**
     * Get all releases from GitHub
     * 
     * @param int $per_page Number of releases to fetch
     * @return array|WP_Error Array of releases or error
     */
    public function get_all_releases($per_page = 10) {
        // Check cache first
        $cache_key = 'themisdb_all_releases_' . $per_page;
        $cached = get_transient($cache_key);
        if ($cached !== false) {
            return $cached;
        }
        
        $url = "https://api.github.com/repos/{$this->repo}/releases?per_page={$per_page}";
        $response = $this->make_request($url);
        
        if (is_wp_error($response)) {
            return $response;
        }
        
        // Parse all releases
        $releases = array();
        foreach ($response as $release_data) {
            $releases[] = $this->parse_release($release_data);
        }
        
        // Cache the result
        set_transient($cache_key, $releases, $this->cache_duration);
        
        return $releases;
    }
    
    /**
     * Parse release data from GitHub API response
     * 
     * @param object $data Release data from API
     * @return array Parsed release data
     */
    private function parse_release($data) {
        $release = array(
            'version' => $data->tag_name,
            'name' => $data->name,
            'published_at' => $data->published_at,
            'body' => $data->body,
            'html_url' => $data->html_url,
            'assets' => array(),
            'sha256sums' => array(),
            'readme' => '',
            'changelog' => ''
        );
        
        // Parse assets
        foreach ($data->assets as $asset) {
            $asset_data = array(
                'name' => $asset->name,
                'size' => $asset->size,
                'download_url' => $asset->browser_download_url,
                'download_count' => $asset->download_count
            );
            
            // Check if this is a SHA256SUMS file
            if (strpos($asset->name, 'SHA256SUMS') !== false || strpos($asset->name, 'sha256') !== false) {
                // Download and parse SHA256 file
                $sha256_content = $this->download_sha256_file($asset->browser_download_url);
                if (!is_wp_error($sha256_content)) {
                    $release['sha256sums'] = $this->parse_sha256_file($sha256_content);
                }
            }
            
            // Check if this is a README file
            if (preg_match('/^README(_.*)?\.md$/i', $asset->name)) {
                $readme_content = $this->download_text_file($asset->browser_download_url);
                if (!is_wp_error($readme_content) && !empty($readme_content)) {
                    $release['readme'] = $readme_content;
                }
            }
            
            // Check if this is a CHANGELOG file
            if (preg_match('/^CHANGELOG(_.*)?\.md$/i', $asset->name) || preg_match('/^RELEASE_NOTES(_.*)?\.md$/i', $asset->name)) {
                $changelog_content = $this->download_text_file($asset->browser_download_url);
                if (!is_wp_error($changelog_content) && !empty($changelog_content)) {
                    $release['changelog'] = $changelog_content;
                }
            }
            
            $release['assets'][] = $asset_data;
        }
        
        return $release;
    }
    
    /**
     * Download SHA256SUMS file content
     * 
     * @param string $url URL to SHA256SUMS file
     * @return string|WP_Error File content or error
     */
    private function download_sha256_file($url) {
        return $this->download_text_file($url);
    }
    
    /**
     * Download text file content (README, CHANGELOG, SHA256SUMS, etc.)
     * 
     * @param string $url URL to text file
     * @return string|WP_Error File content or error
     */
    private function download_text_file($url) {
        $args = array(
            'timeout' => 30
        );
        
        if (!empty($this->token)) {
            $args['headers'] = array(
                'Authorization' => 'Bearer ' . $this->token
            );
        }
        
        $response = wp_remote_get($url, $args);
        
        if (is_wp_error($response)) {
            return $response;
        }
        
        $code = wp_remote_retrieve_response_code($response);
        if ($code !== 200) {
            return new WP_Error('download_failed', 'Failed to download file');
        }
        
        return wp_remote_retrieve_body($response);
    }
    
    /**
     * Parse SHA256SUMS file content
     * 
     * @param string $content File content
     * @return array Array of filename => sha256 hash
     */
    private function parse_sha256_file($content) {
        $checksums = array();
        $lines = explode("\n", $content);
        
        foreach ($lines as $line) {
            $line = trim($line);
            if (empty($line) || strpos($line, '#') === 0) {
                continue;
            }
            
            // Parse format: "hash  filename" or "hash filename"
            if (preg_match('/^([a-f0-9]{64})\s+(.+)$/i', $line, $matches)) {
                $hash = $matches[1];
                $filename = trim($matches[2]);
                // Remove leading ./ or * from filename
                $filename = ltrim($filename, './');
                $filename = ltrim($filename, '*');
                $checksums[$filename] = $hash;
            }
        }
        
        return $checksums;
    }
    
    /**
     * Make HTTP request to GitHub API
     * 
     * @param string $url API endpoint URL
     * @return mixed|WP_Error Decoded JSON response or error
     */
    private function make_request($url) {
        $args = array(
            'timeout' => 30,
            'headers' => array(
                'Accept' => 'application/vnd.github.v3+json'
            )
        );
        
        // Add authorization header if token is set
        if (!empty($this->token)) {
            $args['headers']['Authorization'] = 'Bearer ' . $this->token;
        }
        
        $response = wp_remote_get($url, $args);
        
        if (is_wp_error($response)) {
            return $response;
        }
        
        $code = wp_remote_retrieve_response_code($response);
        if ($code !== 200) {
            return new WP_Error('api_error', 'GitHub API returned error code: ' . $code);
        }
        
        $body = wp_remote_retrieve_body($response);
        $data = json_decode($body);
        
        if (json_last_error() !== JSON_ERROR_NONE) {
            return new WP_Error('json_error', 'Failed to parse JSON response');
        }
        
        return $data;
    }
    
    /**
     * Clear all cached data
     */
    public function clear_cache() {
        delete_transient('themisdb_latest_release');
        
        // Clear all releases caches using WordPress transient API
        // Get common values for cache clearing
        $common_counts = array(1, 5, 10, 20, 30, 50);
        foreach ($common_counts as $count) {
            delete_transient('themisdb_all_releases_' . $count);
        }
        
        // Clear any custom cached counts from options
        $custom_count = get_option('themisdb_releases_count', 10);
        if (!in_array($custom_count, $common_counts)) {
            delete_transient('themisdb_all_releases_' . $custom_count);
        }
    }
}
