# 🔄 Smart Header Format Detection & Update

**Version:** 2.1  
**Status:** Ready to use  
**Features:** Format preservation, intelligent updates

---

## 🎯 Problem Solved

Previously, headers were replaced entirely. Now, existing headers are **recognized and updated in-place**, preserving their format and structure.

### Before (v1)
```cpp
// Old copyright notice
// ...

new header inserted here
```

### After (v2)
```cpp
// Old copyright notice (PRESERVED!)
//

// THEMIS_GAP_STATS: gaps=8 unimpl=5 ... scanned=2026-05-18
```

---

## 📊 Supported Header Formats

### 1. **Copyright/License Blocks** (Detected & Preserved)

**Pattern:** `/* Copyright ... */` or `// Copyright ...`

```cpp
/*
 * Copyright (c) 2026 ThemisDB Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

// THEMIS_GAP_STATS: gaps=5 unimpl=3 stub=2 scanned=2026-05-18

#include "header.h"
```

**Action:** Insert THEMIS_GAP header after copyright block

### 2. **File Description Blocks** (Detected & Preserved)

**Pattern:** `// @file`, `// —`, `// Name:`

```cpp
/*
 * Copyright (c) 2026
 */

// @file accelerator.cpp
// GPU acceleration kernels
// Version: 2.0

// THEMIS_GAP_STATS: gaps=12 unimpl=8 stub=4 scanned=2026-05-18

#include "gpu.h"
```

**Action:** Insert after description block

### 3. **Existing THEMIS_GAP Headers** (In-place Update)

**Pattern:** `// THEMIS_GAP_STATS:` or `// THEMIS_GAP_ANALYSIS`

```cpp
// Old header
// THEMIS_GAP_STATS: gaps=3 unimpl=2 stub=1 scanned=2026-05-10

↓ UPDATE ↓

// Old header
// THEMIS_GAP_STATS: gaps=8 unimpl=5 stub=3 scanned=2026-05-18
```

**Action:** Replace only the stats line, preserve everything else

### 4. **Generic Comments** (Append After)

**Pattern:** Random `//` lines without structure

```cpp
// Some random comment
// Another comment

// THEMIS_GAP_STATS: gaps=2 unimpl=1 stub=1 scanned=2026-05-18

void foo() {}
```

**Action:** Append after last header comment

---

## 🔍 Header Format Detection

### How It Works

```python
from header_format_detector import HeaderFormatDetector

detector = HeaderFormatDetector(Path('src/gpu/kernel.cpp'))

# Get detected format
format_type = detector.get_header_type()
# Returns: 'copyright', 'license', 'description', 'themis_stats', 
#          'themis_analysis', or 'generic_comments'

# Check if file already has THEMIS_GAP header
has_header = detector.has_themis_gap_header()  # True/False
line_num = detector.get_existing_gap_line_number()  # None or line#

# Pretty print analysis
detector.print_analysis()
```

### Detected Types

| Type | Pattern | Action |
|------|---------|--------|
| `copyright` | `/* Copyright ... */` | Preserve, insert after |
| `license` | `// (SPDX\|GPL\|MIT)` | Preserve, insert after |
| `description` | `// @file`, `// —` | Preserve, insert after |
| `themis_stats` | `// THEMIS_GAP_STATS:` | **Update in-place** |
| `themis_analysis` | `// THEMIS_GAP_ANALYSIS` | **Update in-place** |
| `generic_comments` | Random `//` lines | Append after last |

---

## 📈 Usage Examples

### Example 1: Existing Copyright + License

**Before:**
```cpp
/*
 * Copyright (c) 2026 ThemisDB
 * SPDX-License-Identifier: Apache-2.0
 */

#include "query.h"

void optimizeQuery() {
    // STUB: Join optimization
    return;
}
```

**After Update:**
```cpp
/*
 * Copyright (c) 2026 ThemisDB
 * SPDX-License-Identifier: Apache-2.0
 */
//
// THEMIS_GAP_STATS: gaps=1 unimpl=0 stub=1 todo=0 scanned=2026-05-18

#include "query.h"

void optimizeQuery() {
    // STUB: Join optimization
    return;
}
```

✅ **Copyright preserved**, THEMIS_GAP inserted after

### Example 2: Existing THEMIS_GAP Stats

**Before:**
```cpp
// THEMIS_GAP_STATS: gaps=3 unimpl=2 stub=1 scanned=2026-05-10

#include "gpu.h"

void computeGPU() { /* stub */ }
```

**After Update:**
```cpp
// THEMIS_GAP_STATS: gaps=5 unimpl=3 stub=2 scanned=2026-05-18

#include "gpu.h"

void computeGPU() { /* stub */ }
```

✅ **Stats updated in-place** (line not changed, only values updated)

### Example 3: File Description

**Before:**
```cpp
// @file storage/btree.cpp
// B-tree index implementation
// Status: In Progress

#include "btree.h"

void insert() {
    // TODO: implement
}
```

**After Update:**
```cpp
// @file storage/btree.cpp
// B-tree index implementation
// Status: In Progress
//
// THEMIS_GAP_STATS: gaps=1 unimpl=0 stub=0 todo=1 scanned=2026-05-18

#include "btree.h"

void insert() {
    // TODO: implement
}
```

✅ **Description preserved**, THEMIS_GAP appended

---

## 🛠️ Command Line Tools

### 1. Analyze Single File

```bash
python tools/header_format_detector.py src/acceleration/gpu.cpp
```

**Output:**
```
📄 File: gpu.cpp
   Header Type: copyright
   Header End Line: 5
   Has THEMIS_GAP: False
   THEMIS_GAP Line: None
```

### 2. Analyze Entire Repo

```bash
python tools/header_format_detector.py --analyze-repo .
```

**Output:**
```
📊 Header Type Distribution:
   Total Files: 523
   ✓ Copyright Blocks: 389 (74%)
   ✓ License Blocks: 85 (16%)
   ✓ File Descriptions: 67 (13%)
   ✓ THEMIS_GAP_STATS: 0 (0%)
   ✓ THEMIS_GAP_ANALYSIS: 0 (0%)
   - Generic Comments: 45 (9%)
   - No Header: 12 (2%)

💡 Implications:
   - 474 files have copyright/license
   - 0 files already have THEMIS_GAP headers
   - 523 files need new THEMIS_GAP headers
```

### 3. Test Header Updates

```bash
python tools/test_header_updater.py
```

**Output:**
```
Test 1: Update existing stats
   Changed: True
   Expected: True
   ✓ Header was updated

Test 2: Add new stats to copyright
   Changed: True
   Expected: True
   ✓ Copyright preserved, stats added

Test 3: Preserve without changes
   Changed: False
   Expected: False
   - No changes needed (already current)
```

### 4. Update All File Headers

```bash
# Full pipeline (scan + update headers with format detection)
python tools/gap_audit_pipeline_v2.py

# Or just update headers using existing scan
python tools/file_header_updater.py \
    ai_working/gap_scan_v2_aggregate.json \
    . \
    --preserve-format
```

---

## 🔧 Implementation Details

### Smart Update Algorithm

```python
def update_file_header(file_path, gap_stats, detailed=False):
    # Step 1: Detect existing header format
    format = detect_header_format(file)
    
    # Step 2: Find where header ends
    header_end_line = find_header_end(file)
    
    # Step 3: Check for existing THEMIS_GAP
    gap_line_num = find_existing_gap_header(file)
    
    # Step 4: Update or insert based on format
    if gap_line_num is not None:
        # Format 1: Update existing THEMIS_GAP in-place
        lines[gap_line_num] = new_stats
    elif format in ['copyright', 'license', 'description']:
        # Format 2: Insert after header block
        lines.insert(header_end_line, separator)
        lines.insert(header_end_line + 1, new_stats)
    else:
        # Format 3: Append to generic comments
        lines.insert(header_end_line, new_stats)
    
    # Step 5: Write only if changed
    if modified:
        write_file(file, lines)
        return True
    return False
```

### Key Features

✅ **Format Detection:** Identifies copyright, license, description, existing stats  
✅ **In-Place Update:** Modifies existing THEMIS_GAP stats without moving the line  
✅ **Preservation:** Keeps all existing headers intact  
✅ **Smart Insertion:** Places new headers in appropriate location  
✅ **Minimal Changes:** Only writes if file actually changed  
✅ **Encoding Safe:** Uses UTF-8 consistently  

---

## 📊 Expected Results After Pipeline Run

### File Statistics

```
✅ Header Update Complete:
   Total Files: 523
   Files Updated: 487 (93%)
   Files with Gaps: 487
   Files without Gaps: 36
```

### Example File Distribution After First Run

```
src/
├── acceleration/
│   ├── gpu_kernel.cpp
│   │   // THEMIS_GAP_STATS: gaps=12 unimpl=8 stub=4 ... scanned=2026-05-18
│   │
│   └── cpu_fallback.cpp
│       // THEMIS_GAP_STATS: gaps=2 unimpl=1 stub=1 ... scanned=2026-05-18
│
├── security/
│   ├── auth.cpp
│   │   // THEMIS_GAP_STATS: gaps=8 unimpl=5 stub=3 ... scanned=2026-05-18
│   │
│   └── encryption.cpp
│       // THEMIS_GAP_STATS: gaps=0 unimpl=0 stub=0 ... scanned=2026-05-18
```

### Tracking Progress

**Run 1 (baseline):**
```json
{
  "scan_date": "2026-05-18",
  "total_gaps": 1862,
  "unimplemented": 1620,
  "stub": 384
}
```

**Run 2 (after 2 weeks):**
```json
{
  "scan_date": "2026-06-01",
  "total_gaps": 1247,  // ↓ 32% reduction
  "unimplemented": 945,  // ↓ 42% reduction
  "stub": 302  // ↓ 21% reduction
}
```

Each file header shows progress over time! 📈

---

## ✨ Benefits

1. **Non-Destructive:** Never removes or corrupts existing headers
2. **Progressive:** Can re-run scanner, headers auto-update
3. **Traceable:** Every file shows last scan date
4. **Professional:** Respects copyright/license blocks
5. **Flexible:** Handles any header format the repo uses
6. **Low-Risk:** Can be tested before full pipeline run

---

## 🚀 Quick Start

```bash
# 1. Analyze what's in your repo
python tools/header_format_detector.py --analyze-repo .

# 2. Test on a few files
python tools/test_header_updater.py

# 3. Run full pipeline (scan + update with format preservation)
python tools/gap_audit_pipeline_v2.py

# 4. Verify results
grep -r "THEMIS_GAP_STATS" src/ | head -20
```

---

## 🎓 FAQ

**Q: What if my file has multiple header blocks?**  
A: The detector finds the end of all comment blocks and inserts after them.

**Q: Can I manually edit the THEMIS_GAP line?**  
A: Yes! The next scan will overwrite it with current stats. Just edit and run again.

**Q: What about files with no header?**  
A: THEMIS_GAP header is inserted at the very top (line 1).

**Q: Does it handle special characters in comments?**  
A: Yes, full UTF-8 support. Special characters in existing headers are preserved.

**Q: Can I use a different format for THEMIS_GAP?**  
A: Yes, modify `format_header()` and `format_detailed_header()` in `FileGapStats` class.

---

**Status:** ✅ Ready for production use  
**Last Updated:** 2026-05-18
