# Installation Guide - ThemisDB Docker Downloads Plugin

This guide provides detailed installation instructions for the ThemisDB Docker Downloads WordPress plugin.

## Requirements

- **WordPress:** 5.0 or higher
- **PHP:** 7.2 or higher
- **cURL:** Enabled in PHP (for API requests)

## Installation Methods

### Method 1: WordPress Admin Upload (Recommended)

1. **Download the Plugin**
   - Download the latest release as a ZIP file from the GitHub repository
   - Or create a ZIP archive of the `themisdb-docker-downloads` folder

2. **Upload via WordPress Admin**
   - Log in to your WordPress admin panel
   - Navigate to **Plugins → Add New**
   - Click **Upload Plugin** button
   - Click **Choose File** and select the ZIP file
   - Click **Install Now**

3. **Activate the Plugin**
   - After installation completes, click **Activate Plugin**
   - Or navigate to **Plugins → Installed Plugins** and activate it from there

### Method 2: Manual Installation via FTP

1. **Extract the Plugin**
   - Extract the ZIP file to get the `themisdb-docker-downloads` folder

2. **Upload via FTP**
   - Connect to your server via FTP
   - Navigate to `/wp-content/plugins/`
   - Upload the entire `themisdb-docker-downloads` folder

3. **Activate the Plugin**
   - Log in to WordPress admin
   - Go to **Plugins → Installed Plugins**
   - Find "ThemisDB Docker Downloads" and click **Activate**

### Method 3: Manual Installation via File Manager

1. **Access File Manager**
   - Log in to your hosting control panel (cPanel, Plesk, etc.)
   - Open the File Manager

2. **Navigate to Plugins Directory**
   - Go to `public_html/wp-content/plugins/`
   - Or your WordPress installation directory + `/wp-content/plugins/`

3. **Upload the Plugin**
   - Upload the ZIP file
   - Extract it in the plugins directory
   - Ensure the folder name is `themisdb-docker-downloads`

4. **Activate the Plugin**
   - Go to WordPress admin → **Plugins**
   - Activate "ThemisDB Docker Downloads"

## Initial Configuration

After activation, configure the plugin:

1. **Navigate to Settings**
   - Go to **Settings → ThemisDB Docker** in WordPress admin

2. **Configure Docker Hub Settings**
   - **Docker Hub Namespace:** Enter your Docker Hub namespace (default: `themisdb`)
   - **Docker Repository Name:** Enter your repository name (default: `themisdb`)
   - **Cache Duration:** Set cache duration in seconds (recommended: 3600)
   - **Anzahl Tags:** Set number of tags to display (default: 10)

3. **Optional: Add Docker Hub Token**
   - For higher API rate limits, add a Docker Hub Personal Access Token
   - See [Docker Hub Token Setup](#docker-hub-token-setup) below

4. **Test Connection**
   - Click the **Test Connection** button
   - Verify that the connection to Docker Hub is successful

5. **Save Settings**
   - Click **Save Settings** to apply your configuration

## Docker Hub Token Setup

For better API rate limits, create a Docker Hub Personal Access Token:

1. **Log in to Docker Hub**
   - Go to [Docker Hub](https://hub.docker.com/)
   - Sign in to your account

2. **Navigate to Security Settings**
   - Go to **Account Settings → Security**
   - Or visit [https://hub.docker.com/settings/security](https://hub.docker.com/settings/security)

3. **Generate New Token**
   - Click **New Access Token**
   - Enter a description (e.g., "WordPress ThemisDB Plugin")
   - Select **Read-only** permissions
   - Click **Generate**

4. **Copy and Save Token**
   - Copy the generated token immediately (you won't be able to see it again)
   - Go to WordPress admin → **Settings → ThemisDB Docker**
   - Paste the token in the **Docker Hub Token** field
   - Save settings

## Verification

After installation and configuration:

1. **Test the Plugin**
   - Create a new WordPress page or post
   - Add the shortcode: `[themisdb_docker_tags]`
   - Preview or publish the page
   - Verify that Docker tags are displayed correctly

2. **Check Different Views**
   - Try different shortcode variations:
     - `[themisdb_docker_tags style="compact"]`
     - `[themisdb_docker_tags style="table"]`
     - `[themisdb_docker_latest]`

## Troubleshooting

### Plugin Not Appearing in Admin

**Solution:**
- Ensure the folder name is exactly `themisdb-docker-downloads`
- Check file permissions (folders: 755, files: 644)
- Verify PHP version is 7.2 or higher

### Connection Test Fails

**Solution:**
- Check that your server can make outbound HTTP requests
- Verify Docker Hub namespace and repository name are correct
- Ensure the repository is public or you have valid credentials
- Check server firewall settings

### No Tags Displayed

**Solution:**
- Clear the plugin cache
- Check WordPress debug log for errors
- Verify the Docker repository exists and has tags
- Test connection in admin panel

### Permission Errors

**Solution:**
- Verify you have admin privileges in WordPress
- Check file permissions on plugin directory
- Ensure wp-content/plugins is writable

## Uninstallation

To remove the plugin:

1. **Deactivate**
   - Go to **Plugins → Installed Plugins**
   - Click **Deactivate** under ThemisDB Docker Downloads

2. **Delete Plugin Data** (Optional)
   - Plugin automatically clears cache on deactivation
   - Settings remain in database for reinstallation

3. **Delete Plugin**
   - Click **Delete** under the deactivated plugin
   - Confirm deletion
   - This removes all plugin files

4. **Clean Database** (Optional)
   - If you want to remove all settings, run these SQL commands:
   ```sql
   DELETE FROM wp_options WHERE option_name LIKE 'themisdb_docker_%';
   DELETE FROM wp_transients WHERE option_name LIKE '_transient_themisdb_docker_%';
   ```

## Updating the Plugin

### Manual Update

1. Deactivate the current version
2. Delete the old plugin files (or backup first)
3. Install the new version following installation methods above
4. Activate the plugin
5. Review settings and test functionality

### Automatic Update (Future)

When the plugin is submitted to WordPress.org, updates will be automatic through the WordPress admin panel.

## Additional Resources

- **Plugin Documentation:** See README.md in the plugin directory
- **Support:** [GitHub Issues](https://github.com/makr-code/wordpressPlugins/issues)
- **ThemisDB Documentation:** [docs/de/deployment/](../../docs/de/deployment/)

## File Structure Verification

After installation, verify the following structure exists:

```
wp-content/plugins/themisdb-docker-downloads/
├── themisdb-docker-downloads.php
├── README.md
├── CHANGELOG.md
├── INSTALLATION.md
├── LICENSE
├── includes/
│   ├── class-dockerhub-api.php
│   ├── class-admin.php
│   └── class-shortcodes.php
└── assets/
    ├── css/
    │   ├── style.css
    │   └── admin.css
    └── js/
        ├── script.js
        └── admin.js
```

If any files are missing, reinstall the plugin.
