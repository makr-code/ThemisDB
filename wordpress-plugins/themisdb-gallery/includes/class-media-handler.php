<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-media-handler.php                            ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:18                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     248                                            ║
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
 * Media Handler
 * 
 * Handles downloading images and adding them to WordPress media library
 * with proper attribution
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Gallery_Media_Handler {
    
    /**
     * Download and import image to WordPress media library
     * 
     * @param array $image_data Image data from API
     * @param int $post_id Optional post ID to attach image to
     * @return int|WP_Error Attachment ID or error
     */
    public static function import_image($image_data, $post_id = 0) {
        if (empty($image_data['url'])) {
            return new WP_Error('invalid_image', __('Ungültige Bilddaten', 'themisdb-gallery'));
        }
        
        // Download image
        $temp_file = download_url($image_data['url']);
        
        if (is_wp_error($temp_file)) {
            return $temp_file;
        }
        
        // Prepare file array
        $file_array = array(
            'name' => self::generate_filename($image_data),
            'tmp_name' => $temp_file,
        );
        
        // Import to media library
        $attachment_id = media_handle_sideload($file_array, $post_id);
        
        // Clean up temp file
        if (file_exists($temp_file)) {
            @unlink($temp_file);
        }
        
        if (is_wp_error($attachment_id)) {
            return $attachment_id;
        }
        
        // Add attribution metadata
        self::add_attribution_metadata($attachment_id, $image_data);
        
        return $attachment_id;
    }
    
    /**
     * Generate filename for downloaded image
     * 
     * @param array $image_data Image data
     * @return string Filename
     */
    private static function generate_filename($image_data) {
        $title = !empty($image_data['title']) ? $image_data['title'] : 'image';
        $source = !empty($image_data['source']) ? $image_data['source'] : 'external';
        
        // Sanitize title
        $title = sanitize_title($title);
        $title = substr($title, 0, 50); // Limit length
        
        // Get file extension from URL
        $extension = pathinfo(parse_url($image_data['url'], PHP_URL_PATH), PATHINFO_EXTENSION);
        if (empty($extension)) {
            $extension = 'jpg';
        }
        
        return sprintf('%s-%s-%s.%s', $title, strtolower($source), uniqid(), $extension);
    }
    
    /**
     * Add attribution metadata to attachment
     * 
     * @param int $attachment_id Attachment ID
     * @param array $image_data Image data
     */
    private static function add_attribution_metadata($attachment_id, $image_data) {
        // Store original image data
        update_post_meta($attachment_id, '_themisdb_gallery_source', $image_data['source'] ?? '');
        update_post_meta($attachment_id, '_themisdb_gallery_source_url', $image_data['source_url'] ?? '');
        update_post_meta($attachment_id, '_themisdb_gallery_author', $image_data['author'] ?? '');
        update_post_meta($attachment_id, '_themisdb_gallery_author_url', $image_data['author_url'] ?? '');
        update_post_meta($attachment_id, '_themisdb_gallery_license', $image_data['license'] ?? '');
        update_post_meta($attachment_id, '_themisdb_gallery_license_url', $image_data['license_url'] ?? '');
        
        // Update attachment title and alt text
        $title = !empty($image_data['title']) ? $image_data['title'] : 'Image';
        wp_update_post(array(
            'ID' => $attachment_id,
            'post_title' => $title,
        ));
        
        update_post_meta($attachment_id, '_wp_attachment_image_alt', $title);
        
        // Update caption with attribution if auto-attribution is enabled
        if (get_option('themisdb_gallery_auto_attribution') === 'yes') {
            $caption = self::generate_attribution_text($image_data);
            wp_update_post(array(
                'ID' => $attachment_id,
                'post_excerpt' => $caption,
            ));
        }
    }
    
    /**
     * Generate attribution text for image
     * 
     * @param array $image_data Image data
     * @return string Attribution text
     */
    public static function generate_attribution_text($image_data) {
        $parts = array();
        
        // Photo by [Author]
        if (!empty($image_data['author'])) {
            if (!empty($image_data['author_url'])) {
                $parts[] = sprintf(
                    'Foto von <a href="%s" target="_blank" rel="noopener">%s</a>',
                    esc_url($image_data['author_url']),
                    esc_html($image_data['author'])
                );
            } else {
                $parts[] = 'Foto von ' . esc_html($image_data['author']);
            }
        }
        
        // on [Source]
        if (!empty($image_data['source'])) {
            if (!empty($image_data['source_url'])) {
                $parts[] = sprintf(
                    'auf <a href="%s" target="_blank" rel="noopener">%s</a>',
                    esc_url($image_data['source_url']),
                    esc_html($image_data['source'])
                );
            } else {
                $parts[] = 'auf ' . esc_html($image_data['source']);
            }
        }
        
        // License
        if (!empty($image_data['license'])) {
            if (!empty($image_data['license_url'])) {
                $parts[] = sprintf(
                    '(<a href="%s" target="_blank" rel="noopener">%s</a>)',
                    esc_url($image_data['license_url']),
                    esc_html($image_data['license'])
                );
            } else {
                $parts[] = '(' . esc_html($image_data['license']) . ')';
            }
        }
        
        return implode(' ', $parts);
    }
    
    /**
     * Get attribution HTML for attachment
     * 
     * @param int $attachment_id Attachment ID
     * @return string Attribution HTML
     */
    public static function get_attribution_html($attachment_id) {
        $image_data = array(
            'author' => get_post_meta($attachment_id, '_themisdb_gallery_author', true),
            'author_url' => get_post_meta($attachment_id, '_themisdb_gallery_author_url', true),
            'source' => get_post_meta($attachment_id, '_themisdb_gallery_source', true),
            'source_url' => get_post_meta($attachment_id, '_themisdb_gallery_source_url', true),
            'license' => get_post_meta($attachment_id, '_themisdb_gallery_license', true),
            'license_url' => get_post_meta($attachment_id, '_themisdb_gallery_license_url', true),
        );
        
        // Check if this image has attribution data
        if (empty($image_data['author']) && empty($image_data['source'])) {
            return '';
        }
        
        return self::generate_attribution_text($image_data);
    }
    
    /**
     * AJAX handler to import image
     */
    public static function ajax_import_image() {
        check_ajax_referer('themisdb_gallery_admin_nonce', 'nonce');
        
        if (!current_user_can('upload_files')) {
            wp_send_json_error(array('message' => __('Keine Berechtigung zum Hochladen von Dateien', 'themisdb-gallery')));
        }
        
        $image_data = isset($_POST['image_data']) ? json_decode(stripslashes($_POST['image_data']), true) : array();
        $post_id = isset($_POST['post_id']) ? intval($_POST['post_id']) : 0;
        
        if (empty($image_data)) {
            wp_send_json_error(array('message' => __('Keine Bilddaten angegeben', 'themisdb-gallery')));
        }
        
        $attachment_id = self::import_image($image_data, $post_id);
        
        if (is_wp_error($attachment_id)) {
            wp_send_json_error(array('message' => $attachment_id->get_error_message()));
        }
        
        $attachment_url = wp_get_attachment_url($attachment_id);
        $attachment_thumb = wp_get_attachment_image_url($attachment_id, 'thumbnail');
        
        wp_send_json_success(array(
            'attachment_id' => $attachment_id,
            'url' => $attachment_url,
            'thumb' => $attachment_thumb,
            'attribution' => self::get_attribution_html($attachment_id),
        ));
    }
}

// Register AJAX handlers
add_action('wp_ajax_themisdb_gallery_import_image', array('ThemisDB_Gallery_Media_Handler', 'ajax_import_image'));
