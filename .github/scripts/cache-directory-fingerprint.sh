#!/usr/bin/env bash
set -euo pipefail

# Inventory change detection, not integrity validation. Package ABI names and
# sizes identify completed vcpkg archives without rereading multi-GiB payloads.
python - "$@" <<'PY'
import hashlib
import os
import stat
import sys

digest = hashlib.sha256()

def raise_walk_error(error):
    raise error

for index, directory in enumerate(sys.argv[1:]):
    digest.update(f"directory={index}\n".encode())
    if not os.path.exists(directory):
        continue
    for root, directories, files in os.walk(directory, onerror=raise_walk_error):
        directories.sort()
        for name in sorted(files):
            path = os.path.join(root, name)
            info = os.stat(path, follow_symlinks=False)
            if not stat.S_ISREG(info.st_mode):
                continue
            relative = os.fsencode(os.path.relpath(path, directory).replace(os.sep, "/"))
            digest.update(len(relative).to_bytes(8, "big"))
            digest.update(relative)
            digest.update(info.st_size.to_bytes(8, "big"))

print(digest.hexdigest())
PY
