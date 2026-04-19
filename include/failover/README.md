> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# include failover module

Public header surface for failover orchestration and disaster recovery.

## Headers
- `include/failover/auto_failover_manager.h`
- `include/failover/disaster_recovery_manager.h`

## Exposed API
- `AutoFailoverManager`
- `DisasterRecoveryManager`
- Config/result/state enums and structs

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

## Usage

Include the relevant headers from this module:

```cpp
#include "failover/module_header.h"
```

See [`ARCHITECTURE.md`](ARCHITECTURE.md) and [`ROADMAP.md`](ROADMAP.md) for details.
