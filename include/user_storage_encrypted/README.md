<!-- Status: current | validated: 2026-04-06 -->

# User Storage Encrypted

Encrypted, tiered user-storage subsystem for ThemisDB. This module provides HOT/WARM/COLD storage tiers backed by filesystem-level encryption (gocryptfs), automatic key rotation, and configurable security levels.

## Headers

| Header | Purpose |
|---|---|
| `encryption_backend_interface.hpp` | Abstract encryption backend interface (`IEncryptionBackend`) |
| `gocryptfs_backend.hpp` | Gocryptfs-backed concrete backend |
| `key_rotation_scheduler.hpp` | Automatic key rotation with configurable intervals |
| `multi_level_storage.hpp` | HOT/WARM/COLD tiered encrypted storage |
| `security_level.hpp` | `SecurityLevel` enum: STANDARD, HIGH, MAXIMUM |
| `user_models.hpp` | User storage metadata types |

## Quick Example

```cpp
#include "user_storage_encrypted/security_level.hpp"
#include "user_storage_encrypted/gocryptfs_backend.hpp"
#include "user_storage_encrypted/multi_level_storage.hpp"

auto backend = std::make_shared<themis::GocryptfsBackend>();
themis::MultiLevelEncryptedStorage storage(backend, themis::SecurityLevel::HIGH);
storage.store("user-123", data, themis::StorageTier::HOT);
```

## Version
v0.0.1 — first release 2026-03-22

## Implementation
See `../../src/user_storage_encrypted/` for implementation details and `ROADMAP.md` for planned features.

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

## Usage

Include the relevant headers from this module:

```cpp
#include "user_storage_encrypted/module_header.h"
```

See [`ARCHITECTURE.md`](ARCHITECTURE.md) and [`ROADMAP.md`](ROADMAP.md) for details.
