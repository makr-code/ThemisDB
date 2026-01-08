<?php
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
     * Enhanced markdown to HTML conversion with Mermaid support
     * 
     * @param string $markdown Markdown text
     * @return string HTML
     */
    private function markdown_to_html($markdown) {
        // Escape HTML entities first for security
        $html = htmlspecialchars($markdown, ENT_QUOTES, 'UTF-8');
        
        // Process in the correct order to avoid conflicts
        
        // 1. Extract and protect code blocks (including Mermaid) before other processing
        $code_blocks = array();
        $html = preg_replace_callback('/```([a-z]*)\n(.*?)\n```/s', function($matches) use (&$code_blocks) {
            $language = trim($matches[1]);
            $code = $matches[2];
            $placeholder = '___CODE_BLOCK_' . count($code_blocks) . '___';
            
            // Check if it's a Mermaid diagram
            if ($language === 'mermaid') {
                $code_blocks[$placeholder] = '<div class="mermaid">' . "\n" . $code . "\n" . '</div>';
            } else {
                // Regular code block with optional language class
                $lang_class = $language ? ' class="language-' . esc_attr($language) . '"' : '';
                $code_blocks[$placeholder] = '<pre><code' . $lang_class . '>' . $code . '</code></pre>';
            }
            
            return $placeholder;
        }, $html);
        
        // 2. Extract and protect inline code
        $inline_codes = array();
        $html = preg_replace_callback('/`([^`]+)`/', function($matches) use (&$inline_codes) {
            $placeholder = '___INLINE_CODE_' . count($inline_codes) . '___';
            $inline_codes[$placeholder] = '<code>' . $matches[1] . '</code>';
            return $placeholder;
        }, $html);
        
        // 3. Horizontal rules (--- or ***)
        $html = preg_replace('/^(\*{3,}|-{3,})$/m', '<hr>', $html);
        
        // 4. Headers (must process from h6 to h1 to avoid conflicts)
        $html = preg_replace('/^###### (.+)$/m', '<h6>$1</h6>', $html);
        $html = preg_replace('/^##### (.+)$/m', '<h5>$1</h5>', $html);
        $html = preg_replace('/^#### (.+)$/m', '<h4>$1</h4>', $html);
        $html = preg_replace('/^### (.+)$/m', '<h3>$1</h3>', $html);
        $html = preg_replace('/^## (.+)$/m', '<h2>$1</h2>', $html);
        $html = preg_replace('/^# (.+)$/m', '<h1>$1</h1>', $html);
        
        // 5. Bold, italic, strikethrough (process in correct order)
        // Bold and italic combined (***text*** or ___text___)
        $html = preg_replace('/\*\*\*(.+?)\*\*\*/', '<strong><em>$1</em></strong>', $html);
        $html = preg_replace('/___(.+?)___/', '<strong><em>$1</em></strong>', $html);
        // Bold (**text** or __text__)
        $html = preg_replace('/\*\*(.+?)\*\*/', '<strong>$1</strong>', $html);
        $html = preg_replace('/__(.+?)__/', '<strong>$1</strong>', $html);
        // Italic (*text* or _text_)
        $html = preg_replace('/\*(.+?)\*/', '<em>$1</em>', $html);
        $html = preg_replace('/_([^_\s](?:.*?[^_\s])?)_/', '<em>$1</em>', $html);
        // Strikethrough (~~text~~)
        $html = preg_replace('/~~(.+?)~~/', '<del>$1</del>', $html);
        
        // 6. Links and images
        // Images: ![alt](url "title")
        $html = preg_replace_callback('/!\[([^\]]*)\]\(([^\s\)]+)(?:\s+"([^"]*)")?\)/', function($matches) {
            $alt = esc_attr($matches[1]);
            $url = esc_url($matches[2]);
            $title = isset($matches[3]) ? ' title="' . esc_attr($matches[3]) . '"' : '';
            return '<img src="' . $url . '" alt="' . $alt . '"' . $title . ' class="markdown-image">';
        }, $html);
        
        // Links: [text](url "title")
        $html = preg_replace_callback('/\[([^\]]+)\]\(([^\s\)]+)(?:\s+"([^"]*)")?\)/', function($matches) {
            $text = $matches[1];
            $url = esc_url($matches[2]);
            $title = isset($matches[3]) ? ' title="' . esc_attr($matches[3]) . '"' : '';
            return '<a href="' . $url . '"' . $title . ' target="_blank" rel="noopener noreferrer">' . $text . '</a>';
        }, $html);
        
        // 7. Tables
        $html = $this->parse_tables($html);
        
        // 8. Lists (must handle nested lists properly)
        $html = $this->parse_lists($html);
        
        // 9. Blockquotes
        $html = $this->parse_blockquotes($html);
        
        // 10. Restore code blocks
        foreach ($code_blocks as $placeholder => $code_html) {
            $html = str_replace($placeholder, $code_html, $html);
        }
        
        // 11. Restore inline code
        foreach ($inline_codes as $placeholder => $code_html) {
            $html = str_replace($placeholder, $code_html, $html);
        }
        
        // 12. Paragraphs - split by double newlines but avoid wrapping block elements
        $html = $this->wrap_paragraphs($html);
        
        // 13. Line breaks - convert single newlines to <br> within paragraphs
        $html = preg_replace('/\n(?![<\n])/', '<br>', $html);
        
        // Clean up excessive whitespace
        $html = preg_replace('/\n{3,}/', "\n\n", $html);
        
        return $html;
    }
    
    /**
     * Parse markdown tables
     * 
     * @param string $html HTML content
     * @return string Processed HTML
     */
    private function parse_tables($html) {
        // Match markdown tables
        $html = preg_replace_callback('/^(\|.+\|)\n(\|[\s\-\:]+\|)\n((?:\|.+\|\n?)+)/m', function($matches) {
            $header_line = trim($matches[1]);
            $separator_line = trim($matches[2]);
            $body_lines = trim($matches[3]);
            
            // Parse header
            $headers = array_map('trim', explode('|', trim($header_line, '|')));
            
            // Parse alignment from separator line
            $alignments = array();
            $separators = explode('|', trim($separator_line, '|'));
            foreach ($separators as $sep) {
                $sep = trim($sep);
                if (preg_match('/^:.*:$/', $sep)) {
                    $alignments[] = 'center';
                } elseif (preg_match('/^:/', $sep)) {
                    $alignments[] = 'left';
                } elseif (preg_match('/:$/', $sep)) {
                    $alignments[] = 'right';
                } else {
                    $alignments[] = '';
                }
            }
            
            // Build table HTML
            $table = '<table class="markdown-table">';
            
            // Header
            $table .= '<thead><tr>';
            foreach ($headers as $i => $header) {
                $align = isset($alignments[$i]) && $alignments[$i] ? ' style="text-align:' . $alignments[$i] . '"' : '';
                $table .= '<th' . $align . '>' . $header . '</th>';
            }
            $table .= '</tr></thead>';
            
            // Body
            $table .= '<tbody>';
            $body_rows = explode("\n", $body_lines);
            foreach ($body_rows as $row) {
                if (empty(trim($row))) continue;
                $cells = array_map('trim', explode('|', trim($row, '|')));
                $table .= '<tr>';
                foreach ($cells as $i => $cell) {
                    $align = isset($alignments[$i]) && $alignments[$i] ? ' style="text-align:' . $alignments[$i] . '"' : '';
                    $table .= '<td' . $align . '>' . $cell . '</td>';
                }
                $table .= '</tr>';
            }
            $table .= '</tbody>';
            
            $table .= '</table>';
            return $table;
        }, $html);
        
        return $html;
    }
    
    /**
     * Parse markdown lists (ordered and unordered, with nesting)
     * 
     * @param string $html HTML content
     * @return string Processed HTML
     */
    private function parse_lists($html) {
        $lines = explode("\n", $html);
        $result = array();
        $list_stack = array(); // Stack to track nested lists
        
        foreach ($lines as $line) {
            // Check for list items (unordered: -, *, + | ordered: 1., 2., etc.)
            if (preg_match('/^(\s*)([\-\*\+]|\d+\.)\s+(.*)$/', $line, $matches)) {
                $indent = strlen($matches[1]);
                $marker = $matches[2];
                $content = $matches[3];
                $is_ordered = preg_match('/^\d+\.$/', $marker);
                $list_type = $is_ordered ? 'ol' : 'ul';
                
                // Determine nesting level (every 2-4 spaces is one level)
                $level = intval($indent / 2);
                
                // Close lists if we've decreased indentation
                while (count($list_stack) > $level + 1) {
                    $closed_type = array_pop($list_stack);
                    $result[] = '</' . $closed_type . '>';
                }
                
                // Open new list if needed
                if (count($list_stack) === $level) {
                    $result[] = '<' . $list_type . '>';
                    $list_stack[] = $list_type;
                } elseif (count($list_stack) > 0 && $list_stack[count($list_stack) - 1] !== $list_type && count($list_stack) === $level + 1) {
                    // Different list type at same level - close previous and open new
                    $closed_type = array_pop($list_stack);
                    $result[] = '</' . $closed_type . '>';
                    $result[] = '<' . $list_type . '>';
                    $list_stack[] = $list_type;
                }
                
                $result[] = '<li>' . $content . '</li>';
            } else {
                // Close all open lists
                while (!empty($list_stack)) {
                    $closed_type = array_pop($list_stack);
                    $result[] = '</' . $closed_type . '>';
                }
                $result[] = $line;
            }
        }
        
        // Close any remaining open lists
        while (!empty($list_stack)) {
            $closed_type = array_pop($list_stack);
            $result[] = '</' . $closed_type . '>';
        }
        
        return implode("\n", $result);
    }
    
    /**
     * Parse markdown blockquotes
     * 
     * @param string $html HTML content
     * @return string Processed HTML
     */
    private function parse_blockquotes($html) {
        $lines = explode("\n", $html);
        $result = array();
        $in_blockquote = false;
        $blockquote_lines = array();
        
        foreach ($lines as $line) {
            if (preg_match('/^>\s?(.*)$/', $line, $matches)) {
                if (!$in_blockquote) {
                    $in_blockquote = true;
                }
                $blockquote_lines[] = $matches[1];
            } else {
                if ($in_blockquote) {
                    $result[] = '<blockquote>' . implode("\n", $blockquote_lines) . '</blockquote>';
                    $blockquote_lines = array();
                    $in_blockquote = false;
                }
                $result[] = $line;
            }
        }
        
        // Close blockquote if still open
        if ($in_blockquote) {
            $result[] = '<blockquote>' . implode("\n", $blockquote_lines) . '</blockquote>';
        }
        
        return implode("\n", $result);
    }
    
    /**
     * Wrap content in paragraphs, but skip block-level elements
     * 
     * @param string $html HTML content
     * @return string Processed HTML
     */
    private function wrap_paragraphs($html) {
        // Split by double newlines
        $blocks = preg_split('/\n\n+/', $html);
        $result = array();
        
        foreach ($blocks as $block) {
            $block = trim($block);
            if (empty($block)) continue;
            
            // Check if block starts with a block-level element
            if (preg_match('/^<(h[1-6]|ul|ol|pre|blockquote|table|div|hr)[\s>]/', $block)) {
                $result[] = $block;
            } else {
                // Wrap in paragraph
                $result[] = '<p>' . $block . '</p>';
            }
        }
        
        return implode("\n\n", $result);
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
