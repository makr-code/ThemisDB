<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-wikilinks.php                                ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     286                                            ║
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
 * WikiLinks Parser
 * Handles [[WikiLink]] syntax and TOC generation
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_WikiLinks {
    
    /**
     * Convert Markdown with WikiLinks to HTML
     */
    public function convert_markdown_with_wikilinks($markdown) {
        // 1. Parse WikiLinks FIRST (before normal markdown)
        $markdown = $this->parse_wikilinks($markdown);
        
        // 2. Normal Markdown conversion
        $html = ThemisDB_Markdown_Converter::convert($markdown);
        
        return $html;
    }
    
    /**
     * Parse WikiLinks in markdown
     */
    private function parse_wikilinks($markdown) {
        // [[Page Name]] → internal link
        // [[Page Name|Display Text]] → internal link with custom text
        // [[Page Name#Section]] → internal link with anchor
        // [[Category:Name]] → assigns category
        // [[File:image.png|thumb|right|Caption]] → embedded image
        
        $pattern = '/\[\[([^\|\]]+)(?:\|([^\]]+))?\]\]/';
        
        return preg_replace_callback($pattern, function($matches) {
            $page_reference = trim($matches[1]);
            $display_text = isset($matches[2]) ? trim($matches[2]) : '';
            
            // Handle Category: syntax
            if (strpos($page_reference, 'Category:') === 0) {
                $category = substr($page_reference, 9);
                $this->assign_category($category);
                return ''; // Don't render in content
            }
            
            // Handle File: syntax
            if (strpos($page_reference, 'File:') === 0) {
                return $this->render_file_embed($page_reference, $display_text);
            }
            
            // Regular page link
            return $this->create_wiki_link($page_reference, $display_text);
        }, $markdown);
    }
    
    /**
     * Create Wiki Link HTML
     */
    private function create_wiki_link($page_reference, $display_text = '') {
        $page_name = $page_reference;
        $section = '';
        
        // Check for section anchor
        if (strpos($page_reference, '#') !== false) {
            list($page_name, $section) = explode('#', $page_reference, 2);
        }
        
        // Generate slug
        $slug = sanitize_title($page_name);
        
        // Build URL
        if ($section) {
            $section_slug = sanitize_title($section);
            $url = "/wiki/{$slug}/#{$section_slug}";
        } else {
            $url = "/wiki/{$slug}/";
        }
        
        // Set display text
        if (empty($display_text)) {
            $display_text = $page_name;
            if ($section) {
                $display_text .= ' (' . $section . ')';
            }
        }
        
        // Check if page exists
        $post = get_page_by_path($slug, OBJECT, 'themisdb_wiki');
        $class = $post ? 'wikilink' : 'wikilink-new';
        
        return sprintf(
            '<a href="%s" class="%s" data-wiki-page="%s">%s</a>',
            esc_url($url),
            esc_attr($class),
            esc_attr($page_name),
            esc_html($display_text)
        );
    }
    
    /**
     * Render File Embed
     */
    private function render_file_embed($file_reference, $options) {
        // Extract filename: File:image.png
        $filename = substr($file_reference, 5);
        
        // Parse options (thumb, right, left, Caption)
        $is_thumb = strpos($options, 'thumb') !== false;
        $align = 'none';
        if (strpos($options, 'right') !== false) {
            $align = 'right';
        } elseif (strpos($options, 'left') !== false) {
            $align = 'left';
        }
        
        // Extract caption (last part)
        $parts = explode('|', $options);
        $caption = end($parts);
        if (in_array($caption, array('thumb', 'right', 'left'))) {
            $caption = '';
        }
        
        // Build image URL (assume it's in uploads)
        $upload_dir = wp_upload_dir();
        $image_url = $upload_dir['baseurl'] . '/wiki/' . $filename;
        
        // Build HTML
        $class = 'wiki-image';
        if ($is_thumb) {
            $class .= ' wiki-thumb';
        }
        if ($align !== 'none') {
            $class .= ' wiki-' . $align;
        }
        
        $html = '<figure class="' . esc_attr($class) . '">';
        $html .= '<img src="' . esc_url($image_url) . '" alt="' . esc_attr($caption) . '">';
        if ($caption) {
            $html .= '<figcaption>' . esc_html($caption) . '</figcaption>';
        }
        $html .= '</figure>';
        
        return $html;
    }
    
    /**
     * Assign Category (called during save)
     */
    private function assign_category($category_name) {
        global $post;
        
        if (!isset($post) || !$post) {
            return;
        }
        
        // Find or create category
        $term = term_exists($category_name, 'wiki_category');
        if (!$term) {
            $term = wp_insert_term($category_name, 'wiki_category');
        }
        
        if (!is_wp_error($term)) {
            wp_set_object_terms($post->ID, intval($term['term_id']), 'wiki_category', true);
        }
    }
    
    /**
     * Add Table of Contents to HTML
     */
    public function add_table_of_contents($html) {
        // Find all H2, H3, H4 headings
        preg_match_all('/<h([234])>(.*?)<\/h[234]>/i', $html, $matches, PREG_SET_ORDER);
        
        if (count($matches) < 3) {
            return $html; // Only show TOC if >= 3 headings
        }
        
        $toc = '<div class="wiki-toc">';
        $toc .= '<h2>' . __('Table of Contents', 'themisdb-wiki') . '</h2>';
        $toc .= '<ul class="wiki-toc-list">';
        
        foreach ($matches as $match) {
            $level = $match[1];
            $text = strip_tags($match[2]);
            $id = sanitize_title($text);
            
            // Add ID to heading if not present
            if (strpos($match[0], 'id=') === false) {
                $new_heading = '<h' . $level . ' id="' . $id . '">' . $match[2] . '</h' . $level . '>';
                $html = str_replace($match[0], $new_heading, $html);
            }
            
            // Add to TOC
            $indent_class = 'toc-level-' . $level;
            $toc .= '<li class="' . $indent_class . '">';
            $toc .= '<a href="#' . esc_attr($id) . '">' . esc_html($text) . '</a>';
            $toc .= '</li>';
        }
        
        $toc .= '</ul></div>';
        
        // Insert TOC after first paragraph
        $html = preg_replace('/(<p>.*?<\/p>)/s', '$1' . $toc, $html, 1);
        
        return $html;
    }
    
    /**
     * Generate TOC HTML for shortcode
     */
    public function generate_toc_html($content, $depth = 3, $title = 'Contents') {
        // Extract headings up to specified depth
        $pattern = '/<h([2-' . $depth . '])>(.*?)<\/h[2-' . $depth . ']>/i';
        preg_match_all($pattern, $content, $matches, PREG_SET_ORDER);
        
        if (empty($matches)) {
            return '';
        }
        
        $toc = '<div class="wiki-toc wiki-toc-shortcode">';
        $toc .= '<h2>' . esc_html($title) . '</h2>';
        $toc .= '<ul class="wiki-toc-list">';
        
        foreach ($matches as $match) {
            $level = $match[1];
            $text = strip_tags($match[2]);
            $id = sanitize_title($text);
            
            $indent_class = 'toc-level-' . $level;
            $toc .= '<li class="' . $indent_class . '">';
            $toc .= '<a href="#' . esc_attr($id) . '">' . esc_html($text) . '</a>';
            $toc .= '</li>';
        }
        
        $toc .= '</ul></div>';
        
        return $toc;
    }
    
    /**
     * Get backlinks for a wiki page
     */
    public function get_backlinks($page_slug) {
        global $wpdb;
        
        // Properly escape the search term for LIKE query
        $search_pattern = '%[[' . $wpdb->esc_like($page_slug) . '%';
        
        $query = $wpdb->prepare("
            SELECT p.ID, p.post_title, p.post_name
            FROM {$wpdb->posts} p
            INNER JOIN {$wpdb->postmeta} pm ON p.ID = pm.post_id
            WHERE p.post_type = 'themisdb_wiki'
            AND p.post_status = 'publish'
            AND pm.meta_key = '_wiki_markdown'
            AND pm.meta_value LIKE %s
        ", $search_pattern);
        
        return $wpdb->get_results($query);
    }
}
