<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            admin-settings.php                                 ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     218                                            ║
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
 * Admin Settings Template
 */

// Exit if accessed directly
if (!defined('ABSPATH')) {
    exit;
}
?>

<div class="wrap">
    <h1><?php echo esc_html(get_admin_page_title()); ?></h1>
    
    <div class="themisdb-wiki-admin-header">
        <p><?php _e('Configure the automatic integration of ThemisDB documentation from GitHub into your WordPress site.', 'themisdb-wiki-integration'); ?></p>
    </div>
    
    <?php settings_errors(); ?>
    
    <form method="post" action="options.php">
        <?php
        settings_fields('themisdb_wiki_settings');
        do_settings_sections('themisdb_wiki_settings');
        ?>
        
        <table class="form-table">
            <tr>
                <th scope="row">
                    <label for="themisdb_wiki_github_repo"><?php _e('GitHub Repository', 'themisdb-wiki-integration'); ?></label>
                </th>
                <td>
                    <input type="text" 
                           id="themisdb_wiki_github_repo" 
                           name="themisdb_wiki_github_repo" 
                           value="<?php echo esc_attr(get_option('themisdb_wiki_github_repo', 'makr-code/wordpressPlugins')); ?>" 
                           class="regular-text" />
                    <p class="description"><?php _e('GitHub repository in format: owner/repository', 'themisdb-wiki-integration'); ?></p>
                </td>
            </tr>
            
            <tr>
                <th scope="row">
                    <label for="themisdb_wiki_github_branch"><?php _e('Branch', 'themisdb-wiki-integration'); ?></label>
                </th>
                <td>
                    <input type="text" 
                           id="themisdb_wiki_github_branch" 
                           name="themisdb_wiki_github_branch" 
                           value="<?php echo esc_attr(get_option('themisdb_wiki_github_branch', 'main')); ?>" 
                           class="regular-text" />
                    <p class="description"><?php _e('Branch to fetch documentation from (e.g., main, develop)', 'themisdb-wiki-integration'); ?></p>
                </td>
            </tr>
            
            <tr>
                <th scope="row">
                    <label for="themisdb_wiki_docs_path"><?php _e('Documentation Path', 'themisdb-wiki-integration'); ?></label>
                </th>
                <td>
                    <input type="text" 
                           id="themisdb_wiki_docs_path" 
                           name="themisdb_wiki_docs_path" 
                           value="<?php echo esc_attr(get_option('themisdb_wiki_docs_path', 'docs')); ?>" 
                           class="regular-text" />
                    <p class="description"><?php _e('Path to documentation folder in repository', 'themisdb-wiki-integration'); ?></p>
                </td>
            </tr>
            
            <tr>
                <th scope="row">
                    <label for="themisdb_wiki_default_lang"><?php _e('Default Language', 'themisdb-wiki-integration'); ?></label>
                </th>
                <td>
                    <select id="themisdb_wiki_default_lang" name="themisdb_wiki_default_lang">
                        <option value="de" <?php selected(get_option('themisdb_wiki_default_lang', 'de'), 'de'); ?>>Deutsch (DE)</option>
                        <option value="en" <?php selected(get_option('themisdb_wiki_default_lang', 'de'), 'en'); ?>>English (EN)</option>
                        <option value="fr" <?php selected(get_option('themisdb_wiki_default_lang', 'de'), 'fr'); ?>>Français (FR)</option>
                    </select>
                    <p class="description"><?php _e('Default language for documentation', 'themisdb-wiki-integration'); ?></p>
                </td>
            </tr>
            
            <tr>
                <th scope="row">
                    <label for="themisdb_wiki_github_token"><?php _e('GitHub Token (Optional)', 'themisdb-wiki-integration'); ?></label>
                </th>
                <td>
                    <input type="password" 
                           id="themisdb_wiki_github_token" 
                           name="themisdb_wiki_github_token" 
                           value="<?php echo esc_attr(get_option('themisdb_wiki_github_token', '')); ?>" 
                           class="regular-text" />
                    <p class="description"><?php _e('Personal access token for private repositories or higher rate limits', 'themisdb-wiki-integration'); ?></p>
                </td>
            </tr>
            
            <tr>
                <th scope="row">
                    <label for="themisdb_wiki_auto_sync"><?php _e('Auto-Sync', 'themisdb-wiki-integration'); ?></label>
                </th>
                <td>
                    <label>
                        <input type="checkbox" 
                               id="themisdb_wiki_auto_sync" 
                               name="themisdb_wiki_auto_sync" 
                               value="yes" 
                               <?php checked(get_option('themisdb_wiki_auto_sync', 'yes'), 'yes'); ?> />
                        <?php _e('Automatically sync documentation hourly', 'themisdb-wiki-integration'); ?>
                    </label>
                    <p class="description"><?php _e('When enabled, documentation will be refreshed automatically every hour', 'themisdb-wiki-integration'); ?></p>
                </td>
            </tr>
        </table>
        
        <?php submit_button(); ?>
    </form>
    
    <hr />
    
    <h2><?php _e('Manual Sync', 'themisdb-wiki-integration'); ?></h2>
    <p><?php _e('Click the button below to manually clear the cache and force a fresh fetch from GitHub.', 'themisdb-wiki-integration'); ?></p>
    
    <button type="button" id="themisdb-sync-now" class="button button-secondary">
        <?php _e('Sync Now', 'themisdb-wiki-integration'); ?>
    </button>
    
    <div id="themisdb-sync-message" style="margin-top: 10px;"></div>
    
    <hr />
    
    <h2><?php _e('Usage Instructions', 'themisdb-wiki-integration'); ?></h2>
    
    <h3><?php _e('Shortcode: Display Documentation', 'themisdb-wiki-integration'); ?></h3>
    <pre><code>[themisdb_wiki file="README.md" lang="de" show_toc="yes"]</code></pre>
    <p><?php _e('Parameters:', 'themisdb-wiki-integration'); ?></p>
    <ul>
        <li><strong>file</strong>: <?php _e('Markdown file to display (e.g., README.md, features/features.md)', 'themisdb-wiki-integration'); ?></li>
        <li><strong>lang</strong>: <?php _e('Language (de, en, fr) - defaults to setting above', 'themisdb-wiki-integration'); ?></li>
        <li><strong>show_toc</strong>: <?php _e('Show table of contents (yes/no)', 'themisdb-wiki-integration'); ?></li>
    </ul>
    
    <h3><?php _e('Shortcode: List Documentation Files', 'themisdb-wiki-integration'); ?></h3>
    <pre><code>[themisdb_docs lang="de" layout="grid"]</code></pre>
    <p><?php _e('Parameters:', 'themisdb-wiki-integration'); ?></p>
    <ul>
        <li><strong>lang</strong>: <?php _e('Language (de, en, fr)', 'themisdb-wiki-integration'); ?></li>
        <li><strong>layout</strong>: <?php _e('Display layout (list, grid)', 'themisdb-wiki-integration'); ?></li>
    </ul>
    
    <h3><?php _e('Examples', 'themisdb-wiki-integration'); ?></h3>
    <pre><code>
// Display German README
[themisdb_wiki file="README.md" lang="de"]

// Display English architecture documentation with TOC
[themisdb_wiki file="architecture/ARCHITECTURE.md" lang="en" show_toc="yes"]

// List all available documentation in German
[themisdb_docs lang="de" layout="grid"]
    </code></pre>
    
    <script type="text/javascript">
        jQuery(document).ready(function($) {
            $('#themisdb-sync-now').on('click', function() {
                var $button = $(this);
                var $message = $('#themisdb-sync-message');
                
                $button.prop('disabled', true).text('<?php _e('Syncing...', 'themisdb-wiki-integration'); ?>');
                $message.html('');
                
                $.ajax({
                    url: ajaxurl,
                    type: 'POST',
                    data: {
                        action: 'themisdb_sync_docs',
                        nonce: '<?php echo esc_js(wp_create_nonce('themisdb_wiki_nonce')); ?>'
                    },
                    success: function(response) {
                        if (response.success) {
                            $message.html('<div class="notice notice-success"><p>' + response.data.message + '</p></div>');
                        } else {
                            $message.html('<div class="notice notice-error"><p>' + response.data.message + '</p></div>');
                        }
                    },
                    error: function() {
                        $message.html('<div class="notice notice-error"><p><?php _e('An error occurred. Please try again.', 'themisdb-wiki-integration'); ?></p></div>');
                    },
                    complete: function() {
                        $button.prop('disabled', false).text('<?php _e('Sync Now', 'themisdb-wiki-integration'); ?>');
                    }
                });
            });
        });
    </script>
</div>
