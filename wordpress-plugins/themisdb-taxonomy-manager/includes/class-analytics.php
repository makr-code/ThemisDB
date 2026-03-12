<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-analytics.php                                ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:20                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     378                                            ║
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
 * Taxonomy Analytics
 * Provides statistics, similarity calculations, and consolidation suggestions
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Taxonomy_Analytics {
    
    /**
     * Get taxonomy statistics
     * 
     * @return array Statistics data
     */
    public function get_taxonomy_statistics() {
        $categories = get_terms(array(
            'taxonomy' => 'category',
            'hide_empty' => false
        ));
        
        $tags = get_terms(array(
            'taxonomy' => 'post_tag',
            'hide_empty' => false
        ));
        
        // Count unused terms
        $unused_categories = 0;
        foreach ($categories as $cat) {
            if ($cat->count === 0) {
                $unused_categories++;
            }
        }
        
        $unused_tags = 0;
        foreach ($tags as $tag) {
            if ($tag->count === 0) {
                $unused_tags++;
            }
        }
        
        $consolidation_suggestions = $this->get_consolidation_suggestions();
        
        return array(
            'total_categories' => count($categories),
            'total_tags' => count($tags),
            'unused_terms' => $unused_categories + $unused_tags,
            'unused_categories' => $unused_categories,
            'unused_tags' => $unused_tags,
            'consolidation_suggestions' => $consolidation_suggestions
        );
    }
    
    /**
     * Get consolidation suggestions for similar terms
     * 
     * @param float $similarity_threshold Minimum similarity (0-1)
     * @return array Suggestions
     */
    public function get_consolidation_suggestions($similarity_threshold = 0.8) {
        $categories = get_terms(array(
            'taxonomy' => 'category',
            'hide_empty' => false
        ));
        
        $suggestions = array();
        $processed = array();
        
        foreach ($categories as $cat1) {
            if (in_array($cat1->term_id, $processed)) {
                continue;
            }
            
            foreach ($categories as $cat2) {
                if ($cat1->term_id === $cat2->term_id) {
                    continue;
                }
                
                if (in_array($cat2->term_id, $processed)) {
                    continue;
                }
                
                $similarity = $this->calculate_similarity($cat1->name, $cat2->name);
                
                if ($similarity >= $similarity_threshold) {
                    $suggestions[] = array(
                        'id1' => $cat1->term_id,
                        'id2' => $cat2->term_id,
                        'term1' => $cat1->name,
                        'term2' => $cat2->name,
                        'similarity' => $similarity,
                        'post_count' => $cat1->count + $cat2->count
                    );
                    
                    // Mark as processed to avoid duplicate suggestions
                    $processed[] = $cat2->term_id;
                }
            }
        }
        
        return $suggestions;
    }
    
    /**
     * Calculate similarity between two strings
     * Uses Levenshtein distance and similar_text
     * 
     * @param string $str1
     * @param string $str2
     * @return float Similarity score (0-1)
     */
    public function calculate_similarity($str1, $str2) {
        $str1_lower = mb_strtolower($str1, 'UTF-8');
        $str2_lower = mb_strtolower($str2, 'UTF-8');
        
        // Check if one is a substring of the other
        if (strpos($str1_lower, $str2_lower) !== false || strpos($str2_lower, $str1_lower) !== false) {
            return 0.9;
        }
        
        // Use similar_text for percentage similarity
        $percent = 0;
        similar_text($str1_lower, $str2_lower, $percent);
        $similarity = $percent / 100;
        
        // Also check Levenshtein distance for short strings
        if (strlen($str1) < 255 && strlen($str2) < 255) {
            $max_len = max(strlen($str1), strlen($str2));
            if ($max_len > 0) {
                $lev = levenshtein($str1_lower, $str2_lower);
                $lev_similarity = 1 - ($lev / $max_len);
                
                // Use the higher similarity score
                $similarity = max($similarity, $lev_similarity);
            }
        }
        
        return $similarity;
    }
    
    /**
     * Check if two terms are synonyms
     * 
     * @param string $term1
     * @param string $term2
     * @return bool
     */
    public function are_synonyms($term1, $term2) {
        // Define common synonyms for ThemisDB domain
        $synonyms = array(
            array('database', 'db'),
            array('authentication', 'auth'),
            array('authorization', 'authz'),
            array('performance', 'perf'),
            array('optimization', 'optimisation'),
            array('monitoring', 'observability'),
            array('security', 'sec'),
            array('encryption', 'crypto'),
            array('vector search', 'similarity search'),
            array('machine learning', 'ml'),
            array('artificial intelligence', 'ai'),
            array('kubernetes', 'k8s'),
            array('docker', 'container'),
        );
        
        $term1_lower = mb_strtolower($term1, 'UTF-8');
        $term2_lower = mb_strtolower($term2, 'UTF-8');
        
        foreach ($synonyms as $pair) {
            if (($term1_lower === $pair[0] && $term2_lower === $pair[1]) ||
                ($term1_lower === $pair[1] && $term2_lower === $pair[0])) {
                return true;
            }
        }
        
        return false;
    }
    
    /**
     * Get unused terms report
     * 
     * @param string $taxonomy Taxonomy name
     * @return array Unused terms
     */
    public function get_unused_terms($taxonomy = 'category') {
        $terms = get_terms(array(
            'taxonomy' => $taxonomy,
            'hide_empty' => false
        ));
        
        $unused = array();
        foreach ($terms as $term) {
            if ($term->count === 0) {
                $unused[] = array(
                    'term_id' => $term->term_id,
                    'name' => $term->name,
                    'taxonomy' => $taxonomy
                );
            }
        }
        
        return $unused;
    }
    
    /**
     * Cleanup unused terms
     * 
     * @param string $taxonomy Taxonomy name
     * @return int Number of terms deleted
     */
    public function cleanup_unused_terms($taxonomy = 'category') {
        $unused = $this->get_unused_terms($taxonomy);
        $deleted = 0;
        
        foreach ($unused as $term_data) {
            $result = wp_delete_term($term_data['term_id'], $taxonomy);
            if (!is_wp_error($result) && $result) {
                $deleted++;
            }
        }
        
        return $deleted;
    }
    
    /**
     * Consolidate similar categories
     * Merges similar categories based on similarity threshold
     * 
     * @param float $similarity_threshold Minimum similarity (0-1)
     * @return array Consolidation results
     */
    public function consolidate_categories($similarity_threshold = 0.8) {
        $suggestions = $this->get_consolidation_suggestions($similarity_threshold);
        $consolidated = array();
        
        foreach ($suggestions as $suggestion) {
            // Get the actual term objects to check post counts
            $term1 = get_term($suggestion['id1'], 'category');
            $term2 = get_term($suggestion['id2'], 'category');
            
            if (is_wp_error($term1) || is_wp_error($term2)) {
                continue;
            }
            
            // Keep the term with more posts (or lower ID if equal)
            if ($term1->count > $term2->count) {
                $keep_id = $term1->term_id;
                $merge_id = $term2->term_id;
                $keep_name = $term1->name;
                $merge_name = $term2->name;
            } elseif ($term2->count > $term1->count) {
                $keep_id = $term2->term_id;
                $merge_id = $term1->term_id;
                $keep_name = $term2->name;
                $merge_name = $term1->name;
            } else {
                // Equal counts, keep the one with lower ID
                $keep_id = ($term1->term_id < $term2->term_id) ? $term1->term_id : $term2->term_id;
                $merge_id = ($term1->term_id < $term2->term_id) ? $term2->term_id : $term1->term_id;
                $keep_name = ($term1->term_id < $term2->term_id) ? $term1->name : $term2->name;
                $merge_name = ($term1->term_id < $term2->term_id) ? $term2->name : $term1->name;
            }
            
            // Get posts with the merge category
            $posts = get_posts(array(
                'category' => $merge_id,
                'posts_per_page' => -1,
                'fields' => 'ids',
                'post_status' => 'any'
            ));
            
            // Reassign posts to keep category
            foreach ($posts as $post_id) {
                $current_cats = wp_get_post_categories($post_id);
                $current_cats[] = $keep_id;
                
                // Remove merge category
                $current_cats = array_diff($current_cats, array($merge_id));
                
                wp_set_post_categories($post_id, $current_cats);
            }
            
            // Delete merged category
            $result = wp_delete_term($merge_id, 'category');
            
            if (!is_wp_error($result)) {
                $consolidated[] = array(
                    'kept' => $keep_name,
                    'merged' => $merge_name,
                    'posts_moved' => count($posts)
                );
            }
        }
        
        return array(
            'total_merged' => count($consolidated),
            'details' => $consolidated
        );
    }
    
    /**
     * Get category distribution statistics
     * 
     * @return array Distribution data
     */
    public function get_category_distribution() {
        $categories = get_terms(array(
            'taxonomy' => 'category',
            'hide_empty' => false
        ));
        
        $distribution = array(
            'total' => count($categories),
            'with_posts' => 0,
            'empty' => 0,
            'top_level' => 0,
            'with_children' => 0,
            'max_depth' => 0,
            'avg_posts_per_category' => 0
        );
        
        $total_posts = 0;
        
        foreach ($categories as $cat) {
            if ($cat->count > 0) {
                $distribution['with_posts']++;
                $total_posts += $cat->count;
            } else {
                $distribution['empty']++;
            }
            
            if ($cat->parent === 0) {
                $distribution['top_level']++;
            }
            
            // Check for children
            $children = get_terms(array(
                'taxonomy' => 'category',
                'parent' => $cat->term_id,
                'hide_empty' => false
            ));
            
            if (!empty($children)) {
                $distribution['with_children']++;
            }
        }
        
        if ($distribution['with_posts'] > 0) {
            $distribution['avg_posts_per_category'] = round($total_posts / $distribution['with_posts'], 2);
        }
        
        return $distribution;
    }
}
