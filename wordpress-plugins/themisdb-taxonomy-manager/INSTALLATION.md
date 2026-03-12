# Installation Guide

## Quick Install

### WordPress Admin Method (Recommended)

1. Download the plugin folder as a ZIP file
2. Go to your WordPress Admin Dashboard
3. Navigate to **Plugins → Add New → Upload Plugin**
4. Click **Choose File** and select the ZIP file
5. Click **Install Now**
6. Click **Activate Plugin**

### Manual Installation

1. Upload the `themisdb-taxonomy-manager` folder to `/wp-content/plugins/`
2. Go to WordPress Admin Dashboard
3. Navigate to **Plugins**
4. Find "ThemisDB Taxonomy Manager" in the list
5. Click **Activate**

## Post-Installation

After activation, the plugin will automatically:
- ✅ Register 4 custom taxonomies
- ✅ Create default terms with hierarchical structure
- ✅ Flush rewrite rules

## Verify Installation

### Check Taxonomies
1. Go to **Posts → Add New**
2. Look for these new taxonomy boxes in the sidebar:
   - Database Features
   - Use Cases
   - Industries
   - Technical Specs

### Check Tree View
1. Go to **Tools → Taxonomy Tree**
2. You should see the tree view interface with default terms

### Check Widget
1. Go to **Appearance → Widgets**
2. Look for **ThemisDB Taxonomy** widget
3. Drag it to a sidebar to test

## Requirements

- **WordPress**: 5.8 or higher
- **PHP**: 7.4 or higher
- **Browser**: Modern browser with JavaScript enabled

## Troubleshooting

### Taxonomies not showing?
1. Go to **Settings → Permalinks**
2. Click **Save Changes** (this flushes rewrite rules)
3. Refresh your browser

### Widget not appearing?
1. Clear browser cache
2. Check if JavaScript is enabled
3. Verify plugin is activated

### Tree view not working?
1. Check browser console for JavaScript errors
2. Ensure jQuery UI is loaded
3. Verify admin AJAX is accessible

## Next Steps

After installation:
1. Review default terms at **Tools → Taxonomy Tree**
2. Customize term icons and colors
3. Add the widget to your sidebar
4. Start categorizing your posts
5. Use shortcodes in your content

## Support

Need help? Check:
- [README.md](README.md) - Full documentation
- [GitHub Issues](https://github.com/makr-code/wordpressPlugins/issues)
- [WordPress Support](https://wordpress.org/support/)
