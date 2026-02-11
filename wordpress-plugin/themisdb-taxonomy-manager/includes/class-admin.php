<?php
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
    }
    
    /**
     * Register settings
     */
    public function register_settings() {
        register_setting('themisdb_taxonomy_settings', 'themisdb_taxonomy_auto_extract');
        register_setting('themisdb_taxonomy_settings', 'themisdb_taxonomy_auto_tags');
        register_setting('themisdb_taxonomy_settings', 'themisdb_taxonomy_auto_categories');
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
        if ($hook !== 'settings_page_themisdb-taxonomy-manager') {
            return;
        }
        
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
}
