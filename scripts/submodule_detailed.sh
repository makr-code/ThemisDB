#!/usr/bin/env bash
paths=(
  vcpkg
  llama.cpp
  whisper.cpp
  stable-diffusion.cpp
  ffmpeg
  external/GIUF
  plugins/themisdb_ethic_ai
  plugins/themisdb_llm_wiki
  plugins/themisdb_storage
  plugins/themisdb_importer
  plugins/themisdb_plugin_signer
  plugins/themisdb_geo
  plugins/themisdb_timeseries
  projects/Themis.AdminTools.Shared
  docker/tmp/openssl
  tools/semgrep
  tools/codeql
)
for p in "${paths[@]}"; do
  echo
  echo "===== $p ====="
  if [ -d "$p" ]; then
    echo "status:"
    git -C "$p" status --porcelain || true
    echo "last 5 commits:"
    git -C "$p" log -n 5 --oneline --decorate || true
    echo "ls-remote (top 20):"
    git -C "$p" ls-remote origin | sed -n '1,20p' || true
  else
    echo "MISSING PATH"
  fi
done
