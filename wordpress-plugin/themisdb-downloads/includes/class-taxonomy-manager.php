<?php
/**
 * Taxonomy Manager
 * Automatically selects and creates tags and categories for ThemisDB content
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Downloads_Taxonomy_Manager {
    
    /**
     * Constructor
     */
    public function __construct() {
        // Hook into post save to auto-assign taxonomies
        add_action('save_post', array($this, 'auto_assign_taxonomies'), 10, 3);
        
        // Add filter to assign taxonomies when shortcode is used
        add_filter('the_content', array($this, 'process_content_taxonomies'), 999);
    }
    
    /**
     * Auto-assign taxonomies when a post is saved
     * 
     * @param int $post_id The post ID
     * @param WP_Post $post The post object
     * @param bool $update Whether this is an update
     */
    public function auto_assign_taxonomies($post_id, $post, $update) {
        // Check if auto-tagging is enabled
        if (!get_option('themisdb_auto_taxonomy', 0)) {
            return;
        }
        
        // Avoid auto-save and revisions
        if (defined('DOING_AUTOSAVE') && DOING_AUTOSAVE) {
            return;
        }
        
        // Check if post contains ThemisDB shortcodes
        if (!$this->has_themisdb_shortcodes($post->post_content)) {
            return;
        }
        
        // Extract and assign taxonomies
        $this->extract_and_assign_taxonomies($post_id, $post->post_content);
    }
    
    /**
     * Process content and assign taxonomies when shortcode is present
     * 
     * @param string $content The post content
     * @return string The unchanged content
     */
    public function process_content_taxonomies($content) {
        global $post;
        
        if (!$post || !get_option('themisdb_auto_taxonomy', 0)) {
            return $content;
        }
        
        // Check if content has ThemisDB shortcodes
        if ($this->has_themisdb_shortcodes($content)) {
            // Get current post taxonomies to avoid duplicate processing
            $current_tags = wp_get_post_tags($post->ID, array('fields' => 'names'));
            
            // Only process if no ThemisDB tags exist yet
            if (empty($current_tags) || !$this->has_themisdb_tags($current_tags)) {
                $this->extract_and_assign_taxonomies($post->ID, $content);
            }
        }
        
        return $content;
    }
    
    /**
     * Check if content has ThemisDB shortcodes
     * 
     * @param string $content The content to check
     * @return bool
     */
    private function has_themisdb_shortcodes($content) {
        $shortcodes = array(
            'themisdb_downloads',
            'themisdb_latest',
            'themisdb_readme',
            'themisdb_changelog'
        );
        
        foreach ($shortcodes as $shortcode) {
            if (has_shortcode($content, $shortcode)) {
                return true;
            }
        }
        
        return false;
    }
    
    /**
     * Check if current tags include ThemisDB-related tags
     * 
     * @param array $tags Array of tag names
     * @return bool
     */
    private function has_themisdb_tags($tags) {
        $themisdb_keywords = array('themisdb', 'database', 'release', 'download', 'version');
        
        foreach ($tags as $tag) {
            foreach ($themisdb_keywords as $keyword) {
                if (stripos($tag, $keyword) !== false) {
                    return true;
                }
            }
        }
        
        return false;
    }
    
    /**
     * Extract release information and assign taxonomies
     * 
     * @param int $post_id The post ID
     * @param string $content The post content
     */
    private function extract_and_assign_taxonomies($post_id, $content) {
        // Get latest release information
        $api = new ThemisDB_Downloads_GitHub_API();
        $release = $api->get_latest_release();
        
        if (is_wp_error($release) || empty($release)) {
            return;
        }
        
        // Extract taxonomies from release data
        $tags = $this->extract_tags_from_release($release);
        $categories = $this->extract_categories_from_release($release);
        
        // Assign tags if enabled
        if (get_option('themisdb_auto_tags', 1)) {
            $this->assign_tags($post_id, $tags);
        }
        
        // Assign categories if enabled
        if (get_option('themisdb_auto_categories', 1)) {
            $this->assign_categories($post_id, $categories);
        }
    }
    
    /**
     * Extract tags from release data
     * 
     * @param array $release Release data
     * @return array Array of tag names
     */
    private function extract_tags_from_release($release) {
        $tags = array();
        
        // Add base tags
        $tags[] = 'ThemisDB';
        $tags[] = 'Database';
        $tags[] = 'Download';
        
        // Add version tag
        if (!empty($release['version'])) {
            $tags[] = $release['version'];
            
            // Extract major version (e.g., v1.4.0 -> v1.4)
            if (preg_match('/^v?(\d+\.\d+)/', $release['version'], $matches)) {
                $tags[] = 'v' . $matches[1];
            }
        }
        
        // Add platform tags from assets
        if (!empty($release['assets'])) {
            $platforms = array();
            foreach ($release['assets'] as $asset) {
                $platform = $this->detect_platform($asset['name']);
                if ($platform && $platform !== 'other') {
                    $platforms[$platform] = ucfirst($platform);
                }
            }
            $tags = array_merge($tags, array_values($platforms));
        }
        
        // Add release type tag
        if (!empty($release['prerelease']) && $release['prerelease']) {
            $tags[] = 'Pre-Release';
            $tags[] = 'Beta';
        } else {
            $tags[] = 'Stable Release';
        }
        
        // Add year and month tags
        if (!empty($release['published_at'])) {
            $date = strtotime($release['published_at']);
            $tags[] = date('Y', $date);
            $tags[] = date('Y-m', $date);
        }
        
        return array_unique($tags);
    }
    
    /**
     * Extract categories from release data
     * 
     * @param array $release Release data
     * @return array Array of category names
     */
    private function extract_categories_from_release($release) {
        $categories = array();
        
        // Main category
        $categories[] = 'ThemisDB Releases';
        
        // Version-based category
        if (!empty($release['version'])) {
            // Extract major version for category (e.g., v1.4.0 -> Version 1.4)
            if (preg_match('/^v?(\d+)\.(\d+)/', $release['version'], $matches)) {
                $categories[] = 'Version ' . $matches[1] . '.' . $matches[2];
            }
        }
        
        // Release type category
        if (!empty($release['prerelease']) && $release['prerelease']) {
            $categories[] = 'Beta Releases';
        } else {
            $categories[] = 'Stable Releases';
        }
        
        // Year category
        if (!empty($release['published_at'])) {
            $year = date('Y', strtotime($release['published_at']));
            $categories[] = 'Releases ' . $year;
        }
        
        return array_unique($categories);
    }
    
    /**
     * Detect platform from filename
     * 
     * @param string $filename The filename
     * @return string The platform name
     */
    private function detect_platform($filename) {
        $filename_lower = strtolower($filename);
        
        if (strpos($filename_lower, 'windows') !== false || strpos($filename_lower, '.exe') !== false || strpos($filename_lower, 'win') !== false) {
            return 'windows';
        } elseif (strpos($filename_lower, 'linux') !== false || strpos($filename_lower, '.deb') !== false || strpos($filename_lower, '.rpm') !== false) {
            return 'linux';
        } elseif (strpos($filename_lower, 'docker') !== false) {
            return 'docker';
        } elseif (strpos($filename_lower, 'qnap') !== false) {
            return 'qnap';
        } elseif (strpos($filename_lower, 'arm') !== false) {
            return 'arm';
        } elseif (strpos($filename_lower, 'macos') !== false || strpos($filename_lower, 'darwin') !== false) {
            return 'macos';
        }
        
        return 'other';
    }
    
    /**
     * Assign tags to post, creating them if they don't exist
     * 
     * @param int $post_id The post ID
     * @param array $tags Array of tag names
     */
    private function assign_tags($post_id, $tags) {
        if (empty($tags)) {
            return;
        }
        
        $tag_ids = array();
        
        foreach ($tags as $tag_name) {
            // Check if tag exists
            $tag = get_term_by('name', $tag_name, 'post_tag');
            
            if (!$tag) {
                // Create new tag
                $result = wp_insert_term($tag_name, 'post_tag');
                
                if (!is_wp_error($result)) {
                    $tag_ids[] = $result['term_id'];
                }
            } else {
                $tag_ids[] = $tag->term_id;
            }
        }
        
        // Append tags to post (don't replace existing ones)
        if (!empty($tag_ids)) {
            wp_set_post_terms($post_id, $tag_ids, 'post_tag', true);
        }
    }
    
    /**
     * Assign categories to post, creating them if they don't exist
     * 
     * @param int $post_id The post ID
     * @param array $categories Array of category names
     */
    private function assign_categories($post_id, $categories) {
        if (empty($categories)) {
            return;
        }
        
        $category_ids = array();
        
        foreach ($categories as $category_name) {
            // Check if category exists
            $category = get_term_by('name', $category_name, 'category');
            
            if (!$category) {
                // Create new category
                $result = wp_insert_term($category_name, 'category');
                
                if (!is_wp_error($result)) {
                    $category_ids[] = $result['term_id'];
                }
            } else {
                $category_ids[] = $category->term_id;
            }
        }
        
        // Append categories to post (don't replace existing ones)
        if (!empty($category_ids)) {
            wp_set_post_terms($post_id, $category_ids, 'category', true);
        }
    }
    
    /**
     * Get all ThemisDB-related tags
     * 
     * @return array Array of tag objects
     */
    public function get_themisdb_tags() {
        $tags = get_tags(array(
            'hide_empty' => false,
            'search' => 'ThemisDB'
        ));
        
        return $tags;
    }
    
    /**
     * Get all ThemisDB-related categories
     * 
     * @return array Array of category objects
     */
    public function get_themisdb_categories() {
        $categories = get_categories(array(
            'hide_empty' => false,
            'search' => 'ThemisDB'
        ));
        
        return $categories;
    }
    
    /**
     * Clean up orphaned taxonomies (tags/categories with no posts)
     * 
     * @return array Array with count of deleted tags and categories
     */
    public function cleanup_orphaned_taxonomies() {
        $deleted = array(
            'tags' => 0,
            'categories' => 0
        );
        
        // Clean up tags
        $tags = $this->get_themisdb_tags();
        foreach ($tags as $tag) {
            if ($tag->count == 0) {
                wp_delete_term($tag->term_id, 'post_tag');
                $deleted['tags']++;
            }
        }
        
        // Clean up categories (but not the default "Uncategorized")
        $categories = $this->get_themisdb_categories();
        foreach ($categories as $category) {
            if ($category->count == 0 && $category->term_id != 1) {
                wp_delete_term($category->term_id, 'category');
                $deleted['categories']++;
            }
        }
        
        return $deleted;
    }
}
