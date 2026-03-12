<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-admin.php                                    ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:17                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     381                                            ║
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
 * Admin Panel Handler
 * Manages plugin settings and admin interface
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Downloads_Admin {
    
    public function __construct() {
        add_action('admin_menu', array($this, 'add_admin_menu'));
        add_action('admin_init', array($this, 'register_settings'));
        add_action('admin_enqueue_scripts', array($this, 'enqueue_admin_scripts'));
        add_action('wp_ajax_themisdb_clear_cache', array($this, 'ajax_clear_cache'));
    }
    
    /**
     * Add admin menu page
     */
    public function add_admin_menu() {
        add_options_page(
            'ThemisDB Downloads Einstellungen',
            'ThemisDB Downloads',
            'manage_options',
            'themisdb-downloads',
            array($this, 'render_admin_page')
        );
    }
    
    /**
     * Register plugin settings
     */
    public function register_settings() {
        register_setting('themisdb_downloads_settings', 'themisdb_github_repo');
        register_setting('themisdb_downloads_settings', 'themisdb_github_token');
        register_setting('themisdb_downloads_settings', 'themisdb_cache_duration');
        register_setting('themisdb_downloads_settings', 'themisdb_show_prerelease');
        register_setting('themisdb_downloads_settings', 'themisdb_releases_count');
        register_setting('themisdb_downloads_settings', 'themisdb_auto_taxonomy');
        register_setting('themisdb_downloads_settings', 'themisdb_auto_tags');
        register_setting('themisdb_downloads_settings', 'themisdb_auto_categories');
    }
    
    /**
     * Enqueue admin scripts and styles
     */
    public function enqueue_admin_scripts($hook) {
        if ('settings_page_themisdb-downloads' !== $hook) {
            return;
        }
        
        wp_enqueue_style(
            'themisdb-admin-style',
            THEMISDB_DOWNLOADS_PLUGIN_URL . 'assets/css/admin.css',
            array(),
            THEMISDB_DOWNLOADS_VERSION
        );
        
        wp_enqueue_script(
            'themisdb-admin-script',
            THEMISDB_DOWNLOADS_PLUGIN_URL . 'assets/js/admin.js',
            array('jquery'),
            THEMISDB_DOWNLOADS_VERSION,
            true
        );
        
        wp_localize_script('themisdb-admin-script', 'themisdbAdmin', array(
            'ajaxurl' => admin_url('admin-ajax.php'),
            'nonce' => wp_create_nonce('themisdb_admin_nonce')
        ));
    }
    
    /**
     * AJAX handler to clear cache
     */
    public function ajax_clear_cache() {
        check_ajax_referer('themisdb_admin_nonce', 'nonce');
        
        if (!current_user_can('manage_options')) {
            wp_send_json_error('Keine Berechtigung');
        }
        
        $api = new ThemisDB_Downloads_GitHub_API();
        $api->clear_cache();
        
        wp_send_json_success('Cache erfolgreich gelöscht');
    }
    
    /**
     * Render admin settings page
     */
    public function render_admin_page() {
        if (!current_user_can('manage_options')) {
            return;
        }
        
        // Handle settings update
        if (isset($_POST['themisdb_settings_submit'])) {
            check_admin_referer('themisdb_downloads_settings');
            
            update_option('themisdb_github_repo', sanitize_text_field($_POST['themisdb_github_repo']));
            update_option('themisdb_github_token', sanitize_text_field($_POST['themisdb_github_token']));
            update_option('themisdb_cache_duration', intval($_POST['themisdb_cache_duration']));
            update_option('themisdb_show_prerelease', isset($_POST['themisdb_show_prerelease']) ? 1 : 0);
            update_option('themisdb_releases_count', intval($_POST['themisdb_releases_count']));
            update_option('themisdb_auto_taxonomy', isset($_POST['themisdb_auto_taxonomy']) ? 1 : 0);
            update_option('themisdb_auto_tags', isset($_POST['themisdb_auto_tags']) ? 1 : 0);
            update_option('themisdb_auto_categories', isset($_POST['themisdb_auto_categories']) ? 1 : 0);
            
            echo '<div class="notice notice-success"><p>Einstellungen gespeichert!</p></div>';
        }
        
        // Get current settings
        $repo = get_option('themisdb_github_repo', 'makr-code/wordpressPlugins');
        $token = get_option('themisdb_github_token', '');
        $cache_duration = get_option('themisdb_cache_duration', 3600);
        $show_prerelease = get_option('themisdb_show_prerelease', 0);
        $releases_count = get_option('themisdb_releases_count', 10);
        $auto_taxonomy = get_option('themisdb_auto_taxonomy', 1);
        $auto_tags = get_option('themisdb_auto_tags', 1);
        $auto_categories = get_option('themisdb_auto_categories', 1);
        
        // Test API connection
        $api = new ThemisDB_Downloads_GitHub_API();
        $latest = $api->get_latest_release();
        $api_status = is_wp_error($latest) ? 'error' : 'success';
        
        ?>
        <div class="wrap">
            <h1>ThemisDB Downloads Einstellungen</h1>
            
            <div class="themisdb-admin-header">
                <p>Konfigurieren Sie die Einstellungen für das automatische Abrufen von ThemisDB Releases von GitHub.</p>
            </div>
            
            <?php if ($api_status === 'error'): ?>
                <div class="notice notice-error">
                    <p><strong>Fehler beim Abrufen der Releases:</strong> <?php echo esc_html($latest->get_error_message()); ?></p>
                </div>
            <?php else: ?>
                <div class="notice notice-success">
                    <p><strong>API Verbindung erfolgreich!</strong> Neueste Version: <?php echo esc_html($latest['version']); ?></p>
                </div>
            <?php endif; ?>
            
            <form method="post" action="">
                <?php wp_nonce_field('themisdb_downloads_settings'); ?>
                
                <table class="form-table">
                    <tr>
                        <th scope="row">
                            <label for="themisdb_github_repo">GitHub Repository</label>
                        </th>
                        <td>
                            <input type="text" 
                                   id="themisdb_github_repo" 
                                   name="themisdb_github_repo" 
                                   value="<?php echo esc_attr($repo); ?>" 
                                   class="regular-text"
                                   placeholder="owner/repository">
                            <p class="description">Format: owner/repository (z.B. makr-code/wordpressPlugins)</p>
                        </td>
                    </tr>
                    
                    <tr>
                        <th scope="row">
                            <label for="themisdb_github_token">GitHub Personal Access Token</label>
                        </th>
                        <td>
                            <input type="password" 
                                   id="themisdb_github_token" 
                                   name="themisdb_github_token" 
                                   value="<?php echo esc_attr($token); ?>" 
                                   class="regular-text"
                                   placeholder="ghp_...">
                            <p class="description">Optional: Erhöht das API Rate Limit. <a href="https://github.com/settings/tokens" target="_blank">Token erstellen</a></p>
                        </td>
                    </tr>
                    
                    <tr>
                        <th scope="row">
                            <label for="themisdb_cache_duration">Cache Dauer (Sekunden)</label>
                        </th>
                        <td>
                            <input type="number" 
                                   id="themisdb_cache_duration" 
                                   name="themisdb_cache_duration" 
                                   value="<?php echo esc_attr($cache_duration); ?>" 
                                   min="60" 
                                   max="86400" 
                                   class="small-text">
                            <p class="description">Wie lange sollen die Release-Daten zwischengespeichert werden? (Standard: 3600 = 1 Stunde)</p>
                        </td>
                    </tr>
                    
                    <tr>
                        <th scope="row">
                            <label for="themisdb_releases_count">Anzahl Releases</label>
                        </th>
                        <td>
                            <input type="number" 
                                   id="themisdb_releases_count" 
                                   name="themisdb_releases_count" 
                                   value="<?php echo esc_attr($releases_count); ?>" 
                                   min="1" 
                                   max="50" 
                                   class="small-text">
                            <p class="description">Wie viele Releases sollen angezeigt werden? (Standard: 10)</p>
                        </td>
                    </tr>
                    
                    <tr>
                        <th scope="row">
                            Pre-Releases anzeigen
                        </th>
                        <td>
                            <label>
                                <input type="checkbox" 
                                       id="themisdb_show_prerelease" 
                                       name="themisdb_show_prerelease" 
                                       value="1" 
                                       <?php checked($show_prerelease, 1); ?>>
                                Beta- und Alpha-Versionen anzeigen
                            </label>
                        </td>
                    </tr>
                </table>
                
                <h3>Automatische Schlagwörter und Kategorien</h3>
                <p>Das Plugin analysiert Titel und Textinhalt von Beiträgen und verwendet Textanalyse-Techniken (Häufigkeit, Relevanz, Phrase-Erkennung) um automatisch passende Schlagwörter und Kategorien zu erstellen.</p>
                
                <table class="form-table">
                    <tr>
                        <th scope="row">
                            Automatische Taxonomien aktivieren
                        </th>
                        <td>
                            <label>
                                <input type="checkbox" 
                                       id="themisdb_auto_taxonomy" 
                                       name="themisdb_auto_taxonomy" 
                                       value="1" 
                                       <?php checked($auto_taxonomy, 1); ?>>
                                Automatische Zuweisung von Schlagwörtern und Kategorien aktivieren
                            </label>
                            <p class="description">Wenn aktiviert, analysiert das Plugin beim Speichern den Titel und Inhalt von Beiträgen/Seiten und extrahiert automatisch relevante Tags und Kategorien.</p>
                        </td>
                    </tr>
                    
                    <tr>
                        <th scope="row">
                            Automatische Schlagwörter (Tags)
                        </th>
                        <td>
                            <label>
                                <input type="checkbox" 
                                       id="themisdb_auto_tags" 
                                       name="themisdb_auto_tags" 
                                       value="1" 
                                       <?php checked($auto_tags, 1); ?>>
                                Tags automatisch aus Beitragsinhalt extrahieren
                            </label>
                            <p class="description">Tags werden durch Textanalyse extrahiert: Häufigkeit, Titel-Gewichtung, Wortlänge.</p>
                            <p class="description"><strong>Beispiel-Tags:</strong> ThemisDB, Version, Windows, Support, Database, Installation</p>
                        </td>
                    </tr>
                    
                    <tr>
                        <th scope="row">
                            Automatische Kategorien
                        </th>
                        <td>
                            <label>
                                <input type="checkbox" 
                                       id="themisdb_auto_categories" 
                                       name="themisdb_auto_categories" 
                                       value="1" 
                                       <?php checked($auto_categories, 1); ?>>
                                Kategorien automatisch aus Beitragsinhalt extrahieren
                            </label>
                            <p class="description">Kategorien werden durch Phrase-Analyse erstellt (2-3 Wort-Kombinationen).</p>
                            <p class="description"><strong>Beispiel-Kategorien:</strong> ThemisDB Version, Windows Support, Database Management</p>
                        </td>
                    </tr>
                </table>
                
                <p class="submit">
                    <input type="submit" 
                           name="themisdb_settings_submit" 
                           class="button button-primary" 
                           value="Einstellungen speichern">
                    <button type="button" 
                            id="themisdb_clear_cache" 
                            class="button button-secondary">
                        Cache leeren
                    </button>
                </p>
            </form>
            
            <hr>
            
            <h2>Verwendung</h2>
            <div class="themisdb-usage-instructions">
                <h3>Shortcodes</h3>
                <p>Verwenden Sie diese Shortcodes, um Downloads auf Ihren Seiten anzuzeigen:</p>
                
                <h4>Neueste Version anzeigen:</h4>
                <code>[themisdb_downloads]</code>
                
                <h4>Alle Releases anzeigen:</h4>
                <code>[themisdb_downloads show="all"]</code>
                
                <h4>Nur bestimmte Plattform anzeigen:</h4>
                <code>[themisdb_downloads platform="windows"]</code>
                <code>[themisdb_downloads platform="linux"]</code>
                <code>[themisdb_downloads platform="docker"]</code>
                
                <h4>Kompakte Ansicht:</h4>
                <code>[themisdb_downloads style="compact"]</code>
                
                <h4>README anzeigen:</h4>
                <code>[themisdb_readme]</code>
                <code>[themisdb_readme version="v1.3.4"]</code>
                
                <h4>CHANGELOG anzeigen:</h4>
                <code>[themisdb_changelog]</code>
                <code>[themisdb_changelog version="v1.3.4"]</code>
            </div>
            
            <hr>
            
            <h2>Aktuelle Release Informationen</h2>
            <?php if ($api_status === 'success'): ?>
                <table class="widefat">
                    <tr>
                        <th>Version</th>
                        <td><?php echo esc_html($latest['version']); ?></td>
                    </tr>
                    <tr>
                        <th>Veröffentlicht</th>
                        <td><?php echo esc_html(date_i18n(get_option('date_format'), strtotime($latest['published_at']))); ?></td>
                    </tr>
                    <tr>
                        <th>Anzahl Assets</th>
                        <td><?php echo count($latest['assets']); ?></td>
                    </tr>
                    <tr>
                        <th>SHA256 Checksums</th>
                        <td><?php echo count($latest['sha256sums']); ?> Dateien</td>
                    </tr>
                </table>
            <?php endif; ?>
        </div>
        <?php
    }
}
