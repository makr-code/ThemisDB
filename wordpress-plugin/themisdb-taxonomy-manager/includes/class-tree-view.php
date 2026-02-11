<?php
/**
 * Tree View Admin
 * Provides visual tree interface for managing taxonomies
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Taxonomy_Tree_View {
    
    /**
     * Constructor
     */
    public function __construct() {
        add_action('admin_menu', array($this, 'add_admin_menu'));
        add_action('admin_enqueue_scripts', array($this, 'enqueue_scripts'));
        
        // AJAX handlers
        add_action('wp_ajax_themisdb_save_term_order', array($this, 'ajax_save_order'));
    }
    
    /**
     * Add admin menu
     */
    public function add_admin_menu() {
        add_management_page(
            __('Taxonomy Tree', 'themisdb-taxonomy'),
            __('Taxonomy Tree', 'themisdb-taxonomy'),
            'manage_categories',
            'themisdb-taxonomy-tree',
            array($this, 'render_page')
        );
    }
    
    /**
     * Enqueue scripts and styles
     */
    public function enqueue_scripts($hook) {
        if ($hook !== 'tools_page_themisdb-taxonomy-tree') {
            return;
        }
        
        wp_enqueue_style('themisdb-tree-view', THEMISDB_TAXONOMY_URL . 'assets/css/tree-view.css', array(), THEMISDB_TAXONOMY_VERSION);
        
        wp_enqueue_script('jquery-ui-sortable');
        wp_enqueue_script('themisdb-tree-view', THEMISDB_TAXONOMY_URL . 'assets/js/tree-view.js', array('jquery', 'jquery-ui-sortable'), THEMISDB_TAXONOMY_VERSION, true);
        
        wp_localize_script('themisdb-tree-view', 'themisdbTaxonomy', array(
            'ajaxurl' => admin_url('admin-ajax.php'),
            'nonce' => wp_create_nonce('themisdb_taxonomy_tree')
        ));
    }
    
    /**
     * Render admin page
     */
    public function render_page() {
        $current_tax = isset($_GET['taxonomy']) ? sanitize_key($_GET['taxonomy']) : 'themisdb_feature';
        
        $taxonomies = array(
            'themisdb_feature' => __('Database Features', 'themisdb-taxonomy'),
            'themisdb_usecase' => __('Use Cases', 'themisdb-taxonomy'),
            'themisdb_industry' => __('Industries', 'themisdb-taxonomy'),
            'themisdb_techspec' => __('Technical Specs', 'themisdb-taxonomy')
        );
        ?>
        <div class="wrap">
            <h1><?php _e('Taxonomy Tree View', 'themisdb-taxonomy'); ?></h1>
            
            <div class="taxonomy-tabs">
                <?php foreach ($taxonomies as $tax => $label): ?>
                    <a href="?page=themisdb-taxonomy-tree&taxonomy=<?php echo $tax; ?>" 
                       class="nav-tab <?php echo $current_tax === $tax ? 'nav-tab-active' : ''; ?>">
                        <?php echo esc_html($label); ?>
                    </a>
                <?php endforeach; ?>
            </div>
            
            <div class="tree-actions">
                <button class="button" id="expand-all"><?php _e('Expand All', 'themisdb-taxonomy'); ?></button>
                <button class="button" id="collapse-all"><?php _e('Collapse All', 'themisdb-taxonomy'); ?></button>
            </div>
            
            <div class="taxonomy-tree-container">
                <?php $this->render_tree($current_tax); ?>
            </div>
        </div>
        <?php
    }
    
    /**
     * Render taxonomy tree
     */
    private function render_tree($taxonomy) {
        $terms = get_terms(array(
            'taxonomy' => $taxonomy,
            'hide_empty' => false,
            'orderby' => 'term_order',
            'meta_key' => 'term_order',
            'order' => 'ASC'
        ));
        
        if (is_wp_error($terms) || empty($terms)) {
            echo '<p>' . __('No terms found.', 'themisdb-taxonomy') . '</p>';
            return;
        }
        
        // Build hierarchical tree
        $tree = $this->build_tree($terms);
        
        echo '<ul class="taxonomy-tree" data-taxonomy="' . esc_attr($taxonomy) . '">';
        $this->render_tree_items($tree, $taxonomy);
        echo '</ul>';
    }
    
    /**
     * Build hierarchical tree from flat list
     */
    private function build_tree($terms, $parent = 0) {
        $branch = array();
        
        foreach ($terms as $term) {
            if ($term->parent == $parent) {
                $children = $this->build_tree($terms, $term->term_id);
                if ($children) {
                    $term->children = $children;
                }
                $branch[] = $term;
            }
        }
        
        return $branch;
    }
    
    /**
     * Render tree items recursively
     */
    private function render_tree_items($terms, $taxonomy) {
        foreach ($terms as $term) {
            $icon = get_term_meta($term->term_id, 'icon', true) ?: '📦';
            $color = get_term_meta($term->term_id, 'color', true);
            $has_children = isset($term->children) && !empty($term->children);
            
            $style = $color ? 'style="color: ' . esc_attr($color) . ';"' : '';
            
            echo '<li class="tree-item" data-term-id="' . $term->term_id . '">';
            echo '<div class="tree-item-content">';
            
            if ($has_children) {
                echo '<span class="tree-toggle">▼</span>';
            } else {
                echo '<span class="tree-toggle-placeholder"></span>';
            }
            
            echo '<span class="term-handle">☰</span>';
            echo '<span class="term-icon" ' . $style . '>' . esc_html($icon) . '</span> ';
            echo '<span class="term-name">' . esc_html($term->name) . '</span>';
            echo ' <span class="term-count">[' . $term->count . ' posts]</span>';
            
            echo '<span class="term-actions">';
            echo '<a href="' . get_edit_term_link($term->term_id, $taxonomy) . '" class="edit-term">' . __('Edit', 'themisdb-taxonomy') . '</a> | ';
            echo '<a href="' . get_term_link($term) . '" class="view-term" target="_blank">' . __('View', 'themisdb-taxonomy') . '</a>';
            echo '</span>';
            
            echo '</div>';
            
            if ($has_children) {
                echo '<ul class="tree-children">';
                $this->render_tree_items($term->children, $taxonomy);
                echo '</ul>';
            }
            
            echo '</li>';
        }
    }
    
    /**
     * AJAX: Save term order
     */
    public function ajax_save_order() {
        check_ajax_referer('themisdb_taxonomy_tree', 'nonce');
        
        if (!current_user_can('manage_categories')) {
            wp_send_json_error('Permission denied');
        }
        
        $order = isset($_POST['order']) ? $_POST['order'] : array();
        $taxonomy = isset($_POST['taxonomy']) ? sanitize_key($_POST['taxonomy']) : '';
        
        if (empty($order) || empty($taxonomy)) {
            wp_send_json_error('Invalid data');
        }
        
        $position = 0;
        foreach ($order as $term_id) {
            update_term_meta(intval($term_id), 'term_order', $position);
            $position++;
        }
        
        wp_send_json_success(array('message' => 'Order saved successfully'));
    }
}
