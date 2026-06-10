# AQL Query Hardening & Enhancement (v1.6.0) - Implementation Plan

## Task Overview
Implement 4 major enhancements to ThemisDB AQL query engine:
1. Post-generation AQL validation with injection detection
2. Thread leak elimination in timeout manager
3. Per-operation-type circuit breakers
4. Bounded conversation history with token budget

## Current State Analysis

### 1. Post-Generation AQL Validation
**Location**: `src/aql/llm_aql_handler.cpp` - `translateNLToAQL()` and variants
**Current**: Basic syntax highlighting but no post-generation rejection
**Needed**: Integrate `AQLQueryValidator` to reject malformed/injected queries based on configured mode

### 2. Thread Leak in LLMTimeoutManager
**Location**: `include/aql/llm_timeout_manager.h` lines 110-114, 213-217
**Status**: Already documented as FIXED with jthread cleanup approach
**Verification**: Code review shows proper RAII and cleanup patterns in place
**Action**: Verify implementation is correct in .cpp file

### 3. Per-Operation-Type Circuit Breakers
**Location**: `include/aql/llm_aql_handler.h` lines 145-206
**Current**: Config struct has per-operation circuit breaker configs defined
**Status**: Partially implemented - need to verify runtime enforcement
**Needed**: Ensure each operation type uses its dedicated circuit breaker

### 4. Bounded Conversation History
**Location**: `include/aql/aql_conversation_context.h`
**Current**: Config has `max_turns` and `max_history_tokens` fields defined
**Status**: Already documented in API
**Needed**: Verify Impl class enforces sliding window eviction

## Implementation Steps

1. **Verify Thread Leak Fix** - Check llm_timeout_manager.cpp
2. **Enhance Post-Generation Validation** - Add validation stage in translateNLToAQL()
3. **Verify Circuit Breakers** - Ensure per-operation enforcement
4. **Verify Conversation History** - Check sliding window implementation
5. **Add Tests** - Create comprehensive test suite
6. **Update Documentation** - Update ROADMAP/FUTURE_ENHANCEMENTS.md

## Files to Modify
- src/aql/llm_aql_handler.cpp
- src/aql/aql_conversation_context.cpp (if needed)
- src/aql/ROADMAP.md
- src/aql/FUTURE_ENHANCEMENTS.md
- tests/ (new test files or existing ones)
