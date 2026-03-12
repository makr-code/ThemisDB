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
    • Total Lines:     277                                            ║
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
?>

<div class="wrap">
    <h1><?php echo esc_html(get_admin_page_title()); ?></h1>
    
    <div class="themisdb-admin-header">
        <p><?php _e('Configure the Benchmark Visualizer plugin settings.', 'themisdb-benchmark-visualizer'); ?></p>
    </div>

    <?php settings_errors('themisdb_bv_settings'); ?>

    <form method="post" action="options.php">
        <?php
        settings_fields('themisdb_bv_settings');
        do_settings_sections('themisdb_bv_settings');
        ?>

        <table class="form-table" role="presentation">
            <!-- Data Source -->
            <tr>
                <th scope="row">
                    <label for="themisdb_bv_data_source"><?php _e('Data Source', 'themisdb-benchmark-visualizer'); ?></label>
                </th>
                <td>
                    <select name="themisdb_bv_data_source" id="themisdb_bv_data_source">
                        <option value="local" <?php selected(get_option('themisdb_bv_data_source'), 'local'); ?>>
                            <?php _e('Local Files', 'themisdb-benchmark-visualizer'); ?>
                        </option>
                        <option value="github" <?php selected(get_option('themisdb_bv_data_source'), 'github'); ?>>
                            <?php _e('GitHub Repository', 'themisdb-benchmark-visualizer'); ?>
                        </option>
                    </select>
                    <p class="description">
                        <?php _e('Choose whether to load benchmark data from local files or GitHub repository.', 'themisdb-benchmark-visualizer'); ?>
                    </p>
                </td>
            </tr>

            <!-- GitHub Data URL -->
            <tr>
                <th scope="row">
                    <label for="themisdb_bv_github_data_url"><?php _e('GitHub Data URL', 'themisdb-benchmark-visualizer'); ?></label>
                </th>
                <td>
                    <input type="url" 
                           name="themisdb_bv_github_data_url" 
                           id="themisdb_bv_github_data_url" 
                           value="<?php echo esc_attr(get_option('themisdb_bv_github_data_url')); ?>" 
                           class="regular-text" />
                    <p class="description">
                        <?php _e('Base URL for benchmark data in GitHub repository.', 'themisdb-benchmark-visualizer'); ?>
                    </p>
                </td>
            </tr>

            <!-- Default Comparison Databases -->
            <tr>
                <th scope="row">
                    <label for="themisdb_bv_default_comparison_dbs"><?php _e('Default Comparison Databases', 'themisdb-benchmark-visualizer'); ?></label>
                </th>
                <td>
                    <input type="text" 
                           name="themisdb_bv_default_comparison_dbs" 
                           id="themisdb_bv_default_comparison_dbs" 
                           value="<?php echo esc_attr(get_option('themisdb_bv_default_comparison_dbs')); ?>" 
                           class="regular-text" />
                    <p class="description">
                        <?php _e('Comma-separated list of databases to compare (e.g., postgresql,mongodb,neo4j).', 'themisdb-benchmark-visualizer'); ?>
                    </p>
                </td>
            </tr>

            <!-- Default Category -->
            <tr>
                <th scope="row">
                    <label for="themisdb_bv_default_category"><?php _e('Default Category', 'themisdb-benchmark-visualizer'); ?></label>
                </th>
                <td>
                    <select name="themisdb_bv_default_category" id="themisdb_bv_default_category">
                        <option value="all" <?php selected(get_option('themisdb_bv_default_category'), 'all'); ?>>
                            <?php _e('All Operations', 'themisdb-benchmark-visualizer'); ?>
                        </option>
                        <option value="vector_search" <?php selected(get_option('themisdb_bv_default_category'), 'vector_search'); ?>>
                            <?php _e('Vector Search', 'themisdb-benchmark-visualizer'); ?>
                        </option>
                        <option value="aql_query" <?php selected(get_option('themisdb_bv_default_category'), 'aql_query'); ?>>
                            <?php _e('AQL Queries', 'themisdb-benchmark-visualizer'); ?>
                        </option>
                        <option value="graph_traversal" <?php selected(get_option('themisdb_bv_default_category'), 'graph_traversal'); ?>>
                            <?php _e('Graph Traversal', 'themisdb-benchmark-visualizer'); ?>
                        </option>
                    </select>
                    <p class="description">
                        <?php _e('Default category to display when the visualizer loads.', 'themisdb-benchmark-visualizer'); ?>
                    </p>
                </td>
            </tr>

            <!-- Default Metric -->
            <tr>
                <th scope="row">
                    <label for="themisdb_bv_default_metric"><?php _e('Default Metric', 'themisdb-benchmark-visualizer'); ?></label>
                </th>
                <td>
                    <select name="themisdb_bv_default_metric" id="themisdb_bv_default_metric">
                        <option value="latency" <?php selected(get_option('themisdb_bv_default_metric'), 'latency'); ?>>
                            <?php _e('Latency', 'themisdb-benchmark-visualizer'); ?>
                        </option>
                        <option value="throughput" <?php selected(get_option('themisdb_bv_default_metric'), 'throughput'); ?>>
                            <?php _e('Throughput', 'themisdb-benchmark-visualizer'); ?>
                        </option>
                        <option value="memory" <?php selected(get_option('themisdb_bv_default_metric'), 'memory'); ?>>
                            <?php _e('Memory Usage', 'themisdb-benchmark-visualizer'); ?>
                        </option>
                    </select>
                    <p class="description">
                        <?php _e('Default metric to display in charts.', 'themisdb-benchmark-visualizer'); ?>
                    </p>
                </td>
            </tr>

            <!-- Chart Theme -->
            <tr>
                <th scope="row">
                    <label for="themisdb_bv_chart_theme"><?php _e('Chart Theme', 'themisdb-benchmark-visualizer'); ?></label>
                </th>
                <td>
                    <select name="themisdb_bv_chart_theme" id="themisdb_bv_chart_theme">
                        <option value="light" <?php selected(get_option('themisdb_bv_chart_theme'), 'light'); ?>>
                            <?php _e('Light', 'themisdb-benchmark-visualizer'); ?>
                        </option>
                        <option value="dark" <?php selected(get_option('themisdb_bv_chart_theme'), 'dark'); ?>>
                            <?php _e('Dark', 'themisdb-benchmark-visualizer'); ?>
                        </option>
                    </select>
                    <p class="description">
                        <?php _e('Choose between light and dark theme for charts.', 'themisdb-benchmark-visualizer'); ?>
                    </p>
                </td>
            </tr>

            <!-- Auto Update Interval -->
            <tr>
                <th scope="row">
                    <label for="themisdb_bv_auto_update_interval"><?php _e('Cache Duration', 'themisdb-benchmark-visualizer'); ?></label>
                </th>
                <td>
                    <select name="themisdb_bv_auto_update_interval" id="themisdb_bv_auto_update_interval">
                        <option value="3600" <?php selected(get_option('themisdb_bv_auto_update_interval'), '3600'); ?>>
                            <?php _e('1 Hour', 'themisdb-benchmark-visualizer'); ?>
                        </option>
                        <option value="21600" <?php selected(get_option('themisdb_bv_auto_update_interval'), '21600'); ?>>
                            <?php _e('6 Hours', 'themisdb-benchmark-visualizer'); ?>
                        </option>
                        <option value="43200" <?php selected(get_option('themisdb_bv_auto_update_interval'), '43200'); ?>>
                            <?php _e('12 Hours', 'themisdb-benchmark-visualizer'); ?>
                        </option>
                        <option value="86400" <?php selected(get_option('themisdb_bv_auto_update_interval'), '86400'); ?>>
                            <?php _e('24 Hours', 'themisdb-benchmark-visualizer'); ?>
                        </option>
                        <option value="604800" <?php selected(get_option('themisdb_bv_auto_update_interval'), '604800'); ?>>
                            <?php _e('1 Week', 'themisdb-benchmark-visualizer'); ?>
                        </option>
                    </select>
                    <p class="description">
                        <?php _e('How long to cache benchmark data before refreshing.', 'themisdb-benchmark-visualizer'); ?>
                    </p>
                </td>
            </tr>
        </table>

        <?php submit_button(); ?>
    </form>

    <!-- Shortcode Usage -->
    <div class="themisdb-admin-section">
        <h2><?php _e('Shortcode Usage', 'themisdb-benchmark-visualizer'); ?></h2>
        <p><?php _e('Use the following shortcodes to embed the benchmark visualizer:', 'themisdb-benchmark-visualizer'); ?></p>
        
        <div class="themisdb-shortcode-examples">
            <h3><?php _e('Basic Usage', 'themisdb-benchmark-visualizer'); ?></h3>
            <code>[themisdb_benchmark_visualizer]</code>
            
            <h3><?php _e('With Category Filter', 'themisdb-benchmark-visualizer'); ?></h3>
            <code>[themisdb_benchmark_visualizer category="vector_search"]</code>
            
            <h3><?php _e('With Specific Metric', 'themisdb-benchmark-visualizer'); ?></h3>
            <code>[themisdb_benchmark_visualizer metric="throughput"]</code>
            
            <h3><?php _e('With Custom Comparison', 'themisdb-benchmark-visualizer'); ?></h3>
            <code>[themisdb_benchmark_visualizer compare="postgresql,mongodb"]</code>
            
            <h3><?php _e('With Chart Type', 'themisdb-benchmark-visualizer'); ?></h3>
            <code>[themisdb_benchmark_visualizer chart_type="line"]</code>
            
            <h3><?php _e('Combined Parameters', 'themisdb-benchmark-visualizer'); ?></h3>
            <code>[themisdb_benchmark_visualizer category="vector_search" metric="latency" chart_type="bar"]</code>
        </div>
    </div>

    <!-- Plugin Info -->
    <div class="themisdb-admin-section">
        <h2><?php _e('About', 'themisdb-benchmark-visualizer'); ?></h2>
        <p>
            <strong><?php _e('Version:', 'themisdb-benchmark-visualizer'); ?></strong> <?php echo THEMISDB_BV_VERSION; ?><br>
            <strong><?php _e('GitHub:', 'themisdb-benchmark-visualizer'); ?></strong> 
            <a href="https://github.com/makr-code/wordpressPlugins" target="_blank">makr-code/wordpressPlugins</a>
        </p>
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
    box-shadow: 0 1px 1px rgba(0,0,0,.04);
}

.themisdb-shortcode-examples {
    margin-top: 15px;
}

.themisdb-shortcode-examples h3 {
    margin-top: 20px;
    margin-bottom: 5px;
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
