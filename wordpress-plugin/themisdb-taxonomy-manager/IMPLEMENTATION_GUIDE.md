# Intelligent Taxonomy Management - Implementation Guide

## Overview

This implementation adds industry best practices for intelligent WordPress taxonomy management to the ThemisDB Taxonomy Manager plugin. The enhancements focus on preventing meaningless categories, maximizing term reuse, and maintaining a clean, hierarchical taxonomy structure.

## Key Features Implemented

### 1. TF-IDF-Based Relevance Scoring

**File:** `includes/class-tfidf.php`

The TF-IDF (Term Frequency-Inverse Document Frequency) calculator scores terms based on:
- How frequently they appear in the document (TF)
- How unique they are across all documents (IDF)
- Higher scores = more relevant terms

**Usage:**
```php
$tfidf = new ThemisDB_TFIDF();
$score = $tfidf->calculate_tfidf('vector search', $post_content);
$scored_terms = $tfidf->score_terms($terms_array, $post_content, 10);
```

### 2. Enhanced Stop Words & Patterns

**File:** `includes/class-taxonomy-extractor.php`

Prevents meaningless categories like:
- ✓ Month names (January, Februar, etc.)
- ✓ Weekday names (Monday, Montag, etc.)
- ✓ Pure numbers (123, 2026)
- ✓ Date fragments (9 2026, 01 02)
- ✓ Version numbers (v1.0, 2.3)
- ✓ Language codes (de, en, fr)
- ✓ Generic words (test, tmp, demo)
- ✓ Single letters and special characters

### 3. Smart Category/Tag Separation

**Best Practices:**
- **Categories:** Max 5 per post, hierarchical, broad concepts
- **Tags:** Max 10 per post, flat, specific keywords
- **No Overlap:** Terms can't be both category and tag

**Logic:**
```php
// Categories: Broader terms (2+ words)
"Vector Search", "LLM Integration", "Data Models"

// Tags: Specific terms (technical keywords)
"JWT", "OAuth", "Docker", "CUDA", "REST API"
```

### 4. Intelligent Term Matching

**File:** `includes/class-taxonomy-manager.php`

Finds existing categories using:
1. **Exact match** - "Security" matches "Security"
2. **Levenshtein distance** - "Database" matches "Databases" (distance ≤ 2)
3. **Synonym detection** - "DB" matches "Database"
4. **Similarity percentage** - 85%+ similarity triggers match

**Example:**
```php
$manager = new ThemisDB_Taxonomy_Manager();
$cat_id = $manager->find_matching_category('Databases');
// Returns existing "Database" category instead of creating duplicate
```

### 5. Semantic Parent Mapping

**Hierarchical Assignment:**

Automatically finds parent categories for new terms:

```php
$semantic_mapping = array(
    'Security' => ['authentication', 'encryption', 'oauth', 'jwt', 'ssl'],
    'Performance' => ['caching', 'optimization', 'indexing', 'sharding'],
    'LLM Integration' => ['embeddings', 'vector search', 'rag', 'ai'],
    'Development' => ['api', 'rest', 'grpc', 'sdk', 'docker'],
    'Data Models' => ['graph', 'document', 'key-value', 'time-series'],
    'Operations' => ['monitoring', 'backup', 'recovery', 'migration']
);
```

**Result:**
- "OAuth Authentication" → Creates as child of "Security"
- "Vector Embeddings" → Creates as child of "LLM Integration"
- Automatically assigns both child and parent to post

### 6. Taxonomy Analytics Dashboard

**Access:** WordPress Admin → Taxonomy Analytics

**Features:**
- **Statistics Cards:**
  - Total Categories
  - Total Tags
  - Unused Terms (with cleanup button)
  - Consolidation Suggestions (with auto-merge)

- **Category Distribution:**
  - Categories with posts
  - Empty categories
  - Top-level categories
  - Average posts per category

- **Consolidation Suggestions:**
  - Lists similar categories with similarity %
  - One-click merge functionality
  - Automatic post reassignment

### 7. Configuration Options

**Access:** Settings → Taxonomy Manager

**Available Settings:**

| Setting | Default | Description |
|---------|---------|-------------|
| Max Categories per Post | 5 | Maximum categories to assign |
| Max Tags per Post | 10 | Maximum tags to assign |
| Minimum TF-IDF Score | 0.5 | Relevance threshold (0-1) |
| Similarity Threshold | 0.8 | For merge suggestions (0-1) |
| Prefer Existing Terms | Yes | Reuse existing over creating new |
| Auto-Consolidate | Yes | Automatically merge similar terms |
| Max Category Depth | 3 | Hierarchical depth limit |

## File Structure

```
themisdb-taxonomy-manager/
├── includes/
│   ├── class-tfidf.php              # NEW: TF-IDF calculator
│   ├── class-analytics.php          # NEW: Analytics & consolidation
│   ├── class-taxonomy-extractor.php # ENHANCED: Intelligent extraction
│   ├── class-taxonomy-manager.php   # ENHANCED: Semantic mapping
│   └── class-admin.php              # ENHANCED: Analytics dashboard
├── assets/
│   └── js/
│       └── taxonomy-analytics.js    # NEW: Dashboard interactions
└── verify-implementation.php        # NEW: Verification script
```

## API Reference

### ThemisDB_TFIDF

```php
// Calculate TF-IDF score
$score = $tfidf->calculate_tfidf($term, $text);

// Score multiple terms
$scored = $tfidf->score_terms($terms, $text, $limit);

// Clear cache
$tfidf->clear_cache();
```

### ThemisDB_Taxonomy_Analytics

```php
// Get statistics
$stats = $analytics->get_taxonomy_statistics();

// Get consolidation suggestions
$suggestions = $analytics->get_consolidation_suggestions($threshold);

// Calculate similarity
$similarity = $analytics->calculate_similarity($str1, $str2);

// Check synonyms
$are_synonyms = $analytics->are_synonyms($term1, $term2);

// Cleanup unused
$deleted = $analytics->cleanup_unused_terms('category');

// Auto-consolidate
$results = $analytics->consolidate_categories($threshold);
```

### ThemisDB_Taxonomy_Manager

```php
// Find matching category
$cat_id = $manager->find_matching_category($term, 'category');

// Find semantic parent
$parent_id = $manager->find_semantic_parent($term);

// Assign with hierarchy
$manager->assign_with_hierarchy($post_id, $category_names, $append);

// Consolidate categories
$results = $manager->consolidate_categories($threshold);

// Get recommendations
$recommendations = $manager->get_optimization_recommendations();
```

## AJAX Endpoints

### Consolidate Categories
```javascript
$.post(ajaxurl, {
    action: 'themisdb_consolidate_categories',
    nonce: themisdbTaxonomy.nonce
}, function(response) {
    // response.data.total_merged
    // response.data.details[]
});
```

### Cleanup Unused Terms
```javascript
$.post(ajaxurl, {
    action: 'themisdb_cleanup_unused',
    nonce: themisdbTaxonomy.nonce
}, function(response) {
    // response.data.deleted_categories
    // response.data.deleted_tags
    // response.data.total_deleted
});
```

### Merge Terms
```javascript
$.post(ajaxurl, {
    action: 'themisdb_merge_terms',
    nonce: themisdbTaxonomy.nonce,
    term1_id: keepId,
    term2_id: mergeId
}, function(response) {
    // response.data.posts_moved
    // response.data.message
});
```

## Testing

Run the verification script:
```bash
cd wordpress-plugin/themisdb-taxonomy-manager
php verify-implementation.php
```

## Expected Improvements

### Before Implementation:
- 50+ meaningless categories (Januar, 2026, test, 01)
- Duplicate categories (Database / Databases / DB)
- No hierarchical structure
- Categories and tags overlap

### After Implementation:
- 20-30 meaningful top-level categories
- Hierarchical structure (Security → Authentication → OAuth)
- No overlaps between categories and tags
- Automatic reuse of existing terms
- Interactive consolidation dashboard
- TF-IDF-based relevance scoring

## WordPress Integration

### Automatic Extraction on Post Save

When a post is saved, the plugin:
1. Extracts terms from title and content
2. Scores them using TF-IDF
3. Separates into categories (5 max) and tags (10 max)
4. Checks for existing matches (exact, similar, synonym)
5. Finds semantic parents for hierarchical assignment
6. Assigns categories with parent relationships
7. Prevents overlaps between categories and tags

### Manual Consolidation

Administrators can:
1. View consolidation suggestions in Analytics dashboard
2. Merge individual term pairs with one click
3. Run automatic consolidation for all suggestions
4. Clean up unused terms
5. Review category distribution statistics

## Security Considerations

- All AJAX endpoints check nonce and user capabilities
- SQL queries use prepared statements
- User input is sanitized and escaped
- Only administrators can access analytics and consolidation

## Performance

- TF-IDF calculations are cached per term
- Database queries are optimized with proper indexes
- Analytics calculations run on-demand only
- Consolidation is a manual/scheduled process

## Compatibility

- WordPress: 5.8+
- PHP: 7.4+
- MySQL: 5.7+
- Compatible with WordPress REST API
- Works with Gutenberg and Classic Editor

## Support & Documentation

For issues or questions:
1. Check the verification script output
2. Review WordPress debug.log for errors
3. Test in staging environment first
4. Refer to WordPress Codex for taxonomy functions

## License

MIT License - Same as ThemisDB project
