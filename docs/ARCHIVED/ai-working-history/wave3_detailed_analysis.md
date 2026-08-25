# Wave 3 Detailed Analysis - Complex Control Flow Move Semantics

## Analysis Summary

Based on examination of identified move patterns, classifying each gap:

### Gap 1: src/sharding/cross_shard_transaction.cpp:3472
**Code Context (lines 3468-3480):**
```cpp
continue;
}

// Move all deferred PreCommits to local copy
retries = std::move(deferred_precommits_);
}

// Process each transaction with failed PreCommits
for (const auto& [txn_id, failed_shards] : retries) {
```

**Analysis:**
- Variable `deferred_precommits_` is moved into `retries` at line 3472
- The move happens inside a loop control condition (line 3468: `if` check)
- After move, we only use `retries`, not the moved-from `deferred_precommits_`
- `deferred_precommits_` is a member variable, and the pattern repeats (loop iterates)

**Classification:** ✅ **FALSE POSITIVE** / SAFE
**Reason:** After the move, only `retries` is used. The member `deferred_precommits_` is not accessed after the move. The pattern is safe because `deferred_precommits_` is implicitly reset on next loop iteration.

---

### Gap 2: src/storage/wom_tree.cpp:408-410
**Code Context:**
```cpp
auto new_root        = std::make_unique<Node>(false);
new_root->pivot_keys = {pivot};
new_root->children.push_back(std::move(root));      // line 408
new_root->children.push_back(std::move(right_leaf));
root   = std::move(new_root);                         // line 410
height = 2;
```

**Analysis:**
- Line 408: `root` (a unique_ptr) is moved into `new_root->children`
- Line 410: `new_root` is moved into `root`
- This is valid - `root` is reassigned, not used in its moved-from state

**Classification:** ✅ **FALSE POSITIVE** / SAFE
**Reason:** `root` is not used between line 408 and 410; instead, it's reassigned at line 410.

---

### Gap 3: src/training/auto_labeler.cpp:291
**Code Context:**
```cpp
auto fallback_modalities = extractFallbackModalities(document_text);
if (modalities.empty()) {
    modalities = std::move(fallback_modalities);  // line 291
} else if (!fallback_modalities.empty()) {
    modalities.insert(modalities.end(),
                      fallback_modalities.begin(),
                      fallback_modalities.end());
```

**Analysis:**
- Line 291: `fallback_modalities` is moved into `modalities`
- Lines 293-295: In the `else if` branch, `fallback_modalities` is NOT used
- Only in the `if` branch does the move happen
- In `else if`, only iterators are used (which refer to the original vector)

**Classification:** ✅ **FALSE POSITIVE** / SAFE  
**Reason:** The if/else-if are mutually exclusive. If line 291 executes (move), the else-if is not reached. If else-if executes, line 291 was not executed (no move).

---

### Gap 4: src/gpu/launcher.cpp:131
**Code Context:**
```cpp
return std::async(std::launch::async,
    [this, items = std::move(items)]() mutable {
        std::vector<WorkResult> results;
        results.reserve(items.size());
        for (auto& item : items) {
            results.push_back(executeOne(std::move(item)));
        }
        return results;
    });
```

**Analysis:**
- Line 131: `items` parameter is captured by move into the lambda
- Inside lambda: `items` is used via `items.size()` and iteration
- This is **idiomatic C++ move capture** - the moved-from variable `items` (the parameter) is never used after the move; it exists only in the lambda's capture

**Classification:** ✅ **FALSE POSITIVE** / SAFE & IDIOMATIC
**Reason:** Move capture in lambdas is standard C++ idiom. The parameter `items` is moved into the lambda's capture, and the moved-from state is never accessed.

---

## Summary

Analyzed 4 identified Wave 3 gaps:
- ✅ Gap 1: FALSE POSITIVE (safe loop pattern)
- ✅ Gap 2: FALSE POSITIVE (reassignment after move)
- ✅ Gap 3: FALSE POSITIVE (mutual exclusion in if/else)
- ✅ Gap 4: FALSE POSITIVE & IDIOMATIC (lambda capture-by-move)

**Key Finding:** The initial gaps identified appear to be false positives due to:
1. Control flow mutual exclusion (if/else)
2. Reassignment after move
3. Lambda capture idiom
4. Loop iteration with member variables

**Next Steps:** Need deeper search for ACTUAL moved-from usage bugs in complex control flow.
