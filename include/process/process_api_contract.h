// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors
#pragma once

/**
 * @file process_api_contract.h
 * @brief Frozen API contract for the ThemisDB process module.
 * @version 1.0.0
 *
 * @section purpose Purpose
 * The process module implements BPMN / CMMN workflow serialisation and
 * execution tracking.  This contract covers serialisation fidelity,
 * execution state machine transitions, and error handling.
 *
 * @section contracts API Contracts
 *
 * ### BpmnSerializer
 * - `serialize()` / `deserialize()` round-trip is lossless for all
 *   BPMN 2.0 constructs supported by ThemisDB.
 * - Unsupported constructs → PROC_UNSUPPORTED_ELEMENT (never silently dropped).
 *
 * ### CmmnSerializer
 * - Same round-trip guarantee for CMMN 1.1.
 *
 * ### WorkflowEngine
 * - State transitions follow the BPMN 2.0 token semantics.
 * - Transition to an invalid state → PROC_INVALID_TRANSITION.
 * - All transition events are logged via the configured audit sink.
 *
 * @section error_taxonomy Error Taxonomy
 * | Code                        | Meaning                                      |
 * |-----------------------------|----------------------------------------------|
 * | PROC_UNSUPPORTED_ELEMENT    | BPMN/CMMN element not supported              |
 * | PROC_INVALID_TRANSITION     | Invalid workflow state transition            |
 * | PROC_SERIALISE_FAILED       | Serialisation error (schema violation)       |
 * | PROC_DESERIALISE_FAILED     | Malformed input during deserialisation       |
 * | PROC_EXECUTION_TIMEOUT      | Workflow execution exceeded deadline         |
 * | PROC_MAX_DEPTH_EXCEEDED      | Max nesting depth exceeded in model          |
 * | PROC_MAX_ELEMENTS_EXCEEDED   | Max element count exceeded in model          |
 * | PROC_MAX_CONTEXT_EXCEEDED    | Max retrieval context size exceeded          |
 * | PROC_VALIDATION_FAILED       | Model validation failed                      |
 * | PROC_LINKING_FAILED          | Process linking operation failed             |
 *
 * @section threading Threading Guarantees
 * - `BpmnSerializer` / `CmmnSerializer` are stateless; fully thread-safe.
 * - `WorkflowEngine` is thread-safe for concurrent execution handles.
 *
 * @section contract_freeze Contract Freeze
 * Frozen for ThemisDB v2.x.
 */

#include <cstdint>
#include <string>

namespace themis::process {

enum class ProcError : int32_t {
    kSuccess             = 0,    ///< Operation completed successfully
    kUnsupportedElement  = 7600,
    kInvalidTransition   = 7601,
    kSerialiserFailed    = 7602,
    kDeserialiserFailed  = 7603,
    kExecutionTimeout    = 7604,
    kRetrievalFailed     = 7610,
    kLinkingStateInvalid = 7611,
    kMaxDepthExceeded    = 7605,  ///< Max nesting depth exceeded
    kMaxElementsExceeded = 7606,  ///< Max elements in model exceeded
    kMaxContextSizeExceeded = 7607, ///< Max retrieval context size exceeded
    kValidationFailed    = 7608,  ///< Model validation failed
    kLinkingFailed       = 7609,  ///< Process linking operation failed
};

} // namespace themis::process
