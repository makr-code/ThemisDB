# ThemisDB Release Timeline - WordPress Plugin

Interactive visualization of ThemisDB releases and features.

## 📋 Overview

Displays chronological timeline of ThemisDB releases with feature highlights, breaking changes, and migration guides.

- **Shortcode**: `[themisdb_release_timeline]`
- **Visualizes**: Releases from CHANGELOG.md and GitHub
- **Features**: Version filtering, feature highlights, breaking changes

## ✨ Features

### Timeline Visualization
- 📅 **Chronological Display**: All releases in timeline format
- 🎯 **Feature Highlights**: Key features per version
- ⚠️ **Breaking Changes**: Clear warnings for BC breaks
- 📖 **Migration Guides**: Direct links to upgrade docs
- 🎨 **Mermaid.js Timeline**: Visual timeline diagram

### Filtering
- Filter by Major/Minor/Patch versions
- Date range filtering
- Feature category filtering

## 🚀 Installation

1. Copy plugin to `/wp-content/plugins/themisdb-release-timeline/`
2. Activate in WordPress Admin → Plugins
3. Configure in Settings → Release Timeline

## 📖 Usage

### Basic
```php
[themisdb_release_timeline]
```

### Filter Major Versions Only
```php
[themisdb_release_timeline version="major_only"]
```

### Date Range
```php
[themisdb_release_timeline from="v1.0.0" to="v1.4.0"]
```

### Custom Display
```php
[themisdb_release_timeline limit="10" show_breaking_changes="true"]
```

## ⚙️ Settings

- **Data Source**: CHANGELOG.md, GitHub Releases API
- **Update Frequency**: Daily cache refresh
- **Display Options**: Timeline style, colors, icons

## 📄 License

MIT License

---

**Phase 3.1** - ThemisDB WordPress Plugins Suite
