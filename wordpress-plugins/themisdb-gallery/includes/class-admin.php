<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-admin.php                                    ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:18                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     505                                            ║
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
 * 
 * Manages plugin settings page and admin interface
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Gallery_Admin {
    
    public function __construct() {
        add_action('admin_menu', array($this, 'add_settings_page'));
        add_action('admin_init', array($this, 'register_settings'));
        add_action('add_meta_boxes', array($this, 'add_gallery_meta_box'));
        add_action('wp_ajax_themisdb_gallery_search', array($this, 'ajax_search_images'));
        add_action('wp_ajax_themisdb_gallery_generate_ai', array($this, 'ajax_generate_ai_image'));
        
        // Media Library Tab Integration
        add_filter('media_upload_tabs', array($this, 'add_media_upload_tab'));
        add_action('media_upload_themisdb_gallery', array($this, 'media_upload_tab_content'));
    }
    
    /**
     * Add settings page to WordPress admin menu
     */
    public function add_settings_page() {
        add_options_page(
            __('ThemisDB Gallery Einstellungen', 'themisdb-gallery'),
            __('ThemisDB Gallery', 'themisdb-gallery'),
            'manage_options',
            'themisdb-gallery',
            array($this, 'render_settings_page')
        );
    }
    
    /**
     * Register plugin settings
     */
    public function register_settings() {
        // API Keys section
        add_settings_section(
            'themisdb_gallery_api_keys',
            __('API-Schlüssel', 'themisdb-gallery'),
            array($this, 'render_api_keys_section'),
            'themisdb-gallery'
        );
        
        register_setting('themisdb_gallery_options', 'themisdb_gallery_unsplash_key');
        add_settings_field(
            'themisdb_gallery_unsplash_key',
            __('Unsplash Access Key', 'themisdb-gallery'),
            array($this, 'render_text_field'),
            'themisdb-gallery',
            'themisdb_gallery_api_keys',
            array('option' => 'themisdb_gallery_unsplash_key', 'description' => __('Erhalten Sie einen kostenlosen API-Schlüssel auf <a href="https://unsplash.com/developers" target="_blank">unsplash.com/developers</a>', 'themisdb-gallery'))
        );
        
        register_setting('themisdb_gallery_options', 'themisdb_gallery_pexels_key');
        add_settings_field(
            'themisdb_gallery_pexels_key',
            __('Pexels API Key', 'themisdb-gallery'),
            array($this, 'render_text_field'),
            'themisdb-gallery',
            'themisdb_gallery_api_keys',
            array('option' => 'themisdb_gallery_pexels_key', 'description' => __('Erhalten Sie einen kostenlosen API-Schlüssel auf <a href="https://www.pexels.com/api/" target="_blank">pexels.com/api</a>', 'themisdb-gallery'))
        );
        
        register_setting('themisdb_gallery_options', 'themisdb_gallery_pixabay_key');
        add_settings_field(
            'themisdb_gallery_pixabay_key',
            __('Pixabay API Key', 'themisdb-gallery'),
            array($this, 'render_text_field'),
            'themisdb-gallery',
            'themisdb_gallery_api_keys',
            array('option' => 'themisdb_gallery_pixabay_key', 'description' => __('Erhalten Sie einen kostenlosen API-Schlüssel auf <a href="https://pixabay.com/api/docs/" target="_blank">pixabay.com/api</a>', 'themisdb-gallery'))
        );
        
        register_setting('themisdb_gallery_options', 'themisdb_gallery_openai_key');
        add_settings_field(
            'themisdb_gallery_openai_key',
            __('OpenAI API Key (Optional)', 'themisdb-gallery'),
            array($this, 'render_text_field'),
            'themisdb-gallery',
            'themisdb_gallery_api_keys',
            array('option' => 'themisdb_gallery_openai_key', 'description' => __('Für AI-Bildgenerierung mit DALL-E. Erhalten Sie einen API-Schlüssel auf <a href="https://platform.openai.com/api-keys" target="_blank">platform.openai.com</a>', 'themisdb-gallery'))
        );
        
        // General settings section
        add_settings_section(
            'themisdb_gallery_general',
            __('Allgemeine Einstellungen', 'themisdb-gallery'),
            null,
            'themisdb-gallery'
        );
        
        register_setting('themisdb_gallery_options', 'themisdb_gallery_default_provider');
        add_settings_field(
            'themisdb_gallery_default_provider',
            __('Standard-Anbieter', 'themisdb-gallery'),
            array($this, 'render_select_field'),
            'themisdb-gallery',
            'themisdb_gallery_general',
            array(
                'option' => 'themisdb_gallery_default_provider',
                'options' => array(
                    'all' => __('Alle', 'themisdb-gallery'),
                    'unsplash' => 'Unsplash',
                    'pexels' => 'Pexels',
                    'pixabay' => 'Pixabay',
                )
            )
        );
        
        register_setting('themisdb_gallery_options', 'themisdb_gallery_images_per_page', array(
            'type' => 'integer',
            'default' => 20,
        ));
        add_settings_field(
            'themisdb_gallery_images_per_page',
            __('Bilder pro Seite', 'themisdb-gallery'),
            array($this, 'render_number_field'),
            'themisdb-gallery',
            'themisdb_gallery_general',
            array('option' => 'themisdb_gallery_images_per_page', 'min' => 5, 'max' => 50)
        );
        
        register_setting('themisdb_gallery_options', 'themisdb_gallery_cache_duration', array(
            'type' => 'integer',
            'default' => 3600,
        ));
        add_settings_field(
            'themisdb_gallery_cache_duration',
            __('Cache-Dauer (Sekunden)', 'themisdb-gallery'),
            array($this, 'render_number_field'),
            'themisdb-gallery',
            'themisdb_gallery_general',
            array('option' => 'themisdb_gallery_cache_duration', 'min' => 300, 'max' => 86400)
        );
        
        register_setting('themisdb_gallery_options', 'themisdb_gallery_auto_attribution');
        add_settings_field(
            'themisdb_gallery_auto_attribution',
            __('Automatische Quellenangabe', 'themisdb-gallery'),
            array($this, 'render_checkbox_field'),
            'themisdb-gallery',
            'themisdb_gallery_general',
            array('option' => 'themisdb_gallery_auto_attribution', 'description' => __('Automatisch Quellenangaben zu Bildunterschriften hinzufügen', 'themisdb-gallery'))
        );
    }
    
    /**
     * Render API keys section description
     */
    public function render_api_keys_section() {
        echo '<p>' . __('Konfigurieren Sie API-Schlüssel für die Bildsuche. Alle Dienste bieten kostenlose API-Schlüssel für nicht-kommerzielle Nutzung an.', 'themisdb-gallery') . '</p>';
    }
    
    /**
     * Render text input field
     */
    public function render_text_field($args) {
        $value = get_option($args['option'], '');
        $type = isset($args['type']) ? $args['type'] : 'text';
        echo '<input type="' . esc_attr($type) . '" name="' . esc_attr($args['option']) . '" value="' . esc_attr($value) . '" class="regular-text" />';
        if (isset($args['description'])) {
            echo '<p class="description">' . wp_kses_post($args['description']) . '</p>';
        }
    }
    
    /**
     * Render number input field
     */
    public function render_number_field($args) {
        $value = get_option($args['option'], $args['default'] ?? '');
        echo '<input type="number" name="' . esc_attr($args['option']) . '" value="' . esc_attr($value) . '" min="' . esc_attr($args['min'] ?? '') . '" max="' . esc_attr($args['max'] ?? '') . '" />';
        if (isset($args['description'])) {
            echo '<p class="description">' . esc_html($args['description']) . '</p>';
        }
    }
    
    /**
     * Render select field
     */
    public function render_select_field($args) {
        $value = get_option($args['option'], '');
        echo '<select name="' . esc_attr($args['option']) . '">';
        foreach ($args['options'] as $key => $label) {
            echo '<option value="' . esc_attr($key) . '" ' . selected($value, $key, false) . '>' . esc_html($label) . '</option>';
        }
        echo '</select>';
        if (isset($args['description'])) {
            echo '<p class="description">' . esc_html($args['description']) . '</p>';
        }
    }
    
    /**
     * Render checkbox field
     */
    public function render_checkbox_field($args) {
        $value = get_option($args['option'], 'yes');
        echo '<label><input type="checkbox" name="' . esc_attr($args['option']) . '" value="yes" ' . checked($value, 'yes', false) . ' /> ';
        if (isset($args['description'])) {
            echo esc_html($args['description']);
        }
        echo '</label>';
    }
    
    /**
     * Render settings page
     */
    public function render_settings_page() {
        if (!current_user_can('manage_options')) {
            return;
        }
        
        if (isset($_GET['settings-updated'])) {
            add_settings_error('themisdb_gallery_messages', 'themisdb_gallery_message', __('Einstellungen gespeichert', 'themisdb-gallery'), 'updated');
        }
        
        settings_errors('themisdb_gallery_messages');
        ?>
        <div class="wrap">
            <h1><?php echo esc_html(get_admin_page_title()); ?></h1>
            <form action="options.php" method="post">
                <?php
                settings_fields('themisdb_gallery_options');
                do_settings_sections('themisdb-gallery');
                submit_button(__('Einstellungen speichern', 'themisdb-gallery'));
                ?>
            </form>
            
            <hr />
            
            <h2><?php _e('Cache leeren', 'themisdb-gallery'); ?></h2>
            <p><?php _e('Löscht alle gespeicherten Suchergebnisse aus dem Cache.', 'themisdb-gallery'); ?></p>
            <button type="button" class="button" id="themisdb-gallery-clear-cache"><?php _e('Cache leeren', 'themisdb-gallery'); ?></button>
            
            <script>
            jQuery(document).ready(function($) {
                $('#themisdb-gallery-clear-cache').on('click', function() {
                    var button = $(this);
                    button.prop('disabled', true).text('<?php _e('Wird gelöscht...', 'themisdb-gallery'); ?>');
                    
                    $.post(ajaxurl, {
                        action: 'themisdb_gallery_clear_cache',
                        nonce: '<?php echo wp_create_nonce('themisdb_gallery_clear_cache'); ?>'
                    }, function(response) {
                        button.prop('disabled', false).text('<?php _e('Cache leeren', 'themisdb-gallery'); ?>');
                        alert(response.data.message || '<?php _e('Cache geleert', 'themisdb-gallery'); ?>');
                    });
                });
            });
            </script>
        </div>
        <?php
    }
    
    /**
     * Add gallery meta box to post editor
     */
    public function add_gallery_meta_box() {
        $post_types = array('post', 'page');
        foreach ($post_types as $post_type) {
            add_meta_box(
                'themisdb-gallery-metabox',
                __('ThemisDB Gallery - Bildsuche', 'themisdb-gallery'),
                array($this, 'render_gallery_meta_box'),
                $post_type,
                'side',
                'high'
            );
        }
    }
    
    /**
     * Render gallery meta box
     */
    public function render_gallery_meta_box($post) {
        ?>
        <div id="themisdb-gallery-search-widget">
            <div class="themisdb-gallery-search-controls">
                <input type="text" id="themisdb-gallery-search-input" placeholder="<?php _e('Suche nach Bildern...', 'themisdb-gallery'); ?>" />
                <select id="themisdb-gallery-provider">
                    <option value="all"><?php _e('Alle Anbieter', 'themisdb-gallery'); ?></option>
                    <option value="unsplash">Unsplash</option>
                    <option value="pexels">Pexels</option>
                    <option value="pixabay">Pixabay</option>
                </select>
                <button type="button" class="button button-primary" id="themisdb-gallery-search-btn"><?php _e('Suchen', 'themisdb-gallery'); ?></button>
            </div>
            
            <?php if (get_option('themisdb_gallery_openai_key')): ?>
            <div class="themisdb-gallery-ai-controls" style="margin-top: 10px;">
                <input type="text" id="themisdb-gallery-ai-prompt" placeholder="<?php _e('AI Bild beschreiben...', 'themisdb-gallery'); ?>" />
                <button type="button" class="button" id="themisdb-gallery-ai-btn"><?php _e('AI Generieren', 'themisdb-gallery'); ?></button>
            </div>
            <?php endif; ?>
            
            <div id="themisdb-gallery-results" style="margin-top: 15px;"></div>
        </div>
        <?php
    }
    
    /**
     * AJAX handler for image search
     */
    public function ajax_search_images() {
        check_ajax_referer('themisdb_gallery_admin_nonce', 'nonce');
        
        $query = isset($_POST['query']) ? sanitize_text_field($_POST['query']) : '';
        $provider = isset($_POST['provider']) ? sanitize_text_field($_POST['provider']) : 'all';
        $page = isset($_POST['page']) ? intval($_POST['page']) : 1;
        $per_page = get_option('themisdb_gallery_images_per_page', 20);
        
        if (empty($query)) {
            wp_send_json_error(array('message' => __('Bitte geben Sie einen Suchbegriff ein', 'themisdb-gallery')));
        }
        
        $results = ThemisDB_Gallery_Image_API::search_images($query, $provider, $page, $per_page);
        
        if (is_wp_error($results)) {
            wp_send_json_error(array('message' => $results->get_error_message()));
        }
        
        wp_send_json_success(array('images' => $results));
    }
    
    /**
     * AJAX handler for AI image generation
     */
    public function ajax_generate_ai_image() {
        check_ajax_referer('themisdb_gallery_admin_nonce', 'nonce');
        
        $prompt = isset($_POST['prompt']) ? sanitize_text_field($_POST['prompt']) : '';
        
        if (empty($prompt)) {
            wp_send_json_error(array('message' => __('Bitte geben Sie eine Bildbeschreibung ein', 'themisdb-gallery')));
        }
        
        $result = ThemisDB_Gallery_Image_API::generate_ai_image($prompt);
        
        if (is_wp_error($result)) {
            wp_send_json_error(array('message' => $result->get_error_message()));
        }
        
        wp_send_json_success(array('image' => $result));
    }
    
    /**
     * Add ThemisDB Gallery tab to Media Upload tabs
     * 
     * @param array $tabs Existing tabs
     * @return array Modified tabs
     */
    public function add_media_upload_tab($tabs) {
        $tabs['themisdb_gallery'] = __('ThemisDB Gallery', 'themisdb-gallery');
        return $tabs;
    }
    
    /**
     * Render content for Media Upload tab
     */
    public function media_upload_tab_content() {
        wp_iframe(array($this, 'render_media_upload_iframe'));
    }
    
    /**
     * Render iframe content for Media Upload tab
     */
    public function render_media_upload_iframe() {
        // Enqueue necessary scripts and styles
        wp_enqueue_style(
            'themisdb-gallery-media-tab',
            THEMISDB_GALLERY_PLUGIN_URL . 'assets/css/admin.css',
            array(),
            THEMISDB_GALLERY_VERSION
        );
        
        wp_enqueue_script(
            'themisdb-gallery-media-tab',
            THEMISDB_GALLERY_PLUGIN_URL . 'assets/js/media-tab.js',
            array('jquery'),
            THEMISDB_GALLERY_VERSION,
            true
        );
        
        // Localize script for AJAX
        wp_localize_script('themisdb-gallery-media-tab', 'themisdbGalleryMediaTab', array(
            'ajaxurl' => admin_url('admin-ajax.php'),
            'nonce' => wp_create_nonce('themisdb_gallery_admin_nonce'),
            'searchPlaceholder' => __('Suche nach Bildern...', 'themisdb-gallery'),
            'searchRequired' => __('Bitte geben Sie einen Suchbegriff ein', 'themisdb-gallery'),
            'searching' => __('Suche läuft...', 'themisdb-gallery'),
            'noResults' => __('Keine Bilder gefunden', 'themisdb-gallery'),
            'insertImage' => __('Bild einfügen', 'themisdb-gallery'),
            'downloading' => __('Lade herunter...', 'themisdb-gallery'),
            'error' => __('Fehler beim Laden', 'themisdb-gallery'),
            'imported' => __('Importiert', 'themisdb-gallery'),
            'searchBtn' => __('Suchen', 'themisdb-gallery'),
            'generatingAI' => __('Generiere...', 'themisdb-gallery'),
            'generateAIBtn' => __('AI Generieren', 'themisdb-gallery'),
            'aiPromptRequired' => __('Bitte geben Sie eine Bildbeschreibung ein', 'themisdb-gallery')
        ));
        
        ?>
        <div class="themisdb-gallery-media-tab-wrapper">
            <div class="themisdb-gallery-search-controls" style="padding: 20px; background: #f0f0f0; border-bottom: 1px solid #ddd;">
                <h2><?php _e('Suche nach frei verfügbaren Bildern', 'themisdb-gallery'); ?></h2>
                <p><?php _e('Durchsuchen Sie Unsplash, Pexels und Pixabay nach thematisch passenden Bildern mit automatischer Quellenangabe.', 'themisdb-gallery'); ?></p>
                
                <div style="display: flex; gap: 10px; margin-top: 15px;">
                    <input 
                        type="text" 
                        id="themisdb-gallery-media-search-input" 
                        placeholder="<?php _e('Suche nach Bildern...', 'themisdb-gallery'); ?>" 
                        style="flex: 1; padding: 8px;"
                    />
                    <select id="themisdb-gallery-media-provider" style="padding: 8px;">
                        <option value="all"><?php _e('Alle Anbieter', 'themisdb-gallery'); ?></option>
                        <option value="unsplash">Unsplash</option>
                        <option value="pexels">Pexels</option>
                        <option value="pixabay">Pixabay</option>
                    </select>
                    <button 
                        type="button" 
                        class="button button-primary" 
                        id="themisdb-gallery-media-search-btn"
                        style="padding: 8px 20px;"
                    >
                        <?php _e('Suchen', 'themisdb-gallery'); ?>
                    </button>
                </div>
                
                <?php if (get_option('themisdb_gallery_openai_key')): ?>
                <div style="margin-top: 15px; padding-top: 15px; border-top: 1px solid #ddd;">
                    <h3><?php _e('AI Bildgenerierung', 'themisdb-gallery'); ?></h3>
                    <div style="display: flex; gap: 10px;">
                        <input 
                            type="text" 
                            id="themisdb-gallery-media-ai-prompt" 
                            placeholder="<?php _e('AI Bild beschreiben...', 'themisdb-gallery'); ?>" 
                            style="flex: 1; padding: 8px;"
                        />
                        <button 
                            type="button" 
                            class="button" 
                            id="themisdb-gallery-media-ai-btn"
                            style="padding: 8px 20px;"
                        >
                            <?php _e('AI Generieren', 'themisdb-gallery'); ?>
                        </button>
                    </div>
                </div>
                <?php endif; ?>
            </div>
            
            <div id="themisdb-gallery-media-results" style="padding: 20px;">
                <p style="text-align: center; color: #666; margin: 40px 0;">
                    <?php _e('Geben Sie einen Suchbegriff ein, um Bilder zu finden.', 'themisdb-gallery'); ?>
                </p>
            </div>
        </div>
        <?php
    }
}

// Register AJAX handler for clearing cache
add_action('wp_ajax_themisdb_gallery_clear_cache', function() {
    check_ajax_referer('themisdb_gallery_clear_cache', 'nonce');
    
    if (!current_user_can('manage_options')) {
        wp_send_json_error(array('message' => __('Keine Berechtigung', 'themisdb-gallery')));
    }
    
    global $wpdb;
    $wpdb->query("DELETE FROM {$wpdb->options} WHERE option_name LIKE '_transient_themisdb_gallery_%'");
    $wpdb->query("DELETE FROM {$wpdb->options} WHERE option_name LIKE '_transient_timeout_themisdb_gallery_%'");
    
    wp_send_json_success(array('message' => __('Cache erfolgreich geleert', 'themisdb-gallery')));
});
