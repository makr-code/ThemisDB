<?php
/**
 * Admin Settings Class
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Matrix_Admin {
    
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
            __('Feature Matrix Settings', 'themisdb-feature-matrix'),
            __('Feature Matrix', 'themisdb-feature-matrix'),
            'manage_options',
            'themisdb-matrix-settings',
            array($this, 'render_settings_page')
        );
    }
    
    /**
     * Register settings
     */
    public function register_settings() {
        register_setting('themisdb_matrix_settings', 'themisdb_matrix_default_category');
        register_setting('themisdb_matrix_settings', 'themisdb_matrix_default_style');
        register_setting('themisdb_matrix_settings', 'themisdb_matrix_show_legend');
        register_setting('themisdb_matrix_settings', 'themisdb_matrix_enable_filtering');
        register_setting('themisdb_matrix_settings', 'themisdb_matrix_enable_sorting');
        register_setting('themisdb_matrix_settings', 'themisdb_matrix_sticky_header');
        register_setting('themisdb_matrix_settings', 'themisdb_matrix_highlight_themis');
        register_setting('themisdb_matrix_settings', 'themisdb_matrix_enable_export');
        register_setting('themisdb_matrix_settings', 'themisdb_matrix_export_prefix');
    }
    
    /**
     * Render settings page
     */
    public function render_settings_page() {
        if (!current_user_can('manage_options')) {
            return;
        }
        
        include THEMISDB_MATRIX_DIR . 'templates/admin-settings.php';
    }
}
