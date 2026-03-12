<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            admin-settings.php                                 ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:16                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     246                                            ║
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
 * Admin Settings Template for Architecture Diagrams
 */

if (!defined('ABSPATH')) {
    exit;
}
?>

<div class="wrap">
    <h1><?php echo esc_html(get_admin_page_title()); ?></h1>
    
    <div class="themisdb-admin-header">
        <p><?php _e('Configure the Architecture Diagrams plugin settings.', 'themisdb-architecture-diagrams'); ?></p>
    </div>

    <?php settings_errors('themisdb_ad_settings'); ?>

    <form method="post" action="options.php">
        <?php
        settings_fields('themisdb_ad_settings');
        do_settings_sections('themisdb_ad_settings');
        ?>

        <table class="form-table" role="presentation">
            <tr>
                <th scope="row">
                    <label for="themisdb_ad_default_view"><?php _e('Default View', 'themisdb-architecture-diagrams'); ?></label>
                </th>
                <td>
                    <select name="themisdb_ad_default_view" id="themisdb_ad_default_view">
                        <option value="high_level" <?php selected(get_option('themisdb_ad_default_view'), 'high_level'); ?>>
                            <?php _e('High-Level Architecture', 'themisdb-architecture-diagrams'); ?>
                        </option>
                        <option value="storage_layer" <?php selected(get_option('themisdb_ad_default_view'), 'storage_layer'); ?>>
                            <?php _e('Storage Layer', 'themisdb-architecture-diagrams'); ?>
                        </option>
                        <option value="llm_integration" <?php selected(get_option('themisdb_ad_default_view'), 'llm_integration'); ?>>
                            <?php _e('LLM Integration', 'themisdb-architecture-diagrams'); ?>
                        </option>
                        <option value="sharding_raid" <?php selected(get_option('themisdb_ad_default_view'), 'sharding_raid'); ?>>
                            <?php _e('Sharding & RAID', 'themisdb-architecture-diagrams'); ?>
                        </option>
                    </select>
                    <p class="description">
                        <?php _e('Default architecture view when the plugin loads.', 'themisdb-architecture-diagrams'); ?>
                    </p>
                </td>
            </tr>

            <tr>
                <th scope="row">
                    <label for="themisdb_ad_theme"><?php _e('Mermaid Theme', 'themisdb-architecture-diagrams'); ?></label>
                </th>
                <td>
                    <select name="themisdb_ad_theme" id="themisdb_ad_theme">
                        <option value="themis" <?php selected(get_option('themisdb_ad_theme'), 'themis'); ?>>
                            <?php _e('Themis (Recommended)', 'themisdb-architecture-diagrams'); ?>
                        </option>
                        <option value="neutral" <?php selected(get_option('themisdb_ad_theme'), 'neutral'); ?>>
                            <?php _e('Neutral', 'themisdb-architecture-diagrams'); ?>
                        </option>
                        <option value="default" <?php selected(get_option('themisdb_ad_theme'), 'default'); ?>>
                            <?php _e('Default', 'themisdb-architecture-diagrams'); ?>
                        </option>
                        <option value="dark" <?php selected(get_option('themisdb_ad_theme'), 'dark'); ?>>
                            <?php _e('Dark', 'themisdb-architecture-diagrams'); ?>
                        </option>
                        <option value="forest" <?php selected(get_option('themisdb_ad_theme'), 'forest'); ?>>
                            <?php _e('Forest', 'themisdb-architecture-diagrams'); ?>
                        </option>
                    </select>
                    <p class="description">
                        <?php _e('Color theme for diagram rendering. Themis uses official brand colors.', 'themisdb-architecture-diagrams'); ?>
                    </p>
                </td>
            </tr>
            
            <tr>
                <th scope="row">
                    <label for="themisdb_ad_enable_dark_mode"><?php _e('Enable Dark Mode', 'themisdb-architecture-diagrams'); ?></label>
                </th>
                <td>
                    <input type="checkbox" 
                           name="themisdb_ad_enable_dark_mode" 
                           id="themisdb_ad_enable_dark_mode" 
                           value="1" 
                           <?php checked(get_option('themisdb_ad_enable_dark_mode', 1), 1); ?> />
                    <label for="themisdb_ad_enable_dark_mode">
                        <?php _e('Enable automatic dark mode detection (system preference, theme, plugins)', 'themisdb-architecture-diagrams'); ?>
                    </label>
                </td>
            </tr>
            
            <tr>
                <th scope="row">
                    <label for="themisdb_ad_enable_lazy_loading"><?php _e('Enable Lazy Loading', 'themisdb-architecture-diagrams'); ?></label>
                </th>
                <td>
                    <input type="checkbox" 
                           name="themisdb_ad_enable_lazy_loading" 
                           id="themisdb_ad_enable_lazy_loading" 
                           value="1" 
                           <?php checked(get_option('themisdb_ad_enable_lazy_loading', 1), 1); ?> />
                    <label for="themisdb_ad_enable_lazy_loading">
                        <?php _e('Load diagrams only when they become visible (improves performance)', 'themisdb-architecture-diagrams'); ?>
                    </label>
                </td>
            </tr>

            <tr>
                <th scope="row">
                    <label for="themisdb_ad_interactive"><?php _e('Enable Interactivity', 'themisdb-architecture-diagrams'); ?></label>
                </th>
                <td>
                    <input type="checkbox" 
                           name="themisdb_ad_interactive" 
                           id="themisdb_ad_interactive" 
                           value="1" 
                           <?php checked(get_option('themisdb_ad_interactive'), 1); ?> />
                    <label for="themisdb_ad_interactive">
                        <?php _e('Enable clickable components with details', 'themisdb-architecture-diagrams'); ?>
                    </label>
                </td>
            </tr>

            <tr>
                <th scope="row">
                    <label for="themisdb_ad_enable_export"><?php _e('Enable Export', 'themisdb-architecture-diagrams'); ?></label>
                </th>
                <td>
                    <input type="checkbox" 
                           name="themisdb_ad_enable_export" 
                           id="themisdb_ad_enable_export" 
                           value="1" 
                           <?php checked(get_option('themisdb_ad_enable_export'), 1); ?> />
                    <label for="themisdb_ad_enable_export">
                        <?php _e('Enable SVG/PNG export functionality', 'themisdb-architecture-diagrams'); ?>
                    </label>
                </td>
            </tr>

            <tr>
                <th scope="row">
                    <label for="themisdb_ad_show_descriptions"><?php _e('Show Descriptions', 'themisdb-architecture-diagrams'); ?></label>
                </th>
                <td>
                    <input type="checkbox" 
                           name="themisdb_ad_show_descriptions" 
                           id="themisdb_ad_show_descriptions" 
                           value="1" 
                           <?php checked(get_option('themisdb_ad_show_descriptions'), 1); ?> />
                    <label for="themisdb_ad_show_descriptions">
                        <?php _e('Display architecture descriptions below diagrams', 'themisdb-architecture-diagrams'); ?>
                    </label>
                </td>
            </tr>
        </table>

        <?php submit_button(); ?>
    </form>

    <div class="themisdb-admin-section">
        <h2><?php _e('Shortcode Usage', 'themisdb-architecture-diagrams'); ?></h2>
        <div class="themisdb-shortcode-examples">
            <h3><?php _e('Basic Usage', 'themisdb-architecture-diagrams'); ?></h3>
            <code>[themisdb_architecture]</code>
            
            <h3><?php _e('Specific View', 'themisdb-architecture-diagrams'); ?></h3>
            <code>[themisdb_architecture view="storage_layer"]</code>
            
            <h3><?php _e('Custom Theme', 'themisdb-architecture-diagrams'); ?></h3>
            <code>[themisdb_architecture theme="dark"]</code>
            
            <h3><?php _e('Without Controls', 'themisdb-architecture-diagrams'); ?></h3>
            <code>[themisdb_architecture show_controls="false"]</code>
            
            <h3><?php _e('Combined Parameters', 'themisdb-architecture-diagrams'); ?></h3>
            <code>[themisdb_architecture view="llm_integration" theme="neutral" interactive="true"]</code>
        </div>
    </div>

    <div class="themisdb-admin-section">
        <h2><?php _e('Available Views', 'themisdb-architecture-diagrams'); ?></h2>
        <ul>
            <li><strong>high_level</strong> - Complete system architecture overview</li>
            <li><strong>storage_layer</strong> - Storage engine and persistence details</li>
            <li><strong>llm_integration</strong> - LLM/AI integration architecture</li>
            <li><strong>sharding_raid</strong> - Distributed sharding and RAID configuration</li>
        </ul>
    </div>
</div>

<style>
.themisdb-admin-header {
    background: #f0f0f1;
    padding: 15px;
    border-left: 4px solid #2ea44f;
    margin: 20px 0;
}

.themisdb-admin-section {
    margin-top: 30px;
    padding: 20px;
    background: #fff;
    border: 1px solid #ccd0d4;
}

.themisdb-shortcode-examples h3 {
    margin-top: 20px;
    font-size: 14px;
}

.themisdb-shortcode-examples code {
    display: block;
    padding: 10px;
    background: #f6f7f7;
    border: 1px solid #dcdcde;
    border-radius: 3px;
    font-family: Consolas, Monaco, monospace;
    margin-bottom: 15px;
}
</style>
