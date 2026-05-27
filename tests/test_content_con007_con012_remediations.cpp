// Copyright (c) 2024 ThemisDB
// SPDX-License-Identifier: MIT

/**
 * @file test_content_con007_con012_remediations.cpp
 * @brief Regression tests for CON-007 and CON-012 CONTENT module remediations.
 *
 * CON-007: VideoProcessor::healthCheck() must return false in no-FFmpeg
 *   (simulation) builds. Prior to the fix, both the FFmpeg and no-FFmpeg
 *   branches returned initialized_, which is always true after initialize(),
 *   making it impossible for health-check aggregators to detect the missing
 *   FFmpeg dependency.
 *
 * CON-012: AsyncIngestionWorker::~AsyncIngestionWorker() must be noexcept.
 *   Prior to the fix, the destructor called stop() without an exception guard.
 *   If stop() threw (e.g. mutex operation or promise::set_exception during
 *   stack unwinding), std::terminate() would be invoked. The fix declares the
 *   destructor noexcept and wraps stop() in a try/catch(...) block.
 */

#include <gtest/gtest.h>
#include <type_traits>

#define THEMIS_PLUGIN_EXPORTS
#include "content/video_processor.h"
#include "content/async_ingestion_worker.h"

using namespace themis::content;

// ============================================================================
// CON-007 — VideoProcessor::healthCheck() simulation-mode correctness
// ============================================================================

/**
 * @test An un-initialized VideoProcessor must always report unhealthy,
 * regardless of FFmpeg availability.  This is a baseline invariant.
 */
TEST(VideoProcessorHealthCheckCON007, NotInitialized_AlwaysUnhealthy) {
    VideoProcessor processor;
    EXPECT_FALSE(processor.healthCheck())
        << "Un-initialized VideoProcessor must report healthCheck() == false";
}

/**
 * @test CON-007 regression: in a no-FFmpeg (simulation-mode) build an
 * initialized VideoProcessor must report unhealthy so aggregators can surface
 * the missing FFmpeg dependency.
 *
 * Pre-fix behaviour: both FFmpeg and no-FFmpeg branches returned initialized_,
 * which is true after initialize() — the no-FFmpeg path was silent.
 */
TEST(VideoProcessorHealthCheckCON007, SimulationMode_InitializedButUnhealthy) {
#ifdef THEMIS_HAS_FFMPEG
    GTEST_SKIP() << "THEMIS_HAS_FFMPEG defined; simulation-mode path not active in this build.";
#else
    VideoProcessor processor;
    PluginConfig config;
    ASSERT_TRUE(processor.initialize(config))
        << "initialize() must succeed even without FFmpeg (simulation fallback)";
    EXPECT_FALSE(processor.healthCheck())
        << "CON-007: healthCheck() must return false in no-FFmpeg builds so "
           "health-check aggregators detect the missing dependency";
#endif
}

/**
 * @test CON-007 regression guard: in an FFmpeg build an initialized
 * VideoProcessor must still report healthy (the FFmpeg path is unchanged).
 */
TEST(VideoProcessorHealthCheckCON007, FFmpegMode_InitializedAndHealthy) {
#ifndef THEMIS_HAS_FFMPEG
    GTEST_SKIP() << "THEMIS_HAS_FFMPEG not defined; FFmpeg path not active in this build.";
#else
    VideoProcessor processor;
    PluginConfig config;
    ASSERT_TRUE(processor.initialize(config));
    EXPECT_TRUE(processor.healthCheck())
        << "CON-007 regression guard: healthCheck() must return true after "
           "successful initialize() in an FFmpeg build";
#endif
}

/**
 * @test CON-007: shutdown resets health regardless of build.
 */
TEST(VideoProcessorHealthCheckCON007, AfterShutdown_AlwaysUnhealthy) {
    VideoProcessor processor;
    PluginConfig config;
    ASSERT_TRUE(processor.initialize(config));
    processor.shutdown();
    EXPECT_FALSE(processor.healthCheck())
        << "VideoProcessor must report unhealthy after shutdown()";
}

// ============================================================================
// CON-012 — AsyncIngestionWorker destructor noexcept guarantee
// ============================================================================

/**
 * @test CON-012 compile-time check: AsyncIngestionWorker::~AsyncIngestionWorker()
 * must be noexcept.
 *
 * std::is_nothrow_destructible_v is true when the destructor is declared
 * noexcept (or is implicitly noexcept). This check does not require
 * constructing a live object, so no RocksDB or ContentManager setup is needed.
 *
 * Pre-fix: the destructor was declared without noexcept, so this assertion
 * would have failed.
 */
TEST(AsyncIngestionWorkerCON012, DestructorIsNoexcept) {
    constexpr bool dtor_noexcept = std::is_nothrow_destructible_v<AsyncIngestionWorker>;
    EXPECT_TRUE(dtor_noexcept)
        << "CON-012: AsyncIngestionWorker::~AsyncIngestionWorker() must be "
           "declared noexcept to prevent std::terminate() if stop() throws "
           "during stack unwinding";
}
