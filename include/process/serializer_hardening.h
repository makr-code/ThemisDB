// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file serializer_hardening.h
 * @brief Hardening utilities for process model serializers (BPMN, EPK, CMMN, DMN).
 * @version 1.0.0
 *
 * Provides common validation, bounds checking, and error handling for all
 * process model format parsers. Implements Phase 3 edge-case hardening.
 */

#pragma once

#include "process/process_common.h"
#include "process/process_diagnostics.h"
#include <optional>
#include <string_view>
#include <vector>

namespace themis::process {

/**
 * @brief Validation result for serializer input pre-checks.
 */
struct SerializerValidationResult {
    bool ok{false};
    std::string error_message;
    DiagnosticIncidentType incident_type{DiagnosticIncidentType::MALFORMED_INPUT_INCIDENT};

    static SerializerValidationResult success() {
        SerializerValidationResult r;
        r.ok = true;
        return r;
    }

    static SerializerValidationResult failure(
        std::string_view msg,
        DiagnosticIncidentType incident = DiagnosticIncidentType::MALFORMED_INPUT_INCIDENT
    ) {
        SerializerValidationResult r;
        r.ok = false;
        r.error_message = std::string(msg);
        r.incident_type = incident;
        return r;
    }
};

/**
 * @brief Unified input validation for all serializers.
 *
 * Performs the following checks:
 * - Input is not empty
 * - Input size does not exceed kMaxModelInputBytes
 * - Input is not truncated (ends with valid structure markers)
 * - Input has valid UTF-8 encoding (for text formats)
 * - Input contains recognizable format markers
 */
class SerializerInputValidator {
public:
    /**
     * @brief Validate generic serializer input before parsing.
     *
     * @param input The input data to validate.
     * @param format_name Human-readable format name (e.g., "BPMN 2.0", "EPK").
     * @return Validation result with error details if validation fails.
     */
    static [[nodiscard]] SerializerValidationResult validateInput(
        std::string_view input,
        std::string_view format_name = "Process Model"
    );

    /**
     * @brief Check for truncation indicators in XML-based formats.
     *
     * @param xml The XML input to check.
     * @return true if the XML appears to be truncated (missing closing tags),
     *         false if structure appears complete.
     */
    static [[nodiscard]] bool isXmlTruncated(std::string_view xml);

    /**
     * @brief Validate that a string is well-formed UTF-8.
     *
     * @param s The string to check.
     * @return true if the string is valid UTF-8, false otherwise.
     */
    static [[nodiscard]] bool isValidUtf8(std::string_view s);

    /**
     * @brief Extract format version from a BPMN/CMMN/DMN document header.
     *
     * @param xml The XML document.
     * @return Version string (e.g., "2.0"), or empty if not found.
     */
    static [[nodiscard]] std::string extractXmlVersion(std::string_view xml);

    /**
     * @brief Count opening and closing XML tags to detect truncation.
     *
     * @param xml The XML input.
     * @return Tuple of (opening_count, closing_count).
     */
    static [[nodiscard]] std::pair<int32_t, int32_t> countXmlTags(std::string_view xml);

private:
    // Private implementation helpers
    static [[nodiscard]] bool isAsciiControlChar(unsigned char c);
    static [[nodiscard]] bool isValidUtf8Sequence(
        const unsigned char* data,
        size_t remaining_bytes,
        size_t& sequence_length
    );
};

/**
 * @brief Parser state tracking for bounded resource consumption.
 *
 * Tracks nesting depth, element count, and elapsed time to prevent
 * resource exhaustion attacks and stack overflows.
 */
class ParserStateTracker {
public:
    /**
     * @brief Create a new parser state tracker.
     *
     * @param max_depth Maximum allowed nesting depth (kMaxModelNestingDepth).
     * @param max_elements Maximum allowed total elements (kMaxModelElements).
     * @param timeout_ms Maximum operation duration in milliseconds.
     */
    ParserStateTracker(
        int32_t max_depth = kMaxModelNestingDepth,
        int32_t max_elements = kMaxModelElements,
        int64_t timeout_ms = kMaxOperationTimeoutMs
    );

    /**
     * @brief Signal entry into a nested scope (sub-process, choice, etc.).
     *
     * @return true if within depth limits, false if max depth exceeded.
     */
    [[nodiscard]] bool enterScope();

    /**
     * @brief Signal exit from a nested scope.
     *
     * @return true if scope was properly nested, false on underflow.
     */
    [[nodiscard]] bool exitScope();

    /**
     * @brief Record the creation of a new element (node, edge, etc.).
     *
     * @return true if within element limits, false if limit exceeded.
     */
    [[nodiscard]] bool recordElement();

    /**
     * @brief Check if the operation has exceeded the timeout window.
     *
     * @return true if operation has timed out, false otherwise.
     */
    [[nodiscard]] bool hasTimedOut() const;

    /**
     * @brief Get the current nesting depth.
     *
     * @return Current depth (0 = root).
     */
    [[nodiscard]] int32_t getCurrentDepth() const { return current_depth_; }

    /**
     * @brief Get the total elements recorded so far.
     *
     * @return Total element count.
     */
    [[nodiscard]] int32_t getElementCount() const { return element_count_; }

    /**
     * @brief Get elapsed time in milliseconds since creation.
     *
     * @return Milliseconds elapsed.
     */
    [[nodiscard]] int64_t getElapsedMs() const;

    /**
     * @brief Get a diagnostic message describing the current state.
     *
     * @return Human-readable state description.
     */
    [[nodiscard]] std::string getDiagnosticMessage() const;

private:
    int32_t max_depth_;
    int32_t max_elements_;
    int64_t timeout_ms_;
    int32_t current_depth_{0};
    int32_t element_count_{0};
    int64_t start_time_ms_{0};
};

/**
 * @brief Format-specific validation helpers.
 */
class BpmnValidator {
public:
    /**
     * @brief Validate BPMN-specific constraints.
     *
     * - Process ID must be non-empty
     * - Element IDs must be unique
     * - Source/target references must be resolvable
     * - No cycles in flow (except in loop conditions)
     *
     * @param elements List of BPMN element descriptions.
     * @return Validation result.
     */
    static [[nodiscard]] SerializerValidationResult validateBpmnConstraints(
        const std::vector<std::string>& element_ids
    );
};

class EpkValidator {
public:
    /**
     * @brief Validate EPK-specific constraints.
     *
     * - Events, functions, and connectors properly sequenced
     * - No duplicate node IDs
     * - Edges reference existing nodes
     *
     * @param nodes List of EPK node IDs.
     * @param edges List of edge source→target pairs.
     * @return Validation result.
     */
    static [[nodiscard]] SerializerValidationResult validateEpkConstraints(
        const std::vector<std::string>& nodes,
        const std::vector<std::pair<std::string, std::string>>& edges
    );
};

class CmmnValidator {
public:
    /**
     * @brief Validate CMMN-specific constraints.
     *
     * - Case model ID is non-empty
     * - Case plan items form a directed acyclic graph (or valid loop structure)
     *
     * @param case_id The case model ID.
     * @param item_ids List of case plan item IDs.
     * @return Validation result.
     */
    static [[nodiscard]] SerializerValidationResult validateCmmnConstraints(
        std::string_view case_id,
        const std::vector<std::string>& item_ids
    );
};

class DmnValidator {
public:
    /**
     * @brief Validate DMN-specific constraints.
     *
     * - Decision table has at least one rule
     * - Input/output clauses are well-formed
     * - Hit policy is recognized
     *
     * @param decision_id The decision model ID.
     * @param rule_count Number of decision rules.
     * @return Validation result.
     */
    static [[nodiscard]] SerializerValidationResult validateDmnConstraints(
        std::string_view decision_id,
        int32_t rule_count
    );
};

} // namespace themis::process
