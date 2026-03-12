<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-compendium-widget.php                        ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:17                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     147                                            ║
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
 * ThemisDB Compendium Widget
 * 
 * Widget for displaying compendium downloads in sidebars
 */

// Prevent direct access
if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Compendium_Widget extends WP_Widget {
    
    /**
     * Constructor
     */
    public function __construct() {
        parent::__construct(
            'themisdb_compendium_widget',
            __('ThemisDB Kompendium Downloads', 'themisdb-compendium-downloads'),
            array(
                'description' => __('Zeigt ThemisDB Kompendium Download-Links an', 'themisdb-compendium-downloads'),
                'classname' => 'themisdb-compendium-widget'
            )
        );
    }
    
    /**
     * Widget output
     */
    public function widget($args, $instance) {
        $title = !empty($instance['title']) ? $instance['title'] : __('Kompendium', 'themisdb-compendium-downloads');
        $style = !empty($instance['style']) ? $instance['style'] : 'modern';
        $show_version = !empty($instance['show_version']) ? 'yes' : 'no';
        $show_size = !empty($instance['show_size']) ? 'yes' : 'no';
        
        echo $args['before_widget'];
        
        if (!empty($title)) {
            echo $args['before_title'] . esc_html($title) . $args['after_title'];
        }
        
        // Sanitize widget settings for use in shortcode
        $style_safe = esc_attr($style);
        $show_version_safe = esc_attr($show_version);
        $show_size_safe = esc_attr($show_size);
        
        // Use the shortcode to render content
        $shortcode = '[themisdb_compendium_downloads style="' . $style_safe . '" show_version="' . $show_version_safe . '" show_date="no" show_size="' . $show_size_safe . '" layout="compact"]';
        echo do_shortcode($shortcode);
        
        echo $args['after_widget'];
    }
    
    /**
     * Widget form
     */
    public function form($instance) {
        $title = !empty($instance['title']) ? $instance['title'] : __('Kompendium', 'themisdb-compendium-downloads');
        $style = !empty($instance['style']) ? $instance['style'] : 'modern';
        $show_version = isset($instance['show_version']) ? (bool) $instance['show_version'] : true;
        $show_size = isset($instance['show_size']) ? (bool) $instance['show_size'] : true;
        ?>
        <p>
            <label for="<?php echo esc_attr($this->get_field_id('title')); ?>">
                <?php _e('Titel:', 'themisdb-compendium-downloads'); ?>
            </label>
            <input class="widefat" 
                   id="<?php echo esc_attr($this->get_field_id('title')); ?>" 
                   name="<?php echo esc_attr($this->get_field_name('title')); ?>" 
                   type="text" 
                   value="<?php echo esc_attr($title); ?>">
        </p>
        
        <p>
            <label for="<?php echo esc_attr($this->get_field_id('style')); ?>">
                <?php _e('Stil:', 'themisdb-compendium-downloads'); ?>
            </label>
            <select class="widefat" 
                    id="<?php echo esc_attr($this->get_field_id('style')); ?>" 
                    name="<?php echo esc_attr($this->get_field_name('style')); ?>">
                <option value="modern" <?php selected($style, 'modern'); ?>><?php _e('Modern', 'themisdb-compendium-downloads'); ?></option>
                <option value="classic" <?php selected($style, 'classic'); ?>><?php _e('Klassisch', 'themisdb-compendium-downloads'); ?></option>
                <option value="minimal" <?php selected($style, 'minimal'); ?>><?php _e('Minimal', 'themisdb-compendium-downloads'); ?></option>
            </select>
        </p>
        
        <p>
            <input class="checkbox" 
                   type="checkbox" 
                   <?php checked($show_version); ?> 
                   id="<?php echo esc_attr($this->get_field_id('show_version')); ?>" 
                   name="<?php echo esc_attr($this->get_field_name('show_version')); ?>">
            <label for="<?php echo esc_attr($this->get_field_id('show_version')); ?>">
                <?php _e('Version anzeigen', 'themisdb-compendium-downloads'); ?>
            </label>
        </p>
        
        <p>
            <input class="checkbox" 
                   type="checkbox" 
                   <?php checked($show_size); ?> 
                   id="<?php echo esc_attr($this->get_field_id('show_size')); ?>" 
                   name="<?php echo esc_attr($this->get_field_name('show_size')); ?>">
            <label for="<?php echo esc_attr($this->get_field_id('show_size')); ?>">
                <?php _e('Dateigröße anzeigen', 'themisdb-compendium-downloads'); ?>
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
        $instance['style'] = (!empty($new_instance['style'])) ? sanitize_text_field($new_instance['style']) : 'modern';
        $instance['show_version'] = isset($new_instance['show_version']) ? 1 : 0;
        $instance['show_size'] = isset($new_instance['show_size']) ? 1 : 0;
        
        return $instance;
    }
}
