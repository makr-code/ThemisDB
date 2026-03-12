<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-metabox.php                                  ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:21                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     335                                            ║
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
 * Enhanced Meta Box
 * Custom meta box for taxonomy selection with grouped UI
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Enhanced_Metabox {
    
    /**
     * Constructor
     */
    public function __construct() {
        if (get_option('themisdb_taxonomy_enable_custom_metabox', 1)) {
            add_action('add_meta_boxes', array($this, 'add_meta_boxes'));
            add_action('save_post', array($this, 'save_meta_box'), 10, 2);
            add_action('admin_enqueue_scripts', array($this, 'enqueue_scripts'));
        }
    }
    
    /**
     * Add meta boxes
     */
    public function add_meta_boxes() {
        // Remove default meta boxes
        remove_meta_box('themisdb_featurediv', array('post', 'page'), 'side');
        remove_meta_box('themisdb_usecasediv', array('post', 'page'), 'side');
        remove_meta_box('themisdb_industrydiv', array('post', 'page'), 'side');
        remove_meta_box('themisdb_techspecdiv', array('post', 'page'), 'side');
        
        // Add custom meta box for features
        add_meta_box(
            'themisdb_feature_metabox',
            __('Database Features', 'themisdb-taxonomy'),
            array($this, 'render_feature_metabox'),
            array('post', 'page'),
            'normal',
            'high'
        );
        
        // Add custom meta box for use cases
        add_meta_box(
            'themisdb_usecase_metabox',
            __('Use Cases', 'themisdb-taxonomy'),
            array($this, 'render_simple_metabox'),
            array('post', 'page'),
            'normal',
            'default',
            array('taxonomy' => 'themisdb_usecase')
        );
        
        // Add custom meta box for industries
        add_meta_box(
            'themisdb_industry_metabox',
            __('Industries', 'themisdb-taxonomy'),
            array($this, 'render_simple_metabox'),
            array('post', 'page'),
            'normal',
            'default',
            array('taxonomy' => 'themisdb_industry')
        );
        
        // Keep tech specs as tag-style
        add_meta_box(
            'themisdb_techspec_metabox',
            __('Technical Specs', 'themisdb-taxonomy'),
            array($this, 'render_tag_metabox'),
            array('post', 'page'),
            'side',
            'default'
        );
    }
    
    /**
     * Enqueue scripts
     */
    public function enqueue_scripts($hook) {
        if ($hook === 'post.php' || $hook === 'post-new.php') {
            wp_enqueue_style(
                'themisdb-taxonomy-admin',
                THEMISDB_TAXONOMY_PLUGIN_URL . 'assets/css/taxonomy-admin.css',
                array(),
                THEMISDB_TAXONOMY_VERSION
            );
        }
    }
    
    /**
     * Render feature meta box with grouped display
     */
    public function render_feature_metabox($post) {
        wp_nonce_field('themisdb_feature_metabox', 'themisdb_feature_metabox_nonce');
        
        // Get current terms
        $current_terms = wp_get_post_terms($post->ID, 'themisdb_feature', array('fields' => 'ids'));
        if (is_wp_error($current_terms)) {
            $current_terms = array();
        }
        
        // Get parent terms (top level)
        $parent_terms = get_terms(array(
            'taxonomy' => 'themisdb_feature',
            'hide_empty' => false,
            'parent' => 0,
            'orderby' => 'name',
            'order' => 'ASC'
        ));
        
        if (empty($parent_terms) || is_wp_error($parent_terms)) {
            echo '<p>' . __('No features available.', 'themisdb-taxonomy') . '</p>';
            return;
        }
        
        echo '<div class="themisdb-feature-selector">';
        
        foreach ($parent_terms as $parent) {
            $icon = get_term_meta($parent->term_id, 'icon', true);
            $color = get_term_meta($parent->term_id, 'color', true);
            if (empty($color)) {
                $color = '#3498db';
            }
            
            // Get children
            $children = get_terms(array(
                'taxonomy' => 'themisdb_feature',
                'hide_empty' => false,
                'parent' => $parent->term_id,
                'orderby' => 'name',
                'order' => 'ASC'
            ));
            
            echo '<div class="feature-group">';
            echo '<h4 style="color: ' . esc_attr($color) . ';">';
            if (!empty($icon)) {
                echo '<span class="icon">' . esc_html($icon) . '</span> ';
            }
            echo esc_html($parent->name);
            echo '</h4>';
            
            echo '<div class="feature-items">';
            
            // Add checkbox for parent
            echo '<label>';
            echo '<input type="checkbox" name="tax_input[themisdb_feature][]" value="' . esc_attr($parent->term_id) . '" ';
            checked(in_array($parent->term_id, $current_terms));
            echo '> <strong>' . esc_html($parent->name) . '</strong>';
            echo '</label>';
            
            // Add checkboxes for children
            if (!empty($children) && !is_wp_error($children)) {
                foreach ($children as $child) {
                    echo '<label>';
                    echo '<input type="checkbox" name="tax_input[themisdb_feature][]" value="' . esc_attr($child->term_id) . '" ';
                    checked(in_array($child->term_id, $current_terms));
                    echo '> ' . esc_html($child->name);
                    echo '</label>';
                }
            }
            
            echo '</div>'; // feature-items
            echo '</div>'; // feature-group
        }
        
        echo '</div>'; // themisdb-feature-selector
    }
    
    /**
     * Render simple hierarchical meta box
     */
    public function render_simple_metabox($post, $metabox) {
        $taxonomy = $metabox['args']['taxonomy'];
        
        wp_nonce_field('themisdb_' . $taxonomy . '_metabox', 'themisdb_' . $taxonomy . '_metabox_nonce');
        
        $current_terms = wp_get_post_terms($post->ID, $taxonomy, array('fields' => 'ids'));
        if (is_wp_error($current_terms)) {
            $current_terms = array();
        }
        
        $terms = get_terms(array(
            'taxonomy' => $taxonomy,
            'hide_empty' => false,
            'orderby' => 'name',
            'order' => 'ASC'
        ));
        
        if (empty($terms) || is_wp_error($terms)) {
            echo '<p>' . __('No terms available.', 'themisdb-taxonomy') . '</p>';
            return;
        }
        
        echo '<div class="themisdb-simple-selector">';
        echo '<div class="feature-items">';
        
        foreach ($terms as $term) {
            echo '<label>';
            echo '<input type="checkbox" name="tax_input[' . esc_attr($taxonomy) . '][]" value="' . esc_attr($term->term_id) . '" ';
            checked(in_array($term->term_id, $current_terms));
            echo '> ' . esc_html($term->name);
            echo '</label>';
        }
        
        echo '</div>';
        echo '</div>';
    }
    
    /**
     * Render tag-style meta box for tech specs
     */
    public function render_tag_metabox($post) {
        wp_nonce_field('themisdb_techspec_metabox', 'themisdb_techspec_metabox_nonce');
        
        $current_terms = wp_get_post_terms($post->ID, 'themisdb_techspec');
        $current_names = array();
        if (!is_wp_error($current_terms)) {
            foreach ($current_terms as $term) {
                $current_names[] = $term->name;
            }
        }
        
        ?>
        <div class="tagsdiv" id="themisdb_techspec">
            <div class="jaxtag">
                <div class="nojs-tags hide-if-js">
                    <label for="tax-input-themisdb_techspec">
                        <?php _e('Add or remove tech specs', 'themisdb-taxonomy'); ?>
                    </label>
                    <textarea name="tax_input[themisdb_techspec]" rows="3" cols="25" class="the-tags" id="tax-input-themisdb_techspec"><?php echo esc_textarea(implode(',', $current_names)); ?></textarea>
                </div>
            </div>
            <div class="tagchecklist">
                <?php
                foreach ($current_terms as $term) {
                    echo '<span><a class="ntdelbutton" tabindex="0">X</a>&nbsp;' . esc_html($term->name) . '</span>';
                }
                ?>
            </div>
        </div>
        <p class="howto"><?php _e('Separate tech specs with commas', 'themisdb-taxonomy'); ?></p>
        <?php
        
        // Popular tech specs
        $popular = get_terms(array(
            'taxonomy' => 'themisdb_techspec',
            'orderby' => 'count',
            'order' => 'DESC',
            'number' => 10,
            'hide_empty' => false
        ));
        
        if (!empty($popular) && !is_wp_error($popular)) {
            echo '<p class="howto">' . __('Choose from popular:', 'themisdb-taxonomy') . '</p>';
            echo '<div class="tagcloud">';
            foreach ($popular as $term) {
                echo '<a href="#" class="tag-link" data-term="' . esc_attr($term->name) . '">' . esc_html($term->name) . '</a> ';
            }
            echo '</div>';
        }
    }
    
    /**
     * Save meta box data
     */
    public function save_meta_box($post_id, $post) {
        // Check if our nonces are set
        $taxonomies = array('themisdb_feature', 'themisdb_usecase', 'themisdb_industry', 'themisdb_techspec');
        
        foreach ($taxonomies as $taxonomy) {
            $nonce_name = 'themisdb_' . str_replace('themisdb_', '', $taxonomy) . '_metabox_nonce';
            
            if (!isset($_POST[$nonce_name])) {
                continue;
            }
            
            if (!wp_verify_nonce($_POST[$nonce_name], 'themisdb_' . str_replace('themisdb_', '', $taxonomy) . '_metabox')) {
                continue;
            }
            
            // Check permissions
            if (!current_user_can('edit_post', $post_id)) {
                continue;
            }
            
            // Save taxonomy terms
            if (isset($_POST['tax_input'][$taxonomy])) {
                $terms = $_POST['tax_input'][$taxonomy];
                
                if ($taxonomy === 'themisdb_techspec') {
                    // Handle comma-separated tags
                    if (is_string($terms)) {
                        $terms = explode(',', $terms);
                        $terms = array_map('trim', $terms);
                        $terms = array_filter($terms);
                    }
                } else {
                    // Handle checkbox arrays
                    $terms = array_map('intval', (array) $terms);
                }
                
                wp_set_post_terms($post_id, $terms, $taxonomy);
            } else {
                // Clear terms if none selected
                wp_set_post_terms($post_id, array(), $taxonomy);
            }
        }
    }
}

// Initialize
new ThemisDB_Enhanced_Metabox();
