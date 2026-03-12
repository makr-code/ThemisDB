<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-compendium-admin.php                         ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:16                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     255                                            ║
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
 * ThemisDB Compendium Admin Settings
 * 
 * Handles admin settings page
 */

// Prevent direct access
if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Compendium_Admin {
    
    /**
     * Constructor
     */
    public function __construct() {
        add_action('admin_menu', array($this, 'add_admin_menu'));
        add_action('admin_init', array($this, 'register_settings'));
    }
    
    /**
     * Add admin menu
     */
    public function add_admin_menu() {
        add_options_page(
            __('ThemisDB Kompendium Downloads', 'themisdb-compendium-downloads'),
            __('Kompendium Downloads', 'themisdb-compendium-downloads'),
            'manage_options',
            'themisdb-compendium-downloads',
            array($this, 'render_settings_page')
        );
    }
    
    /**
     * Register settings
     */
    public function register_settings() {
        register_setting('themisdb_compendium_settings', 'themisdb_compendium_github_repo');
        register_setting('themisdb_compendium_settings', 'themisdb_compendium_show_file_sizes');
        register_setting('themisdb_compendium_settings', 'themisdb_compendium_cache_duration');
        register_setting('themisdb_compendium_settings', 'themisdb_compendium_button_style');
        register_setting('themisdb_compendium_settings', 'themisdb_compendium_search_term');
        
        add_settings_section(
            'themisdb_compendium_main_section',
            __('Haupteinstellungen', 'themisdb-compendium-downloads'),
            array($this, 'render_section_info'),
            'themisdb-compendium-downloads'
        );
        
        add_settings_field(
            'themisdb_compendium_github_repo',
            __('GitHub Repository', 'themisdb-compendium-downloads'),
            array($this, 'render_github_repo_field'),
            'themisdb-compendium-downloads',
            'themisdb_compendium_main_section'
        );
        
        add_settings_field(
            'themisdb_compendium_show_file_sizes',
            __('Dateigrößen anzeigen', 'themisdb-compendium-downloads'),
            array($this, 'render_show_file_sizes_field'),
            'themisdb-compendium-downloads',
            'themisdb_compendium_main_section'
        );
        
        add_settings_field(
            'themisdb_compendium_cache_duration',
            __('Cache-Dauer (Sekunden)', 'themisdb-compendium-downloads'),
            array($this, 'render_cache_duration_field'),
            'themisdb-compendium-downloads',
            'themisdb_compendium_main_section'
        );
        
        add_settings_field(
            'themisdb_compendium_button_style',
            __('Button-Stil', 'themisdb-compendium-downloads'),
            array($this, 'render_button_style_field'),
            'themisdb-compendium-downloads',
            'themisdb_compendium_main_section'
        );
        
        add_settings_field(
            'themisdb_compendium_search_term',
            __('Dateiname-Filter', 'themisdb-compendium-downloads'),
            array($this, 'render_search_term_field'),
            'themisdb-compendium-downloads',
            'themisdb_compendium_main_section'
        );
    }
    
    /**
     * Render section info
     */
    public function render_section_info() {
        echo '<p>' . __('Konfigurieren Sie die Einstellungen für die Kompendium-Downloads.', 'themisdb-compendium-downloads') . '</p>';
    }
    
    /**
     * Render GitHub repo field
     */
    public function render_github_repo_field() {
        $value = get_option('themisdb_compendium_github_repo', 'makr-code/wordpressPlugins');
        echo '<input type="text" name="themisdb_compendium_github_repo" value="' . esc_attr($value) . '" class="regular-text">';
        echo '<p class="description">' . __('Format: owner/repository (z.B. makr-code/wordpressPlugins)', 'themisdb-compendium-downloads') . '</p>';
    }
    
    /**
     * Render show file sizes field
     */
    public function render_show_file_sizes_field() {
        $value = get_option('themisdb_compendium_show_file_sizes', 1);
        echo '<label>';
        echo '<input type="checkbox" name="themisdb_compendium_show_file_sizes" value="1" ' . checked($value, 1, false) . '>';
        echo ' ' . __('Dateigröße bei Downloads anzeigen', 'themisdb-compendium-downloads');
        echo '</label>';
    }
    
    /**
     * Render cache duration field
     */
    public function render_cache_duration_field() {
        $value = get_option('themisdb_compendium_cache_duration', 3600);
        echo '<input type="number" name="themisdb_compendium_cache_duration" value="' . esc_attr($value) . '" class="small-text" min="60">';
        echo '<p class="description">' . __('Wie lange sollen Release-Daten zwischengespeichert werden? (Standard: 3600 = 1 Stunde)', 'themisdb-compendium-downloads') . '</p>';
    }
    
    /**
     * Render button style field
     */
    public function render_button_style_field() {
        $value = get_option('themisdb_compendium_button_style', 'modern');
        $styles = array(
            'modern' => __('Modern', 'themisdb-compendium-downloads'),
            'classic' => __('Klassisch', 'themisdb-compendium-downloads'),
            'minimal' => __('Minimal', 'themisdb-compendium-downloads')
        );
        
        echo '<select name="themisdb_compendium_button_style">';
        foreach ($styles as $key => $label) {
            echo '<option value="' . esc_attr($key) . '" ' . selected($value, $key, false) . '>' . esc_html($label) . '</option>';
        }
        echo '</select>';
    }
    
    /**
     * Render search term field
     */
    public function render_search_term_field() {
        $value = get_option('themisdb_compendium_search_term', 'kompendium');
        echo '<input type="text" name="themisdb_compendium_search_term" value="' . esc_attr($value) . '" class="regular-text">';
        echo '<p class="description">' . __('Suchbegriff für PDF-Dateinamen in Releases (z.B. "kompendium", "compendium", "documentation")', 'themisdb-compendium-downloads') . '</p>';
    }
    
    /**
     * Render settings page
     */
    public function render_settings_page() {
        if (!current_user_can('manage_options')) {
            return;
        }
        
        // Handle cache clear
        if (isset($_POST['clear_cache']) && check_admin_referer('themisdb_clear_cache')) {
            delete_transient('themisdb_compendium_release_data');
            echo '<div class="notice notice-success is-dismissible"><p>' . 
                 __('Cache wurde erfolgreich geleert.', 'themisdb-compendium-downloads') . 
                 '</p></div>';
        }
        
        ?>
        <div class="wrap">
            <h1><?php echo esc_html(get_admin_page_title()); ?></h1>
            
            <form method="post" action="options.php">
                <?php
                settings_fields('themisdb_compendium_settings');
                do_settings_sections('themisdb-compendium-downloads');
                submit_button(__('Einstellungen speichern', 'themisdb-compendium-downloads'));
                ?>
            </form>
            
            <hr>
            
            <h2><?php _e('Cache verwalten', 'themisdb-compendium-downloads'); ?></h2>
            <p><?php _e('Löschen Sie den Cache, um die neuesten Release-Daten von GitHub abzurufen.', 'themisdb-compendium-downloads'); ?></p>
            <form method="post">
                <?php wp_nonce_field('themisdb_clear_cache'); ?>
                <input type="submit" name="clear_cache" class="button button-secondary" value="<?php _e('Cache leeren', 'themisdb-compendium-downloads'); ?>">
            </form>
            
            <hr>
            
            <h2><?php _e('Verwendung', 'themisdb-compendium-downloads'); ?></h2>
            <p><?php _e('Fügen Sie den folgenden Shortcode in Ihre Seiten oder Beiträge ein:', 'themisdb-compendium-downloads'); ?></p>
            <code>[themisdb_compendium_downloads]</code>
            
            <h3><?php _e('Shortcode-Optionen', 'themisdb-compendium-downloads'); ?></h3>
            <ul>
                <li><code>style</code> - <?php _e('Stil: modern, classic, minimal', 'themisdb-compendium-downloads'); ?></li>
                <li><code>show_version</code> - <?php _e('Version anzeigen: yes, no', 'themisdb-compendium-downloads'); ?></li>
                <li><code>show_date</code> - <?php _e('Datum anzeigen: yes, no', 'themisdb-compendium-downloads'); ?></li>
                <li><code>show_size</code> - <?php _e('Dateigröße anzeigen: yes, no', 'themisdb-compendium-downloads'); ?></li>
                <li><code>layout</code> - <?php _e('Layout: cards, list, compact', 'themisdb-compendium-downloads'); ?></li>
            </ul>
            
            <h3><?php _e('Beispiele', 'themisdb-compendium-downloads'); ?></h3>
            <p><code>[themisdb_compendium_downloads style="modern" layout="cards"]</code></p>
            <p><code>[themisdb_compendium_downloads style="minimal" show_version="no"]</code></p>
            
            <hr>
            
            <h2><?php _e('Download-Statistiken', 'themisdb-compendium-downloads'); ?></h2>
            <?php
            $stats = get_option('themisdb_compendium_download_stats', array());
            if (!empty($stats)) {
                echo '<table class="widefat">';
                echo '<thead><tr><th>' . __('Datei', 'themisdb-compendium-downloads') . '</th><th>' . __('Downloads', 'themisdb-compendium-downloads') . '</th></tr></thead>';
                echo '<tbody>';
                foreach ($stats as $file => $count) {
                    echo '<tr><td>' . esc_html($file) . '</td><td>' . esc_html($count) . '</td></tr>';
                }
                echo '</tbody></table>';
            } else {
                echo '<p>' . __('Noch keine Downloads erfasst.', 'themisdb-compendium-downloads') . '</p>';
            }
            ?>
        </div>
        <?php
    }
}
