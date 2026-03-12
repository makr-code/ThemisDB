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
    • Total Lines:     298                                            ║
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
 * Admin Settings Class
 * 
 * Handles WordPress admin functionality
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Feature_Matrix_Admin {
    
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
            __('ThemisDB Feature Matrix Settings', 'themisdb-feature-matrix'),
            __('Feature Matrix', 'themisdb-feature-matrix'),
            'manage_options',
            'themisdb-feature-matrix',
            array($this, 'render_settings_page')
        );
    }
    
    /**
     * Register settings
     */
    public function register_settings() {
        // Register settings group
        register_setting('themisdb_fm_settings', 'themisdb_fm_default_view');
        register_setting('themisdb_fm_settings', 'themisdb_fm_enable_filters');
        register_setting('themisdb_fm_settings', 'themisdb_fm_enable_csv_export');
        register_setting('themisdb_fm_settings', 'themisdb_fm_show_themis_highlight');
        register_setting('themisdb_fm_settings', 'themisdb_fm_default_style');
        register_setting('themisdb_fm_settings', 'themisdb_fm_sticky_header');
        register_setting('themisdb_fm_settings', 'themisdb_fm_enable_tooltips');
        register_setting('themisdb_fm_settings', 'themisdb_fm_custom_colors');
        
        // Add settings section
        add_settings_section(
            'themisdb_fm_general',
            __('General Settings', 'themisdb-feature-matrix'),
            array($this, 'general_section_callback'),
            'themisdb-feature-matrix'
        );
        
        // Add settings fields
        add_settings_field(
            'default_view',
            __('Default View', 'themisdb-feature-matrix'),
            array($this, 'default_view_callback'),
            'themisdb-feature-matrix',
            'themisdb_fm_general'
        );
        
        add_settings_field(
            'default_style',
            __('Default Style', 'themisdb-feature-matrix'),
            array($this, 'default_style_callback'),
            'themisdb-feature-matrix',
            'themisdb_fm_general'
        );
        
        add_settings_field(
            'enable_filters',
            __('Enable Category Filters', 'themisdb-feature-matrix'),
            array($this, 'enable_filters_callback'),
            'themisdb-feature-matrix',
            'themisdb_fm_general'
        );
        
        add_settings_field(
            'enable_csv_export',
            __('Enable CSV Export', 'themisdb-feature-matrix'),
            array($this, 'enable_csv_export_callback'),
            'themisdb-feature-matrix',
            'themisdb_fm_general'
        );
        
        add_settings_field(
            'show_themis_highlight',
            __('Show ThemisDB Highlight', 'themisdb-feature-matrix'),
            array($this, 'show_themis_highlight_callback'),
            'themisdb-feature-matrix',
            'themisdb_fm_general'
        );
        
        add_settings_field(
            'sticky_header',
            __('Sticky Header', 'themisdb-feature-matrix'),
            array($this, 'sticky_header_callback'),
            'themisdb-feature-matrix',
            'themisdb_fm_general'
        );
        
        add_settings_field(
            'enable_tooltips',
            __('Enable Tooltips', 'themisdb-feature-matrix'),
            array($this, 'enable_tooltips_callback'),
            'themisdb-feature-matrix',
            'themisdb_fm_general'
        );
    }
    
    /**
     * General section callback
     */
    public function general_section_callback() {
        echo '<p>' . esc_html__('Configure default settings for the Feature Matrix plugin.', 'themisdb-feature-matrix') . '</p>';
    }
    
    /**
     * Default view field callback
     */
    public function default_view_callback() {
        $value = get_option('themisdb_fm_default_view', 'all');
        ?>
        <select name="themisdb_fm_default_view">
            <option value="all" <?php selected($value, 'all'); ?>><?php _e('All Features', 'themisdb-feature-matrix'); ?></option>
            <option value="data_models" <?php selected($value, 'data_models'); ?>><?php _e('Data Models', 'themisdb-feature-matrix'); ?></option>
            <option value="ai_ml" <?php selected($value, 'ai_ml'); ?>><?php _e('AI/ML', 'themisdb-feature-matrix'); ?></option>
            <option value="performance" <?php selected($value, 'performance'); ?>><?php _e('Performance', 'themisdb-feature-matrix'); ?></option>
            <option value="compatibility" <?php selected($value, 'compatibility'); ?>><?php _e('Compatibility', 'themisdb-feature-matrix'); ?></option>
            <option value="licensing" <?php selected($value, 'licensing'); ?>><?php _e('Licensing', 'themisdb-feature-matrix'); ?></option>
        </select>
        <p class="description"><?php _e('Select the default category to display.', 'themisdb-feature-matrix'); ?></p>
        <?php
    }
    
    /**
     * Default style field callback
     */
    public function default_style_callback() {
        $value = get_option('themisdb_fm_default_style', 'modern');
        ?>
        <select name="themisdb_fm_default_style">
            <option value="modern" <?php selected($value, 'modern'); ?>><?php _e('Modern', 'themisdb-feature-matrix'); ?></option>
            <option value="compact" <?php selected($value, 'compact'); ?>><?php _e('Compact', 'themisdb-feature-matrix'); ?></option>
            <option value="minimal" <?php selected($value, 'minimal'); ?>><?php _e('Minimal', 'themisdb-feature-matrix'); ?></option>
        </select>
        <p class="description"><?php _e('Select the default visual style.', 'themisdb-feature-matrix'); ?></p>
        <?php
    }
    
    /**
     * Enable filters field callback
     */
    public function enable_filters_callback() {
        $value = get_option('themisdb_fm_enable_filters', 'yes');
        ?>
        <label>
            <input type="checkbox" name="themisdb_fm_enable_filters" value="yes" <?php checked($value, 'yes'); ?> />
            <?php _e('Show category filter buttons', 'themisdb-feature-matrix'); ?>
        </label>
        <p class="description"><?php _e('Allow users to filter features by category.', 'themisdb-feature-matrix'); ?></p>
        <?php
    }
    
    /**
     * Enable CSV export field callback
     */
    public function enable_csv_export_callback() {
        $value = get_option('themisdb_fm_enable_csv_export', 'yes');
        ?>
        <label>
            <input type="checkbox" name="themisdb_fm_enable_csv_export" value="yes" <?php checked($value, 'yes'); ?> />
            <?php _e('Show CSV export button', 'themisdb-feature-matrix'); ?>
        </label>
        <p class="description"><?php _e('Allow users to export the comparison as CSV.', 'themisdb-feature-matrix'); ?></p>
        <?php
    }
    
    /**
     * Show ThemisDB highlight field callback
     */
    public function show_themis_highlight_callback() {
        $value = get_option('themisdb_fm_show_themis_highlight', 'yes');
        ?>
        <label>
            <input type="checkbox" name="themisdb_fm_show_themis_highlight" value="yes" <?php checked($value, 'yes'); ?> />
            <?php _e('Highlight ThemisDB column with "⭐ Recommended" badge', 'themisdb-feature-matrix'); ?>
        </label>
        <p class="description"><?php _e('Add visual emphasis to the ThemisDB column.', 'themisdb-feature-matrix'); ?></p>
        <?php
    }
    
    /**
     * Sticky header field callback
     */
    public function sticky_header_callback() {
        $value = get_option('themisdb_fm_sticky_header', 'yes');
        ?>
        <label>
            <input type="checkbox" name="themisdb_fm_sticky_header" value="yes" <?php checked($value, 'yes'); ?> />
            <?php _e('Keep table header visible when scrolling', 'themisdb-feature-matrix'); ?>
        </label>
        <p class="description"><?php _e('Make the table header sticky for easier navigation.', 'themisdb-feature-matrix'); ?></p>
        <?php
    }
    
    /**
     * Enable tooltips field callback
     */
    public function enable_tooltips_callback() {
        $value = get_option('themisdb_fm_enable_tooltips', 'yes');
        ?>
        <label>
            <input type="checkbox" name="themisdb_fm_enable_tooltips" value="yes" <?php checked($value, 'yes'); ?> />
            <?php _e('Show info tooltips on hover', 'themisdb-feature-matrix'); ?>
        </label>
        <p class="description"><?php _e('Display helpful tooltips when hovering over features.', 'themisdb-feature-matrix'); ?></p>
        <?php
    }
    
    /**
     * Render settings page
     */
    public function render_settings_page() {
        if (!current_user_can('manage_options')) {
            return;
        }
        
        // Show success message
        if (isset($_GET['settings-updated'])) {
            add_settings_error(
                'themisdb_fm_messages',
                'themisdb_fm_message',
                __('Settings Saved', 'themisdb-feature-matrix'),
                'updated'
            );
        }
        
        settings_errors('themisdb_fm_messages');
        ?>
        <div class="wrap">
            <h1><?php echo esc_html(get_admin_page_title()); ?></h1>
            
            <div class="card">
                <h2><?php _e('Shortcode Usage', 'themisdb-feature-matrix'); ?></h2>
                <p><?php _e('Use this shortcode in your posts or pages:', 'themisdb-feature-matrix'); ?></p>
                <code>[themisdb_feature_matrix]</code>
                
                <h3><?php _e('Shortcode Parameters', 'themisdb-feature-matrix'); ?></h3>
                <ul>
                    <li><code>category</code> - Filter by category: all, data_models, ai_ml, performance, compatibility, licensing</li>
                    <li><code>style</code> - Visual style: modern, compact, minimal</li>
                    <li><code>highlight_themis</code> - Highlight ThemisDB column: yes, no</li>
                    <li><code>sticky_header</code> - Enable sticky header: yes, no</li>
                    <li><code>filterable</code> - Show category filters: yes, no</li>
                </ul>
                
                <h3><?php _e('Example', 'themisdb-feature-matrix'); ?></h3>
                <code>[themisdb_feature_matrix category="ai_ml" style="modern" highlight_themis="yes"]</code>
            </div>
            
            <form action="options.php" method="post">
                <?php
                settings_fields('themisdb_fm_settings');
                do_settings_sections('themisdb-feature-matrix');
                submit_button(__('Save Settings', 'themisdb-feature-matrix'));
                ?>
            </form>
        </div>
        <?php
    }
}
