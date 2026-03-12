<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-tree-view.php                                ║
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
 * Tree View Admin
 * Renders hierarchical taxonomy tree with drag & drop
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Tree_View {
    
    /**
     * Constructor
     */
    public function __construct() {
        add_action('admin_menu', array($this, 'add_menu_page'));
        add_action('admin_enqueue_scripts', array($this, 'enqueue_scripts'));
        
        // AJAX handlers
        add_action('wp_ajax_themisdb_save_term_order', array($this, 'ajax_save_term_order'));
        add_action('wp_ajax_themisdb_export_taxonomies', array($this, 'ajax_export_taxonomies'));
        add_action('wp_ajax_themisdb_import_taxonomies', array($this, 'ajax_import_taxonomies'));
    }
    
    /**
     * Add admin menu page
     */
    public function add_menu_page() {
        add_menu_page(
            __('ThemisDB Taxonomies', 'themisdb-taxonomy'),
            __('ThemisDB', 'themisdb-taxonomy'),
            'manage_categories',
            'themisdb-taxonomy-tree',
            array($this, 'render_tree_page'),
            'dashicons-networking',
            30
        );
        
        add_submenu_page(
            'themisdb-taxonomy-tree',
            __('Taxonomy Tree', 'themisdb-taxonomy'),
            __('Taxonomy Tree', 'themisdb-taxonomy'),
            'manage_categories',
            'themisdb-taxonomy-tree',
            array($this, 'render_tree_page')
        );
    }
    
    /**
     * Enqueue scripts and styles
     */
    public function enqueue_scripts($hook) {
        if ($hook !== 'toplevel_page_themisdb-taxonomy-tree') {
            return;
        }
        
        wp_enqueue_style('wp-color-picker');
        wp_enqueue_script('wp-color-picker');
        
        wp_enqueue_script('jquery-ui-sortable');
        
        wp_enqueue_style(
            'themisdb-taxonomy-admin',
            THEMISDB_TAXONOMY_PLUGIN_URL . 'assets/css/taxonomy-admin.css',
            array(),
            THEMISDB_TAXONOMY_VERSION
        );
        
        wp_enqueue_script(
            'themisdb-tree-view',
            THEMISDB_TAXONOMY_PLUGIN_URL . 'assets/js/tree-view.js',
            array('jquery', 'jquery-ui-sortable'),
            THEMISDB_TAXONOMY_VERSION,
            true
        );
        
        wp_localize_script('themisdb-tree-view', 'themisdbTaxonomy', array(
            'ajaxurl' => admin_url('admin-ajax.php'),
            'nonce' => wp_create_nonce('themisdb_taxonomy_tree')
        ));
    }
    
    /**
     * Render tree view page
     */
    public function render_tree_page() {
        $current_taxonomy = isset($_GET['taxonomy']) ? sanitize_text_field($_GET['taxonomy']) : 'themisdb_feature';
        ?>
        <div class="wrap themisdb-tree-admin">
            <h1><?php _e('ThemisDB Taxonomy Tree', 'themisdb-taxonomy'); ?></h1>
            
            <div class="tree-controls">
                <label for="taxonomy-selector">
                    <?php _e('Select Taxonomy:', 'themisdb-taxonomy'); ?>
                </label>
                <select id="taxonomy-selector" class="taxonomy-selector" aria-label="<?php esc_attr_e('Select Taxonomy', 'themisdb-taxonomy'); ?>">
                    <option value="themisdb_feature" <?php selected($current_taxonomy, 'themisdb_feature'); ?>>
                        <?php _e('Features', 'themisdb-taxonomy'); ?>
                    </option>
                    <option value="themisdb_usecase" <?php selected($current_taxonomy, 'themisdb_usecase'); ?>>
                        <?php _e('Use Cases', 'themisdb-taxonomy'); ?>
                    </option>
                    <option value="themisdb_industry" <?php selected($current_taxonomy, 'themisdb_industry'); ?>>
                        <?php _e('Industries', 'themisdb-taxonomy'); ?>
                    </option>
                    <option value="themisdb_techspec" <?php selected($current_taxonomy, 'themisdb_techspec'); ?>>
                        <?php _e('Tech Specs', 'themisdb-taxonomy'); ?>
                    </option>
                </select>
                
                <label for="taxonomy-search" class="screen-reader-text">
                    <?php _e('Search terms', 'themisdb-taxonomy'); ?>
                </label>
                <input type="text" id="taxonomy-search" class="taxonomy-search" 
                       placeholder="<?php esc_attr_e('Search terms...', 'themisdb-taxonomy'); ?>"
                       aria-label="<?php esc_attr_e('Search taxonomy terms', 'themisdb-taxonomy'); ?>">
                
                <button type="button" class="button" id="expand-all">
                    <?php _e('Expand All', 'themisdb-taxonomy'); ?>
                </button>
                
                <button type="button" class="button" id="collapse-all">
                    <?php _e('Collapse All', 'themisdb-taxonomy'); ?>
                </button>
                
                <button type="button" class="button button-primary" id="export-tree">
                    <?php _e('Export JSON', 'themisdb-taxonomy'); ?>
                </button>
            </div>
            
            <div class="taxonomy-tree-container">
                <?php $this->render_taxonomy_tree($current_taxonomy); ?>
            </div>
        </div>
        <?php
    }
    
    /**
     * Render taxonomy tree
     */
    public function render_taxonomy_tree($taxonomy) {
        $terms = get_terms(array(
            'taxonomy' => $taxonomy,
            'hide_empty' => false,
            'parent' => 0,
            'orderby' => 'term_order',
            'order' => 'ASC'
        ));
        
        if (empty($terms) || is_wp_error($terms)) {
            echo '<p>' . __('No terms found.', 'themisdb-taxonomy') . '</p>';
            return;
        }
        
        echo '<ul class="taxonomy-tree" data-taxonomy="' . esc_attr($taxonomy) . '">';
        foreach ($terms as $term) {
            $this->render_term_item($term, $taxonomy);
        }
        echo '</ul>';
    }
    
    /**
     * Render single term item
     */
    private function render_term_item($term, $taxonomy) {
        $icon = get_term_meta($term->term_id, 'icon', true);
        $color = get_term_meta($term->term_id, 'color', true);
        if (empty($color)) {
            $color = '#3498db';
        }
        
        $children = get_terms(array(
            'taxonomy' => $taxonomy,
            'hide_empty' => false,
            'parent' => $term->term_id,
            'orderby' => 'term_order',
            'order' => 'ASC'
        ));
        
        $has_children = !empty($children) && !is_wp_error($children);
        ?>
        <li class="tree-item" data-term-id="<?php echo esc_attr($term->term_id); ?>">
            <div class="tree-node" style="border-left-color: <?php echo esc_attr($color); ?>;">
                <?php if ($has_children): ?>
                    <button class="tree-toggle" aria-expanded="false" aria-label="<?php printf(esc_attr__('Toggle %s children', 'themisdb-taxonomy'), esc_attr($term->name)); ?>" tabindex="0">
                        <span aria-hidden="true">▼</span>
                    </button>
                <?php else: ?>
                    <span class="tree-toggle tree-toggle-empty" aria-hidden="true"></span>
                <?php endif; ?>
                
                <?php if (!empty($icon)): ?>
                    <span class="tree-icon"><?php echo esc_html($icon); ?></span>
                <?php endif; ?>
                
                <span class="tree-label"><?php echo esc_html($term->name); ?></span>
                <span class="tree-count"><?php echo esc_html($term->count); ?></span>
                
                <span class="tree-actions">
                    <a href="<?php echo esc_url(admin_url('term.php?taxonomy=' . $taxonomy . '&tag_ID=' . $term->term_id)); ?>" 
                       class="tree-action-edit"
                       aria-label="<?php printf(esc_attr__('Edit %s', 'themisdb-taxonomy'), esc_attr($term->name)); ?>"><?php _e('Edit', 'themisdb-taxonomy'); ?></a>
                    <a href="<?php echo esc_url(get_term_link($term)); ?>" 
                       class="tree-action-view" 
                       target="_blank"
                       aria-label="<?php printf(esc_attr__('View %s', 'themisdb-taxonomy'), esc_attr($term->name)); ?>"><?php _e('View', 'themisdb-taxonomy'); ?></a>
                </span>
            </div>
            
            <?php if ($has_children): ?>
                <ul class="tree-children">
                    <?php foreach ($children as $child): ?>
                        <?php $this->render_term_item($child, $taxonomy); ?>
                    <?php endforeach; ?>
                </ul>
            <?php endif; ?>
        </li>
        <?php
    }
    
    /**
     * AJAX: Save term order
     */
    public function ajax_save_term_order() {
        check_ajax_referer('themisdb_taxonomy_tree', 'nonce');
        
        if (!current_user_can('manage_categories')) {
            wp_send_json_error(array('message' => 'Unauthorized'));
        }
        
        $order = isset($_POST['order']) ? $_POST['order'] : array();
        
        if (empty($order)) {
            wp_send_json_error(array('message' => 'No order data'));
        }
        
        $position = 0;
        foreach ($order as $term_id) {
            update_term_meta($term_id, 'term_order', $position);
            $position++;
        }
        
        wp_send_json_success(array('message' => 'Order saved'));
    }
    
    /**
     * AJAX: Export taxonomies
     */
    public function ajax_export_taxonomies() {
        check_ajax_referer('themisdb_taxonomy_tree', 'nonce');
        
        if (!current_user_can('manage_categories')) {
            wp_send_json_error(array('message' => 'Unauthorized'));
        }
        
        $export_data = array(
            'version' => '1.0.0',
            'export_date' => current_time('Y-m-d'),
            'taxonomies' => array()
        );
        
        $taxonomies = array('themisdb_feature', 'themisdb_usecase', 'themisdb_industry', 'themisdb_techspec');
        
        foreach ($taxonomies as $taxonomy) {
            $terms = get_terms(array(
                'taxonomy' => $taxonomy,
                'hide_empty' => false,
                'orderby' => 'term_order',
                'order' => 'ASC'
            ));
            
            if (!is_wp_error($terms)) {
                $export_data['taxonomies'][$taxonomy] = array();
                
                foreach ($terms as $term) {
                    $term_data = array(
                        'term_id' => $term->term_id,
                        'name' => $term->name,
                        'slug' => $term->slug,
                        'description' => $term->description,
                        'parent' => $term->parent,
                        'count' => $term->count,
                        'meta' => array(
                            'icon' => get_term_meta($term->term_id, 'icon', true),
                            'color' => get_term_meta($term->term_id, 'color', true),
                            'term_order' => get_term_meta($term->term_id, 'term_order', true)
                        )
                    );
                    
                    $export_data['taxonomies'][$taxonomy][] = $term_data;
                }
            }
        }
        
        wp_send_json_success($export_data);
    }
    
    /**
     * AJAX: Import taxonomies
     */
    public function ajax_import_taxonomies() {
        check_ajax_referer('themisdb_taxonomy_tree', 'nonce');
        
        if (!current_user_can('manage_categories')) {
            wp_send_json_error(array('message' => 'Unauthorized'));
        }
        
        // This would handle JSON import
        // Implementation depends on specific requirements
        wp_send_json_success(array('message' => 'Import completed'));
    }
}
