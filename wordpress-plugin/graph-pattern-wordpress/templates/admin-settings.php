<?php
/**
 * Admin Settings Template
 */

if (!defined('ABSPATH')) {
    exit;
}
?>

<div class="wrap">
    <h1><?php echo esc_html(get_admin_page_title()); ?></h1>

    <div class="themisdb-admin-container" style="max-width: 800px;">
        <div class="card" style="margin-top: 20px; padding: 20px;">
            <h2><?php _e('Default Settings', 'themisdb-graph-pattern'); ?></h2>
            <p><?php _e('Configure default settings for the Graph Pattern Visualizer. These can be overridden using shortcode attributes.', 'themisdb-graph-pattern'); ?></p>

            <form method="post" action="options.php">
                <?php
                settings_fields('themisdb_gp_settings');
                do_settings_sections('themisdb_gp_settings');
                ?>

                <table class="form-table" role="presentation">
                    <tbody>
                        <!-- Default Layout -->
                        <tr>
                            <th scope="row">
                                <label for="themisdb_gp_default_layout">
                                    <?php _e('Default Layout', 'themisdb-graph-pattern'); ?>
                                </label>
                            </th>
                            <td>
                                <select name="themisdb_gp_default_layout" id="themisdb_gp_default_layout" class="regular-text">
                                    <option value="force_directed" <?php selected(get_option('themisdb_gp_default_layout'), 'force_directed'); ?>>
                                        <?php _e('Force Directed', 'themisdb-graph-pattern'); ?>
                                    </option>
                                    <option value="hierarchical_top" <?php selected(get_option('themisdb_gp_default_layout'), 'hierarchical_top'); ?>>
                                        <?php _e('Hierarchical (Top-Down)', 'themisdb-graph-pattern'); ?>
                                    </option>
                                    <option value="hierarchical_left" <?php selected(get_option('themisdb_gp_default_layout'), 'hierarchical_left'); ?>>
                                        <?php _e('Hierarchical (Left-Right)', 'themisdb-graph-pattern'); ?>
                                    </option>
                                    <option value="circular" <?php selected(get_option('themisdb_gp_default_layout'), 'circular'); ?>>
                                        <?php _e('Circular', 'themisdb-graph-pattern'); ?>
                                    </option>
                                </select>
                                <p class="description">
                                    <?php _e('Choose the default graph layout algorithm.', 'themisdb-graph-pattern'); ?>
                                </p>
                            </td>
                        </tr>

                        <!-- Node Color Scheme -->
                        <tr>
                            <th scope="row">
                                <label for="themisdb_gp_node_color_scheme">
                                    <?php _e('Node Color Scheme', 'themisdb-graph-pattern'); ?>
                                </label>
                            </th>
                            <td>
                                <select name="themisdb_gp_node_color_scheme" id="themisdb_gp_node_color_scheme" class="regular-text">
                                    <option value="category" <?php selected(get_option('themisdb_gp_node_color_scheme'), 'category'); ?>>
                                        <?php _e('By Category/Group', 'themisdb-graph-pattern'); ?>
                                    </option>
                                    <option value="level" <?php selected(get_option('themisdb_gp_node_color_scheme'), 'level'); ?>>
                                        <?php _e('By Level', 'themisdb-graph-pattern'); ?>
                                    </option>
                                    <option value="connections" <?php selected(get_option('themisdb_gp_node_color_scheme'), 'connections'); ?>>
                                        <?php _e('By Connection Count', 'themisdb-graph-pattern'); ?>
                                    </option>
                                </select>
                                <p class="description">
                                    <?php _e('How node colors should be determined.', 'themisdb-graph-pattern'); ?>
                                </p>
                            </td>
                        </tr>

                        <!-- Enable Physics -->
                        <tr>
                            <th scope="row">
                                <?php _e('Physics Simulation', 'themisdb-graph-pattern'); ?>
                            </th>
                            <td>
                                <label>
                                    <input type="checkbox" 
                                           name="themisdb_gp_enable_physics" 
                                           value="1" 
                                           <?php checked(get_option('themisdb_gp_enable_physics'), 1); ?>>
                                    <?php _e('Enable physics-based layout by default', 'themisdb-graph-pattern'); ?>
                                </label>
                                <p class="description">
                                    <?php _e('Physics simulation creates natural-looking layouts but may impact performance on large graphs.', 'themisdb-graph-pattern'); ?>
                                </p>
                            </td>
                        </tr>

                        <!-- Show Labels -->
                        <tr>
                            <th scope="row">
                                <?php _e('Node Labels', 'themisdb-graph-pattern'); ?>
                            </th>
                            <td>
                                <label>
                                    <input type="checkbox" 
                                           name="themisdb_gp_show_labels" 
                                           value="1" 
                                           <?php checked(get_option('themisdb_gp_show_labels'), 1); ?>>
                                    <?php _e('Show node labels by default', 'themisdb-graph-pattern'); ?>
                                </label>
                                <p class="description">
                                    <?php _e('Display text labels on nodes. Can be toggled via the overlay panel.', 'themisdb-graph-pattern'); ?>
                                </p>
                            </td>
                        </tr>

                        <!-- Edge Smooth -->
                        <tr>
                            <th scope="row">
                                <?php _e('Smooth Edges', 'themisdb-graph-pattern'); ?>
                            </th>
                            <td>
                                <label>
                                    <input type="checkbox" 
                                           name="themisdb_gp_edge_smooth" 
                                           value="1" 
                                           <?php checked(get_option('themisdb_gp_edge_smooth'), 1); ?>>
                                    <?php _e('Use smooth curved edges', 'themisdb-graph-pattern'); ?>
                                </label>
                                <p class="description">
                                    <?php _e('Curved edges look better but may impact rendering performance.', 'themisdb-graph-pattern'); ?>
                                </p>
                            </td>
                        </tr>

                        <!-- Max Nodes -->
                        <tr>
                            <th scope="row">
                                <label for="themisdb_gp_max_nodes">
                                    <?php _e('Maximum Nodes', 'themisdb-graph-pattern'); ?>
                                </label>
                            </th>
                            <td>
                                <input type="number" 
                                       name="themisdb_gp_max_nodes" 
                                       id="themisdb_gp_max_nodes" 
                                       value="<?php echo esc_attr(get_option('themisdb_gp_max_nodes', 500)); ?>"
                                       min="50"
                                       max="5000"
                                       step="50"
                                       class="small-text">
                                <p class="description">
                                    <?php _e('Maximum number of nodes to display. Higher values may impact performance.', 'themisdb-graph-pattern'); ?>
                                </p>
                            </td>
                        </tr>

                        <!-- Enable Search -->
                        <tr>
                            <th scope="row">
                                <?php _e('Search Feature', 'themisdb-graph-pattern'); ?>
                            </th>
                            <td>
                                <label>
                                    <input type="checkbox" 
                                           name="themisdb_gp_enable_search" 
                                           value="1" 
                                           <?php checked(get_option('themisdb_gp_enable_search'), 1); ?>>
                                    <?php _e('Enable node search in overlay panel', 'themisdb-graph-pattern'); ?>
                                </label>
                            </td>
                        </tr>

                        <!-- Enable Filters -->
                        <tr>
                            <th scope="row">
                                <?php _e('Filter Feature', 'themisdb-graph-pattern'); ?>
                            </th>
                            <td>
                                <label>
                                    <input type="checkbox" 
                                           name="themisdb_gp_enable_filters" 
                                           value="1" 
                                           <?php checked(get_option('themisdb_gp_enable_filters'), 1); ?>>
                                    <?php _e('Enable group filters in overlay panel', 'themisdb-graph-pattern'); ?>
                                </label>
                            </td>
                        </tr>

                        <!-- Enable Export -->
                        <tr>
                            <th scope="row">
                                <?php _e('Export Feature', 'themisdb-graph-pattern'); ?>
                            </th>
                            <td>
                                <label>
                                    <input type="checkbox" 
                                           name="themisdb_gp_enable_export" 
                                           value="1" 
                                           <?php checked(get_option('themisdb_gp_enable_export'), 1); ?>>
                                    <?php _e('Enable graph export (PNG, JSON)', 'themisdb-graph-pattern'); ?>
                                </label>
                            </td>
                        </tr>
                    </tbody>
                </table>

                <?php submit_button(__('Save Settings', 'themisdb-graph-pattern')); ?>
            </form>
        </div>

        <!-- Usage Instructions -->
        <div class="card" style="margin-top: 20px; padding: 20px;">
            <h2><?php _e('Usage Instructions', 'themisdb-graph-pattern'); ?></h2>
            
            <h3><?php _e('Basic Shortcode', 'themisdb-graph-pattern'); ?></h3>
            <pre style="background: #f6f8fa; padding: 12px; border-radius: 6px; overflow-x: auto;">[themisdb_graph]</pre>

            <h3><?php _e('Shortcode with Parameters', 'themisdb-graph-pattern'); ?></h3>
            <pre style="background: #f6f8fa; padding: 12px; border-radius: 6px; overflow-x: auto;">[themisdb_graph data_source="default" layout="force_directed" height="600px" show_controls="true" show_overlay="true"]</pre>

            <h3><?php _e('Available Parameters', 'themisdb-graph-pattern'); ?></h3>
            <ul style="list-style: disc; padding-left: 20px; line-height: 1.8;">
                <li><code>data_source</code> - Data source identifier (default: "default")</li>
                <li><code>layout</code> - Layout algorithm: force_directed, hierarchical_top, hierarchical_left, circular</li>
                <li><code>height</code> - Graph container height (default: "600px")</li>
                <li><code>show_controls</code> - Show control bar (default: "true")</li>
                <li><code>show_overlay</code> - Show options overlay panel (default: "true")</li>
            </ul>

            <h3><?php _e('Features', 'themisdb-graph-pattern'); ?></h3>
            <ul style="list-style: disc; padding-left: 20px; line-height: 1.8;">
                <li>🔍 <strong><?php _e('Search', 'themisdb-graph-pattern'); ?>:</strong> <?php _e('Find nodes by name with highlighting', 'themisdb-graph-pattern'); ?></li>
                <li>🎨 <strong><?php _e('Color Customization', 'themisdb-graph-pattern'); ?>:</strong> <?php _e('Change node group colors dynamically', 'themisdb-graph-pattern'); ?></li>
                <li>👁️ <strong><?php _e('Show/Hide Groups', 'themisdb-graph-pattern'); ?>:</strong> <?php _e('Toggle visibility of node categories', 'themisdb-graph-pattern'); ?></li>
                <li>🎛️ <strong><?php _e('Layout Controls', 'themisdb-graph-pattern'); ?>:</strong> <?php _e('Adjust physics, spacing, and edge strength', 'themisdb-graph-pattern'); ?></li>
                <li>📥 <strong><?php _e('Export', 'themisdb-graph-pattern'); ?>:</strong> <?php _e('Download as PNG or JSON', 'themisdb-graph-pattern'); ?></li>
                <li>📱 <strong><?php _e('Responsive', 'themisdb-graph-pattern'); ?>:</strong> <?php _e('Works on mobile and desktop', 'themisdb-graph-pattern'); ?></li>
                <li>⌨️ <strong><?php _e('Keyboard Controls', 'themisdb-graph-pattern'); ?>:</strong> <?php _e('Navigate with arrow keys', 'themisdb-graph-pattern'); ?></li>
            </ul>

            <h3><?php _e('Inspiration', 'themisdb-graph-pattern'); ?></h3>
            <p>
                <?php _e('This plugin is inspired by Neo4j Bloom\'s intuitive graph exploration interface, bringing similar capabilities to ThemisDB architecture visualization on WordPress.', 'themisdb-graph-pattern'); ?>
            </p>
        </div>

        <!-- Plugin Info -->
        <div class="card" style="margin-top: 20px; padding: 20px;">
            <h2><?php _e('About', 'themisdb-graph-pattern'); ?></h2>
            <p>
                <strong><?php _e('Version:', 'themisdb-graph-pattern'); ?></strong> <?php echo THEMISDB_GP_VERSION; ?><br>
                <strong><?php _e('Plugin:', 'themisdb-graph-pattern'); ?></strong> ThemisDB Graph Pattern Visualizer<br>
                <strong><?php _e('Repository:', 'themisdb-graph-pattern'); ?></strong> 
                <a href="https://github.com/<?php echo THEMISDB_GP_GITHUB_REPO; ?>" target="_blank">
                    <?php echo THEMISDB_GP_GITHUB_REPO; ?>
                </a>
            </p>
        </div>
    </div>
</div>

<style>
.themisdb-admin-container pre {
    font-size: 13px;
    line-height: 1.6;
}
.themisdb-admin-container code {
    background: #f6f8fa;
    padding: 2px 6px;
    border-radius: 3px;
    font-family: 'Courier New', Courier, monospace;
    font-size: 13px;
}
.themisdb-admin-container h3 {
    margin-top: 24px;
    margin-bottom: 12px;
}
</style>
