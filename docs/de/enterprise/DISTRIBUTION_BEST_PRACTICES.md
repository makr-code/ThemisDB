# Enterprise Source Code Distribution - Best Practices

**Stand:** 6. April 2026  
**Version:** v1.3.0  
**Kategorie:** 🏢 Enterprise  
**Purpose:** Document best practices for distributing enterprise source code

---

## 📑 Table of Contents

- [Executive Summary](#executive-summary)
- [Distribution Models](#distribution-models)
- [Best Practices](#best-practices)

---

## Executive Summary

This document outlines best practices for distributing enterprise source code separately from the community edition, following industry standards established by GitLab, MongoDB, Elastic, and other successful commercial open-source projects.

---

## Table of Contents

1. [Current Implementation](#current-implementation)
2. [Industry Best Practices](#industry-best-practices)
3. [Recommended Approach](#recommended-approach)
4. [Distribution Options](#distribution-options)
5. [License Management](#license-management)
6. [FAQ](#faq)

---

## Current Implementation

### ✅ Phase 1: Source Code Separation (COMPLETE)

**Public Repository (GitHub):**
- ✅ Enterprise source code removed from git tracking
- ✅ `.gitignore` prevents re-adding enterprise files
- ✅ Build system handles missing enterprise gracefully
- ✅ Community Edition fully functional
- ✅ Clear documentation (ENTERPRISE.md)

**What Happens Now:**

| User Type | Current Clones | New Clones |
|-----------|----------------|------------|
| **Existing Users** | Keep enterprise files locally | Get only Community Edition |
| **New Users** | N/A | Get only Community Edition |
| **Enterprise Customers** | Keep enterprise files locally | Need separate enterprise package |

---

## Industry Best Practices

### Model 1: GitLab-Style Separation ⭐ RECOMMENDED

**Used by:** GitLab, Sentry, PostHog

**Structure:**
```
Public Repository (Community Edition):
  - Full-featured community version
  - Plugin interfaces/hooks for enterprise
  - Documentation of enterprise features (not implementation)

Private Repository or Package (Enterprise Edition):
  - Enterprise implementations
  - Only accessible to licensed customers
  - Integrates via documented interfaces
```

**Advantages:**
- ✅ Clear separation of concerns
- ✅ Community can build and test independently
- ✅ Enterprise fully protected
- ✅ No destructive updates
- ✅ Professional image

**ThemisDB Status:** ✅ **Already following this model!**

---

### Model 2: MongoDB-Style Modules

**Used by:** MongoDB, Confluent Kafka

**Structure:**
```
Public Repository:
  - Core database engine (community)
  - Modular architecture with plugin system

Separate Distribution:
  - Enterprise modules as compiled binaries
  - OR: Source code in private repository
  - Installed as additional packages
```

**Advantages:**
- ✅ Modular design encourages clean architecture
- ✅ Can distribute as binaries (no source needed)
- ✅ Easy upgrades

**ThemisDB Status:** ✅ **Already supports this via DLL architecture!**

---

### Model 3: Elastic-Style SSPL

**Used by:** Elasticsearch, MongoDB (newer versions)

**Structure:**
- Single repository with all code
- Different licenses for different parts
- Server Side Public License (SSPL) restricts commercial cloud use

**Not Recommended for ThemisDB:**
- ❌ Controversial licensing
- ❌ Complex legal implications
- ❌ Community friction

---

## Recommended Approach

### ✅ Best Practice: Hybrid GitLab + Plugin Model

**For ThemisDB, we recommend continuing with the current approach plus:**

### Phase 2: Enterprise Distribution (Next Step)

#### Option A: Private Git Repository (Preferred for Source Access)

**Setup:**
```bash
# 1. Create private repository (GitHub/GitLab/Bitbucket)
ThemisDB-Enterprise (Private)
├── src/enterprise/
├── include/enterprise/
├── plugins/enterprise/
├── LICENSE-ENTERPRISE.txt
├── README-ENTERPRISE.md
└── INTEGRATION.md

# 2. Grant access to licensed customers
# Via GitHub/GitLab team membership

# 3. Customers integrate via git submodule or manual copy
```

**Customer Workflow:**
```bash
# Clone public repository
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# Add enterprise source (Option 1: Submodule)
git submodule add https://github.com/makr-code/ThemisDB-Enterprise.git enterprise-src
cp -r enterprise-src/src/enterprise src/
cp -r enterprise-src/include/enterprise include/
cp -r enterprise-src/plugins/enterprise plugins/

# OR (Option 2: Direct clone)
git clone https://github.com/makr-code/ThemisDB-Enterprise.git ../ThemisDB-Enterprise
cp -r ../ThemisDB-Enterprise/src/enterprise src/
cp -r ../ThemisDB-Enterprise/include/enterprise include/

# Build with enterprise
cmake -B build -S . -DTHEMIS_BUILD_ENTERPRISE=ON
cmake --build build
```

**Advantages:**
- ✅ Version control for enterprise code
- ✅ Easy updates (git pull)
- ✅ Access control via Git platform
- ✅ Audit trail of who has access

---

#### Option B: Zip/Tar Archive Distribution (Simpler for Most Cases) ⭐

**Setup:**
```bash
# Package enterprise source
./.github/workflows/04-release_publish-enterprise.yml v1.3.0

# Creates:
themisdb-enterprise-v1.3.0.tar.gz
└── themisdb-enterprise-v1.3.0/
    ├── src/enterprise/
    ├── include/enterprise/
    ├── plugins/enterprise/
    ├── LICENSE-ENTERPRISE.txt
    └── INTEGRATION.md
```

**Distribution:**
1. **Email to customers** with license key
2. **Download portal** with authentication
3. **Customer portal** with license management

**Customer Workflow:**
```bash
# Clone public repository
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# Download and extract enterprise package
curl -H "Authorization: Bearer <LICENSE_KEY>" \
  https://enterprise.themisdb.com/download/v1.3.0 \
  -o themisdb-enterprise-v1.3.0.tar.gz

tar -xzf themisdb-enterprise-v1.3.0.tar.gz
cp -r themisdb-enterprise-v1.3.0/src/enterprise src/
cp -r themisdb-enterprise-v1.3.0/include/enterprise include/
cp -r themisdb-enterprise-v1.3.0/plugins/enterprise plugins/

# Build with enterprise
cmake -B build -S . -DTHEMIS_BUILD_ENTERPRISE=ON
cmake --build build
```

**Advantages:**
- ✅ Simple to implement
- ✅ No additional Git infrastructure
- ✅ Easy to version
- ✅ Works with any download mechanism
- ✅ Customers don't need git access setup

---

#### Option C: Binary Distribution (No Source)

**For customers who don't need source access:**

```bash
# Distribute pre-compiled enterprise DLLs
themisdb-enterprise-binaries-v1.3.0.zip
└── lib/enterprise/
    ├── themis_enterprise_sharding.dll
    ├── themis_enterprise_analytics.dll
    ├── themis_enterprise_replication.dll
    └── ...

# Customer just copies to build directory
cp lib/enterprise/*.dll /opt/themisdb/lib/enterprise/
```

**Advantages:**
- ✅ Protect source code completely
- ✅ Simplest for customers
- ✅ Smaller download size
- ✅ Faster deployment

**Disadvantages:**
- ❌ Platform-specific (need Windows, Linux, macOS builds)
- ❌ Customers can't modify/debug
- ❌ Requires CI/CD for multiple platforms

---

## License Management

### Recommended: License Key System

**In enterprise source/binaries, add license validation:**

```cpp
// src/enterprise/common/license_validation.cpp
#include "enterprise/license_validation.h"

namespace themis::enterprise {

bool validateLicense(const std::string& license_key) {
    // Online validation
    auto response = http_client.post(
        "https://license.themisdb.com/validate",
        {{"key", license_key}, {"product", "ThemisDB-Enterprise"}}
    );
    
    if (response.status == 200) {
        auto data = json::parse(response.body);
        return data["valid"] && data["expires"] > now();
    }
    
    return false;
}

} // namespace themis::enterprise
```

**License file format:**
```json
{
  "license_key": "THEMIS-ENT-1234-5678-90AB-CDEF",
  "customer": "Acme Corporation",
  "tier": "enterprise",
  "features": ["sharding", "analytics", "replication"],
  "issued": "2024-01-01",
  "expires": "2025-12-31",
  "signature": "..."
}
```

---

## FAQ

### Q: Should we force-delete enterprise files from existing clones?

**A: No. ❌ Not recommended.**

**Reasons:**
- Destructive to licensed customers
- Could cause data loss if locally modified
- Bad user experience
- Breaks builds for active customers

**Better approach:**
- New clones: Get only community edition
- Existing clones: Keep what they have
- Licensed customers: Get enterprise via separate channel

---

### Q: What if someone already has the enterprise code?

**A: This is acceptable and common in commercial open source.**

**Reasoning:**
- Past commits are always in git history anyway
- Source code without license key has limited value
- Focus on preventing *future* access, not past
- License enforcement is legal, not technical

**Similar examples:**
- GitLab EE source was public until 2018
- MongoDB enterprise source still semi-accessible via old commits
- Most commercial OSS has "leaky" history

---

### Q: How do other companies handle this?

**A: Most use one of these approaches:**

1. **GitLab:** Private repo + compiled binaries in omnibus package
2. **MongoDB:** SSPL license + enterprise modules
3. **Elastic:** Dual licensing (Elastic License + SSPL)
4. **Confluent:** Apache Kafka (free) + Confluent Platform (paid)
5. **Redis:** BSD for core + Commons Clause for modules

**ThemisDB's approach (GitLab-style) is considered best practice.**

---

### Q: Can we use Git Submodules?

**A: Yes, but adds complexity.**

**Pros:**
- ✅ Clean separation
- ✅ Version tracking
- ✅ Easy updates

**Cons:**
- ❌ More complex for customers
- ❌ Requires git access setup
- ❌ Submodule gotchas (detached HEAD, etc.)

**Recommendation:** Use for internal development, but offer zip/tar for customers.

---

### Q: Should enterprise source be open source?

**A: No, that defeats the purpose.**

**If you want transparency:**
- Keep high-level docs public (you already do this ✅)
- Offer "source available" for large customers (view but not redistribute)
- Focus on feature documentation, not implementation

---

## Implementation Roadmap

### ✅ Phase 1: Source Separation (COMPLETE)
- [x] Remove enterprise from public repo
- [x] Update .gitignore
- [x] Update build system
- [x] Documentation (ENTERPRISE.md)

### 🔄 Phase 2: Distribution Setup (Recommended Next Steps)

**Option A: Simple Zip Distribution (Fastest)**
1. Create packaging script
2. Setup download mechanism (email, portal, etc.)
3. Document customer integration process
4. Update ENTERPRISE.md with instructions

**Option B: Private Repository (More Professional)**
1. Create private GitHub/GitLab repository
2. Setup access management
3. Write integration documentation
4. Automate customer onboarding

### 📋 Phase 3: License Management (Optional)
1. Implement license key validation
2. Create customer portal
3. Setup automated delivery
4. Add telemetry (optional)

---

## Recommended Next Action

### For ThemisDB: ✅ **Simple Zip Distribution**

**Why:**
- ✅ Fastest to implement
- ✅ No additional infrastructure
- ✅ Works for all customers
- ✅ Easy to version and maintain

**Implementation Steps:**

1. **Create packaging script:** `.github/workflows/04-release_publish-enterprise.yml`
2. **Test integration:** Verify customers can integrate easily
3. **Update ENTERPRISE.md:** Add integration instructions
4. **Setup distribution:** Email, Google Drive, or simple web server

**Later upgrades:**
- Private Git repository (when team grows)
- Automated portal (when sales increase)
- License server (when scaling)

---

## Conclusion

**Current Status:** ✅ ThemisDB already follows best practices

**Recommendation:** 
- Continue current approach (enterprise in .gitignore)
- Add simple zip distribution for customers
- Consider private repo later as business scales

**Key Principle:** 
> "Make it easy for customers to succeed, hard for non-customers to access."

---

**Questions?** Contact: engineering@themisdb.com

**See Also:**
- [ENTERPRISE.md](../../ENTERPRISE.md) - Customer-facing documentation
- [IMPLEMENTATION_SUMMARY.md](../reports/IMPLEMENTATION_SUMMARY.md) - Technical implementation
