<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            admin-settings.php                                 ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:20                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     221                                            ║
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
 * Template: Admin Settings Page
 */

// Prevent direct access
if (!defined('ABSPATH')) {
    exit;
}

// Check user capabilities
if (!current_user_can('manage_options')) {
    return;
}

// Save settings
if (isset($_POST['themisdb_rt_save_settings'])) {
    check_admin_referer('themisdb_rt_settings');
    
    update_option('themisdb_rt_github_repo', sanitize_text_field($_POST['github_repo']));
    update_option('themisdb_rt_changelog_path', sanitize_text_field($_POST['changelog_path']));
    update_option('themisdb_rt_manual_releases', wp_kses_post($_POST['manual_releases']));
    update_option('themisdb_rt_default_view', sanitize_text_field($_POST['default_view']));
    update_option('themisdb_rt_default_theme', sanitize_text_field($_POST['default_theme']));
    update_option('themisdb_rt_show_breaking', isset($_POST['show_breaking']) ? '1' : '0');
    update_option('themisdb_rt_show_features', isset($_POST['show_features']) ? '1' : '0');
    
    // Clear cache
    global $wpdb;
    $wpdb->query($wpdb->prepare(
        "DELETE FROM {$wpdb->options} WHERE option_name LIKE %s",
        '_transient_themisdb_rt_%'
    ));
    
    echo '<div class="notice notice-success"><p>Settings saved successfully!</p></div>';
}

// Get current settings
$github_repo = get_option('themisdb_rt_github_repo', 'makr-code/wordpressPlugins');
$changelog_path = get_option('themisdb_rt_changelog_path', '');
$manual_releases = get_option('themisdb_rt_manual_releases', '');
$default_view = get_option('themisdb_rt_default_view', 'chronological');
$default_theme = get_option('themisdb_rt_default_theme', 'neutral');
$show_breaking = get_option('themisdb_rt_show_breaking', '1');
$show_features = get_option('themisdb_rt_show_features', '1');
?>

<div class="wrap">
    <h1>🕒 Release Timeline Settings</h1>
    
    <p>Configure data sources and display options for the Release Timeline Visualizer.</p>
    
    <form method="post" action="">
        <?php wp_nonce_field('themisdb_rt_settings'); ?>
        
        <h2>Data Sources</h2>
        
        <table class="form-table">
            <tr>
                <th scope="row">
                    <label for="github_repo">GitHub Repository</label>
                </th>
                <td>
                    <input type="text" id="github_repo" name="github_repo" 
                           value="<?php echo esc_attr($github_repo); ?>" 
                           class="regular-text" placeholder="owner/repository">
                    <p class="description">
                        GitHub repository in format: owner/repository (e.g., makr-code/wordpressPlugins)
                    </p>
                </td>
            </tr>
            
            <tr>
                <th scope="row">
                    <label for="changelog_path">CHANGELOG Path</label>
                </th>
                <td>
                    <input type="text" id="changelog_path" name="changelog_path" 
                           value="<?php echo esc_attr($changelog_path); ?>" 
                           class="regular-text" placeholder="/path/to/CHANGELOG.md">
                    <p class="description">
                        Absolute path to CHANGELOG.md file on server
                    </p>
                </td>
            </tr>
            
            <tr>
                <th scope="row">
                    <label for="manual_releases">Manual Release Data</label>
                </th>
                <td>
                    <textarea id="manual_releases" name="manual_releases" 
                              rows="10" class="large-text code"><?php echo esc_textarea($manual_releases); ?></textarea>
                    <p class="description">
                        JSON array of release objects. Example:<br>
                        <code>[{"version":"v1.0.0","name":"First Release","date":"2024-01-01","features":["Feature 1"],"breaking":false}]</code>
                    </p>
                </td>
            </tr>
        </table>
        
        <h2>Display Options</h2>
        
        <table class="form-table">
            <tr>
                <th scope="row">
                    <label for="default_view">Default View</label>
                </th>
                <td>
                    <select id="default_view" name="default_view">
                        <option value="chronological" <?php selected($default_view, 'chronological'); ?>>Chronological</option>
                        <option value="gantt" <?php selected($default_view, 'gantt'); ?>>Gantt Chart</option>
                        <option value="mindmap" <?php selected($default_view, 'mindmap'); ?>>Mind Map</option>
                    </select>
                </td>
            </tr>
            
            <tr>
                <th scope="row">
                    <label for="default_theme">Default Theme</label>
                </th>
                <td>
                    <select id="default_theme" name="default_theme">
                        <option value="neutral" <?php selected($default_theme, 'neutral'); ?>>Neutral</option>
                        <option value="dark" <?php selected($default_theme, 'dark'); ?>>Dark</option>
                        <option value="forest" <?php selected($default_theme, 'forest'); ?>>Forest</option>
                    </select>
                </td>
            </tr>
            
            <tr>
                <th scope="row">Display Options</th>
                <td>
                    <fieldset>
                        <label>
                            <input type="checkbox" name="show_breaking" value="1" 
                                   <?php checked($show_breaking, '1'); ?>>
                            Show breaking changes warning
                        </label>
                        <br>
                        <label>
                            <input type="checkbox" name="show_features" value="1" 
                                   <?php checked($show_features, '1'); ?>>
                            Show feature highlights
                        </label>
                    </fieldset>
                </td>
            </tr>
        </table>
        
        <h2>Shortcode Examples</h2>
        
        <div style="background: #f5f5f5; padding: 1rem; border-left: 4px solid #2563eb; margin: 1rem 0;">
            <p><strong>Basic usage:</strong></p>
            <code>[themisdb_release_timeline]</code>
            
            <p style="margin-top: 1rem;"><strong>Chronological view:</strong></p>
            <code>[themisdb_release_timeline view="chronological"]</code>
            
            <p style="margin-top: 1rem;"><strong>Gantt chart with dark theme:</strong></p>
            <code>[themisdb_release_timeline view="gantt" theme="dark"]</code>
            
            <p style="margin-top: 1rem;"><strong>Mind map of last 5 releases:</strong></p>
            <code>[themisdb_release_timeline view="mindmap" releases="5"]</code>
            
            <p style="margin-top: 1rem;"><strong>From CHANGELOG file:</strong></p>
            <code>[themisdb_release_timeline source="changelog"]</code>
        </div>
        
        <p class="submit">
            <input type="submit" name="themisdb_rt_save_settings" 
                   class="button button-primary" value="Save Settings">
        </p>
    </form>
    
    <hr>
    
    <h2>Cache Management</h2>
    <p>Release data is cached for 1 hour. Clear cache if you need to force reload from source.</p>
    
    <?php if (isset($_POST['clear_cache'])): ?>
        <?php
        check_admin_referer('themisdb_rt_clear_cache');
        global $wpdb;
        $deleted = $wpdb->query($wpdb->prepare(
            "DELETE FROM {$wpdb->options} WHERE option_name LIKE %s",
            '_transient_themisdb_rt_%'
        ));
        echo '<div class="notice notice-success"><p>Cache cleared! (' . $deleted . ' entries removed)</p></div>';
        ?>
    <?php endif; ?>
    
    <form method="post" action="">
        <?php wp_nonce_field('themisdb_rt_clear_cache'); ?>
        <p>
            <input type="submit" name="clear_cache" class="button" value="Clear Cache">
        </p>
    </form>
</div>
