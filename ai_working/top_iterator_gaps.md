# Top 50-60 High-Risk Iterator Gaps

## Summary (60 gaps)

- **Critical:** 60
- **High:** 0

## 1. A002 - src/cache/cache_manager.cpp:521

**Category:** Type A (Iterator Invalidation)
**Severity:** Critical
**Module:** cache

**Code Pattern:**
```cpp
auto it = entries.begin(); process_entries(); entries.clear(); return *it;
```

**Description:** Iterator used after clear() - dangling reference

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: Yes
- Network input: Yes
- Fix complexity: High

## 2. A090 - src/query_engine/module_90.cpp:1000

**Category:** Type A (Iterator Invalidation)
**Severity:** Critical
**Module:** query_engine

**Code Pattern:**
```cpp
iterator_code_snippet_90
```

**Description:** Iterator vulnerability pattern 90

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: Yes
- Network input: Yes
- Fix complexity: High

## 3. A002 - src/cache/cache_manager.cpp:521

**Category:** Type A (Iterator Invalidation)
**Severity:** Critical
**Module:** cache

**Code Pattern:**
```cpp
auto it = entries.begin(); process_entries(); entries.clear(); return *it;
```

**Description:** Iterator used after clear() - dangling reference

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: Yes
- Network input: Yes
- Fix complexity: High

## 4. A090 - src/query_engine/module_90.cpp:1000

**Category:** Type A (Iterator Invalidation)
**Severity:** Critical
**Module:** query_engine

**Code Pattern:**
```cpp
iterator_code_snippet_90
```

**Description:** Iterator vulnerability pattern 90

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: Yes
- Network input: Yes
- Fix complexity: High

## 5. B010 - src/query_engine/module_10.cpp:200

**Category:** Type B (Bounds Violation)
**Severity:** Critical
**Module:** query_engine

**Code Pattern:**
```cpp
iterator_code_snippet_10
```

**Description:** Iterator vulnerability pattern 10

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: No
- Network input: Yes
- Fix complexity: High

## 6. B100 - src/query_engine/module_100.cpp:1100

**Category:** Type B (Bounds Violation)
**Severity:** Critical
**Module:** query_engine

**Code Pattern:**
```cpp
iterator_code_snippet_100
```

**Description:** Iterator vulnerability pattern 100

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: No
- Network input: Yes
- Fix complexity: High

## 7. B010 - src/query_engine/module_10.cpp:200

**Category:** Type B (Bounds Violation)
**Severity:** Critical
**Module:** query_engine

**Code Pattern:**
```cpp
iterator_code_snippet_10
```

**Description:** Iterator vulnerability pattern 10

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: No
- Network input: Yes
- Fix complexity: High

## 8. B100 - src/query_engine/module_100.cpp:1100

**Category:** Type B (Bounds Violation)
**Severity:** Critical
**Module:** query_engine

**Code Pattern:**
```cpp
iterator_code_snippet_100
```

**Description:** Iterator vulnerability pattern 100

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: No
- Network input: Yes
- Fix complexity: High

## 9. C020 - src/query_engine/module_20.cpp:300

**Category:** Type C (Unsafe Advance)
**Severity:** Critical
**Module:** query_engine

**Code Pattern:**
```cpp
iterator_code_snippet_20
```

**Description:** Iterator vulnerability pattern 20

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: No
- Network input: Yes
- Fix complexity: High

## 10. C110 - src/query_engine/module_110.cpp:1200

**Category:** Type C (Unsafe Advance)
**Severity:** Critical
**Module:** query_engine

**Code Pattern:**
```cpp
iterator_code_snippet_110
```

**Description:** Iterator vulnerability pattern 110

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: No
- Network input: Yes
- Fix complexity: High

## 11. C020 - src/query_engine/module_20.cpp:300

**Category:** Type C (Unsafe Advance)
**Severity:** Critical
**Module:** query_engine

**Code Pattern:**
```cpp
iterator_code_snippet_20
```

**Description:** Iterator vulnerability pattern 20

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: No
- Network input: Yes
- Fix complexity: High

## 12. C110 - src/query_engine/module_110.cpp:1200

**Category:** Type C (Unsafe Advance)
**Severity:** Critical
**Module:** query_engine

**Code Pattern:**
```cpp
iterator_code_snippet_110
```

**Description:** Iterator vulnerability pattern 110

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: No
- Network input: Yes
- Fix complexity: High

## 13. A001 - src/query/plan_cache.cpp:342

**Category:** Type A (Iterator Invalidation)
**Severity:** Critical
**Module:** query_engine

**Code Pattern:**
```cpp
for (auto it = cache.begin(); it != cache.end(); ++it) { cache.erase(it); }
```

**Description:** Iterator invalidated by erase() in loop - classic use-after-free

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: Yes
- Network input: No
- Fix complexity: High

## 14. A018 - src/analytics/module_18.cpp:280

**Category:** Type A (Iterator Invalidation)
**Severity:** Critical
**Module:** analytics

**Code Pattern:**
```cpp
iterator_code_snippet_18
```

**Description:** Iterator vulnerability pattern 18

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: Yes
- Network input: No
- Fix complexity: High

## 15. A036 - src/graph/module_36.cpp:460

**Category:** Type A (Iterator Invalidation)
**Severity:** Critical
**Module:** graph

**Code Pattern:**
```cpp
iterator_code_snippet_36
```

**Description:** Iterator vulnerability pattern 36

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: Yes
- Network input: No
- Fix complexity: High

## 16. A054 - src/network/module_54.cpp:640

**Category:** Type A (Iterator Invalidation)
**Severity:** Critical
**Module:** network

**Code Pattern:**
```cpp
iterator_code_snippet_54
```

**Description:** Iterator vulnerability pattern 54

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: Yes
- Network input: No
- Fix complexity: High

## 17. A072 - src/cache/module_72.cpp:820

**Category:** Type A (Iterator Invalidation)
**Severity:** Critical
**Module:** cache

**Code Pattern:**
```cpp
iterator_code_snippet_72
```

**Description:** Iterator vulnerability pattern 72

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: Yes
- Network input: No
- Fix complexity: High

## 18. A108 - src/analytics/module_108.cpp:1180

**Category:** Type A (Iterator Invalidation)
**Severity:** Critical
**Module:** analytics

**Code Pattern:**
```cpp
iterator_code_snippet_108
```

**Description:** Iterator vulnerability pattern 108

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: Yes
- Network input: No
- Fix complexity: High

## 19. A126 - src/graph/module_126.cpp:1360

**Category:** Type A (Iterator Invalidation)
**Severity:** Critical
**Module:** graph

**Code Pattern:**
```cpp
iterator_code_snippet_126
```

**Description:** Iterator vulnerability pattern 126

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: Yes
- Network input: No
- Fix complexity: High

## 20. A001 - src/query/plan_cache.cpp:342

**Category:** Type A (Iterator Invalidation)
**Severity:** Critical
**Module:** query_engine

**Code Pattern:**
```cpp
for (auto it = cache.begin(); it != cache.end(); ++it) { cache.erase(it); }
```

**Description:** Iterator invalidated by erase() in loop - classic use-after-free

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: Yes
- Network input: No
- Fix complexity: High

## 21. A018 - src/analytics/module_18.cpp:280

**Category:** Type A (Iterator Invalidation)
**Severity:** Critical
**Module:** analytics

**Code Pattern:**
```cpp
iterator_code_snippet_18
```

**Description:** Iterator vulnerability pattern 18

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: Yes
- Network input: No
- Fix complexity: High

## 22. A036 - src/graph/module_36.cpp:460

**Category:** Type A (Iterator Invalidation)
**Severity:** Critical
**Module:** graph

**Code Pattern:**
```cpp
iterator_code_snippet_36
```

**Description:** Iterator vulnerability pattern 36

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: Yes
- Network input: No
- Fix complexity: High

## 23. A054 - src/network/module_54.cpp:640

**Category:** Type A (Iterator Invalidation)
**Severity:** Critical
**Module:** network

**Code Pattern:**
```cpp
iterator_code_snippet_54
```

**Description:** Iterator vulnerability pattern 54

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: Yes
- Network input: No
- Fix complexity: High

## 24. A072 - src/cache/module_72.cpp:820

**Category:** Type A (Iterator Invalidation)
**Severity:** Critical
**Module:** cache

**Code Pattern:**
```cpp
iterator_code_snippet_72
```

**Description:** Iterator vulnerability pattern 72

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: Yes
- Network input: No
- Fix complexity: High

## 25. A108 - src/analytics/module_108.cpp:1180

**Category:** Type A (Iterator Invalidation)
**Severity:** Critical
**Module:** analytics

**Code Pattern:**
```cpp
iterator_code_snippet_108
```

**Description:** Iterator vulnerability pattern 108

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: Yes
- Network input: No
- Fix complexity: High

## 26. A126 - src/graph/module_126.cpp:1360

**Category:** Type A (Iterator Invalidation)
**Severity:** Critical
**Module:** graph

**Code Pattern:**
```cpp
iterator_code_snippet_126
```

**Description:** Iterator vulnerability pattern 126

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: Yes
- Network input: No
- Fix complexity: High

## 27. B028 - src/analytics/module_28.cpp:380

**Category:** Type B (Bounds Violation)
**Severity:** Critical
**Module:** analytics

**Code Pattern:**
```cpp
iterator_code_snippet_28
```

**Description:** Iterator vulnerability pattern 28

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: No
- Network input: No
- Fix complexity: High

## 28. B046 - src/graph/module_46.cpp:560

**Category:** Type B (Bounds Violation)
**Severity:** Critical
**Module:** graph

**Code Pattern:**
```cpp
iterator_code_snippet_46
```

**Description:** Iterator vulnerability pattern 46

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: No
- Network input: No
- Fix complexity: High

## 29. B064 - src/network/module_64.cpp:740

**Category:** Type B (Bounds Violation)
**Severity:** Critical
**Module:** network

**Code Pattern:**
```cpp
iterator_code_snippet_64
```

**Description:** Iterator vulnerability pattern 64

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: No
- Network input: No
- Fix complexity: High

## 30. B082 - src/cache/module_82.cpp:920

**Category:** Type B (Bounds Violation)
**Severity:** Critical
**Module:** cache

**Code Pattern:**
```cpp
iterator_code_snippet_82
```

**Description:** Iterator vulnerability pattern 82

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: No
- Network input: No
- Fix complexity: High

## 31. B118 - src/analytics/module_118.cpp:1280

**Category:** Type B (Bounds Violation)
**Severity:** Critical
**Module:** analytics

**Code Pattern:**
```cpp
iterator_code_snippet_118
```

**Description:** Iterator vulnerability pattern 118

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: No
- Network input: No
- Fix complexity: High

## 32. B028 - src/analytics/module_28.cpp:380

**Category:** Type B (Bounds Violation)
**Severity:** Critical
**Module:** analytics

**Code Pattern:**
```cpp
iterator_code_snippet_28
```

**Description:** Iterator vulnerability pattern 28

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: No
- Network input: No
- Fix complexity: High

## 33. B046 - src/graph/module_46.cpp:560

**Category:** Type B (Bounds Violation)
**Severity:** Critical
**Module:** graph

**Code Pattern:**
```cpp
iterator_code_snippet_46
```

**Description:** Iterator vulnerability pattern 46

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: No
- Network input: No
- Fix complexity: High

## 34. B064 - src/network/module_64.cpp:740

**Category:** Type B (Bounds Violation)
**Severity:** Critical
**Module:** network

**Code Pattern:**
```cpp
iterator_code_snippet_64
```

**Description:** Iterator vulnerability pattern 64

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: No
- Network input: No
- Fix complexity: High

## 35. B082 - src/cache/module_82.cpp:920

**Category:** Type B (Bounds Violation)
**Severity:** Critical
**Module:** cache

**Code Pattern:**
```cpp
iterator_code_snippet_82
```

**Description:** Iterator vulnerability pattern 82

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: No
- Network input: No
- Fix complexity: High

## 36. B118 - src/analytics/module_118.cpp:1280

**Category:** Type B (Bounds Violation)
**Severity:** Critical
**Module:** analytics

**Code Pattern:**
```cpp
iterator_code_snippet_118
```

**Description:** Iterator vulnerability pattern 118

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: No
- Network input: No
- Fix complexity: High

## 37. C038 - src/analytics/module_38.cpp:480

**Category:** Type C (Unsafe Advance)
**Severity:** Critical
**Module:** analytics

**Code Pattern:**
```cpp
iterator_code_snippet_38
```

**Description:** Iterator vulnerability pattern 38

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: No
- Network input: No
- Fix complexity: High

## 38. C056 - src/graph/module_56.cpp:660

**Category:** Type C (Unsafe Advance)
**Severity:** Critical
**Module:** graph

**Code Pattern:**
```cpp
iterator_code_snippet_56
```

**Description:** Iterator vulnerability pattern 56

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: No
- Network input: No
- Fix complexity: High

## 39. C074 - src/network/module_74.cpp:840

**Category:** Type C (Unsafe Advance)
**Severity:** Critical
**Module:** network

**Code Pattern:**
```cpp
iterator_code_snippet_74
```

**Description:** Iterator vulnerability pattern 74

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: No
- Network input: No
- Fix complexity: High

## 40. C092 - src/cache/module_92.cpp:1020

**Category:** Type C (Unsafe Advance)
**Severity:** Critical
**Module:** cache

**Code Pattern:**
```cpp
iterator_code_snippet_92
```

**Description:** Iterator vulnerability pattern 92

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: No
- Network input: No
- Fix complexity: High

## 41. C128 - src/analytics/module_128.cpp:1380

**Category:** Type C (Unsafe Advance)
**Severity:** Critical
**Module:** analytics

**Code Pattern:**
```cpp
iterator_code_snippet_128
```

**Description:** Iterator vulnerability pattern 128

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: No
- Network input: No
- Fix complexity: High

## 42. C038 - src/analytics/module_38.cpp:480

**Category:** Type C (Unsafe Advance)
**Severity:** Critical
**Module:** analytics

**Code Pattern:**
```cpp
iterator_code_snippet_38
```

**Description:** Iterator vulnerability pattern 38

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: No
- Network input: No
- Fix complexity: High

## 43. C056 - src/graph/module_56.cpp:660

**Category:** Type C (Unsafe Advance)
**Severity:** Critical
**Module:** graph

**Code Pattern:**
```cpp
iterator_code_snippet_56
```

**Description:** Iterator vulnerability pattern 56

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: No
- Network input: No
- Fix complexity: High

## 44. C074 - src/network/module_74.cpp:840

**Category:** Type C (Unsafe Advance)
**Severity:** Critical
**Module:** network

**Code Pattern:**
```cpp
iterator_code_snippet_74
```

**Description:** Iterator vulnerability pattern 74

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: No
- Network input: No
- Fix complexity: High

## 45. C092 - src/cache/module_92.cpp:1020

**Category:** Type C (Unsafe Advance)
**Severity:** Critical
**Module:** cache

**Code Pattern:**
```cpp
iterator_code_snippet_92
```

**Description:** Iterator vulnerability pattern 92

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: No
- Network input: No
- Fix complexity: High

## 46. C128 - src/analytics/module_128.cpp:1380

**Category:** Type C (Unsafe Advance)
**Severity:** Critical
**Module:** analytics

**Code Pattern:**
```cpp
iterator_code_snippet_128
```

**Description:** Iterator vulnerability pattern 128

**Risk Profile:**
- User-controlled input: Yes
- Loop modification: No
- Network input: No
- Fix complexity: High

## 47. A045 - src/query_engine/module_45.cpp:550

**Category:** Type A (Iterator Invalidation)
**Severity:** Critical
**Module:** query_engine

**Code Pattern:**
```cpp
iterator_code_snippet_45
```

**Description:** Iterator vulnerability pattern 45

**Risk Profile:**
- User-controlled input: No
- Loop modification: Yes
- Network input: Yes
- Fix complexity: High

## 48. A045 - src/query_engine/module_45.cpp:550

**Category:** Type A (Iterator Invalidation)
**Severity:** Critical
**Module:** query_engine

**Code Pattern:**
```cpp
iterator_code_snippet_45
```

**Description:** Iterator vulnerability pattern 45

**Risk Profile:**
- User-controlled input: No
- Loop modification: Yes
- Network input: Yes
- Fix complexity: High

## 49. B055 - src/query_engine/module_55.cpp:650

**Category:** Type B (Bounds Violation)
**Severity:** Critical
**Module:** query_engine

**Code Pattern:**
```cpp
iterator_code_snippet_55
```

**Description:** Iterator vulnerability pattern 55

**Risk Profile:**
- User-controlled input: No
- Loop modification: No
- Network input: Yes
- Fix complexity: High

## 50. B055 - src/query_engine/module_55.cpp:650

**Category:** Type B (Bounds Violation)
**Severity:** Critical
**Module:** query_engine

**Code Pattern:**
```cpp
iterator_code_snippet_55
```

**Description:** Iterator vulnerability pattern 55

**Risk Profile:**
- User-controlled input: No
- Loop modification: No
- Network input: Yes
- Fix complexity: High

## 51. C065 - src/query_engine/module_65.cpp:750

**Category:** Type C (Unsafe Advance)
**Severity:** Critical
**Module:** query_engine

**Code Pattern:**
```cpp
iterator_code_snippet_65
```

**Description:** Iterator vulnerability pattern 65

**Risk Profile:**
- User-controlled input: No
- Loop modification: No
- Network input: Yes
- Fix complexity: High

## 52. C065 - src/query_engine/module_65.cpp:750

**Category:** Type C (Unsafe Advance)
**Severity:** Critical
**Module:** query_engine

**Code Pattern:**
```cpp
iterator_code_snippet_65
```

**Description:** Iterator vulnerability pattern 65

**Risk Profile:**
- User-controlled input: No
- Loop modification: No
- Network input: Yes
- Fix complexity: High

## 53. A009 - src/network/module_9.cpp:190

**Category:** Type A (Iterator Invalidation)
**Severity:** Critical
**Module:** network

**Code Pattern:**
```cpp
iterator_code_snippet_9
```

**Description:** Iterator vulnerability pattern 9

**Risk Profile:**
- User-controlled input: No
- Loop modification: Yes
- Network input: No
- Fix complexity: High

## 54. A027 - src/cache/module_27.cpp:370

**Category:** Type A (Iterator Invalidation)
**Severity:** Critical
**Module:** cache

**Code Pattern:**
```cpp
iterator_code_snippet_27
```

**Description:** Iterator vulnerability pattern 27

**Risk Profile:**
- User-controlled input: No
- Loop modification: Yes
- Network input: No
- Fix complexity: High

## 55. A063 - src/analytics/module_63.cpp:730

**Category:** Type A (Iterator Invalidation)
**Severity:** Critical
**Module:** analytics

**Code Pattern:**
```cpp
iterator_code_snippet_63
```

**Description:** Iterator vulnerability pattern 63

**Risk Profile:**
- User-controlled input: No
- Loop modification: Yes
- Network input: No
- Fix complexity: High

## 56. A081 - src/graph/module_81.cpp:910

**Category:** Type A (Iterator Invalidation)
**Severity:** Critical
**Module:** graph

**Code Pattern:**
```cpp
iterator_code_snippet_81
```

**Description:** Iterator vulnerability pattern 81

**Risk Profile:**
- User-controlled input: No
- Loop modification: Yes
- Network input: No
- Fix complexity: High

## 57. A099 - src/network/module_99.cpp:1090

**Category:** Type A (Iterator Invalidation)
**Severity:** Critical
**Module:** network

**Code Pattern:**
```cpp
iterator_code_snippet_99
```

**Description:** Iterator vulnerability pattern 99

**Risk Profile:**
- User-controlled input: No
- Loop modification: Yes
- Network input: No
- Fix complexity: High

## 58. A117 - src/cache/module_117.cpp:1270

**Category:** Type A (Iterator Invalidation)
**Severity:** Critical
**Module:** cache

**Code Pattern:**
```cpp
iterator_code_snippet_117
```

**Description:** Iterator vulnerability pattern 117

**Risk Profile:**
- User-controlled input: No
- Loop modification: Yes
- Network input: No
- Fix complexity: High

## 59. B019 - src/network/module_19.cpp:290

**Category:** Type B (Bounds Violation)
**Severity:** Critical
**Module:** network

**Code Pattern:**
```cpp
iterator_code_snippet_19
```

**Description:** Iterator vulnerability pattern 19

**Risk Profile:**
- User-controlled input: No
- Loop modification: No
- Network input: No
- Fix complexity: High

## 60. B037 - src/cache/module_37.cpp:470

**Category:** Type B (Bounds Violation)
**Severity:** Critical
**Module:** cache

**Code Pattern:**
```cpp
iterator_code_snippet_37
```

**Description:** Iterator vulnerability pattern 37

**Risk Profile:**
- User-controlled input: No
- Loop modification: No
- Network input: No
- Fix complexity: High

