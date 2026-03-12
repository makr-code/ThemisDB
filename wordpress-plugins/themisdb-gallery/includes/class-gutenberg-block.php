<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-gutenberg-block.php                          ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:18                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     177                                            ║
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
 * Gutenberg Block Handler
 * 
 * Registers custom Gutenberg blocks for the gallery
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Gallery_Gutenberg_Block {
    
    public function __construct() {
        add_action('init', array($this, 'register_blocks'));
        add_action('enqueue_block_editor_assets', array($this, 'enqueue_block_editor_assets'));
    }
    
    /**
     * Register Gutenberg blocks
     */
    public function register_blocks() {
        // Check if Gutenberg is available
        if (!function_exists('register_block_type')) {
            return;
        }
        
        // Register image search block
        register_block_type('themisdb-gallery/image-search', array(
            'editor_script' => 'themisdb-gallery-block-editor',
            'editor_style' => 'themisdb-gallery-block-editor',
            'style' => 'themisdb-gallery-block',
            'render_callback' => array($this, 'render_image_search_block'),
            'attributes' => array(
                'query' => array(
                    'type' => 'string',
                    'default' => '',
                ),
                'provider' => array(
                    'type' => 'string',
                    'default' => 'all',
                ),
                'columns' => array(
                    'type' => 'number',
                    'default' => 3,
                ),
                'limit' => array(
                    'type' => 'number',
                    'default' => 12,
                ),
                'showAttribution' => array(
                    'type' => 'boolean',
                    'default' => true,
                ),
            ),
        ));
        
        // Register gallery block
        register_block_type('themisdb-gallery/gallery', array(
            'editor_script' => 'themisdb-gallery-block-editor',
            'editor_style' => 'themisdb-gallery-block-editor',
            'style' => 'themisdb-gallery-block',
            'render_callback' => array($this, 'render_gallery_block'),
            'attributes' => array(
                'ids' => array(
                    'type' => 'array',
                    'default' => array(),
                ),
                'columns' => array(
                    'type' => 'number',
                    'default' => 3,
                ),
                'showAttribution' => array(
                    'type' => 'boolean',
                    'default' => true,
                ),
            ),
        ));
    }
    
    /**
     * Enqueue block editor assets
     */
    public function enqueue_block_editor_assets() {
        wp_enqueue_script(
            'themisdb-gallery-block-editor',
            THEMISDB_GALLERY_PLUGIN_URL . 'assets/js/blocks.js',
            array('wp-blocks', 'wp-element', 'wp-editor', 'wp-components', 'wp-i18n'),
            THEMISDB_GALLERY_VERSION,
            true
        );
        
        wp_enqueue_style(
            'themisdb-gallery-block-editor',
            THEMISDB_GALLERY_PLUGIN_URL . 'assets/css/blocks-editor.css',
            array('wp-edit-blocks'),
            THEMISDB_GALLERY_VERSION
        );
        
        wp_enqueue_style(
            'themisdb-gallery-block',
            THEMISDB_GALLERY_PLUGIN_URL . 'assets/css/blocks.css',
            array(),
            THEMISDB_GALLERY_VERSION
        );
        
        // Pass data to JavaScript
        wp_localize_script('themisdb-gallery-block-editor', 'themisdbGalleryBlock', array(
            'ajaxurl' => admin_url('admin-ajax.php'),
            'nonce' => wp_create_nonce('themisdb_gallery_admin_nonce'),
        ));
    }
    
    /**
     * Render image search block
     * 
     * @param array $attributes Block attributes
     * @return string HTML output
     */
    public function render_image_search_block($attributes) {
        $query = isset($attributes['query']) ? $attributes['query'] : '';
        $provider = isset($attributes['provider']) ? $attributes['provider'] : 'all';
        $columns = isset($attributes['columns']) ? intval($attributes['columns']) : 3;
        $limit = isset($attributes['limit']) ? intval($attributes['limit']) : 12;
        $show_attribution = isset($attributes['showAttribution']) ? ($attributes['showAttribution'] ? 'yes' : 'no') : 'yes';
        
        if (empty($query)) {
            return '<div class="themisdb-gallery-notice">' . __('Bitte geben Sie einen Suchbegriff ein.', 'themisdb-gallery') . '</div>';
        }
        
        // Use shortcode to render
        return do_shortcode('[themisdb_gallery search="' . esc_attr($query) . '" provider="' . esc_attr($provider) . '" columns="' . esc_attr($columns) . '" limit="' . esc_attr($limit) . '" show_attribution="' . esc_attr($show_attribution) . '"]');
    }
    
    /**
     * Render gallery block
     * 
     * @param array $attributes Block attributes
     * @return string HTML output
     */
    public function render_gallery_block($attributes) {
        $ids = isset($attributes['ids']) ? $attributes['ids'] : array();
        $columns = isset($attributes['columns']) ? intval($attributes['columns']) : 3;
        $show_attribution = isset($attributes['showAttribution']) ? ($attributes['showAttribution'] ? 'yes' : 'no') : 'yes';
        
        if (empty($ids)) {
            return '<div class="themisdb-gallery-notice">' . __('Keine Bilder ausgewählt.', 'themisdb-gallery') . '</div>';
        }
        
        $ids_string = implode(',', array_map('intval', $ids));
        
        // Use shortcode to render
        return do_shortcode('[themisdb_gallery ids="' . esc_attr($ids_string) . '" columns="' . esc_attr($columns) . '" show_attribution="' . esc_attr($show_attribution) . '"]');
    }
}
