#include <gtest/gtest.h>
#include "core/concerns/adapter_registry.h"
#include "core/concerns/adapter_signing.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <thread>
#include <vector>

using namespace themis::core::concerns;

// ---------------------------------------------------------------------------
// Helper: create a temporary file with given content; removed by RAII guard
// ---------------------------------------------------------------------------
struct TempFile {
    std::string path;

    explicit TempFile(const std::string& suffix, const std::string& content = "") {
        path = (std::filesystem::temp_directory_path() /
                ("themis_test_plugin" + suffix))
                   .string();
        if (!content.empty()) {
            std::ofstream f(path, std::ios::binary);
            f << content;
        }
    }

    ~TempFile() {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }
};

// ---------------------------------------------------------------------------
// PL_01 — empty path string → false, no crash
// ---------------------------------------------------------------------------
TEST(PluginLoadingTest, PL_01_EmptyPathReturnsFalse) {
    AdapterRegistry reg;
    EXPECT_FALSE(reg.loadFromPlugin("", "alpha"));
}

// ---------------------------------------------------------------------------
// PL_02 — non-existent file path → false
// ---------------------------------------------------------------------------
TEST(PluginLoadingTest, PL_02_NonExistentPathReturnsFalse) {
    AdapterRegistry reg;
    const std::string bogus = "/tmp/__themis_no_such_plugin_pl02__.so";
    EXPECT_FALSE(std::filesystem::exists(bogus));
    EXPECT_FALSE(reg.loadFromPlugin(bogus, "alpha"));
}

// ---------------------------------------------------------------------------
// PL_03 — path is a regular text file, not a shared library → false
//         (dlopen will fail with a format error)
// ---------------------------------------------------------------------------
TEST(PluginLoadingTest, PL_03_NotALibraryReturnsFalse) {
    TempFile tf{"_pl03.so", "this is not an ELF/PE shared library\n"};
    AdapterRegistry reg;
    EXPECT_FALSE(reg.loadFromPlugin(tf.path, "alpha"));
}

// ---------------------------------------------------------------------------
// PL_04 — kRequireSignature: .sig file absent → false
// ---------------------------------------------------------------------------
TEST(PluginLoadingTest, PL_04_RequireSignatureMissingSigFileReturnsFalse) {
    TempFile tf{"_pl04.so", "fake lib content"};

    AdapterRegistry reg;
    reg.setTrustPolicy(AdapterTrustPolicy::kRequireSignature);
    EXPECT_FALSE(reg.loadFromPlugin(tf.path, "alpha"));
}

// ---------------------------------------------------------------------------
// PL_05 — kRequireSignature: .sig file present but digest mismatches → false
// ---------------------------------------------------------------------------
TEST(PluginLoadingTest, PL_05_RequireSignatureDigestMismatchReturnsFalse) {
    const std::string lib_content = "fake shared library bytes";
    TempFile tf_lib{"_pl05.so", lib_content};
    TempFile tf_sig{"_pl05.so.sig",
                    "0000000000000000000000000000000000000000000000000000000000000000\n"};
    // Give .sig the library's path + ".sig" name
    // Rename the sig temp file to match the lib path
    std::error_code ec;
    std::filesystem::rename(tf_sig.path, tf_lib.path + ".sig", ec);
    tf_sig.path = tf_lib.path + ".sig"; // update so RAII removes it

    AdapterRegistry reg;
    reg.setTrustPolicy(AdapterTrustPolicy::kRequireSignature);
    EXPECT_FALSE(reg.loadFromPlugin(tf_lib.path, "alpha"));
}

// ---------------------------------------------------------------------------
// PL_06 — kTrustAll: signature check is skipped; failure is from dlopen
// ---------------------------------------------------------------------------
TEST(PluginLoadingTest, PL_06_TrustAllSkipsSignatureCheck) {
    // A real text file: dlopen will fail (not a valid library), but we must
    // NOT hit the "signature required but not found" branch.
    TempFile tf{"_pl06.so", "not a real library"};

    AdapterRegistry reg;
    reg.setTrustPolicy(AdapterTrustPolicy::kTrustAll);
    // Should return false due to dlopen failure, not due to missing .sig
    bool result = reg.loadFromPlugin(tf.path, "alpha");
    EXPECT_FALSE(result);
    // Registry state must remain consistent
    EXPECT_EQ(reg.count(), 0u);
}

// ---------------------------------------------------------------------------
// PL_07 — setTrustPolicy does not corrupt registry state
// ---------------------------------------------------------------------------
TEST(PluginLoadingTest, PL_07_SetTrustPolicyDoesNotCorruptRegistry) {
    AdapterRegistry reg;

    // Register a real adapter first
    struct IFoo { virtual ~IFoo() = default; virtual int v() const = 0; };
    struct FooImpl : IFoo { int v() const override { return 7; } };

    reg.registerAdapter<IFoo>("foo", std::make_shared<FooImpl>());
    EXPECT_EQ(reg.count(), 1u);

    // Change policy
    reg.setTrustPolicy(AdapterTrustPolicy::kRequireSignature);
    reg.setTrustPolicy(AdapterTrustPolicy::kTrustAll);

    // Adapter must still be resolvable
    auto p = reg.resolve<IFoo>();
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->v(), 7);
}

// ---------------------------------------------------------------------------
// PL_08 — concurrent loadFromPlugin calls on same bad path do not crash
// ---------------------------------------------------------------------------
TEST(PluginLoadingTest, PL_08_ConcurrentLoadFromPluginBadPathNoCrash) {
    const std::string bogus = "/tmp/__themis_no_such_plugin_pl08__.so";

    AdapterRegistry reg;
    constexpr int kThreads = 8;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&reg, &bogus] {
            (void)reg.loadFromPlugin(bogus, "alpha");
        });
    }
    for (auto& t : threads) {
      t.join();
    }

    // Registry must be in a valid state after concurrent failures
    EXPECT_EQ(reg.count(), 0u);
}
