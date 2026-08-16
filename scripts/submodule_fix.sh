#!/usr/bin/env bash
set -uo pipefail

# Submodule repair script — run from repo root (WSL/bash expected)
git config --global url."git@github.com:".insteadOf "https://github.com/" || true
echo "SSH mapping set"

paths=(vcpkg llama.cpp whisper.cpp stable-diffusion.cpp ffmpeg external/GIUF plugins/themisdb_ethic_ai plugins/themisdb_llm_wiki plugins/themisdb_storage plugins/themisdb_importer plugins/themisdb_plugin_signer plugins/themisdb_geo plugins/themisdb_timeseries projects/Themis.AdminTools.Shared docker/tmp/openssl tools/semgrep tools/codeql)
log=submodule_fix_run.txt

echo "Run started: $(date -Iseconds)" > "$log"
stashed=()
updated=()
skipped=()

for p in "${paths[@]}"; do
  echo
  echo "===== $p =====" | tee -a "$log"
  if [ -d "$p" ]; then
    echo "--- git status --porcelain ---" | tee -a "$log"
    git -C "$p" status --porcelain | tee -a "$log"

    super_sha=$(git ls-tree HEAD "$p" 2>/dev/null | awk '{print $3}') || true
    echo "--- superproject recorded commit ---" | tee -a "$log"
    echo "$super_sha" | tee -a "$log"

    echo "--- ls-remote check for recorded commit ---" | tee -a "$log"
    if [ -n "$super_sha" ]; then
      git -C "$p" ls-remote origin "$super_sha" | tee -a "$log"
    else
      echo "(no recorded commit)" | tee -a "$log"
    fi

    status=$(git -C "$p" status --porcelain)
    if [ -n "$status" ]; then
      echo "Stashing local changes in $p" | tee -a "$log"
      git -C "$p" stash push -m "autostash: pre-repair $(date -Iseconds)" | tee -a "$log"
      stashed+=("$p")
    fi

    if [ -n "$super_sha" ]; then
      found=$(git -C "$p" ls-remote origin "$super_sha" ) || true
    else
      found=""
    fi

    if [ -z "$found" ]; then
      echo "Recorded commit not on origin for $p; attempting pointer update to origin/HEAD" | tee -a "$log"
      remote_head_sha=$(git -C "$p" ls-remote origin HEAD | awk '{print $1}' | head -n1) || true
      if [ -n "$remote_head_sha" ]; then
        git -C "$p" fetch origin || true
        git -C "$p" checkout "$remote_head_sha" --detach || git -C "$p" checkout origin/HEAD --detach || true
        git add "$p" || true
        updated+=("$p")
        echo "Updated pointer for $p to $remote_head_sha" | tee -a "$log"
      else
        echo "No origin/HEAD for $p — skipping pointer update" | tee -a "$log"
        skipped+=("$p")
      fi
    else
      echo "Recorded commit present on origin for $p — no pointer change needed" | tee -a "$log"
    fi
  else
    echo "MISSING PATH: $p" | tee -a "$log"
  fi

done

if ! git diff --staged --quiet --ignore-submodules --; then
  echo "Staged changes present — committing superproject update" | tee -a "$log"
  git commit -m "chore(submodules): repair missing submodule commits and stash local changes" | tee -a "$log" || true
else
  echo "No staged submodule pointer changes" | tee -a "$log"
fi

echo "Pushing develop branch to origin (may require permissions)" | tee -a "$log"
git push origin develop 2>&1 | tee -a "$log" || true

echo "Run finished: $(date -Iseconds)" | tee -a "$log"
echo "Stashed:" | tee -a "$log"
printf '%s
' "${stashed[@]}" | tee -a "$log"
echo "Updated:" | tee -a "$log"
printf '%s
' "${updated[@]}" | tee -a "$log"
echo "Skipped:" | tee -a "$log"
printf '%s
' "${skipped[@]}" | tee -a "$log"

echo DONE | tee -a "$log"
