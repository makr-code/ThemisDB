<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-category-hierarchy.php                       ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:21                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     338                                            ║
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
 * Category Hierarchy Manager
 * Handles hierarchical category structures up to 3 levels deep
 * and consolidates categories to minimize redundancy
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Category_Hierarchy {
    
    /**
     * Maximum category depth
     */
    private $max_depth = 3;
    
    /**
     * Category hierarchy rules
     * Defines parent-child relationships
     */
    private $hierarchy_rules = array(
        // Parent => Children
        'Documentation' => array('Guides', 'API Reference', 'Architecture'),
        'Development' => array('Tools', 'Client SDKs', 'Plugins'),
        'Security' => array('Authentication', 'Encryption', 'Compliance', 'Audit'),
        'Operations' => array('Deployment', 'Monitoring & Observability', 'Performance'),
        'Features' => array('LLM Integration', 'Vector Search', 'Time-Series', 'Geospatial'),
        'Enterprise' => array('Enterprise Features', 'Governance', 'Legal', 'Policies'),
        'Data Management' => array('Storage', 'Sharding', 'Replication', 'Backup'),
        'Query' => array('AQL Query Language', 'Query Language', 'Search'),
        'Integration' => array('Integrations', 'Connectors', 'APIs'),
    );
    
    /**
     * Category consolidation rules
     * Maps similar categories to canonical names
     */
    private $consolidation_rules = array(
        'Monitoring' => 'Monitoring & Observability',
        'Observability' => 'Monitoring & Observability',
        'AQL' => 'AQL Query Language',
        'Query Language' => 'AQL Query Language',
        'APIs' => 'API Reference',
        'API' => 'API Reference',
        'Auth' => 'Authentication',
        'Compliance & Governance' => 'Compliance',
        'Time Series' => 'Time-Series',
        'TimeSeries' => 'Time-Series',
        'Vector' => 'Vector Search',
        'Geo' => 'Geospatial',
        'GIS' => 'Geospatial',
        'SDK' => 'Client SDKs',
        'Clients' => 'Client SDKs',
    );
    
    /**
     * Constructor
     */
    public function __construct() {
        $this->max_depth = get_option('themisdb_taxonomy_max_category_depth', 3);
        
        // Allow customization via filter
        $this->hierarchy_rules = apply_filters('themisdb_category_hierarchy_rules', $this->hierarchy_rules);
        $this->consolidation_rules = apply_filters('themisdb_category_consolidation_rules', $this->consolidation_rules);
    }
    
    /**
     * Consolidate category name using consolidation rules
     * 
     * @param string $category_name The category name to consolidate
     * @return string The consolidated category name
     */
    public function consolidate_category($category_name) {
        if (isset($this->consolidation_rules[$category_name])) {
            return $this->consolidation_rules[$category_name];
        }
        return $category_name;
    }
    
    /**
     * Get parent category for a given category
     * 
     * @param string $category_name The category name
     * @return string|null The parent category name or null
     */
    public function get_parent_category($category_name) {
        // First consolidate the category
        $category_name = $this->consolidate_category($category_name);
        
        // Search for parent in hierarchy rules
        foreach ($this->hierarchy_rules as $parent => $children) {
            if (in_array($category_name, $children)) {
                return $parent;
            }
        }
        
        return null;
    }
    
    /**
     * Get full category path (parent > child > grandchild)
     * 
     * @param string $category_name The category name
     * @return array Array of category names from root to leaf
     */
    public function get_category_path($category_name) {
        $path = array();
        $current = $category_name;
        $depth = 0;
        
        // Build path from leaf to root
        while ($current && $depth < $this->max_depth) {
            $current = $this->consolidate_category($current);
            array_unshift($path, $current);
            $current = $this->get_parent_category($current);
            $depth++;
        }
        
        return $path;
    }
    
    /**
     * Create or get category with proper parent-child hierarchy
     * 
     * @param string $category_name The category name
     * @return int|WP_Error The category term ID or WP_Error
     */
    public function get_or_create_hierarchical_category($category_name) {
        // Consolidate the category name
        $category_name = $this->consolidate_category($category_name);
        
        // Get full category path
        $path = $this->get_category_path($category_name);
        
        // Create categories from root to leaf
        $parent_id = 0;
        $category_id = null;
        
        foreach ($path as $cat_name) {
            // Check if category exists
            $term = get_term_by('name', $cat_name, 'category');
            
            if ($term) {
                $category_id = $term->term_id;
                
                // Update parent if needed
                if ($parent_id > 0 && $term->parent != $parent_id) {
                    wp_update_term($category_id, 'category', array(
                        'parent' => $parent_id
                    ));
                }
            } else {
                // Create new category with parent
                $result = wp_insert_term($cat_name, 'category', array(
                    'parent' => $parent_id
                ));
                
                if (is_wp_error($result)) {
                    return $result;
                }
                
                $category_id = $result['term_id'];
            }
            
            // This category becomes parent for next level
            $parent_id = $category_id;
        }
        
        return $category_id;
    }
    
    /**
     * Get all child categories of a parent
     * 
     * @param string $parent_name The parent category name
     * @return array Array of child category names
     */
    public function get_child_categories($parent_name) {
        if (isset($this->hierarchy_rules[$parent_name])) {
            return $this->hierarchy_rules[$parent_name];
        }
        return array();
    }
    
    /**
     * Check if a category should be a child of another
     * 
     * @param string $child_name The potential child category
     * @param string $parent_name The potential parent category
     * @return bool True if child should be under parent
     */
    public function is_child_of($child_name, $parent_name) {
        $children = $this->get_child_categories($parent_name);
        return in_array($child_name, $children);
    }
    
    /**
     * Optimize category structure by consolidating similar categories
     * 
     * @return array Statistics about consolidation
     */
    public function consolidate_existing_categories() {
        $stats = array(
            'consolidated' => 0,
            'hierarchized' => 0,
            'errors' => 0
        );
        
        // Get all categories
        $categories = get_categories(array(
            'hide_empty' => false
        ));
        
        foreach ($categories as $category) {
            // Check if this category should be consolidated
            $canonical_name = $this->consolidate_category($category->name);
            
            if ($canonical_name !== $category->name) {
                // Find or create canonical category
                $canonical_term = get_term_by('name', $canonical_name, 'category');
                
                if ($canonical_term) {
                    // Move posts from old category to canonical
                    $posts = get_posts(array(
                        'category' => $category->term_id,
                        'posts_per_page' => -1,
                        'fields' => 'ids'
                    ));
                    
                    foreach ($posts as $post_id) {
                        wp_set_post_categories($post_id, array($canonical_term->term_id), true);
                        wp_remove_object_terms($post_id, $category->term_id, 'category');
                    }
                    
                    // Delete old category if empty
                    if (empty(get_posts(array('category' => $category->term_id, 'posts_per_page' => 1)))) {
                        wp_delete_term($category->term_id, 'category');
                        $stats['consolidated']++;
                    }
                }
            }
            
            // Check if category should have a parent
            $parent_name = $this->get_parent_category($category->name);
            if ($parent_name && $category->parent == 0) {
                $parent_term = get_term_by('name', $parent_name, 'category');
                
                if (!$parent_term) {
                    // Create parent category
                    $result = wp_insert_term($parent_name, 'category');
                    if (!is_wp_error($result)) {
                        $parent_term = get_term($result['term_id'], 'category');
                    }
                }
                
                if ($parent_term) {
                    wp_update_term($category->term_id, 'category', array(
                        'parent' => $parent_term->term_id
                    ));
                    $stats['hierarchized']++;
                }
            }
        }
        
        return $stats;
    }
    
    /**
     * Get recommended categories for optimization
     * Returns categories that could be consolidated or hierarchized
     * 
     * @return array Array of recommendations
     */
    public function get_optimization_recommendations() {
        $recommendations = array();
        
        $categories = get_categories(array('hide_empty' => false));
        
        foreach ($categories as $category) {
            $rec = array(
                'current_name' => $category->name,
                'current_id' => $category->term_id,
                'post_count' => $category->count,
                'actions' => array()
            );
            
            // Check for consolidation
            $canonical = $this->consolidate_category($category->name);
            if ($canonical !== $category->name) {
                $rec['actions'][] = array(
                    'type' => 'consolidate',
                    'target' => $canonical,
                    'reason' => 'Similar category exists'
                );
            }
            
            // Check for missing parent
            $parent_name = $this->get_parent_category($category->name);
            if ($parent_name && $category->parent == 0) {
                $rec['actions'][] = array(
                    'type' => 'add_parent',
                    'target' => $parent_name,
                    'reason' => 'Should be child of ' . $parent_name
                );
            }
            
            if (!empty($rec['actions'])) {
                $recommendations[] = $rec;
            }
        }
        
        return $recommendations;
    }
}
