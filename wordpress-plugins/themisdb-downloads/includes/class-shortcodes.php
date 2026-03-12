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
    • Total Lines:     538                                            ║
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
 * Shortcodes Handler
 * Handles shortcode rendering for displaying downloads
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Downloads_Shortcodes {
    
    private $api;
    
    public function __construct() {
        $this->api = new ThemisDB_Downloads_GitHub_API();
        
        // Register shortcodes
        add_shortcode('themisdb_downloads', array($this, 'render_downloads'));
        add_shortcode('themisdb_latest', array($this, 'render_latest'));
        add_shortcode('themisdb_verify', array($this, 'render_verify_tool'));
        add_shortcode('themisdb_readme', array($this, 'render_readme'));
        add_shortcode('themisdb_changelog', array($this, 'render_changelog'));
    }
    
    /**
     * Render downloads shortcode
     * 
     * @param array $atts Shortcode attributes
     * @return string HTML output
     */
    public function render_downloads($atts) {
        $atts = shortcode_atts(array(
            'show' => 'latest', // 'latest' or 'all'
            'platform' => '', // Filter by platform (windows, linux, docker)
            'style' => 'default', // 'default', 'compact', 'table'
            'limit' => 10 // Number of releases to show
        ), $atts);
        
        // Get releases
        if ($atts['show'] === 'all') {
            $releases = $this->api->get_all_releases(intval($atts['limit']));
        } else {
            $latest = $this->api->get_latest_release();
            $releases = is_wp_error($latest) ? array() : array($latest);
        }
        
        if (is_wp_error($releases)) {
            return $this->render_error($releases);
        }
        
        if (empty($releases)) {
            return '<div class="themisdb-downloads-notice">Keine Releases gefunden.</div>';
        }
        
        // Filter by platform if specified
        if (!empty($atts['platform'])) {
            $releases = $this->filter_by_platform($releases, $atts['platform']);
        }
        
        // Render based on style
        switch ($atts['style']) {
            case 'compact':
                return $this->render_compact($releases);
            case 'table':
                return $this->render_table($releases);
            default:
                return $this->render_default($releases);
        }
    }
    
    /**
     * Render latest release shortcode
     */
    public function render_latest($atts) {
        $atts = shortcode_atts(array(
            'show' => 'version' // 'version', 'date', 'link'
        ), $atts);
        
        $latest = $this->api->get_latest_release();
        
        if (is_wp_error($latest)) {
            return '';
        }
        
        switch ($atts['show']) {
            case 'version':
                return '<span class="themisdb-version">' . esc_html($latest['version']) . '</span>';
            case 'date':
                return '<span class="themisdb-date">' . date_i18n(get_option('date_format'), strtotime($latest['published_at'])) . '</span>';
            case 'link':
                return '<a href="' . esc_url($latest['html_url']) . '" class="themisdb-link" target="_blank">Neueste Version: ' . esc_html($latest['version']) . '</a>';
            default:
                return esc_html($latest['version']);
        }
    }
    
    /**
     * Render verification tool shortcode
     */
    public function render_verify_tool($atts) {
        ob_start();
        ?>
        <div class="themisdb-verify-tool">
            <h3>Download-Verifizierung</h3>
            <p>Überprüfen Sie die Integrität Ihrer heruntergeladenen Datei mit dem SHA256-Checksum:</p>
            
            <div class="verify-input-group">
                <label for="themisdb-file-upload">Datei auswählen:</label>
                <input type="file" id="themisdb-file-upload" class="themisdb-file-input">
            </div>
            
            <div class="verify-input-group">
                <label for="themisdb-expected-hash">Erwarteter SHA256-Hash:</label>
                <input type="text" id="themisdb-expected-hash" class="themisdb-hash-input" placeholder="Checksum aus der Download-Liste kopieren">
            </div>
            
            <button type="button" id="themisdb-verify-button" class="button button-primary">Verifizieren</button>
            
            <div id="themisdb-verify-result" class="verify-result"></div>
            
            <div class="verify-instructions">
                <h4>Manuelle Verifizierung (Kommandozeile):</h4>
                <p><strong>Windows (PowerShell):</strong></p>
                <code>Get-FileHash -Algorithm SHA256 themis-*.zip | Format-List</code>
                
                <p><strong>Linux/macOS:</strong></p>
                <code>sha256sum themis-*.tar.gz</code>
                
                <p><strong>Vergleichen Sie den berechneten Hash mit dem angezeigten SHA256-Checksum.</strong></p>
            </div>
        </div>
        <?php
        return ob_get_clean();
    }
    
    /**
     * Render default style
     */
    private function render_default($releases) {
        ob_start();
        ?>
        <div class="themisdb-downloads-container">
            <?php foreach ($releases as $release): ?>
                <div class="themisdb-release">
                    <div class="release-header">
                        <h3 class="release-version">
                            <?php echo esc_html(!empty($release['name']) ? $release['name'] : $release['version']); ?>
                            <span class="release-tag"><?php echo esc_html($release['version']); ?></span>
                        </h3>
                        <p class="release-date">
                            Veröffentlicht: <?php echo date_i18n(get_option('date_format'), strtotime($release['published_at'])); ?>
                        </p>
                    </div>
                    
                    <?php if (!empty($release['body'])): ?>
                        <div class="release-notes">
                            <?php echo wp_kses_post(wpautop($release['body'])); ?>
                        </div>
                    <?php endif; ?>
                    
                    <div class="release-downloads">
                        <h4>Downloads:</h4>
                        <?php if (!empty($release['assets'])): ?>
                            <div class="downloads-grid">
                                <?php foreach ($release['assets'] as $asset): ?>
                                    <?php 
                                    // Skip SHA256SUMS files from asset list
                                    if (strpos($asset['name'], 'SHA256') !== false) {
                                        continue;
                                    }
                                    
                                    $platform = $this->detect_platform($asset['name']);
                                    $icon = $this->get_platform_icon($platform);
                                    $sha256 = isset($release['sha256sums'][$asset['name']]) ? $release['sha256sums'][$asset['name']] : '';
                                    ?>
                                    <div class="download-item" data-platform="<?php echo esc_attr($platform); ?>">
                                        <div class="download-icon"><?php echo $icon; ?></div>
                                        <div class="download-info">
                                            <a href="<?php echo esc_url($asset['download_url']); ?>" 
                                               class="download-link" 
                                               target="_blank">
                                                <?php echo esc_html($asset['name']); ?>
                                            </a>
                                            <div class="download-meta">
                                                <span class="file-size"><?php echo size_format($asset['size']); ?></span>
                                                <span class="download-count">↓ <?php echo number_format_i18n($asset['download_count']); ?></span>
                                            </div>
                                            <?php if ($sha256): ?>
                                                <div class="download-checksum">
                                                    <strong>SHA256:</strong>
                                                    <code class="sha256-hash" data-hash="<?php echo esc_attr($sha256); ?>">
                                                        <?php echo esc_html($sha256); ?>
                                                    </code>
                                                    <button type="button" 
                                                            class="copy-hash-button" 
                                                            data-hash="<?php echo esc_attr($sha256); ?>"
                                                            title="Hash kopieren">
                                                        📋
                                                    </button>
                                                </div>
                                            <?php endif; ?>
                                        </div>
                                    </div>
                                <?php endforeach; ?>
                            </div>
                        <?php else: ?>
                            <p>Keine Download-Dateien verfügbar.</p>
                        <?php endif; ?>
                    </div>
                    
                    <?php if (!empty($release['sha256sums'])): ?>
                        <details class="release-checksums">
                            <summary>Alle SHA256 Checksums anzeigen</summary>
                            <div class="checksums-list">
                                <?php foreach ($release['sha256sums'] as $filename => $hash): ?>
                                    <div class="checksum-item">
                                        <span class="checksum-filename"><?php echo esc_html($filename); ?></span>
                                        <code class="checksum-hash"><?php echo esc_html($hash); ?></code>
                                    </div>
                                <?php endforeach; ?>
                            </div>
                        </details>
                    <?php endif; ?>
                </div>
            <?php endforeach; ?>
        </div>
        <?php
        return ob_get_clean();
    }
    
    /**
     * Render compact style
     */
    private function render_compact($releases) {
        ob_start();
        ?>
        <div class="themisdb-downloads-compact">
            <?php foreach ($releases as $release): ?>
                <div class="release-compact">
                    <strong><?php echo esc_html($release['version']); ?></strong>
                    <span class="release-date-compact">
                        (<?php echo date_i18n(get_option('date_format'), strtotime($release['published_at'])); ?>)
                    </span>
                    <?php if (!empty($release['assets'])): ?>
                        <div class="downloads-compact">
                            <?php foreach ($release['assets'] as $asset): ?>
                                <?php if (strpos($asset['name'], 'SHA256') === false): ?>
                                    <a href="<?php echo esc_url($asset['download_url']); ?>" 
                                       class="download-link-compact"
                                       target="_blank">
                                        <?php echo esc_html($asset['name']); ?>
                                    </a>
                                <?php endif; ?>
                            <?php endforeach; ?>
                        </div>
                    <?php endif; ?>
                </div>
            <?php endforeach; ?>
        </div>
        <?php
        return ob_get_clean();
    }
    
    /**
     * Render table style
     */
    private function render_table($releases) {
        ob_start();
        ?>
        <div class="themisdb-downloads-table">
            <table class="wp-list-table widefat">
                <thead>
                    <tr>
                        <th>Version</th>
                        <th>Datum</th>
                        <th>Datei</th>
                        <th>Größe</th>
                        <th>SHA256</th>
                        <th>Download</th>
                    </tr>
                </thead>
                <tbody>
                    <?php foreach ($releases as $release): ?>
                        <?php foreach ($release['assets'] as $asset): ?>
                            <?php if (strpos($asset['name'], 'SHA256') !== false) continue; ?>
                            <?php $sha256 = isset($release['sha256sums'][$asset['name']]) ? $release['sha256sums'][$asset['name']] : ''; ?>
                            <tr>
                                <td><?php echo esc_html($release['version']); ?></td>
                                <td><?php echo date_i18n('Y-m-d', strtotime($release['published_at'])); ?></td>
                                <td><?php echo esc_html($asset['name']); ?></td>
                                <td><?php echo size_format($asset['size']); ?></td>
                                <td>
                                    <?php if ($sha256): ?>
                                        <code title="<?php echo esc_attr($sha256); ?>">
                                            <?php echo substr($sha256, 0, 12); ?>...
                                        </code>
                                    <?php endif; ?>
                                </td>
                                <td>
                                    <a href="<?php echo esc_url($asset['download_url']); ?>" 
                                       class="button button-small"
                                       target="_blank">
                                        Download
                                    </a>
                                </td>
                            </tr>
                        <?php endforeach; ?>
                    <?php endforeach; ?>
                </tbody>
            </table>
        </div>
        <?php
        return ob_get_clean();
    }
    
    /**
     * Filter releases by platform
     */
    private function filter_by_platform($releases, $platform) {
        $filtered = array();
        
        foreach ($releases as $release) {
            $filtered_assets = array();
            foreach ($release['assets'] as $asset) {
                if ($this->detect_platform($asset['name']) === $platform) {
                    $filtered_assets[] = $asset;
                }
            }
            
            if (!empty($filtered_assets)) {
                $release['assets'] = $filtered_assets;
                $filtered[] = $release;
            }
        }
        
        return $filtered;
    }
    
    /**
     * Detect platform from filename
     */
    private function detect_platform($filename) {
        $filename_lower = strtolower($filename);
        
        if (strpos($filename_lower, 'windows') !== false || strpos($filename_lower, '.exe') !== false || strpos($filename_lower, 'win') !== false) {
            return 'windows';
        } elseif (strpos($filename_lower, 'linux') !== false || strpos($filename_lower, '.deb') !== false || strpos($filename_lower, '.rpm') !== false) {
            return 'linux';
        } elseif (strpos($filename_lower, 'docker') !== false) {
            return 'docker';
        } elseif (strpos($filename_lower, 'qnap') !== false) {
            return 'qnap';
        } elseif (strpos($filename_lower, 'arm') !== false) {
            return 'arm';
        } elseif (strpos($filename_lower, 'macos') !== false || strpos($filename_lower, 'darwin') !== false) {
            return 'macos';
        }
        
        return 'other';
    }
    
    /**
     * Get platform icon
     */
    private function get_platform_icon($platform) {
        $icons = array(
            'windows' => '🪟',
            'linux' => '🐧',
            'docker' => '🐳',
            'qnap' => '💾',
            'arm' => '📱',
            'macos' => '🍎',
            'other' => '📦'
        );
        
        return isset($icons[$platform]) ? $icons[$platform] : $icons['other'];
    }
    
    /**
     * Render README shortcode
     * 
     * @param array $atts Shortcode attributes
     * @return string HTML output
     */
    public function render_readme($atts) {
        $atts = shortcode_atts(array(
            'version' => 'latest', // 'latest' or specific version tag
            'style' => 'default' // 'default' or 'raw'
        ), $atts);
        
        // Get release
        if ($atts['version'] === 'latest') {
            $release = $this->api->get_latest_release();
        } else {
            // For specific version, get all releases and find matching one
            $all_releases = $this->api->get_all_releases(50);
            $release = null;
            if (!is_wp_error($all_releases)) {
                foreach ($all_releases as $r) {
                    if ($r['version'] === $atts['version'] || $r['version'] === 'v' . $atts['version']) {
                        $release = $r;
                        break;
                    }
                }
            }
        }
        
        if (is_wp_error($release) || empty($release)) {
            return '<div class="themisdb-downloads-notice">README nicht verfügbar.</div>';
        }
        
        if (empty($release['readme'])) {
            return '<div class="themisdb-downloads-notice">Kein README für diese Version gefunden.</div>';
        }
        
        // Render README
        ob_start();
        ?>
        <div class="themisdb-readme-container">
            <div class="readme-header">
                <h3>README - <?php echo esc_html($release['version']); ?></h3>
            </div>
            <div class="readme-content">
                <?php
                if ($atts['style'] === 'raw') {
                    echo '<pre>' . esc_html($release['readme']) . '</pre>';
                } else {
                    // Parse markdown to HTML (basic conversion)
                    echo wp_kses_post($this->markdown_to_html($release['readme']));
                }
                ?>
            </div>
        </div>
        <?php
        return ob_get_clean();
    }
    
    /**
     * Render CHANGELOG shortcode
     * 
     * @param array $atts Shortcode attributes
     * @return string HTML output
     */
    public function render_changelog($atts) {
        $atts = shortcode_atts(array(
            'version' => 'latest', // 'latest' or specific version tag
            'style' => 'default' // 'default' or 'raw'
        ), $atts);
        
        // Get release
        if ($atts['version'] === 'latest') {
            $release = $this->api->get_latest_release();
        } else {
            // For specific version, get all releases and find matching one
            $all_releases = $this->api->get_all_releases(50);
            $release = null;
            if (!is_wp_error($all_releases)) {
                foreach ($all_releases as $r) {
                    if ($r['version'] === $atts['version'] || $r['version'] === 'v' . $atts['version']) {
                        $release = $r;
                        break;
                    }
                }
            }
        }
        
        if (is_wp_error($release) || empty($release)) {
            return '<div class="themisdb-downloads-notice">CHANGELOG nicht verfügbar.</div>';
        }
        
        if (empty($release['changelog'])) {
            return '<div class="themisdb-downloads-notice">Kein CHANGELOG für diese Version gefunden.</div>';
        }
        
        // Render CHANGELOG
        ob_start();
        ?>
        <div class="themisdb-changelog-container">
            <div class="changelog-header">
                <h3>CHANGELOG - <?php echo esc_html($release['version']); ?></h3>
            </div>
            <div class="changelog-content">
                <?php
                if ($atts['style'] === 'raw') {
                    echo '<pre>' . esc_html($release['changelog']) . '</pre>';
                } else {
                    // Parse markdown to HTML (basic conversion)
                    echo wp_kses_post($this->markdown_to_html($release['changelog']));
                }
                ?>
            </div>
        </div>
        <?php
        return ob_get_clean();
    }
    
    /**
     * Convert markdown to HTML using shared converter
     * 
     * @param string $markdown Markdown text
     * @return string HTML
     */
    private function markdown_to_html($markdown) {
        return ThemisDB_Markdown_Converter::convert($markdown);
    }
    
    /**
     * Render error message
     */
    private function render_error($error) {
        return '<div class="themisdb-downloads-error">
            <p><strong>Fehler beim Laden der Releases:</strong> ' . esc_html($error->get_error_message()) . '</p>
        </div>';
    }
}
