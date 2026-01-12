# Documentation TODO Verification Template

**Document**: [FILENAME]  
**Verification Date**: YYYY-MM-DD  
**Verifier**: [NAME]  
**Total Items**: X

---

## 📊 Status Breakdown

- ✅ **Implemented** (X items): Already done, needs doc update
- ❌ **Gap** (X items): Actual implementation needed
- 🔄 **Partial** (X items): Some implementation exists, needs completion
- 📝 **Doc-only** (X items): Documentation task only
- ⚠️ **Outdated** (X items): No longer relevant

**Verification Confidence**: [ ] High [ ] Medium [ ] Low

---

## 📋 Detailed Verification

### ✅ Implemented Items

List items that are already implemented in the codebase but not marked as complete in documentation.

#### Item 1: [Description]
- **Line Number**: [line]
- **Content**: `[TODO content]`
- **Evidence**: 
  - File: `src/path/to/file.cpp`
  - Function/Class: `ClassName::methodName()`
  - Tests: `tests/test_feature.cpp`
- **Action**: Update documentation to mark as complete
- **Confidence**: High/Medium/Low

---

### ❌ Gap Items

List items that represent actual implementation gaps.

#### Item 1: [Description]
- **Line Number**: [line]
- **Content**: `[TODO content]`
- **Verification**: 
  - Searched in: `src/`, `include/`, `tests/`, `plugins/`
  - No matching implementation found
  - No related tests found
- **Priority**: P0/P1/P2/P3
- **Complexity**: Simple/Medium/Complex
- **Action**: Create GitHub issue for implementation
- **Issue #**: [to be created]

---

### 🔄 Partial Implementation

List items with partial implementation that needs completion.

#### Item 1: [Description]
- **Line Number**: [line]
- **Content**: `[TODO content]`
- **Current Status**:
  - Partially implemented in: `src/path/to/file.cpp`
  - Missing components: [list]
  - Existing tests: `tests/test_partial.cpp`
- **Remaining Work**: 
  - [ ] Complete feature X
  - [ ] Add tests for Y
  - [ ] Update documentation
- **Action**: Create GitHub issue for completion

---

### 📝 Documentation-Only Items

List items that are purely documentation tasks.

#### Item 1: [Description]
- **Line Number**: [line]
- **Content**: `[TODO content]`
- **Type**: 
  - [ ] Write new documentation
  - [ ] Update existing documentation
  - [ ] Add examples/tutorials
  - [ ] Fix documentation errors
- **Action**: Assign documentation task

---

### ⚠️ Outdated Items

List items that are no longer relevant or have been superseded.

#### Item 1: [Description]
- **Line Number**: [line]
- **Content**: `[TODO content]`
- **Reason for Obsolescence**:
  - [ ] Already completed in previous version
  - [ ] Requirement changed
  - [ ] Alternative approach implemented
  - [ ] No longer needed
- **Evidence**: [explanation or git commit reference]
- **Action**: Remove from documentation

---

## 🎯 Recommendations

### Immediate Actions
1. **Update Documentation** (X items)
   - Mark implemented features as complete
   - Estimated effort: X hours

2. **Create GitHub Issues** (X items)
   - For verified implementation gaps
   - Priority: P0/P1 items first

3. **Remove Outdated Items** (X items)
   - Clean up documentation
   - Archive if needed for historical reference

### Medium-Term Actions
1. **Complete Partial Implementations** (X items)
   - Prioritize based on impact
   - Estimated effort: X days/weeks

2. **Documentation Updates** (X items)
   - Write missing documentation
   - Add examples and tutorials

### Long-Term Considerations
- Recurring patterns in gaps
- Areas needing architectural review
- Documentation maintenance strategy

---

## 📈 Statistics

### By Category
| Category | Implemented | Gap | Partial | Doc-only | Outdated | Total |
|----------|-------------|-----|---------|----------|----------|-------|
| Security | X | X | X | X | X | X |
| Performance | X | X | X | X | X | X |
| LLM/AI | X | X | X | X | X | X |
| Analytics | X | X | X | X | X | X |
| Enterprise | X | X | X | X | X | X |
| Testing | X | X | X | X | X | X |
| General | X | X | X | X | X | X |
| **Total** | **X** | **X** | **X** | **X** | **X** | **X** |

### By Priority (for Gaps)
- **P0 (Critical)**: X items
- **P1 (High)**: X items
- **P2 (Medium)**: X items
- **P3 (Low)**: X items

### Verification Quality
- **High Confidence**: X items (XX%)
- **Medium Confidence**: X items (XX%)
- **Low Confidence**: X items (XX%) - requires additional manual review

---

## 🔗 Related Issues

List of GitHub issues created from this verification:
- Issue #XX: [Description]
- Issue #XX: [Description]

---

## 📝 Notes

Additional observations, patterns, or recommendations from the verification process.

### Patterns Identified
- [Pattern 1]
- [Pattern 2]

### Recommendations for Future
- [Recommendation 1]
- [Recommendation 2]

---

## ✅ Sign-Off

- [ ] All items reviewed and categorized
- [ ] Evidence documented for each item
- [ ] GitHub issues created for gaps
- [ ] Documentation updates identified
- [ ] Stakeholders notified

**Verified by**: [NAME]  
**Date**: YYYY-MM-DD  
**Approved by**: [NAME]  
**Date**: YYYY-MM-DD

---

**Related Documents**:
- Main Meta-Issue: Issue #8 - Verify Documentation TODOs
- Verification Methodology: `scripts/verification/README.md`
- Automated Report: `*_verification.json`
