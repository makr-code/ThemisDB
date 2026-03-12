<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-shortcodes.php                               ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:17                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     331                                            ║
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
 * Shortcode Handler
 * Handles all shortcodes for displaying Docker images
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Docker_Downloads_Shortcodes {
    
    public function __construct() {
        add_shortcode('themisdb_docker_tags', array($this, 'render_docker_tags'));
        add_shortcode('themisdb_docker_latest', array($this, 'render_latest_tag'));
    }
    
    /**
     * Render Docker tags shortcode
     * 
     * @param array $atts Shortcode attributes
     * @return string HTML output
     */
    public function render_docker_tags($atts) {
        $atts = shortcode_atts(array(
            'show' => 'latest',      // 'latest' or 'all'
            'limit' => 10,           // Number of tags to show
            'style' => 'default',    // 'default', 'compact', 'table'
            'architecture' => '',    // Filter by architecture (amd64, arm64, etc.)
        ), $atts);
        
        $api = new ThemisDB_Docker_Downloads_DockerHub_API();
        
        if ($atts['show'] === 'latest') {
            $tag = $api->get_latest_tag();
            if (is_wp_error($tag)) {
                return $this->render_error($tag);
            }
            $tags = array($tag);
        } else {
            $tags = $api->get_tags($atts['limit']);
            if (is_wp_error($tags)) {
                return $this->render_error($tags);
            }
        }
        
        // Filter by architecture if specified
        if (!empty($atts['architecture'])) {
            $tags = $this->filter_by_architecture($tags, $atts['architecture']);
        }
        
        // Render based on style
        switch ($atts['style']) {
            case 'compact':
                return $this->render_compact_style($tags);
            case 'table':
                return $this->render_table_style($tags);
            default:
                return $this->render_default_style($tags);
        }
    }
    
    /**
     * Render latest tag shortcode (just the tag name)
     * 
     * @return string Tag name or error
     */
    public function render_latest_tag() {
        $api = new ThemisDB_Docker_Downloads_DockerHub_API();
        $tag = $api->get_latest_tag();
        
        if (is_wp_error($tag)) {
            return 'Error: ' . esc_html($tag->get_error_message());
        }
        
        return esc_html($tag['name']);
    }
    
    /**
     * Filter tags by architecture
     * 
     * @param array $tags Tags array
     * @param string $architecture Architecture to filter by
     * @return array Filtered tags
     */
    private function filter_by_architecture($tags, $architecture) {
        $filtered = array();
        
        foreach ($tags as $tag) {
            foreach ($tag['images'] as $image) {
                if ($image['architecture'] === $architecture) {
                    $filtered[] = $tag;
                    break;
                }
            }
        }
        
        return $filtered;
    }
    
    /**
     * Render default style
     * 
     * @param array $tags Tags array
     * @return string HTML output
     */
    private function render_default_style($tags) {
        ob_start();
        ?>
        <div class="themisdb-docker-downloads-container">
            <?php foreach ($tags as $tag): ?>
                <div class="themisdb-docker-tag">
                    <div class="tag-header">
                        <h3 class="tag-name">
                            🐳 <?php echo esc_html($tag['name']); ?>
                        </h3>
                        <span class="tag-date">
                            <?php echo esc_html(human_time_diff(strtotime($tag['last_updated']), current_time('timestamp'))); ?> ago
                        </span>
                    </div>
                    
                    <div class="tag-info">
                        <div class="pull-command">
                            <strong>Pull Command:</strong>
                            <code class="docker-command"><?php echo esc_html($tag['pull_command']); ?></code>
                            <button class="copy-command-btn" data-command="<?php echo esc_attr($tag['pull_command']); ?>">
                                📋 Copy
                            </button>
                        </div>
                        
                        <?php if (!empty($tag['images'])): ?>
                            <div class="tag-images">
                                <strong>Available Architectures:</strong>
                                <ul class="architecture-list">
                                    <?php foreach ($tag['images'] as $image): ?>
                                        <li class="architecture-item">
                                            <span class="arch-platform">
                                                <?php 
                                                $platform_icon = $this->get_platform_icon($image['architecture'], $image['os']);
                                                echo esc_html($platform_icon . ' ' . $image['architecture']);
                                                if (!empty($image['variant'])) {
                                                    echo '/' . esc_html($image['variant']);
                                                }
                                                ?>
                                            </span>
                                            <?php if (!empty($image['digest'])): ?>
                                                <div class="image-digest">
                                                    <strong>Digest:</strong>
                                                    <code class="digest-value" title="<?php echo esc_attr($image['digest']); ?>">
                                                        <?php echo esc_html(substr($image['digest'], 0, 19) . '...'); ?>
                                                    </code>
                                                    <button class="copy-digest-btn" data-digest="<?php echo esc_attr($image['digest']); ?>">
                                                        📋
                                                    </button>
                                                </div>
                                            <?php endif; ?>
                                            <?php if ($image['size'] > 0): ?>
                                                <span class="image-size">
                                                    Size: <?php echo esc_html($this->format_bytes($image['size'])); ?>
                                                </span>
                                            <?php endif; ?>
                                        </li>
                                    <?php endforeach; ?>
                                </ul>
                            </div>
                        <?php endif; ?>
                        
                        <div class="tag-links">
                            <a href="<?php echo esc_url($tag['tag_url']); ?>" target="_blank" class="docker-hub-link">
                                View on Docker Hub →
                            </a>
                        </div>
                    </div>
                </div>
            <?php endforeach; ?>
        </div>
        <?php
        return ob_get_clean();
    }
    
    /**
     * Render compact style
     * 
     * @param array $tags Tags array
     * @return string HTML output
     */
    private function render_compact_style($tags) {
        ob_start();
        ?>
        <div class="themisdb-docker-downloads-compact">
            <ul class="docker-tags-list">
                <?php foreach ($tags as $tag): ?>
                    <li class="docker-tag-item">
                        <span class="tag-name">🐳 <?php echo esc_html($tag['name']); ?></span>
                        <code class="pull-command"><?php echo esc_html($tag['pull_command']); ?></code>
                        <button class="copy-btn" data-command="<?php echo esc_attr($tag['pull_command']); ?>">
                            📋
                        </button>
                    </li>
                <?php endforeach; ?>
            </ul>
        </div>
        <?php
        return ob_get_clean();
    }
    
    /**
     * Render table style
     * 
     * @param array $tags Tags array
     * @return string HTML output
     */
    private function render_table_style($tags) {
        ob_start();
        ?>
        <div class="themisdb-docker-downloads-table">
            <table class="docker-tags-table">
                <thead>
                    <tr>
                        <th>Tag</th>
                        <th>Architectures</th>
                        <th>Last Updated</th>
                        <th>Pull Command</th>
                        <th>Actions</th>
                    </tr>
                </thead>
                <tbody>
                    <?php foreach ($tags as $tag): ?>
                        <tr>
                            <td class="tag-name">🐳 <?php echo esc_html($tag['name']); ?></td>
                            <td class="architectures">
                                <?php
                                $archs = array_map(function($img) {
                                    return $img['architecture'];
                                }, $tag['images']);
                                echo esc_html(implode(', ', array_unique($archs)));
                                ?>
                            </td>
                            <td class="last-updated">
                                <?php echo esc_html(human_time_diff(strtotime($tag['last_updated']), current_time('timestamp'))); ?> ago
                            </td>
                            <td class="pull-command">
                                <code><?php echo esc_html($tag['pull_command']); ?></code>
                            </td>
                            <td class="actions">
                                <button class="copy-btn" data-command="<?php echo esc_attr($tag['pull_command']); ?>">
                                    📋 Copy
                                </button>
                                <a href="<?php echo esc_url($tag['tag_url']); ?>" target="_blank" class="hub-link">
                                    View
                                </a>
                            </td>
                        </tr>
                    <?php endforeach; ?>
                </tbody>
            </table>
        </div>
        <?php
        return ob_get_clean();
    }
    
    /**
     * Render error message
     * 
     * @param WP_Error $error Error object
     * @return string HTML output
     */
    private function render_error($error) {
        return '<div class="themisdb-docker-error">' . 
               '<strong>Error:</strong> ' . esc_html($error->get_error_message()) . 
               '</div>';
    }
    
    /**
     * Get platform icon based on architecture
     * 
     * @param string $architecture Architecture name
     * @param string $os Operating system
     * @return string Icon
     */
    private function get_platform_icon($architecture, $os) {
        $icons = array(
            'amd64' => '💻',
            'arm64' => '📱',
            'arm' => '📱',
            'i386' => '🖥️',
            'ppc64le' => '⚙️',
            's390x' => '🔧'
        );
        
        return isset($icons[$architecture]) ? $icons[$architecture] : '🐳';
    }
    
    /**
     * Format bytes to human-readable format
     * 
     * @param int $bytes Bytes
     * @return string Formatted string
     */
    private function format_bytes($bytes) {
        $units = array('B', 'KB', 'MB', 'GB', 'TB');
        
        for ($i = 0; $bytes > 1024 && $i < count($units) - 1; $i++) {
            $bytes /= 1024;
        }
        
        return round($bytes, 2) . ' ' . $units[$i];
    }
}
