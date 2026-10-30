---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Chaos Module Developer Guide

## Intended use

Use this guide when adding a test or controlled integration scenario around the
process-local fault registry and scheduler. The module records labels and
timing; the test consumer must implement the behavior associated with a label.
It is not a production failure control plane.

## Basic lifecycle

```cpp
#include "chaos/chaos_framework.h"

#include <chrono>
#include <memory>

using namespace std::chrono_literals;
using namespace themis::chaos;

auto injector = std::make_shared<FaultInjector>("scenario-1");
FaultSpec fault{FaultType::NODE_FAILURE, "test-node", 100ms};

if (injector->injectFault(fault) && injector->isFaultActive("test-node")) {
    // Test code observes the simulated state and asserts its own behavior.
}
injector->recoverFault("test-node", FaultType::NODE_FAILURE);
```

The state is visible only to consumers querying this injector. A descriptor
does not stop a node or intercept I/O. See [API.md](API.md) for validation,
callback, and exception semantics.

## Scheduling a scenario

Construct a scheduler with a non-null shared injector, configure a tick and
wake strategy if required, queue entries, and start it when the harness is
ready. Once the scenario ends:

1. Call `stop()` and wait for it to return; it joins the worker.
2. Inspect `pendingCount()` and call `clearPending()` if queued work must not
   fire after a later restart.
3. Recover each active target/type and verify with `isFaultActive()` or
   `activeFaultCount()`.
4. Ensure callbacks are short, non-throwing, and do not re-enter the injector
   from recovery.

`stop()` does not clear the queue. Zero or negative fault duration remains
active until explicit recovery. A process restart discards in-memory state but
does not undo external effects introduced by test code.

## Validation and tests

The focused targets are registered by
[`tests/chaos/CMakeLists.txt`](../../tests/chaos/CMakeLists.txt). With an
existing configured build, run:

```sh
ctest --test-dir <build-dir> --output-on-failure \
  -R 'test_chaos_(framework|stress|concurrency_hardening|callback_determinism|scheduler_timing)_chaos_FocusedTests'
```

The tests cover input rejection, expiry, recovery, callbacks, pending work,
tick strategies, concurrent use, and restart timing. For new scenarios, assert
the consumer's behavior as well as injector state, then verify the scenario
returns to a clean baseline. See [IMPLEMENTATION.md](IMPLEMENTATION.md) for
source-level invariants and [RUNBOOK.md](RUNBOOK.md) for recovery steps.
