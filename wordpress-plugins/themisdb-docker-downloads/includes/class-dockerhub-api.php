<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-dockerhub-api.php                            ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:17                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     203                                            ║
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
 * Docker Hub API Handler
 * Handles communication with Docker Hub API to fetch image tags and metadata
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Docker_Downloads_DockerHub_API {
    
    private $namespace;
    private $repository;
    private $token;
    private $cache_duration;
    
    public function __construct() {
        $this->namespace = get_option('themisdb_docker_namespace', 'themisdb');
        $this->repository = get_option('themisdb_docker_repository', 'themisdb');
        $this->token = get_option('themisdb_docker_token', '');
        $this->cache_duration = get_option('themisdb_docker_cache_duration', 3600);
    }
    
    /**
     * Get the latest tags from Docker Hub
     * 
     * @param int $per_page Number of tags to fetch
     * @return array|WP_Error Tags data or error
     */
    public function get_tags($per_page = 10) {
        // Check cache first
        $cache_key = 'themisdb_docker_all_tags_' . $per_page;
        $cached = get_transient($cache_key);
        if ($cached !== false) {
            return $cached;
        }
        
        $url = sprintf(
            'https://hub.docker.com/v2/repositories/%s/%s/tags?page_size=%d',
            urlencode($this->namespace),
            urlencode($this->repository),
            intval($per_page)
        );
        $response = $this->make_request($url);
        
        if (is_wp_error($response)) {
            return $response;
        }
        
        // Parse tags data
        $tags = array();
        if (isset($response->results) && is_array($response->results)) {
            foreach ($response->results as $tag_data) {
                $tags[] = $this->parse_tag($tag_data);
            }
        }
        
        // Cache the result
        set_transient($cache_key, $tags, $this->cache_duration);
        
        return $tags;
    }
    
    /**
     * Get the latest tag from Docker Hub
     * 
     * @return array|WP_Error Tag data or error
     */
    public function get_latest_tag() {
        // Check cache first
        $cached = get_transient('themisdb_docker_latest_tags');
        if ($cached !== false) {
            return $cached;
        }
        
        $tags = $this->get_tags(1);
        if (is_wp_error($tags) || empty($tags)) {
            return is_wp_error($tags) ? $tags : new WP_Error('no_tags', 'No tags found');
        }
        
        $latest = $tags[0];
        
        // Cache the result
        set_transient('themisdb_docker_latest_tags', $latest, $this->cache_duration);
        
        return $latest;
    }
    
    /**
     * Parse tag data from Docker Hub API response
     * 
     * @param object $data Tag data from API
     * @return array Parsed tag data
     */
    private function parse_tag($data) {
        $tag = array(
            'name' => $data->name,
            'last_updated' => $data->last_updated,
            'full_size' => isset($data->full_size) ? $data->full_size : 0,
            'images' => array(),
            'pull_command' => "docker pull {$this->namespace}/{$this->repository}:{$data->name}",
            'tag_url' => "https://hub.docker.com/r/{$this->namespace}/{$this->repository}/tags?name={$data->name}"
        );
        
        // Parse image information (architecture-specific digests)
        if (isset($data->images) && is_array($data->images)) {
            foreach ($data->images as $image) {
                $image_data = array(
                    'architecture' => isset($image->architecture) ? $image->architecture : 'unknown',
                    'os' => isset($image->os) ? $image->os : 'linux',
                    'digest' => isset($image->digest) ? $image->digest : '',
                    'size' => isset($image->size) ? $image->size : 0,
                    'variant' => isset($image->variant) ? $image->variant : ''
                );
                $tag['images'][] = $image_data;
            }
        }
        
        return $tag;
    }
    
    /**
     * Make HTTP request to Docker Hub API
     * 
     * @param string $url API endpoint URL
     * @return mixed|WP_Error Decoded JSON response or error
     */
    private function make_request($url) {
        $args = array(
            'timeout' => 30,
            'headers' => array(
                'Accept' => 'application/json'
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
            return new WP_Error('api_error', 'Docker Hub API returned error code: ' . $code);
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
        delete_transient('themisdb_docker_latest_tags');
        
        // Clear all tags caches using WordPress transient API
        $common_counts = array(1, 5, 10, 20, 30, 50);
        foreach ($common_counts as $count) {
            delete_transient('themisdb_docker_all_tags_' . $count);
        }
        
        // Clear any custom cached counts from options
        $custom_count = get_option('themisdb_docker_tags_count', 10);
        if (!in_array($custom_count, $common_counts)) {
            delete_transient('themisdb_docker_all_tags_' . $custom_count);
        }
    }
}
