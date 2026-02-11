<?php
/**
 * Taxonomy Manager
 * Main class that coordinates taxonomy extraction and assignment
 * Uses both extractor and hierarchy manager
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Taxonomy_Manager {
    
    /**
     * Taxonomy extractor instance
     */
    private $extractor;
    
    /**
     * Category hierarchy manager instance
     */
    private $hierarchy;
    
    /**
     * Constructor
     */
    public function __construct() {
        $this->extractor = new ThemisDB_Taxonomy_Extractor();
        $this->hierarchy = new ThemisDB_Category_Hierarchy();
        
        // Hook into post save
        add_action('save_post', array($this, 'auto_assign_taxonomies'), 10, 3);
        
        // Add REST API endpoint for external use
        add_action('rest_api_init', array($this, 'register_rest_routes'));
    }
    
    /**
     * Auto-assign taxonomies when post is saved
     * 
     * @param int $post_id
     * @param WP_Post $post
     * @param bool $update
     */
    public function auto_assign_taxonomies($post_id, $post, $update) {
        // Check if auto-extraction is enabled
        if (!get_option('themisdb_taxonomy_auto_extract', 1)) {
            return;
        }
        
        // Avoid auto-save and revisions
        if (defined('DOING_AUTOSAVE') && DOING_AUTOSAVE) {
            return;
        }
        
        // Only process posts and pages
        if (!in_array($post->post_type, array('post', 'page'))) {
            return;
        }
        
        // Check permissions
        if (!current_user_can('edit_post', $post_id)) {
            return;
        }
        
        // Extract taxonomies
        $result = $this->extractor->extract_taxonomies($post, array(
            'extract_from_content' => get_option('themisdb_taxonomy_auto_categories', 1),
            'extract_from_metadata' => true
        ));
        
        // Assign categories with hierarchy
        if (get_option('themisdb_taxonomy_auto_categories', 1)) {
            $this->assign_categories_with_hierarchy($post_id, $result['categories']);
        }
        
        // Assign tags
        if (get_option('themisdb_taxonomy_auto_tags', 1)) {
            $this->assign_tags($post_id, $result['tags']);
        }
    }
    
    /**
     * Assign categories with proper hierarchy
     * 
     * @param int $post_id
     * @param array $category_names
     */
    public function assign_categories_with_hierarchy($post_id, $category_names) {
        if (empty($category_names)) {
            return;
        }
        
        $category_ids = array();
        
        foreach ($category_names as $cat_name) {
            // Create or get category with proper hierarchy
            $cat_id = $this->hierarchy->get_or_create_hierarchical_category($cat_name);
            
            if (!is_wp_error($cat_id)) {
                $category_ids[] = $cat_id;
            }
        }
        
        // Assign categories to post (append, don't replace)
        if (!empty($category_ids)) {
            wp_set_post_categories($post_id, $category_ids, true);
        }
    }
    
    /**
     * Assign tags to post
     * 
     * @param int $post_id
     * @param array $tag_names
     */
    public function assign_tags($post_id, $tag_names) {
        if (empty($tag_names)) {
            return;
        }
        
        $tag_ids = array();
        
        foreach ($tag_names as $tag_name) {
            $tag = get_term_by('name', $tag_name, 'post_tag');
            
            if (!$tag) {
                $result = wp_insert_term($tag_name, 'post_tag');
                if (!is_wp_error($result)) {
                    $tag_ids[] = $result['term_id'];
                }
            } else {
                $tag_ids[] = $tag->term_id;
            }
        }
        
        // Assign tags to post (append, don't replace)
        if (!empty($tag_ids)) {
            wp_set_post_terms($post_id, $tag_ids, 'post_tag', true);
        }
    }
    
    /**
     * Extract and assign taxonomies for a batch of posts
     * 
     * @param array $post_ids Array of post IDs
     * @param array $options Extraction options
     * @return array Statistics
     */
    public function batch_assign_taxonomies($post_ids, $options = array()) {
        $stats = array(
            'processed' => 0,
            'categories_assigned' => 0,
            'tags_assigned' => 0,
            'errors' => 0
        );
        
        foreach ($post_ids as $post_id) {
            try {
                $post = get_post($post_id);
                if (!$post) {
                    continue;
                }
                
                $result = $this->extractor->extract_taxonomies($post, $options);
                
                $this->assign_categories_with_hierarchy($post_id, $result['categories']);
                $this->assign_tags($post_id, $result['tags']);
                
                $stats['processed']++;
                $stats['categories_assigned'] += count($result['categories']);
                $stats['tags_assigned'] += count($result['tags']);
                
            } catch (Exception $e) {
                $stats['errors']++;
            }
        }
        
        return $stats;
    }
    
    /**
     * Consolidate existing categories using hierarchy rules
     * 
     * @return array Statistics
     */
    public function consolidate_categories() {
        if (!get_option('themisdb_taxonomy_consolidate_categories', 1)) {
            return array('message' => 'Consolidation disabled');
        }
        
        return $this->hierarchy->consolidate_existing_categories();
    }
    
    /**
     * Get optimization recommendations
     * 
     * @return array Recommendations
     */
    public function get_optimization_recommendations() {
        return $this->hierarchy->get_optimization_recommendations();
    }
    
    /**
     * Register REST API routes
     */
    public function register_rest_routes() {
        register_rest_route('themisdb/v1', '/taxonomy/extract', array(
            'methods' => 'POST',
            'callback' => array($this, 'rest_extract_taxonomies'),
            'permission_callback' => function() {
                return current_user_can('edit_posts');
            }
        ));
        
        register_rest_route('themisdb/v1', '/taxonomy/consolidate', array(
            'methods' => 'POST',
            'callback' => array($this, 'rest_consolidate_categories'),
            'permission_callback' => function() {
                return current_user_can('manage_categories');
            }
        ));
        
        register_rest_route('themisdb/v1', '/taxonomy/recommendations', array(
            'methods' => 'GET',
            'callback' => array($this, 'rest_get_recommendations'),
            'permission_callback' => function() {
                return current_user_can('manage_categories');
            }
        ));
    }
    
    /**
     * REST API: Extract taxonomies
     */
    public function rest_extract_taxonomies($request) {
        $post_id = $request->get_param('post_id');
        $text = $request->get_param('text');
        $options = $request->get_param('options') ?: array();
        
        if ($post_id) {
            $post = get_post($post_id);
            if (!$post) {
                return new WP_Error('invalid_post', 'Post not found', array('status' => 404));
            }
            
            $result = $this->extractor->extract_taxonomies($post, $options);
        } elseif ($text) {
            // Create temporary post object for extraction
            $temp_post = new stdClass();
            $temp_post->post_title = $request->get_param('title') ?: '';
            $temp_post->post_content = $text;
            $temp_post->ID = 0;
            
            $result = $this->extractor->extract_taxonomies($temp_post, $options);
        } else {
            return new WP_Error('missing_params', 'Either post_id or text required', array('status' => 400));
        }
        
        return rest_ensure_response($result);
    }
    
    /**
     * REST API: Consolidate categories
     */
    public function rest_consolidate_categories($request) {
        $stats = $this->consolidate_categories();
        return rest_ensure_response($stats);
    }
    
    /**
     * REST API: Get recommendations
     */
    public function rest_get_recommendations($request) {
        $recommendations = $this->get_optimization_recommendations();
        return rest_ensure_response($recommendations);
    }
    
    /**
     * Get extractor instance
     */
    public function get_extractor() {
        return $this->extractor;
    }
    
    /**
     * Get hierarchy manager instance
     */
    public function get_hierarchy() {
        return $this->hierarchy;
    }
}
