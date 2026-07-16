# Git Flow Workflow - Visual Guide

## Branch Structure Overview

```
                     Production Releases
                            ↓
    ═══════════════════════════════════════════════
    ║                  main                       ║  ← Always stable
    ║  (v1.3.0)  (v1.3.1)  (v1.4.0)             ║  ← Tagged releases
    ═══════════════════════════════════════════════
         ↑           ↑          ↑
         │           │          └─── release/1.4.0 ────┐
         │           └─── hotfix/1.3.1                 │
         │                                              │
    ═══════════════════════════════════════════════════│══════
    ║                 develop                          │     ║
    ║  ├─ Latest completed features                    ↓     ║
    ║  └─ Integration branch                                 ║
    ═══════════════════════════════════════════════════════
         ↑       ↑       ↑
         │       │       └─── bugfix/query-timeout
         │       └─── feature/llm-streaming
         └─── feature/vector-search
```

## Complete Development Lifecycle

```
┌─────────────────────────────────────────────────────────────┐
│  1. New Feature Development                                 │
└─────────────────────────────────────────────────────────────┘

    develop (protected)
        │
        ├─ create branch
        ↓
    feature/vector-search
        │
        ├─ commit: "feat: Add vector search"
        ├─ commit: "test: Add vector tests"
        ├─ commit: "docs: Update API docs"
        │
        ├─ PR Review
        ├─ CI Checks
        │
        └─ squash and merge (recommended)
        ↓
    develop (updated)


┌─────────────────────────────────────────────────────────────┐
│  2. Release Process                                         │
└─────────────────────────────────────────────────────────────┘

    develop (with features A, B, C)
        │
        ├─ create release branch
        ↓
    release/1.4.0
        │
        ├─ Version bump: 1.3.3 → 1.4.0
        ├─ Update CHANGELOG.md
        ├─ Final testing
        ├─ Bug fixes (critical only)
        │
        ├─ merge to main ──────────┐
        │                          ↓
        │                      main (v1.4.0)
        │                          │
        │                          ├─ Tag: v1.4.0
        │                          ├─ Deploy to production
        │                          │
        └─ merge back ─────────────┘
        ↓
    develop (includes release fixes)


┌─────────────────────────────────────────────────────────────┐
│  3. Hotfix (Production Bug)                                 │
└─────────────────────────────────────────────────────────────┘

    main (v1.4.0) ← Production issue!
        │
        ├─ create hotfix branch
        ↓
    hotfix/1.4.1-critical-bug
        │
        ├─ Fix bug
        ├─ Version bump: 1.4.0 → 1.4.1
        ├─ Test fix
        │
        ├─ merge to main ──────────┐
        │                          ↓
        │                      main (v1.4.1)
        │                          │
        │                          ├─ Tag: v1.4.1
        │                          ├─ Deploy hotfix
        │                          │
        └─ also merge to develop ──┘
        ↓
    develop (includes hotfix)
```

## Pull Request Flow

```
┌─────────────────────────────────────────────────────────────┐
│  Feature PR: feature/xyz → develop                         │
└─────────────────────────────────────────────────────────────┘

Developer                      GitHub                    Maintainer
    │                             │                          │
    ├─ Create PR ────────────────→│                          │
    │                             ├─ Trigger CI              │
    │                             │   ├─ Build               │
    │                             │   ├─ Tests               │
    │                             │   ├─ Linting             │
    │                             │   └─ Security Scan       │
    │                             │                          │
    │                             ├─ Request Review ────────→│
    │                             │                          │
    │                             │                    ├─ Review Code
    │                             │                    ├─ Add Comments
    │                             │                    │
    │←─────────────────────────── │←─── Request Changes ────┤
    │                             │                          │
    ├─ Address Feedback ─────────→│                          │
    ├─ Push Updates ─────────────→│                          │
    │                             ├─ Re-run CI               │
    │                             │                          │
    │                             ├─ Request Re-review ─────→│
    │                             │                          │
    │                             │                    ├─ Approve
    │                             │                          │
    │                             │←──── Merge ──────────────┤
    │                             │                          │
    │                             ├─ Merge to develop        │
    │                             ├─ Delete branch           │
    │                             │                          │
    │←─────────── Notify ─────────┤                          │
    │                             │                          │
```

## Branch Lifecycle Timeline

```
Week 1                Week 2                Week 3                Week 4
│                     │                     │                     │
├─ Feature Dev ───────┼─ Complete ─────────┤                     │
│  feature/A          │  Merge to develop   │                     │
│                     │                     │                     │
├─ Feature Dev ───────┼──────────────────── ┼─ Complete ─────────┤
│  feature/B          │                     │  Merge to develop   │
│                     │                     │                     │
│                     │                     │                     │
│                     │                     ├─ Release Branch ────┤
│                     │                     │  release/1.4.0      │
│                     │                     │  - Version bump     │
│                     │                     │  - Testing          │
│                     │                     │  - Bug fixes        │
│                     │                     │                     │
│                     │                     │                     ├─ Release
│                     │                     │                     │  Tag v1.4.0
│                     │                     │                     │  Deploy
│                     │                     │                     │
│                     │                     │                     ├─ Start Next
│                     │                     │                     │  Cycle
```

## Real World Example

### Scenario: Adding New Feature + Bug Fix + Release

```
Time: Monday Morning
─────────────────────────────────────────────────────────
develop (v1.3.3)
    │
    ├─ Alice creates feature/vector-search
    │  └─ Implements vector similarity search
    │
    ├─ Bob creates feature/llm-streaming
    │  └─ Adds streaming support for LLM responses
    │
    └─ Charlie creates bugfix/memory-leak
       └─ Fixes memory leak in connection pool


Time: Tuesday
─────────────────────────────────────────────────────────
develop
    │
    ├─ Alice: PR ready, CI passing, awaiting review
    │
    ├─ Bob: Still implementing
    │
    └─ Charlie: PR merged! ✓


Time: Wednesday
─────────────────────────────────────────────────────────
develop (now includes Charlie's fix)
    │
    ├─ Alice: PR approved and merged! ✓
    │  └─ develop now includes vector search
    │
    └─ Bob: PR in review


Time: Thursday
─────────────────────────────────────────────────────────
develop (includes Alice + Charlie's changes)
    │
    ├─ Bob: PR approved and merged! ✓
    │  └─ develop now includes all 3 changes
    │
    └─ Team decides: Ready for release!


Time: Friday - Release Day
─────────────────────────────────────────────────────────
develop (v1.3.3 + new features)
    │
    ├─ Create release/1.4.0
    │  ├─ Bump version to 1.4.0
    │  ├─ Update CHANGELOG
    │  ├─ Run full test suite
    │  └─ Create release notes
    │
    ├─ Merge to main
    │  ├─ Tag v1.4.0
    │  └─ Trigger deployment
    │
    └─ Merge back to develop


Time: Next Monday - New Cycle
─────────────────────────────────────────────────────────
main (v1.4.0) ← Production
    │
develop (v1.4.0) ← Ready for next features
    │
    └─ New features start here...


Time: Monday Afternoon - OOPS! Critical Bug!
─────────────────────────────────────────────────────────
main (v1.4.0) ← Bug in production!
    │
    ├─ Create hotfix/1.4.1-security-fix
    │  ├─ Fix security vulnerability
    │  ├─ Bump to 1.4.1
    │  └─ Emergency testing
    │
    ├─ Merge to main
    │  ├─ Tag v1.4.1
    │  └─ Deploy hotfix ASAP
    │
    └─ Also merge to develop
       └─ Keep develop in sync

Result:
main: v1.4.1 (stable)
develop: v1.4.1 + ongoing features
```

## Decision Tree: Which Branch?

```
                    Starting new work?
                           │
                ┌──────────┴──────────┐
                │                     │
           New Feature?          Fixing Bug?
                │                     │
         ┌──────┴──────┐       ┌─────┴─────┐
         │             │       │           │
    In develop?   In production?   In develop?   In production?
         │             │       │           │
         ↓             ↓       ↓           ↓
  feature/xyz    hotfix/x.y.z  bugfix/abc  hotfix/x.y.z
  from develop   from main     from develop from main
         │             │       │           │
         ↓             ↓       ↓           ↓
  → develop      → main+develop → develop  → main+develop


                    Creating PR?
                           │
                ┌──────────┴──────────┐
                │                     │
          Feature/Bugfix?        Hotfix?
                │                     │
                ↓                     ↓
          Target: develop       Target: main
          Reviewers: 1          Reviewers: 1+
          Can wait              Urgent review
```

## State Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                     Branch States                           │
└─────────────────────────────────────────────────────────────┘

┌─────────┐
│ develop │ ◀──────────────── Base for features
└────┬────┘                    Always exists
     │                         Protected
     │ create feature
     ↓
┌──────────────┐
│ feature/xyz  │ ◀───────────── Active development
└──────┬───────┘                Can be many
       │                        Short-lived (1-2 weeks)
       │ complete & review
       ↓
┌─────────────┐
│ PR Review   │ ◀──────────────  Code review state
└──────┬──────┘                  CI checks run
       │                         Approval required
       │ approved & merged
       ↓
┌─────────┐
│ develop │ ◀──────────────── Updated with feature
└────┬────┘                    Delete feature branch
     │
     │ ready for release
     ↓
┌──────────────┐
│ release/x.y  │ ◀──────────── Release preparation
└──────┬───────┘                Final testing
       │                        Bug fixes only
       │ stable & tested
       ↓
┌─────────┐
│  main   │ ◀──────────────── Production release
└────┬────┘                    Tagged (vX.Y.Z)
     │                         Deployed
     │
     └──────────────────────→ Merge back to develop
                              Delete release branch
```

## Color-Coded Branches (Conceptual)

```
Branch Colors (for visualization):

🟢 main          - Green  (Stable, Production)
🔵 develop       - Blue   (Active Development)
🟡 feature/*     - Yellow (In Progress)
🟠 release/*     - Orange (Preparing Release)
🔴 hotfix/*      - Red    (Critical Fix)
⚪ bugfix/*      - White  (Bug Fix)
```

## Merge Strategy Summary

```
Source Branch      →  Target Branch     Merge Strategy
──────────────────────────────────────────────────────────
feature/xyz        →  develop          Squash or Merge Commit
bugfix/abc         →  develop          Squash or Merge Commit
release/x.y        →  main             Merge Commit (--no-ff)
release/x.y        →  develop          Merge Commit (--no-ff)
hotfix/x.y.z       →  main             Merge Commit (--no-ff)
hotfix/x.y.z       →  develop          Merge Commit or Cherry-pick
```

## Key Principles Visualized

```
1. ONE-WAY FLOW (Normal Development)

   feature → develop → release → main
     ✓        ✓         ✓        ✓


2. TWO-WAY FLOW (Release/Hotfix)

   develop → release → main
                ↓       ↓
              develop ← ┘
                ↑
   main → hotfix ┘


3. NEVER DO THIS

   feature → main         ❌ No direct to main
   main → develop         ❌ No backflow (except hotfix/release)
   feature → feature      ❌ No cross-feature merges
```

---

## Quick Command Reference

**Create Feature:**
```bash
git checkout develop && git pull && git checkout -b feature/NAME
```

**Update Feature:**
```bash
git checkout feature/NAME && git pull origin develop
```

**Create Release:**
```bash
git checkout develop && git checkout -b release/X.Y.Z
```

**Create Hotfix:**
```bash
git checkout main && git checkout -b hotfix/X.Y.Z
```

---

**Print this diagram for your desk!** 📄

Save as PDF: `Ctrl+P` → `Save as PDF`
