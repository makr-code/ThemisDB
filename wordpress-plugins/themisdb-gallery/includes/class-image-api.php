<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-image-api.php                                ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:18                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     351                                            ║
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
 * Image API Handler
 * 
 * Handles communication with external image APIs (Unsplash, Pexels, Pixabay)
 * and AI image generation services
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Gallery_Image_API {
    
    /**
     * Search images from Unsplash
     * 
     * @param string $query Search query
     * @param int $page Page number
     * @param int $per_page Images per page
     * @return array|WP_Error Results or error
     */
    public static function search_unsplash($query, $page = 1, $per_page = 20) {
        $api_key = get_option('themisdb_gallery_unsplash_key');
        
        if (empty($api_key)) {
            return new WP_Error('no_api_key', __('Unsplash API-Schlüssel nicht konfiguriert', 'themisdb-gallery'));
        }
        
        $cache_key = 'themisdb_gallery_unsplash_' . md5($query . $page . $per_page);
        $cached = get_transient($cache_key);
        
        if ($cached !== false) {
            return $cached;
        }
        
        $url = sprintf(
            'https://api.unsplash.com/search/photos?query=%s&page=%d&per_page=%d',
            urlencode($query),
            $page,
            $per_page
        );
        
        $response = wp_remote_get($url, array(
            'headers' => array(
                'Authorization' => 'Client-ID ' . $api_key,
            ),
            'timeout' => 15,
        ));
        
        if (is_wp_error($response)) {
            return $response;
        }
        
        $body = wp_remote_retrieve_body($response);
        $data = json_decode($body, true);
        
        if (empty($data['results'])) {
            return array();
        }
        
        $results = array();
        foreach ($data['results'] as $photo) {
            $results[] = array(
                'id' => $photo['id'],
                'title' => $photo['description'] ?? $photo['alt_description'] ?? 'Untitled',
                'url' => $photo['urls']['regular'],
                'thumb' => $photo['urls']['thumb'],
                'width' => $photo['width'],
                'height' => $photo['height'],
                'author' => $photo['user']['name'],
                'author_url' => $photo['user']['links']['html'],
                'source' => 'Unsplash',
                'source_url' => $photo['links']['html'],
                'license' => 'Unsplash License',
                'license_url' => 'https://unsplash.com/license',
            );
        }
        
        $cache_duration = get_option('themisdb_gallery_cache_duration', 3600);
        set_transient($cache_key, $results, $cache_duration);
        
        return $results;
    }
    
    /**
     * Search images from Pexels
     * 
     * @param string $query Search query
     * @param int $page Page number
     * @param int $per_page Images per page
     * @return array|WP_Error Results or error
     */
    public static function search_pexels($query, $page = 1, $per_page = 20) {
        $api_key = get_option('themisdb_gallery_pexels_key');
        
        if (empty($api_key)) {
            return new WP_Error('no_api_key', __('Pexels API-Schlüssel nicht konfiguriert', 'themisdb-gallery'));
        }
        
        $cache_key = 'themisdb_gallery_pexels_' . md5($query . $page . $per_page);
        $cached = get_transient($cache_key);
        
        if ($cached !== false) {
            return $cached;
        }
        
        $url = sprintf(
            'https://api.pexels.com/v1/search?query=%s&page=%d&per_page=%d',
            urlencode($query),
            $page,
            $per_page
        );
        
        $response = wp_remote_get($url, array(
            'headers' => array(
                'Authorization' => $api_key,
            ),
            'timeout' => 15,
        ));
        
        if (is_wp_error($response)) {
            return $response;
        }
        
        $body = wp_remote_retrieve_body($response);
        $data = json_decode($body, true);
        
        if (empty($data['photos'])) {
            return array();
        }
        
        $results = array();
        foreach ($data['photos'] as $photo) {
            $results[] = array(
                'id' => $photo['id'],
                'title' => $photo['alt'] ?? 'Untitled',
                'url' => $photo['src']['large'],
                'thumb' => $photo['src']['medium'],
                'width' => $photo['width'],
                'height' => $photo['height'],
                'author' => $photo['photographer'],
                'author_url' => $photo['photographer_url'],
                'source' => 'Pexels',
                'source_url' => $photo['url'],
                'license' => 'Pexels License',
                'license_url' => 'https://www.pexels.com/license/',
            );
        }
        
        $cache_duration = get_option('themisdb_gallery_cache_duration', 3600);
        set_transient($cache_key, $results, $cache_duration);
        
        return $results;
    }
    
    /**
     * Search images from Pixabay
     * 
     * @param string $query Search query
     * @param int $page Page number
     * @param int $per_page Images per page
     * @return array|WP_Error Results or error
     */
    public static function search_pixabay($query, $page = 1, $per_page = 20) {
        $api_key = get_option('themisdb_gallery_pixabay_key');
        
        if (empty($api_key)) {
            return new WP_Error('no_api_key', __('Pixabay API-Schlüssel nicht konfiguriert', 'themisdb-gallery'));
        }
        
        $cache_key = 'themisdb_gallery_pixabay_' . md5($query . $page . $per_page);
        $cached = get_transient($cache_key);
        
        if ($cached !== false) {
            return $cached;
        }
        
        $url = sprintf(
            'https://pixabay.com/api/?key=%s&q=%s&page=%d&per_page=%d&image_type=photo',
            $api_key,
            urlencode($query),
            $page,
            $per_page
        );
        
        $response = wp_remote_get($url, array(
            'timeout' => 15,
        ));
        
        if (is_wp_error($response)) {
            return $response;
        }
        
        $body = wp_remote_retrieve_body($response);
        $data = json_decode($body, true);
        
        if (empty($data['hits'])) {
            return array();
        }
        
        $results = array();
        foreach ($data['hits'] as $photo) {
            $results[] = array(
                'id' => $photo['id'],
                'title' => $photo['tags'] ?? 'Untitled',
                'url' => $photo['largeImageURL'],
                'thumb' => $photo['previewURL'],
                'width' => $photo['imageWidth'],
                'height' => $photo['imageHeight'],
                'author' => $photo['user'],
                'author_url' => 'https://pixabay.com/users/' . $photo['user'] . '-' . $photo['user_id'],
                'source' => 'Pixabay',
                'source_url' => $photo['pageURL'],
                'license' => 'Pixabay License',
                'license_url' => 'https://pixabay.com/service/license/',
            );
        }
        
        $cache_duration = get_option('themisdb_gallery_cache_duration', 3600);
        set_transient($cache_key, $results, $cache_duration);
        
        return $results;
    }
    
    /**
     * Search images from all available providers
     * 
     * @param string $query Search query
     * @param string $provider Provider name or 'all'
     * @param int $page Page number
     * @param int $per_page Images per page
     * @return array|WP_Error Results or error
     */
    public static function search_images($query, $provider = 'all', $page = 1, $per_page = 20) {
        if (empty($query)) {
            return new WP_Error('empty_query', __('Suchbegriff darf nicht leer sein', 'themisdb-gallery'));
        }
        
        $results = array();
        
        if ($provider === 'all' || $provider === 'unsplash') {
            $unsplash_results = self::search_unsplash($query, $page, $per_page);
            if (!is_wp_error($unsplash_results)) {
                $results = array_merge($results, $unsplash_results);
            }
        }
        
        if ($provider === 'all' || $provider === 'pexels') {
            $pexels_results = self::search_pexels($query, $page, $per_page);
            if (!is_wp_error($pexels_results)) {
                $results = array_merge($results, $pexels_results);
            }
        }
        
        if ($provider === 'all' || $provider === 'pixabay') {
            $pixabay_results = self::search_pixabay($query, $page, $per_page);
            if (!is_wp_error($pixabay_results)) {
                $results = array_merge($results, $pixabay_results);
            }
        }
        
        // Shuffle results when searching all providers
        if ($provider === 'all' && !empty($results)) {
            shuffle($results);
        }
        
        return $results;
    }
    
    /**
     * Generate image using AI (OpenAI DALL-E)
     * 
     * @param string $prompt Image description
     * @param string $size Image size (256x256, 512x512, 1024x1024)
     * @return array|WP_Error Result or error
     */
    public static function generate_ai_image($prompt, $size = '1024x1024') {
        $api_key = get_option('themisdb_gallery_openai_key');
        
        if (empty($api_key)) {
            return new WP_Error('no_api_key', __('OpenAI API-Schlüssel nicht konfiguriert', 'themisdb-gallery'));
        }
        
        $response = wp_remote_post('https://api.openai.com/v1/images/generations', array(
            'headers' => array(
                'Authorization' => 'Bearer ' . $api_key,
                'Content-Type' => 'application/json',
            ),
            'body' => json_encode(array(
                'prompt' => $prompt,
                'n' => 1,
                'size' => $size,
            )),
            'timeout' => 60,
        ));
        
        if (is_wp_error($response)) {
            return $response;
        }
        
        $body = wp_remote_retrieve_body($response);
        $data = json_decode($body, true);
        
        if (empty($data['data'][0]['url'])) {
            return new WP_Error('no_image', __('Kein Bild generiert', 'themisdb-gallery'));
        }
        
        // Parse size safely
        $size_parts = explode('x', $size);
        $width = isset($size_parts[0]) ? (int) $size_parts[0] : 1024;
        $height = isset($size_parts[1]) ? (int) $size_parts[1] : 1024;
        
        return array(
            'id' => 'ai_' . time(),
            'title' => $prompt,
            'url' => $data['data'][0]['url'],
            'thumb' => $data['data'][0]['url'],
            'width' => $width,
            'height' => $height,
            'author' => 'AI Generated (DALL-E)',
            'author_url' => '',
            'source' => 'OpenAI DALL-E',
            'source_url' => '',
            'license' => 'AI Generated',
            'license_url' => 'https://openai.com/policies/terms-of-use',
        );
    }
}
