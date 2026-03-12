<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-taxonomy-manager.php                         ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:21                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     639                                            ║
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
 * Registers and manages custom taxonomies for ThemisDB
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Taxonomy_Manager {
    
    /**
     * Analytics instance
     */
    private $analytics;
    
    /**
     * Semantic mapping for parent category finding
     */
    private $semantic_mapping = array(
        'Security' => array('authentication', 'encryption', 'oauth', 'jwt', 'ssl', 'tls', 'compliance', 'audit', 'auth', 'authz'),
        'Performance' => array('caching', 'optimization', 'indexing', 'sharding', 'benchmark', 'latency', 'throughput', 'scaling'),
        'LLM Integration' => array('embeddings', 'vector search', 'rag', 'ai', 'machine learning', 'nlp', 'llm', 'neural'),
        'Development' => array('api', 'rest', 'grpc', 'sdk', 'client', 'integration', 'docker', 'kubernetes', 'k8s'),
        'Data Models' => array('graph', 'document', 'key-value', 'time-series', 'multi-model', 'relational', 'nosql'),
        'Operations' => array('monitoring', 'backup', 'recovery', 'migration', 'deployment', 'observability')
    );
    
    /**
     * Taxonomies configuration
     */
    private $taxonomies = array(
        'themisdb_feature' => array(
            'singular' => 'Database Feature',
            'plural' => 'Database Features',
            'hierarchical' => true,
            'post_types' => array('post', 'page')
        ),
        'themisdb_usecase' => array(
            'singular' => 'Use Case',
            'plural' => 'Use Cases',
            'hierarchical' => true,
            'post_types' => array('post', 'page')
        ),
        'themisdb_industry' => array(
            'singular' => 'Industry',
            'plural' => 'Industries',
            'hierarchical' => true,
            'post_types' => array('post', 'page')
        ),
        'themisdb_techspec' => array(
            'singular' => 'Technical Spec',
            'plural' => 'Technical Specs',
            'hierarchical' => false,
            'post_types' => array('post')
        )
    );
    
    /**
     * Constructor
     */
    public function __construct() {
        add_action('init', array($this, 'register_taxonomies'), 0);
        add_action('init', array($this, 'register_shortcodes'));
        
        // Initialize analytics
        if (class_exists('ThemisDB_Taxonomy_Analytics')) {
            $this->analytics = new ThemisDB_Taxonomy_Analytics();
        }
    }
    
    /**
     * Register all custom taxonomies
     */
    public function register_taxonomies() {
        foreach ($this->taxonomies as $taxonomy => $config) {
            $labels = array(
                'name' => _x($config['plural'], 'taxonomy general name', 'themisdb-taxonomy'),
                'singular_name' => _x($config['singular'], 'taxonomy singular name', 'themisdb-taxonomy'),
                'search_items' => __('Search ' . $config['plural'], 'themisdb-taxonomy'),
                'all_items' => __('All ' . $config['plural'], 'themisdb-taxonomy'),
                'parent_item' => $config['hierarchical'] ? __('Parent ' . $config['singular'], 'themisdb-taxonomy') : null,
                'parent_item_colon' => $config['hierarchical'] ? __('Parent ' . $config['singular'] . ':', 'themisdb-taxonomy') : null,
                'edit_item' => __('Edit ' . $config['singular'], 'themisdb-taxonomy'),
                'update_item' => __('Update ' . $config['singular'], 'themisdb-taxonomy'),
                'add_new_item' => __('Add New ' . $config['singular'], 'themisdb-taxonomy'),
                'new_item_name' => __('New ' . $config['singular'] . ' Name', 'themisdb-taxonomy'),
                'menu_name' => __($config['plural'], 'themisdb-taxonomy'),
            );
            
            $args = array(
                'hierarchical' => $config['hierarchical'],
                'labels' => $labels,
                'show_ui' => true,
                'show_admin_column' => true,
                'query_var' => true,
                'rewrite' => array('slug' => str_replace('themisdb_', '', $taxonomy)),
                'show_in_rest' => true,
            );
            
            register_taxonomy($taxonomy, $config['post_types'], $args);
        }
    }
    
    /**
     * Insert default terms on activation
     */
    public function insert_default_terms() {
        $this->insert_feature_terms();
        $this->insert_usecase_terms();
        $this->insert_industry_terms();
        $this->insert_techspec_terms();
    }
    
    /**
     * Insert default Database Features terms
     */
    private function insert_feature_terms() {
        $features = array(
            'Data Models' => array(
                'Relational SQL',
                'Graph Database',
                'Document Store',
                'Vector Database',
                'Time-Series',
                'Key-Value Store'
            ),
            'AI/ML' => array(
                'Embedded LLM',
                'Vector Search',
                'RAG Support',
                'GPU Acceleration',
                'Model Inference'
            ),
            'Performance' => array(
                'Horizontal Scaling',
                'Auto-Sharding',
                'Replication',
                'Caching',
                'Query Optimization'
            ),
            'Compatibility' => array(
                'SQL Protocol',
                'MongoDB Protocol',
                'Cypher (Graph)',
                'REST API',
                'GraphQL API',
                'gRPC'
            )
        );
        
        foreach ($features as $parent_name => $children) {
            $parent = wp_insert_term($parent_name, 'themisdb_feature');
            if (!is_wp_error($parent)) {
                $parent_id = $parent['term_id'];
                foreach ($children as $child_name) {
                    wp_insert_term($child_name, 'themisdb_feature', array('parent' => $parent_id));
                }
            }
        }
    }
    
    /**
     * Insert default Use Cases terms
     */
    private function insert_usecase_terms() {
        $usecases = array(
            'AI & Machine Learning',
            'Real-Time Analytics',
            'Graph Analytics',
            'IoT Data Management',
            'Content Management',
            'E-Commerce',
            'Social Networks',
            'Recommendation Systems',
            'Knowledge Graphs',
            'Semantic Search'
        );
        
        foreach ($usecases as $usecase) {
            wp_insert_term($usecase, 'themisdb_usecase');
        }
    }
    
    /**
     * Insert default Industries terms
     */
    private function insert_industry_terms() {
        $industries = array(
            'Healthcare',
            'Finance',
            'E-Commerce',
            'Telecommunications',
            'Manufacturing',
            'Education',
            'Government',
            'Media & Entertainment',
            'Transportation',
            'Energy'
        );
        
        foreach ($industries as $industry) {
            wp_insert_term($industry, 'themisdb_industry');
        }
    }
    
    /**
     * Insert default Technical Specs terms
     */
    private function insert_techspec_terms() {
        $techspecs = array(
            'ACID',
            'MVCC',
            'C++',
            'RocksDB',
            'llama.cpp',
            'CUDA',
            'OpenCL',
            'Docker',
            'Kubernetes',
            'High Availability',
            'Disaster Recovery'
        );
        
        foreach ($techspecs as $techspec) {
            wp_insert_term($techspec, 'themisdb_techspec');
        }
    }
    
    /**
     * Register shortcodes
     */
    public function register_shortcodes() {
        add_shortcode('themisdb_taxonomy', array($this, 'shortcode_taxonomy_list'));
        add_shortcode('themisdb_term_card', array($this, 'shortcode_term_card'));
    }
    
    /**
     * Shortcode: [themisdb_taxonomy]
     */
    public function shortcode_taxonomy_list($atts) {
        $atts = shortcode_atts(array(
            'taxonomy' => 'themisdb_feature',
            'style' => 'list',
            'show_icons' => 'yes',
            'show_count' => 'yes',
            'parent' => 0,
            'limit' => -1,
            'orderby' => 'name',
            'order' => 'ASC'
        ), $atts);
        
        $args = array(
            'taxonomy' => $atts['taxonomy'],
            'parent' => $atts['parent'],
            'number' => $atts['limit'],
            'orderby' => $atts['orderby'],
            'order' => $atts['order'],
            'hide_empty' => false
        );
        
        $terms = get_terms($args);
        
        if (is_wp_error($terms) || empty($terms)) {
            return '';
        }
        
        ob_start();
        
        if ($atts['style'] === 'list') {
            $this->render_list_style($terms, $atts);
        } elseif ($atts['style'] === 'cloud') {
            $this->render_cloud_style($terms, $atts);
        } elseif ($atts['style'] === 'grid') {
            $this->render_grid_style($terms, $atts);
        }
        
        return ob_get_clean();
    }
    
    /**
     * Render list style
     */
    private function render_list_style($terms, $atts) {
        echo '<ul class="themisdb-taxonomy-list">';
        foreach ($terms as $term) {
            $icon = get_term_meta($term->term_id, 'icon', true);
            $color = get_term_meta($term->term_id, 'color', true);
            $link = get_term_link($term);
            
            echo '<li>';
            echo '<a href="' . esc_url($link) . '">';
            
            if ($atts['show_icons'] === 'yes' && $icon) {
                $style = $color ? 'style="color: ' . esc_attr($color) . ';"' : '';
                echo '<span class="icon" ' . $style . '>' . esc_html($icon) . '</span> ';
            }
            
            echo '<span class="name">' . esc_html($term->name) . '</span>';
            
            if ($atts['show_count'] === 'yes') {
                echo ' <span class="count">(' . $term->count . ')</span>';
            }
            
            echo '</a>';
            echo '</li>';
        }
        echo '</ul>';
    }
    
    /**
     * Render cloud style
     */
    private function render_cloud_style($terms, $atts) {
        $max_count = 0;
        foreach ($terms as $term) {
            if ($term->count > $max_count) {
                $max_count = $term->count;
            }
        }
        
        echo '<div class="themisdb-tag-cloud">';
        foreach ($terms as $term) {
            $color = get_term_meta($term->term_id, 'color', true);
            $link = get_term_link($term);
            
            // Scale font size based on count
            $font_size = $max_count > 0 ? (1 + ($term->count / $max_count) * 1.5) : 1;
            
            $style = 'font-size: ' . $font_size . 'em;';
            if ($color) {
                $style .= ' color: ' . esc_attr($color) . ';';
            }
            
            echo '<a href="' . esc_url($link) . '" style="' . $style . '" title="' . $term->count . ' items">';
            echo esc_html($term->name);
            echo '</a> ';
        }
        echo '</div>';
    }
    
    /**
     * Render grid style
     */
    private function render_grid_style($terms, $atts) {
        echo '<div class="themisdb-taxonomy-grid">';
        foreach ($terms as $term) {
            $icon = get_term_meta($term->term_id, 'icon', true);
            $color = get_term_meta($term->term_id, 'color', true);
            $link = get_term_link($term);
            
            $border_style = $color ? 'border-color: ' . esc_attr($color) . ';' : '';
            
            echo '<div class="grid-item" style="' . $border_style . '">';
            
            if ($atts['show_icons'] === 'yes' && $icon) {
                echo '<div class="icon">' . esc_html($icon) . '</div>';
            }
            
            echo '<h4><a href="' . esc_url($link) . '">' . esc_html($term->name) . '</a></h4>';
            
            if ($atts['show_count'] === 'yes') {
                echo '<span class="count">' . $term->count . ' posts</span>';
            }
            
            echo '</div>';
        }
        echo '</div>';
    }
    
    /**
     * Shortcode: [themisdb_term_card]
     */
    public function shortcode_term_card($atts) {
        $atts = shortcode_atts(array(
            'term_id' => 0,
            'show_description' => 'yes',
            'show_posts' => 'yes'
        ), $atts);
        
        if (!$atts['term_id']) {
            return '';
        }
        
        $term = get_term($atts['term_id']);
        
        if (is_wp_error($term)) {
            return '';
        }
        
        $icon = get_term_meta($term->term_id, 'icon', true);
        $color = get_term_meta($term->term_id, 'color', true);
        $link = get_term_link($term);
        
        ob_start();
        ?>
        <div class="themisdb-term-card" style="border-color: <?php echo esc_attr($color); ?>;">
            <?php if ($icon): ?>
                <div class="card-icon"><?php echo esc_html($icon); ?></div>
            <?php endif; ?>
            
            <h3><a href="<?php echo esc_url($link); ?>"><?php echo esc_html($term->name); ?></a></h3>
            
            <?php if ($atts['show_description'] === 'yes' && $term->description): ?>
                <p class="description"><?php echo esc_html($term->description); ?></p>
            <?php endif; ?>
            
            <?php if ($atts['show_posts'] === 'yes'): ?>
                <div class="post-count">
                    <a href="<?php echo esc_url($link); ?>"><?php echo $term->count; ?> posts</a>
                </div>
            <?php endif; ?>
        </div>
        <?php
        return ob_get_clean();
    }
    
    /**
     * Find matching category using exact match, similarity, and synonyms
     * 
     * @param string $term Term to find
     * @param string $taxonomy Taxonomy name (default: 'category')
     * @return int|false Term ID if found, false otherwise
     */
    public function find_matching_category($term, $taxonomy = 'category') {
        // 1. Exact match
        $existing = term_exists($term, $taxonomy);
        if ($existing && is_array($existing)) {
            return $existing['term_id'];
        }
        
        // 2. Similarity search using Levenshtein distance
        $categories = get_terms(array(
            'taxonomy' => $taxonomy,
            'hide_empty' => false
        ));
        
        if (!is_wp_error($categories)) {
            foreach ($categories as $cat) {
                // Levenshtein distance for short strings
                if (strlen($term) < 255 && strlen($cat->name) < 255) {
                    $distance = levenshtein(strtolower($term), strtolower($cat->name));
                    if ($distance <= 2) { // Max 2 character difference
                        return $cat->term_id;
                    }
                }
                
                // Synonym check
                if ($this->analytics && $this->analytics->are_synonyms($term, $cat->name)) {
                    return $cat->term_id;
                }
                
                // Similarity using similar_text
                $percent = 0;
                similar_text(strtolower($term), strtolower($cat->name), $percent);
                if ($percent > 85) { // 85% similar
                    return $cat->term_id;
                }
            }
        }
        
        return false;
    }
    
    /**
     * Find semantic parent category for a term
     * 
     * @param string $term Term to find parent for
     * @return int|false Parent term ID if found, false otherwise
     */
    public function find_semantic_parent($term) {
        $term_lower = strtolower($term);
        
        foreach ($this->semantic_mapping as $parent => $keywords) {
            foreach ($keywords as $keyword) {
                // Check if term contains the keyword
                if (stripos($term_lower, $keyword) !== false) {
                    // Find or create parent category
                    $parent_term = term_exists($parent, 'category');
                    if (!$parent_term) {
                        $parent_term = wp_insert_term($parent, 'category');
                    }
                    return is_array($parent_term) ? $parent_term['term_id'] : false;
                }
            }
        }
        
        return false;
    }
    
    /**
     * Assign categories to post with hierarchical structure
     * 
     * @param int $post_id Post ID
     * @param array $category_names Category names to assign
     * @param bool $append Whether to append or replace categories
     */
    public function assign_with_hierarchy($post_id, $category_names, $append = false) {
        $category_ids = array();
        $max_categories = get_option('themisdb_taxonomy_max_categories', 5);
        
        foreach ($category_names as $cat_name) {
            if (count($category_ids) >= $max_categories) {
                break;
            }
            
            // 1. Check if category exists
            $cat_id = $this->find_matching_category($cat_name, 'category');
            
            if ($cat_id) {
                // Existing category found
                $category_ids[] = $cat_id;
                
                // Get parent categories
                $parents = get_ancestors($cat_id, 'category');
                $category_ids = array_merge($category_ids, $parents);
            } else {
                // 2. Find semantic parent
                $parent_id = $this->find_semantic_parent($cat_name);
                
                if ($parent_id) {
                    // Create as child category
                    $new_cat = wp_insert_term($cat_name, 'category', array(
                        'parent' => $parent_id
                    ));
                    
                    if (!is_wp_error($new_cat) && isset($new_cat['term_id'])) {
                        $category_ids[] = $new_cat['term_id'];
                        $category_ids[] = $parent_id; // Also assign parent
                    }
                } else {
                    // Create as top-level category
                    $new_cat = wp_insert_term($cat_name, 'category');
                    if (!is_wp_error($new_cat) && isset($new_cat['term_id'])) {
                        $category_ids[] = $new_cat['term_id'];
                    }
                }
            }
        }
        
        // Remove duplicates
        $category_ids = array_unique($category_ids);
        
        // Limit to max categories
        $category_ids = array_slice($category_ids, 0, $max_categories);
        
        // Assign to post
        wp_set_post_categories($post_id, $category_ids, $append);
    }
    
    /**
     * Consolidate similar categories
     * 
     * @param float $similarity_threshold Minimum similarity (0-1)
     * @return array Consolidation results
     */
    public function consolidate_categories($similarity_threshold = 0.8) {
        if (!$this->analytics) {
            return array(
                'total_merged' => 0,
                'details' => array(),
                'error' => 'Analytics not available'
            );
        }
        
        return $this->analytics->consolidate_categories($similarity_threshold);
    }
    
    /**
     * Get optimization recommendations
     * 
     * @return array Recommendations
     */
    public function get_optimization_recommendations() {
        if (!$this->analytics) {
            return array();
        }
        
        $recommendations = array();
        
        // Get consolidation suggestions
        $consolidation = $this->analytics->get_consolidation_suggestions(0.8);
        
        foreach ($consolidation as $suggestion) {
            $recommendations[] = array(
                'current_name' => $suggestion['term1'],
                'post_count' => $suggestion['post_count'],
                'actions' => array(
                    array(
                        'type' => 'merge',
                        'target' => $suggestion['term2'],
                        'reason' => sprintf('%.0f%% similar', $suggestion['similarity'] * 100)
                    )
                )
            );
        }
        
        // Get unused terms
        $unused = $this->analytics->get_unused_terms('category');
        foreach ($unused as $term) {
            $recommendations[] = array(
                'current_name' => $term['name'],
                'post_count' => 0,
                'actions' => array(
                    array(
                        'type' => 'delete',
                        'target' => '',
                        'reason' => 'No posts assigned'
                    )
                )
            );
        }
        
        return $recommendations;
    }
}
