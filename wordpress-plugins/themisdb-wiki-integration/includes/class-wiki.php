<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-wiki.php                                     ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     289                                            ║
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
 * Wiki Custom Post Type
 * Handles wiki page registration and management
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Wiki {
    
    /**
     * Constructor
     */
    public function __construct() {
        add_action('init', array($this, 'register_post_type'));
        add_action('save_post_themisdb_wiki', array($this, 'save_wiki_metadata'), 10, 3);
        add_filter('the_content', array($this, 'process_wiki_content'));
    }
    
    /**
     * Register Wiki Custom Post Type
     */
    public function register_post_type() {
        register_post_type('themisdb_wiki', array(
            'labels' => array(
                'name' => __('Wiki Pages', 'themisdb-wiki'),
                'singular_name' => __('Wiki Page', 'themisdb-wiki'),
                'add_new' => __('Add Wiki Page', 'themisdb-wiki'),
                'add_new_item' => __('Add New Wiki Page', 'themisdb-wiki'),
                'edit_item' => __('Edit Wiki Page', 'themisdb-wiki'),
                'new_item' => __('New Wiki Page', 'themisdb-wiki'),
                'view_item' => __('View Wiki Page', 'themisdb-wiki'),
                'search_items' => __('Search Wiki Pages', 'themisdb-wiki'),
                'not_found' => __('No wiki pages found', 'themisdb-wiki'),
                'not_found_in_trash' => __('No wiki pages found in trash', 'themisdb-wiki'),
                'all_items' => __('All Wiki Pages', 'themisdb-wiki'),
                'menu_name' => __('Wiki', 'themisdb-wiki')
            ),
            'public' => true,
            'has_archive' => true,
            'rewrite' => array('slug' => 'wiki'),
            'supports' => array('title', 'editor', 'author', 'revisions', 'comments', 'custom-fields'),
            'show_in_rest' => true,
            'menu_icon' => 'dashicons-book',
            'capability_type' => 'post',
            'hierarchical' => false,
            'taxonomies' => array('wiki_category'),
            'menu_position' => 5
        ));
        
        // Register Wiki Category Taxonomy
        register_taxonomy('wiki_category', 'themisdb_wiki', array(
            'labels' => array(
                'name' => __('Wiki Categories', 'themisdb-wiki'),
                'singular_name' => __('Wiki Category', 'themisdb-wiki'),
                'search_items' => __('Search Wiki Categories', 'themisdb-wiki'),
                'all_items' => __('All Wiki Categories', 'themisdb-wiki'),
                'edit_item' => __('Edit Wiki Category', 'themisdb-wiki'),
                'update_item' => __('Update Wiki Category', 'themisdb-wiki'),
                'add_new_item' => __('Add New Wiki Category', 'themisdb-wiki'),
                'new_item_name' => __('New Wiki Category Name', 'themisdb-wiki'),
                'menu_name' => __('Categories', 'themisdb-wiki')
            ),
            'hierarchical' => true,
            'show_ui' => true,
            'show_admin_column' => true,
            'query_var' => true,
            'rewrite' => array('slug' => 'wiki-category')
        ));
        
        // Add meta boxes
        add_action('add_meta_boxes_themisdb_wiki', array($this, 'add_meta_boxes'));
    }
    
    /**
     * Add Meta Boxes
     */
    public function add_meta_boxes() {
        add_meta_box(
            'wiki_markdown_editor',
            __('Markdown Editor', 'themisdb-wiki'),
            array($this, 'render_markdown_editor'),
            'themisdb_wiki',
            'normal',
            'high'
        );
        
        add_meta_box(
            'wiki_metadata',
            __('Wiki Metadata', 'themisdb-wiki'),
            array($this, 'render_metadata_box'),
            'themisdb_wiki',
            'side',
            'default'
        );
        
        add_meta_box(
            'wiki_github_sync',
            __('GitHub Sync', 'themisdb-wiki'),
            array($this, 'render_github_sync_box'),
            'themisdb_wiki',
            'side',
            'default'
        );
    }
    
    /**
     * Render Markdown Editor Meta Box
     */
    public function render_markdown_editor($post) {
        wp_nonce_field('wiki_markdown_nonce', 'wiki_markdown_nonce');
        
        $markdown = get_post_meta($post->ID, '_wiki_markdown', true);
        if (empty($markdown)) {
            // Try to convert HTML back to markdown or use post content
            $markdown = $post->post_content;
        }
        
        echo '<textarea id="wiki-markdown-editor" name="wiki_markdown" style="width:100%;height:500px;">';
        echo esc_textarea($markdown);
        echo '</textarea>';
        echo '<p class="description">' . __('Use Markdown syntax. WikiLinks: [[Page Name]], [[Page Name|Display Text]], [[Page Name#Section]]', 'themisdb-wiki') . '</p>';
    }
    
    /**
     * Render Metadata Meta Box
     */
    public function render_metadata_box($post) {
        $parent_page = get_post_meta($post->ID, '_wiki_parent', true);
        $contributors = get_post_meta($post->ID, '_wiki_contributors', true);
        
        echo '<p><label for="wiki_parent">' . __('Parent Page:', 'themisdb-wiki') . '</label>';
        wp_dropdown_pages(array(
            'post_type' => 'themisdb_wiki',
            'selected' => $parent_page,
            'name' => 'wiki_parent',
            'id' => 'wiki_parent',
            'show_option_none' => __('None', 'themisdb-wiki'),
            'exclude' => $post->ID
        ));
        echo '</p>';
        
        echo '<p><label>' . __('Contributors:', 'themisdb-wiki') . '</label><br>';
        if (is_array($contributors) && !empty($contributors)) {
            foreach ($contributors as $user_id) {
                $user = get_userdata($user_id);
                if ($user) {
                    echo get_avatar($user_id, 32) . ' ' . esc_html($user->display_name) . '<br>';
                }
            }
        } else {
            echo '<em>' . __('No contributors yet', 'themisdb-wiki') . '</em>';
        }
        echo '</p>';
    }
    
    /**
     * Render GitHub Sync Meta Box
     */
    public function render_github_sync_box($post) {
        $github_path = get_post_meta($post->ID, '_wiki_github_path', true);
        $last_sync = get_post_meta($post->ID, '_wiki_last_sync', true);
        
        echo '<p><label for="wiki_github_path">' . __('GitHub Wiki Path:', 'themisdb-wiki') . '</label>';
        echo '<input type="text" id="wiki_github_path" name="wiki_github_path" value="' . esc_attr($github_path) . '" style="width:100%;" placeholder="page-name.md">';
        echo '</p>';
        
        if ($last_sync) {
            echo '<p><strong>' . __('Last Sync:', 'themisdb-wiki') . '</strong><br>';
            echo human_time_diff($last_sync, current_time('timestamp')) . ' ' . __('ago', 'themisdb-wiki');
            echo '</p>';
        }
        
        echo '<p>';
        echo '<button type="button" id="sync-to-github" class="button button-secondary">' . __('Push to GitHub', 'themisdb-wiki') . '</button>';
        echo '</p>';
    }
    
    /**
     * Save Wiki Metadata
     */
    public function save_wiki_metadata($post_id, $post, $update) {
        // Check nonce
        if (!isset($_POST['wiki_markdown_nonce']) || !wp_verify_nonce($_POST['wiki_markdown_nonce'], 'wiki_markdown_nonce')) {
            return;
        }
        
        // Check autosave
        if (defined('DOING_AUTOSAVE') && DOING_AUTOSAVE) {
            return;
        }
        
        // Check permissions
        if (!current_user_can('edit_post', $post_id)) {
            return;
        }
        
        // Save markdown source
        if (isset($_POST['wiki_markdown'])) {
            // Get raw markdown, unslash but preserve formatting
            $markdown = wp_unslash($_POST['wiki_markdown']);
            // Only remove dangerous HTML/scripts but preserve markdown formatting
            $allowed_html = array();  // No HTML allowed in markdown source
            $markdown = wp_kses($markdown, $allowed_html);
            update_post_meta($post_id, '_wiki_markdown', $markdown);
            
            // Convert markdown to HTML for post content
            $wikilinks = new ThemisDB_WikiLinks();
            $html = $wikilinks->convert_markdown_with_wikilinks($markdown);
            
            // Update post content (unhook to avoid infinite loop)
            remove_action('save_post_themisdb_wiki', array($this, 'save_wiki_metadata'), 10);
            wp_update_post(array(
                'ID' => $post_id,
                'post_content' => $html
            ));
            add_action('save_post_themisdb_wiki', array($this, 'save_wiki_metadata'), 10, 3);
            
            // Save revision with markdown
            if ($update) {
                $version_manager = new ThemisDB_Wiki_Version_Manager();
                $version_manager->save_revision($post_id, $markdown, __('Updated via editor', 'themisdb-wiki'));
            }
        }
        
        // Save parent page
        if (isset($_POST['wiki_parent'])) {
            update_post_meta($post_id, '_wiki_parent', intval($_POST['wiki_parent']));
        }
        
        // Save GitHub path
        if (isset($_POST['wiki_github_path'])) {
            update_post_meta($post_id, '_wiki_github_path', sanitize_text_field($_POST['wiki_github_path']));
        }
        
        // Update contributors list
        $contributors = get_post_meta($post_id, '_wiki_contributors', true);
        if (!is_array($contributors)) {
            $contributors = array();
        }
        
        $current_user_id = get_current_user_id();
        if (!in_array($current_user_id, $contributors)) {
            $contributors[] = $current_user_id;
            update_post_meta($post_id, '_wiki_contributors', $contributors);
        }
    }
    
    /**
     * Process Wiki Content
     */
    public function process_wiki_content($content) {
        global $post;
        
        if (!is_singular('themisdb_wiki')) {
            return $content;
        }
        
        // Add TOC if content has enough headings
        $wikilinks = new ThemisDB_WikiLinks();
        $content = $wikilinks->add_table_of_contents($content);
        
        return $content;
    }
}
