# C++ Gap Scanner — Best Practice Integration & GitHub Automation

**Date:** 2026-05-18  
**Scope:** Advanced scanner patterns + GitHub CLI automation  
**Status:** Strategic Guidelines (ready to implement)

---

## 📚 Established C++ Scanning Best Practices

### 1. Industry-Standard Static Analyzers

| Tool | Category | C++ Focus | ThemisDB Fit | Effort |
|------|----------|-----------|--------------|--------|
| **Clang-Tidy** | LLVM native | Modernization, correctness, readability | [IDEAL] | Low |
| **CppCheck** | Lightweight | Memory, logic, performance bugs | [GOOD] | Low |
| **MISRA C++** | Safety critical | Memory safety, predictability | [HIGH] | Medium |
| **Coverity** | Commercial SaaS | Deep semantic analysis | [EXCELLENT] | High ($) |
| **SonarQube** | SaaS/OSS | Multi-language + quality gates | [GOOD] | Medium |
| **AddressSanitizer** | Runtime instrumentation | Memory errors (heap/stack) | [ESSENTIAL] | Low |
| **ThreadSanitizer** | Runtime instrumentation | Data race detection | [ESSENTIAL] | Low |
| **UBSan** | Runtime instrumentation | Undefined behavior | [ESSENTIAL] | Low |

### 2. LLVM/Clang-Tidy Built-in Checks

ThemisDB should enable these clang-tidy groups (best for C++17/20):

```yaml
Checks: >
  -*,                                          # Disable all by default
  clang-analyzer-*,                            # AST-based analysis
  modernize-*,                                 # C++11/14/17/20 upgrades
  readability-*,                               # Code clarity
  performance-*,                               # Runtime efficiency
  portability-*,                               # Platform issues
  bugprone-*,                                  # Common mistakes
  cppcoreguidelines-*,                         # C++ Core Guidelines
  google-*,                                    # Google C++ Style
  llvm-*                                       # LLVM conventions
CheckOptions:
  - key: modernize-use-auto.MinTypeNameLength
    value: '10'                                # auto for long types only
  - key: readability-magic-numbers.IgnoredIntegerValues
    value: '0,1,2,100'                         # Common harmless constants
  - key: cppcoreguidelines-pro-bounds-pointer-arithmetic.NatTypes
    value: 'size_t,ptrdiff_t'
```

### 3. Scanner Categories Not Yet in Phase 1

#### Category A: Concurrency Gaps
```cpp
// 🔴 CRITICAL — Data race
class Cache {
    std::unordered_map<K, V> data_;            // Shared state
    // Missing: std::mutex data_mutex_;
    
    V get(const K& key) {
        return data_[key];                     // RACE CONDITION
    }
    
    void set(const K& key, V val) {
        data_[key] = val;                      // RACE CONDITION
    }
};
```

**Detection Pattern:**
- Non-const member access to `std::map/unordered_map/vector` without mutex
- Multiple `void get/set` methods without synchronized access
- `std::thread` creation + shared state without `std::mutex` in scope

**Expected Gaps:** 40-60 (concentration in cache/index modules)
**Severity:** CRITICAL (data corruption)

---

#### Category B: Container/Algorithm Misuse
```cpp
// 🟠 HIGH — Using old-style iteration
std::vector<Item> items;
for (int i = 0; i < items.size(); ++i) {      // Size casts to unsigned
    process(items[i]);
}

// Better: std::ranges::for_each(items, process);
// Or: for (auto& item : items) process(item);
```

**Detection Pattern:**
- Loop variable `int i` compared to `.size()` → HIGH
- `std::find` not used when searching → MEDIUM
- Manual pointer arithmetic instead of `.at()` → HIGH

**Expected Gaps:** 50-80

---

#### Category C: RAII Violations
```cpp
// 🔴 CRITICAL — Resource leak
void process_file(const std::string& path) {
    FILE* f = fopen(path.c_str(), "r");
    if (!f) return;
    
    // Missing: Guard with std::unique_ptr<FILE, decltype(&fclose)>
    char buffer[1024];
    fread(buffer, 1, sizeof(buffer), f);
    // fclose(f);  ← MISSING OR CONDITIONAL
}
```

**Detection Pattern:**
- `fopen/malloc/new` followed by conditional early `return/throw` without cleanup
- No `try/catch` or RAII wrapper
- Pointer assigned but no corresponding `delete` in destructor or error path

**Expected Gaps:** 30-50

---

#### Category D: Const-Correctness
```cpp
// 🟠 MEDIUM — Non-const method on logically const data
class QueryOptimizer {
    mutable int cache_hits_;                   // OK for cache
    
    // BUG: Should be const
    int get_plan_cost() {                      // Missing: const
        return calculated_cost_;               // No modification
    }
};
```

**Detection Pattern:**
- Method doesn't assign to `this->*` but also not marked `const`
- Heuristic: method reads fields only → suggests missing `const`

**Expected Gaps:** 60-100

---

#### Category E: Exception Safety
```cpp
// 🔴 HIGH — Not exception-safe
void UserManager::create_user(const User& u) {
    users_.push_back(u);                       // May throw
    db_.insert(u.id, u);                       // If this throws, u in memory but not in DB
    // Missing: Transaction or rollback on failure
}
```

**Detection Pattern:**
- Multiple state-modifying operations without transaction/rollback
- Resource allocation not paired with RAII
- No `noexcept` on guarantee-providing functions

**Expected Gaps:** 40-70

---

#### Category F: Naming & Conventions
```cpp
// 🟡 MEDIUM — Inconsistent naming
class DBQueryParser {
    void parse_SQL();                          // snake_case (breaks ThemisDB convention)
    void parseQuery();                         // camelCase
    void Parse_Table();                        // Mixed
};

// ThemisDB convention: snake_case or PascalCase for classes
// See ARCHITECTURE.md → Naming Conventions
```

**Detection Pattern:**
- Class methods mix camelCase + snake_case in same class
- Member variables don't follow `variable_name_` (private) pattern
- Function names don't match module style guide

**Expected Gaps:** 80-150 (high volume, low severity)

---

#### Category G: Platform-Specific Issues
```cpp
// 🟠 HIGH — Windows-only, breaks on Linux
#ifdef _WIN32
    HANDLE h = CreateFileW(...);               // Windows API
    WaitForSingleObject(h, INFINITE);
#else
    // Missing: Linux equivalent
    // Should use POSIX: open(), select(), epoll()
#endif

// Or missing entirely:
int fd = open(path, O_RDONLY);                // POSIX but no fallback on Windows
```

**Detection Pattern:**
- `#ifdef _WIN32` or `#ifdef __linux__` with incomplete coverage
- POSIX calls without Windows fallback
- Windows APIs without POSIX fallback
- Platform-specific constants without abstraction

**Expected Gaps:** 30-50

---

#### Category H: Performance Anti-patterns
```cpp
// 🟠 HIGH — Inefficient memory usage
void build_index(const std::vector<Record>& records) {
    for (const auto& rec : records) {
        std::string key = std::to_string(rec.id);    // Allocation per iteration
        index_.insert({key, rec});                    // Copies whole Record
    }
}

// Better: Use string_view, move semantics
```

**Detection Pattern:**
- String allocation in hot loops (for/while)
- Large struct copies without move semantics
- Repeated allocation of small fixed objects
- `O(n²)` patterns: nested loop with vec find

**Expected Gaps:** 40-60

---

#### Category I: Documentation Gaps (Doxygen)
```cpp
// 🟡 MEDIUM — Missing API documentation
class TransactionManager {
    // Missing: @brief, @param, @return, @throws
    Status begin_transaction(const TxnOptions& opts) {
        // Implementation
    }
    
    void abort();  // No documentation
};
```

**Detection Pattern:**
- Public method without `/**` block above
- `@param` mismatch (function has 3 params, doc has 2)
- Missing `@throws` for throwing functions
- Missing `@brief` summary

**Expected Gaps:** 200-400 (documentation heavy)

---

## 🎯 Recommended Scanner Build Order (by ROI)

| Phase | Categories | Effort | Expected Gaps | Priority | Timeline |
|-------|-----------|--------|---------------|----------|----------|
| **1 (DONE)** | Security, Memory, Reliability | 3 files, 670 lines | 130-230 | [P0] CRITICAL | ✅ 1-2 weeks |
| **2 (NEXT)** | Concurrency, RAII, Exception Safety | 3 files, 500 lines | 110-180 | [P0] CRITICAL | 1-2 weeks |
| **3** | Container Misuse, Platform Issues | 2 files, 400 lines | 80-130 | [P1] HIGH | 1 week |
| **4** | Const-Correctness, Performance | 2 files, 350 lines | 100-150 | [P2] MEDIUM | 1 week |
| **5** | Naming/Conventions, Documentation | 2 files, 300 lines | 280-550 | [P3] LOW | 1-2 weeks |

**Total v3 Full Stack:** ~10 files, ~2,200 lines Python, ~800-1,600 new gaps detected

---

## 🤖 GitHub Issue Automation

### Strategy 1: Batch Create Issues from Clustered Gaps

**Current State:**
- [x] Gap data aggregated in JSON
- [x] Manual clustering (13 issues) in `clustered_issues/`
- [ ] Auto-create via `gh` CLI

**Implementation:**

```bash
# File: tools/github_issue_creator.py
# Purpose: Transform clustered_issues/*.md → GitHub issues via gh CLI

class GitHubIssueCreator:
    def __init__(self, repo: str = "makr-code/ThemisDB"):
        self.repo = repo
        self.gh_available = self._check_gh_cli()
    
    def create_from_cluster(self, cluster_file: str) -> Issue:
        """
        Read: ai_working/clustered_issues/MOD-001_acceleration_gaps.md
        Execute: gh issue create --repo makr-code/ThemisDB \
                    --title "MOD-001: Acceleration Module — 621 gaps" \
                    --body @MOD-001_acceleration_gaps.md \
                    --label "gap-scanner,acceleration,P1-high" \
                    --assignee @core-team
        Return: Issue object with URL
        """
```

**gh CLI Commands Needed:**

```bash
# 1. Create single issue (interactive)
gh issue create --repo makr-code/ThemisDB \
  --title "META-001: Unimplemented Code Paths (1,620 gaps)" \
  --body-file ai_working/clustered_issues/META-001.md \
  --label "gap-scanner,critical,needs-implementation" \
  --assignee @lead-engineer

# 2. Batch create (scripted)
for file in ai_working/clustered_issues/*.md; do
  title=$(head -1 "$file" | sed 's/^# //')
  gh issue create --repo makr-code/ThemisDB \
    --title "$title" \
    --body-file "$file" \
    --label "gap-scanner" \
    --milestone "Q3 2026"
done

# 3. Link issues to GitHub Project
gh project item-add <project_id> --issue <issue_id>

# 4. Assign labels dynamically
gh issue edit <issue_id> --add-label "security,critical"

# 5. Close issues (after gaps fixed)
gh issue close <issue_id> --comment "Fixed in PR #1234"
```

---

### Strategy 2: Metric-Based Issue Prioritization

```python
class IssuePrioritizer:
    """Auto-assign GitHub milestone/label based on gap severity"""
    
    PRIORITY_MAP = {
        'critical': {
            'label': 'P0-critical',
            'milestone': 'Current Sprint',
            'assignee': 'senior-engineer',
            'duedate': 'next-friday'
        },
        'high': {
            'label': 'P1-high',
            'milestone': 'Next Sprint',
            'assignee': 'team',
            'duedate': 'in-2-weeks'
        },
        'medium': {
            'label': 'P2-medium',
            'milestone': 'Backlog',
            'assignee': 'unassigned',
            'duedate': None
        }
    }
    
    def prioritize_issues(self, gap_data: Dict) -> List[IssueMetadata]:
        """
        Input: ai_working/gap_scan_v3_aggregate.json
        Output: [(issue_title, priority, assignee, milestone), ...]
        """
        issues = []
        
        for module, gaps in gap_data.items():
            critical_count = gaps.get('severity_critical', 0)
            high_count = gaps.get('severity_high', 0)
            
            if critical_count > 50:
                priority = 'critical'
            elif high_count > 20:
                priority = 'high'
            else:
                priority = 'medium'
            
            meta = self.PRIORITY_MAP[priority]
            issues.append(IssueMetadata(
                title=f"MOD-{module}: {critical_count} critical + {high_count} high gaps",
                priority=priority,
                label=meta['label'],
                milestone=meta['milestone'],
                assignee=meta['assignee']
            ))
        
        return issues
```

---

### Strategy 3: End-to-End Automation Script

```bash
#!/bin/bash
# File: tools/gap_scanner_to_github.sh
# Purpose: Single command → Scan → Cluster → Create Issues

set -euo pipefail

REPO="makr-code/ThemisDB"
BRANCH="develop"

echo "[1/5] Running Phase 1 gap scanner..."
python tools/gap_scanner_v3.py . ai_working

echo "[2/5] Aggregating results..."
python tools/gap_clusterer.py ai_working

echo "[3/5] Generating GitHub issue templates..."
python tools/github_issue_templates.py ai_working

echo "[4/5] Authenticating gh CLI..."
gh auth status || gh auth login

echo "[5/5] Creating GitHub issues..."
python tools/github_issue_creator.py \
  --repo "$REPO" \
  --cluster-dir ai_working/clustered_issues/ \
  --auto-assign \
  --verbose

echo "[OK] Gap analysis complete!"
echo "    View issues: gh issue list --repo $REPO --label gap-scanner"
```

**Single Command Execution:**

```bash
cd /path/to/ThemisDB
bash tools/gap_scanner_to_github.sh
```

---

## 📊 Recommended Next Actions

### Immediate (This Week)
1. **Fix v3 Scanner rglob() bug** (in progress)
2. **Run Phase 1 full scan locally** → generates 130-230 gaps
3. **Execute `gap_scanner_to_github.sh`** → auto-create 13 issues

### Short-term (Next 2 Weeks)
1. **Implement Phase 2 scanners** (Concurrency, RAII, Exception Safety)
   - 3 new files, 500 lines
   - Expected: +110-180 gaps
2. **Batch create issues** for Phase 2 results
3. **Auto-assign to modules** (e.g., acceleration, index, storage)
4. **Set up GitHub Projects board** for tracking

### Medium-term (Month 2)
1. **Phase 3 scanners** (Container, Platform)
2. **Integrate clang-tidy** results (requires CMake setup)
3. **Add MISRA C++ checks** (C++ safety profile)
4. **Set up CI/CD** to run scanners on every PR

---

## 🔌 Integration with Existing Tools

### Integration Point 1: CMake + clang-tidy
```cmake
# cmake/CodeQuality.cmake
if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy)
    if(CLANG_TIDY_EXE)
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
    endif()
endif()

# Run: cmake --preset windows-release -DENABLE_CLANG_TIDY=ON
```

### Integration Point 2: GitHub Actions
```yaml
# .github/workflows/gap-scanner.yml
name: Gap Scanner
on: [push, pull_request]

jobs:
  scan:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - uses: actions/setup-python@v4
        with:
          python-version: '3.13'
      - run: pip install -r requirements-dev.txt
      - run: bash tools/gap_scanner_to_github.sh
      - uses: actions/upload-artifact@v3
        with:
          name: gap-reports
          path: ai_working/gap_scan_v*.json
```

### Integration Point 3: CodeQL (GitHub Advanced Security)
```yaml
# .github/workflows/codeql-analysis.yml
- uses: github/codeql-action/init@v2
  with:
    languages: cpp
    queries: security-and-quality
```

---

## 📈 Success Metrics

After implementing full v3 + GitHub automation:

| Metric | Target | Current | Improvement |
|--------|--------|---------|--------------|
| **Total Gaps Found** | 800-1,600 | 7,500 (v2) | +10-20% accuracy |
| **Critical % of Total** | 30-40% | 34% | Refined categorization |
| **Issues Created** | 25-35 | 13 | +12-22 issues |
| **Avg Issue Size** | 50-100 gaps | ~577 | More granular |
| **Time to Create Issues** | <5 min | Manual (1 hour) | 12x faster |
| **False Positive Rate** | <5% | ~30% (v2) | Much cleaner |

---

## 🎯 Approval to Proceed?

**Option A: Phase 2 Scanners First** (Concurrency, RAII, Exception Safety)
- Effort: 1-2 weeks
- Impact: +110-180 gaps, focus on CRITICAL issues
- ROI: HIGH (fixes prevent crashes/data corruption)

**Option B: GitHub Automation First**
- Effort: 3-4 days
- Impact: Auto-create 13 issues from Phase 1 results
- ROI: MEDIUM (enables team tracking, unblocks implementation)

**Option C: Both in Parallel**
- Effort: 2-3 weeks
- Impact: Full Phase 1+2 complete, all issues auto-created
- ROI: HIGHEST

**Recommendation:** **Option C** — Parallel execution of Phase 2 + GitHub automation yields maximum value.
