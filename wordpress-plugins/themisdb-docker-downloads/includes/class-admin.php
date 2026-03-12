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
    • Total Lines:     301                                            ║
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
 * Handles the WordPress admin interface for the plugin
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Docker_Downloads_Admin {
    
    public function __construct() {
        add_action('admin_menu', array($this, 'add_admin_menu'));
        add_action('admin_init', array($this, 'register_settings'));
        add_action('admin_enqueue_scripts', array($this, 'enqueue_admin_scripts'));
        add_action('wp_ajax_themisdb_docker_test_connection', array($this, 'ajax_test_connection'));
        add_action('wp_ajax_themisdb_docker_clear_cache', array($this, 'ajax_clear_cache'));
    }
    
    /**
     * Add admin menu page
     */
    public function add_admin_menu() {
        add_options_page(
            'ThemisDB Docker Downloads Einstellungen',
            'ThemisDB Docker',
            'manage_options',
            'themisdb-docker-downloads',
            array($this, 'render_settings_page')
        );
    }
    
    /**
     * Register plugin settings
     */
    public function register_settings() {
        register_setting('themisdb_docker_downloads_settings', 'themisdb_docker_namespace');
        register_setting('themisdb_docker_downloads_settings', 'themisdb_docker_repository');
        register_setting('themisdb_docker_downloads_settings', 'themisdb_docker_cache_duration');
        register_setting('themisdb_docker_downloads_settings', 'themisdb_docker_token');
        register_setting('themisdb_docker_downloads_settings', 'themisdb_docker_tags_count');
    }
    
    /**
     * Enqueue admin scripts and styles
     */
    public function enqueue_admin_scripts($hook) {
        if ($hook !== 'settings_page_themisdb-docker-downloads') {
            return;
        }
        
        wp_enqueue_style(
            'themisdb-docker-downloads-admin',
            THEMISDB_DOCKER_DOWNLOADS_PLUGIN_URL . 'assets/css/admin.css',
            array(),
            THEMISDB_DOCKER_DOWNLOADS_VERSION
        );
        
        wp_enqueue_script(
            'themisdb-docker-downloads-admin',
            THEMISDB_DOCKER_DOWNLOADS_PLUGIN_URL . 'assets/js/admin.js',
            array('jquery'),
            THEMISDB_DOCKER_DOWNLOADS_VERSION,
            true
        );
        
        wp_localize_script('themisdb-docker-downloads-admin', 'themisdbDockerAdmin', array(
            'ajaxurl' => admin_url('admin-ajax.php'),
            'nonce' => wp_create_nonce('themisdb_docker_admin_nonce')
        ));
    }
    
    /**
     * Render settings page
     */
    public function render_settings_page() {
        if (!current_user_can('manage_options')) {
            return;
        }
        
        ?>
        <div class="wrap">
            <h1><?php echo esc_html(get_admin_page_title()); ?></h1>
            
            <form method="post" action="options.php">
                <?php
                settings_fields('themisdb_docker_downloads_settings');
                ?>
                
                <table class="form-table">
                    <tr>
                        <th scope="row">
                            <label for="themisdb_docker_namespace">Docker Hub Namespace</label>
                        </th>
                        <td>
                            <input type="text" 
                                   id="themisdb_docker_namespace" 
                                   name="themisdb_docker_namespace" 
                                   value="<?php echo esc_attr(get_option('themisdb_docker_namespace', 'themisdb')); ?>" 
                                   class="regular-text">
                            <p class="description">
                                Der Namespace auf Docker Hub (z.B. "themisdb" für themisdb/themisdb)
                            </p>
                        </td>
                    </tr>
                    
                    <tr>
                        <th scope="row">
                            <label for="themisdb_docker_repository">Docker Repository Name</label>
                        </th>
                        <td>
                            <input type="text" 
                                   id="themisdb_docker_repository" 
                                   name="themisdb_docker_repository" 
                                   value="<?php echo esc_attr(get_option('themisdb_docker_repository', 'themisdb')); ?>" 
                                   class="regular-text">
                            <p class="description">
                                Der Repository-Name auf Docker Hub (z.B. "themisdb" für themisdb/themisdb)
                            </p>
                        </td>
                    </tr>
                    
                    <tr>
                        <th scope="row">
                            <label for="themisdb_docker_cache_duration">Cache Dauer (Sekunden)</label>
                        </th>
                        <td>
                            <input type="number" 
                                   id="themisdb_docker_cache_duration" 
                                   name="themisdb_docker_cache_duration" 
                                   value="<?php echo esc_attr(get_option('themisdb_docker_cache_duration', 3600)); ?>" 
                                   min="60" 
                                   step="60" 
                                   class="small-text">
                            <p class="description">
                                Wie lange sollen Docker Hub Daten gecacht werden? (Empfohlen: 3600 = 1 Stunde)
                            </p>
                        </td>
                    </tr>
                    
                    <tr>
                        <th scope="row">
                            <label for="themisdb_docker_tags_count">Anzahl Tags</label>
                        </th>
                        <td>
                            <input type="number" 
                                   id="themisdb_docker_tags_count" 
                                   name="themisdb_docker_tags_count" 
                                   value="<?php echo esc_attr(get_option('themisdb_docker_tags_count', 10)); ?>" 
                                   min="1" 
                                   max="100" 
                                   class="small-text">
                            <p class="description">
                                Anzahl der anzuzeigenden Docker Tags (1-100)
                            </p>
                        </td>
                    </tr>
                    
                    <tr>
                        <th scope="row">
                            <label for="themisdb_docker_token">Docker Hub Token (Optional)</label>
                        </th>
                        <td>
                            <input type="password" 
                                   id="themisdb_docker_token" 
                                   name="themisdb_docker_token" 
                                   value="<?php echo esc_attr(get_option('themisdb_docker_token', '')); ?>" 
                                   class="regular-text">
                            <p class="description">
                                Optional: Docker Hub Personal Access Token für höhere Rate Limits
                            </p>
                        </td>
                    </tr>
                </table>
                
                <?php submit_button('Einstellungen speichern'); ?>
            </form>
            
            <hr>
            
            <h2>Tools & Tests</h2>
            
            <table class="form-table">
                <tr>
                    <th scope="row">Docker Hub Verbindung testen</th>
                    <td>
                        <button type="button" id="test-dockerhub-connection" class="button">
                            Verbindung testen
                        </button>
                        <span id="connection-test-result"></span>
                    </td>
                </tr>
                
                <tr>
                    <th scope="row">Cache leeren</th>
                    <td>
                        <button type="button" id="clear-cache" class="button">
                            Cache leeren
                        </button>
                        <p class="description">Löscht alle gecachten Docker Hub Daten</p>
                        <span id="clear-cache-result"></span>
                    </td>
                </tr>
            </table>
            
            <hr>
            
            <h2>Shortcodes</h2>
            
            <table class="form-table">
                <tr>
                    <th scope="row">Neueste Tags anzeigen</th>
                    <td><code>[themisdb_docker_tags]</code></td>
                </tr>
                
                <tr>
                    <th scope="row">Alle Tags anzeigen</th>
                    <td><code>[themisdb_docker_tags show="all"]</code></td>
                </tr>
                
                <tr>
                    <th scope="row">Spezifische Architektur filtern</th>
                    <td>
                        <code>[themisdb_docker_tags architecture="amd64"]</code><br>
                        <code>[themisdb_docker_tags architecture="arm64"]</code>
                    </td>
                </tr>
                
                <tr>
                    <th scope="row">Kompakte Ansicht</th>
                    <td><code>[themisdb_docker_tags style="compact"]</code></td>
                </tr>
                
                <tr>
                    <th scope="row">Tabellen-Ansicht</th>
                    <td><code>[themisdb_docker_tags style="table"]</code></td>
                </tr>
            </table>
        </div>
        <?php
    }
    
    /**
     * AJAX handler to test Docker Hub connection
     */
    public function ajax_test_connection() {
        check_ajax_referer('themisdb_docker_admin_nonce', 'nonce');
        
        if (!current_user_can('manage_options')) {
            wp_send_json_error('Keine Berechtigung');
        }
        
        $api = new ThemisDB_Docker_Downloads_DockerHub_API();
        $latest = $api->get_latest_tag();
        
        if (is_wp_error($latest)) {
            wp_send_json_error($latest->get_error_message());
        }
        
        wp_send_json_success('Verbindung erfolgreich! Neuester Tag: ' . $latest['name']);
    }
    
    /**
     * AJAX handler to clear cache
     */
    public function ajax_clear_cache() {
        check_ajax_referer('themisdb_docker_admin_nonce', 'nonce');
        
        if (!current_user_can('manage_options')) {
            wp_send_json_error('Keine Berechtigung');
        }
        
        $api = new ThemisDB_Docker_Downloads_DockerHub_API();
        $api->clear_cache();
        
        wp_send_json_success('Cache erfolgreich geleert');
    }
}
