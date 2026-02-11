<?php
/**
 * Taxonomy Widget
 * Display taxonomies with icons in various styles
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
            array('description' => __('Display taxonomies with icons', 'themisdb-taxonomy'))
        );
    }
    
    /**
     * Front-end display of widget
     */
    public function widget($args, $instance) {
        echo $args['before_widget'];
        
        if (!empty($instance['title'])) {
            echo $args['before_title'] . apply_filters('widget_title', $instance['title']) . $args['after_title'];
        }
        
        $taxonomy = !empty($instance['taxonomy']) ? $instance['taxonomy'] : 'themisdb_feature';
        $style = !empty($instance['style']) ? $instance['style'] : 'list';
        $show_icons = !empty($instance['show_icons']) ? $instance['show_icons'] : 'yes';
        $show_count = !empty($instance['show_count']) ? $instance['show_count'] : 'yes';
        $limit = !empty($instance['limit']) ? intval($instance['limit']) : 10;
        
        $terms = get_terms(array(
            'taxonomy' => $taxonomy,
            'number' => $limit,
            'orderby' => 'name',
            'order' => 'ASC',
            'hide_empty' => true
        ));
        
        if (!is_wp_error($terms) && !empty($terms)) {
            if ($style === 'list') {
                $this->render_list($terms, $show_icons, $show_count);
            } elseif ($style === 'cloud') {
                $this->render_cloud($terms, $show_count);
            } elseif ($style === 'grid') {
                $this->render_grid($terms, $show_icons, $show_count);
            }
        }
        
        echo $args['after_widget'];
    }
    
    /**
     * Render list style
     */
    private function render_list($terms, $show_icons, $show_count) {
        echo '<ul class="themisdb-taxonomy-list">';
        foreach ($terms as $term) {
            $icon = get_term_meta($term->term_id, 'icon', true);
            $color = get_term_meta($term->term_id, 'color', true);
            $link = get_term_link($term);
            
            echo '<li>';
            echo '<a href="' . esc_url($link) . '">';
            
            if ($show_icons === 'yes' && $icon) {
                $style = $color ? 'style="color: ' . esc_attr($color) . ';"' : '';
                echo '<span class="icon" ' . $style . '>' . esc_html($icon) . '</span> ';
            }
            
            echo '<span class="name">' . esc_html($term->name) . '</span>';
            
            if ($show_count === 'yes') {
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
        $max_count = 0;
        foreach ($terms as $term) {
            if ($term->count > $max_count) {
                $max_count = $term->count;
            }
        }
        
        echo '<div class="themisdb-tag-cloud">';
        foreach ($terms as $term) {
            $color = get_term_meta($term->term_id, 'color', true);
            $link = get_term_link($term);
            
            $font_size = $max_count > 0 ? (1 + ($term->count / $max_count) * 1.5) : 1;
            
            $style = 'font-size: ' . $font_size . 'em;';
            if ($color) {
                $style .= ' color: ' . esc_attr($color) . ';';
            }
            
            $title = $show_count === 'yes' ? $term->count . ' items' : '';
            
            echo '<a href="' . esc_url($link) . '" style="' . $style . '" title="' . esc_attr($title) . '">';
            echo esc_html($term->name);
            echo '</a> ';
        }
        echo '</div>';
    }
    
    /**
     * Render grid style
     */
    private function render_grid($terms, $show_icons, $show_count) {
        echo '<div class="themisdb-taxonomy-grid">';
        foreach ($terms as $term) {
            $icon = get_term_meta($term->term_id, 'icon', true);
            $color = get_term_meta($term->term_id, 'color', true);
            $link = get_term_link($term);
            
            $border_style = $color ? 'border-color: ' . esc_attr($color) . ';' : '';
            
            echo '<div class="grid-item" style="' . $border_style . '">';
            
            if ($show_icons === 'yes' && $icon) {
                echo '<div class="icon">' . esc_html($icon) . '</div>';
            }
            
            echo '<h4><a href="' . esc_url($link) . '">' . esc_html($term->name) . '</a></h4>';
            
            if ($show_count === 'yes') {
                echo '<span class="count">' . $term->count . ' posts</span>';
            }
            
            echo '</div>';
        }
        echo '</div>';
    }
    
    /**
     * Back-end widget form
     */
    public function form($instance) {
        $title = !empty($instance['title']) ? $instance['title'] : '';
        $taxonomy = !empty($instance['taxonomy']) ? $instance['taxonomy'] : 'themisdb_feature';
        $style = !empty($instance['style']) ? $instance['style'] : 'list';
        $show_icons = !empty($instance['show_icons']) ? $instance['show_icons'] : 'yes';
        $show_count = !empty($instance['show_count']) ? $instance['show_count'] : 'yes';
        $limit = !empty($instance['limit']) ? $instance['limit'] : 10;
        ?>
        <p>
            <label for="<?php echo esc_attr($this->get_field_id('title')); ?>">
                <?php _e('Title:', 'themisdb-taxonomy'); ?>
            </label>
            <input class="widefat" id="<?php echo esc_attr($this->get_field_id('title')); ?>" 
                   name="<?php echo esc_attr($this->get_field_name('title')); ?>" type="text" 
                   value="<?php echo esc_attr($title); ?>">
        </p>
        
        <p>
            <label for="<?php echo esc_attr($this->get_field_id('taxonomy')); ?>">
                <?php _e('Taxonomy:', 'themisdb-taxonomy'); ?>
            </label>
            <select class="widefat" id="<?php echo esc_attr($this->get_field_id('taxonomy')); ?>" 
                    name="<?php echo esc_attr($this->get_field_name('taxonomy')); ?>">
                <option value="themisdb_feature" <?php selected($taxonomy, 'themisdb_feature'); ?>>Database Features</option>
                <option value="themisdb_usecase" <?php selected($taxonomy, 'themisdb_usecase'); ?>>Use Cases</option>
                <option value="themisdb_industry" <?php selected($taxonomy, 'themisdb_industry'); ?>>Industries</option>
                <option value="themisdb_techspec" <?php selected($taxonomy, 'themisdb_techspec'); ?>>Technical Specs</option>
            </select>
        </p>
        
        <p>
            <label for="<?php echo esc_attr($this->get_field_id('style')); ?>">
                <?php _e('Display Style:', 'themisdb-taxonomy'); ?>
            </label>
            <select class="widefat" id="<?php echo esc_attr($this->get_field_id('style')); ?>" 
                    name="<?php echo esc_attr($this->get_field_name('style')); ?>">
                <option value="list" <?php selected($style, 'list'); ?>>List</option>
                <option value="cloud" <?php selected($style, 'cloud'); ?>>Tag Cloud</option>
                <option value="grid" <?php selected($style, 'grid'); ?>>Grid</option>
            </select>
        </p>
        
        <p>
            <label for="<?php echo esc_attr($this->get_field_id('show_icons')); ?>">
                <?php _e('Show Icons:', 'themisdb-taxonomy'); ?>
            </label>
            <select class="widefat" id="<?php echo esc_attr($this->get_field_id('show_icons')); ?>" 
                    name="<?php echo esc_attr($this->get_field_name('show_icons')); ?>">
                <option value="yes" <?php selected($show_icons, 'yes'); ?>>Yes</option>
                <option value="no" <?php selected($show_icons, 'no'); ?>>No</option>
            </select>
        </p>
        
        <p>
            <label for="<?php echo esc_attr($this->get_field_id('show_count')); ?>">
                <?php _e('Show Count:', 'themisdb-taxonomy'); ?>
            </label>
            <select class="widefat" id="<?php echo esc_attr($this->get_field_id('show_count')); ?>" 
                    name="<?php echo esc_attr($this->get_field_name('show_count')); ?>">
                <option value="yes" <?php selected($show_count, 'yes'); ?>>Yes</option>
                <option value="no" <?php selected($show_count, 'no'); ?>>No</option>
            </select>
        </p>
        
        <p>
            <label for="<?php echo esc_attr($this->get_field_id('limit')); ?>">
                <?php _e('Limit:', 'themisdb-taxonomy'); ?>
            </label>
            <input class="widefat" id="<?php echo esc_attr($this->get_field_id('limit')); ?>" 
                   name="<?php echo esc_attr($this->get_field_name('limit')); ?>" type="number" 
                   value="<?php echo esc_attr($limit); ?>" min="1" max="100">
        </p>
        <?php
    }
    
    /**
     * Save widget settings
     */
    public function update($new_instance, $old_instance) {
        $instance = array();
        $instance['title'] = (!empty($new_instance['title'])) ? sanitize_text_field($new_instance['title']) : '';
        $instance['taxonomy'] = (!empty($new_instance['taxonomy'])) ? sanitize_key($new_instance['taxonomy']) : 'themisdb_feature';
        $instance['style'] = (!empty($new_instance['style'])) ? sanitize_key($new_instance['style']) : 'list';
        $instance['show_icons'] = (!empty($new_instance['show_icons'])) ? sanitize_key($new_instance['show_icons']) : 'yes';
        $instance['show_count'] = (!empty($new_instance['show_count'])) ? sanitize_key($new_instance['show_count']) : 'yes';
        $instance['limit'] = (!empty($new_instance['limit'])) ? intval($new_instance['limit']) : 10;
        
        return $instance;
    }
}
