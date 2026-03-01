# ThemisDB Taxonomy Manager - Testing Guide

## Overview
This document provides testing procedures for the ThemisDB Taxonomy Manager plugin v1.0.0.

## Pre-Testing Setup

### Requirements
- WordPress 5.0 or higher
- PHP 7.2 or higher
- Admin access to WordPress installation

### Installation Steps
1. Upload plugin folder to `/wp-content/plugins/themisdb-taxonomy-manager/`
2. Activate plugin in WordPress Admin → Plugins
3. Default taxonomies and terms should be created automatically

## Test Cases

### 1. Custom Taxonomies Registration

#### Test 1.1: Verify Taxonomies Exist
- Navigate to Posts → Add New
- Check for custom taxonomy meta boxes:
  - Database Features
  - Use Cases
  - Industries
  - Technical Specs
- **Expected**: All 4 taxonomies should be visible

#### Test 1.2: Verify Default Terms
- Navigate to Posts → Database Features
- **Expected**: Should see parent terms:
  - Data Models (with icon 📊)
  - AI/ML (with icon 🤖)
  - Performance (with icon ⚡)
  - Compatibility (with icon 🔗)
- Each parent should have child terms

#### Test 1.3: Verify Term Meta
- Edit any feature term
- **Expected**: Should see:
  - Icon (Emoji) field
  - Color picker field

### 2. Tree View Admin

#### Test 2.1: Access Tree View
- Navigate to ThemisDB → Taxonomy Tree
- **Expected**: Tree view page should load

#### Test 2.2: Taxonomy Selector
- Use dropdown to switch between taxonomies
- **Expected**: Page should reload with selected taxonomy

#### Test 2.3: Expand/Collapse
- Click triangle icon on parent terms
- Click "Expand All" button
- Click "Collapse All" button
- **Expected**: Children should show/hide smoothly

#### Test 2.4: Search/Filter
- Type in search box
- **Expected**: Only matching terms should be visible

#### Test 2.5: Export JSON
- Click "Export JSON" button
- **Expected**: JSON file should download with current date in filename

### 3. Drag & Drop

#### Test 3.1: Reorder Terms
- Drag a term to new position
- **Expected**: 
  - Visual feedback during drag
  - Success notification after drop
  - Order persists on page reload

#### Test 3.2: Move Between Levels
- Drag a term from one parent to another
- **Expected**: Term should move successfully

### 4. Enhanced Meta Box

#### Test 4.1: Features Meta Box
- Create/edit a post
- Navigate to Database Features meta box
- **Expected**:
  - Features grouped by parent category
  - Each group shows icon and color
  - Checkboxes for parent and children

#### Test 4.2: Save Selections
- Select multiple features
- Save post
- Reload post editor
- **Expected**: Selections should persist

#### Test 4.3: Use Cases/Industries
- Check Use Cases meta box
- Check Industries meta box
- **Expected**: Simple checkbox list display

#### Test 4.4: Tech Specs (Tags)
- Check Technical Specs meta box
- **Expected**: Tag-style input with comma separation

### 5. Widgets

#### Test 5.1: Widget Registration
- Navigate to Appearance → Widgets
- **Expected**: "ThemisDB Taxonomy" widget should be available

#### Test 5.2: Widget Configuration
- Add widget to sidebar
- Configure:
  - Title: "Database Features"
  - Taxonomy: Features
  - Style: List
  - Show Count: Yes
- Save widget
- **Expected**: Configuration should save

#### Test 5.3: Widget Display - List
- View frontend with widget
- **Expected**: 
  - Features displayed as list
  - Icons visible
  - Post counts shown

#### Test 5.4: Widget Display - Cloud
- Change widget style to Cloud
- **Expected**: Terms displayed as tag cloud with varying sizes

#### Test 5.5: Widget Display - Grid
- Change widget style to Grid
- **Expected**: Terms displayed as cards with icons and colors

### 6. Shortcodes

#### Test 6.1: Taxonomy List Shortcode
- Create a page
- Add shortcode: `[themisdb_taxonomy taxonomy="themisdb_feature" style="list"]`
- Publish and view
- **Expected**: Features displayed as list

#### Test 6.2: Taxonomy Cloud Shortcode
- Add shortcode: `[themisdb_taxonomy taxonomy="themisdb_usecase" style="cloud"]`
- **Expected**: Use cases displayed as tag cloud

#### Test 6.3: Taxonomy Grid Shortcode
- Add shortcode: `[themisdb_taxonomy taxonomy="themisdb_industry" style="grid"]`
- **Expected**: Industries displayed as grid

#### Test 6.4: Taxonomy Info Shortcode
- Get term ID from Features
- Add shortcode: `[themisdb_taxonomy_info term_id="123"]`
- **Expected**: Term details displayed with icon, name, description, count

### 7. Archive Templates

#### Test 7.1: Feature Archive
- Click on a feature link
- **Expected**:
  - Custom header with gradient background
  - Icon displayed
  - Term name and description
  - Post count
  - List of posts with that feature

#### Test 7.2: Breadcrumbs
- View a child term archive (e.g., "Embedded LLM")
- **Expected**: Breadcrumb showing: Home / Features / AI/ML / Embedded LLM

### 8. REST API

#### Test 8.1: Get All Taxonomies
```bash
curl http://yoursite.com/wp-json/themisdb/v1/taxonomies
```
- **Expected**: JSON array of 4 taxonomies

#### Test 8.2: Get Taxonomy Terms
```bash
curl http://yoursite.com/wp-json/themisdb/v1/taxonomy/themisdb_feature
```
- **Expected**: JSON array of all feature terms with metadata

#### Test 8.3: Get Taxonomy Tree
```bash
curl http://yoursite.com/wp-json/themisdb/v1/taxonomy/themisdb_feature/tree
```
- **Expected**: Hierarchical JSON structure

### 9. SEO & Schema.org

#### Test 9.1: Schema Markup
- View any taxonomy archive page
- View page source
- Search for `application/ld+json`
- **Expected**: Schema.org CollectionPage markup present

#### Test 9.2: Validate Schema
- Use Google's Rich Results Test
- **Expected**: No errors in schema markup

### 10. Admin Settings

#### Test 10.1: Access Settings
- Navigate to Settings → Taxonomy Manager
- **Expected**: Settings page should load with tabs

#### Test 10.2: Basic Settings
- Toggle each checkbox setting
- Save changes
- **Expected**: Settings should persist

#### Test 10.3: Color Picker
- Change default color
- **Expected**: Color picker should work

#### Test 10.4: Optimization Tab
- Switch to Optimization tab
- Click "Get Recommendations"
- **Expected**: Recommendations displayed (or message if none)

#### Test 10.5: Consolidation
- Click "Run Consolidation"
- **Expected**: Results displayed with stats

#### Test 10.6: Category Hierarchy Tab
- Switch to Category Hierarchy tab
- **Expected**: Tree display of current categories

## Automated Testing Checklist

- [ ] All PHP files pass syntax check (`php -l`)
- [ ] No JavaScript console errors
- [ ] CSS loads correctly
- [ ] AJAX requests complete successfully
- [ ] Database queries execute without errors

## Performance Testing

### Test Load Times
1. Create 100+ posts with taxonomies
2. Test tree view rendering time
3. Test widget rendering time
4. Test archive page load time
- **Expected**: All should load in < 2 seconds

### Test Memory Usage
- Monitor PHP memory usage during operations
- **Expected**: Should stay within WordPress limits

## Browser Compatibility

Test in:
- [ ] Chrome (latest)
- [ ] Firefox (latest)
- [ ] Safari (latest)
- [ ] Edge (latest)

## Mobile Responsiveness

- [ ] Tree view on mobile (< 768px)
- [ ] Widgets on mobile
- [ ] Archive pages on mobile
- [ ] Meta boxes on mobile

## Security Testing

- [ ] Verify nonce checks in AJAX handlers
- [ ] Test capability checks for admin actions
- [ ] Verify input sanitization
- [ ] Test SQL injection prevention
- [ ] Test XSS prevention

## Bug Tracking

### Known Issues
- None at this time

### Resolved Issues
- Initial implementation complete

## Test Results Summary

| Test Category | Total Tests | Passed | Failed | Notes |
|---------------|-------------|--------|--------|-------|
| Taxonomies    | 3           | -      | -      |       |
| Tree View     | 5           | -      | -      |       |
| Drag & Drop   | 2           | -      | -      |       |
| Meta Boxes    | 4           | -      | -      |       |
| Widgets       | 5           | -      | -      |       |
| Shortcodes    | 4           | -      | -      |       |
| Archives      | 2           | -      | -      |       |
| REST API      | 3           | -      | -      |       |
| SEO           | 2           | -      | -      |       |
| Settings      | 6           | -      | -      |       |

## Sign-Off

- [ ] Developer Testing Complete
- [ ] QA Testing Complete
- [ ] User Acceptance Testing Complete
- [ ] Ready for Production

---

**Version**: 1.0.0  
**Last Updated**: 2026-02-11  
**Tester**: _____________  
**Date**: _____________
