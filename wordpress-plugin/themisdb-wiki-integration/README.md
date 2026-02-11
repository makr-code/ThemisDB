# ThemisDB Wiki Integration v1.0.0

Native WordPress wiki with Markdown editor, [[WikiLinks]], version history, and GitHub Wiki sync.

## Features

✅ Custom Post Type for Wiki pages  
✅ Markdown editor (SimpleMDE)  
✅ [[WikiLink]] syntax support  
✅ Auto-generated Table of Contents  
✅ Version history with diff viewer  
✅ GitHub Wiki synchronization  
✅ Full-text search with live suggestions  
✅ Responsive design with Themis brand colors  
✅ Contributors tracking  
✅ Related pages and backlinks  

## Installation

1. Upload to `/wp-content/plugins/themisdb-wiki-integration/`
2. Activate the plugin through the 'Plugins' menu in WordPress
3. Go to Wiki → Add New to create your first wiki page
4. (Optional) Configure GitHub sync in Settings → ThemisDB Wiki

## WikiLink Syntax

```markdown
[[Page Name]]                  → Link to page
[[Page|Display Text]]          → Custom link text
[[Page#Section]]               → Link to section
[[Category:Name]]              → Assign category
[[File:image.png|thumb|right]] → Embed image with thumbnail
```

## GitHub Sync

### Setup
1. Go to Settings → ThemisDB Wiki
2. Enter your GitHub repository (e.g., `owner/repository`)
3. Add a Personal Access Token with repo permissions
4. Choose sync direction:
   - Manual: Sync only when you click the button
   - WordPress → GitHub: Auto-push on save
   - GitHub → WordPress: Pull from GitHub
   - Bidirectional: Two-way sync

### Usage
- **Push to GitHub**: Edit a wiki page and click "Push to GitHub" in the sidebar
- **Pull from GitHub**: Go to Settings → ThemisDB Wiki → Sync tab and click "Sync Now"
- **Bulk Sync**: Click "Bulk Sync All Pages" to sync all wiki pages from GitHub

## Shortcodes

```php
// Display specific wiki page
[themisdb_wiki_page page="getting-started"]

// Show wiki index by category
[themisdb_wiki_index category="documentation"]

// Recent wiki changes
[themisdb_wiki_recent limit="5"]

// Wiki search form
[themisdb_wiki_search]

// Table of Contents
[themisdb_wiki_toc depth="3" title="Contents"]
```

## Custom Templates

To customize wiki page display, copy these templates to your theme:

- `templates/single-themisdb_wiki.php` → Single wiki page
- `templates/archive-themisdb_wiki.php` → Wiki archive/list

## Development

### Hooks

```php
// Filter wiki content before display
add_filter('themisdb_wiki_content', function($content, $post_id) {
    // Modify content
    return $content;
}, 10, 2);

// Action when wiki page is saved
add_action('themisdb_wiki_saved', function($post_id) {
    // Do something
}, 10, 1);

// Filter WikiLink output
add_filter('themisdb_wikilink_html', function($html, $page_name, $display_text) {
    // Customize link HTML
    return $html;
}, 10, 3);
```

## Requirements

- WordPress 5.0 or higher
- PHP 7.4 or higher
- Modern browser with JavaScript enabled

## Credits

- **SimpleMDE** - Markdown editor
- **jsdiff** - Diff algorithm
- **ThemisDB Team** - Plugin development

## License

MIT License - See LICENSE file for details

## Support

- Issues: https://github.com/makr-code/ThemisDB/issues
- Documentation: https://github.com/makr-code/ThemisDB/wiki
