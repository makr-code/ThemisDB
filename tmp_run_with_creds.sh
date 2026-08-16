#!/bin/bash
set -eo pipefail
cd /mnt/c/Projects/ThemisDB
b64=$(printf '%s' 'makr-code:Move24$date' | base64)
# Temporär Authorization-Header setzen
git config --local http.extraHeader "Authorization: Basic ${b64}"
# Gefiltertes Reparaturskript ausführen und Output in eine Logdatei schreiben
bash scripts/submodule_fix_filtered.sh > submodule_fix_filtered_run_with_creds.txt 2>&1 || true
# Header wieder entfernen
git config --local --unset-all http.extraHeader || true
echo "DONE: log -> submodule_fix_filtered_run_with_creds.txt"
