# ThemisDB Taxonomy Manager - Quick Start Guide

## Installation

### Step 1: Upload Plugin
```bash
# Via FTP or cPanel File Manager
Upload the entire 'themisdb-taxonomy-manager' folder to:
/wp-content/plugins/
```

OR

```bash
# Via WordPress Admin
1. Zip the plugin folder
2. Go to Plugins → Add New → Upload Plugin
3. Choose ZIP file and click Install Now
```

### Step 2: Activate Plugin
1. Go to WordPress Admin → Plugins
2. Find "ThemisDB Taxonomy Manager"
3. Click "Activate"

### Step 3: Verify Installation
After activation, you should see:
- New menu item: **ThemisDB** in admin sidebar
- 4 new taxonomy meta boxes on post/page editor
- New widget available: **ThemisDB Taxonomy**

## First Steps

### 1. Check Default Terms
Navigate to **ThemisDB → Taxonomy Tree**

You should see default terms organized by taxonomy:

#### Features (themisdb_feature)
- 📊 Data Models
  - Relational SQL
  - Graph Database
  - Document Store
  - Vector Database
  - Time-Series
  - Key-Value Store
- 🤖 AI/ML
  - Embedded LLM
  - Vector Search
  - RAG Support
  - GPU Acceleration
  - Model Inference
- ⚡ Performance
  - Horizontal Scaling
  - Auto-Sharding
  - Replication
  - Caching
  - Query Optimization
- 🔗 Compatibility
  - SQL Protocol
  - MongoDB Protocol
  - Cypher (Graph)
  - REST API
  - GraphQL API
  - gRPC

#### Use Cases (themisdb_usecase)
- AI & Machine Learning
- Real-Time Analytics
- Graph Analytics
- IoT Data Management
- Content Management
- E-Commerce
- Social Networks
- Recommendation Systems
- Knowledge Graphs
- Semantic Search

#### Industries (themisdb_industry)
- Healthcare
- Finance
- E-Commerce
- Telecommunications
- Manufacturing
- Education
- Government
- Media & Entertainment
- Transportation
- Energy

#### Tech Specs (themisdb_techspec)
- ACID, MVCC, C++, RocksDB, llama.cpp
- CUDA, OpenCL, Docker, Kubernetes
- High Availability, Disaster Recovery

### 2. Configure Settings
Navigate to **Settings → Taxonomy Manager**

Recommended initial settings:
- ✅ Enable Auto-Extraction: **ON**
- ✅ Auto-Assign Categories: **ON**
- ✅ Auto-Assign Tags: **ON**
- Maximum Category Depth: **3**
- ✅ Consolidate Categories: **ON**
- ✅ Enable Custom Meta Box: **ON**
- Default Icon: **📊**
- Default Color: **#3498db**
- ✅ Show in REST API: **ON**
- ✅ Enable SEO Schema: **ON**
- Breadcrumb Separator: **" / "**

Click **Save Changes**

### 3. Add Widget to Sidebar
1. Go to **Appearance → Widgets**
2. Find **ThemisDB Taxonomy** widget
3. Drag it to your desired sidebar
4. Configure:
   - Title: "Database Features"
   - Taxonomy: "Features"
   - Display Style: "Grid"
   - ✅ Show Count: **ON**
   - Parent Only: **OFF**
5. Click **Save**

### 4. Create Test Post
1. Go to **Posts → Add New**
2. Title: "Exploring Vector Databases"
3. Content: "Vector databases are optimized for AI/ML workloads..."
4. In **Database Features** meta box:
   - Check: Data Models → Vector Database
   - Check: AI/ML → Vector Search
   - Check: Performance → Query Optimization
5. In **Use Cases** meta box:
   - Check: AI & Machine Learning
   - Check: Semantic Search
6. In **Industries** meta box:
   - Check: Healthcare
7. In **Technical Specs** meta box:
   - Add: CUDA, Docker
8. Click **Publish**

### 5. View Taxonomy Archive
1. View the published post on frontend
2. Click on "Vector Database" feature link
3. You should see:
   - Custom header with gradient
   - 🗄️ or 📊 icon
   - Term description
   - Post count
   - Breadcrumbs: Home / Features / Data Models / Vector Database
   - List of posts with this feature

### 6. Test Shortcode
1. Create a new page: "All Features"
2. Add shortcode:
   ```
   [themisdb_taxonomy taxonomy="themisdb_feature" style="grid" show_count="yes"]
   ```
3. Publish and view
4. You should see all features displayed as cards with icons and colors

## Common Tasks

### Add New Feature
1. Go to **Posts → Database Features**
2. Click **Add New Feature**
3. Name: "Real-Time Processing"
4. Slug: "real-time-processing"
5. Parent: "Performance"
6. Icon: ⚡
7. Color: #f39c12
8. Click **Add New Feature**

### Customize Term Icons/Colors
1. Go to **Posts → Database Features**
2. Hover over term and click **Edit**
3. Update **Icon (Emoji)** field
4. Update **Color** field
5. Click **Update**

### Reorder Terms
1. Go to **ThemisDB → Taxonomy Tree**
2. Select taxonomy from dropdown
3. Drag terms to reorder
4. Changes save automatically

### Export Taxonomies
1. Go to **ThemisDB → Taxonomy Tree**
2. Click **Export JSON**
3. JSON file downloads with all taxonomy data

### View REST API
Open in browser:
```
https://yoursite.com/wp-json/themisdb/v1/taxonomies
https://yoursite.com/wp-json/themisdb/v1/taxonomy/themisdb_feature
https://yoursite.com/wp-json/themisdb/v1/taxonomy/themisdb_feature/tree
```

## Troubleshooting

### Issue: Taxonomies not showing
**Solution**: 
1. Deactivate and reactivate plugin
2. Go to Settings → Permalinks → Save Changes (flush rewrite rules)

### Issue: Default terms not created
**Solution**:
1. Go to WordPress Admin → Options
2. Delete option: `themisdb_default_terms_created`
3. Deactivate and reactivate plugin

### Issue: Meta boxes not showing
**Solution**:
1. Go to Settings → Taxonomy Manager
2. Ensure "Enable Custom Meta Box" is ON
3. Clear browser cache

### Issue: Widget not displaying
**Solution**:
1. Check if terms exist in selected taxonomy
2. Ensure "Hide Empty" is not filtering all terms
3. Check theme's sidebar is active

### Issue: Drag & drop not working
**Solution**:
1. Check browser console for JavaScript errors
2. Ensure jQuery UI is loaded
3. Try different browser
4. Check for JavaScript conflicts with other plugins

### Issue: Export not downloading
**Solution**:
1. Check browser's download folder
2. Check if popups are blocked
3. Try different browser

## Getting Help

### Documentation
- Full documentation: README.md
- Architecture details: ARCHITECTURE.md
- Testing guide: TESTING.md

### Support Channels
- GitHub Issues: https://github.com/makr-code/wordpressPlugins/issues
- Wiki: https://github.com/makr-code/wordpressPlugins/wiki

### Debug Mode
Enable WordPress debug mode to see detailed errors:
```php
// In wp-config.php
define('WP_DEBUG', true);
define('WP_DEBUG_LOG', true);
define('WP_DEBUG_DISPLAY', false);
```

Check logs in: `/wp-content/debug.log`

## Next Steps

1. ✅ **Customize Terms**: Add your own features, use cases, industries
2. ✅ **Configure Widgets**: Add taxonomy widgets to sidebars
3. ✅ **Use Shortcodes**: Display taxonomies on pages
4. ✅ **Assign to Posts**: Tag your content with taxonomies
5. ✅ **Monitor Analytics**: Track taxonomy usage
6. ✅ **Optimize**: Use consolidation to clean up redundant terms

## Additional Resources

### Example Shortcodes
```php
// Feature list with icons
[themisdb_taxonomy taxonomy="themisdb_feature" style="list" show_count="yes"]

// Use case cloud
[themisdb_taxonomy taxonomy="themisdb_usecase" style="cloud" min_size="0.8" max_size="2"]

// Industry grid, parents only
[themisdb_taxonomy taxonomy="themisdb_industry" style="grid" parent_only="yes"]

// Single term info
[themisdb_taxonomy_info term_id="123"]
```

### PHP Integration
```php
// Get taxonomy manager
$manager = themisdb_get_taxonomy_manager();

// Extract from post
$result = $manager->get_extractor()->extract_taxonomies($post_id);

// Assign terms
$manager->assign_categories_with_hierarchy($post_id, $result['categories']);

// Get term icon and color
$icon = get_term_meta($term_id, 'icon', true);
$color = get_term_meta($term_id, 'color', true);
```

---

**Version**: 1.0.0  
**Last Updated**: 2026-02-11  
**Need Help?**: Create an issue on GitHub
