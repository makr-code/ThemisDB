<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-taxonomy-manager.php                         ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:17                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     399                                            ║
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
 * Taxonomy Manager
 * Automatically extracts and creates tags and categories from post/page content
 * Uses text analysis techniques including word frequency, relevance, and best practices
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Downloads_Taxonomy_Manager {
    
    /**
     * Common stop words to exclude from tag extraction (German and English)
     */
    private $stop_words = array(
        // German stop words
        'der', 'die', 'das', 'und', 'oder', 'aber', 'ist', 'sind', 'ein', 'eine', 'einen', 'einer',
        'mit', 'von', 'zu', 'auf', 'für', 'in', 'im', 'an', 'am', 'bei', 'nach', 'vor', 'über',
        'unter', 'durch', 'um', 'aus', 'dem', 'den', 'des', 'als', 'auch', 'nur', 'noch', 'nicht',
        'sich', 'sein', 'seine', 'ihr', 'ihre', 'haben', 'hat', 'wird', 'werden', 'wurde', 'wurden',
        'kann', 'können', 'muss', 'soll', 'sollte', 'würde', 'diese', 'dieser', 'dieses', 'dass',
        'wenn', 'ob', 'wie', 'was', 'wer', 'wo', 'wann', 'warum', 'welche', 'welcher', 'welches',
        // English stop words
        'the', 'is', 'are', 'was', 'were', 'be', 'been', 'being', 'have', 'has', 'had', 'do', 'does',
        'did', 'will', 'would', 'should', 'could', 'may', 'might', 'must', 'can', 'a', 'an', 'and',
        'or', 'but', 'if', 'then', 'else', 'when', 'at', 'by', 'for', 'with', 'about', 'against',
        'between', 'into', 'through', 'during', 'before', 'after', 'above', 'below', 'to', 'from',
        'up', 'down', 'in', 'out', 'on', 'off', 'over', 'under', 'again', 'further', 'once', 'here',
        'there', 'all', 'both', 'each', 'few', 'more', 'most', 'other', 'some', 'such', 'only',
        'own', 'same', 'so', 'than', 'too', 'very', 'this', 'that', 'these', 'those'
    );
    
    /**
     * Minimum word length for tag extraction
     */
    private $min_word_length = 3;
    
    /**
     * Maximum number of tags to extract
     */
    private $max_tags = 15;
    
    /**
     * Maximum number of categories to extract
     */
    private $max_categories = 5;
    
    /**
     * Constructor
     */
    public function __construct() {
        // Hook into post save to auto-assign taxonomies
        add_action('save_post', array($this, 'auto_assign_taxonomies'), 10, 3);
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
        
        // Only process posts and pages
        if (!in_array($post->post_type, array('post', 'page'))) {
            return;
        }
        
        // Check user permissions
        if (!current_user_can('edit_post', $post_id)) {
            return;
        }
        
        // Extract and assign taxonomies from post content
        $this->extract_and_assign_taxonomies_from_content($post);
    }
    
    /**
     * Extract and assign taxonomies from post content
     * 
     * @param WP_Post $post The post object
     */
    private function extract_and_assign_taxonomies_from_content($post) {
        // Combine title and content for analysis
        $text = $post->post_title . ' ' . $post->post_content;
        
        // Strip shortcodes and HTML tags
        $text = strip_shortcodes($text);
        $text = wp_strip_all_tags($text);
        
        // Extract tags if enabled
        if (get_option('themisdb_auto_tags', 1)) {
            $tags = $this->extract_tags_from_text($text, $post->post_title);
            $this->assign_tags($post->ID, $tags);
        }
        
        // Extract categories if enabled
        if (get_option('themisdb_auto_categories', 1)) {
            $categories = $this->extract_categories_from_text($text, $post->post_title);
            $this->assign_categories($post->ID, $categories);
        }
    }
    
    /**
     * Extract tags from text using frequency and relevance analysis
     * 
     * @param string $text The text to analyze
     * @param string $title The post title (given higher weight)
     * @return array Array of tag names
     */
    private function extract_tags_from_text($text, $title = '') {
        // Store original text for capitalization check
        $original_text = $text;
        
        // Tokenize text into words
        $words = $this->tokenize_text($text);
        $title_words = $this->tokenize_text($title);
        
        // Calculate word frequencies
        $word_freq = array_count_values($words);
        
        // Give title words higher weight (3x frequency)
        foreach ($title_words as $word) {
            if (isset($word_freq[$word])) {
                $word_freq[$word] *= 3;
            }
        }
        
        // Filter and score words
        $scored_words = array();
        foreach ($word_freq as $word => $freq) {
            // Skip if word is too short
            if (mb_strlen($word) < $this->min_word_length) {
                continue;
            }
            
            // Skip stop words
            if (in_array($word, $this->stop_words)) {
                continue;
            }
            
            // Skip if word is purely numeric
            if (is_numeric($word)) {
                continue;
            }
            
            // Calculate relevance score
            // Score = frequency * word_length_factor * capitalization_bonus
            $score = $freq;
            
            // Longer words are generally more meaningful
            if (mb_strlen($word) > 6) {
                $score *= 1.5;
            }
            
            // Check if word appears capitalized in original text
            $capitalized_word = mb_convert_case($word, MB_CASE_TITLE, 'UTF-8');
            if (mb_stripos($original_text, $capitalized_word) !== false && mb_strlen($word) > 3) {
                $score *= 1.3;
            }
            
            $scored_words[$word] = $score;
        }
        
        // Sort by score descending
        arsort($scored_words);
        
        // Get top N tags
        $tags = array_keys(array_slice($scored_words, 0, $this->max_tags));
        
        // Capitalize first letter only (preserve rest of word)
        $tags = array_map(function($tag) {
            return mb_strtoupper(mb_substr($tag, 0, 1)) . mb_substr($tag, 1);
        }, $tags);
        
        return $tags;
    }
    
    /**
     * Extract categories from text using phrase analysis
     * 
     * @param string $text The text to analyze
     * @param string $title The post title
     * @return array Array of category names
     */
    private function extract_categories_from_text($text, $title = '') {
        $categories = array();
        
        // Extract bigrams and trigrams (2-3 word phrases)
        $phrases = $this->extract_phrases($text . ' ' . $title);
        
        // Score phrases
        $scored_phrases = array();
        foreach ($phrases as $phrase => $freq) {
            // Skip if phrase is too short
            if (mb_strlen($phrase) < 5) {
                continue;
            }
            
            // Check if phrase appears in title (higher relevance)
            $score = $freq;
            if (mb_stripos($title, $phrase) !== false) {
                $score *= 2;
            }
            
            $scored_phrases[$phrase] = $score;
        }
        
        // Sort by score
        arsort($scored_phrases);
        
        // Get top N categories
        $categories = array_keys(array_slice($scored_phrases, 0, $this->max_categories));
        
        // Capitalize first letter of each word (preserve rest)
        $categories = array_map(function($cat) {
            $words = explode(' ', $cat);
            $words = array_map(function($word) {
                return mb_strtoupper(mb_substr($word, 0, 1)) . mb_substr($word, 1);
            }, $words);
            return implode(' ', $words);
        }, $categories);
        
        return $categories;
    }
    
    /**
     * Tokenize text into words
     * 
     * @param string $text The text to tokenize
     * @return array Array of words
     */
    private function tokenize_text($text) {
        // Convert to lowercase
        $text = mb_strtolower($text, 'UTF-8');
        
        // Remove special characters but keep umlauts and accented characters
        $text = preg_replace('/[^\p{L}\p{N}\s]/u', ' ', $text);
        
        // Split into words
        $words = preg_split('/\s+/', $text, -1, PREG_SPLIT_NO_EMPTY);
        
        return $words;
    }
    
    /**
     * Extract phrases (bigrams and trigrams) from text
     * 
     * @param string $text The text to analyze
     * @return array Array of phrases with frequencies
     */
    private function extract_phrases($text) {
        $words = $this->tokenize_text($text);
        $phrases = array();
        
        // Extract bigrams (2-word phrases)
        for ($i = 0; $i < count($words) - 1; $i++) {
            $word1 = $words[$i];
            $word2 = $words[$i + 1];
            
            // Skip if contains stop words
            if (in_array($word1, $this->stop_words) || in_array($word2, $this->stop_words)) {
                continue;
            }
            
            $phrase = $word1 . ' ' . $word2;
            if (!isset($phrases[$phrase])) {
                $phrases[$phrase] = 0;
            }
            $phrases[$phrase]++;
        }
        
        // Extract trigrams (3-word phrases)
        for ($i = 0; $i < count($words) - 2; $i++) {
            $word1 = $words[$i];
            $word2 = $words[$i + 1];
            $word3 = $words[$i + 2];
            
            // Skip if contains stop words
            if (in_array($word1, $this->stop_words) || 
                in_array($word2, $this->stop_words) || 
                in_array($word3, $this->stop_words)) {
                continue;
            }
            
            $phrase = $word1 . ' ' . $word2 . ' ' . $word3;
            if (!isset($phrases[$phrase])) {
                $phrases[$phrase] = 0;
            }
            $phrases[$phrase]++;
        }
        
        return $phrases;
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
}
