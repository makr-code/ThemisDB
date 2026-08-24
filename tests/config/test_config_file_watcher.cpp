#include <gtest/gtest.h>
#include "config/config_file_watcher.h"
#include "config/config_path_resolver.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>

namespace themis {
namespace config {
namespace test {

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

/// @brief Write content to a file, creating directories as needed.
/// @param path File path to write to
/// @param content Optional content (default: empty file). Single-arg version creates empty file.
inline void writeFile(const std::filesystem::path& path,
                      const std::string& content = "") {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path);
    if (!file) {
        throw std::runtime_error("Failed to create file: " + path.string());
    }
    if (!content.empty()) {
        file << content;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// ConfigFileWatcher unit tests
// ─────────────────────────────────────────────────────────────────────────────

class ConfigFileWatcherTest : public ::testing::Test {
protected:
    void SetUp() override {
        watch_dir_ = std::filesystem::temp_directory_path() /
                     ("themis_fw_test_" + std::to_string(
                          std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(watch_dir_);
    }

    void TearDown() override {
        std::filesystem::remove_all(watch_dir_);
    }

    std::filesystem::path watch_dir_;
};

// ── Construction & lifecycle ──────────────────────────────────────────────────

TEST_F(ConfigFileWatcherTest, ConstructorSetsProperties) {
    ConfigFileWatcher watcher(watch_dir_.string(), [] {});
    EXPECT_EQ(watcher.watchPath(), watch_dir_.string());
    EXPECT_EQ(watcher.debounceInterval(), std::chrono::milliseconds(200));
    EXPECT_FALSE(watcher.isRunning());
}

TEST_F(ConfigFileWatcherTest, CustomDebounce) {
    auto ms = std::chrono::milliseconds(500);
    ConfigFileWatcher watcher(watch_dir_.string(), [] {}, ms);
    EXPECT_EQ(watcher.debounceInterval(), ms);
}

TEST_F(ConfigFileWatcherTest, StartAndStop) {
    ConfigFileWatcher watcher(watch_dir_.string(), [] {});
    bool ok = watcher.start();
    EXPECT_TRUE(ok);
    EXPECT_TRUE(watcher.isRunning());

    watcher.stop();
    EXPECT_FALSE(watcher.isRunning());
}

TEST_F(ConfigFileWatcherTest, StartIdempotent) {
    ConfigFileWatcher watcher(watch_dir_.string(), [] {});
    EXPECT_TRUE(watcher.start());
    EXPECT_TRUE(watcher.start()); // second call should return true without error
    EXPECT_TRUE(watcher.isRunning());
    watcher.stop();
}

TEST_F(ConfigFileWatcherTest, StopIdempotent) {
    ConfigFileWatcher watcher(watch_dir_.string(), [] {});
    watcher.start();
    watcher.stop();
    watcher.stop(); // second stop should be a no-op
    EXPECT_FALSE(watcher.isRunning());
}

TEST_F(ConfigFileWatcherTest, StopBeforeStart) {
    ConfigFileWatcher watcher(watch_dir_.string(), [] {});
    ASSERT_NO_THROW(watcher.stop()); // must not throw
    EXPECT_FALSE(watcher.isRunning());
}

TEST_F(ConfigFileWatcherTest, DestructorStopsThread) {
    {
        ConfigFileWatcher watcher(watch_dir_.string(), [] {});
        watcher.start();
        EXPECT_TRUE(watcher.isRunning());
        // Destructor called here – must join cleanly without crash/hang
    }
    SUCCEED();
}

TEST_F(ConfigFileWatcherTest, StartFailsOnNonexistentPath) {
    ConfigFileWatcher watcher("/nonexistent_path_xyz_123", [] {});
    bool ok = watcher.start();
    EXPECT_FALSE(ok);
    EXPECT_FALSE(watcher.isRunning());
}

// ── File-system event detection ───────────────────────────────────────────────

#if defined(__linux__) || defined(__APPLE__)

TEST_F(ConfigFileWatcherTest, DetectsYamlFileCreation) {
    std::atomic<int> fire_count{0};
    // Use a short debounce for tests
    ConfigFileWatcher watcher(watch_dir_.string(),
                               [&] { fire_count.fetch_add(1, std::memory_order_relaxed); },
                               std::chrono::milliseconds(50));
    ASSERT_TRUE(watcher.start());

    // Give the watcher thread a moment to set up inotify/kqueue
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    writeFile(watch_dir_ / "app.yaml");

    // Wait up to 2 s for the callback to fire
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (fire_count.load() == 0 && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    watcher.stop();
    EXPECT_GE(fire_count.load(), 1) << "Callback should fire after .yaml file creation";
}

TEST_F(ConfigFileWatcherTest, DetectsJsonFileModification) {
    // Pre-create the file so we're testing modification, not creation
    auto json_file = watch_dir_ / "settings.json";
    writeFile(json_file, "{}");

    std::atomic<int> fire_count{0};
    ConfigFileWatcher watcher(watch_dir_.string(),
                               [&] { fire_count.fetch_add(1, std::memory_order_relaxed); },
                               std::chrono::milliseconds(50));
    ASSERT_TRUE(watcher.start());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    writeFile(json_file, "{\"key\":\"value\"}");

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (fire_count.load() == 0 && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    watcher.stop();
    EXPECT_GE(fire_count.load(), 1) << "Callback should fire after .json file modification";
}

TEST_F(ConfigFileWatcherTest, IgnoresNonConfigFiles) {
    std::atomic<int> fire_count{0};
    ConfigFileWatcher watcher(watch_dir_.string(),
                               [&] { fire_count.fetch_add(1, std::memory_order_relaxed); },
                               std::chrono::milliseconds(50));
    ASSERT_TRUE(watcher.start());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Write a file with an unrelated extension
    writeFile(watch_dir_ / "notes.txt", "some text");

    // Wait briefly – no callback should fire
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    watcher.stop();
    EXPECT_EQ(fire_count.load(), 0)
        << "Callback must NOT fire for non-yaml/json file changes";
}

TEST_F(ConfigFileWatcherTest, DebounceCollapsesBurstIntoSingleCallback) {
    std::atomic<int> fire_count{0};
    // 200 ms debounce – fire many events in rapid succession and expect ≤ a
    // small number of callbacks (debounce collapses them).
    ConfigFileWatcher watcher(watch_dir_.string(),
                               [&] { fire_count.fetch_add(1, std::memory_order_relaxed); },
                               std::chrono::milliseconds(200));
    ASSERT_TRUE(watcher.start());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Write 5 files in rapid succession (< 200 ms total)
    for (int i = 0; i < 5; ++i) {
        writeFile(watch_dir_ / ("burst_" + std::to_string(i) + ".yaml"));
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Wait for the debounce window to expire and at most one more to fire
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    watcher.stop();
    // We expect the burst to produce ≤ 3 callbacks (ideally 1-2), definitely
    // not one per file.
    EXPECT_LE(fire_count.load(), 3)
        << "Debounce should collapse a rapid burst into ≤ 3 callbacks";
    EXPECT_GE(fire_count.load(), 1)
        << "At least one callback should fire after the burst";
}

#endif // __linux__ || __APPLE__

// ─────────────────────────────────────────────────────────────────────────────
// ConfigPathResolver::startHotReload / stopHotReload integration tests
// ─────────────────────────────────────────────────────────────────────────────

class HotReloadIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        ConfigPathResolver::resetMetrics();
        ConfigPathResolver::clearCache();
        watch_dir_ = std::filesystem::temp_directory_path() /
                     ("themis_hr_test_" + std::to_string(
                          std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(watch_dir_);
    }

    void TearDown() override {
        ConfigPathResolver::stopHotReload();
        std::filesystem::remove_all(watch_dir_);
    }

    std::filesystem::path watch_dir_;
};

TEST_F(HotReloadIntegrationTest, StartHotReloadReturnsTrue) {
    bool ok = ConfigPathResolver::startHotReload(watch_dir_.string(),
                                                 std::chrono::milliseconds(50));
    EXPECT_TRUE(ok) << "startHotReload should succeed for an accessible directory";
}

TEST_F(HotReloadIntegrationTest, StartHotReloadIdempotent) {
    EXPECT_TRUE(ConfigPathResolver::startHotReload(watch_dir_.string(),
                                                    std::chrono::milliseconds(50)));
    EXPECT_TRUE(ConfigPathResolver::startHotReload(watch_dir_.string(),
                                                    std::chrono::milliseconds(50)));
}

TEST_F(HotReloadIntegrationTest, StopHotReloadBeforeStart) {
    ASSERT_NO_THROW(ConfigPathResolver::stopHotReload());
}

TEST_F(HotReloadIntegrationTest, StartHotReloadFailsForNonexistentDir) {
    bool ok = ConfigPathResolver::startHotReload("/nonexistent_xyz_dir",
                                                 std::chrono::milliseconds(50));
    EXPECT_FALSE(ok);
}

#if defined(__linux__) || defined(__APPLE__)

TEST_F(HotReloadIntegrationTest, FileChangeTriggersCacheFlush) {
    // Seed the cache with a dummy entry by working from the test directory.
    // We use the global LRU cache via tryResolve(); create a file that matches
    // a known path so it resolves and gets cached.
    auto config_sub = watch_dir_ / "config";
    std::filesystem::create_directories(config_sub);
    writeFile(config_sub / "app.yaml");

    auto prev_cwd = std::filesystem::current_path();
    std::filesystem::current_path(watch_dir_);
    ConfigPathResolver::clearCache();
    ConfigPathResolver::tryResolve("config/app.yaml"); // prime cache
    std::filesystem::current_path(prev_cwd);

    auto stats_before = ConfigPathResolver::cacheStats();

    ASSERT_TRUE(ConfigPathResolver::startHotReload(watch_dir_.string(),
                                                    std::chrono::milliseconds(50)));
    // Give watcher time to initialise
    std::this_thread::sleep_for(std::chrono::milliseconds(60));

    // Trigger a file-system event
    writeFile(watch_dir_ / "config" / "app.yaml", "changed");

    // Wait for the watcher to fire and clear the cache
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (std::chrono::steady_clock::now() < deadline) {
        auto stats_after = ConfigPathResolver::cacheStats();
        if (stats_after.size == 0) break; // cache was cleared
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }

    auto stats_after = ConfigPathResolver::cacheStats();
    EXPECT_EQ(stats_after.size, 0u)
        << "File change should trigger a cache flush via the file watcher";

    ConfigPathResolver::stopHotReload();
}

#endif // __linux__ || __APPLE__

} // namespace test
} // namespace config
} // namespace themis
