<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-github-sync.php                              ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     355                                            ║
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
 * GitHub Sync
 * Handles synchronization with GitHub Wiki
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Wiki_GitHub_Sync {
    
    private $api_url = 'https://api.github.com';
    
    /**
     * Constructor
     */
    public function __construct() {
        // AJAX handlers
        add_action('wp_ajax_sync_wiki_to_github', array($this, 'ajax_push_to_github'));
        add_action('wp_ajax_sync_wiki_from_github', array($this, 'ajax_pull_from_github'));
        add_action('wp_ajax_bulk_sync_wiki', array($this, 'ajax_bulk_sync'));
    }
    
    /**
     * Get GitHub Configuration
     */
    private function get_config() {
        return array(
            'repo' => get_option('themisdb_wiki_github_repo', ''),
            'token' => get_option('themisdb_wiki_github_token', ''),
            'branch' => get_option('themisdb_wiki_github_branch', 'main')
        );
    }
    
    /**
     * Push Wiki Page to GitHub
     */
    public function push_to_github($post_id) {
        $config = $this->get_config();
        
        if (empty($config['repo']) || empty($config['token'])) {
            return new WP_Error('config_error', __('GitHub configuration is incomplete', 'themisdb-wiki'));
        }
        
        $post = get_post($post_id);
        if (!$post || $post->post_type !== 'themisdb_wiki') {
            return new WP_Error('invalid_post', __('Invalid wiki post', 'themisdb-wiki'));
        }
        
        $markdown = get_post_meta($post_id, '_wiki_markdown', true);
        if (empty($markdown)) {
            $markdown = $post->post_content;
        }
        
        $github_path = get_post_meta($post_id, '_wiki_github_path', true);
        if (empty($github_path)) {
            $github_path = $post->post_name . '.md';
        }
        
        // Get file SHA if exists (for updates)
        $sha = $this->get_file_sha($config, $github_path);
        
        // Prepare request body
        $body = array(
            'message' => 'Update ' . $post->post_title,
            'content' => base64_encode($markdown),
            'branch' => $config['branch']
        );
        
        if ($sha) {
            $body['sha'] = $sha;
        }
        
        // Make API request
        $response = wp_remote_post(
            "{$this->api_url}/repos/{$config['repo']}/contents/wiki/{$github_path}",
            array(
                'headers' => array(
                    'Authorization' => 'Bearer ' . $config['token'],
                    'Content-Type' => 'application/json',
                    'User-Agent' => 'ThemisDB-Wiki-Integration'
                ),
                'body' => wp_json_encode($body),
                'timeout' => 30
            )
        );
        
        if (is_wp_error($response)) {
            return $response;
        }
        
        $status_code = wp_remote_retrieve_response_code($response);
        
        if ($status_code !== 200 && $status_code !== 201) {
            $body = json_decode(wp_remote_retrieve_body($response), true);
            return new WP_Error('github_api_error', 
                isset($body['message']) ? $body['message'] : __('GitHub API error', 'themisdb-wiki')
            );
        }
        
        // Update last sync time
        update_post_meta($post_id, '_wiki_last_sync', current_time('timestamp'));
        
        return true;
    }
    
    /**
     * Pull Wiki Page from GitHub
     */
    public function pull_from_github($github_path) {
        $config = $this->get_config();
        
        if (empty($config['repo']) || empty($config['token'])) {
            return new WP_Error('config_error', __('GitHub configuration is incomplete', 'themisdb-wiki'));
        }
        
        // Fetch file from GitHub
        $response = wp_remote_get(
            "{$this->api_url}/repos/{$config['repo']}/contents/wiki/{$github_path}",
            array(
                'headers' => array(
                    'Authorization' => 'Bearer ' . $config['token'],
                    'Accept' => 'application/vnd.github.v3+json',
                    'User-Agent' => 'ThemisDB-Wiki-Integration'
                ),
                'timeout' => 30
            )
        );
        
        if (is_wp_error($response)) {
            return $response;
        }
        
        $status_code = wp_remote_retrieve_response_code($response);
        
        if ($status_code !== 200) {
            return new WP_Error('github_api_error', __('Failed to fetch file from GitHub', 'themisdb-wiki'));
        }
        
        $data = json_decode(wp_remote_retrieve_body($response), true);
        $markdown = base64_decode($data['content']);
        
        // Find or create WordPress post
        $slug = basename($github_path, '.md');
        $post = get_page_by_path($slug, OBJECT, 'themisdb_wiki');
        
        // Convert markdown to HTML
        $wikilinks = new ThemisDB_WikiLinks();
        $html = $wikilinks->convert_markdown_with_wikilinks($markdown);
        
        if (!$post) {
            // Create new post
            $post_id = wp_insert_post(array(
                'post_type' => 'themisdb_wiki',
                'post_title' => ucwords(str_replace('-', ' ', $slug)),
                'post_name' => $slug,
                'post_status' => 'publish',
                'post_content' => $html
            ));
        } else {
            // Update existing post
            $post_id = $post->ID;
            wp_update_post(array(
                'ID' => $post_id,
                'post_content' => $html
            ));
        }
        
        // Save markdown source
        update_post_meta($post_id, '_wiki_markdown', $markdown);
        update_post_meta($post_id, '_wiki_github_path', $github_path);
        update_post_meta($post_id, '_wiki_last_sync', current_time('timestamp'));
        
        return $post_id;
    }
    
    /**
     * Get File SHA from GitHub (needed for updates)
     */
    private function get_file_sha($config, $github_path) {
        $response = wp_remote_get(
            "{$this->api_url}/repos/{$config['repo']}/contents/wiki/{$github_path}",
            array(
                'headers' => array(
                    'Authorization' => 'Bearer ' . $config['token'],
                    'Accept' => 'application/vnd.github.v3+json',
                    'User-Agent' => 'ThemisDB-Wiki-Integration'
                ),
                'timeout' => 30
            )
        );
        
        if (is_wp_error($response) || wp_remote_retrieve_response_code($response) !== 200) {
            return null;
        }
        
        $data = json_decode(wp_remote_retrieve_body($response), true);
        return isset($data['sha']) ? $data['sha'] : null;
    }
    
    /**
     * Bulk Sync All Pages
     */
    public function sync_all() {
        $config = $this->get_config();
        
        if (empty($config['repo']) || empty($config['token'])) {
            return new WP_Error('config_error', __('GitHub configuration is incomplete', 'themisdb-wiki'));
        }
        
        // Get all GitHub wiki files
        $response = wp_remote_get(
            "{$this->api_url}/repos/{$config['repo']}/contents/wiki",
            array(
                'headers' => array(
                    'Authorization' => 'Bearer ' . $config['token'],
                    'Accept' => 'application/vnd.github.v3+json',
                    'User-Agent' => 'ThemisDB-Wiki-Integration'
                ),
                'timeout' => 30
            )
        );
        
        if (is_wp_error($response)) {
            return $response;
        }
        
        $status_code = wp_remote_retrieve_response_code($response);
        
        if ($status_code !== 200) {
            return new WP_Error('github_api_error', __('Failed to list GitHub wiki files', 'themisdb-wiki'));
        }
        
        $files = json_decode(wp_remote_retrieve_body($response), true);
        $synced = 0;
        $errors = array();
        
        foreach ($files as $file) {
            if ($file['type'] === 'file' && substr($file['name'], -3) === '.md') {
                $result = $this->pull_from_github($file['name']);
                if (is_wp_error($result)) {
                    $errors[] = $file['name'] . ': ' . $result->get_error_message();
                } else {
                    $synced++;
                }
            }
        }
        
        return array(
            'synced' => $synced,
            'errors' => $errors
        );
    }
    
    /**
     * AJAX: Push to GitHub
     */
    public function ajax_push_to_github() {
        check_ajax_referer('themisdb_wiki_nonce', 'nonce');
        
        $post_id = isset($_POST['post_id']) ? intval($_POST['post_id']) : 0;
        
        if (!$post_id || !current_user_can('edit_post', $post_id)) {
            wp_send_json_error(array('message' => __('Unauthorized', 'themisdb-wiki')));
        }
        
        $result = $this->push_to_github($post_id);
        
        if (is_wp_error($result)) {
            wp_send_json_error(array('message' => $result->get_error_message()));
        }
        
        wp_send_json_success(array('message' => __('Successfully pushed to GitHub', 'themisdb-wiki')));
    }
    
    /**
     * AJAX: Pull from GitHub
     */
    public function ajax_pull_from_github() {
        check_ajax_referer('themisdb_wiki_nonce', 'nonce');
        
        if (!current_user_can('manage_options')) {
            wp_send_json_error(array('message' => __('Unauthorized', 'themisdb-wiki')));
        }
        
        $github_path = isset($_POST['github_path']) ? sanitize_text_field($_POST['github_path']) : '';
        
        if (empty($github_path)) {
            wp_send_json_error(array('message' => __('GitHub path is required', 'themisdb-wiki')));
        }
        
        $result = $this->pull_from_github($github_path);
        
        if (is_wp_error($result)) {
            wp_send_json_error(array('message' => $result->get_error_message()));
        }
        
        wp_send_json_success(array(
            'message' => __('Successfully pulled from GitHub', 'themisdb-wiki'),
            'post_id' => $result
        ));
    }
    
    /**
     * AJAX: Bulk Sync
     */
    public function ajax_bulk_sync() {
        check_ajax_referer('themisdb_wiki_nonce', 'nonce');
        
        if (!current_user_can('manage_options')) {
            wp_send_json_error(array('message' => __('Unauthorized', 'themisdb-wiki')));
        }
        
        $result = $this->sync_all();
        
        if (is_wp_error($result)) {
            wp_send_json_error(array('message' => $result->get_error_message()));
        }
        
        $message = sprintf(
            __('Synced %d pages. %d errors.', 'themisdb-wiki'),
            $result['synced'],
            count($result['errors'])
        );
        
        wp_send_json_success(array(
            'message' => $message,
            'synced' => $result['synced'],
            'errors' => $result['errors']
        ));
    }
}
