<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-taxonomy-extractor.php                       ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:21                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     609                                            ║
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
 * Taxonomy Extractor
 * Combines content-based and structure-based taxonomy extraction
 * Extracts from both post content (text analysis) and metadata (file paths, frontmatter)
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Taxonomy_Extractor {
    
    /**
     * TF-IDF Calculator instance
     */
    private $tfidf;
    
    /**
     * Constructor
     */
    public function __construct() {
        // Initialize TF-IDF calculator
        if (class_exists('ThemisDB_TFIDF')) {
            $this->tfidf = new ThemisDB_TFIDF();
        }
    }
    
    /**
     * Stop words for text analysis (German and English)
     */
    private $stop_words = array(
        // Monatsnamen (DE/EN)
        'januar', 'februar', 'märz', 'april', 'mai', 'juni', 'juli', 'august', 'september', 'oktober', 'november', 'dezember',
        'january', 'february', 'march', 'may', 'june', 'july', 'august', 'september', 'october', 'november', 'december',
        
        // Wochentage (DE/EN)
        'monday', 'montag', 'tuesday', 'dienstag', 'wednesday', 'mittwoch',
        'thursday', 'donnerstag', 'friday', 'freitag', 'saturday', 'samstag',
        'sunday', 'sonntag',
        
        // Monat-Abkürzungen
        'jan', 'feb', 'mar', 'apr', 'may', 'jun', 'jul', 'aug', 'sep', 'oct', 'nov', 'dec',
        
        // Generische Wörter
        'use', 'test', 'tmp', 'example', 'demo', 'sample', 'draft', 'untitled',
        
        // Artikel/Präpositionen (DE/EN)
        'der', 'die', 'das', 'ein', 'eine', 'einen', 'the', 'a', 'an',
        'und', 'oder', 'aber', 'and', 'or', 'but', 'with', 'from', 'to',
        'mit', 'von', 'zu', 'auf', 'für', 'in', 'im', 'an', 'bei', 'nach',
        
        // Sehr häufige Verben
        'ist', 'sind', 'war', 'waren', 'be', 'is', 'are', 'was', 'were',
        'have', 'has', 'had', 'do', 'does', 'did', 'will', 'would', 'can', 'could',
        'if', 'for', 'about', 'at'
    );
    
    /**
     * Patterns to exclude from categories/tags
     */
    private $exclude_patterns = array(
        '/^\d+$/',                          // Pure numbers: 123
        '/^\d{4}$/',                        // Years: 2026
        '/^\d{1,2}$/',                      // Single/double digits: 1, 01
        '/^v?\d+\.\d+/',                    // Version numbers: v1.0, 2.3
        '/^\d+\s+\d+$/',                    // Date fragments: "9 2026", "01 02"
        '/^\d{1,2}[\.\/\-]\d{1,2}[\.\/\-]\d{2,4}$/',  // Dates: 01.02.2026, 1/2/26
        '/^(de|en|fr|es|ja|zh|ru|pt|it)$/i', // Language codes
        '/^(use|tmp|test|demo|example)$/i',  // Generic words
        '/^[a-z]$/i',                       // Single letters
        '/^[\W_]+$/',                       // Only special characters
        '/^(page|post|article|blog|site|web)$/i', // WordPress generic
        '/^(http|https|www|ftp)$/i',       // URL fragments
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
        
        // Extract all potential terms
        $all_terms = $this->extract_all_terms($text, $post->post_title);
        
        // Score terms using TF-IDF if available
        if ($this->tfidf) {
            $scored_terms = $this->tfidf->score_terms($all_terms, $text, 20);
        } else {
            // Fallback: simple frequency-based scoring
            $scored_terms = $this->score_by_frequency($all_terms, $text);
        }
        
        // Separate into categories and tags
        $separated = $this->separate_categories_and_tags($scored_terms);
        
        return array(
            'categories' => $separated['categories'],
            'tags' => $separated['tags']
        );
    }
    
    /**
     * Extract all potential terms from text
     * 
     * @param string $text
     * @param string $title
     * @return array
     */
    private function extract_all_terms($text, $title = '') {
        $terms = array();
        
        // Extract phrases (2-3 words)
        $phrases = $this->extract_phrases($text);
        $terms = array_merge($terms, $phrases);
        
        // Extract keywords
        $keywords = $this->extract_keywords($text, $title);
        $terms = array_merge($terms, $keywords);
        
        // Extract key topics
        $text_lower = mb_strtolower($text, 'UTF-8');
        foreach ($this->key_topics as $topic) {
            if (mb_stripos($text_lower, $topic) !== false) {
                $terms[] = ucwords($topic);
            }
        }
        
        return array_unique($terms);
    }
    
    /**
     * Score terms by frequency (fallback when TF-IDF unavailable)
     * 
     * @param array $terms
     * @param string $text
     * @return array
     */
    private function score_by_frequency($terms, $text) {
        $text_lower = mb_strtolower($text, 'UTF-8');
        $scored = array();
        
        foreach ($terms as $term) {
            $term_lower = mb_strtolower($term, 'UTF-8');
            $count = substr_count($text_lower, $term_lower);
            
            $scored[] = array(
                'term' => $term,
                'score' => $count
            );
        }
        
        // Sort by score descending
        usort($scored, function($a, $b) {
            return $b['score'] <=> $a['score'];
        });
        
        return array_slice($scored, 0, 20);
    }
    
    /**
     * Separate extracted terms into categories and tags
     * WordPress Best Practice:
     * - Categories: Hierarchical, broad (3-5 per post)
     * - Tags: Flat, specific (8-10 per post)
     * 
     * @param array $extracted_terms Scored terms
     * @return array Array with 'categories' and 'tags'
     */
    private function separate_categories_and_tags($extracted_terms) {
        $categories = array();
        $tags = array();
        
        // Get configurable limits
        $max_categories = get_option('themisdb_taxonomy_max_categories', 5);
        $max_tags = get_option('themisdb_taxonomy_max_tags', 10);
        
        foreach ($extracted_terms as $term_data) {
            $term = $term_data['term'];
            
            // Skip if already categorized
            if (in_array($term, $categories) || in_array($term, $tags)) {
                continue;
            }
            
            // Categories: Broader terms (2+ words, general concepts)
            if ($this->is_broad_term($term) && count($categories) < $max_categories) {
                // Check if exists as tag
                if (!$this->exists_as_tag($term) && !in_array($term, $tags)) {
                    $categories[] = $term;
                }
            }
            // Tags: Specific terms (1 word or technical terms)
            elseif ($this->is_specific_term($term) && count($tags) < $max_tags) {
                // Check if exists as category
                if (!$this->exists_as_category($term) && !in_array($term, $categories)) {
                    $tags[] = $term;
                }
            }
        }
        
        return array(
            'categories' => $categories,
            'tags' => $tags
        );
    }
    
    /**
     * Check if term is broad (suitable for category)
     * 
     * @param string $term
     * @return bool
     */
    private function is_broad_term($term) {
        // Multi-word phrases are typically broader
        $word_count = count(explode(' ', trim($term)));
        if ($word_count >= 2) {
            return true;
        }
        
        // Check against broad concept patterns
        $broad_patterns = array(
            'security', 'performance', 'development', 'operations',
            'integration', 'architecture', 'monitoring', 'deployment',
            'database', 'llm', 'ai', 'machine', 'data', 'features'
        );
        
        $term_lower = mb_strtolower($term, 'UTF-8');
        foreach ($broad_patterns as $pattern) {
            if (stripos($term_lower, $pattern) !== false) {
                return true;
            }
        }
        
        return false;
    }
    
    /**
     * Check if term is specific (suitable for tag)
     * 
     * @param string $term
     * @return bool
     */
    private function is_specific_term($term) {
        // Single words or technical acronyms
        $word_count = count(explode(' ', trim($term)));
        if ($word_count === 1) {
            return true;
        }
        
        // Technical terms are specific
        $specific_patterns = array(
            'api', 'sdk', 'jwt', 'oauth', 'ssl', 'tls', 'grpc', 'rest',
            'docker', 'kubernetes', 'k8s', 'sql', 'nosql', 'json',
            'cuda', 'gpu', 'cpu', 'mvcc', 'acid'
        );
        
        $term_lower = mb_strtolower($term, 'UTF-8');
        if (in_array($term_lower, $specific_patterns)) {
            return true;
        }
        
        return !$this->is_broad_term($term);
    }
    
    /**
     * Check if term exists as a tag
     * 
     * @param string $term
     * @return bool
     */
    private function exists_as_tag($term) {
        $existing = term_exists($term, 'post_tag');
        return $existing !== 0 && $existing !== null;
    }
    
    /**
     * Check if term exists as a category
     * 
     * @param string $term
     * @return bool
     */
    private function exists_as_category($term) {
        $existing = term_exists($term, 'category');
        return $existing !== 0 && $existing !== null;
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
