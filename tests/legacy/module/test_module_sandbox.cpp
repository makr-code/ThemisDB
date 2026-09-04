/// @file test_module_sandbox.cpp
/// @brief Unit tests for ModuleSandbox and AbiChecker (Phase 4)

#include <gtest/gtest.h>
#include "themis/base/module_sandbox.h"
#include "themis/base/module_loader.h"

#ifndef _WIN32
#  include <sys/resource.h>
#  include <sys/stat.h>
#  include <unistd.h>
#endif

#if defined(__linux__)
#  include <cerrno>
#  include <fstream>
#  include <sys/mman.h>
#  include <sys/types.h>
#  include <sys/wait.h>
#  include <chrono>
#  include <cstring>
#endif

using namespace themis::modules;

// =============================================================================
// AbiChecker – version checks
// =============================================================================

TEST(AbiChecker, DefaultConstruction) {
    EXPECT_NO_THROW({ AbiChecker checker; });
}

TEST(AbiChecker, UseDefaultListsDoesNotThrow) {
    AbiChecker checker;
    EXPECT_NO_THROW(checker.useDefaultLists());
}

TEST(AbiChecker, CompatibleVersions) {
    AbiChecker checker;
    ModuleMetadata meta;
    meta.version      = "1.6.0";
    meta.themisMajor  = 1;
    meta.themisMinor  = 5; // module is older minor → compatible
    meta.themisPatch  = 0;

    auto result = checker.checkVersions(meta, 1 /*host_major*/, 6 /*host_minor*/);
    EXPECT_TRUE(result.compatible);
    EXPECT_TRUE(result.issues.empty()) << result.issues[0];
}

TEST(AbiChecker, IncompatibleMajor) {
    AbiChecker checker;
    ModuleMetadata meta;
    meta.version      = "2.0.0";
    meta.themisMajor  = 2;
    meta.themisMinor  = 0;
    meta.themisPatch  = 0;

    auto result = checker.checkVersions(meta, 1, 6);
    EXPECT_FALSE(result.compatible);
    EXPECT_FALSE(result.issues.empty());
}

TEST(AbiChecker, IncompatibleMinorTooNew) {
    AbiChecker checker;
    ModuleMetadata meta;
    meta.version      = "1.9.0";
    meta.themisMajor  = 1;
    meta.themisMinor  = 9; // module requires 1.9, but host is 1.6
    meta.themisPatch  = 0;

    auto result = checker.checkVersions(meta, 1, 6);
    EXPECT_FALSE(result.compatible);
}

TEST(AbiChecker, ExactVersionMatch) {
    AbiChecker checker;
    ModuleMetadata meta;
    meta.version      = "1.6.0";
    meta.themisMajor  = 1;
    meta.themisMinor  = 6;
    meta.themisPatch  = 0;

    auto result = checker.checkVersions(meta, 1, 6);
    EXPECT_TRUE(result.compatible);
}

TEST(AbiChecker, EmptyVersionIsWarningNotFailure) {
    AbiChecker checker;
    ModuleMetadata meta;
    meta.version      = ""; // missing version string
    meta.themisMajor  = 1;
    meta.themisMinor  = 0;
    meta.themisPatch  = 0;

    auto result = checker.checkVersions(meta, 1, 6);
    EXPECT_TRUE(result.compatible); // warning only, not fail
    EXPECT_FALSE(result.issues.empty()) << "Should have at least a warning";
}

TEST(AbiChecker, RequiredSymbolsMissingOnNullHandle) {
    AbiChecker checker;
    checker.addRequiredSymbol("themis_module_init");

    // null handle → cannot resolve any symbol
    auto result = checker.checkRequiredSymbols(nullptr);
    EXPECT_FALSE(result.compatible);
    EXPECT_FALSE(result.issues.empty());
}

TEST(AbiChecker, NoRequiredSymbolsAlwaysPasses) {
    AbiChecker checker;
    // No required symbols added
    auto result = checker.checkRequiredSymbols(nullptr);
    EXPECT_TRUE(result.compatible);
    EXPECT_TRUE(result.issues.empty());
}

TEST(AbiChecker, DeprecatedSymbolsOnNullHandleNoIssues) {
    AbiChecker checker;
    checker.addDeprecatedSymbol("old_symbol");
    // null handle → dlsym/GetProcAddress returns nullptr → no deprecated symbol found
    auto result = checker.checkDeprecatedSymbols(nullptr);
    EXPECT_TRUE(result.compatible); // deprecated is non-fatal
    EXPECT_TRUE(result.issues.empty());
}

TEST(AbiChecker, FullCheckWithNullHandleVersionIncompatible) {
    AbiChecker checker;
    checker.useDefaultLists();

    ModuleMetadata meta;
    meta.version      = "2.0.0";
    meta.themisMajor  = 2;
    meta.themisMinor  = 0;
    meta.themisPatch  = 0;

    auto result = checker.check(nullptr, meta, 1, 6);
    EXPECT_FALSE(result.compatible);
    EXPECT_FALSE(result.summary.empty());
}

TEST(AbiChecker, FullCheckCompatibleVersionsNullHandle) {
    AbiChecker checker;
    // No required symbols → will still check version only
    ModuleMetadata meta;
    meta.version      = "1.5.0";
    meta.themisMajor  = 1;
    meta.themisMinor  = 5;
    meta.themisPatch  = 0;

    auto result = checker.check(nullptr, meta, 1, 6);
    // Version OK, no required symbols → should be compatible
    EXPECT_TRUE(result.compatible);
}

TEST(AbiChecker, SummaryAlwaysNonEmpty) {
    AbiChecker checker;
    ModuleMetadata meta;
    meta.themisMajor = 1; meta.themisMinor = 0;
    auto result = checker.check(nullptr, meta, 1, 0);
    EXPECT_FALSE(result.summary.empty());
}

// =============================================================================
// ModuleSandbox – configuration & construction
// =============================================================================

TEST(ModuleSandbox, DefaultConfig) {
    ModuleSandbox::Config cfg;
    EXPECT_EQ(cfg.max_memory_mb,   256u);
    EXPECT_EQ(cfg.max_cpu_percent, 50);
    EXPECT_FALSE(cfg.allow_network);
    EXPECT_EQ(cfg.fs_access, ModuleSandbox::FilesystemAccess::READ_ONLY);
}

TEST(ModuleSandbox, ConstructWithDefaultConfig) {
    EXPECT_NO_THROW({ ModuleSandbox sb; });
}

TEST(ModuleSandbox, InitiallyNotActive) {
    ModuleSandbox sb;
    EXPECT_FALSE(sb.isActive());
}

TEST(ModuleSandbox, LaunchSucceeds) {
    ModuleSandbox sb;
    bool ok = sb.launch("test_module");
    EXPECT_TRUE(ok) << "launch() must succeed; error: " << sb.lastError();
    EXPECT_TRUE(sb.isActive());
}

TEST(ModuleSandbox, ShutdownMakesInactive) {
    ModuleSandbox sb;
    sb.launch("test_module");
    ASSERT_TRUE(sb.isActive());
    sb.shutdown();
    EXPECT_FALSE(sb.isActive());
}

TEST(ModuleSandbox, StatsAvailableAfterLaunch) {
    ModuleSandbox sb;
    sb.launch("test_module");
    // stats() must not crash
    auto stats = sb.stats();
    (void)stats;
    EXPECT_FALSE(stats.killed);
}

TEST(ModuleSandbox, LaunchWarningsAreStrings) {
    ModuleSandbox sb;
    sb.launch("test_module");
    // Each warning (if any) must be a non-empty string
    for (const auto& w : sb.launchWarnings()) {
        EXPECT_FALSE(w.empty());
    }
}

TEST(ModuleSandbox, ZeroMemoryLimitSkipped) {
    ModuleSandbox::Config cfg;
    cfg.max_memory_mb = 0;
    cfg.max_cpu_percent = 0;
    ModuleSandbox sb(cfg);
    EXPECT_TRUE(sb.launch("no_limits_module"));
    EXPECT_TRUE(sb.isActive());
}

TEST(ModuleSandbox, DestructorShutdown) {
    // Verify destructor implicitly shuts down an active sandbox
    {
        ModuleSandbox sb;
        sb.launch("temporary_module");
        EXPECT_TRUE(sb.isActive());
    } // ~ModuleSandbox should call shutdown() without crash
    SUCCEED();
}

TEST(ModuleSandbox, NetworkIsolationWarning) {
    ModuleSandbox::Config cfg;
    cfg.allow_network   = false;
    cfg.max_memory_mb   = 0; // no limits
    cfg.max_cpu_percent = 0;
    ModuleSandbox sb(cfg);
    sb.launch("network_restricted_module");
    // On most platforms, network isolation produces a warning (not a failure)
    // No assertion needed; just ensure it doesn't crash.
    SUCCEED();
}

TEST(ModuleSandbox, DefaultCpuTimeSecondsIsZero) {
    ModuleSandbox::Config cfg;
    EXPECT_EQ(cfg.max_cpu_time_seconds, 0u);
}

TEST(ModuleSandbox, CpuTimeLimitZeroSkipped) {
    ModuleSandbox::Config cfg;
    cfg.max_cpu_time_seconds = 0;
    cfg.max_memory_mb        = 0;
    cfg.max_cpu_percent      = 0;
    ModuleSandbox sb(cfg);
#if defined(__linux__)
    struct rlimit before_launch{};
    getrlimit(RLIMIT_CPU, &before_launch);
#endif
    EXPECT_TRUE(sb.launch("no_cpu_time_limit"));
    EXPECT_TRUE(sb.isActive());
#if defined(__linux__)
    // RLIMIT_CPU must be unchanged since no CPU time limit was configured
    struct rlimit after_launch{};
    getrlimit(RLIMIT_CPU, &after_launch);
    EXPECT_EQ(after_launch.rlim_cur, before_launch.rlim_cur);
#endif
}

TEST(ModuleSandbox, CpuTimeLimitApplied) {
    ModuleSandbox::Config cfg;
    cfg.max_cpu_time_seconds = 3600; // 1-hour cap
    cfg.max_memory_mb        = 0;
    cfg.max_cpu_percent      = 0;
    ModuleSandbox sb(cfg);
    // Snapshot original limit before launch
#if defined(__linux__)
    struct rlimit before{};
    getrlimit(RLIMIT_CPU, &before);
#endif
    // launch() must succeed regardless of whether RLIMIT_CPU is supported
    bool ok = sb.launch("cpu_time_limited_module");
    EXPECT_TRUE(ok) << "launch() failed: " << sb.lastError();
    EXPECT_TRUE(sb.isActive());
#if defined(__linux__)
    // If RLIMIT_CPU was applied, verify the limit is now 3600 (or unchanged on failure)
    struct rlimit after_launch{};
    getrlimit(RLIMIT_CPU, &after_launch);
    // If cpu_limit_applied: limit must equal 3600; otherwise it remains unchanged.
    bool any_rlimit_warning = false;
    for (const auto& w : sb.launchWarnings())
        if (w.find("RLIMIT_CPU not applied") != std::string::npos) {
          any_rlimit_warning = true;
        }
    if (!any_rlimit_warning) {
        EXPECT_EQ(after_launch.rlim_cur, static_cast<rlim_t>(3600u));
    }
#endif
    sb.shutdown();
    EXPECT_FALSE(sb.isActive());
#if defined(__linux__)
    // After shutdown the limit should be restored to the original value
    struct rlimit after_shutdown{};
    getrlimit(RLIMIT_CPU, &after_shutdown);
    if (!any_rlimit_warning) {
        EXPECT_EQ(after_shutdown.rlim_cur, before.rlim_cur);
    }
#endif
}

// =============================================================================
// ModuleSandbox – cgroup v2 resource enforcement (Linux only)
// =============================================================================

#if defined(__linux__)

// Helper: returns true when /sys/fs/cgroup/cgroup.controllers exists AND the
// process can create directories under /sys/fs/cgroup/ (i.e. cgroup v2 is
// both present and delegated to us).
static bool cgroupV2Writable() {
    struct stat st{};
    if (::stat("/sys/fs/cgroup/cgroup.controllers", &st) != 0) {
      return false;
    }
    return ::access("/sys/fs/cgroup", W_OK) == 0;
}

/// When cgroup v2 is not writable the sandbox must still launch successfully
/// using the RLIMIT_* fallback path.
TEST(ModuleSandbox, CgroupV2FallbackWhenUnavailable) {
    if (cgroupV2Writable()) {
        GTEST_SKIP() << "cgroup v2 is writable – fallback path not exercised";
    }

    ModuleSandbox::Config cfg;
    cfg.max_memory_mb    = 128;
    cfg.max_cpu_percent  = 50;
    cfg.max_cpu_time_seconds = 0;
    ModuleSandbox sb(cfg);

    EXPECT_TRUE(sb.launch("fallback_module"))
        << "launch() must succeed even without cgroup v2: " << sb.lastError();
    EXPECT_TRUE(sb.isActive());
    sb.shutdown();
    EXPECT_FALSE(sb.isActive());
}

/// When cgroup v2 IS writable, setupCgroupV2 must create the cgroup directory,
/// populate memory.max, and teardownCgroupV2 must remove it on shutdown.
TEST(ModuleSandbox, CgroupV2SetupAndTeardown) {
    if (!cgroupV2Writable()) {
        GTEST_SKIP() << "cgroup v2 not writable – skipping cgroup v2 setup test";
    }

    ModuleSandbox::Config cfg;
    cfg.max_memory_mb   = 64;   // 64 MB
    cfg.max_cpu_percent = 25;
    cfg.max_cpu_time_seconds = 0;
    ModuleSandbox sb(cfg);

    ASSERT_TRUE(sb.launch("cgroup_test_module"))
        << "launch() failed: " << sb.lastError();
    EXPECT_TRUE(sb.isActive());

    // Verify the cgroup directory was created and memory.max was written.
    // The expected path follows the pattern /sys/fs/cgroup/themis/<name>_<pid>/
    const std::string expected_base = "/sys/fs/cgroup/themis";
    struct stat st{};
    EXPECT_EQ(::stat(expected_base.c_str(), &st), 0)
        << "Parent cgroup dir missing: " << expected_base;

    // Verify memory.max contains a non-zero limit.
    bool found_memory_max = false;
    for (const auto& entry : {
            expected_base + "/cgroup_test_module_" + std::to_string(::getpid())
         }) {
        std::ifstream mem_max(entry + "/memory.max");
        if (mem_max.is_open()) {
            uint64_t limit = 0;
            mem_max >> limit;
            EXPECT_EQ(limit, static_cast<uint64_t>(64) * 1024 * 1024)
                << "memory.max should be 64 MiB";
            found_memory_max = true;
            break;
        }
    }
    EXPECT_TRUE(found_memory_max) << "memory.max not found under cgroup dir";

    // Shutdown must remove the cgroup directory.
    sb.shutdown();
    EXPECT_FALSE(sb.isActive());

    // After teardown the sandbox-specific sub-directory must be gone.
    const std::string cg_dir =
        expected_base + "/cgroup_test_module_" + std::to_string(::getpid());
    EXPECT_NE(::stat(cg_dir.c_str(), &st), 0)
        << "cgroup dir should have been removed after shutdown: " << cg_dir;
}

/// Integration test: fork a child process, enroll it in a cgroup limited to
/// a small amount of memory, have it allocate more than the limit, and verify
/// the OOM killer terminates it within 500 ms.
///
/// Exit-code protocol used by the child:
///   42  – cgroup setup succeeded; child allocated memory and survived (test failure)
///   0   – cgroup launch failed inside child (skip/no-op, parent will skip)
///   OOM – killed by SIGKILL before it can exit (expected / pass)
TEST(ModuleSandbox, CgroupV2MemoryLimitEnforcement) {
    if (!cgroupV2Writable()) {
        GTEST_SKIP() << "cgroup v2 not writable – skipping OOM enforcement test";
    }

    // Use an 8 MiB hard limit; the child will try to fault 32 MiB.
    constexpr size_t limit_mb  = 8;
    constexpr size_t alloc_mb  = 32;
    // Sentinel exit code: child exits with this value when the sandbox
    // launched successfully and it survived the allocation (test failure).
    constexpr int EXIT_SURVIVED = 42;

    pid_t child = ::fork();
    ASSERT_NE(child, -1) << "fork() failed: " << ::strerror(errno);

    if (child == 0) {
        // ── child ────────────────────────────────────────────────────────
        // Create a sandbox for ourselves and join the cgroup.
        ModuleSandbox::Config cfg;
        cfg.max_memory_mb   = limit_mb;
        cfg.max_cpu_percent = 0;
        cfg.max_cpu_time_seconds = 0;
        ModuleSandbox sb(cfg);

        // If cgroup setup fails (e.g. kernel delegates our specific sub-tree
        // differently), exit 0 so the parent can distinguish this from
        // a successful-but-survived allocation.
        if (!sb.launch("oom_test_child")) {
            _exit(0);
        }

        // Allocate and touch pages so the kernel must account for them.
        // mmap with MAP_POPULATE forces physical-page faults immediately.
        const size_t bytes = alloc_mb * 1024 * 1024;
        void* p = ::mmap(nullptr, bytes, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
        if (p != MAP_FAILED) {
            // Touch every page to ensure physical allocation.
            volatile char* mem = static_cast<volatile char*>(p);
            for (size_t i = 0; i < bytes; i += 4096) {
                mem[i] = static_cast<char>(i & 0xFF);
            }
            ::munmap(p, bytes);
        }

        // If we reach here the OOM killer did not fire after a successful
        // cgroup v2 setup.  Use the sentinel code so the parent knows.
        _exit(EXIT_SURVIVED);
    }

    // ── parent ───────────────────────────────────────────────────────────
    // Wait for the child with a 500 ms deadline.
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
    int wstatus = 0;
    bool child_done = false;

    while (std::chrono::steady_clock::now() < deadline) {
        pid_t ret = ::waitpid(child, &wstatus, WNOHANG);
        if (ret == child) { child_done = true; break; }
        struct timespec ts{0, 10'000'000}; // 10 ms
        ::nanosleep(&ts, nullptr);
    }

    if (!child_done) {
        // Timed out – kill the child and mark the test as failed.
        ::kill(child, SIGKILL);
        ::waitpid(child, &wstatus, 0);
        FAIL() << "Child was not killed within 500 ms; "
                  "cgroup v2 OOM enforcement may not be working";
    }

    if (WIFEXITED(wstatus)) {
        const int code = WEXITSTATUS(wstatus);
        if (code == 0) {
            // cgroup launch failed inside the child – treat as skip.
            GTEST_SKIP() << "cgroup v2 launch failed inside child process; skipping";
        }
        // Any other non-zero code (including EXIT_SURVIVED) means the child
        // survived the allocation with an active cgroup – that is a failure.
        FAIL() << "Child survived memory allocation with cgroup v2 active "
                  "(exit code " << code << "); OOM enforcement is not working";
    }

    // Killed by a signal – the only acceptable signal is SIGKILL from OOM.
    ASSERT_TRUE(WIFSIGNALED(wstatus)) << "Unexpected child state";
    EXPECT_EQ(WTERMSIG(wstatus), SIGKILL)
        << "Expected SIGKILL from cgroup OOM killer";
}

#endif // __linux__
