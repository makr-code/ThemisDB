# Workflow Trigger Emoji Implementation Guide

**Status:** Complete mapping generated for all 73 workflows  
**Date:** 2026-09-23  
**Implementation Phase:** Next Sprint (Phase 4)

## Objective

Add Unicode emoji markers to all workflow `name:` fields to visually indicate **how each workflow is triggered**.

## Emoji System

```
PRIMARY TRIGGERS:
📅 = Schedule-based (cron)
🔄 = Event-driven (push/pull_request/release)
⏱️ = Manual dispatch (workflow_dispatch)
🔗 = Reusable (workflow_call)
🚀 = Release/Publication
❓ = Composite/Approval gates

SECONDARY CHARACTERISTICS:
🎯 = Governance/Gates
🧪 = Testing/Benchmark
🔧 = Maintenance/Infrastructure
📝 = Documentation
🛡️ = Security
```

## Implementation Method

### Option A: Batch Script (Recommended)
Execute the Python script in `.github/scripts/apply_workflow_emojis.py` (to be created):

```bash
python3 .github/scripts/apply_workflow_emojis.py --mapping-file .github/WORKFLOW_EMOJI_MAPPING.json --apply
```

This will:
1. Read `.github/WORKFLOW_EMOJI_MAPPING.json` (pre-generated)
2. Update all 73 workflow `name:` fields in-place
3. Validate syntax with `actionlint`
4. Report changes via git diff

### Option B: Manual Review + Per-Workflow Application
1. Review `.github/WORKFLOW_EMOJI_MAPPING.json` for accuracy
2. Apply emojis one workflow at a time per team request
3. Validate after each batch (e.g., 10 workflows per PR)

## Mapping Files

Both files have been generated and persisted to `.github/`:

1. **`.github/WORKFLOW_EMOJI_MAPPING.json`** (29 KB)
   - Complete JSON structure for all 73 workflows
   - Fields: `filename`, `current_name`, `triggers`, `primary_trigger`, `secondary_tags`, `recommended_emoji`, `new_name`
   - Machine-readable format for scripting

2. **`.github/WORKFLOW_EMOJI_QUICK_REFERENCE.txt`** (15 KB)
   - Human-readable summary organized by trigger type
   - Trigger distribution statistics (📅 19.2%, 🔄 19.2%, ⏱️ 32.9%, etc.)
   - Quick lookup tables for each category

## Example Transformations

| Current Name | New Name | Trigger |
|---|---|---|
| `Build: Mainline` | `🔄 Build: Mainline` | push (post-merge) |
| `Security: CodeQL` | `📅🛡️ Security: CodeQL` | schedule + manual |
| `Release: Mainline` | `🚀 Release: Mainline` | release (GitHub) |
| `Maintenance: Issues` | `📅🔧 Maintenance: Issues` | schedule (weekly) |
| `reusable-cmake-build.yml` | `🔗 reusable-cmake-build.yml` | workflow_call only |

## Trigger Distribution

**Total: 73 workflows**

- **📅 Schedule (14)**: 19.2% — Cron-based automation (nightly, weekly)
- **🔄 Push (14)**: 19.2% — Post-merge CI/CD feedback
- **⏱️ Manual Dispatch (24)**: 32.9% — On-demand (heavy builds, security scans)
- **🔗 Reusable (1)**: 1.4% — Workflow components
- **❓ Composite/Approval (17)**: 23.3% — Reusable gates, approval workflows
- **🔄 Pull Request (3)**: 4.1% — PR-specific validation

## Next Steps

1. **Sprint 10 (Recommended):**
   - Review `.github/WORKFLOW_EMOJI_MAPPING.json` for accuracy
   - Create `apply-emoji-names.yml` automation workflow
   - Execute batch rename for all 73 workflows
   - Validate with `actionlint` (already passing)
   - Update WORKFLOW_REGISTRY.md with emoji annotations

2. **Documentation Updates:**
   - Add emoji legend to `.github/WORKFLOW_GUIDELINES.md`
   - Update GitHub Actions dashboard documentation
   - Link emoji system in contributor onboarding guide

3. **CI/CD Integration:**
   - Add emoji compliance check to workflow validation (must match primary trigger type)
   - Automate emoji additions on new workflow creation

## Validation Checklist

- [ ] All 73 workflows in `.github/workflows/` are listed in mapping
- [ ] Each workflow has a valid primary trigger type
- [ ] Emoji combination matches trigger semantics
- [ ] No conflicting emojis (e.g., 🔄 and ⏱️ both when only one applies)
- [ ] Recommended `new_name` matches emoji + original name format
- [ ] Batch rename script tested on sample workflows
- [ ] `actionlint` passes after all renames
- [ ] WORKFLOW_REGISTRY.md updated with emoji descriptions

## References

- **Mapping:** `.github/WORKFLOW_EMOJI_MAPPING.json`
- **Quick Reference:** `.github/WORKFLOW_EMOJI_QUICK_REFERENCE.txt`
- **Guidelines:** `.github/WORKFLOW_GUIDELINES.md`
- **Registry:** `.github/WORKFLOW_REGISTRY.md`
