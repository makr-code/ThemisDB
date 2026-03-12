<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-search.php                                   ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     274                                            ║
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
 * Wiki Search
 * Handles full-text search functionality
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Wiki_Search {
    
    /**
     * Constructor
     */
    public function __construct() {
        // AJAX handlers
        add_action('wp_ajax_themisdb_wiki_search', array($this, 'ajax_search'));
        add_action('wp_ajax_nopriv_themisdb_wiki_search', array($this, 'ajax_search'));
        
        // Modify main search query
        add_action('pre_get_posts', array($this, 'modify_search_query'));
    }
    
    /**
     * Search Wiki Pages
     */
    public function search($query) {
        global $wpdb;
        
        if (empty($query)) {
            return array();
        }
        
        $search_term = '%' . $wpdb->esc_like($query) . '%';
        
        $sql = $wpdb->prepare("
            SELECT DISTINCT p.ID, p.post_title, p.post_name, p.post_content, pm.meta_value as markdown
            FROM {$wpdb->posts} p
            LEFT JOIN {$wpdb->postmeta} pm ON p.ID = pm.post_id AND pm.meta_key = '_wiki_markdown'
            WHERE p.post_type = 'themisdb_wiki'
            AND p.post_status = 'publish'
            AND (
                p.post_title LIKE %s
                OR p.post_content LIKE %s
                OR pm.meta_value LIKE %s
            )
            ORDER BY 
                CASE 
                    WHEN p.post_title LIKE %s THEN 1
                    WHEN p.post_content LIKE %s THEN 2
                    ELSE 3
                END,
                p.post_title ASC
            LIMIT 20
        ", $search_term, $search_term, $search_term, $search_term, $search_term);
        
        $results = $wpdb->get_results($sql);
        
        $formatted_results = array();
        
        foreach ($results as $result) {
            $formatted_results[] = array(
                'id' => $result->ID,
                'title' => $result->post_title,
                'url' => get_permalink($result->ID),
                'excerpt' => $this->generate_excerpt($result->post_content, $query),
                'slug' => $result->post_name
            );
        }
        
        return $formatted_results;
    }
    
    /**
     * Generate Search Excerpt
     */
    private function generate_excerpt($content, $query, $length = 150) {
        // Strip HTML
        $content = wp_strip_all_tags($content);
        
        // Find query position
        $pos = stripos($content, $query);
        
        if ($pos !== false) {
            // Extract context around query
            $start = max(0, $pos - 75);
            $excerpt = substr($content, $start, $length);
            
            // Add ellipsis
            if ($start > 0) {
                $excerpt = '...' . $excerpt;
            }
            if (strlen($content) > $start + $length) {
                $excerpt .= '...';
            }
            
            // Highlight query
            $excerpt = preg_replace('/(' . preg_quote($query, '/') . ')/i', '<mark>$1</mark>', $excerpt);
        } else {
            // Just return first part of content
            $excerpt = substr($content, 0, $length);
            if (strlen($content) > $length) {
                $excerpt .= '...';
            }
        }
        
        return $excerpt;
    }
    
    /**
     * Get Search Form HTML
     */
    public function get_search_form() {
        $output = '<form class="wiki-search" role="search" method="get" action="' . esc_url(home_url('/')) . '">';
        $output .= '<input type="hidden" name="post_type" value="themisdb_wiki">';
        $output .= '<div class="wiki-search-wrapper">';
        $output .= '<input type="text" ';
        $output .= 'name="s" ';
        $output .= 'id="wiki-search-input" ';
        $output .= 'class="wiki-search-input" ';
        $output .= 'placeholder="' . esc_attr__('Search wiki...', 'themisdb-wiki') . '" ';
        $output .= 'autocomplete="off" ';
        $output .= 'value="' . get_search_query() . '">';
        $output .= '<button type="submit" class="wiki-search-submit">';
        $output .= '<span class="dashicons dashicons-search"></span>';
        $output .= '<span class="screen-reader-text">' . __('Search', 'themisdb-wiki') . '</span>';
        $output .= '</button>';
        $output .= '</div>';
        $output .= '<div id="search-suggestions" class="search-suggestions" style="display:none;"></div>';
        $output .= '</form>';
        
        return $output;
    }
    
    /**
     * AJAX Search Handler
     */
    public function ajax_search() {
        $query = isset($_POST['query']) ? sanitize_text_field($_POST['query']) : '';
        
        if (empty($query) || strlen($query) < 2) {
            wp_send_json_success(array('results' => array()));
            return;
        }
        
        $results = $this->search($query);
        
        wp_send_json_success(array('results' => $results));
    }
    
    /**
     * Modify WordPress Search Query to Include Wiki Pages
     */
    public function modify_search_query($query) {
        if (!is_admin() && $query->is_search() && $query->is_main_query()) {
            // Get current post types
            $post_types = $query->get('post_type');
            
            // If no post types set, default to all searchable types including wiki
            if (empty($post_types)) {
                $post_types = array('post', 'page', 'themisdb_wiki');
                $query->set('post_type', $post_types);
            } elseif ($post_types === 'themisdb_wiki') {
                // Keep it as wiki only
                $query->set('post_type', 'themisdb_wiki');
            }
        }
        
        return $query;
    }
    
    /**
     * Get Related Pages (based on tags/categories)
     */
    public function get_related_pages($post_id, $limit = 5) {
        // Get categories of current post
        $categories = wp_get_object_terms($post_id, 'wiki_category', array('fields' => 'ids'));
        
        if (empty($categories) || is_wp_error($categories)) {
            return array();
        }
        
        // Query for related posts
        $query = new WP_Query(array(
            'post_type' => 'themisdb_wiki',
            'posts_per_page' => $limit,
            'post__not_in' => array($post_id),
            'tax_query' => array(
                array(
                    'taxonomy' => 'wiki_category',
                    'field' => 'term_id',
                    'terms' => $categories
                )
            )
        ));
        
        $related = array();
        
        while ($query->have_posts()) {
            $query->the_post();
            $related[] = array(
                'id' => get_the_ID(),
                'title' => get_the_title(),
                'url' => get_permalink(),
                'excerpt' => get_the_excerpt()
            );
        }
        
        wp_reset_postdata();
        
        return $related;
    }
    
    /**
     * Get Popular Pages (by view count)
     */
    public function get_popular_pages($limit = 10) {
        $query = new WP_Query(array(
            'post_type' => 'themisdb_wiki',
            'posts_per_page' => $limit,
            'meta_key' => '_wiki_view_count',
            'orderby' => 'meta_value_num',
            'order' => 'DESC'
        ));
        
        $popular = array();
        
        while ($query->have_posts()) {
            $query->the_post();
            $popular[] = array(
                'id' => get_the_ID(),
                'title' => get_the_title(),
                'url' => get_permalink(),
                'views' => get_post_meta(get_the_ID(), '_wiki_view_count', true)
            );
        }
        
        wp_reset_postdata();
        
        return $popular;
    }
    
    /**
     * Track Page View
     */
    public function track_page_view($post_id) {
        $count = get_post_meta($post_id, '_wiki_view_count', true);
        $count = $count ? intval($count) + 1 : 1;
        update_post_meta($post_id, '_wiki_view_count', $count);
    }
}
