<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-version-manager.php                          ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     260                                            ║
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
 * Version Manager
 * Handles wiki page revisions and diff viewing
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Wiki_Version_Manager {
    
    /**
     * Constructor
     */
    public function __construct() {
        // AJAX handlers
        add_action('wp_ajax_get_wiki_revisions', array($this, 'ajax_get_revisions'));
        add_action('wp_ajax_get_wiki_diff', array($this, 'ajax_get_diff'));
        add_action('wp_ajax_restore_wiki_revision', array($this, 'ajax_restore_revision'));
    }
    
    /**
     * Save Revision
     */
    public function save_revision($post_id, $markdown, $message = '') {
        // WordPress handles revisions automatically
        // We just need to store custom metadata with the revision
        
        $revision_id = wp_save_post_revision($post_id);
        
        if ($revision_id) {
            // Store markdown source with revision
            update_metadata('post', $revision_id, '_wiki_markdown', $markdown);
            update_metadata('post', $revision_id, '_revision_message', $message);
            update_metadata('post', $revision_id, '_revision_author', get_current_user_id());
            update_metadata('post', $revision_id, '_revision_timestamp', current_time('timestamp'));
        }
        
        return $revision_id;
    }
    
    /**
     * Get Revisions
     */
    public function get_revisions($post_id, $limit = 10) {
        $revisions = wp_get_post_revisions($post_id, array(
            'posts_per_page' => $limit,
            'orderby' => 'date',
            'order' => 'DESC'
        ));
        
        $output = array();
        
        foreach ($revisions as $revision) {
            $author_id = $revision->post_author;
            $author = get_userdata($author_id);
            
            $output[] = array(
                'id' => $revision->ID,
                'date' => $revision->post_modified,
                'date_formatted' => get_the_modified_date('Y-m-d H:i', $revision),
                'author' => $author ? $author->display_name : __('Unknown', 'themisdb-wiki'),
                'author_id' => $author_id,
                'message' => get_metadata('post', $revision->ID, '_revision_message', true),
                'markdown' => get_metadata('post', $revision->ID, '_wiki_markdown', true)
            );
        }
        
        return $output;
    }
    
    /**
     * Get Diff Between Two Revisions
     */
    public function get_diff($revision_id_old, $revision_id_new) {
        $old_markdown = get_metadata('post', $revision_id_old, '_wiki_markdown', true);
        $new_markdown = get_metadata('post', $revision_id_new, '_wiki_markdown', true);
        
        if (empty($old_markdown)) {
            $old_revision = wp_get_post_revision($revision_id_old);
            if ($old_revision) {
                $old_markdown = $old_revision->post_content;
            }
        }
        
        if (empty($new_markdown)) {
            $new_revision = wp_get_post_revision($revision_id_new);
            if ($new_revision) {
                $new_markdown = $new_revision->post_content;
            }
        }
        
        return array(
            'old' => $old_markdown,
            'new' => $new_markdown
        );
    }
    
    /**
     * Restore Revision
     */
    public function restore_revision($post_id, $revision_id) {
        $revision = wp_get_post_revision($revision_id);
        
        if (!$revision || $revision->post_parent != $post_id) {
            return new WP_Error('invalid_revision', __('Invalid revision', 'themisdb-wiki'));
        }
        
        // Get markdown from revision
        $markdown = get_metadata('post', $revision_id, '_wiki_markdown', true);
        
        if (empty($markdown)) {
            $markdown = $revision->post_content;
        }
        
        // Convert to HTML
        $wikilinks = new ThemisDB_WikiLinks();
        $html = $wikilinks->convert_markdown_with_wikilinks($markdown);
        
        // Update post
        wp_update_post(array(
            'ID' => $post_id,
            'post_content' => $html
        ));
        
        // Update markdown meta
        update_post_meta($post_id, '_wiki_markdown', $markdown);
        
        // Save a new revision marking the restore
        $this->save_revision($post_id, $markdown, sprintf(__('Restored from revision #%d', 'themisdb-wiki'), $revision_id));
        
        return true;
    }
    
    /**
     * AJAX: Get Revisions
     */
    public function ajax_get_revisions() {
        check_ajax_referer('themisdb_wiki_nonce', 'nonce');
        
        $post_id = isset($_POST['post_id']) ? intval($_POST['post_id']) : 0;
        
        if (!$post_id || !current_user_can('edit_post', $post_id)) {
            wp_send_json_error(array('message' => __('Unauthorized', 'themisdb-wiki')));
        }
        
        $revisions = $this->get_revisions($post_id);
        
        wp_send_json_success(array('revisions' => $revisions));
    }
    
    /**
     * AJAX: Get Diff
     */
    public function ajax_get_diff() {
        check_ajax_referer('themisdb_wiki_nonce', 'nonce');
        
        $revision_id_old = isset($_POST['old_id']) ? intval($_POST['old_id']) : 0;
        $revision_id_new = isset($_POST['new_id']) ? intval($_POST['new_id']) : 0;
        
        if (!$revision_id_old || !$revision_id_new) {
            wp_send_json_error(array('message' => __('Invalid revision IDs', 'themisdb-wiki')));
        }
        
        $diff = $this->get_diff($revision_id_old, $revision_id_new);
        
        wp_send_json_success($diff);
    }
    
    /**
     * AJAX: Restore Revision
     */
    public function ajax_restore_revision() {
        check_ajax_referer('themisdb_wiki_nonce', 'nonce');
        
        $post_id = isset($_POST['post_id']) ? intval($_POST['post_id']) : 0;
        $revision_id = isset($_POST['revision_id']) ? intval($_POST['revision_id']) : 0;
        
        if (!$post_id || !$revision_id || !current_user_can('edit_post', $post_id)) {
            wp_send_json_error(array('message' => __('Unauthorized', 'themisdb-wiki')));
        }
        
        $result = $this->restore_revision($post_id, $revision_id);
        
        if (is_wp_error($result)) {
            wp_send_json_error(array('message' => $result->get_error_message()));
        }
        
        wp_send_json_success(array('message' => __('Revision restored successfully', 'themisdb-wiki')));
    }
    
    /**
     * Render Revision History Widget (for admin)
     */
    public function render_revision_history($post_id) {
        $revisions = $this->get_revisions($post_id, 20);
        
        if (empty($revisions)) {
            echo '<p>' . __('No revisions found', 'themisdb-wiki') . '</p>';
            return;
        }
        
        echo '<div class="wiki-revision-history">';
        echo '<h3>' . __('Revision History', 'themisdb-wiki') . '</h3>';
        echo '<table class="revision-table widefat">';
        echo '<thead>';
        echo '<tr>';
        echo '<th>' . __('Date', 'themisdb-wiki') . '</th>';
        echo '<th>' . __('Author', 'themisdb-wiki') . '</th>';
        echo '<th>' . __('Message', 'themisdb-wiki') . '</th>';
        echo '<th>' . __('Actions', 'themisdb-wiki') . '</th>';
        echo '</tr>';
        echo '</thead>';
        echo '<tbody>';
        
        foreach ($revisions as $revision) {
            echo '<tr>';
            echo '<td>' . esc_html($revision['date_formatted']) . '</td>';
            echo '<td>' . esc_html($revision['author']) . '</td>';
            echo '<td>' . esc_html($revision['message']) . '</td>';
            echo '<td>';
            echo '<a href="#" class="view-diff" data-revision-id="' . esc_attr($revision['id']) . '">' . __('View Diff', 'themisdb-wiki') . '</a> | ';
            echo '<a href="#" class="restore-revision" data-revision-id="' . esc_attr($revision['id']) . '">' . __('Restore', 'themisdb-wiki') . '</a>';
            echo '</td>';
            echo '</tr>';
        }
        
        echo '</tbody>';
        echo '</table>';
        echo '</div>';
        
        echo '<div id="diff-viewer" class="wiki-diff" style="display:none;">';
        echo '<h4>' . __('Changes', 'themisdb-wiki') . '</h4>';
        echo '<div class="diff-container"></div>';
        echo '</div>';
    }
}
