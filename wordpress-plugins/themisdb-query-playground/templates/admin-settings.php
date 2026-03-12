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
    • Total Lines:     142                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 9d3ecaa0e  2026-02-28  Add ThemisDB Wiki Integration plugin with documentation i... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */


if (!defined('ABSPATH')) exit;
?>

<div class="wrap">
    <h1><?php echo esc_html(get_admin_page_title()); ?></h1>
    
    <div class="themisdb-admin-header" style="background: #f0f0f1; padding: 15px; border-left: 4px solid #2ea44f; margin: 20px 0;">
        <p>Configure ThemisDB Query Playground settings. Requires ThemisDB PHP Client.</p>
    </div>

    <?php settings_errors('themisdb_qp_settings'); ?>

    <form method="post" action="options.php">
        <?php settings_fields('themisdb_qp_settings'); ?>

        <h2>Connection Settings</h2>
        <table class="form-table">
            <tr>
                <th><label for="themisdb_qp_endpoint">ThemisDB Endpoint</label></th>
                <td>
                    <input type="text" name="themisdb_qp_endpoint" id="themisdb_qp_endpoint" 
                           value="<?php echo esc_attr(get_option('themisdb_qp_endpoint')); ?>" 
                           class="regular-text" placeholder="http://localhost:8080" />
                    <p class="description">URL to ThemisDB instance (e.g., http://themisdb:8080)</p>
                </td>
            </tr>
            <tr>
                <th><label for="themisdb_qp_client_path">PHP Client Path</label></th>
                <td>
                    <input type="text" name="themisdb_qp_client_path" id="themisdb_qp_client_path" 
                           value="<?php echo esc_attr(get_option('themisdb_qp_client_path')); ?>" 
                           class="regular-text" placeholder="/path/to/ThemisDB/clients/php" />
                    <p class="description">Path to ThemisDB PHP client directory</p>
                </td>
            </tr>
            <tr>
                <th><label for="themisdb_qp_namespace">Namespace</label></th>
                <td>
                    <input type="text" name="themisdb_qp_namespace" id="themisdb_qp_namespace" 
                           value="<?php echo esc_attr(get_option('themisdb_qp_namespace')); ?>" 
                           class="regular-text" placeholder="default" />
                </td>
            </tr>
            <tr>
                <th><label for="themisdb_qp_timeout">Timeout (seconds)</label></th>
                <td>
                    <input type="number" name="themisdb_qp_timeout" id="themisdb_qp_timeout" 
                           value="<?php echo esc_attr(get_option('themisdb_qp_timeout')); ?>" 
                           min="5" max="300" />
                </td>
            </tr>
        </table>

        <h2>Security Settings</h2>
        <table class="form-table">
            <tr>
                <th><label for="themisdb_qp_enable_execution">Enable Query Execution</label></th>
                <td>
                    <input type="checkbox" name="themisdb_qp_enable_execution" id="themisdb_qp_enable_execution" 
                           value="1" <?php checked(get_option('themisdb_qp_enable_execution'), 1); ?> />
                    <label for="themisdb_qp_enable_execution">Allow users to execute queries</label>
                </td>
            </tr>
            <tr>
                <th><label for="themisdb_qp_read_only_mode">Read-Only Mode</label></th>
                <td>
                    <input type="checkbox" name="themisdb_qp_read_only_mode" id="themisdb_qp_read_only_mode" 
                           value="1" <?php checked(get_option('themisdb_qp_read_only_mode'), 1); ?> />
                    <label for="themisdb_qp_read_only_mode">Block INSERT/UPDATE/DELETE queries</label>
                </td>
            </tr>
            <tr>
                <th><label for="themisdb_qp_max_results">Max Results</label></th>
                <td>
                    <input type="number" name="themisdb_qp_max_results" id="themisdb_qp_max_results" 
                           value="<?php echo esc_attr(get_option('themisdb_qp_max_results')); ?>" 
                           min="10" max="1000" />
                    <p class="description">Maximum number of results to return</p>
                </td>
            </tr>
        </table>

        <h2>Display Settings</h2>
        <table class="form-table">
            <tr>
                <th><label for="themisdb_qp_enable_examples">Enable Examples</label></th>
                <td>
                    <input type="checkbox" name="themisdb_qp_enable_examples" id="themisdb_qp_enable_examples" 
                           value="1" <?php checked(get_option('themisdb_qp_enable_examples'), 1); ?> />
                    <label for="themisdb_qp_enable_examples">Show example queries</label>
                </td>
            </tr>
            <tr>
                <th><label for="themisdb_qp_theme">Editor Theme</label></th>
                <td>
                    <select name="themisdb_qp_theme" id="themisdb_qp_theme">
                        <option value="monokai" <?php selected(get_option('themisdb_qp_theme'), 'monokai'); ?>>Monokai</option>
                        <option value="dracula" <?php selected(get_option('themisdb_qp_theme'), 'dracula'); ?>>Dracula</option>
                        <option value="default" <?php selected(get_option('themisdb_qp_theme'), 'default'); ?>>Default</option>
                    </select>
                </td>
            </tr>
        </table>

        <?php submit_button(); ?>
    </form>

    <div style="margin-top: 30px; padding: 20px; background: #fff; border: 1px solid #ccd0d4;">
        <h2>Shortcode Usage</h2>
        <h3>Basic</h3>
        <code>[themisdb_query_playground]</code>
        
        <h3>With Default Query</h3>
        <code>[themisdb_query_playground default_query="SELECT * FROM urn:themis:relational:wikipedia_articles LIMIT 10"]</code>
        
        <h3>Custom Height</h3>
        <code>[themisdb_query_playground height="600px"]</code>
    </div>
</div>
