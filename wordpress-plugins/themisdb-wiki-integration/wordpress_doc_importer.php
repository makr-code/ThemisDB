<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            wordpress_doc_importer.php                         ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     514                                            ║
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
 * WordPress Documentation Importer with Intelligent Categories
 * 
 * Imports ThemisDB documentation into WordPress using intelligently extracted
 * categories and tags from the wordpress_category_extractor.py script.
 * 
 * Usage:
 *   1. Run python script to generate wordpress_categories.json
 *   2. Upload this script to WordPress themes/functions.php or as a plugin
 *   3. Run via WordPress admin or WP-CLI
 * 
 * WP-CLI Usage:
 *   wp eval-file wordpress_doc_importer.php
 * 
 * @package ThemisDB
 * @version 1.0.0
 */

// Exit if accessed directly
if (!defined('ABSPATH') && !defined('WP_CLI')) {
    exit;
}

class ThemisDB_Doc_Importer {
    
    /**
     * Path to the JSON file with extracted categories
     */
    private $json_file;
    
    /**
     * WordPress category ID cache
     */
    private $category_cache = array();
    
    /**
     * WordPress tag ID cache
     */
    private $tag_cache = array();
    
    /**
     * Statistics
     */
    private $stats = array(
        'processed' => 0,
        'created' => 0,
        'updated' => 0,
        'skipped' => 0,
        'errors' => 0,
        'categories_created' => 0,
        'tags_created' => 0,
    );
    
    /**
     * Constructor
     * 
     * @param string $json_file Path to wordpress_categories.json
     */
    public function __construct($json_file = 'wordpress_categories.json') {
        $this->json_file = $json_file;
    }
    
    /**
     * Main import function
     */
    public function import() {
        $this->log("Starting ThemisDB Documentation Import...");
        
        // Load JSON data
        $data = $this->load_json_data();
        if (!$data) {
            $this->log("ERROR: Failed to load JSON data", 'error');
            return false;
        }
        
        $this->log(sprintf(
            "Loaded data: %d documents, %d categories, %d tags",
            $data['metadata']['total_documents'],
            count($data['categories']),
            count($data['tags'])
        ));
        
        // Create categories
        $this->log("Creating/checking categories...");
        $this->ensure_categories($data['categories']);
        
        // Create tags
        $this->log("Creating/checking tags...");
        $this->ensure_tags($data['tags']);
        
        // Import documents
        $this->log("Importing documents...");
        foreach ($data['documents'] as $doc) {
            $this->import_document($doc);
        }
        
        // Print statistics
        $this->print_statistics();
        
        return true;
    }
    
    /**
     * Load JSON data from file
     */
    private function load_json_data() {
        if (!file_exists($this->json_file)) {
            $this->log("ERROR: JSON file not found: {$this->json_file}", 'error');
            return false;
        }
        
        $json = file_get_contents($this->json_file);
        $data = json_decode($json, true);
        
        if (json_last_error() !== JSON_ERROR_NONE) {
            $this->log("ERROR: Invalid JSON: " . json_last_error_msg(), 'error');
            return false;
        }
        
        return $data;
    }
    
    /**
     * Ensure all categories exist in WordPress
     */
    private function ensure_categories($categories) {
        foreach ($categories as $category_name => $count) {
            $cat_id = $this->get_or_create_category($category_name);
            if ($cat_id) {
                $this->category_cache[$category_name] = $cat_id;
            }
        }
        
        $this->log(sprintf(
            "Categories ready: %d total (%d new created)",
            count($this->category_cache),
            $this->stats['categories_created']
        ));
    }
    
    /**
     * Ensure all tags exist in WordPress
     */
    private function ensure_tags($tags) {
        foreach ($tags as $tag_name => $count) {
            $tag_id = $this->get_or_create_tag($tag_name);
            if ($tag_id) {
                $this->tag_cache[$tag_name] = $tag_id;
            }
        }
        
        $this->log(sprintf(
            "Tags ready: %d total (%d new created)",
            count($this->tag_cache),
            $this->stats['tags_created']
        ));
    }
    
    /**
     * Get or create a category
     * Uses shared taxonomy manager if available for hierarchical categories
     */
    private function get_or_create_category($name) {
        // Use shared taxonomy manager if available
        if (function_exists('themisdb_get_taxonomy_manager')) {
            $manager = themisdb_get_taxonomy_manager();
            $hierarchy = $manager->get_hierarchy();
            
            // Check if category already exists first
            $existing = get_term_by('name', $name, 'category');
            $cat_id = $hierarchy->get_or_create_hierarchical_category($name);
            
            if (!is_wp_error($cat_id)) {
                // Only increment if it was newly created
                if (!$existing) {
                    $this->stats['categories_created']++;
                }
                return $cat_id;
            }
        }
        
        // Fallback to standard category creation
        // Check if category exists
        $term = term_exists($name, 'category');
        
        if ($term) {
            return $term['term_id'];
        }
        
        // Create new category
        $result = wp_insert_term($name, 'category');
        
        if (is_wp_error($result)) {
            $this->log("ERROR creating category '{$name}': " . $result->get_error_message(), 'error');
            return false;
        }
        
        $this->stats['categories_created']++;
        return $result['term_id'];
    }
    
    /**
     * Get or create a tag
     */
    private function get_or_create_tag($name) {
        // Check if tag exists
        $term = term_exists($name, 'post_tag');
        
        if ($term) {
            return $term['term_id'];
        }
        
        // Create new tag
        $result = wp_insert_term($name, 'post_tag');
        
        if (is_wp_error($result)) {
            $this->log("ERROR creating tag '{$name}': " . $result->get_error_message(), 'error');
            return false;
        }
        
        $this->stats['tags_created']++;
        return $result['term_id'];
    }
    
    /**
     * Import a single document
     */
    private function import_document($doc) {
        $this->stats['processed']++;
        
        // Prepare category IDs
        $category_ids = array();
        foreach ($doc['categories'] as $cat_name) {
            if (isset($this->category_cache[$cat_name])) {
                $category_ids[] = $this->category_cache[$cat_name];
            }
        }
        
        // Prepare tag names (WordPress accepts names directly)
        $tag_names = $doc['tags'];
        
        // Check if post already exists by title and content hash
        $existing_post = $this->find_existing_post($doc['title'], $doc['content_hash']);
        
        $post_data = array(
            'post_title'    => wp_strip_all_tags($doc['title']),
            'post_content'  => $this->prepare_content($doc['file_path']),
            'post_status'   => 'publish',
            'post_type'     => 'post',
            'post_category' => $category_ids,
            'tags_input'    => $tag_names,
            'meta_input'    => array(
                '_themisdb_file_path' => $doc['file_path'],
                '_themisdb_content_hash' => $doc['content_hash'],
                '_themisdb_language' => $doc['language'],
                '_themisdb_date_modified' => $doc['date_modified'],
            ),
        );
        
        if ($existing_post) {
            // Update existing post
            $post_data['ID'] = $existing_post->ID;
            $result = wp_update_post($post_data);
            
            if (is_wp_error($result)) {
                $this->log("ERROR updating post '{$doc['title']}': " . $result->get_error_message(), 'error');
                $this->stats['errors']++;
            } else {
                $this->stats['updated']++;
                $this->log("Updated: {$doc['title']}");
            }
        } else {
            // Create new post
            $result = wp_insert_post($post_data);
            
            if (is_wp_error($result)) {
                $this->log("ERROR creating post '{$doc['title']}': " . $result->get_error_message(), 'error');
                $this->stats['errors']++;
            } else {
                $this->stats['created']++;
                $this->log("Created: {$doc['title']}");
            }
        }
    }
    
    /**
     * Find existing post by title and content hash
     */
    private function find_existing_post($title, $content_hash) {
        // First try to find by content hash (most reliable)
        $posts = get_posts(array(
            'post_type'      => 'post',
            'post_status'    => 'any',
            'posts_per_page' => 1,
            'meta_query'     => array(
                array(
                    'key'     => '_themisdb_content_hash',
                    'value'   => $content_hash,
                    'compare' => '=',
                ),
            ),
        ));
        
        if (!empty($posts)) {
            return $posts[0];
        }
        
        // Fallback: search by exact title
        global $wpdb;
        $post_id = $wpdb->get_var($wpdb->prepare(
            "SELECT ID FROM {$wpdb->posts} WHERE post_title = %s AND post_type = 'post' LIMIT 1",
            $title
        ));
        
        return $post_id ? get_post($post_id) : null;
    }
    
    /**
     * Prepare content from markdown file
     */
    private function prepare_content($file_path) {
        // Security: Validate file path
        $file_path = realpath($file_path);
        if ($file_path === false) {
            return "Content not available - invalid file path";
        }
        
        // Security: Ensure file is within an allowed directory
        // Allow files from the documentation directory or theme directory
        $allowed_bases = array(
            ABSPATH,  // WordPress root
            get_template_directory(),  // Theme directory
            WP_CONTENT_DIR,  // wp-content directory
        );
        
        $is_allowed = false;
        foreach ($allowed_bases as $base) {
            $base = realpath($base);
            if ($base && strpos($file_path, $base) === 0) {
                $is_allowed = true;
                break;
            }
        }
        
        if (!$is_allowed) {
            return "Content not available - file path not in allowed directory";
        }
        
        // Check if file exists and is readable
        if (!file_exists($file_path) || !is_readable($file_path)) {
            return "Content not available - file not found or not readable: " . basename($file_path);
        }
        
        // If markdown converter is available, use it
        if (class_exists('ThemisDB_Markdown_Converter')) {
            $markdown = file_get_contents($file_path);
            return ThemisDB_Markdown_Converter::convert($markdown);
        }
        
        // Otherwise, just read the file as-is
        return file_get_contents($file_path);
    }
    
    /**
     * Print import statistics
     */
    private function print_statistics() {
        $this->log("\n" . str_repeat("=", 60));
        $this->log("IMPORT STATISTICS");
        $this->log(str_repeat("=", 60));
        $this->log("Documents processed: {$this->stats['processed']}");
        $this->log("Posts created: {$this->stats['created']}");
        $this->log("Posts updated: {$this->stats['updated']}");
        $this->log("Posts skipped: {$this->stats['skipped']}");
        $this->log("Errors: {$this->stats['errors']}");
        $this->log("Categories created: {$this->stats['categories_created']}");
        $this->log("Tags created: {$this->stats['tags_created']}");
        $this->log(str_repeat("=", 60) . "\n");
    }
    
    /**
     * Log message
     */
    private function log($message, $level = 'info') {
        if (defined('WP_CLI')) {
            WP_CLI::log($message);
        } else {
            error_log("[ThemisDB Import] {$message}");
            echo $message . "\n";
        }
    }
}

// Run import if executed via WP-CLI
if (defined('WP_CLI') && WP_CLI) {
    // Get JSON file path from command line arguments
    $json_file = 'wordpress_categories.json';  // Default
    if (isset($argv) && count($argv) > 1) {
        $json_file = sanitize_text_field($argv[1]);
        
        // Validate that the file exists and is readable
        if (!file_exists($json_file) || !is_readable($json_file)) {
            WP_CLI::error("JSON file not found or not readable: {$json_file}");
            exit(1);
        }
        
        // Validate file extension
        if (pathinfo($json_file, PATHINFO_EXTENSION) !== 'json') {
            WP_CLI::error("Invalid file type. Expected .json file.");
            exit(1);
        }
    }
    
    $importer = new ThemisDB_Doc_Importer($json_file);
    $importer->import();
}

// Make available as WordPress admin tool
add_action('admin_menu', function() {
    add_management_page(
        'ThemisDB Doc Import',
        'ThemisDB Import',
        'manage_options',
        'themisdb-doc-import',
        function() {
            if (isset($_POST['run_import']) && check_admin_referer('themisdb_import')) {
                $json_file = isset($_POST['json_file']) ? sanitize_text_field($_POST['json_file']) : 'wordpress_categories.json';
                
                echo '<div class="wrap">';
                echo '<h1>ThemisDB Documentation Import</h1>';
                echo '<pre>';
                
                $importer = new ThemisDB_Doc_Importer($json_file);
                $importer->import();
                
                echo '</pre>';
                echo '</div>';
                return;
            }
            
            ?>
            <div class="wrap">
                <h1>ThemisDB Documentation Import</h1>
                <form method="post" action="">
                    <?php wp_nonce_field('themisdb_import'); ?>
                    <table class="form-table">
                        <tr>
                            <th scope="row">
                                <label for="json_file">JSON File Path</label>
                            </th>
                            <td>
                                <input type="text" 
                                       name="json_file" 
                                       id="json_file" 
                                       value="<?php echo esc_attr(get_template_directory() . '/wordpress_categories.json'); ?>" 
                                       class="regular-text">
                                <p class="description">
                                    Full path to wordpress_categories.json generated by the Python extractor script.
                                </p>
                            </td>
                        </tr>
                    </table>
                    
                    <p class="submit">
                        <input type="submit" 
                               name="run_import" 
                               class="button button-primary" 
                               value="Run Import">
                    </p>
                </form>
                
                <div class="card">
                    <h2>Instructions</h2>
                    <ol>
                        <li>Run the Python category extractor:
                            <pre>python3 tools/wordpress_category_extractor.py --docs-path docs --output wordpress_categories.json</pre>
                        </li>
                        <li>Upload the generated <code>wordpress_categories.json</code> file to your WordPress installation</li>
                        <li>Enter the full path to the JSON file above</li>
                        <li>Click "Run Import" to start the import process</li>
                    </ol>
                    
                    <h3>WP-CLI Alternative</h3>
                    <p>You can also run the import via WP-CLI:</p>
                    <pre>wp eval-file wordpress_doc_importer.php /path/to/wordpress_categories.json</pre>
                </div>
            </div>
            <?php
        }
    );
});
