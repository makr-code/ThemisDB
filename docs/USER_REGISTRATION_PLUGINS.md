# User Registration Plugins

## Overview

ThemisDB supports plugin-based user registration, allowing integration with external identity providers and data sources. User registration is delegated to plugins rather than being handled directly by the AccessControl class.

## Primary Plugins

### 1. Apache Arrow Plugin (`arrow`)

**Purpose:** Bulk user imports from columnar data sources

**Features:**
- Import users from Parquet files
- Apache Arrow Flight server integration
- Bulk synchronization from analytical databases
- Columnar data mapping (username, password_hash, roles)

**Configuration:**
```cpp
ArrowUserRegistrationPlugin::Config config;
config.arrow_source_uri = "file:///data/users.parquet";
config.username_column = "username";
config.password_column = "password_hash";
config.roles_column = "roles";
config.auto_sync = true;
```

**Use Cases:**
- Bulk import users from data warehouses
- Synchronize users from analytical databases (ClickHouse, DuckDB)
- Import users from Parquet files exported from other systems
- Arrow Flight-based real-time user provisioning

**Example:**
```cpp
auto arrow_plugin = std::make_shared<ArrowUserRegistrationPlugin>(config);
access_control.getUserRegistrationPluginManager().registerPlugin(arrow_plugin);

// Register user via Arrow
access_control.registerUser("alice@example.com", "password", "arrow");
```

### 2. WebDAV Plugin (`webdav`)

**Purpose:** Integration with WebDAV servers, Active Directory, and SharePoint

**Features:**
- Active Directory authentication via WebDAV
- SharePoint user management integration
- OwnCloud/Nextcloud user synchronization
- PROPFIND-based user directory queries
- AD group to role mapping

**Configuration:**
```cpp
WebDAVUserRegistrationPlugin::Config config;
config.webdav_base_url = "https://sharepoint.company.com";
config.webdav_username = "admin";
config.webdav_password = "admin_password";
config.user_directory = "/users";
config.active_directory_mode = true;
config.verify_ssl = true;
```

**Use Cases:**
- Corporate Active Directory integration
- SharePoint document library access control
- Network file server authentication
- Enterprise single sign-on (SSO) via WebDAV

**Example:**
```cpp
auto webdav_plugin = std::make_shared<WebDAVUserRegistrationPlugin>(config);
access_control.getUserRegistrationPluginManager().registerPlugin(webdav_plugin);

// Register user via WebDAV (authenticates against AD)
access_control.registerUser("bob@company.com", "password", "webdav");
```

## Plugin Interface

All user registration plugins implement the `IUserRegistrationPlugin` interface:

```cpp
class IUserRegistrationPlugin {
public:
    // Get plugin name
    virtual std::string getName() const = 0;
    
    // Check if plugin is available
    virtual bool isAvailable() const = 0;
    
    // Register user through plugin
    virtual Result<UserRegistrationData> registerUser(
        const std::string& user_id,
        const std::string& password,
        const std::unordered_map<std::string, std::string>& attributes = {}
    ) = 0;
    
    // Authenticate user through plugin
    virtual Result<UserRegistrationData> authenticateUser(
        const std::string& user_id,
        const std::string& password
    ) = 0;
    
    // Batch synchronization
    virtual Result<std::vector<UserRegistrationData>> syncUsers() = 0;
    
    // Update user information
    virtual Result<UserRegistrationData> updateUser(
        const std::string& user_id
    ) = 0;
};
```

## User Registration Data

Plugins return `UserRegistrationData` containing:

```cpp
struct UserRegistrationData {
    std::string user_id;              // User identifier
    std::string password_hash;        // Pre-hashed password
    std::vector<std::string> roles;   // Assigned roles
    std::unordered_map<std::string, std::string> attributes;
    std::string source;               // Plugin name ("arrow", "webdav")
    std::string source_uri;           // Original source URI
};
```

## Plugin Manager

The `UserRegistrationPluginManager` manages multiple plugins:

```cpp
// Register plugins
auto& plugin_mgr = access_control.getUserRegistrationPluginManager();
plugin_mgr.registerPlugin(arrow_plugin);
plugin_mgr.registerPlugin(webdav_plugin);

// Get available plugins
auto plugins = plugin_mgr.getAvailablePlugins();

// Get specific plugin
auto arrow = plugin_mgr.getPlugin("arrow");

// Get default plugin (prefers arrow, then webdav)
auto default_plugin = plugin_mgr.getDefaultPlugin();
```

## Usage Examples

### Example 1: Register User via Default Plugin

```cpp
// Uses first available plugin (arrow > webdav > first available)
auto result = access_control.registerUser("user@example.com", "password");
```

### Example 2: Register User via Specific Plugin

```cpp
// Explicitly use Arrow plugin
auto result = access_control.registerUser(
    "user@example.com",
    "password",
    "arrow",  // plugin name
    {{"department", "engineering"}}  // attributes
);
```

### Example 3: Register User via WebDAV (Active Directory)

```cpp
// Authenticates against AD, maps AD groups to roles
auto result = access_control.registerUser(
    "bob@company.com",
    "ad_password",
    "webdav"
);
```

### Example 4: Bulk User Synchronization

```cpp
auto& plugin_mgr = access_control.getUserRegistrationPluginManager();
auto arrow_plugin = plugin_mgr.getPlugin("arrow");

if (arrow_plugin) {
    auto sync_result = arrow_plugin->syncUsers();
    if (sync_result.is_ok()) {
        for (const auto& user_data : sync_result.value()) {
            // Process each user
            // password already hashed by plugin
        }
    }
}
```

## Build Configuration

### Apache Arrow Support

Enable Arrow plugin at build time:

```bash
cmake -DTHEMIS_ENABLE_ARROW=ON ..
```

The Arrow plugin requires:
- Apache Arrow C++ library
- Parquet library (for Parquet file support)

### WebDAV Support

Enable WebDAV plugin at build time:

```bash
cmake -DTHEMIS_ENABLE_WEBDAV=ON ..
```

The WebDAV plugin requires:
- libcurl (for HTTP/WebDAV requests)
- OpenSSL (for HTTPS support)

## Security Considerations

### Password Handling

- Plugins are responsible for password hashing
- Use SHA-256 for compatibility (upgrade to bcrypt/Argon2 recommended)
- Passwords never stored in plain text

### WebDAV Authentication

- Supports HTTPS with SSL certificate verification
- Basic authentication over secure connection
- Optional: Disable SSL verification for testing (not recommended for production)

### Active Directory Integration

- Groups mapped to roles automatically
- AD properties (displayName, mail) imported as user attributes
- Supports nested group membership

### Apache Arrow Security

- Parquet files should be access-controlled at filesystem level
- Arrow Flight supports TLS and authentication
- Validate data integrity before import

## Extending with Custom Plugins

Create a custom plugin by implementing `IUserRegistrationPlugin`:

```cpp
class CustomUserRegistrationPlugin : public IUserRegistrationPlugin {
public:
    std::string getName() const override {
        return "custom";
    }
    
    bool isAvailable() const override {
        return true;  // Check if external service is available
    }
    
    Result<UserRegistrationData> registerUser(
        const std::string& user_id,
        const std::string& password,
        const std::unordered_map<std::string, std::string>& attributes
    ) override {
        // 1. Validate with external service
        // 2. Hash password
        // 3. Map roles and attributes
        // 4. Return UserRegistrationData
        
        UserRegistrationData data;
        data.user_id = user_id;
        data.password_hash = hashPassword(password);
        data.source = "custom";
        data.roles.push_back("readonly");
        
        return Result<UserRegistrationData>::Ok(data);
    }
    
    // Implement other methods...
};
```

Register the custom plugin:

```cpp
auto custom_plugin = std::make_shared<CustomUserRegistrationPlugin>();
access_control.getUserRegistrationPluginManager().registerPlugin(custom_plugin);
```

## Testing

Use the mock plugin for unit tests:

```cpp
#include "mock_user_registration_plugin.h"

auto mock_plugin = std::make_shared<MockUserRegistrationPlugin>();
access_control.getUserRegistrationPluginManager().registerPlugin(mock_plugin);

// Test registration
auto result = access_control.registerUser("test@example.com", "password");
EXPECT_TRUE(result.is_ok());
```

## Troubleshooting

### No Plugins Available

```
Error: No user registration plugins available
```

**Solution:** Register at least one plugin before calling `registerUser()`:

```cpp
auto plugin = std::make_shared<ArrowUserRegistrationPlugin>(config);
access_control.getUserRegistrationPluginManager().registerPlugin(plugin);
```

### Plugin Not Found

```
Error: User registration plugin not found: arrow
```

**Solution:** Check plugin name and ensure it's registered:

```cpp
auto plugins = plugin_mgr.getAvailablePlugins();
for (const auto& p : plugins) {
    std::cout << "Available: " << p->getName() << std::endl;
}
```

### WebDAV Authentication Failed

```
Error: WebDAV authentication failed: Invalid credentials
```

**Solutions:**
- Verify username and password
- Check WebDAV server URL
- Ensure SSL certificate is valid (or disable verification for testing)
- Verify user has access to specified directory

### Arrow Build Not Enabled

```
Error: Apache Arrow support not enabled in build
```

**Solution:** Rebuild with Arrow support:

```bash
cmake -DTHEMIS_ENABLE_ARROW=ON ..
cmake --build .
```

## Performance Considerations

### Apache Arrow

- **Batch operations:** Use `syncUsers()` for bulk imports (efficient for 1000s of users)
- **Columnar format:** Arrow's columnar format is ideal for large datasets
- **Memory efficiency:** Arrow uses zero-copy when possible

### WebDAV

- **Network latency:** WebDAV requests involve network round-trips
- **Caching:** Consider caching user data locally after authentication
- **Connection pooling:** Reuse HTTP connections for multiple requests

## Compliance

### Active Directory Integration

- LDAP/AD properties automatically mapped to user attributes
- Group membership determines role assignments
- Audit logs capture AD-based registrations

### Data Privacy

- User data remains under organizational control
- No external cloud services required
- Supports on-premise deployments

## See Also

- [Access Control Framework Documentation](SECURITY_FRAMEWORK_IMPLEMENTATION.md)
- [RBAC Configuration](../include/security/rbac.h)
- [Plugin Interface](../include/security/user_registration_plugin.h)
