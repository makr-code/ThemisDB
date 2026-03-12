<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-term-meta.php                                ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:21                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     167                                            ║
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
 * Term Meta Handler
 * Manages icon and color metadata for terms
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Term_Meta {
    
    /**
     * Constructor
     */
    public function __construct() {
        // Add custom fields to term edit form
        add_action('themisdb_feature_edit_form_fields', array($this, 'edit_form_fields'), 10, 2);
        add_action('themisdb_usecase_edit_form_fields', array($this, 'edit_form_fields'), 10, 2);
        add_action('themisdb_industry_edit_form_fields', array($this, 'edit_form_fields'), 10, 2);
        add_action('themisdb_techspec_edit_form_fields', array($this, 'edit_form_fields'), 10, 2);
        
        // Save term meta
        add_action('edited_themisdb_feature', array($this, 'save_term_meta'), 10, 2);
        add_action('edited_themisdb_usecase', array($this, 'save_term_meta'), 10, 2);
        add_action('edited_themisdb_industry', array($this, 'save_term_meta'), 10, 2);
        add_action('edited_themisdb_techspec', array($this, 'save_term_meta'), 10, 2);
        
        // Enqueue color picker
        add_action('admin_enqueue_scripts', array($this, 'enqueue_scripts'));
    }
    
    /**
     * Add custom fields to term edit form
     */
    public function edit_form_fields($term, $taxonomy) {
        $icon = get_term_meta($term->term_id, 'icon', true);
        $color = get_term_meta($term->term_id, 'color', true) ?: '#3498db';
        $extended_description = get_term_meta($term->term_id, 'extended_description', true);
        $featured = get_term_meta($term->term_id, 'featured', true);
        $term_order = get_term_meta($term->term_id, 'term_order', true) ?: 0;
        ?>
        <tr class="form-field">
            <th scope="row">
                <label for="term_icon"><?php _e('Icon', 'themisdb-taxonomy'); ?></label>
            </th>
            <td>
                <input type="text" name="term_icon" id="term_icon" value="<?php echo esc_attr($icon); ?>" 
                       class="regular-text" placeholder="📦 or fa fa-database">
                <p class="description"><?php _e('Enter emoji (📦) or Font Awesome class (fa fa-database)', 'themisdb-taxonomy'); ?></p>
            </td>
        </tr>
        
        <tr class="form-field">
            <th scope="row">
                <label for="term_color"><?php _e('Color', 'themisdb-taxonomy'); ?></label>
            </th>
            <td>
                <input type="text" name="term_color" id="term_color" value="<?php echo esc_attr($color); ?>" 
                       class="color-picker" data-default-color="#3498db">
                <p class="description"><?php _e('Choose a color for this term', 'themisdb-taxonomy'); ?></p>
            </td>
        </tr>
        
        <tr class="form-field">
            <th scope="row">
                <label for="extended_description"><?php _e('Extended Description', 'themisdb-taxonomy'); ?></label>
            </th>
            <td>
                <textarea name="extended_description" id="extended_description" rows="5" 
                          class="large-text"><?php echo esc_textarea($extended_description); ?></textarea>
                <p class="description"><?php _e('Long-form description for this term', 'themisdb-taxonomy'); ?></p>
            </td>
        </tr>
        
        <tr class="form-field">
            <th scope="row">
                <label for="featured"><?php _e('Featured', 'themisdb-taxonomy'); ?></label>
            </th>
            <td>
                <label>
                    <input type="checkbox" name="featured" id="featured" value="1" <?php checked($featured, 1); ?>>
                    <?php _e('Mark this term as featured', 'themisdb-taxonomy'); ?>
                </label>
            </td>
        </tr>
        
        <tr class="form-field">
            <th scope="row">
                <label for="term_order"><?php _e('Order', 'themisdb-taxonomy'); ?></label>
            </th>
            <td>
                <input type="number" name="term_order" id="term_order" value="<?php echo esc_attr($term_order); ?>" 
                       class="small-text" step="1">
                <p class="description"><?php _e('Manual sort order (lower numbers appear first)', 'themisdb-taxonomy'); ?></p>
            </td>
        </tr>
        <?php
    }
    
    /**
     * Save term meta
     */
    public function save_term_meta($term_id, $tt_id) {
        if (isset($_POST['term_icon'])) {
            update_term_meta($term_id, 'icon', sanitize_text_field($_POST['term_icon']));
        }
        
        if (isset($_POST['term_color'])) {
            update_term_meta($term_id, 'color', sanitize_hex_color($_POST['term_color']));
        }
        
        if (isset($_POST['extended_description'])) {
            update_term_meta($term_id, 'extended_description', wp_kses_post($_POST['extended_description']));
        }
        
        if (isset($_POST['featured'])) {
            update_term_meta($term_id, 'featured', 1);
        } else {
            delete_term_meta($term_id, 'featured');
        }
        
        if (isset($_POST['term_order'])) {
            update_term_meta($term_id, 'term_order', intval($_POST['term_order']));
        }
    }
    
    /**
     * Enqueue color picker scripts
     */
    public function enqueue_scripts($hook) {
        if ($hook === 'edit-tags.php' || $hook === 'term.php') {
            wp_enqueue_style('wp-color-picker');
            wp_enqueue_script('wp-color-picker');
            
            wp_enqueue_script(
                'themisdb-term-editor',
                THEMISDB_TAXONOMY_URL . 'assets/js/term-editor.js',
                array('jquery', 'wp-color-picker'),
                THEMISDB_TAXONOMY_VERSION,
                true
            );
        }
    }
}
