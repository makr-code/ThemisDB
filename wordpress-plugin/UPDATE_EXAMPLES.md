# Automatic Updates - Usage Examples

This document provides practical examples for using the automatic update system.

---

## For WordPress Administrators

### Checking for Updates

1. **Automatic Check** (Default)
   - WordPress checks twice daily automatically
   - No action required

2. **Manual Check**
   ```
   Dashboard → Updates → Click "Check Again"
   ```

### Installing Updates

1. Navigate to: `Dashboard → Updates`
2. Find plugin in "Plugins" section
3. Click "Update Now"
4. Wait for confirmation

### Viewing Update Details

Before updating:
1. Click "View details" next to the update notification
2. Review:
   - Version number
   - Changelog
   - Requirements (WordPress & PHP versions)
3. Decide whether to update

---

## For Plugin Developers

### Creating a New Release

**Step 1: Update Version Numbers**

In your plugin's main PHP file:
```php
/**
 * Version: 1.1.0
 */
define('PLUGIN_VERSION', '1.1.0');
```

In `update-info.json`:
```json
{
  "version": "1.1.0",
  ...
}
```

**Step 2: Create GitHub Release**

```bash
# Commit changes
git add .
git commit -m "Release v1.1.0"
git push

# Create and push tag
git tag v1.1.0
git push origin v1.1.0

# Create GitHub release (via UI or CLI)
gh release create v1.1.0 \
  --title "Version 1.1.0" \
  --notes "## What's New
  
- Added feature X
- Fixed bug Y
- Improved performance Z"
```

**Step 3: Wait for Distribution**

- Updates appear within 12 hours (cache duration)
- Or immediately after manual update check

### Testing Updates Locally

**Option 1: Using wp-env (Recommended)**

```bash
# Install wp-env
npm -g install @wordpress/env

# Start WordPress
wp-env start

# Install plugin
wp-env run cli wp plugin install /path/to/plugin --activate

# Check for updates
wp-env run cli wp plugin update --all --dry-run
```

**Option 2: Using Local WordPress Installation**

```bash
# Copy plugin to WordPress
cp -r themisdb-plugin-name /path/to/wordpress/wp-content/plugins/

# Activate plugin via WP-CLI
wp plugin activate themisdb-plugin-name

# Force update check
wp transient delete --all
wp plugin update --all --dry-run
```

---

## For Repository Maintainers

### Bulk Release Process

When releasing multiple plugins:

```bash
#!/bin/bash
# Update all plugin versions
VERSION="1.1.0"

# Update each plugin's main file and update-info.json
for plugin in wordpress-plugin/themisdb-*; do
  echo "Updating $plugin to $VERSION"
  # Update version in main PHP file
  # Update version in update-info.json
done

# Commit and create release
git add .
git commit -m "Release v$VERSION - All plugins"
git tag v$VERSION
git push origin main --tags

# Create GitHub release
gh release create v$VERSION \
  --title "ThemisDB WordPress Plugins v$VERSION" \
  --notes-file CHANGELOG.md
```

### Creating Plugin ZIP Files

For optimal distribution, create ZIP files for each plugin:

```bash
#!/bin/bash
cd wordpress-plugin

for plugin_dir in themisdb-*; do
  if [ -d "$plugin_dir" ]; then
    echo "Creating ZIP for $plugin_dir"
    zip -r "${plugin_dir}.zip" "$plugin_dir" \
      -x "*.git*" "*.DS_Store" "node_modules/*"
  fi
done

# Upload to GitHub release
gh release upload v1.1.0 *.zip
```

---

## Troubleshooting Examples

### Problem: Update Not Appearing

**Solution:**
```bash
# Clear transient cache
wp transient delete themisdb_update_plugin-slug

# Or via WordPress Admin
Dashboard → Updates → Check Again
```

### Problem: Update Fails

**Check 1: File Permissions**
```bash
# Ensure WordPress can write to plugins directory
sudo chown -R www-data:www-data /var/www/html/wp-content/plugins
```

**Check 2: GitHub API**
```bash
# Test API connectivity
curl -I https://api.github.com/repos/makr-code/ThemisDB/releases/latest

# Test metadata
curl https://raw.githubusercontent.com/makr-code/ThemisDB/main/wordpress-plugin/plugin-slug/update-info.json
```

**Check 3: WordPress Debug**
```php
// Add to wp-config.php
define('WP_DEBUG', true);
define('WP_DEBUG_LOG', true);

// Check debug.log for errors
tail -f wp-content/debug.log
```

### Problem: Rate Limited by GitHub

**Solution: Add GitHub Token**
```php
// In wp-config.php (NEVER commit this!)
define('THEMISDB_GITHUB_TOKEN', 'ghp_xxxxxxxxxxxx');
```

Then update the updater class to use the token:
```php
$response = wp_remote_get($api_url, array(
    'timeout' => 10,
    'headers' => array(
        'Accept' => 'application/vnd.github.v3+json',
        'Authorization' => defined('THEMISDB_GITHUB_TOKEN') 
            ? 'token ' . THEMISDB_GITHUB_TOKEN 
            : '',
    ),
));
```

---

## Integration Examples

### With CI/CD Pipeline

**GitHub Actions Example:**

```yaml
name: Release WordPress Plugin

on:
  push:
    tags:
      - 'v*'

jobs:
  release:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      
      - name: Create plugin ZIPs
        run: |
          cd wordpress-plugin
          for plugin in themisdb-*; do
            zip -r "${plugin}.zip" "$plugin"
          done
      
      - name: Create Release
        uses: softprops/action-gh-release@v1
        with:
          files: wordpress-plugin/*.zip
        env:
          GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}
```

### With Automated Testing

**Test Updates Before Release:**

```bash
#!/bin/bash
# test-update.sh

# Start test WordPress instance
wp-env start

# Install old version
wp-env run cli wp plugin install ./old-plugin.zip --activate

# Trigger update check
wp-env run cli wp transient delete --all

# Install update
wp-env run cli wp plugin update plugin-name

# Run tests
wp-env run cli wp eval "if (defined('PLUGIN_VERSION') && PLUGIN_VERSION === '1.1.0') { echo 'Update successful'; exit(0); } else { exit(1); }"
```

---

## Monitoring Examples

### Check Update Status

**Via WP-CLI:**
```bash
# List available updates
wp plugin list --fields=name,version,update --format=table

# Check specific plugin
wp plugin get themisdb-plugin-name --field=update_version
```

**Via WordPress REST API:**
```bash
# Get plugin info (requires authentication)
curl -X GET "https://example.com/wp-json/wp/v2/plugins" \
  -H "Authorization: Bearer YOUR_TOKEN"
```

### Log Update Events

Add to your plugin:
```php
add_action('upgrader_process_complete', function($upgrader, $options) {
    if ($options['type'] === 'plugin' && 
        in_array('themisdb-plugin-name/themisdb-plugin-name.php', $options['plugins'])) {
        
        error_log('ThemisDB Plugin updated to version ' . PLUGIN_VERSION);
        
        // Optional: Send notification
        wp_mail(
            'admin@example.com',
            'Plugin Updated',
            'ThemisDB Plugin updated to version ' . PLUGIN_VERSION
        );
    }
}, 10, 2);
```

---

## Advanced Examples

### Custom Update Server

If you want to use a different update source:

```php
new ThemisDB_Plugin_Updater(
    PLUGIN_FILE,
    'plugin-slug',
    PLUGIN_VERSION,
    'your-github-username',  // Custom GitHub username
    'your-repository'         // Custom repository
);
```

### Beta/Development Releases

For testing development versions:

```php
// In update-info.json, use pre-release tags
{
  "version": "1.1.0-beta.1",
  ...
}

// WordPress will offer updates to 1.1.0-beta.1 > 1.0.0
```

---

## Best Practices

### 1. Semantic Versioning
```
MAJOR.MINOR.PATCH
  ^     ^     ^
  |     |     |
  |     |     +-- Bug fixes
  |     +-------- New features (backward compatible)
  +-------------- Breaking changes
```

### 2. Always Test Updates
```bash
# Before release:
1. Test on staging environment
2. Verify with different WP versions
3. Check PHP compatibility
4. Test rollback scenario
```

### 3. Communicate Changes
```markdown
## Changelog

### [1.1.0] - 2026-02-17

#### Added
- New feature X with Y capability

#### Changed
- Improved performance of Z

#### Fixed
- Bug causing issue A
```

### 4. Monitor After Release
```bash
# Check error logs
wp-env run cli wp eval "var_dump(get_option('recently_activated'));"

# Monitor user reports
# Watch GitHub issues
# Check support forums
```

---

## Examples Summary

| Task | Command/Action |
|------|----------------|
| Check for updates | `Dashboard → Updates → Check Again` |
| Install update | `Dashboard → Updates → Update Now` |
| View details | Click "View details" link |
| Create release | `git tag v1.1.0 && git push --tags` |
| Test locally | `wp plugin update plugin-name --dry-run` |
| Clear cache | `wp transient delete themisdb_update_*` |
| Debug issues | Enable WP_DEBUG and check logs |

---

For more information, see [AUTOMATIC_UPDATES.md](AUTOMATIC_UPDATES.md)
