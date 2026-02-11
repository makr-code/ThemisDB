<?php
/**
 * Taxonomy Extractor
 * Combines content-based and structure-based taxonomy extraction
 * Extracts from both post content (text analysis) and metadata (file paths, frontmatter)
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Taxonomy_Extractor {
    
    /**
     * Stop words for text analysis (German and English)
     */
    private $stop_words = array(
        // German
        'der', 'die', 'das', 'und', 'oder', 'aber', 'ist', 'sind', 'ein', 'eine',
        'mit', 'von', 'zu', 'auf', 'für', 'in', 'im', 'an', 'bei', 'nach',
        // English
        'the', 'is', 'are', 'was', 'were', 'be', 'have', 'has', 'had', 'do',
        'a', 'an', 'and', 'or', 'but', 'if', 'for', 'with', 'about', 'at'
    );
    
    /**
     * Patterns to exclude from categories/tags
     */
    private $exclude_patterns = array(
        '/^\d+$/',              // Pure numbers
        '/^\d{4}$/',            // Years
        '/^v?\d+\.\d+/',        // Version numbers
        '/^(januar|februar|märz|april|mai|juni|juli|august|september|oktober|november|dezember)$/i',
        '/^(january|february|march|april|may|june|july|august|september|october|november|december)$/i',
        '/^\d+\s+\d+$/',        // "9 2026" patterns
        '/^(de|en|fr|es|ja)$/', // Language codes
        '/^(use|tmp|test)$/i',  // Generic words
    );
    
    /**
     * Key topics for tag extraction
     */
    private $key_topics = array(
        'vector search', 'graph database', 'time-series', 'llm', 'ai', 'machine learning',
        'security', 'encryption', 'authentication', 'compliance',
        'docker', 'kubernetes', 'monitoring', 'performance', 'backup',
        'multi-model', 'query', 'api', 'rest', 'grpc', 'integration'
    );
    
    /**
     * Extract categories and tags from post
     * 
     * @param WP_Post|int $post Post object or ID
     * @param array $options Extraction options
     * @return array Array with 'categories' and 'tags'
     */
    public function extract_taxonomies($post, $options = array()) {
        if (is_numeric($post)) {
            $post = get_post($post);
        }
        
        if (!$post) {
            return array('categories' => array(), 'tags' => array());
        }
        
        $defaults = array(
            'extract_from_content' => true,
            'extract_from_metadata' => true,
            'extract_from_path' => false,
            'max_categories' => 5,
            'max_tags' => 15,
            'path_info' => null // For structure-based extraction
        );
        
        $options = wp_parse_args($options, $defaults);
        
        $categories = array();
        $tags = array();
        
        // Extract from content (text analysis)
        if ($options['extract_from_content']) {
            $content_result = $this->extract_from_content($post);
            $categories = array_merge($categories, $content_result['categories']);
            $tags = array_merge($tags, $content_result['tags']);
        }
        
        // Extract from metadata (post meta, custom fields)
        if ($options['extract_from_metadata']) {
            $meta_result = $this->extract_from_metadata($post);
            $categories = array_merge($categories, $meta_result['categories']);
            $tags = array_merge($tags, $meta_result['tags']);
        }
        
        // Extract from file path (for documentation imports)
        if ($options['extract_from_path'] && $options['path_info']) {
            $path_result = $this->extract_from_path($options['path_info']);
            $categories = array_merge($categories, $path_result['categories']);
            $tags = array_merge($tags, $path_result['tags']);
        }
        
        // Remove duplicates and filter
        $categories = $this->filter_and_deduplicate($categories);
        $tags = $this->filter_and_deduplicate($tags);
        
        // Limit to max counts
        $categories = array_slice($categories, 0, $options['max_categories']);
        $tags = array_slice($tags, 0, $options['max_tags']);
        
        return array(
            'categories' => $categories,
            'tags' => $tags
        );
    }
    
    /**
     * Extract from post content using text analysis
     * 
     * @param WP_Post $post
     * @return array
     */
    private function extract_from_content($post) {
        $text = $post->post_title . ' ' . $post->post_content;
        $text = strip_shortcodes($text);
        $text = wp_strip_all_tags($text);
        
        $categories = $this->extract_phrases($text);
        $tags = $this->extract_keywords($text, $post->post_title);
        
        return array(
            'categories' => $categories,
            'tags' => $tags
        );
    }
    
    /**
     * Extract from post metadata
     * 
     * @param WP_Post $post
     * @return array
     */
    private function extract_from_metadata($post) {
        $categories = array();
        $tags = array();
        
        // Check for explicit categories in meta
        $meta_categories = get_post_meta($post->ID, '_themisdb_categories', true);
        if ($meta_categories && is_array($meta_categories)) {
            $categories = array_merge($categories, $meta_categories);
        }
        
        // Check for explicit tags in meta
        $meta_tags = get_post_meta($post->ID, '_themisdb_tags', true);
        if ($meta_tags && is_array($meta_tags)) {
            $tags = array_merge($tags, $meta_tags);
        }
        
        // Check file path if stored
        $file_path = get_post_meta($post->ID, '_themisdb_file_path', true);
        if ($file_path) {
            $path_result = $this->extract_from_path($file_path);
            $categories = array_merge($categories, $path_result['categories']);
        }
        
        return array(
            'categories' => $categories,
            'tags' => $tags
        );
    }
    
    /**
     * Extract from file path (structure-based)
     * 
     * @param string $path File path
     * @return array
     */
    private function extract_from_path($path) {
        $categories = array();
        $tags = array();
        
        // Semantic mapping for common directory names
        $path_mapping = array(
            'security' => 'Security',
            'guides' => 'Guides',
            'api' => 'API Reference',
            'architecture' => 'Architecture',
            'deployment' => 'Deployment',
            'features' => 'Features',
            'llm' => 'LLM Integration',
            'performance' => 'Performance',
            'monitoring' => 'Monitoring & Observability',
        );
        
        // Extract directory names from path
        $path_parts = explode('/', trim($path, '/'));
        
        foreach ($path_parts as $part) {
            $part_lower = strtolower($part);
            
            // Skip language codes and common patterns
            if (preg_match('/^(de|en|fr|es|ja|docs?)$/', $part_lower)) {
                continue;
            }
            
            // Check mapping
            if (isset($path_mapping[$part_lower])) {
                $categories[] = $path_mapping[$part_lower];
            } elseif ($this->is_valid_category($part)) {
                // Capitalize and clean up
                $clean = str_replace(array('_', '-'), ' ', $part);
                $clean = ucwords($clean);
                $categories[] = $clean;
            }
        }
        
        return array(
            'categories' => $categories,
            'tags' => $tags
        );
    }
    
    /**
     * Extract phrases (for categories) from text
     * 
     * @param string $text
     * @return array
     */
    private function extract_phrases($text) {
        $words = $this->tokenize($text);
        $phrases = array();
        
        // Extract bigrams (2-word phrases)
        for ($i = 0; $i < count($words) - 1; $i++) {
            if ($this->is_stop_word($words[$i]) || $this->is_stop_word($words[$i + 1])) {
                continue;
            }
            
            $phrase = ucwords($words[$i] . ' ' . $words[$i + 1]);
            if (!isset($phrases[$phrase])) {
                $phrases[$phrase] = 0;
            }
            $phrases[$phrase]++;
        }
        
        // Sort by frequency and get top phrases
        arsort($phrases);
        return array_keys(array_slice($phrases, 0, 10));
    }
    
    /**
     * Extract keywords (for tags) from text
     * 
     * @param string $text
     * @param string $title
     * @return array
     */
    private function extract_keywords($text, $title = '') {
        $words = $this->tokenize($text);
        $title_words = $this->tokenize($title);
        
        $word_freq = array_count_values($words);
        
        // Give title words higher weight
        foreach ($title_words as $word) {
            if (isset($word_freq[$word])) {
                $word_freq[$word] *= 3;
            }
        }
        
        // Check for key topics
        $tags = array();
        $text_lower = mb_strtolower($text, 'UTF-8');
        foreach ($this->key_topics as $topic) {
            if (mb_stripos($text_lower, $topic) !== false) {
                $tags[] = ucwords($topic);
            }
        }
        
        // Add frequent words
        arsort($word_freq);
        foreach (array_keys(array_slice($word_freq, 0, 10)) as $word) {
            if (!$this->is_stop_word($word) && mb_strlen($word) > 3) {
                $tags[] = ucfirst($word);
            }
        }
        
        return array_unique($tags);
    }
    
    /**
     * Tokenize text into words
     * 
     * @param string $text
     * @return array
     */
    private function tokenize($text) {
        $text = mb_strtolower($text, 'UTF-8');
        $text = preg_replace('/[^\p{L}\p{N}\s]/u', ' ', $text);
        $words = preg_split('/\s+/', $text, -1, PREG_SPLIT_NO_EMPTY);
        return $words;
    }
    
    /**
     * Check if word is a stop word
     * 
     * @param string $word
     * @return bool
     */
    private function is_stop_word($word) {
        return in_array(mb_strtolower($word, 'UTF-8'), $this->stop_words);
    }
    
    /**
     * Check if category name is valid
     * 
     * @param string $name
     * @return bool
     */
    private function is_valid_category($name) {
        if (empty($name) || mb_strlen($name) < 2) {
            return false;
        }
        
        foreach ($this->exclude_patterns as $pattern) {
            if (preg_match($pattern, $name)) {
                return false;
            }
        }
        
        // Reject if mostly numbers
        if (preg_match('/\d/', $name) && strlen(preg_replace('/\D/', '', $name)) / strlen($name) > 0.5) {
            return false;
        }
        
        return true;
    }
    
    /**
     * Filter and deduplicate array
     * 
     * @param array $items
     * @return array
     */
    private function filter_and_deduplicate($items) {
        $filtered = array();
        
        foreach ($items as $item) {
            if ($this->is_valid_category($item)) {
                $filtered[] = $item;
            }
        }
        
        return array_unique($filtered);
    }
}
