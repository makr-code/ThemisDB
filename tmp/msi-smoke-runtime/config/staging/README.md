# Staging Environment Config Overlay

This directory contains staging-environment config file overrides.

## How It Works

`ConfigPathResolver` probes `config/staging/<relative-path>` **before** the
standard `config/<relative-path>` root when the active environment is set to
`STAGING`.

Resolution order for a request of `config/lora_training_config.yaml` in STAGING:

1. `config/staging/ai_ml/lora_training_config.yaml` ← overlay (checked first)
2. `config/ai_ml/lora_training_config.yaml`         ← canonical new path
3. `config/lora_training_config.yaml`                ← legacy fallback

If a file does not exist in this overlay directory the resolver falls through
to the next path in the chain without error.

## Activation

**Via environment variable (recommended for CI/CD):**

```bash
THEMIS_CONFIG_ENV=staging ./themisdb
```

**Programmatically:**

```cpp
#include "config/config_path_resolver.h"
using namespace themis::config;

ConfigPathResolver::setEnvironment(ConfigEnvironment::STAGING);
```

`setEnvironment()` clears the path-resolution LRU cache atomically to prevent
stale entries from a previous environment from leaking into the new one.

## Overlay Structure

Place override files in the same sub-directory structure used by the canonical
`config/` root:

```
config/staging/
├── ai_ml/
│   └── lora_training_config.yaml   # overrides config/ai_ml/lora_training_config.yaml
├── security/
│   └── tls_config.yaml             # overrides config/security/tls_config.yaml
└── core/
    └── server_config.yaml          # overrides config/core/server_config.yaml
```

## Guidelines

- Only commit files here that **must** differ from production settings for
  pre-production validation.
- Keep secrets out of version control; use environment variables or a secrets
  manager instead.
- The `PATH_MAPPING` table is unchanged; only the filesystem search order is
  affected by the overlay prefix.
- The production path set is never modified when overlay files are updated.
- Staging overrides should be as close to production values as possible;
  use `config/dev/` for more permissive development-only settings.
