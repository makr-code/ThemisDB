<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-admin.php                                    ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     356                                            ║
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
 * Admin Interface
 * Handles WordPress admin settings and pages
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Wiki_Admin {
    
    /**
     * Constructor
     */
    public function __construct() {
        add_action('admin_menu', array($this, 'add_admin_menu'));
        add_action('admin_init', array($this, 'register_settings'));
    }
    
    /**
     * Add Admin Menu
     */
    public function add_admin_menu() {
        add_options_page(
            __('ThemisDB Wiki Settings', 'themisdb-wiki'),
            __('ThemisDB Wiki', 'themisdb-wiki'),
            'manage_options',
            'themisdb-wiki',
            array($this, 'render_settings_page')
        );
    }
    
    /**
     * Register Settings
     */
    public function register_settings() {
        register_setting('themisdb_wiki_settings', 'themisdb_wiki_github_repo');
        register_setting('themisdb_wiki_settings', 'themisdb_wiki_github_token');
        register_setting('themisdb_wiki_settings', 'themisdb_wiki_github_branch');
        register_setting('themisdb_wiki_settings', 'themisdb_wiki_sync_direction');
        register_setting('themisdb_wiki_settings', 'themisdb_wiki_auto_sync');
        
        // GitHub Settings Section
        add_settings_section(
            'themisdb_wiki_github_section',
            __('GitHub Wiki Synchronization', 'themisdb-wiki'),
            array($this, 'render_github_section'),
            'themisdb-wiki'
        );
        
        add_settings_field(
            'themisdb_wiki_github_repo',
            __('GitHub Repository', 'themisdb-wiki'),
            array($this, 'render_repo_field'),
            'themisdb-wiki',
            'themisdb_wiki_github_section'
        );
        
        add_settings_field(
            'themisdb_wiki_github_token',
            __('GitHub Personal Access Token', 'themisdb-wiki'),
            array($this, 'render_token_field'),
            'themisdb-wiki',
            'themisdb_wiki_github_section'
        );
        
        add_settings_field(
            'themisdb_wiki_github_branch',
            __('GitHub Branch', 'themisdb-wiki'),
            array($this, 'render_branch_field'),
            'themisdb-wiki',
            'themisdb_wiki_github_section'
        );
        
        add_settings_field(
            'themisdb_wiki_sync_direction',
            __('Sync Direction', 'themisdb-wiki'),
            array($this, 'render_sync_direction_field'),
            'themisdb-wiki',
            'themisdb_wiki_github_section'
        );
        
        add_settings_field(
            'themisdb_wiki_auto_sync',
            __('Auto-Sync on Save', 'themisdb-wiki'),
            array($this, 'render_auto_sync_field'),
            'themisdb-wiki',
            'themisdb_wiki_github_section'
        );
    }
    
    /**
     * Render Settings Page
     */
    public function render_settings_page() {
        ?>
        <div class="wrap">
            <h1><?php echo esc_html(get_admin_page_title()); ?></h1>
            
            <h2 class="nav-tab-wrapper">
                <a href="#settings" class="nav-tab nav-tab-active"><?php _e('Settings', 'themisdb-wiki'); ?></a>
                <a href="#sync" class="nav-tab"><?php _e('Sync', 'themisdb-wiki'); ?></a>
                <a href="#about" class="nav-tab"><?php _e('About', 'themisdb-wiki'); ?></a>
            </h2>
            
            <div id="settings" class="tab-content active">
                <form method="post" action="options.php">
                    <?php
                    settings_fields('themisdb_wiki_settings');
                    do_settings_sections('themisdb-wiki');
                    submit_button();
                    ?>
                </form>
            </div>
            
            <div id="sync" class="tab-content" style="display:none;">
                <h2><?php _e('GitHub Sync', 'themisdb-wiki'); ?></h2>
                
                <?php $this->render_sync_panel(); ?>
            </div>
            
            <div id="about" class="tab-content" style="display:none;">
                <h2><?php _e('About ThemisDB Wiki Integration', 'themisdb-wiki'); ?></h2>
                
                <p><?php _e('Version:', 'themisdb-wiki'); ?> <strong><?php echo THEMISDB_WIKI_VERSION; ?></strong></p>
                
                <h3><?php _e('Features', 'themisdb-wiki'); ?></h3>
                <ul>
                    <li>✅ <?php _e('Custom Post Type for Wiki pages', 'themisdb-wiki'); ?></li>
                    <li>✅ <?php _e('Markdown editor (SimpleMDE)', 'themisdb-wiki'); ?></li>
                    <li>✅ <?php _e('[[WikiLink]] syntax support', 'themisdb-wiki'); ?></li>
                    <li>✅ <?php _e('Auto-generated Table of Contents', 'themisdb-wiki'); ?></li>
                    <li>✅ <?php _e('Version history with diff viewer', 'themisdb-wiki'); ?></li>
                    <li>✅ <?php _e('GitHub Wiki synchronization', 'themisdb-wiki'); ?></li>
                    <li>✅ <?php _e('Full-text search', 'themisdb-wiki'); ?></li>
                    <li>✅ <?php _e('Responsive design', 'themisdb-wiki'); ?></li>
                </ul>
                
                <h3><?php _e('WikiLink Syntax', 'themisdb-wiki'); ?></h3>
                <pre><code>[[Page Name]]                  → Link to page
[[Page|Display Text]]          → Custom text
[[Page#Section]]               → Link to section
[[Category:Name]]              → Assign category
[[File:image.png|thumb|right]] → Embed image</code></pre>
            </div>
        </div>
        
        <script>
        jQuery(document).ready(function($) {
            $('.nav-tab').on('click', function(e) {
                e.preventDefault();
                var target = $(this).attr('href');
                
                $('.nav-tab').removeClass('nav-tab-active');
                $(this).addClass('nav-tab-active');
                
                $('.tab-content').hide();
                $(target).show();
            });
        });
        </script>
        <?php
    }
    
    /**
     * Render GitHub Section
     */
    public function render_github_section() {
        echo '<p>' . __('Configure GitHub Wiki synchronization settings.', 'themisdb-wiki') . '</p>';
    }
    
    /**
     * Render Repository Field
     */
    public function render_repo_field() {
        $value = get_option('themisdb_wiki_github_repo', '');
        echo '<input type="text" name="themisdb_wiki_github_repo" value="' . esc_attr($value) . '" class="regular-text" placeholder="owner/repository">';
        echo '<p class="description">' . __('Format: owner/repository (e.g., makr-code/wordpressPlugins)', 'themisdb-wiki') . '</p>';
    }
    
    /**
     * Render Token Field
     */
    public function render_token_field() {
        $value = get_option('themisdb_wiki_github_token', '');
        echo '<input type="password" name="themisdb_wiki_github_token" value="' . esc_attr($value) . '" class="regular-text">';
        echo '<p class="description">' . __('GitHub Personal Access Token with repo permissions', 'themisdb-wiki') . '</p>';
    }
    
    /**
     * Render Branch Field
     */
    public function render_branch_field() {
        $value = get_option('themisdb_wiki_github_branch', 'main');
        echo '<input type="text" name="themisdb_wiki_github_branch" value="' . esc_attr($value) . '" class="regular-text">';
        echo '<p class="description">' . __('Default: main', 'themisdb-wiki') . '</p>';
    }
    
    /**
     * Render Sync Direction Field
     */
    public function render_sync_direction_field() {
        $value = get_option('themisdb_wiki_sync_direction', 'manual');
        ?>
        <select name="themisdb_wiki_sync_direction">
            <option value="manual" <?php selected($value, 'manual'); ?>><?php _e('Manual', 'themisdb-wiki'); ?></option>
            <option value="wp_to_github" <?php selected($value, 'wp_to_github'); ?>><?php _e('WordPress → GitHub', 'themisdb-wiki'); ?></option>
            <option value="github_to_wp" <?php selected($value, 'github_to_wp'); ?>><?php _e('GitHub → WordPress', 'themisdb-wiki'); ?></option>
            <option value="bidirectional" <?php selected($value, 'bidirectional'); ?>><?php _e('Bidirectional', 'themisdb-wiki'); ?></option>
        </select>
        <?php
    }
    
    /**
     * Render Auto Sync Field
     */
    public function render_auto_sync_field() {
        $value = get_option('themisdb_wiki_auto_sync', 'no');
        ?>
        <label>
            <input type="checkbox" name="themisdb_wiki_auto_sync" value="yes" <?php checked($value, 'yes'); ?>>
            <?php _e('Automatically sync to GitHub when saving wiki pages', 'themisdb-wiki'); ?>
        </label>
        <?php
    }
    
    /**
     * Render Sync Panel
     */
    private function render_sync_panel() {
        $repo = get_option('themisdb_wiki_github_repo', '');
        $last_sync = get_option('themisdb_wiki_last_bulk_sync', 0);
        
        if (empty($repo)) {
            echo '<div class="notice notice-warning">';
            echo '<p>' . __('Please configure GitHub settings first.', 'themisdb-wiki') . '</p>';
            echo '</div>';
            return;
        }
        
        ?>
        <div class="wiki-sync-panel">
            <h3><?php _e('GitHub Sync Status', 'themisdb-wiki'); ?></h3>
            
            <p><strong><?php _e('Repository:', 'themisdb-wiki'); ?></strong> <?php echo esc_html($repo); ?></p>
            
            <?php if ($last_sync): ?>
            <p><strong><?php _e('Last bulk sync:', 'themisdb-wiki'); ?></strong> 
                <?php echo human_time_diff($last_sync, current_time('timestamp')) . ' ' . __('ago', 'themisdb-wiki'); ?>
            </p>
            <?php endif; ?>
            
            <p>
                <button type="button" id="sync-now" class="button button-primary">
                    <?php _e('Sync Now (GitHub → WordPress)', 'themisdb-wiki'); ?>
                </button>
                <button type="button" id="sync-all" class="button button-secondary">
                    <?php _e('Bulk Sync All Pages', 'themisdb-wiki'); ?>
                </button>
            </p>
            
            <div id="sync-log" class="sync-log" style="display:none;">
                <h4><?php _e('Sync Log', 'themisdb-wiki'); ?></h4>
                <div class="sync-log-content"></div>
            </div>
        </div>
        
        <script>
        jQuery(document).ready(function($) {
            $('#sync-now, #sync-all').on('click', function(e) {
                e.preventDefault();
                var $button = $(this);
                var action = $(this).attr('id') === 'sync-now' ? 'sync_wiki_from_github' : 'bulk_sync_wiki';
                
                $button.prop('disabled', true).text('<?php _e('Syncing...', 'themisdb-wiki'); ?>');
                $('#sync-log').show();
                $('#sync-log .sync-log-content').html('<p><?php _e('Starting sync...', 'themisdb-wiki'); ?></p>');
                
                $.post(ajaxurl, {
                    action: action,
                    nonce: '<?php echo wp_create_nonce('themisdb_wiki_nonce'); ?>'
                }, function(response) {
                    if (response.success) {
                        $('#sync-log .sync-log-content').append(
                            '<p class="success">✓ ' + response.data.message + '</p>'
                        );
                        if (response.data.errors && response.data.errors.length > 0) {
                            $('#sync-log .sync-log-content').append('<p class="errors"><strong><?php _e('Errors:', 'themisdb-wiki'); ?></strong></p>');
                            $.each(response.data.errors, function(i, error) {
                                $('#sync-log .sync-log-content').append('<p class="error">✗ ' + error + '</p>');
                            });
                        }
                    } else {
                        $('#sync-log .sync-log-content').append(
                            '<p class="error">✗ ' + response.data.message + '</p>'
                        );
                    }
                }).always(function() {
                    $button.prop('disabled', false).text($button.attr('id') === 'sync-now' ? '<?php _e('Sync Now (GitHub → WordPress)', 'themisdb-wiki'); ?>' : '<?php _e('Bulk Sync All Pages', 'themisdb-wiki'); ?>');
                });
            });
        });
        </script>
        
        <style>
        .wiki-sync-panel {
            background: #fff;
            border: 1px solid #ccd0d4;
            border-radius: 4px;
            padding: 20px;
            margin-top: 20px;
        }
        .sync-log {
            margin-top: 20px;
            background: #f8f9fa;
            border: 1px solid #ddd;
            padding: 15px;
            border-radius: 4px;
        }
        .sync-log-content {
            max-height: 400px;
            overflow-y: auto;
        }
        .sync-log p.success {
            color: #46b450;
        }
        .sync-log p.error {
            color: #dc3232;
        }
        </style>
        <?php
    }
}
