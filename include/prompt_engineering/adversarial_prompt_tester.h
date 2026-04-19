/**
 * @file adversarial_prompt_tester.h
 * @brief Adversarial prompt testing framework for LLM safety regression.
 *
 * ## Purpose
 *
 * `SimpleAdversarialTester` runs a configurable battery of adversarial test
 * cases against the `PromptInjectionDetector` and produces a structured
 * `AdversarialTestReport`.  The framework is designed for CI regression
 * pipelines: every test case asserts whether a given attack payload should
 * be **blocked** by the injection detector, and the report tracks pass/fail
 * per case as well as an aggregate block rate.
 *
 * ## Attack categories (OWASP LLM Top 10 aligned)
 *
 * | Category            | Description                                             |
 * |---------------------|---------------------------------------------------------|
 * | `JAILBREAK`         | Attempts to make the model ignore safety constraints.   |
 * | `ROLE_OVERRIDE`     | "Act as DAN / pretend you are…" instructions.           |
 * | `INDIRECT_INJECTION`| Malicious content injected via retrieved context/tools. |
 * | `PROMPT_LEAKING`    | Requests to reveal the system prompt verbatim.          |
 * | `DATA_EXTRACTION`   | Attempts to extract training data or PII.               |
 *
 * ## Usage
 * ```cpp
 * SimpleAdversarialTester tester;
 * tester.loadDefaultTestSuite();           // OWASP-aligned built-in cases
 *
 * // Optionally add custom cases:
 * tester.addTestCase({"CUSTOM-01", AttackCategory::JAILBREAK,
 *                     "My custom payload", true});
 *
 * auto report = tester.runAll();
 * std::cout << "Block rate: " << report.blockRate() * 100 << " %\n";
 * if (!report.passed(0.95)) { // fail CI
 * }
 * ```
 *
 * Copyright (c) 2026 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <functional>
#include <string>
#include <vector>

namespace themis {
namespace prompt_engineering {

// ── Attack category ───────────────────────────────────────────────────────────

/**
 * @brief High-level category of an adversarial test case.
 *
 * Aligned with the OWASP LLM Top 10 (2023) vulnerability taxonomy.
 */
enum class AttackCategory {
    JAILBREAK,          ///< Bypass safety/alignment constraints.
    ROLE_OVERRIDE,      ///< Impersonation / "act-as" role hijack.
    INDIRECT_INJECTION, ///< Payload embedded in retrieved tool/context data.
    PROMPT_LEAKING,     ///< Exfiltrate system or developer instructions.
    DATA_EXTRACTION,    ///< Extract training data, PII, or secrets.
};

/// Convert @p category to a human-readable string (e.g., "JAILBREAK").
const char* attackCategoryName(AttackCategory category) noexcept;

// ── Test case ─────────────────────────────────────────────────────────────────

/**
 * @brief A single adversarial test case.
 */
struct AdversarialTestCase {
    std::string    id;               ///< Stable identifier, e.g. "APT-01".
    AttackCategory category;         ///< Attack category.
    std::string    payload;          ///< The adversarial prompt text.
    bool           expected_blocked; ///< `true` if the payload should be detected.
};

// ── Per-case result ───────────────────────────────────────────────────────────

/**
 * @brief Result of running a single adversarial test case.
 */
struct AdversarialTestResult {
    AdversarialTestCase test_case;           ///< The case that was run.
    bool                blocked       = false; ///< Whether the detector blocked it.
    double              detection_ms  = 0.0;   ///< Time taken to detect (ms).

    /**
     * @brief Returns `true` iff the outcome matches `expected_blocked`.
     *
     * A case passes when:
     *  - `expected_blocked == true`  AND  `blocked == true`  (true positive), or
     *  - `expected_blocked == false` AND  `blocked == false` (true negative).
     */
    bool passed() const noexcept {
        return blocked == test_case.expected_blocked;
    }
};

// ── Aggregate report ──────────────────────────────────────────────────────────

/**
 * @brief Aggregated result of a full adversarial test run.
 */
struct AdversarialTestReport {
    std::vector<AdversarialTestResult> results;        ///< One entry per test case.
    size_t                             total   = 0;    ///< Total cases run.
    size_t                             blocked_count = 0; ///< Cases where blocked == true.
    size_t                             passed_count  = 0; ///< Cases where result.passed().

    /**
     * @brief Fraction of cases where the detector correctly blocked/allowed.
     *
     * Returns 1.0 if no cases were run.
     */
    double passRate() const noexcept {
        return (total == 0) ? 1.0
                            : static_cast<double>(passed_count) /
                                  static_cast<double>(total);
    }

    /**
     * @brief Fraction of cases that were blocked (regardless of expected outcome).
     */
    double blockRate() const noexcept {
        return (total == 0) ? 0.0
                            : static_cast<double>(blocked_count) /
                                  static_cast<double>(total);
    }

    /**
     * @brief Returns `true` iff `passRate() >= min_pass_rate`.
     *
     * @param min_pass_rate  Minimum acceptable pass rate (0.0–1.0).
     */
    bool passed(double min_pass_rate = 1.0) const noexcept {
        return passRate() >= min_pass_rate;
    }
};

// ── Abstract interface ────────────────────────────────────────────────────────

/**
 * @brief Abstract interface for adversarial prompt testing.
 */
class IAdversarialPromptTester {
public:
    virtual ~IAdversarialPromptTester() = default;

    /**
     * @brief Add a test case to the active suite.
     *
     * @throws std::invalid_argument if a case with the same `id` already exists.
     */
    virtual void addTestCase(AdversarialTestCase test_case) = 0;

    /**
     * @brief Populate the suite with the built-in OWASP-aligned default cases.
     *
     * Safe to call even if custom cases have already been added; duplicates
     * (by id) are silently skipped.
     */
    virtual void loadDefaultTestSuite() = 0;

    /**
     * @brief Run all test cases and return an aggregated report.
     */
    virtual AdversarialTestReport runAll() const = 0;

    /**
     * @brief Run a single test case by id.
     *
     * @throws std::out_of_range if no case with @p id is registered.
     */
    virtual AdversarialTestResult runOne(const std::string& id) const = 0;

    /**
     * @brief Return all registered test cases.
     */
    virtual std::vector<AdversarialTestCase> testCases() const = 0;
};

// ── Concrete implementation ───────────────────────────────────────────────────

/**
 * @brief Injection-detector–backed adversarial prompt tester.
 *
 * Detection is implemented via case-insensitive substring matching against a
 * curated blocklist derived from the OWASP LLM Top 10 patterns (the same
 * patterns used by `PromptInjectionDetector`).  Callers can inject a custom
 * detector function via `setDetectorFn()` to integrate with the full
 * `PromptInjectionDetector` class from this module.
 *
 * ### Custom detector injection
 * ```cpp
 * PromptInjectionDetector real_detector;
 * SimpleAdversarialTester tester;
 * tester.setDetectorFn([&real_detector](const std::string& payload) {
 *     return real_detector.detect(payload).is_injection;
 * });
 * tester.loadDefaultTestSuite();
 * auto report = tester.runAll();
 * ```
 */
class SimpleAdversarialTester final : public IAdversarialPromptTester {
public:
    /// Detector function type: maps payload → true iff injection detected.
    using DetectorFn = std::function<bool(const std::string& payload)>;

    SimpleAdversarialTester();

    /**
     * @brief Replace the built-in blocklist detector with a custom function.
     *
     * Must be called before `runAll()` / `runOne()`.
     */
    void setDetectorFn(DetectorFn fn);

    // IAdversarialPromptTester interface
    void addTestCase(AdversarialTestCase test_case) override;
    void loadDefaultTestSuite()                     override;
    AdversarialTestReport runAll()                const override;
    AdversarialTestResult runOne(const std::string& id) const override;
    std::vector<AdversarialTestCase> testCases()  const override;

private:
    /// Default detector: case-insensitive substring search against blocklist.
    static bool defaultDetect(const std::string& payload);

    DetectorFn                       detector_fn_;
    std::vector<AdversarialTestCase> cases_;
};

} // namespace prompt_engineering
} // namespace themis
