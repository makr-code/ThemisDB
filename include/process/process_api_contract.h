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
    kUnsupportedElement  = 7600,
    kInvalidTransition   = 7601,
    kSerialiserFailed    = 7602,
    kDeserialiserFailed  = 7603,
    kExecutionTimeout    = 7604,
};

} // namespace themis::process
