<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-widget.php                                   ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:21                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     294                                            ║
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
 * Taxonomy Widget
 * Displays taxonomies in List, Cloud, or Grid format
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Taxonomy_Widget extends WP_Widget {
    
    /**
     * Constructor
     */
    public function __construct() {
        parent::__construct(
            'themisdb_taxonomy_widget',
            __('ThemisDB Taxonomy', 'themisdb-taxonomy'),
            array(
                'description' => __('Display custom taxonomies in various styles', 'themisdb-taxonomy')
            )
        );
        
        add_action('wp_enqueue_scripts', array($this, 'enqueue_widget_styles'));
    }
    
    /**
     * Enqueue widget styles
     */
    public function enqueue_widget_styles() {
        if (is_active_widget(false, false, $this->id_base)) {
            wp_enqueue_style(
                'themisdb-taxonomy-widget',
                THEMISDB_TAXONOMY_PLUGIN_URL . 'assets/css/taxonomy-widget.css',
                array(),
                THEMISDB_TAXONOMY_VERSION
            );
        }
    }
    
    /**
     * Widget form in admin
     */
    public function form($instance) {
        $title = isset($instance['title']) ? $instance['title'] : __('Taxonomies', 'themisdb-taxonomy');
        $taxonomy = isset($instance['taxonomy']) ? $instance['taxonomy'] : 'themisdb_feature';
        $style = isset($instance['style']) ? $instance['style'] : 'list';
        $show_count = isset($instance['show_count']) ? (bool) $instance['show_count'] : true;
        $parent_only = isset($instance['parent_only']) ? (bool) $instance['parent_only'] : false;
        ?>
        <p>
            <label for="<?php echo esc_attr($this->get_field_id('title')); ?>">
                <?php _e('Title:', 'themisdb-taxonomy'); ?>
            </label>
            <input class="widefat" 
                   id="<?php echo esc_attr($this->get_field_id('title')); ?>" 
                   name="<?php echo esc_attr($this->get_field_name('title')); ?>" 
                   type="text" 
                   value="<?php echo esc_attr($title); ?>">
        </p>
        
        <p>
            <label for="<?php echo esc_attr($this->get_field_id('taxonomy')); ?>">
                <?php _e('Taxonomy:', 'themisdb-taxonomy'); ?>
            </label>
            <select class="widefat" 
                    id="<?php echo esc_attr($this->get_field_id('taxonomy')); ?>" 
                    name="<?php echo esc_attr($this->get_field_name('taxonomy')); ?>">
                <option value="themisdb_feature" <?php selected($taxonomy, 'themisdb_feature'); ?>>
                    <?php _e('Features', 'themisdb-taxonomy'); ?>
                </option>
                <option value="themisdb_usecase" <?php selected($taxonomy, 'themisdb_usecase'); ?>>
                    <?php _e('Use Cases', 'themisdb-taxonomy'); ?>
                </option>
                <option value="themisdb_industry" <?php selected($taxonomy, 'themisdb_industry'); ?>>
                    <?php _e('Industries', 'themisdb-taxonomy'); ?>
                </option>
                <option value="themisdb_techspec" <?php selected($taxonomy, 'themisdb_techspec'); ?>>
                    <?php _e('Tech Specs', 'themisdb-taxonomy'); ?>
                </option>
            </select>
        </p>
        
        <p>
            <label for="<?php echo esc_attr($this->get_field_id('style')); ?>">
                <?php _e('Display Style:', 'themisdb-taxonomy'); ?>
            </label>
            <select class="widefat" 
                    id="<?php echo esc_attr($this->get_field_id('style')); ?>" 
                    name="<?php echo esc_attr($this->get_field_name('style')); ?>">
                <option value="list" <?php selected($style, 'list'); ?>><?php _e('List', 'themisdb-taxonomy'); ?></option>
                <option value="cloud" <?php selected($style, 'cloud'); ?>><?php _e('Cloud', 'themisdb-taxonomy'); ?></option>
                <option value="grid" <?php selected($style, 'grid'); ?>><?php _e('Grid', 'themisdb-taxonomy'); ?></option>
            </select>
        </p>
        
        <p>
            <input class="checkbox" 
                   type="checkbox" 
                   id="<?php echo esc_attr($this->get_field_id('show_count')); ?>" 
                   name="<?php echo esc_attr($this->get_field_name('show_count')); ?>" 
                   <?php checked($show_count); ?>>
            <label for="<?php echo esc_attr($this->get_field_id('show_count')); ?>">
                <?php _e('Show Count', 'themisdb-taxonomy'); ?>
            </label>
        </p>
        
        <p>
            <input class="checkbox" 
                   type="checkbox" 
                   id="<?php echo esc_attr($this->get_field_id('parent_only')); ?>" 
                   name="<?php echo esc_attr($this->get_field_name('parent_only')); ?>" 
                   <?php checked($parent_only); ?>>
            <label for="<?php echo esc_attr($this->get_field_id('parent_only')); ?>">
                <?php _e('Parent Terms Only', 'themisdb-taxonomy'); ?>
            </label>
        </p>
        <?php
    }
    
    /**
     * Update widget settings
     */
    public function update($new_instance, $old_instance) {
        $instance = array();
        $instance['title'] = (!empty($new_instance['title'])) ? sanitize_text_field($new_instance['title']) : '';
        $instance['taxonomy'] = (!empty($new_instance['taxonomy'])) ? sanitize_text_field($new_instance['taxonomy']) : 'themisdb_feature';
        $instance['style'] = (!empty($new_instance['style'])) ? sanitize_text_field($new_instance['style']) : 'list';
        $instance['show_count'] = isset($new_instance['show_count']) ? 1 : 0;
        $instance['parent_only'] = isset($new_instance['parent_only']) ? 1 : 0;
        return $instance;
    }
    
    /**
     * Display widget
     */
    public function widget($args, $instance) {
        $title = apply_filters('widget_title', isset($instance['title']) ? $instance['title'] : '');
        $taxonomy = isset($instance['taxonomy']) ? $instance['taxonomy'] : 'themisdb_feature';
        $style = isset($instance['style']) ? $instance['style'] : 'list';
        $show_count = isset($instance['show_count']) ? (bool) $instance['show_count'] : true;
        $parent_only = isset($instance['parent_only']) ? (bool) $instance['parent_only'] : false;
        
        echo $args['before_widget'];
        
        if (!empty($title)) {
            echo $args['before_title'] . $title . $args['after_title'];
        }
        
        $term_args = array(
            'taxonomy' => $taxonomy,
            'hide_empty' => false,
            'orderby' => 'name',
            'order' => 'ASC'
        );
        
        if ($parent_only) {
            $term_args['parent'] = 0;
        }
        
        $terms = get_terms($term_args);
        
        if (!empty($terms) && !is_wp_error($terms)) {
            switch ($style) {
                case 'cloud':
                    $this->render_cloud($terms, $show_count);
                    break;
                case 'grid':
                    $this->render_grid($terms, $show_count);
                    break;
                case 'list':
                default:
                    $this->render_list($terms, $show_count);
                    break;
            }
        }
        
        echo $args['after_widget'];
    }
    
    /**
     * Render list style
     */
    private function render_list($terms, $show_count) {
        echo '<ul class="themisdb-taxonomy-list">';
        foreach ($terms as $term) {
            $icon = get_term_meta($term->term_id, 'icon', true);
            echo '<li>';
            echo '<a href="' . esc_url(get_term_link($term)) . '">';
            if (!empty($icon)) {
                echo '<span class="icon">' . esc_html($icon) . '</span> ';
            }
            echo esc_html($term->name);
            if ($show_count) {
                echo ' <span class="count">(' . $term->count . ')</span>';
            }
            echo '</a>';
            echo '</li>';
        }
        echo '</ul>';
    }
    
    /**
     * Render cloud style
     */
    private function render_cloud($terms, $show_count) {
        // Calculate font sizes based on count
        $counts = wp_list_pluck($terms, 'count');
        $min_count = min($counts);
        $max_count = max($counts);
        $spread = $max_count - $min_count;
        if ($spread <= 0) {
            $spread = 1;
        }
        
        $min_size = 0.8;
        $max_size = 2;
        
        echo '<div class="themisdb-tag-cloud">';
        foreach ($terms as $term) {
            $color = get_term_meta($term->term_id, 'color', true);
            if (empty($color)) {
                $color = '#3498db';
            }
            
            $size = $min_size + (($term->count - $min_count) / $spread) * ($max_size - $min_size);
            
            echo '<a href="' . esc_url(get_term_link($term)) . '" ';
            echo 'style="font-size: ' . esc_attr($size) . 'em; color: ' . esc_attr($color) . ';">';
            echo esc_html($term->name);
            echo '</a> ';
        }
        echo '</div>';
    }
    
    /**
     * Render grid style
     */
    private function render_grid($terms, $show_count) {
        echo '<div class="themisdb-taxonomy-grid">';
        foreach ($terms as $term) {
            $icon = get_term_meta($term->term_id, 'icon', true);
            $color = get_term_meta($term->term_id, 'color', true);
            if (empty($color)) {
                $color = '#3498db';
            }
            
            echo '<div class="taxonomy-card" style="border-color: ' . esc_attr($color) . ';">';
            echo '<a href="' . esc_url(get_term_link($term)) . '">';
            if (!empty($icon)) {
                echo '<span class="card-icon">' . esc_html($icon) . '</span>';
            }
            echo '<h3>' . esc_html($term->name) . '</h3>';
            if ($show_count) {
                $taxonomy_obj = get_taxonomy($term->taxonomy);
                echo '<span class="card-count">' . $term->count . ' ' . strtolower($taxonomy_obj->labels->name) . '</span>';
            }
            echo '</a>';
            echo '</div>';
        }
        echo '</div>';
    }
}

/**
 * Register widget
 */
function themisdb_register_taxonomy_widget() {
    register_widget('ThemisDB_Taxonomy_Widget');
}
add_action('widgets_init', 'themisdb_register_taxonomy_widget');
