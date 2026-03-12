<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-tfidf.php                                    ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:21                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     197                                            ║
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
 * TF-IDF Calculator
 * Calculates Term Frequency-Inverse Document Frequency for relevance scoring
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_TFIDF {
    
    /**
     * Cache for document frequencies
     */
    private $document_frequencies = array();
    
    /**
     * Total document count
     */
    private $total_documents = 0;
    
    /**
     * Constructor
     */
    public function __construct() {
        $this->total_documents = $this->get_total_post_count();
    }
    
    /**
     * Calculate TF-IDF score for a term in text
     * 
     * @param string $term The term to score
     * @param string $text The document text
     * @return float TF-IDF score
     */
    public function calculate_tfidf($term, $text) {
        $tf = $this->calculate_term_frequency($term, $text);
        $idf = $this->calculate_inverse_document_frequency($term);
        
        return $tf * $idf;
    }
    
    /**
     * Calculate Term Frequency
     * TF = (Number of times term appears in document) / (Total terms in document)
     * 
     * @param string $term
     * @param string $text
     * @return float
     */
    private function calculate_term_frequency($term, $text) {
        $text_lower = mb_strtolower($text, 'UTF-8');
        $term_lower = mb_strtolower($term, 'UTF-8');
        
        // Count occurrences
        $term_count = substr_count($text_lower, $term_lower);
        
        // Count total words
        $words = preg_split('/\s+/', $text_lower, -1, PREG_SPLIT_NO_EMPTY);
        $total_words = count($words);
        
        if ($total_words === 0) {
            return 0;
        }
        
        return $term_count / $total_words;
    }
    
    /**
     * Calculate Inverse Document Frequency
     * IDF = log(Total Documents / Documents containing term)
     * 
     * @param string $term
     * @return float
     */
    private function calculate_inverse_document_frequency($term) {
        $doc_frequency = $this->get_term_document_frequency($term);
        
        if ($doc_frequency === 0) {
            // New term, high relevance
            return log($this->total_documents + 1);
        }
        
        return log($this->total_documents / $doc_frequency);
    }
    
    /**
     * Get document frequency for a term
     * Count how many posts contain this term
     * 
     * @param string $term
     * @return int
     */
    private function get_term_document_frequency($term) {
        // Check cache first
        $cache_key = md5($term);
        if (isset($this->document_frequencies[$cache_key])) {
            return $this->document_frequencies[$cache_key];
        }
        
        global $wpdb;
        
        $term_lower = mb_strtolower($term, 'UTF-8');
        
        // Search in post title and content
        $query = $wpdb->prepare(
            "SELECT COUNT(DISTINCT ID) 
             FROM {$wpdb->posts} 
             WHERE post_status = 'publish' 
             AND post_type = 'post'
             AND (LOWER(post_title) LIKE %s OR LOWER(post_content) LIKE %s)",
            '%' . $wpdb->esc_like($term_lower) . '%',
            '%' . $wpdb->esc_like($term_lower) . '%'
        );
        
        $count = (int) $wpdb->get_var($query);
        
        // Cache the result
        $this->document_frequencies[$cache_key] = $count;
        
        return $count;
    }
    
    /**
     * Get total published post count
     * 
     * @return int
     */
    private function get_total_post_count() {
        $count = wp_count_posts('post');
        return isset($count->publish) ? (int) $count->publish : 1;
    }
    
    /**
     * Score multiple terms and return sorted by relevance
     * 
     * @param array $terms Array of terms to score
     * @param string $text Document text
     * @param int $limit Maximum number of terms to return
     * @return array Sorted array of terms with scores
     */
    public function score_terms($terms, $text, $limit = 10) {
        $scored_terms = array();
        
        foreach ($terms as $term) {
            $score = $this->calculate_tfidf($term, $text);
            
            $scored_terms[] = array(
                'term' => $term,
                'score' => $score
            );
        }
        
        // Sort by score descending
        usort($scored_terms, function($a, $b) {
            return $b['score'] <=> $a['score'];
        });
        
        // Apply limit
        if ($limit > 0) {
            $scored_terms = array_slice($scored_terms, 0, $limit);
        }
        
        return $scored_terms;
    }
    
    /**
     * Clear document frequency cache
     */
    public function clear_cache() {
        $this->document_frequencies = array();
        $this->total_documents = $this->get_total_post_count();
    }
}
