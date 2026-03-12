<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            admin-settings.php                                 ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:18                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     193                                            ║
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
 * Admin Settings Template
 */

if (!defined('ABSPATH')) {
    exit;
}

// Handle form submission
if (isset($_POST['themisdb_matrix_save'])) {
    check_admin_referer('themisdb_matrix_settings');
    
    update_option('themisdb_matrix_default_category', sanitize_text_field($_POST['default_category']));
    update_option('themisdb_matrix_default_style', sanitize_text_field($_POST['default_style']));
    update_option('themisdb_matrix_show_legend', isset($_POST['show_legend']) ? 1 : 0);
    update_option('themisdb_matrix_enable_filtering', isset($_POST['enable_filtering']) ? 1 : 0);
    update_option('themisdb_matrix_enable_sorting', isset($_POST['enable_sorting']) ? 1 : 0);
    update_option('themisdb_matrix_sticky_header', isset($_POST['sticky_header']) ? 1 : 0);
    update_option('themisdb_matrix_highlight_themis', isset($_POST['highlight_themis']) ? 1 : 0);
    update_option('themisdb_matrix_enable_export', isset($_POST['enable_export']) ? 1 : 0);
    update_option('themisdb_matrix_export_prefix', sanitize_text_field($_POST['export_prefix']));
    
    echo '<div class="notice notice-success"><p>' . __('Settings saved successfully!', 'themisdb-feature-matrix') . '</p></div>';
}
?>

<div class="wrap">
    <h1><?php _e('ThemisDB Feature Matrix Settings', 'themisdb-feature-matrix'); ?></h1>
    
    <form method="post" action="">
        <?php wp_nonce_field('themisdb_matrix_settings'); ?>
        
        <table class="form-table">
            <tbody>
                <tr>
                    <th scope="row">
                        <label for="default_category"><?php _e('Default Category', 'themisdb-feature-matrix'); ?></label>
                    </th>
                    <td>
                        <select name="default_category" id="default_category" class="regular-text">
                            <option value="all" <?php selected(get_option('themisdb_matrix_default_category', 'all'), 'all'); ?>>
                                <?php _e('All Features', 'themisdb-feature-matrix'); ?>
                            </option>
                            <option value="data_models" <?php selected(get_option('themisdb_matrix_default_category'), 'data_models'); ?>>
                                <?php _e('Data Models', 'themisdb-feature-matrix'); ?>
                            </option>
                            <option value="ai_ml" <?php selected(get_option('themisdb_matrix_default_category'), 'ai_ml'); ?>>
                                <?php _e('AI/ML Features', 'themisdb-feature-matrix'); ?>
                            </option>
                            <option value="performance" <?php selected(get_option('themisdb_matrix_default_category'), 'performance'); ?>>
                                <?php _e('Performance & Scaling', 'themisdb-feature-matrix'); ?>
                            </option>
                            <option value="compatibility" <?php selected(get_option('themisdb_matrix_default_category'), 'compatibility'); ?>>
                                <?php _e('Protocol Compatibility', 'themisdb-feature-matrix'); ?>
                            </option>
                            <option value="pricing" <?php selected(get_option('themisdb_matrix_default_category'), 'pricing'); ?>>
                                <?php _e('Licensing & Cost', 'themisdb-feature-matrix'); ?>
                            </option>
                        </select>
                        <p class="description">
                            <?php _e('Default category to display when the shortcode is loaded.', 'themisdb-feature-matrix'); ?>
                        </p>
                    </td>
                </tr>
                
                <tr>
                    <th scope="row">
                        <label for="default_style"><?php _e('Default Style', 'themisdb-feature-matrix'); ?></label>
                    </th>
                    <td>
                        <label>
                            <input type="radio" name="default_style" value="modern" 
                                   <?php checked(get_option('themisdb_matrix_default_style', 'modern'), 'modern'); ?>>
                            <?php _e('Modern', 'themisdb-feature-matrix'); ?>
                        </label>
                        <br>
                        <label>
                            <input type="radio" name="default_style" value="minimal" 
                                   <?php checked(get_option('themisdb_matrix_default_style'), 'minimal'); ?>>
                            <?php _e('Minimal', 'themisdb-feature-matrix'); ?>
                        </label>
                        <p class="description">
                            <?php _e('Visual style for the feature matrix.', 'themisdb-feature-matrix'); ?>
                        </p>
                    </td>
                </tr>
                
                <tr>
                    <th scope="row"><?php _e('Enable Features', 'themisdb-feature-matrix'); ?></th>
                    <td>
                        <fieldset>
                            <label>
                                <input type="checkbox" name="show_legend" value="1" 
                                       <?php checked(get_option('themisdb_matrix_show_legend', 1), 1); ?>>
                                <?php _e('Show Legend', 'themisdb-feature-matrix'); ?>
                            </label>
                            <br>
                            <label>
                                <input type="checkbox" name="enable_filtering" value="1" 
                                       <?php checked(get_option('themisdb_matrix_enable_filtering', 1), 1); ?>>
                                <?php _e('Enable Filtering', 'themisdb-feature-matrix'); ?>
                            </label>
                            <br>
                            <label>
                                <input type="checkbox" name="enable_sorting" value="1" 
                                       <?php checked(get_option('themisdb_matrix_enable_sorting', 1), 1); ?>>
                                <?php _e('Enable Sorting', 'themisdb-feature-matrix'); ?>
                            </label>
                            <br>
                            <label>
                                <input type="checkbox" name="sticky_header" value="1" 
                                       <?php checked(get_option('themisdb_matrix_sticky_header', 1), 1); ?>>
                                <?php _e('Sticky Header', 'themisdb-feature-matrix'); ?>
                            </label>
                            <br>
                            <label>
                                <input type="checkbox" name="highlight_themis" value="1" 
                                       <?php checked(get_option('themisdb_matrix_highlight_themis', 1), 1); ?>>
                                <?php _e('Highlight ThemisDB', 'themisdb-feature-matrix'); ?>
                            </label>
                        </fieldset>
                    </td>
                </tr>
                
                <tr>
                    <th scope="row"><?php _e('Export Settings', 'themisdb-feature-matrix'); ?></th>
                    <td>
                        <label>
                            <input type="checkbox" name="enable_export" value="1" 
                                   <?php checked(get_option('themisdb_matrix_enable_export', 1), 1); ?>>
                            <?php _e('Enable CSV Export', 'themisdb-feature-matrix'); ?>
                        </label>
                        <br><br>
                        <label for="export_prefix">
                            <?php _e('Export Filename Prefix:', 'themisdb-feature-matrix'); ?>
                        </label>
                        <input type="text" name="export_prefix" id="export_prefix" value="<?php echo esc_attr(get_option('themisdb_matrix_export_prefix', 'themisdb-comparison')); ?>" class="regular-text">
                        <p class="description">
                            <?php _e('Prefix for exported CSV files. Date will be automatically appended.', 'themisdb-feature-matrix'); ?>
                        </p>
                    </td>
                </tr>
            </tbody>
        </table>
        
        <p class="submit">
            <input type="submit" name="themisdb_matrix_save" class="button button-primary" value="<?php _e('Save Settings', 'themisdb-feature-matrix'); ?>">
        </p>
    </form>
    
    <hr>
    
    <h2><?php _e('Shortcode Usage', 'themisdb-feature-matrix'); ?></h2>
    <p><?php _e('Use the following shortcode to display the feature matrix:', 'themisdb-feature-matrix'); ?></p>
    <code>[themisdb_feature_matrix]</code>
    
    <h3><?php _e('Shortcode Parameters', 'themisdb-feature-matrix'); ?></h3>
    <ul>
        <li><code>category</code> - <?php _e('Filter by category (all, data_models, ai_ml, performance, compatibility, pricing)', 'themisdb-feature-matrix'); ?></li>
        <li><code>style</code> - <?php _e('Visual style (modern, minimal)', 'themisdb-feature-matrix'); ?></li>
        <li><code>show_legend</code> - <?php _e('Show legend (yes, no)', 'themisdb-feature-matrix'); ?></li>
        <li><code>filterable</code> - <?php _e('Enable filtering (yes, no)', 'themisdb-feature-matrix'); ?></li>
        <li><code>sticky_header</code> - <?php _e('Sticky table header (yes, no)', 'themisdb-feature-matrix'); ?></li>
        <li><code>highlight_themis</code> - <?php _e('Highlight ThemisDB column (yes, no)', 'themisdb-feature-matrix'); ?></li>
    </ul>
    
    <h3><?php _e('Examples', 'themisdb-feature-matrix'); ?></h3>
    <p><code>[themisdb_feature_matrix category="ai_ml"]</code></p>
    <p><code>[themisdb_feature_matrix category="all" style="modern" show_legend="yes"]</code></p>
</div>
