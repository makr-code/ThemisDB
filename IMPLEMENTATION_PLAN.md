# Stub Remediation Implementation Plan

## Stub #303: LLM Model Storage Enumeration

### Status
- `listModels()` already uses `scanPrefix()` which is implemented
- Need to add `prefixIterator()` method for alternative enumeration path
- Need to verify the implementation works correctly

### Changes Required
1. **RocksDBWrapper**
   - Add `prefixIterator()` method returning SafeIterator at prefix start
   - Properly handle boundary conditions

2. **LLMModelStorage**
   - Verify `listModels()` works with current `scanPrefix()` implementation
   - Ensure filtering logic is correct
   - No changes needed if scanPrefix works correctly

3. **Tests**
   - Add test for `listModels()` with multiple models
   - Add test for filtering

## Stub #304: LoRA Feedback Graph Linking

### Status
- `createGraphLink()` and `removeGraphLink()` exist but don't use callbacks
- Header defines callback injection methods but implementation doesn't use them

### Changes Required
1. **FeedbackStorageService**
   - Update `createGraphLink()` to check for callback and use it
   - Update `removeGraphLink()` to check for callback and use it
   - Keep fallback to direct graph index for backward compatibility
   - Improve error handling and logging

2. **Tests**
   - Add test for callback-based graph linking
   - Add test for error cases

## Files to Modify
- `include/storage/rocksdb_wrapper.h` - Add prefixIterator() declaration
- `src/storage/rocksdb_wrapper.cpp` - Implement prefixIterator()
- `src/llm/lora_framework/lora_feedback_storage.cpp` - Wire callbacks
- `tests/` - Add new tests

## Build & Test Strategy
- Build only affected tests to verify changes
- Ensure no regression in existing tests
