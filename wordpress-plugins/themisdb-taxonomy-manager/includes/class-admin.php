<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-admin.php                                    ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:20                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     845                                            ║
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
 * Admin Panel for Taxonomy Manager
 * Provides configuration and management interface
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Taxonomy_Admin {
    
    /**
     * Constructor
     */
    public function __construct() {
        add_action('admin_menu', array($this, 'add_admin_menu'));
        add_action('admin_init', array($this, 'register_settings'));
        add_action('admin_enqueue_scripts', array($this, 'enqueue_admin_scripts'));
        
        // AJAX handlers
        add_action('wp_ajax_themisdb_consolidate_categories', array($this, 'ajax_consolidate'));
        add_action('wp_ajax_themisdb_get_recommendations', array($this, 'ajax_recommendations'));
        add_action('wp_ajax_themisdb_cleanup_unused', array($this, 'ajax_cleanup_unused'));
        add_action('wp_ajax_themisdb_merge_terms', array($this, 'ajax_merge_terms'));
    }
    
    /**
     * Add admin menu
     */
    public function add_admin_menu() {
        add_options_page(
            __('ThemisDB Taxonomy Manager', 'themisdb-taxonomy-manager'),
            __('Taxonomy Manager', 'themisdb-taxonomy-manager'),
            'manage_options',
            'themisdb-taxonomy-manager',
            array($this, 'admin_page')
        );
        
        // Add Analytics submenu
        add_menu_page(
            __('Taxonomy Analytics', 'themisdb-taxonomy-manager'),
            __('Taxonomy Analytics', 'themisdb-taxonomy-manager'),
            'manage_categories',
            'themisdb-taxonomy-analytics',
            array($this, 'analytics_page'),
            'dashicons-chart-bar',
            31
        );
    }
    
    /**
     * Register settings
     */
    public function register_settings() {
        register_setting('themisdb_taxonomy_settings', 'themisdb_taxonomy_auto_extract');
        register_setting('themisdb_taxonomy_settings', 'themisdb_taxonomy_auto_tags');
        register_setting('themisdb_taxonomy_settings', 'themisdb_taxonomy_auto_categories');
        register_setting('themisdb_taxonomy_settings', 'themisdb_taxonomy_max_categories');
        register_setting('themisdb_taxonomy_settings', 'themisdb_taxonomy_max_tags');
        register_setting('themisdb_taxonomy_settings', 'themisdb_taxonomy_min_tfidf_score');
        register_setting('themisdb_taxonomy_settings', 'themisdb_taxonomy_similarity_threshold');
        register_setting('themisdb_taxonomy_settings', 'themisdb_taxonomy_prefer_existing');
        register_setting('themisdb_taxonomy_settings', 'themisdb_taxonomy_max_category_depth');
        register_setting('themisdb_taxonomy_settings', 'themisdb_taxonomy_min_category_posts');
        register_setting('themisdb_taxonomy_settings', 'themisdb_taxonomy_consolidate_categories');
        register_setting('themisdb_taxonomy_settings', 'themisdb_taxonomy_show_in_rest');
        register_setting('themisdb_taxonomy_settings', 'themisdb_taxonomy_enable_seo_schema');
        register_setting('themisdb_taxonomy_settings', 'themisdb_taxonomy_default_icon');
        register_setting('themisdb_taxonomy_settings', 'themisdb_taxonomy_default_color');
        register_setting('themisdb_taxonomy_settings', 'themisdb_taxonomy_breadcrumb_separator');
        register_setting('themisdb_taxonomy_settings', 'themisdb_taxonomy_enable_custom_metabox');
    }
    
    /**
     * Enqueue admin scripts
     */
    public function enqueue_admin_scripts($hook) {
        // Enqueue for settings page
        if ($hook === 'settings_page_themisdb-taxonomy-manager') {
            wp_enqueue_style(
                'themisdb-taxonomy-admin',
                THEMISDB_TAXONOMY_PLUGIN_URL . 'assets/css/admin.css',
                array(),
                THEMISDB_TAXONOMY_VERSION
            );
            
            wp_enqueue_script(
                'themisdb-taxonomy-admin',
                THEMISDB_TAXONOMY_PLUGIN_URL . 'assets/js/admin.js',
                array('jquery'),
                THEMISDB_TAXONOMY_VERSION,
                true
            );
            
            wp_localize_script('themisdb-taxonomy-admin', 'themisdbTaxonomy', array(
                'ajaxurl' => admin_url('admin-ajax.php'),
                'nonce' => wp_create_nonce('themisdb_taxonomy_admin')
            ));
        }
        
        // Enqueue for analytics page
        if ($hook === 'toplevel_page_themisdb-taxonomy-analytics') {
            wp_enqueue_style(
                'themisdb-taxonomy-analytics',
                THEMISDB_TAXONOMY_PLUGIN_URL . 'assets/css/admin.css',
                array(),
                THEMISDB_TAXONOMY_VERSION
            );
            
            wp_enqueue_script(
                'themisdb-taxonomy-analytics',
                THEMISDB_TAXONOMY_PLUGIN_URL . 'assets/js/taxonomy-analytics.js',
                array('jquery'),
                THEMISDB_TAXONOMY_VERSION,
                true
            );
            
            wp_localize_script('themisdb-taxonomy-analytics', 'themisdbTaxonomy', array(
                'ajaxurl' => admin_url('admin-ajax.php'),
                'nonce' => wp_create_nonce('themisdb_taxonomy_admin'),
                'i18n' => array(
                    'confirmCleanup' => __('Delete all unused terms? This cannot be undone.', 'themisdb-taxonomy-manager'),
                    'confirmConsolidate' => __('Automatically merge similar categories? This cannot be undone.', 'themisdb-taxonomy-manager'),
                    'confirmMerge' => __('Merge these categories? This cannot be undone.', 'themisdb-taxonomy-manager'),
                    'processing' => __('Processing...', 'themisdb-taxonomy-manager'),
                    'merging' => __('Merging...', 'themisdb-taxonomy-manager'),
                    'cleanup' => __('Cleanup', 'themisdb-taxonomy-manager'),
                    'autoMerge' => __('Auto Merge', 'themisdb-taxonomy-manager'),
                    'merge' => __('Merge', 'themisdb-taxonomy-manager')
                )
            ));
        }
    }
    
    /**
     * Admin page
     */
    public function admin_page() {
        ?>
        <div class="wrap">
            <h1><?php _e('ThemisDB Taxonomy Manager', 'themisdb-taxonomy-manager'); ?></h1>
            
            <h2 class="nav-tab-wrapper">
                <a href="#settings" class="nav-tab nav-tab-active"><?php _e('Settings', 'themisdb-taxonomy-manager'); ?></a>
                <a href="#optimization" class="nav-tab"><?php _e('Optimization', 'themisdb-taxonomy-manager'); ?></a>
                <a href="#hierarchy" class="nav-tab"><?php _e('Category Hierarchy', 'themisdb-taxonomy-manager'); ?></a>
            </h2>
            
            <!-- Settings Tab -->
            <div id="settings" class="tab-content active">
                <form method="post" action="options.php">
                    <?php settings_fields('themisdb_taxonomy_settings'); ?>
                    
                    <table class="form-table">
                        <tr>
                            <th scope="row">
                                <label for="themisdb_taxonomy_auto_extract">
                                    <?php _e('Enable Auto-Extraction', 'themisdb-taxonomy-manager'); ?>
                                </label>
                            </th>
                            <td>
                                <input type="checkbox" 
                                       name="themisdb_taxonomy_auto_extract" 
                                       id="themisdb_taxonomy_auto_extract" 
                                       value="1" 
                                       <?php checked(1, get_option('themisdb_taxonomy_auto_extract', 1)); ?>>
                                <p class="description">
                                    <?php _e('Automatically extract and assign taxonomies when posts are saved', 'themisdb-taxonomy-manager'); ?>
                                </p>
                            </td>
                        </tr>
                        
                        <tr>
                            <th scope="row">
                                <label for="themisdb_taxonomy_auto_categories">
                                    <?php _e('Auto-Assign Categories', 'themisdb-taxonomy-manager'); ?>
                                </label>
                            </th>
                            <td>
                                <input type="checkbox" 
                                       name="themisdb_taxonomy_auto_categories" 
                                       id="themisdb_taxonomy_auto_categories" 
                                       value="1" 
                                       <?php checked(1, get_option('themisdb_taxonomy_auto_categories', 1)); ?>>
                                <p class="description">
                                    <?php _e('Automatically extract and assign categories with hierarchical structure', 'themisdb-taxonomy-manager'); ?>
                                </p>
                            </td>
                        </tr>
                        
                        <tr>
                            <th scope="row">
                                <label for="themisdb_taxonomy_auto_tags">
                                    <?php _e('Auto-Assign Tags', 'themisdb-taxonomy-manager'); ?>
                                </label>
                            </th>
                            <td>
                                <input type="checkbox" 
                                       name="themisdb_taxonomy_auto_tags" 
                                       id="themisdb_taxonomy_auto_tags" 
                                       value="1" 
                                       <?php checked(1, get_option('themisdb_taxonomy_auto_tags', 1)); ?>>
                                <p class="description">
                                    <?php _e('Automatically extract and assign tags from content', 'themisdb-taxonomy-manager'); ?>
                                </p>
                            </td>
                        </tr>
                        
                        <tr>
                            <th scope="row">
                                <label for="themisdb_taxonomy_max_categories">
                                    <?php _e('Max Categories per Post', 'themisdb-taxonomy-manager'); ?>
                                </label>
                            </th>
                            <td>
                                <input type="number" 
                                       name="themisdb_taxonomy_max_categories" 
                                       id="themisdb_taxonomy_max_categories" 
                                       value="<?php echo esc_attr(get_option('themisdb_taxonomy_max_categories', 5)); ?>" 
                                       min="1" 
                                       max="10" 
                                       class="small-text">
                                <p class="description">
                                    <?php _e('Maximum categories to assign per post (1-10, default: 5)', 'themisdb-taxonomy-manager'); ?>
                                </p>
                            </td>
                        </tr>
                        
                        <tr>
                            <th scope="row">
                                <label for="themisdb_taxonomy_max_tags">
                                    <?php _e('Max Tags per Post', 'themisdb-taxonomy-manager'); ?>
                                </label>
                            </th>
                            <td>
                                <input type="number" 
                                       name="themisdb_taxonomy_max_tags" 
                                       id="themisdb_taxonomy_max_tags" 
                                       value="<?php echo esc_attr(get_option('themisdb_taxonomy_max_tags', 10)); ?>" 
                                       min="1" 
                                       max="20" 
                                       class="small-text">
                                <p class="description">
                                    <?php _e('Maximum tags to assign per post (1-20, default: 10)', 'themisdb-taxonomy-manager'); ?>
                                </p>
                            </td>
                        </tr>
                        
                        <tr>
                            <th scope="row">
                                <label for="themisdb_taxonomy_min_tfidf_score">
                                    <?php _e('Minimum TF-IDF Score', 'themisdb-taxonomy-manager'); ?>
                                </label>
                            </th>
                            <td>
                                <input type="number" 
                                       name="themisdb_taxonomy_min_tfidf_score" 
                                       id="themisdb_taxonomy_min_tfidf_score" 
                                       value="<?php echo esc_attr(get_option('themisdb_taxonomy_min_tfidf_score', 0.5)); ?>" 
                                       min="0" 
                                       max="1" 
                                       step="0.1" 
                                       class="small-text">
                                <p class="description">
                                    <?php _e('Minimum relevance score for term extraction (0-1, default: 0.5)', 'themisdb-taxonomy-manager'); ?>
                                </p>
                            </td>
                        </tr>
                        
                        <tr>
                            <th scope="row">
                                <label for="themisdb_taxonomy_similarity_threshold">
                                    <?php _e('Similarity Threshold', 'themisdb-taxonomy-manager'); ?>
                                </label>
                            </th>
                            <td>
                                <input type="number" 
                                       name="themisdb_taxonomy_similarity_threshold" 
                                       id="themisdb_taxonomy_similarity_threshold" 
                                       value="<?php echo esc_attr(get_option('themisdb_taxonomy_similarity_threshold', 0.8)); ?>" 
                                       min="0" 
                                       max="1" 
                                       step="0.1" 
                                       class="small-text">
                                <p class="description">
                                    <?php _e('Similarity threshold for merge suggestions (0-1, default: 0.8)', 'themisdb-taxonomy-manager'); ?>
                                </p>
                            </td>
                        </tr>
                        
                        <tr>
                            <th scope="row">
                                <label for="themisdb_taxonomy_prefer_existing">
                                    <?php _e('Prefer Existing Terms', 'themisdb-taxonomy-manager'); ?>
                                </label>
                            </th>
                            <td>
                                <input type="checkbox" 
                                       name="themisdb_taxonomy_prefer_existing" 
                                       id="themisdb_taxonomy_prefer_existing" 
                                       value="1" 
                                       <?php checked(1, get_option('themisdb_taxonomy_prefer_existing', 1)); ?>>
                                <p class="description">
                                    <?php _e('Prefer using existing categories/tags over creating new ones', 'themisdb-taxonomy-manager'); ?>
                                </p>
                            </td>
                        </tr>
                        
                        <tr>
                            <th scope="row">
                                <label for="themisdb_taxonomy_max_category_depth">
                                    <?php _e('Maximum Category Depth', 'themisdb-taxonomy-manager'); ?>
                                </label>
                            </th>
                            <td>
                                <input type="number" 
                                       name="themisdb_taxonomy_max_category_depth" 
                                       id="themisdb_taxonomy_max_category_depth" 
                                       value="<?php echo esc_attr(get_option('themisdb_taxonomy_max_category_depth', 3)); ?>" 
                                       min="1" 
                                       max="5" 
                                       class="small-text">
                                <p class="description">
                                    <?php _e('Maximum depth for hierarchical categories (1-5, default: 3)', 'themisdb-taxonomy-manager'); ?>
                                </p>
                            </td>
                        </tr>
                        
                        <tr>
                            <th scope="row">
                                <label for="themisdb_taxonomy_consolidate_categories">
                                    <?php _e('Consolidate Categories', 'themisdb-taxonomy-manager'); ?>
                                </label>
                            </th>
                            <td>
                                <input type="checkbox" 
                                       name="themisdb_taxonomy_consolidate_categories" 
                                       id="themisdb_taxonomy_consolidate_categories" 
                                       value="1" 
                                       <?php checked(1, get_option('themisdb_taxonomy_consolidate_categories', 1)); ?>>
                                <p class="description">
                                    <?php _e('Automatically consolidate similar categories to minimize redundancy', 'themisdb-taxonomy-manager'); ?>
                                </p>
                            </td>
                        </tr>
                        
                        <tr>
                            <th scope="row">
                                <label for="themisdb_taxonomy_enable_custom_metabox">
                                    <?php _e('Enable Custom Meta Box', 'themisdb-taxonomy'); ?>
                                </label>
                            </th>
                            <td>
                                <input type="checkbox" 
                                       name="themisdb_taxonomy_enable_custom_metabox" 
                                       id="themisdb_taxonomy_enable_custom_metabox" 
                                       value="1" 
                                       <?php checked(1, get_option('themisdb_taxonomy_enable_custom_metabox', 1)); ?>>
                                <p class="description">
                                    <?php _e('Use enhanced meta box for post editing', 'themisdb-taxonomy'); ?>
                                </p>
                            </td>
                        </tr>
                        
                        <tr>
                            <th scope="row">
                                <label for="themisdb_taxonomy_default_icon">
                                    <?php _e('Default Icon', 'themisdb-taxonomy'); ?>
                                </label>
                            </th>
                            <td>
                                <input type="text" 
                                       name="themisdb_taxonomy_default_icon" 
                                       id="themisdb_taxonomy_default_icon" 
                                       value="<?php echo esc_attr(get_option('themisdb_taxonomy_default_icon', '📊')); ?>" 
                                       maxlength="2"
                                       class="small-text">
                                <p class="description">
                                    <?php _e('Default emoji icon for new terms', 'themisdb-taxonomy'); ?>
                                </p>
                            </td>
                        </tr>
                        
                        <tr>
                            <th scope="row">
                                <label for="themisdb_taxonomy_default_color">
                                    <?php _e('Default Color', 'themisdb-taxonomy'); ?>
                                </label>
                            </th>
                            <td>
                                <input type="color" 
                                       name="themisdb_taxonomy_default_color" 
                                       id="themisdb_taxonomy_default_color" 
                                       value="<?php echo esc_attr(get_option('themisdb_taxonomy_default_color', '#3498db')); ?>">
                                <p class="description">
                                    <?php _e('Default color for new terms', 'themisdb-taxonomy'); ?>
                                </p>
                            </td>
                        </tr>
                        
                        <tr>
                            <th scope="row">
                                <label for="themisdb_taxonomy_show_in_rest">
                                    <?php _e('Show in REST API', 'themisdb-taxonomy'); ?>
                                </label>
                            </th>
                            <td>
                                <input type="checkbox" 
                                       name="themisdb_taxonomy_show_in_rest" 
                                       id="themisdb_taxonomy_show_in_rest" 
                                       value="1" 
                                       <?php checked(1, get_option('themisdb_taxonomy_show_in_rest', 1)); ?>>
                                <p class="description">
                                    <?php _e('Make taxonomies available in REST API', 'themisdb-taxonomy'); ?>
                                </p>
                            </td>
                        </tr>
                        
                        <tr>
                            <th scope="row">
                                <label for="themisdb_taxonomy_enable_seo_schema">
                                    <?php _e('Enable SEO Schema', 'themisdb-taxonomy'); ?>
                                </label>
                            </th>
                            <td>
                                <input type="checkbox" 
                                       name="themisdb_taxonomy_enable_seo_schema" 
                                       id="themisdb_taxonomy_enable_seo_schema" 
                                       value="1" 
                                       <?php checked(1, get_option('themisdb_taxonomy_enable_seo_schema', 1)); ?>>
                                <p class="description">
                                    <?php _e('Add Schema.org markup to taxonomy pages', 'themisdb-taxonomy'); ?>
                                </p>
                            </td>
                        </tr>
                        
                        <tr>
                            <th scope="row">
                                <label for="themisdb_taxonomy_breadcrumb_separator">
                                    <?php _e('Breadcrumb Separator', 'themisdb-taxonomy'); ?>
                                </label>
                            </th>
                            <td>
                                <input type="text" 
                                       name="themisdb_taxonomy_breadcrumb_separator" 
                                       id="themisdb_taxonomy_breadcrumb_separator" 
                                       value="<?php echo esc_attr(get_option('themisdb_taxonomy_breadcrumb_separator', ' / ')); ?>" 
                                       class="small-text">
                                <p class="description">
                                    <?php _e('Separator for breadcrumb navigation (default: " / ")', 'themisdb-taxonomy'); ?>
                                </p>
                            </td>
                        </tr>
                    </table>
                    
                    <?php submit_button(); ?>
                </form>
            </div>
            
            <!-- Optimization Tab -->
            <div id="optimization" class="tab-content" style="display:none;">
                <h2><?php _e('Category Optimization', 'themisdb-taxonomy-manager'); ?></h2>
                <p><?php _e('Consolidate similar categories and optimize the category structure.', 'themisdb-taxonomy-manager'); ?></p>
                
                <button type="button" class="button button-primary" id="btn-consolidate">
                    <?php _e('Run Consolidation', 'themisdb-taxonomy-manager'); ?>
                </button>
                
                <button type="button" class="button" id="btn-get-recommendations">
                    <?php _e('Get Recommendations', 'themisdb-taxonomy-manager'); ?>
                </button>
                
                <div id="optimization-results" style="margin-top: 20px;"></div>
            </div>
            
            <!-- Hierarchy Tab -->
            <div id="hierarchy" class="tab-content" style="display:none;">
                <h2><?php _e('Category Hierarchy', 'themisdb-taxonomy-manager'); ?></h2>
                <p><?php _e('Current category structure with parent-child relationships:', 'themisdb-taxonomy-manager'); ?></p>
                
                <?php $this->display_category_hierarchy(); ?>
            </div>
        </div>
        
        <style>
            .tab-content { padding: 20px 0; }
            .category-tree { margin-left: 20px; }
            .category-tree li { list-style: none; margin: 5px 0; }
            .category-tree .parent { font-weight: bold; }
            .category-tree .child { margin-left: 30px; color: #666; }
            #optimization-results { background: #f5f5f5; padding: 15px; border-radius: 4px; }
        </style>
        
        <script>
        jQuery(document).ready(function($) {
            // Tab switching
            $('.nav-tab').on('click', function(e) {
                e.preventDefault();
                var target = $(this).attr('href');
                
                $('.nav-tab').removeClass('nav-tab-active');
                $(this).addClass('nav-tab-active');
                
                $('.tab-content').hide();
                $(target).show();
            });
            
            // Consolidation
            $('#btn-consolidate').on('click', function() {
                var $btn = $(this);
                $btn.prop('disabled', true).text('<?php _e('Processing...', 'themisdb-taxonomy-manager'); ?>');
                
                $.post(ajaxurl, {
                    action: 'themisdb_consolidate_categories',
                    nonce: themisdbTaxonomy.nonce
                }, function(response) {
                    $('#optimization-results').html('<h3>Results:</h3><pre>' + JSON.stringify(response.data, null, 2) + '</pre>');
                    $btn.prop('disabled', false).text('<?php _e('Run Consolidation', 'themisdb-taxonomy-manager'); ?>');
                });
            });
            
            // Recommendations
            $('#btn-get-recommendations').on('click', function() {
                var $btn = $(this);
                $btn.prop('disabled', true).text('<?php _e('Loading...', 'themisdb-taxonomy-manager'); ?>');
                
                $.post(ajaxurl, {
                    action: 'themisdb_get_recommendations',
                    nonce: themisdbTaxonomy.nonce
                }, function(response) {
                    var html = '<h3>Recommendations:</h3>';
                    if (response.data.length === 0) {
                        html += '<p>No recommendations. Your categories are optimized!</p>';
                    } else {
                        html += '<ul>';
                        response.data.forEach(function(rec) {
                            html += '<li><strong>' + rec.current_name + '</strong> (' + rec.post_count + ' posts):<ul>';
                            rec.actions.forEach(function(action) {
                                html += '<li>' + action.type + ': ' + action.target + ' - ' + action.reason + '</li>';
                            });
                            html += '</ul></li>';
                        });
                        html += '</ul>';
                    }
                    $('#optimization-results').html(html);
                    $btn.prop('disabled', false).text('<?php _e('Get Recommendations', 'themisdb-taxonomy-manager'); ?>');
                });
            });
        });
        </script>
        <?php
    }
    
    /**
     * Display category hierarchy
     */
    private function display_category_hierarchy() {
        $categories = get_categories(array(
            'hide_empty' => false,
            'orderby' => 'name',
            'order' => 'ASC'
        ));
        
        // Build hierarchy tree
        $tree = array();
        foreach ($categories as $cat) {
            if ($cat->parent == 0) {
                $tree[$cat->term_id] = array(
                    'cat' => $cat,
                    'children' => array()
                );
            }
        }
        
        // Add children
        foreach ($categories as $cat) {
            if ($cat->parent > 0 && isset($tree[$cat->parent])) {
                $tree[$cat->parent]['children'][] = $cat;
            }
        }
        
        echo '<ul class="category-tree">';
        foreach ($tree as $node) {
            echo '<li class="parent">';
            echo esc_html($node['cat']->name) . ' (' . $node['cat']->count . ')';
            
            if (!empty($node['children'])) {
                echo '<ul>';
                foreach ($node['children'] as $child) {
                    echo '<li class="child">' . esc_html($child->name) . ' (' . $child->count . ')</li>';
                }
                echo '</ul>';
            }
            
            echo '</li>';
        }
        echo '</ul>';
    }
    
    /**
     * AJAX: Consolidate categories
     */
    public function ajax_consolidate() {
        check_ajax_referer('themisdb_taxonomy_admin', 'nonce');
        
        if (!current_user_can('manage_categories')) {
            wp_send_json_error(array('message' => 'Unauthorized'));
        }
        
        $manager = themisdb_get_taxonomy_manager();
        $stats = $manager->consolidate_categories();
        
        wp_send_json_success($stats);
    }
    
    /**
     * AJAX: Get recommendations
     */
    public function ajax_recommendations() {
        check_ajax_referer('themisdb_taxonomy_admin', 'nonce');
        
        if (!current_user_can('manage_categories')) {
            wp_send_json_error(array('message' => 'Unauthorized'));
        }
        
        $manager = themisdb_get_taxonomy_manager();
        $recommendations = $manager->get_optimization_recommendations();
        
        wp_send_json_success($recommendations);
    }
    
    /**
     * AJAX: Cleanup unused terms
     */
    public function ajax_cleanup_unused() {
        check_ajax_referer('themisdb_taxonomy_admin', 'nonce');
        
        if (!current_user_can('manage_categories')) {
            wp_send_json_error(array('message' => 'Unauthorized'));
        }
        
        $analytics = new ThemisDB_Taxonomy_Analytics();
        $deleted_categories = $analytics->cleanup_unused_terms('category');
        $deleted_tags = $analytics->cleanup_unused_terms('post_tag');
        
        wp_send_json_success(array(
            'deleted_categories' => $deleted_categories,
            'deleted_tags' => $deleted_tags,
            'total_deleted' => $deleted_categories + $deleted_tags
        ));
    }
    
    /**
     * AJAX: Merge terms
     */
    public function ajax_merge_terms() {
        check_ajax_referer('themisdb_taxonomy_admin', 'nonce');
        
        if (!current_user_can('manage_categories')) {
            wp_send_json_error(array('message' => 'Unauthorized'));
        }
        
        $term1_id = isset($_POST['term1_id']) ? intval($_POST['term1_id']) : 0;
        $term2_id = isset($_POST['term2_id']) ? intval($_POST['term2_id']) : 0;
        
        if (!$term1_id || !$term2_id) {
            wp_send_json_error(array('message' => 'Invalid term IDs'));
        }
        
        // Get posts with term2
        $posts = get_posts(array(
            'category' => $term2_id,
            'posts_per_page' => -1,
            'fields' => 'ids',
            'post_status' => 'any'
        ));
        
        // Reassign to term1
        foreach ($posts as $post_id) {
            $current_cats = wp_get_post_categories($post_id);
            $current_cats[] = $term1_id;
            $current_cats = array_diff($current_cats, array($term2_id));
            wp_set_post_categories($post_id, $current_cats);
        }
        
        // Delete term2
        $result = wp_delete_term($term2_id, 'category');
        
        if (is_wp_error($result)) {
            wp_send_json_error(array('message' => $result->get_error_message()));
        }
        
        wp_send_json_success(array(
            'posts_moved' => count($posts),
            'message' => sprintf('Merged successfully. %d posts moved.', count($posts))
        ));
    }
    
    /**
     * Render analytics page
     */
    public function analytics_page() {
        $analytics = new ThemisDB_Taxonomy_Analytics();
        $stats = $analytics->get_taxonomy_statistics();
        $distribution = $analytics->get_category_distribution();
        
        ?>
        <div class="wrap">
            <h1><?php _e('ThemisDB Taxonomy Analytics', 'themisdb-taxonomy-manager'); ?></h1>
            
            <div class="taxonomy-stats" style="display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 20px; margin: 20px 0;">
                <div class="stat-box" style="background: #fff; padding: 20px; border-left: 4px solid #2271b1; box-shadow: 0 1px 3px rgba(0,0,0,0.1);">
                    <h3 style="margin: 0 0 10px 0; font-size: 14px; color: #666;"><?php _e('Total Categories', 'themisdb-taxonomy-manager'); ?></h3>
                    <span class="stat-number" style="font-size: 32px; font-weight: bold; color: #2271b1;"><?php echo esc_html($stats['total_categories']); ?></span>
                </div>
                
                <div class="stat-box" style="background: #fff; padding: 20px; border-left: 4px solid #00a32a; box-shadow: 0 1px 3px rgba(0,0,0,0.1);">
                    <h3 style="margin: 0 0 10px 0; font-size: 14px; color: #666;"><?php _e('Total Tags', 'themisdb-taxonomy-manager'); ?></h3>
                    <span class="stat-number" style="font-size: 32px; font-weight: bold; color: #00a32a;"><?php echo esc_html($stats['total_tags']); ?></span>
                </div>
                
                <div class="stat-box" style="background: #fff; padding: 20px; border-left: 4px solid #dba617; box-shadow: 0 1px 3px rgba(0,0,0,0.1);">
                    <h3 style="margin: 0 0 10px 0; font-size: 14px; color: #666;"><?php _e('Unused Terms', 'themisdb-taxonomy-manager'); ?></h3>
                    <span class="stat-number" style="font-size: 32px; font-weight: bold; color: #dba617;"><?php echo esc_html($stats['unused_terms']); ?></span>
                    <br>
                    <button class="button" id="btn-cleanup" style="margin-top: 10px;"><?php _e('Cleanup', 'themisdb-taxonomy-manager'); ?></button>
                </div>
                
                <div class="stat-box" style="background: #fff; padding: 20px; border-left: 4px solid #d63638; box-shadow: 0 1px 3px rgba(0,0,0,0.1);">
                    <h3 style="margin: 0 0 10px 0; font-size: 14px; color: #666;"><?php _e('Consolidation Suggestions', 'themisdb-taxonomy-manager'); ?></h3>
                    <span class="stat-number" style="font-size: 32px; font-weight: bold; color: #d63638;"><?php echo esc_html(count($stats['consolidation_suggestions'])); ?></span>
                    <br>
                    <button class="button button-primary" id="btn-auto-consolidate" style="margin-top: 10px;"><?php _e('Auto Merge', 'themisdb-taxonomy-manager'); ?></button>
                </div>
            </div>
            
            <h2><?php _e('Category Distribution', 'themisdb-taxonomy-manager'); ?></h2>
            <table class="widefat" style="margin-bottom: 20px;">
                <tbody>
                    <tr>
                        <td><strong><?php _e('Categories with Posts:', 'themisdb-taxonomy-manager'); ?></strong></td>
                        <td><?php echo esc_html($distribution['with_posts']); ?></td>
                    </tr>
                    <tr>
                        <td><strong><?php _e('Empty Categories:', 'themisdb-taxonomy-manager'); ?></strong></td>
                        <td><?php echo esc_html($distribution['empty']); ?></td>
                    </tr>
                    <tr>
                        <td><strong><?php _e('Top-Level Categories:', 'themisdb-taxonomy-manager'); ?></strong></td>
                        <td><?php echo esc_html($distribution['top_level']); ?></td>
                    </tr>
                    <tr>
                        <td><strong><?php _e('Categories with Children:', 'themisdb-taxonomy-manager'); ?></strong></td>
                        <td><?php echo esc_html($distribution['with_children']); ?></td>
                    </tr>
                    <tr>
                        <td><strong><?php _e('Average Posts per Category:', 'themisdb-taxonomy-manager'); ?></strong></td>
                        <td><?php echo esc_html($distribution['avg_posts_per_category']); ?></td>
                    </tr>
                </tbody>
            </table>
            
            <h2><?php _e('Consolidation Suggestions', 'themisdb-taxonomy-manager'); ?></h2>
            <?php if (empty($stats['consolidation_suggestions'])): ?>
                <p><?php _e('No consolidation suggestions. Your categories are well-organized!', 'themisdb-taxonomy-manager'); ?></p>
            <?php else: ?>
                <table class="wp-list-table widefat fixed striped">
                    <thead>
                        <tr>
                            <th><?php _e('Term 1', 'themisdb-taxonomy-manager'); ?></th>
                            <th><?php _e('Term 2', 'themisdb-taxonomy-manager'); ?></th>
                            <th><?php _e('Similarity', 'themisdb-taxonomy-manager'); ?></th>
                            <th><?php _e('Total Posts', 'themisdb-taxonomy-manager'); ?></th>
                            <th><?php _e('Action', 'themisdb-taxonomy-manager'); ?></th>
                        </tr>
                    </thead>
                    <tbody>
                        <?php foreach ($stats['consolidation_suggestions'] as $suggestion): ?>
                        <tr>
                            <td><?php echo esc_html($suggestion['term1']); ?></td>
                            <td><?php echo esc_html($suggestion['term2']); ?></td>
                            <td><?php echo round($suggestion['similarity'] * 100); ?>%</td>
                            <td><?php echo esc_html($suggestion['post_count']); ?></td>
                            <td>
                                <button class="button btn-merge" 
                                        data-term1="<?php echo esc_attr($suggestion['id1']); ?>"
                                        data-term2="<?php echo esc_attr($suggestion['id2']); ?>">
                                    <?php _e('Merge', 'themisdb-taxonomy-manager'); ?>
                                </button>
                            </td>
                        </tr>
                        <?php endforeach; ?>
                    </tbody>
                </table>
            <?php endif; ?>
            
            <div id="analytics-results" style="margin-top: 20px;"></div>
        </div>
        
        <style>
            #analytics-results {
                background: #f0f6fc;
                padding: 15px;
                border-radius: 4px;
                border: 1px solid #c3dafe;
                display: none;
            }
            #analytics-results.show {
                display: block;
            }
            #analytics-results.error {
                background: #fef2f2;
                border-color: #fecaca;
            }
            #analytics-results.success {
                background: #f0fdf4;
                border-color: #bbf7d0;
            }
        </style>
        <?php
    }
}
