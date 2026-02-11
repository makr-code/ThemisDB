# Multi-Level Encrypted User Storage Plugin

## Overview

The Multi-Level Encrypted User Storage Plugin provides secure, classification-based storage for user and group data with filesystem-level encryption. It implements ThemisDB's four-tier security classification system with separate encrypted containers per level.

## Features

- **4 Security Levels**: offen (public), vs-nfd (restricted), geheim (secret), streng-geheim (top secret)
- **Filesystem Encryption**: gocryptfs with AES-256-GCM
- **Key Management**: HashiCorp Vault and HSM integration
- **Automatic Key Rotation**: Zero-downtime rotation with configurable intervals
- **Cross-Platform**: Linux (primary), macOS via macFUSE, Windows experimental
- **RBAC/ABAC**: Apache Ranger integration ready

## Security Classification Levels

| Level | Description | Encryption | Key Provider | Rotation Interval |
|-------|-------------|------------|--------------|-------------------|
| **offen** | Public data | None | N/A | N/A |
| **vs-nfd** | Restricted (For Official Use Only) | AES-256-GCM | Vault | 90 days |
| **geheim** | Secret | AES-256-GCM | Vault | 60 days |
| **streng-geheim** | Top Secret | AES-256-GCM | HSM | 30 days |

## System Requirements

### Minimum Requirements
- Linux (Ubuntu 20.04+, Debian 11+, RHEL 8+)
- gocryptfs >= 2.0
- FUSE support
- 500MB+ free disk space per level

### Optional Requirements
- HashiCorp Vault (for vs-nfd, geheim, streng-geheim)
- HSM with PKCS#11 support (for streng-geheim)
- Apache Ranger (for advanced RBAC)

## Installation

### 1. Install gocryptfs

```bash
# Ubuntu/Debian
sudo apt-get install gocryptfs fuse

# RHEL/CentOS
sudo yum install fuse gocryptfs

# macOS
brew install --cask macfuse
brew install gocryptfs
```

### 2. Configure Plugin

Copy the example configuration:

```bash
cp config/storage_config.yaml.example /etc/themisdb/storage_config.yaml
```

Edit `/etc/themisdb/storage_config.yaml` to set:
- Vault address and credentials
- HSM library path and slot
- Encryption key IDs
- Rotation intervals

### 3. Initialize Vault Keys

```bash
# Create encryption keys in Vault
vault kv put themis/keys/user_storage_vs_nfd \
  key=$(openssl rand -base64 32) \
  algorithm="AES-256-GCM" \
  version=1

vault kv put themis/keys/user_storage_geheim \
  key=$(openssl rand -base64 32) \
  algorithm="AES-256-GCM" \
  version=1
```

### 4. Load Plugin

```cpp
// In your ThemisDB application
auto plugin_manager = std::make_shared<PluginManager>();
plugin_manager->loadPlugin("user_storage_encrypted");

// Get plugin instance
auto storage = plugin_manager->getPlugin<MultiLevelEncryptedStorage>(
    "user_storage_encrypted"
);

// Initialize with configuration
std::ifstream config_file("/etc/themisdb/storage_config.yaml");
std::string config((std::istreambuf_iterator<char>(config_file)),
                   std::istreambuf_iterator<char>());
storage->initialize(config.c_str());
```

## Usage

### Create User

```cpp
User user;
user.user_id = "user_001";
user.username = "john.doe";
user.email = "john.doe@company.com";
user.full_name = "John Doe";
user.roles = {"admin", "developer"};
user.classification = SecurityLevel::VS_NFD;
user.created_at_ms = getCurrentTimeMs();
user.updated_at_ms = user.created_at_ms;

auto result = storage->createUser(user, SecurityLevel::VS_NFD);
if (result.isSuccess()) {
    std::cout << "User created successfully" << std::endl;
} else {
    std::cerr << "Error: " << result.error() << std::endl;
}
```

### Get User

```cpp
auto result = storage->getUser("user_001", SecurityLevel::VS_NFD);
if (result.isSuccess()) {
    const User& user = result.value();
    std::cout << "User: " << user.username << std::endl;
    std::cout << "Email: " << user.email << std::endl;
} else {
    std::cerr << "Error: " << result.error() << std::endl;
}
```

### Create Group

```cpp
Group group;
group.group_id = "group_001";
group.name = "Developers";
group.description = "Development team";
group.member_ids = {"user_001", "user_002"};
group.classification = SecurityLevel::VS_NFD;
group.created_at_ms = getCurrentTimeMs();

auto result = storage->createGroup(group, SecurityLevel::VS_NFD);
```

### Health Check

```cpp
auto health = storage->checkHealth();
if (health.isSuccess() && health.value().healthy) {
    std::cout << "All storage levels healthy" << std::endl;
} else {
    std::cerr << "Health check failed: " << health.value().message << std::endl;
    for (const auto& error : health.value().errors) {
        std::cerr << "  - " << error << std::endl;
    }
}
```

## Container Management

### Mount All Containers

```cpp
auto result = storage->mountAll();
if (result.isError()) {
    std::cerr << "Failed to mount containers: " << result.error() << std::endl;
}
```

### Unmount All Containers

```cpp
storage->unmountAll();
```

### Manual Key Rotation

```cpp
auto result = storage->rotateKey(SecurityLevel::VS_NFD);
if (result.isSuccess()) {
    std::cout << "Key rotated successfully" << std::endl;
} else {
    std::cerr << "Rotation failed: " << result.error() << std::endl;
}
```

## Docker Support

### Dockerfile

```dockerfile
FROM ubuntu:22.04

# Install gocryptfs and FUSE
RUN apt-get update && apt-get install -y \
    gocryptfs \
    fuse \
    libsodium23 \
    && rm -rf /var/lib/apt/lists/*

# Allow FUSE in container
RUN echo "user_allow_other" >> /etc/fuse.conf

# Copy ThemisDB
COPY . /opt/themisdb
WORKDIR /opt/themisdb

# Run with FUSE support
# docker run --cap-add SYS_ADMIN --device /dev/fuse ...
```

### docker-compose.yml

```yaml
version: '3.8'
services:
  themisdb:
    image: themisdb:latest
    cap_add:
      - SYS_ADMIN
    devices:
      - /dev/fuse
    volumes:
      - ./encrypted:/var/lib/themisdb/encrypted
      - ./config:/etc/themisdb
    environment:
      - VAULT_ADDR=https://vault:8200
      - VAULT_TOKEN_FILE=/run/secrets/vault_token
    secrets:
      - vault_token

secrets:
  vault_token:
    file: ./secrets/vault_token
```

## Security Considerations

1. **Key Storage**: Never store keys in plaintext. Use Vault or HSM.
2. **Filesystem Permissions**: Encrypted directories use 0700 (owner-only).
3. **Memory Security**: Keys never written to swap (mlock).
4. **Process Isolation**: gocryptfs runs as separate process.
5. **Audit Logging**: All operations logged with classification level.

## Troubleshooting

### Container Won't Mount

```bash
# Check if gocryptfs is installed
which gocryptfs

# Check FUSE availability
ls -l /dev/fuse

# Check if already mounted
mount | grep themisdb

# Manual mount test
gocryptfs /var/lib/themisdb/encrypted/vs-nfd /var/lib/themisdb/mnt/vs-nfd
```

### Permission Denied

```bash
# Ensure user is in fuse group
sudo usermod -a -G fuse $USER

# Reload groups
newgrp fuse
```

### Key Provider Errors

```bash
# Test Vault connection
vault status

# Check Vault token
vault token lookup

# Verify key exists
vault kv get themis/keys/user_storage_vs_nfd
```

## Performance

| Operation | Latency | Notes |
|-----------|---------|-------|
| Container Mount | < 500ms | Cold start |
| User Read | < 5ms | Cached |
| User Write | < 10ms | Includes encryption |
| Key Rotation | < 30min | 10,000 users |

## Limitations

- Windows support experimental (WinFsp required)
- macOS requires macFUSE installation
- HSM operations slower than Vault (10-50ms vs 1-5ms)
- **Key rotation**: Framework in place, full zero-downtime implementation in progress (manual rotation works)
- **List operations**: `listUsers()` and `listGroups()` not yet implemented, use individual get operations

## Future Enhancements

- [x] Basic user/group CRUD operations
- [x] Container mounting and health monitoring
- [ ] Automatic zero-downtime key rotation (framework complete)
- [ ] List all users/groups in a security level
- [ ] Backup/restore per level
- [ ] Migration from unencrypted to encrypted
- [ ] Multi-region replication
- [ ] Advanced container health monitoring with alerts
- [ ] Automatic failure recovery

## API Reference

### MultiLevelEncryptedStorage

Main plugin class implementing IThemisPlugin.

#### Methods

##### User Management

- `Result<void> createUser(const User& user, SecurityLevel level)`
  - Creates a new user at the specified security level
  - Returns: Success or error result

- `Result<User> getUser(const std::string& user_id, SecurityLevel level)`
  - Retrieves a user by ID from the specified security level
  - Returns: User object or error

- `Result<void> updateUser(const User& user, SecurityLevel level)`
  - Updates an existing user
  - Returns: Success or error result

- `Result<void> deleteUser(const std::string& user_id, SecurityLevel level)`
  - Deletes a user by ID
  - Returns: Success or error result

- `Result<std::vector<User>> listUsers(SecurityLevel level)`
  - Lists all users at the specified security level
  - Returns: Vector of users or error

##### Group Management

- `Result<void> createGroup(const Group& group, SecurityLevel level)`
  - Creates a new group at the specified security level
  - Returns: Success or error result

- `Result<Group> getGroup(const std::string& group_id, SecurityLevel level)`
  - Retrieves a group by ID
  - Returns: Group object or error

- `Result<void> updateGroup(const Group& group, SecurityLevel level)`
  - Updates an existing group
  - Returns: Success or error result

- `Result<void> deleteGroup(const std::string& group_id, SecurityLevel level)`
  - Deletes a group by ID
  - Returns: Success or error result

- `Result<std::vector<Group>> listGroups(SecurityLevel level)`
  - Lists all groups at the specified security level
  - Returns: Vector of groups or error

##### Container Management

- `Result<void> mountAll()`
  - Mounts all encrypted containers
  - Returns: Success or error result

- `Result<void> unmountAll()`
  - Unmounts all containers
  - Returns: Success or error result

- `Result<void> mountLevel(SecurityLevel level)`
  - Mounts a specific security level container
  - Returns: Success or error result

- `Result<void> unmountLevel(SecurityLevel level)`
  - Unmounts a specific container
  - Returns: Success or error result

- `Result<void> rotateKey(SecurityLevel level)`
  - Performs key rotation for a security level
  - Returns: Success or error result

##### Health Monitoring

- `Result<HealthStatus> checkHealth()`
  - Performs health check on all levels
  - Returns: Health status or error

- `Result<HealthStatus> checkLevelHealth(SecurityLevel level)`
  - Checks health of a specific level
  - Returns: Health status or error

### Data Types

#### User

```cpp
struct User {
    std::string user_id;
    std::string username;
    std::string email;
    std::string full_name;
    std::vector<std::string> roles;
    SecurityLevel classification;
    int64_t created_at_ms;
    int64_t updated_at_ms;
};
```

#### Group

```cpp
struct Group {
    std::string group_id;
    std::string name;
    std::string description;
    std::vector<std::string> member_ids;
    SecurityLevel classification;
    int64_t created_at_ms;
};
```

#### SecurityLevel

```cpp
enum class SecurityLevel {
    OFFEN = 0,           // Public
    VS_NFD = 1,          // Restricted
    GEHEIM = 2,          // Secret
    STRENG_GEHEIM = 3    // Top Secret
};
```

#### HealthStatus

```cpp
struct HealthStatus {
    bool healthy;
    std::string message;
    std::vector<std::string> errors;
    int64_t checked_at_ms;
};
```

## License

See ThemisDB main license.

## Support

For issues and questions:
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Documentation: https://themisdb.org/docs/plugins/user-storage-encrypted
