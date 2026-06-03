# Phase 3 W1-R Remediation Plan for ThemisDB Replication Module

## Overview
Implementing exception safety, timeout protection, and edge case handling for:
- src/replication/replication_manager.cpp (WAL I/O, replication streams)
- src/replication/conflict_resolution.cpp (resolver chain, metadata enrichment)
- src/replication/logical_replication.cpp (edge cases, validation)

## Batch A: File I/O Timeout Protection

### Target: src/replication/replication_manager.cpp

#### Changes:
1. **WALEntry::deserialize (line ~101)**: Add buffer validation
2. **WALManager::readFrom (line ~320)**: Add timeout wrapper for file reads
3. **ReplicationStream::streamLoop (line ~750)**: Add timeout guards for WAL reads
4. **Follower persistence**: Add timeout for state persistence operations

#### Implementation:
- Create TimeoutWrapper utility function using std::future with wait_for()
- Add timeout values as constants (configurable)
- Document timeout behavior and fallback strategy
- Add timeout exceeded error logging

## Batch B: Exception Safety in Conflict Resolution

### Target: src/replication/conflict_resolution.cpp

#### Changes:
1. **JSON parsing**: Wrap in try-catch blocks
2. **Vector operations**: RAII-based state management for merges
3. **Recursive merges**: Transaction-like semantics (all-or-nothing)
4. **Error handling**: Proper rollback on exceptions

## Batch C: Resolver Chain Error Handling

### Target: src/replication/replication_manager.cpp (resolveConflict functions)

#### Changes:
1. **Empty conflict set validation**: Fail-closed behavior
2. **Invalid JSON validation**: Check before processing
3. **Result types**: Return structured errors (Result<T, Error>)
4. **Resolution failure propagation**: Clear error codes

## Batch D: Edge Case Guards

### Target: All files

#### Changes:
1. **Vector bounds checking**: Before index operations
2. **String boundary operations**: At buffer edges
3. **Numeric validation**: NaN/infinity checks
4. **Container emptiness**: Before accessing front/back

---
Implementation Status: READY TO EXECUTE
