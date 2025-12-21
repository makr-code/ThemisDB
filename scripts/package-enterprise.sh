#!/bin/bash
# Package ThemisDB Enterprise Source Code
# Usage: ./package-enterprise.sh <version>

set -e

VERSION="${1:-$(cat VERSION)}"
PACKAGE_NAME="themisdb-enterprise-${VERSION}"
PACKAGE_DIR="dist/${PACKAGE_NAME}"
ARCHIVE_NAME="${PACKAGE_NAME}.tar.gz"

echo "================================================"
echo "ThemisDB Enterprise Source Package Builder"
echo "Version: ${VERSION}"
echo "================================================"
echo ""

# Check if enterprise directories exist
if [ ! -d "src/enterprise" ]; then
    echo "❌ Error: src/enterprise/ directory not found"
    echo "   Enterprise source code is not available in this clone."
    exit 1
fi

echo "✓ Enterprise source directories found"
echo ""

# Create package directory
echo "Creating package directory..."
rm -rf "${PACKAGE_DIR}"
mkdir -p "${PACKAGE_DIR}"

# Copy enterprise source code
echo "Copying enterprise source code..."
mkdir -p "${PACKAGE_DIR}/src"
mkdir -p "${PACKAGE_DIR}/include"
mkdir -p "${PACKAGE_DIR}/plugins"
mkdir -p "${PACKAGE_DIR}/examples"

cp -r src/enterprise "${PACKAGE_DIR}/src/"
cp -r include/enterprise "${PACKAGE_DIR}/include/"
cp -r plugins/enterprise "${PACKAGE_DIR}/plugins/"

# Copy examples if they exist
if [ -d "examples/gpu_impact_analysis" ]; then
    cp -r examples/gpu_impact_analysis "${PACKAGE_DIR}/examples/"
fi

# Copy tests if they exist
if [ -f "tests/test_enterprise_scalability.cpp" ]; then
    mkdir -p "${PACKAGE_DIR}/tests"
    cp tests/test_enterprise_*.cpp "${PACKAGE_DIR}/tests/" 2>/dev/null || true
    cp tests/test_gpu_impact_analysis_plugin.cpp "${PACKAGE_DIR}/tests/" 2>/dev/null || true
fi

# Create integration documentation
echo "Creating integration documentation..."
cat > "${PACKAGE_DIR}/INTEGRATION.md" << 'EOF'
# ThemisDB Enterprise Source Integration

## Quick Start

1. **Clone the community repository:**
   ```bash
   git clone https://github.com/makr-code/ThemisDB.git
   cd ThemisDB
   ```

2. **Extract this enterprise package:**
   ```bash
   tar -xzf themisdb-enterprise-VERSION.tar.gz
   cd themisdb-enterprise-VERSION
   ```

3. **Copy enterprise files to community repository:**
   ```bash
   cp -r src/enterprise ../ThemisDB/src/
   cp -r include/enterprise ../ThemisDB/include/
   cp -r plugins/enterprise ../ThemisDB/plugins/
   
   # Optional: Copy examples and tests
   cp -r examples/* ../ThemisDB/examples/ 2>/dev/null || true
   cp -r tests/* ../ThemisDB/tests/ 2>/dev/null || true
   ```

4. **Build with enterprise features:**
   ```bash
   cd ../ThemisDB
   
   # Linux/macOS
   cmake -B build -S . \
     -DTHEMIS_BUILD_ENTERPRISE=ON \
     -DTHEMIS_ENTERPRISE_SHARDING=ON \
     -DTHEMIS_ENTERPRISE_ANALYTICS=ON \
     -DTHEMIS_ENTERPRISE_REPLICATION=ON \
     -DTHEMIS_ENTERPRISE_SECURITY=ON
   
   cmake --build build --target themis_enterprise_all
   
   # Windows
   cmake -B build -S . -G "Visual Studio 17 2022" ^
     -DTHEMIS_BUILD_ENTERPRISE=ON ^
     -DTHEMIS_ENTERPRISE_SHARDING=ON
   
   cmake --build build --config Release --target themis_enterprise_all
   ```

## Directory Structure

```
themisdb-enterprise-VERSION/
├── src/enterprise/           # Enterprise implementation source
│   ├── analytics/           # OLAP/CEP analytics engine
│   ├── sharding/            # Horizontal sharding
│   ├── replication/         # HA and replication
│   ├── security/            # HSM and advanced security
│   ├── management/          # Enterprise management APIs
│   └── common/              # Shared enterprise utilities
├── include/enterprise/       # Enterprise headers
├── plugins/enterprise/       # Enterprise plugins
├── examples/                 # Enterprise examples
├── tests/                    # Enterprise tests
├── LICENSE-ENTERPRISE.txt    # Commercial license
├── INTEGRATION.md           # This file
└── README-ENTERPRISE.md     # Enterprise feature documentation
```

## Available Enterprise Modules

### Sharding Module (themis_enterprise_sharding)
- VCC-URN/PKI based sharding
- Consistent hashing
- Cross-shard joins
- Automatic rebalancing

### Analytics Module (themis_enterprise_analytics)
- OLAP engine (CUBE, ROLLUP)
- CEP streaming
- Materialized views
- Columnar storage

### Replication Module (themis_enterprise_replication)
- Leader-follower replication
- Multi-master with CRDTs
- WAL replication
- Geo-replication

### Security Module (themis_enterprise_security)
- HSM integration
- Advanced field-level encryption
- Enhanced RBAC
- Compliance reporting

### GPU Module (themis_enterprise_gpu)
- Multi-GPU support
- Advanced GPU memory management
- GPU Impact Analysis plugin

## License Validation

Enterprise features require a valid license key. Configure in:

```yaml
# config/enterprise_license.yaml
license:
  key: "YOUR-LICENSE-KEY-HERE"
  customer: "Your Organization Name"
  tier: "enterprise"
  expiry: "2026-12-31"
```

## Support

For enterprise support:
- Email: support@themisdb.com
- Portal: https://support.themisdb.com
- Phone: +1-XXX-XXX-XXXX (business hours)

## Version Information

- Enterprise Package Version: VERSION
- Compatible Community Versions: 1.3.0+
- Build Date: BUILD_DATE
EOF

# Replace VERSION placeholder
sed -i "s/VERSION/${VERSION}/g" "${PACKAGE_DIR}/INTEGRATION.md" 2>/dev/null || \
    sed -i '' "s/VERSION/${VERSION}/g" "${PACKAGE_DIR}/INTEGRATION.md"
sed -i "s/BUILD_DATE/$(date -u +%Y-%m-%d)/g" "${PACKAGE_DIR}/INTEGRATION.md" 2>/dev/null || \
    sed -i '' "s/BUILD_DATE/$(date -u +%Y-%m-%d)/g" "${PACKAGE_DIR}/INTEGRATION.md"

# Create enterprise-specific README
cat > "${PACKAGE_DIR}/README-ENTERPRISE.md" << 'EOF'
# ThemisDB Enterprise Edition

This package contains the enterprise source code for ThemisDB.

## What's Included

- Enterprise implementation modules (sharding, analytics, replication, etc.)
- Enterprise plugin source code
- Enterprise examples and tests
- Integration documentation

## Getting Started

See [INTEGRATION.md](INTEGRATION.md) for integration instructions.

## License

This code is provided under a commercial license. 
See [LICENSE-ENTERPRISE.txt](LICENSE-ENTERPRISE.txt) for terms.

## Documentation

Full documentation available at:
- https://makr-code.github.io/ThemisDB/
- https://github.com/makr-code/ThemisDB/blob/main/ENTERPRISE.md

## Support

Enterprise customers receive:
- 24/7 support
- Priority bug fixes
- Custom integrations
- Technical account manager (Hyperscaler tier)

Contact: support@themisdb.com
EOF

# Create license file
cat > "${PACKAGE_DIR}/LICENSE-ENTERPRISE.txt" << 'EOF'
ThemisDB Enterprise Edition License Agreement

Copyright (c) 2025 ThemisDB Team. All rights reserved.

This software is provided under a commercial license and is proprietary.

1. LICENSE GRANT
   Subject to the terms of this agreement and payment of applicable fees,
   licensor grants licensee a non-exclusive, non-transferable license to
   use the ThemisDB Enterprise Edition software.

2. RESTRICTIONS
   - May not redistribute, sublicense, or resell the software
   - May not remove or modify license validation mechanisms
   - May not use for competitive analysis or benchmarking without permission
   - Source code may be modified for internal use only

3. SUPPORT AND UPDATES
   - License includes support and updates for the license period
   - Updates provided via authorized channels only

4. TERMINATION
   - License terminates upon expiration or breach of terms
   - Must cease use and destroy all copies upon termination

5. WARRANTY DISCLAIMER
   SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND.

For complete terms and conditions, contact: legal@themisdb.com
License Key: [PROVIDED SEPARATELY]
EOF

# Create manifest file
cat > "${PACKAGE_DIR}/MANIFEST.txt" << EOF
ThemisDB Enterprise Source Package
Version: ${VERSION}
Build Date: $(date -u +"%Y-%m-%d %H:%M:%S UTC")
Package: ${ARCHIVE_NAME}

Contents:
EOF

# List all files
find "${PACKAGE_DIR}" -type f | sed "s|${PACKAGE_DIR}/||" | sort >> "${PACKAGE_DIR}/MANIFEST.txt"

# Create tarball
echo ""
echo "Creating archive..."
cd dist
tar -czf "${ARCHIVE_NAME}" "${PACKAGE_NAME}"
cd ..

# Calculate checksums
echo ""
echo "Generating checksums..."
if command -v sha256sum &> /dev/null; then
    sha256sum "dist/${ARCHIVE_NAME}" > "dist/${ARCHIVE_NAME}.sha256"
elif command -v shasum &> /dev/null; then
    shasum -a 256 "dist/${ARCHIVE_NAME}" > "dist/${ARCHIVE_NAME}.sha256"
fi

# Generate summary
echo ""
echo "================================================"
echo "✓ Package created successfully!"
echo "================================================"
echo ""
echo "Package: dist/${ARCHIVE_NAME}"
echo "Size: $(du -h "dist/${ARCHIVE_NAME}" | cut -f1)"
echo "SHA256: $(cat "dist/${ARCHIVE_NAME}.sha256" 2>/dev/null || echo "N/A")"
echo ""
echo "Contents:"
echo "  - $(find "${PACKAGE_DIR}/src/enterprise" -name "*.cpp" -o -name "*.h" | wc -l) source files"
echo "  - $(find "${PACKAGE_DIR}/include/enterprise" -name "*.h" | wc -l) header files"
echo "  - $(find "${PACKAGE_DIR}/plugins/enterprise" -type f | wc -l) plugin files"
echo ""
echo "Next steps:"
echo "  1. Test integration with community edition"
echo "  2. Distribute to licensed customers"
echo "  3. Update customer portal with new version"
echo ""
echo "Distribution:"
echo "  Email: support@themisdb.com"
echo "  Portal: https://enterprise.themisdb.com/downloads"
echo ""
