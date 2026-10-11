---
Author: "Copilot Task Agent"
Created: "2026-10-11"
Last Updated: "2026-10-11"
Status: "GitHub Issue-Based Team Assignment Ready"
---

# Wave 10 Phase 1: GitHub Issue-Based Team Assignment

## 🎯 Requirement Acknowledged
✅ **Team assignments running via GitHub Issues** (not manual form)

---

## 📋 All Assignment Issues Created & Ready

### Epic (Centralized Hub)
| Issue | Role | Effort | Purpose |
|-------|------|--------|---------|
| **#6929** | **Epic** | N/A | Centralized tracking for all 7 team roles + timeline |

### Block D: Governance Framework (57 tests)
| Issue | Role | Effort | Purpose |
|-------|------|--------|---------|
| **#6921** | Block D Lead | 50-60h | Wave exits, Edition compliance, Roadmap alignment tests |
| **#6922** | Block D Support | 40-50h | CI setup, documentation, PR templates |

### Block A: Phase 3 Batch 1 (90 tests)
| Issue | Role | Effort | Purpose |
|-------|------|--------|---------|
| **#6923** | distributed_tensor Lead | 25h | 25 tests (allocation, operations, performance) |
| **#6924** | llama_cpp Lead | 20h | 20 tests (model loading, inference, errors) |
| **#6925** | stable_diffusion Lead | 20h | 20 tests (initialization, generation, edge cases) |
| **#6926** | execution Lead | 25h | 25 tests (compilation, execution, performance) |
| **#6927** | Integration Lead | 30-40h | CMake registration, cross-module coordination |

### Shared
| Issue | Role | Effort | Purpose |
|-------|------|--------|---------|
| **#6928** | Code Reviewer | 40-60h | Week 5 convergence, PR approvals, merge sign-off |

**Total:** 8 issues, 7 roles, 6-7 developers, ~190-200 team-hours

---

## 🔗 How Team Members Claim Their Role

### Step 1: Navigate to Your Issue
Click on the issue link (e.g., https://github.com/makr-code/ThemisDB/issues/6921)

### Step 2: Assign to Yourself
- Click **"Assign yourself"** or **"Assignees"** → select your name

### Step 3: Reply with Confirmation
Post a comment confirming:
```
I'm assigned to this issue.
Available for Weeks X-Y: YES / NO
Kickoff time (09:00 UTC Monday Oct 14): OK / Need to adjust
Questions: [any blockers or dependencies?]
```

### Step 4: Review Your Reference Materials
- Read linked documentation in the issue
- Check `BLOCK_D_GOVERNANCE_IMPLEMENTATION_GUIDE.md` or `BLOCK_A_PHASE3_BATCH1_IMPLEMENTATION.md`
- Review test templates in `ai_working/test_templates/`

### Step 5: Join #wave-10-phase1 Slack
- Async coordination channel
- Daily standups (15 min)
- Blocker escalation

---

## 📊 Issue Metadata (Pre-Configured)

### Labels (Auto-Applied)
- `wave-10` — All issues (program tracking)
- `block-d` — Governance issues (#6921, #6922)
- `block-a` — Implementation issues (#6923-#6927)
- `assignment` — All team role issues
- Module-specific: `distributed_tensor`, `llama_cpp`, `stable_diffusion`, `execution`
- `integration`, `cmake` — Cross-module issues (#6927)
- `review` — Code review issue (#6928)

### Milestone
- `Wave 10 Phase 1 - Parallel D+A` — Groups all issues, tracks progress

### Progress Tracking
Each developer posts daily updates in their issue:
```
### Day N Progress
- Tests written: X
- Files created: [list]
- CMake status: [status]
- Blockers: [any issues?]
- Next: [tomorrow's plan]
```

---

## 📅 Timeline

### Week 2 (Oct 14-18)
- **Mon Oct 14:** Developers claim issues + confirm availability
- **Mon-Fri:** Setup + 25% progress both streams
- **Fri EOD:** Issue updates with Week 2 exit criteria

### Week 3 (Oct 21-25)
- **Wed Oct 23:** Mid-phase sync meeting (30 min)
- **Daily:** Progress updates in issues
- **Fri EOD:** 50% progress both streams

### Week 4 (Oct 28-Nov 1)
- Block D enters code review
- Block A continues implementation
- **Fri EOD:** 75% progress both streams

### Week 5 (Nov 4-8)
- **Daily:** Code review ramp-up (40+ hours)
- **Fri EOD:** All 147 tests merged ✅

---

## ✅ Each Issue Contains

✓ **Effort & Timeline** — Hours + weeks  
✓ **Responsibility** — What you're delivering  
✓ **Deliverables** — Specific files, targets, counts  
✓ **Skills Required** — What to know  
✓ **Reference Materials** — Docs to read  
✓ **Success Criteria Checklist** — How to know you're done  
✓ **Escalation Protocol** — Who to ask if stuck  

---

## 🎯 Success Definition per Issue

### Block D Lead (#6921)
- [ ] 30 Wave exit criteria tests written
- [ ] 15 Edition compliance tests written
- [ ] 12 Roadmap alignment tests written
- [ ] All 3 audit CMake targets registered
- [ ] Configure passes, no linker errors
- [ ] 100% API coverage verified

### Block D Support (#6922)
- [ ] Nightly CI pipeline operational
- [ ] Documentation template created
- [ ] PR checklist template created
- [ ] Daily standups coordinated
- [ ] GitHub Milestone + Labels active

### Module Leads (#6923-#6926, Each)
- [ ] X tests written (25, 20, 20, or 25)
- [ ] >= 95% line coverage verified
- [ ] All public APIs tested
- [ ] Error paths covered (EXPECT_THROW)
- [ ] CMake targets registered
- [ ] Local build succeeds (no linker errors)

### Integration Lead (#6927)
- [ ] Test skeletons created (all 4 modules)
- [ ] CMake registration complete
- [ ] Configure passes (no errors)
- [ ] Cross-module link tests passing
- [ ] Module dependencies documented

### Code Reviewer (#6928)
- [ ] Code review protocol finalized
- [ ] PR templates ready (both types)
- [ ] Design pre-approvals given (Week 3)
- [ ] Async reviews posted (Week 4)
- [ ] All 147 tests reviewed & approved (Week 5)
- [ ] Merge coordination completed

---

## 🔗 Documentation Navigation

**From Any Issue, You Can Access:**

1. **Epic #6929** — Central hub (overview, timeline, success criteria)
2. **Implementation Guides:**
   - `BLOCK_D_GOVERNANCE_IMPLEMENTATION_GUIDE.md` (Block D specs)
   - `BLOCK_A_PHASE3_BATCH1_IMPLEMENTATION.md` (Block A specs by module)
3. **Planning Documents:**
   - `WAVE10_PHASE1_EXECUTIVE_SUMMARY.md` (1-page overview)
   - `PARALLEL_D_A_EXECUTION_PLAN.md` (4-week detailed plan)
   - `WEEK2_KICKOFF_CHECKLIST.md` (Monday action plan)
4. **Test Templates:**
   - `test_templates/distributed_tensor/`
   - `test_templates/llama_cpp/`
   - `test_templates/stable_diffusion/`
   - `test_templates/execution/`

**All in:** `/home/runner/work/ThemisDB/ThemisDB/ai_working/`

---

## 📞 Communication Channels

**Primary:** GitHub Issues (daily async updates)  
**Slack:** #wave-10-phase1 (coordination + blockers)  
**Sync Meetings:** Weeks 2, 3, 5 (calendar invites)  
**Escalation:** See `PARALLEL_D_A_EXECUTION_PLAN.md` section 8

---

## 🚀 How to Start Monday Oct 14

### For You (Project Lead)
1. Share Epic #6929 with the team
2. Ask developers to claim their issue (GitHub Assign)
3. Collect confirmations of availability + kickoff time
4. Send calendar invite for Monday 09:00 UTC kickoff
5. Brief team on daily standup format

### For Each Developer
1. Click your issue link (#6921-#6928)
2. Click "Assign yourself"
3. Reply: "I'm assigned. Available [dates]. 09:00 UTC OK."
4. Read your implementation guide
5. Show up Monday 09:00 UTC ready to code

---

## ✨ Status: READY TO LAUNCH 🚀

- ✅ All 8 GitHub issues created
- ✅ Epic linking everything
- ✅ Each issue has complete context
- ✅ Labels & Milestone configured
- ✅ Documentation linked
- ✅ Awaiting developer assignments (GitHub Assign)

**No more manual form-filling. Developers claim issues directly from GitHub.**

---

## 📊 Issue Summary

```
Epic #6929 (Wave 10 Phase 1: Parallel D+A)
│
├─ Block D Stream (57 tests, 90-110 hours)
│  ├─ #6921: Block D Lead (50-60h)
│  └─ #6922: Block D Support (40-50h)
│
├─ Block A Stream (90 tests, 120-150 hours)
│  ├─ #6923: distributed_tensor Lead (25h)
│  ├─ #6924: llama_cpp Lead (20h)
│  ├─ #6925: stable_diffusion Lead (20h)
│  ├─ #6926: execution Lead (25h)
│  └─ #6927: Integration Lead (30-40h)
│
└─ Shared (40-60 hours)
   └─ #6928: Code Reviewer (40-60h)

Total: 7 roles, 6-7 developers, ~190-200 hours, Week 2-5
Target: 147 tests merged by Nov 8 ✅
```

---

## 🎯 Next Action

**Share this with your team:**

> "Wave 10 Phase 1 team assignments are now open as GitHub Issues. 
> 
> Claim your role: **https://github.com/makr-code/ThemisDB/issues/6929**
> 
> Then assign yourself to your specific issue (#6921-#6928).
> 
> Kickoff: **Monday Oct 14, 09:00 UTC**"

---

**Repo:** makr-code/ThemisDB  
**Milestone:** Wave 10 Phase 1 - Parallel D+A  
**Labels:** wave-10, block-d, block-a, assignment  
**Issues:** #6921-#6928 (team roles) + #6929 (epic)
