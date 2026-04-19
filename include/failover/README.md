> **Build:** `cmake --preset release && cmake --build build/release`

# include failover module

Public header surface for failover orchestration and disaster recovery.

## Headers
- `auto_failover_manager.h`
- `disaster_recovery_manager.h`

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

```cpp
#include "failover/auto_failover_manager.h"
#include "failover/disaster_recovery_manager.h"
```

