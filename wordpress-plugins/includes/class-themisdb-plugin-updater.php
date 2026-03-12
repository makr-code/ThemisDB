<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-themisdb-plugin-updater.php                  ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:16                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     336                                            ║
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
 * ThemisDB Plugin Updater
 * 
 * Handles automatic updates from GitHub repository for ThemisDB plugins.
 * Integrates with WordPress update system.
 * 
 * @package ThemisDB
 * @version 1.0.0
 * @link https://github.com/makr-code/wordpressPlugins
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Plugin_Updater {
    
    /**
     * GitHub username
     * @var string
     */
    private $username;
    
    /**
     * GitHub repository name
     * @var string
     */
    private $repository;

    /**
     * GitHub repository branch
     * @var string
     */
    private $repository_branch = 'main';

    /**
     * Optional plugin base path inside repository
     * @var string
     */
    private $repository_plugin_path = '';
    
    /**
     * Plugin slug (directory name)
     * @var string
     */
    private $plugin_slug;
    
    /**
     * Plugin file path relative to plugins directory
     * @var string
     */
    private $plugin_file;
    
    /**
     * Current plugin version
     * @var string
     */
    private $version;
    
    /**
     * GitHub API base URL
     * @var string
     */
    private $github_api_url = 'https://api.github.com';
    
    /**
     * Transient cache duration (12 hours)
     * @var int
     */
    private $cache_duration = 43200;
    
    /**
     * Initialize the updater
     * 
     * @param string $plugin_file Main plugin file path
     * @param string $plugin_slug Plugin slug (directory name)
     * @param string $version Current plugin version
     * @param string $username GitHub username (default: makr-code)
     * @param string $repository GitHub repository (default: wordpressPlugins)
     */
    public function __construct($plugin_file, $plugin_slug, $version, $username = 'makr-code', $repository = 'wordpressPlugins') {
        $this->plugin_file = plugin_basename($plugin_file);
        $this->plugin_slug = $plugin_slug;
        $this->version = $version;
        $this->username = apply_filters('themisdb_plugin_updater_repo_owner', $username, $plugin_slug);
        $this->repository = apply_filters('themisdb_plugin_updater_repo_name', $repository, $plugin_slug);
        $this->repository_branch = apply_filters('themisdb_plugin_updater_repo_branch', 'main', $plugin_slug);

        $path = apply_filters('themisdb_plugin_updater_repo_plugin_path', '', $plugin_slug);
        $this->repository_plugin_path = trim((string) $path, '/');
        
        // Hook into WordPress update system
        add_filter('pre_set_site_transient_update_plugins', array($this, 'check_for_update'));
        add_filter('plugins_api', array($this, 'plugin_info'), 10, 3);
        add_filter('upgrader_post_install', array($this, 'after_install'), 10, 3);
        
        // Clear cache when user manually checks for updates
        add_action('admin_init', array($this, 'maybe_clear_cache'));
    }
    
    /**
     * Check for plugin updates
     * 
     * @param object $transient WordPress update transient
     * @return object Modified transient
     */
    public function check_for_update($transient) {
        if (empty($transient->checked)) {
            return $transient;
        }
        
        // Get remote version information
        $remote_version = $this->get_remote_version();
        
        if ($remote_version && version_compare($this->version, $remote_version->version, '<')) {
            $plugin_data = array(
                'slug' => $this->plugin_slug,
                'new_version' => $remote_version->version,
                'url' => $remote_version->homepage,
                'package' => $remote_version->download_url,
                'tested' => $remote_version->tested,
                'requires_php' => $remote_version->requires_php,
                'requires' => $remote_version->requires,
            );
            
            $transient->response[$this->plugin_file] = (object) $plugin_data;
        }
        
        return $transient;
    }
    
    /**
     * Get plugin information for the update details screen
     * 
     * @param false|object|array $result
     * @param string $action
     * @param object $args
     * @return false|object
     */
    public function plugin_info($result, $action, $args) {
        if ($action !== 'plugin_information' || $args->slug !== $this->plugin_slug) {
            return $result;
        }
        
        $remote_version = $this->get_remote_version();
        
        if (!$remote_version) {
            return $result;
        }
        
        $result = (object) array(
            'name' => $remote_version->name,
            'slug' => $this->plugin_slug,
            'version' => $remote_version->version,
            'author' => $remote_version->author,
            'author_profile' => $remote_version->author_profile,
            'requires' => $remote_version->requires,
            'tested' => $remote_version->tested,
            'requires_php' => $remote_version->requires_php,
            'download_link' => $remote_version->download_url,
            'last_updated' => $remote_version->last_updated,
            'sections' => array(
                'description' => $remote_version->description,
                'changelog' => $remote_version->changelog,
            ),
            'banners' => array(),
            'icons' => array(),
        );
        
        return $result;
    }
    
    /**
     * Perform additional actions after plugin installation
     * 
     * @param bool $response Installation response
     * @param array $hook_extra Extra arguments
     * @param array $result Installation result
     * @return array
     */
    public function after_install($response, $hook_extra, $result) {
        global $wp_filesystem;
        
        $install_directory = plugin_dir_path($result['destination']);
        $wp_filesystem->move($result['destination'], $install_directory . $this->plugin_slug);
        $result['destination'] = $install_directory . $this->plugin_slug;
        
        return $result;
    }
    
    /**
     * Get remote version information from GitHub
     * 
     * @return object|false Version information or false on failure
     */
    private function get_remote_version() {
        $cache_key = 'themisdb_update_' . $this->plugin_slug;
        $remote_version = get_transient($cache_key);
        
        if ($remote_version !== false) {
            return $remote_version;
        }
        
        // Fetch plugin metadata from GitHub
        $metadata = $this->fetch_plugin_metadata();
        
        if (!$metadata) {
            return false;
        }
        
        // Resolve the best matching release for this specific plugin.
        $release = $this->fetch_release_for_plugin();
        
        if (!$release) {
            return false;
        }
        
        $remote_version = (object) array(
            'version' => isset($metadata['version']) ? $metadata['version'] : $this->extract_version_from_tag($release->tag_name),
            'name' => isset($metadata['name']) ? $metadata['name'] : $this->plugin_slug,
            'slug' => $this->plugin_slug,
            'homepage' => isset($metadata['homepage']) ? $metadata['homepage'] : "https://github.com/{$this->username}/{$this->repository}",
            'description' => isset($metadata['description']) ? $metadata['description'] : '',
            'author' => isset($metadata['author']) ? $metadata['author'] : 'ThemisDB Team',
            'author_profile' => isset($metadata['author_uri']) ? $metadata['author_uri'] : "https://github.com/{$this->username}",
            'requires' => isset($metadata['requires']) ? $metadata['requires'] : '5.8',
            'tested' => isset($metadata['tested']) ? $metadata['tested'] : '6.4',
            'requires_php' => isset($metadata['requires_php']) ? $metadata['requires_php'] : '7.4',
            'download_url' => $this->get_download_url($release),
            'last_updated' => $release->published_at,
            'changelog' => isset($release->body) ? $release->body : '',
        );
        
        // Cache the result
        set_transient($cache_key, $remote_version, $this->cache_duration);
        
        return $remote_version;
    }
    
    /**
     * Fetch plugin metadata from GitHub repository
     * 
     * @return array|false Plugin metadata or false on failure
     */
    private function fetch_plugin_metadata() {
        $paths = array();

        if (!empty($this->repository_plugin_path)) {
            $paths[] = $this->repository_plugin_path;
        }

        // Default path for the new dedicated plugin repository.
        $paths[] = '';

        // Backward-compatibility fallbacks during migration.
        $paths[] = 'wordpress-plugins';
        $paths[] = 'wordpress-plugin';

        $paths = array_values(array_unique($paths));

        foreach ($paths as $path) {
            $prefix = empty($path) ? '' : $path . '/';
            $metadata_url = "https://raw.githubusercontent.com/{$this->username}/{$this->repository}/{$this->repository_branch}/{$prefix}{$this->plugin_slug}/update-info.json";

            $response = wp_remote_get($metadata_url, array(
                'timeout' => 10,
                'headers' => array(
                    'Accept' => 'application/json',
                ),
            ));

            if (is_wp_error($response)) {
                continue;
            }

            $status = wp_remote_retrieve_response_code($response);
            if ($status !== 200) {
                continue;
            }

            $body = wp_remote_retrieve_body($response);
            $metadata = json_decode($body, true);

            if (is_array($metadata)) {
                return $metadata;
            }
        }

        return false;
    }
    
    /**
     * Fetch latest release from GitHub API
     * 
     * @return object|false Release information or false on failure
     */
    private function fetch_latest_release() {
        $api_url = "{$this->github_api_url}/repos/{$this->username}/{$this->repository}/releases/latest";
        
        $response = wp_remote_get($api_url, array(
            'timeout' => 10,
            'headers' => array(
                'Accept' => 'application/vnd.github.v3+json',
            ),
        ));
        
        if (is_wp_error($response)) {
            return false;
        }
        
        $body = wp_remote_retrieve_body($response);
        $release = json_decode($body);
        
        return (isset($release->tag_name)) ? $release : false;
    }

    /**
     * Fetch the best matching release for the current plugin.
     *
     * @return object|false Release information or false on failure
     */
    private function fetch_release_for_plugin() {
        $api_url = "{$this->github_api_url}/repos/{$this->username}/{$this->repository}/releases?per_page=30";

        $response = wp_remote_get($api_url, array(
            'timeout' => 10,
            'headers' => array(
                'Accept' => 'application/vnd.github.v3+json',
            ),
        ));

        if (is_wp_error($response)) {
            return $this->fetch_latest_release();
        }

        $body = wp_remote_retrieve_body($response);
        $releases = json_decode($body);

        if (!is_array($releases)) {
            return $this->fetch_latest_release();
        }

        foreach ($releases as $release) {
            if (!isset($release->tag_name) || !empty($release->draft)) {
                continue;
            }

            if ($this->release_has_plugin_asset($release) || $this->tag_matches_plugin($release->tag_name)) {
                return $release;
            }
        }

        return $this->fetch_latest_release();
    }

    /**
     * Check whether a release has a zip asset for this plugin.
     *
     * @param object $release Release object
     * @return bool
     */
    private function release_has_plugin_asset($release) {
        if (!isset($release->assets) || !is_array($release->assets)) {
            return false;
        }

        $expected = strtolower($this->plugin_slug . '.zip');

        foreach ($release->assets as $asset) {
            if (!isset($asset->name)) {
                continue;
            }

            if (strtolower($asset->name) === $expected) {
                return true;
            }
        }

        return false;
    }

    /**
     * Check whether a tag appears to belong to this plugin.
     *
     * Supported examples:
     * - themisdb-downloads/v1.2.0
     * - themisdb-downloads-v1.2.0
     *
     * @param string $tag_name Tag name
     * @return bool
     */
    private function tag_matches_plugin($tag_name) {
        $tag = strtolower((string) $tag_name);
        $slug = strtolower($this->plugin_slug);

        return (strpos($tag, $slug . '/v') === 0) ||
               (strpos($tag, $slug . '-v') === 0) ||
               (strpos($tag, $slug . '_v') === 0);
    }

    /**
     * Extract a plugin version from release tag names.
     *
     * @param string $tag_name Tag name
     * @return string Normalized semantic version if possible
     */
    private function extract_version_from_tag($tag_name) {
        $tag = (string) $tag_name;

        $prefix_patterns = array(
            '/^' . preg_quote($this->plugin_slug, '/') . '\/v/i',
            '/^' . preg_quote($this->plugin_slug, '/') . '-v/i',
            '/^' . preg_quote($this->plugin_slug, '/') . '_v/i',
            '/^v/i',
        );

        foreach ($prefix_patterns as $pattern) {
            $normalized = preg_replace($pattern, '', $tag);
            if ($normalized !== $tag) {
                return $normalized;
            }
        }

        return $tag;
    }
    
    /**
     * Get download URL for the plugin
     * 
     * @param object $release Release information
     * @return string Download URL
     */
    private function get_download_url($release) {
        // Look for plugin-specific asset
        if (isset($release->assets) && is_array($release->assets)) {
            $exact_asset_name = $this->plugin_slug . '.zip';

            foreach ($release->assets as $asset) {
                if (isset($asset->name) && isset($asset->browser_download_url) &&
                    strtolower($asset->name) === strtolower($exact_asset_name)) {
                    return $asset->browser_download_url;
                }
            }

            foreach ($release->assets as $asset) {
                if (strpos($asset->name, $this->plugin_slug) !== false && 
                    (strpos($asset->name, '.zip') !== false)) {
                    return $asset->browser_download_url;
                }
            }
        }
        
        // Fallback to zipball URL
        return "https://github.com/{$this->username}/{$this->repository}/releases/download/{$release->tag_name}/{$this->plugin_slug}.zip";
    }
    
    /**
     * Clear update cache when WordPress forces an update check
     * This is triggered when users click "Check Again" on the Updates page
     */
    public function maybe_clear_cache() {
        global $pagenow;
        
        // Clear cache when on update-core.php page and update check is triggered
        if ($pagenow === 'update-core.php' && 
            isset($_GET['force-check']) && 
            $_GET['force-check'] == '1' &&
            current_user_can('update_plugins') &&
            check_admin_referer('update-core')) {
            
            $cache_key = 'themisdb_update_' . $this->plugin_slug;
            delete_transient($cache_key);
        }
    }
}
