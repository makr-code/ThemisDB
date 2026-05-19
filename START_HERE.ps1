# Phase 3 Manual Execution - Windows PowerShell
# START HERE

Write-Host "=" * 80
Write-Host "PHASE 3: LOCAL OLLAMA -> GITHUB PRs -> CLOSE ISSUES"
Write-Host "=" * 80
Write-Host ""
Write-Host "Status: Ready for manual, module-by-module execution"
Write-Host "Modules ready: INDEX, ANALYTICS, STORAGE (generated + tested)"
Write-Host ""

Write-Host "=" * 80
Write-Host "QUICK START: Execute these commands in order"
Write-Host "=" * 80
Write-Host ""

Write-Host "1. Verify gh CLI:"
Write-Host "   gh auth status"
Write-Host ""

Write-Host "2. Show first module workflow:"
Write-Host "   python quick_reference.py index"
Write-Host ""

Write-Host "3. View detailed gh commands:"
Write-Host "   python workflow_gh_cli.py index"
Write-Host ""

Write-Host "4. View GitHub issue status:"
Write-Host "   python issue_tracker.py index"
Write-Host ""

Write-Host "=" * 80
Write-Host "MANUAL WORKFLOW SUMMARY"
Write-Host "=" * 80
Write-Host ""

$workflow = @(
    "1. Check Issue:       gh issue view #5270",
    "2. Create Branch:     git checkout -b feature/phase3-index-codegen",
    "3. Commit Code:       git commit --allow-empty -m 'Phase 3: Index (#5270)'",
    "4. Create PR:         gh pr create --draft --title 'Phase 3: ... - INDEX' --base develop --body-file ai_working/pr_body_index.md",
    "5. [SAVE PR_NUM from output above]",
    "6. Link to Issue:     gh pr comment `$PR_NUM --body 'Closes #5270'",
    "7. Update Issue:      gh issue comment #5270 --body 'PR created...'",
    "8. Verify:            gh pr view `$PR_NUM && gh issue view #5270"
)

foreach ($step in $workflow) {
    Write-Host $step
}

Write-Host ""
Write-Host "=" * 80
Write-Host "FOR OTHER MODULES"
Write-Host "=" * 80
Write-Host ""
Write-Host "List all 65 issues:"
Write-Host "  python issue_tracker.py --list"
Write-Host ""
Write-Host "Show workflow for ANALYTICS:"
Write-Host "  python quick_reference.py analytics"
Write-Host ""
Write-Host "Show workflow for STORAGE:"
Write-Host "  python quick_reference.py storage"
Write-Host ""

Write-Host "=" * 80
Write-Host "NEXT STEP"
Write-Host "=" * 80
Write-Host ""
Write-Host "Ready to execute? Run this first:"
Write-Host ""
Write-Host "  python quick_reference.py index"
Write-Host ""
