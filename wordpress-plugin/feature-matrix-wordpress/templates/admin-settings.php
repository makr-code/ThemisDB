<?php
/**
 * Admin Settings Template for Feature Matrix
 */

if (!defined('ABSPATH')) {
    exit;
}
?>

<div class="wrap">
    <h1><?php echo esc_html(get_admin_page_title()); ?></h1>
    
    <div class="themisdb-admin-header">
        <p><?php _e('Configure the Feature Matrix plugin settings.', 'themisdb-feature-matrix'); ?></p>
    </div>

    <?php settings_errors('themisdb_fm_settings'); ?>

    <form method="post" action="options.php">
        <?php
        settings_fields('themisdb_fm_settings');
        do_settings_sections('themisdb_fm_settings');
        ?>

        <table class="form-table" role="presentation">
            <tr>
                <th scope="row">
                    <label for="themisdb_fm_default_category"><?php _e('Default Category', 'themisdb-feature-matrix'); ?></label>
                </th>
                <td>
                    <select name="themisdb_fm_default_category" id="themisdb_fm_default_category">
                        <option value="all" <?php selected(get_option('themisdb_fm_default_category'), 'all'); ?>>
                            <?php _e('All Features', 'themisdb-feature-matrix'); ?>
                        </option>
                        <option value="architecture" <?php selected(get_option('themisdb_fm_default_category'), 'architecture'); ?>>
                            <?php _e('Architecture', 'themisdb-feature-matrix'); ?>
                        </option>
                        <option value="ai_ml" <?php selected(get_option('themisdb_fm_default_category'), 'ai_ml'); ?>>
                            <?php _e('AI/ML', 'themisdb-feature-matrix'); ?>
                        </option>
                    </select>
                </td>
            </tr>

            <tr>
                <th scope="row">
                    <label for="themisdb_fm_default_comparison_dbs"><?php _e('Default Comparison Databases', 'themisdb-feature-matrix'); ?></label>
                </th>
                <td>
                    <input type="text" 
                           name="themisdb_fm_default_comparison_dbs" 
                           id="themisdb_fm_default_comparison_dbs" 
                           value="<?php echo esc_attr(get_option('themisdb_fm_default_comparison_dbs')); ?>" 
                           class="regular-text" />
                    <p class="description">
                        <?php _e('Comma-separated list (e.g., postgresql,mongodb,neo4j).', 'themisdb-feature-matrix'); ?>
                    </p>
                </td>
            </tr>

            <tr>
                <th scope="row">
                    <label for="themisdb_fm_show_mermaid_diagrams"><?php _e('Show Mermaid Diagrams', 'themisdb-feature-matrix'); ?></label>
                </th>
                <td>
                    <input type="checkbox" 
                           name="themisdb_fm_show_mermaid_diagrams" 
                           id="themisdb_fm_show_mermaid_diagrams" 
                           value="1" 
                           <?php checked(get_option('themisdb_fm_show_mermaid_diagrams'), 1); ?> />
                    <label for="themisdb_fm_show_mermaid_diagrams">
                        <?php _e('Enable feature hierarchy diagrams', 'themisdb-feature-matrix'); ?>
                    </label>
                </td>
            </tr>

            <tr>
                <th scope="row">
                    <label for="themisdb_fm_table_view"><?php _e('Table View', 'themisdb-feature-matrix'); ?></label>
                </th>
                <td>
                    <select name="themisdb_fm_table_view" id="themisdb_fm_table_view">
                        <option value="detailed" <?php selected(get_option('themisdb_fm_table_view'), 'detailed'); ?>>
                            <?php _e('Detailed', 'themisdb-feature-matrix'); ?>
                        </option>
                        <option value="compact" <?php selected(get_option('themisdb_fm_table_view'), 'compact'); ?>>
                            <?php _e('Compact', 'themisdb-feature-matrix'); ?>
                        </option>
                    </select>
                </td>
            </tr>

            <tr>
                <th scope="row">
                    <label for="themisdb_fm_enable_tooltips"><?php _e('Enable Tooltips', 'themisdb-feature-matrix'); ?></label>
                </th>
                <td>
                    <input type="checkbox" 
                           name="themisdb_fm_enable_tooltips" 
                           id="themisdb_fm_enable_tooltips" 
                           value="1" 
                           <?php checked(get_option('themisdb_fm_enable_tooltips'), 1); ?> />
                    <label for="themisdb_fm_enable_tooltips">
                        <?php _e('Show tooltips on hover', 'themisdb-feature-matrix'); ?>
                    </label>
                </td>
            </tr>
        </table>

        <?php submit_button(); ?>
    </form>

    <div class="themisdb-admin-section">
        <h2><?php _e('Shortcode Usage', 'themisdb-feature-matrix'); ?></h2>
        <div class="themisdb-shortcode-examples">
            <h3><?php _e('Basic Usage', 'themisdb-feature-matrix'); ?></h3>
            <code>[themisdb_feature_matrix]</code>
            
            <h3><?php _e('Filter by Category', 'themisdb-feature-matrix'); ?></h3>
            <code>[themisdb_feature_matrix category="ai_ml"]</code>
            
            <h3><?php _e('Compact View', 'themisdb-feature-matrix'); ?></h3>
            <code>[themisdb_feature_matrix view="compact"]</code>
            
            <h3><?php _e('Without Diagram', 'themisdb-feature-matrix'); ?></h3>
            <code>[themisdb_feature_matrix show_diagram="false"]</code>
        </div>
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
