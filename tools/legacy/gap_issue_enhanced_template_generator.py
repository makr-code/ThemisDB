#!/usr/bin/env python3
"""
Enhanced GitHub Issue Template Generator — AI-Agent Ready

Transforms:
  ai_working/gap_scan_v3_aggregate.json → Detailed Issue Templates
  With concrete tasks, success criteria, and acceptance tests

Features:
  - Detailed acceptance criteria per gap category
  - Concrete implementation tasks with success metrics
  - Clear scope boundaries (what's IN / OUT of scope)
  - AI-agent execution instructions
  - Testability requirements
  - Performance/correctness targets
"""

import json
from pathlib import Path
from typing import Dict, List, Tuple
from dataclasses import dataclass


@dataclass
class GapCategory:
    """Gap category with detailed implementation guidance"""
    name: str
    cwe: str
    description: str
    patterns: List[str]
    acceptance_criteria: List[str]
    test_requirements: List[str]
    scope_inclusions: List[str]
    scope_exclusions: List[str]
    ai_instructions: str


# Define detailed gap categories with AI-agent instructions
GAP_CATEGORIES = {
    'security': GapCategory(
        name='Security Vulnerabilities',
        cwe='CWE-78/89/79',
        description='Unsafe functions, hardcoded secrets, injection vulnerabilities',
        patterns=[
            'Unsafe string functions (strcpy, sprintf, gets)',
            'Hardcoded credentials/API keys/secrets',
            'SQL injection via unsanitized queries',
            'Command injection via system calls',
            'Unchecked user input validation'
        ],
        acceptance_criteria=[
            'All unsafe C functions replaced with safe alternatives (std::string, std::format)',
            'Secrets moved to environment variables or secure vault',
            'User input validated against whitelist patterns',
            'SQL queries use parameterized statements',
            'Command execution uses vector<string> argv, not shell strings'
        ],
        test_requirements=[
            'Unit tests for input validation (normal, boundary, malicious cases)',
            'Integration tests for secret handling (not leaked in logs)',
            'Fuzzing harness for injection vectors',
            'Security code review checklist completed'
        ],
        scope_inclusions=[
            'Direct function calls in user-facing APIs',
            'Input processing paths (HTTP, CLI, config files)',
            'Credential/API key storage',
            'Database query construction'
        ],
        scope_exclusions=[
            'Third-party library vulnerabilities (report separately)',
            'Cryptographic algorithms (use standard libraries)',
            'Network protocol security (defer to security team)'
        ],
        ai_instructions="""
EXECUTION INSTRUCTIONS FOR AI AGENTS:
1. Locate all instances of vulnerable pattern in module
2. For each instance:
   - [ ] Identify call site and data flow
   - [ ] Determine safe replacement (std::string, parameterized query, env var)
   - [ ] Write unit test covering both normal and attack cases
   - [ ] Update documentation (security.md)
3. Run full test suite to ensure no regression
4. Submit PR with security-focused commit message
"""
    ),
    
    'memory': GapCategory(
        name='Memory Safety & Leaks',
        cwe='CWE-401/416/119',
        description='Memory leaks, use-after-free, buffer overflows',
        patterns=[
            'Unmatched new/delete pairs',
            'Pointer arithmetic without bounds checks',
            'Raw pointers in container operations',
            'Missing RAII wrappers for resources',
            'Exception-unsafe cleanup paths'
        ],
        acceptance_criteria=[
            'All dynamic allocations use smart pointers (unique_ptr/shared_ptr)',
            'No raw pointer arithmetic in public APIs',
            'All resource cleanup in destructors (RAII)',
            'Exception-safe even if constructor fails midway',
            'Valgrind/AddressSanitizer reports 0 memory errors'
        ],
        test_requirements=[
            'Memory leak detection (valgrind --leak-check=full)',
            'AddressSanitizer enabled in test build',
            'Exception safety tests (constructor exceptions)',
            'Large object lifecycle tests (allocation/deallocation stress)'
        ],
        scope_inclusions=[
            'Heap allocations (new/delete, malloc)',
            'Smart pointer migrations',
            'Exception paths in constructors',
            'Vector/map/string growth patterns'
        ],
        scope_exclusions=[
            'Stack-allocated objects (inherently safe)',
            'Third-party memory managers',
            'Custom allocator performance tuning'
        ],
        ai_instructions="""
EXECUTION INSTRUCTIONS FOR AI AGENTS:
1. Search for 'new' keyword not in smart_ptr constructors
2. For each raw 'new':
   - [ ] Identify owning scope
   - [ ] Determine if unique_ptr or shared_ptr
   - [ ] Add corresponding smart_ptr wrapper
   - [ ] Remove manual delete
   - [ ] Add unit test for cleanup (with exceptions)
3. Run AddressSanitizer: cmake --preset windows-release && ctest --preset windows-release
4. Verify Valgrind: valgrind --leak-check=full ./build/bin/test_<module>
5. Submit PR titled "Fix: Memory safety in <module> — replace raw new/delete with smart pointers"
"""
    ),
    
    'reliability': GapCategory(
        name='Reliability & Error Handling',
        cwe='CWE-252/391',
        description='Ignored error codes, missing timeouts, incomplete retry logic',
        patterns=[
            'Function calls with unchecked return values',
            'Network operations without timeouts',
            'Retry loops with unbounded attempts',
            'Errors logged but not propagated',
            'Fallback paths that mask errors'
        ],
        acceptance_criteria=[
            'All fallible function calls checked (return value or exceptions)',
            'Timeouts set on all I/O operations (network, disk, locks)',
            'Retry policies explicit and bounded (max 3x with exponential backoff)',
            'Errors logged at appropriate level with context',
            'No silent failures in production code'
        ],
        test_requirements=[
            'Timeout tests (verify timeout triggers)',
            'Error path tests (all error branches covered)',
            'Retry behavior tests (correct backoff, max attempts)',
            'Chaos tests (simulated failures at each fallible call)'
        ],
        scope_inclusions=[
            'I/O operations (network, disk, mutex)',
            'External service calls',
            'Error logging statements',
            'Retry/timeout configuration'
        ],
        scope_exclusions=[
            'Optimization of retry algorithms',
            'Circuit breaker patterns (design separately)',
            'Error recovery business logic'
        ],
        ai_instructions="""
EXECUTION INSTRUCTIONS FOR AI AGENTS:
1. Find all function calls marked [[nodiscard]] or returning error codes
2. For each unchecked call:
   - [ ] Add check: if (!result) or try-catch
   - [ ] Log error with context: logger->error("operation failed: {}", why);
   - [ ] Return error up the stack OR handle with bounded retry
   - [ ] Add test for error case
3. Find all I/O calls (send, recv, fopen, lock_guard)
   - [ ] Add timeout context
   - [ ] Document timeout value and rationale
   - [ ] Add timeout exceeded error handler
4. Run test suite: ctest --preset windows-release
5. Check test coverage: ctest --preset windows-release --coverage
"""
    ),
    
    'concurrency': GapCategory(
        name='Thread Safety & Data Races',
        cwe='CWE-362/366',
        description='Data races, missing synchronization, deadlock risks',
        patterns=[
            'Shared state without mutex protection',
            'Non-atomic flag checks without synchronization',
            'Lock acquisition order inconsistency',
            'Condition variable misuse (spurious wake)',
            'Reader-writer lock priority inversion'
        ],
        acceptance_criteria=[
            'All shared state protected by mutex',
            'Lock acquisition order consistent (prevent deadlock)',
            'Condition variables used correctly (check predicate in loop)',
            'std::atomic used for simple flags',
            'Thread sanitizer reports 0 data races'
        ],
        test_requirements=[
            'ThreadSanitizer enabled (-fsanitize=thread)',
            'Concurrent access stress tests (N threads, M operations)',
            'Deadlock detection tests (timeouts)',
            'Memory ordering tests (acquire/release semantics)'
        ],
        scope_inclusions=[
            'Shared data members (non-const)',
            'Global/static mutable state',
            'Lock-guarded critical sections',
            'Condition variables',
            'std::atomic operations'
        ],
        scope_exclusions=[
            'Atomic<T> optimization tuning',
            'Lock-free data structures (advanced)',
            'OS-specific synchronization'
        ],
        ai_instructions="""
EXECUTION INSTRUCTIONS FOR AI AGENTS:
1. Find all mutable member variables in classes with concurrent access
2. For each:
   - [ ] Add mutable std::mutex + std::lock_guard wrapper
   - [ ] Mark accessor functions const (if read-only)
   - [ ] Add doxygen comment: "Thread-safe via mutex_"
   - [ ] Add unit test: concurrent reads + writes
3. Find all static/global mutable variables
   - [ ] Either make const OR protect with std::call_once + static lock
   - [ ] Document synchronization in comment
4. Run ThreadSanitizer: ctest --preset windows-release
5. Verify no "WARNING: ThreadSanitizer: data race" in output
"""
    ),
    
    'raii': GapCategory(
        name='RAII & Resource Management',
        cwe='CWE-404/460',
        description='Resource leaks, missing destructors, improper cleanup',
        patterns=[
            'Class without destructor (holds file/socket/lock)',
            'Destructor not virtual (polymorphic cleanup)',
            'Move assignment operator missing',
            'Swap idiom not implemented',
            'Cleanup in try-finally instead of RAII'
        ],
        acceptance_criteria=[
            'All resource-holding classes have destructor + move semantics',
            'Virtual destructors in polymorphic classes',
            'Cleanup guaranteed even on exception',
            'No manual close/release calls needed in client code',
            'Copy operations explicitly deleted or defined (rule of five)'
        ],
        test_requirements=[
            'Resource leak tests (create/destroy cycles)',
            'Exception safety tests (throw during operations)',
            'Move semantics tests (moved-from state valid)',
            'Destructor call sequence tests (virtual dispatch)'
        ],
        scope_inclusions=[
            'File handles, sockets, mutexes',
            'Memory pools, buffer allocators',
            'Database connections, transaction scopes',
            'Move constructors/assignment operators'
        ],
        scope_exclusions=[
            'Third-party RAII wrappers',
            'Optimization of cleanup paths',
            'Custom allocator policies'
        ],
        ai_instructions="""
EXECUTION INSTRUCTIONS FOR AI AGENTS:
1. Find all classes holding resources (FILE*, socket, mutex, etc)
2. For each:
   - [ ] Check if destructor exists; if not, add
   - [ ] Verify destructor calls cleanup (close, unlock, free)
   - [ ] Add move constructor + move assignment operator
   - [ ] Delete copy constructor/assignment (if move-only resource)
   - [ ] Add doxygen comment documenting resource lifetime
3. Find all virtual classes without virtual destructor
   - [ ] Add virtual ~ClassName() = default;
4. Add tests for resource lifecycle
5. Verify no resource leaks with valgrind
"""
    ),
    
    'container': GapCategory(
        name='STL Container Misuse',
        cwe='CWE-1104/831',
        description='O(n²) patterns, missing reserves, inefficient operations',
        patterns=[
            'std::vector append in loop without reserve',
            'std::string concatenation in loop',
            'std::map used instead of std::unordered_map',
            'std::find in loop (O(n²) with vector)',
            'Unnecessary copies from containers'
        ],
        acceptance_criteria=[
            'std::vector::reserve called before loops',
            'std::stringstream or .append() used in loops',
            'std::map only for ordered traversal',
            'No repeated linear searches without indexing',
            'const references used for container element access'
        ],
        test_requirements=[
            'Performance tests: large container operations',
            'O(n) behavior verified (not O(n²))',
            'Memory allocation patterns checked (strace, valgrind)',
            'Cache efficiency tests (if applicable)'
        ],
        scope_inclusions=[
            'Loop-based container operations',
            'Container selection (vector vs set vs map)',
            'Pre-allocation patterns',
            'Element access methods'
        ],
        scope_exclusions=[
            'Algorithm complexity refactoring (separate PR)',
            'Custom container implementations',
            'Data structure redesigns'
        ],
        ai_instructions="""
EXECUTION INSTRUCTIONS FOR AI AGENTS:
1. Search for:
   - Loop + push_back without prior reserve()
   - std::string += in loop (not std::stringstream)
   - std::map usage where .find() is only operation
   - Nested loops with .find() (O(n²) pattern)
2. For each pattern:
   - [ ] Replace with efficient alternative
   - [ ] Add reserve(estimated_size) before loops
   - [ ] Add performance test with large inputs (1M+ elements)
   - [ ] Verify no allocation churn (strace or perf)
3. Run performance benchmarks to verify improvement
4. Submit PR: "Perf: Fix container inefficiencies in <module>"
"""
    ),
    
    'platform': GapCategory(
        name='Platform Portability',
        cwe='CWE-758/1007',
        description='Windows/Linux incompatibilities, endianness, 32/64-bit issues',
        patterns=[
            'Platform-specific code without #ifdef',
            'Hardcoded paths (C:\\\ vs /root/)',
            'sizeof assumptions across platforms',
            'Pointer-to-int casting without uintptr_t',
            'Byte order assumptions (endianness)'
        ],
        acceptance_criteria=[
            'Code builds on Windows, Linux, macOS without warnings',
            'Platform-specific code guarded with #ifdef',
            'Paths use std::filesystem::path',
            'No raw pointer casts (use reinterpret_cast + uintptr_t)',
            'No endianness assumptions'
        ],
        test_requirements=[
            'Cross-platform build tests (Windows + Linux)',
            'Path handling tests (forward slashes, drive letters)',
            'Endianness tests (big/little endian)',
            'Alignment tests (on platforms with strict alignment)'
        ],
        scope_inclusions=[
            'Platform-specific APIs (Windows.h, unistd.h)',
            'Path construction and manipulation',
            'Binary format I/O',
            'Integer size assumptions'
        ],
        scope_exclusions=[
            'Third-party library portability',
            'Build system configuration (CMake)',
            'Docker container setup'
        ],
        ai_instructions="""
EXECUTION INSTRUCTIONS FOR AI AGENTS:
1. Compile with -Wall -Wextra and check for platform warnings
2. Search for hardcoded platform-specific code:
   - Find C:\\\\ paths → replace with std::filesystem::path
   - Find /etc or /home → replace with env vars or config
   - Find Windows API calls → wrap in #ifdef _WIN32
3. Find sizeof() assumptions:
   - [ ] Verify on both 32 and 64-bit
   - [ ] Use fixed-width types (uint64_t) where size matters
4. Find pointer casts:
   - [ ] Replace (uint64_t)ptr with reinterpret_cast<uintptr_t>(ptr)
5. Verify builds on Windows and Linux with no errors/warnings
"""
    ),
    
    'performance': GapCategory(
        name='Performance Anti-Patterns',
        cwe='CWE-1104',
        description='String concatenation loops, synchronous I/O, unnecessary copies',
        patterns=[
            'std::string += in loop',
            'std::endl vs \'\\n\' (unnecessary flush)',
            'std::regex compiled in loop',
            'Mutex lock in hot path',
            'std::function with dynamic dispatch overhead'
        ],
        acceptance_criteria=[
            'Hot paths identified and optimized',
            'String concatenation uses std::stringstream',
            'Regex patterns compiled once and reused',
            'Lock-free operations in hot paths',
            'Benchmark regression tests in place'
        ],
        test_requirements=[
            'Micro benchmarks for critical sections',
            'Regression tests (performance targets)',
            'Profiling data (flame graphs, perf records)',
            'Memory allocation tracking'
        ],
        scope_inclusions=[
            'Hot paths (>10% CPU in profiling)',
            'Loop performance patterns',
            'Allocation/deallocation patterns',
            'Synchronization overhead'
        ],
        scope_exclusions=[
            'Algorithm complexity improvements (separate)',
            'Compiler optimizations (handled by flags)',
            'Hardware-specific tuning (SIMD, etc.)'
        ],
        ai_instructions="""
EXECUTION INSTRUCTIONS FOR AI AGENTS:
1. Run benchmark suite with -O3 optimization
2. Generate flame graph to find hot paths
3. For each hot path:
   - [ ] Profile with perf or VTune
   - [ ] Identify allocation churn
   - [ ] Remove unnecessary copies
   - [ ] Cache expensive computations
4. Verify improvements with benchmark regression tests
5. Submit PR: "Perf: Optimize <section> in <module> — <specific improvement>"
"""
    ),
}


def generate_detailed_issue_body(
    module: str,
    total_gaps: int,
    critical: int,
    high: int,
    medium: int,
    gaps_by_file: Dict[str, List[Dict]],
    gap_categories: Dict[str, int]
) -> str:
    """Generate detailed GitHub issue body with AI-agent instructions"""
    
    # Determine priority
    if critical >= 10:
        priority = '🔴 CRITICAL'
    elif high >= 5:
        priority = '🟠 HIGH'
    else:
        priority = '🟡 MEDIUM'
    
    body = f"""# {priority} — {module.upper()} Module Gap Analysis & Remediation Guide

**AI Agent Ready:** This issue contains detailed acceptance criteria, scope boundaries, and execution instructions for automated implementation.

---

## Executive Summary

| Metric | Value |
|--------|-------|
| **Total Gaps** | {total_gaps} |
| **🔴 CRITICAL** | {critical} (~{100*critical//max(total_gaps,1)}%) |
| **🟠 HIGH** | {high} (~{100*high//max(total_gaps,1)}%) |
| **🟡 MEDIUM** | {medium} (~{100*medium//max(total_gaps,1)}%) |
| **Estimated Effort** | {_estimate_effort(critical, high)} |
| **Priority** | {priority} |

---

## Gap Breakdown by Category

"""
    
    # List gaps by category with detailed remediation guidance
    for cat_name in sorted(gap_categories.keys(), key=lambda x: gap_categories[x], reverse=True):
        count = gap_categories[cat_name]
        if count == 0:
            continue
        
        if cat_name in GAP_CATEGORIES:
            category = GAP_CATEGORIES[cat_name]
            body += f"""
### {category.name} ({count} gaps)

**CWE:** {category.cwe}  
**Description:** {category.description}

#### Patterns to Fix

"""
            for pattern in category.patterns:
                body += f"- [ ] {pattern}\n"
            
            body += f"""

#### Acceptance Criteria

When fixing these gaps, ensure:

"""
            for criterion in category.acceptance_criteria:
                body += f"- [ ] {criterion}\n"
            
            body += f"""

#### Test Requirements

Verify fixes with:

"""
            for test in category.test_requirements:
                body += f"- [ ] {test}\n"
            
            body += f"""

#### Scope Definition

**IN SCOPE — Fix in this PR:**
"""
            for item in category.scope_inclusions:
                body += f"- {item}\n"
            
            body += """
**OUT OF SCOPE — Handle separately:**
"""
            for item in category.scope_exclusions:
                body += f"- {item}\n"
    
    # Top files with gaps
    body += "\n---\n\n## High-Impact Files (Priority Order)\n\n"
    
    top_files = sorted(
        gaps_by_file.items(),
        key=lambda x: sum(1 for g in x[1] if g.get('severity') == 'CRITICAL') * 100 +
                      sum(1 for g in x[1] if g.get('severity') == 'HIGH') * 10,
        reverse=True
    )[:15]
    
    for idx, (file_path, gaps_in_file) in enumerate(top_files, 1):
        critical_count = sum(1 for g in gaps_in_file if g.get('severity') == 'CRITICAL')
        high_count = sum(1 for g in gaps_in_file if g.get('severity') == 'HIGH')
        
        body += f"\n#### {idx}. `{file_path}`\n\n"
        body += f"- **Gaps:** {len(gaps_in_file)} (🔴 {critical_count}, 🟠 {high_count})\n"
        
        # Extract unique categories in this file
        cats_in_file = {}
        for gap in gaps_in_file:
            cat = gap.get('category', 'unknown')
            cats_in_file[cat] = cats_in_file.get(cat, 0) + 1
        
        body += "- **Categories:** " + ", ".join([f"{cat} ({cnt})" for cat, cnt in sorted(cats_in_file.items())]) + "\n"
        body += f"- [ ] Review and remediate gaps in this file\n"
    
    # AI agent instructions
    body += f"""

---

## 🤖 AI Agent Execution Instructions

### Prerequisites

- [ ] `cmake --preset windows-release` configured
- [ ] Full test suite passing: `ctest --preset windows-release`
- [ ] Python >= 3.10 with gap scanner tools available

### Execution Steps

For each gap category above:

1. **Locate All Instances**
   - Use gap scanner detailed output: `ai_working/gap_scan_v3_{module}.json`
   - Identify all source files with gaps

2. **Implement Fixes** (Follow category-specific instructions above)
   - Apply acceptance criteria
   - Implement test cases
   - Update documentation

3. **Verify Fixes**
   - [ ] All tests passing: `ctest --preset windows-release --filter "test_<module>*"`
   - [ ] No new compiler warnings: `cmake --build --preset windows-release 2>&1 | grep warning`
   - [ ] Memory sanitizer clean: `ctest --preset windows-release --sanitizer`
   - [ ] Code review checklist: [see below](#code-review-checklist)

4. **Submit PR**
   - Title: "Fix: <Module> gap remediation — <specific improvements>"
   - Description: Reference this issue + category-specific improvements
   - Checklist: ✅ Tests passing, ✅ Documentation updated, ✅ No regressions

---

## Code Review Checklist

- [ ] All acceptance criteria met for fixed gaps
- [ ] Tests cover both normal and error cases
- [ ] No new compiler warnings
- [ ] Memory safety verified (Valgrind/ASAN clean)
- [ ] Thread safety verified (if applicable)
- [ ] Documentation updated (code comments, README, ROADMAP)
- [ ] Performance impact acceptable (benchmarks if needed)
- [ ] Backwards compatibility maintained

---

## Related Documentation

- [Module Architecture]({module}/ARCHITECTURE.md)
- [Module Roadmap]({module}/ROADMAP.md)
- [Gap Scanner Report](ai_working/gap_scan_v3_{module}.json)
- [Full Gap Index](ai_working/MODULE_GAPS_INDEX.md)

---

## Resources

- [ThemisDB Contribution Guide](CONTRIBUTING.md)
- [C++ Best Practices](.github/instructions/cpp-best-practices.instructions.md)
- [Security Guidelines](SECURITY.md)

---

*Generated by Enhanced Gap Issue Template Generator*  
*Scope: AI-Agent Ready with Detailed Remediation Guidance*
"""
    
    return body


def _estimate_effort(critical: int, high: int) -> str:
    """Estimate development effort"""
    hours = (critical * 2.5) + (high * 1.5)
    
    if hours < 8:
        return f"{hours:.1f}h (1 dev-day)"
    elif hours < 40:
        days = hours / 8
        return f"{days:.1f} dev-days"
    else:
        weeks = hours / 40
        return f"{weeks:.1f} weeks"


def generate_all_enhanced_templates(aggregate_file: str, output_dir: str):
    """Generate enhanced issue templates for all modules"""
    
    with open(aggregate_file, 'r') as f:
        aggregate = json.load(f)
    
    output_path = Path(output_dir) / 'enhanced_issues'
    output_path.mkdir(parents=True, exist_ok=True)
    
    generated = 0
    for module, data in aggregate.items():
        total = data.get('total', 0)
        if total == 0:
            continue
        
        body = generate_detailed_issue_body(
            module=module,
            total_gaps=total,
            critical=data.get('severity_critical', 0),
            high=data.get('severity_high', 0),
            medium=data.get('severity_medium', 0),
            gaps_by_file=data.get('gaps_by_file', {}),
            gap_categories=data.get('by_category', {})
        )
        
        output_file = output_path / f"{module}_enhanced_issues.md"
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(body)
        
        generated += 1
        print(f"[OK] Generated: {output_file}")
    
    print(f"\n[OK] Generated {generated} enhanced issue templates")
    print(f"[OK] Location: {output_path}")
    
    return generated


if __name__ == '__main__':
    import sys
    
    aggregate_file = 'ai_working/gap_scan_v3_aggregate.json'
    output_dir = 'ai_working'
    
    if not Path(aggregate_file).exists():
        print(f"[ERROR] Gap analysis file not found: {aggregate_file}")
        sys.exit(1)
    
    generate_all_enhanced_templates(aggregate_file, output_dir)
