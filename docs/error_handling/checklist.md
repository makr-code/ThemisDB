# Error Handling Checklist

**Status:** Authoritative – rules are enforced by `tools/error_handling_audit.py`  
**References:** `phase4_migration_matrix.md`, `phase4_final_summary_handoff.md`, `phase4_complete_inventory.md`

---

## Rules for C++ Files (`.cpp`, `.cc`, `.cxx`, `.h`, `.hpp`)

### RULE-CPP-001 – No bare `return nullptr` in non-trivial functions

**Violation:** Returning `nullptr` from a function whose signature implies error propagation (factory functions, parsers, loaders, getters that can fail).

**Required pattern:**

```cpp
// ✅ Correct
Result<T*> myFactory() {
    if (!ready) return Err<T*>(ERR_INDEX_NOT_INITIALIZED, "not ready");
    return Ok(ptr);
}

// ❌ Wrong
T* myFactory() {
    if (!ready) return nullptr;
    return ptr;
}
```

**Note:** `return nullptr` is _allowed_ in constructors, test helpers, and deliberately nullable accessors whose name ends in `OrNull`, `OrNullptr`, or `MaybeNull`.

---

### RULE-CPP-002 – No catch-all handlers without logging

**Violation:** `catch (...)` without at least one logging call inside the catch body.

**Required pattern:**

```cpp
// ✅ Correct
catch (...) {
    THEMIS_ERROR("unexpected exception in {}::{}", __FILE__, __func__);
    throw;
}

// ❌ Wrong
catch (...) {
    return false;
}
```

---

### RULE-CPP-003 – No local/ad-hoc `struct Status` definitions

**Violation:** Defining a new `struct Status { bool ok; … }` (or similar) locally instead of using `Result<T>` from `include/utils/expected.h`.

**Required pattern:**

```cpp
// ✅ Correct
Result<void> doWork() { … }

// ❌ Wrong
struct Status { bool success; std::string msg; };
Status doWork() { … }
```

**Note:** `struct Status` definitions that are _inside_ `namespace rocksdb` or `namespace leveldb` (third-party) are exempt.

---

### RULE-CPP-004 – `Result<T>` / `tl::expected` must be used for fallible operations

All fallible C++ functions must return `Result<T>` (alias for `tl::expected<T, themis::Error>` in `include/utils/expected.h`). Functions that previously returned `std::optional<T>` _solely to signal failure_ must be migrated.

**Error codes** must come from the `themis::errors::ErrorCode` enum defined in `include/utils/error_registry.h`.

---

## Rules for Python Files (`.py`)

### RULE-PY-001 – No bare `except:` or `except Exception:` without re-raise or logging

**Violation:** Silently swallowing exceptions.

```python
# ❌ Wrong
try:
    do_work()
except:
    pass

# ✅ Correct
try:
    do_work()
except Exception as e:
    logger.error("do_work failed: %s", e)
    raise
```

---

### RULE-PY-002 – No `return None` used as sentinel for errors in public API functions

Functions documented as "returns X on success" should raise an exception rather than return `None` on error.

---

## Rules for C# Files (`.cs`)

### RULE-CS-001 – No empty `catch` blocks

```csharp
// ❌ Wrong
catch (Exception) { }

// ✅ Correct
catch (Exception ex) { logger.LogError(ex, "…"); throw; }
```

---

### RULE-CS-002 – No `catch (Exception)` swallowing without logging

Same as RULE-CS-001 but broader: any `catch` that doesn't contain a logging call or `throw` is a violation.

---

## Rules for PHP Files (`.php`)

### RULE-PHP-001 – No empty `catch` blocks

```php
// ❌ Wrong
catch (\Exception $e) {}

// ✅ Correct
catch (\Exception $e) { error_log($e->getMessage()); throw $e; }
```

---

## Rules for PowerShell Files (`.ps1`)

### RULE-PS1-001 – No empty `catch` blocks

```powershell
# ❌ Wrong
catch {}

# ✅ Correct
catch { Write-Error $_; throw }
```

---

## Exemptions

The following paths are excluded from all rules (see also `tools/error_handling_audit.ignore`):

- `llama.cpp/` – third-party submodule
- `vcpkg/` – package manager
- `ports/`, `ports-overlays/` – vcpkg port overlays
- `tests/` – test helpers may use nullable patterns
- Any path listed in `tools/error_handling_audit.ignore`
