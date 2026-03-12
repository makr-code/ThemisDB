<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-custom-taxonomies.php                        ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:21                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     326                                            ║
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
 * Custom Taxonomies Registration
 * Registers 4 custom taxonomies: features, use-cases, industries, tech-specs
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Custom_Taxonomies {
    
    /**
     * Constructor
     */
    public function __construct() {
        add_action('init', array($this, 'register_taxonomies'));
        add_action('init', array($this, 'create_default_terms'), 15);
        add_action('create_term', array($this, 'save_term_meta'), 10, 3);
        add_action('edit_term', array($this, 'save_term_meta'), 10, 3);
        
        // Add term meta fields
        add_action('themisdb_feature_add_form_fields', array($this, 'add_term_meta_fields'));
        add_action('themisdb_feature_edit_form_fields', array($this, 'edit_term_meta_fields'), 10, 2);
    }
    
    /**
     * Register custom taxonomies
     */
    public function register_taxonomies() {
        // 1. Database Features (themisdb_feature) - Hierarchical with icon/color
        register_taxonomy('themisdb_feature', array('post', 'page'), array(
            'labels' => array(
                'name' => __('Database Features', 'themisdb-taxonomy'),
                'singular_name' => __('Feature', 'themisdb-taxonomy'),
                'search_items' => __('Search Features', 'themisdb-taxonomy'),
                'all_items' => __('All Features', 'themisdb-taxonomy'),
                'parent_item' => __('Parent Feature', 'themisdb-taxonomy'),
                'parent_item_colon' => __('Parent Feature:', 'themisdb-taxonomy'),
                'edit_item' => __('Edit Feature', 'themisdb-taxonomy'),
                'update_item' => __('Update Feature', 'themisdb-taxonomy'),
                'add_new_item' => __('Add New Feature', 'themisdb-taxonomy'),
                'new_item_name' => __('New Feature Name', 'themisdb-taxonomy'),
                'menu_name' => __('Features', 'themisdb-taxonomy'),
            ),
            'hierarchical' => true,
            'public' => true,
            'show_ui' => true,
            'show_admin_column' => true,
            'show_in_nav_menus' => true,
            'show_tagcloud' => true,
            'show_in_rest' => get_option('themisdb_taxonomy_show_in_rest', 1),
            'rewrite' => array('slug' => 'features', 'with_front' => false),
        ));
        
        // 2. Use Cases (themisdb_usecase) - Hierarchical
        register_taxonomy('themisdb_usecase', array('post', 'page'), array(
            'labels' => array(
                'name' => __('Use Cases', 'themisdb-taxonomy'),
                'singular_name' => __('Use Case', 'themisdb-taxonomy'),
                'search_items' => __('Search Use Cases', 'themisdb-taxonomy'),
                'all_items' => __('All Use Cases', 'themisdb-taxonomy'),
                'parent_item' => __('Parent Use Case', 'themisdb-taxonomy'),
                'parent_item_colon' => __('Parent Use Case:', 'themisdb-taxonomy'),
                'edit_item' => __('Edit Use Case', 'themisdb-taxonomy'),
                'update_item' => __('Update Use Case', 'themisdb-taxonomy'),
                'add_new_item' => __('Add New Use Case', 'themisdb-taxonomy'),
                'new_item_name' => __('New Use Case Name', 'themisdb-taxonomy'),
                'menu_name' => __('Use Cases', 'themisdb-taxonomy'),
            ),
            'hierarchical' => true,
            'public' => true,
            'show_ui' => true,
            'show_admin_column' => true,
            'show_in_nav_menus' => true,
            'show_tagcloud' => true,
            'show_in_rest' => get_option('themisdb_taxonomy_show_in_rest', 1),
            'rewrite' => array('slug' => 'use-cases', 'with_front' => false),
        ));
        
        // 3. Industries (themisdb_industry) - Hierarchical
        register_taxonomy('themisdb_industry', array('post', 'page'), array(
            'labels' => array(
                'name' => __('Industries', 'themisdb-taxonomy'),
                'singular_name' => __('Industry', 'themisdb-taxonomy'),
                'search_items' => __('Search Industries', 'themisdb-taxonomy'),
                'all_items' => __('All Industries', 'themisdb-taxonomy'),
                'parent_item' => __('Parent Industry', 'themisdb-taxonomy'),
                'parent_item_colon' => __('Parent Industry:', 'themisdb-taxonomy'),
                'edit_item' => __('Edit Industry', 'themisdb-taxonomy'),
                'update_item' => __('Update Industry', 'themisdb-taxonomy'),
                'add_new_item' => __('Add New Industry', 'themisdb-taxonomy'),
                'new_item_name' => __('New Industry Name', 'themisdb-taxonomy'),
                'menu_name' => __('Industries', 'themisdb-taxonomy'),
            ),
            'hierarchical' => true,
            'public' => true,
            'show_ui' => true,
            'show_admin_column' => true,
            'show_in_nav_menus' => true,
            'show_tagcloud' => true,
            'show_in_rest' => get_option('themisdb_taxonomy_show_in_rest', 1),
            'rewrite' => array('slug' => 'industries', 'with_front' => false),
        ));
        
        // 4. Technical Specs (themisdb_techspec) - Non-hierarchical (tags style)
        register_taxonomy('themisdb_techspec', array('post', 'page'), array(
            'labels' => array(
                'name' => __('Technical Specs', 'themisdb-taxonomy'),
                'singular_name' => __('Tech Spec', 'themisdb-taxonomy'),
                'search_items' => __('Search Tech Specs', 'themisdb-taxonomy'),
                'popular_items' => __('Popular Tech Specs', 'themisdb-taxonomy'),
                'all_items' => __('All Tech Specs', 'themisdb-taxonomy'),
                'edit_item' => __('Edit Tech Spec', 'themisdb-taxonomy'),
                'update_item' => __('Update Tech Spec', 'themisdb-taxonomy'),
                'add_new_item' => __('Add New Tech Spec', 'themisdb-taxonomy'),
                'new_item_name' => __('New Tech Spec Name', 'themisdb-taxonomy'),
                'menu_name' => __('Tech Specs', 'themisdb-taxonomy'),
            ),
            'hierarchical' => false,
            'public' => true,
            'show_ui' => true,
            'show_admin_column' => true,
            'show_in_nav_menus' => true,
            'show_tagcloud' => true,
            'show_in_rest' => get_option('themisdb_taxonomy_show_in_rest', 1),
            'rewrite' => array('slug' => 'tech-specs', 'with_front' => false),
        ));
    }
    
    /**
     * Create default terms
     */
    public function create_default_terms() {
        // Only create once
        if (get_option('themisdb_default_terms_created')) {
            return;
        }
        
        // Default Features with hierarchy
        $features = array(
            'Data Models' => array(
                'icon' => '📊',
                'color' => '#3498db',
                'children' => array('Relational SQL', 'Graph Database', 'Document Store', 'Vector Database', 'Time-Series', 'Key-Value Store')
            ),
            'AI/ML' => array(
                'icon' => '🤖',
                'color' => '#7c4dff',
                'children' => array('Embedded LLM', 'Vector Search', 'RAG Support', 'GPU Acceleration', 'Model Inference')
            ),
            'Performance' => array(
                'icon' => '⚡',
                'color' => '#f39c12',
                'children' => array('Horizontal Scaling', 'Auto-Sharding', 'Replication', 'Caching', 'Query Optimization')
            ),
            'Compatibility' => array(
                'icon' => '🔗',
                'color' => '#27ae60',
                'children' => array('SQL Protocol', 'MongoDB Protocol', 'Cypher (Graph)', 'REST API', 'GraphQL API', 'gRPC')
            ),
        );
        
        foreach ($features as $parent_name => $data) {
            $parent_term = wp_insert_term($parent_name, 'themisdb_feature');
            if (!is_wp_error($parent_term)) {
                update_term_meta($parent_term['term_id'], 'icon', $data['icon']);
                update_term_meta($parent_term['term_id'], 'color', $data['color']);
                
                // Create children
                foreach ($data['children'] as $child_name) {
                    $child_term = wp_insert_term($child_name, 'themisdb_feature', array(
                        'parent' => $parent_term['term_id']
                    ));
                }
            }
        }
        
        // Default Use Cases
        $usecases = array(
            'AI & Machine Learning',
            'Real-Time Analytics',
            'Graph Analytics',
            'IoT Data Management',
            'Content Management',
            'E-Commerce',
            'Social Networks',
            'Recommendation Systems',
            'Knowledge Graphs',
            'Semantic Search'
        );
        
        foreach ($usecases as $usecase) {
            wp_insert_term($usecase, 'themisdb_usecase');
        }
        
        // Default Industries
        $industries = array(
            'Healthcare',
            'Finance',
            'E-Commerce',
            'Telecommunications',
            'Manufacturing',
            'Education',
            'Government',
            'Media & Entertainment',
            'Transportation',
            'Energy'
        );
        
        foreach ($industries as $industry) {
            wp_insert_term($industry, 'themisdb_industry');
        }
        
        // Default Tech Specs (non-hierarchical)
        $techspecs = array(
            'ACID',
            'MVCC',
            'C++',
            'RocksDB',
            'llama.cpp',
            'CUDA',
            'OpenCL',
            'Docker',
            'Kubernetes',
            'High Availability',
            'Disaster Recovery'
        );
        
        foreach ($techspecs as $techspec) {
            wp_insert_term($techspec, 'themisdb_techspec');
        }
        
        update_option('themisdb_default_terms_created', 1);
    }
    
    /**
     * Add term meta fields (for new terms)
     */
    public function add_term_meta_fields($taxonomy) {
        ?>
        <div class="form-field">
            <label for="term_icon"><?php _e('Icon (Emoji)', 'themisdb-taxonomy'); ?></label>
            <input type="text" name="term_icon" id="term_icon" value="" maxlength="2">
            <p class="description"><?php _e('Enter an emoji icon (e.g., 📊, 🤖, ⚡)', 'themisdb-taxonomy'); ?></p>
        </div>
        
        <div class="form-field">
            <label for="term_color"><?php _e('Color', 'themisdb-taxonomy'); ?></label>
            <input type="color" name="term_color" id="term_color" value="<?php echo esc_attr(get_option('themisdb_taxonomy_default_color', '#3498db')); ?>">
            <p class="description"><?php _e('Choose a color for this term', 'themisdb-taxonomy'); ?></p>
        </div>
        <?php
    }
    
    /**
     * Edit term meta fields (for existing terms)
     */
    public function edit_term_meta_fields($term, $taxonomy) {
        $icon = get_term_meta($term->term_id, 'icon', true);
        $color = get_term_meta($term->term_id, 'color', true);
        if (empty($color)) {
            $color = get_option('themisdb_taxonomy_default_color', '#3498db');
        }
        ?>
        <tr class="form-field">
            <th scope="row">
                <label for="term_icon"><?php _e('Icon (Emoji)', 'themisdb-taxonomy'); ?></label>
            </th>
            <td>
                <input type="text" name="term_icon" id="term_icon" value="<?php echo esc_attr($icon); ?>" maxlength="2">
                <p class="description"><?php _e('Enter an emoji icon (e.g., 📊, 🤖, ⚡)', 'themisdb-taxonomy'); ?></p>
            </td>
        </tr>
        
        <tr class="form-field">
            <th scope="row">
                <label for="term_color"><?php _e('Color', 'themisdb-taxonomy'); ?></label>
            </th>
            <td>
                <input type="color" name="term_color" id="term_color" value="<?php echo esc_attr($color); ?>">
                <p class="description"><?php _e('Choose a color for this term', 'themisdb-taxonomy'); ?></p>
            </td>
        </tr>
        <?php
    }
    
    /**
     * Save term meta
     */
    public function save_term_meta($term_id, $tt_id, $taxonomy) {
        if ($taxonomy !== 'themisdb_feature') {
            return;
        }
        
        if (isset($_POST['term_icon'])) {
            update_term_meta($term_id, 'icon', sanitize_text_field($_POST['term_icon']));
        }
        
        if (isset($_POST['term_color'])) {
            update_term_meta($term_id, 'color', sanitize_hex_color($_POST['term_color']));
        }
    }
}
