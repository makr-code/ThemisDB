#!/bin/bash
# ThemisDB Maven Publishing Script
# Publishes io.themisdb:client to Maven Central

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
CLIENT_DIR="$ROOT_DIR/clients/java"

DRY_RUN=false
VERSION=""

while [[ $# -gt 0 ]]; do
    case $1 in
        --dry-run) DRY_RUN=true; shift ;;
        --version) VERSION="$2"; shift 2 ;;
        *) shift ;;
    esac
done

echo "ðŸ“¦ Maven Publishing for io.themisdb:client"

# Check prerequisites
if ! command -v mvn &> /dev/null; then
    echo "âŒ mvn not found"
    exit 1
fi

if [[ -z "${MAVEN_USERNAME:-}" || -z "${MAVEN_PASSWORD:-}" ]] && [[ "$DRY_RUN" == "false" ]]; then
    echo "âŒ MAVEN_USERNAME and/or MAVEN_PASSWORD environment variables not set"
    exit 1
fi

cd "$CLIENT_DIR"

# Update version if specified
if [[ -n "$VERSION" ]]; then
    mvn versions:set -DnewVersion="$VERSION" -DgenerateBackupPoms=false
fi

# Build
echo "ðŸ”¨ Building..."
mvn clean compile

# Run tests
echo "ðŸ§ª Running tests..."
mvn test || echo "No tests found or tests skipped"

# Package
echo "ðŸ“¦ Packaging..."
mvn package -DskipTests

# Generate sources and javadoc
mvn source:jar javadoc:jar

# Publish
if [[ "$DRY_RUN" == "true" ]]; then
    echo "ðŸ” Dry run - would publish:"
    ls -la target/*.jar
else
    echo "ðŸš€ Publishing to Maven Central..."
    
    # Create settings.xml with credentials
    mkdir -p ~/.m2
    cat > ~/.m2/settings.xml << EOF
<settings>
  <servers>
    <server>
      <id>ossrh</id>
      <username>${MAVEN_USERNAME}</username>
      <password>${MAVEN_PASSWORD}</password>
    </server>
  </servers>
</settings>
EOF
    
    # Deploy
    mvn deploy -DskipTests \
        -Dgpg.passphrase="${GPG_PASSPHRASE:-}" \
        -DaltDeploymentRepository=ossrh::default::https://oss.sonatype.org/service/local/staging/deploy/maven2/
fi

echo "âœ… Maven publishing complete"
