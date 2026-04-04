#!/bin/bash
# ThemisDB Comparative Benchmark - Wait for Databases
#
# This script waits for all benchmark databases to become available.
# Usage: ./wait_for_databases.sh [timeout_seconds]

set -e

TIMEOUT=${1:-300}  # Default 5 minutes
START_TIME=$(date +%s)

echo "Waiting for databases to become available (timeout: ${TIMEOUT}s)..."

# Function to check if a service is ready
check_service() {
    local name=$1
    local host=$2
    local port=$3
    local check_cmd=$4
    
    if eval "$check_cmd" &>/dev/null; then
        echo "âœ“ $name is ready"
        return 0
    else
        return 1
    fi
}

# Wait for ThemisDB
wait_for_themisdb() {
    local host=${THEMISDB_HOST:-localhost}
    local port=${THEMISDB_PORT:-8765}
    check_service "ThemisDB" "$host" "$port" \
        "curl -fsS http://${host}:${port}/health"
}

# Wait for PostgreSQL
wait_for_postgresql() {
    local host=${POSTGRESQL_HOST:-localhost}
    local port=${POSTGRESQL_PORT:-5432}
    check_service "PostgreSQL" "$host" "$port" \
        "pg_isready -h ${host} -p ${port} -U benchmark"
}

# Wait for MongoDB
wait_for_mongodb() {
    local host=${MONGODB_HOST:-localhost}
    local port=${MONGODB_PORT:-27017}
    check_service "MongoDB" "$host" "$port" \
        "mongosh --host ${host} --port ${port} --eval 'db.adminCommand(\"ping\")'"
}

# Wait for Redis
wait_for_redis() {
    local host=${REDIS_HOST:-localhost}
    local port=${REDIS_PORT:-6379}
    check_service "Redis" "$host" "$port" \
        "redis-cli -h ${host} -p ${port} ping"
}

# Wait for ArangoDB
wait_for_arangodb() {
    local host=${ARANGODB_HOST:-localhost}
    local port=${ARANGODB_PORT:-8529}
    check_service "ArangoDB" "$host" "$port" \
        "curl -fsS http://${host}:${port}/_api/version"
}

# Wait for Neo4j
wait_for_neo4j() {
    local host=${NEO4J_HOST:-localhost}
    local port=${NEO4J_HTTP_PORT:-7474}
    check_service "Neo4j" "$host" "$port" \
        "curl -fsS http://${host}:${port}"
}

# Wait for Milvus
wait_for_milvus() {
    local host=${MILVUS_HOST:-localhost}
    local port=${MILVUS_PORT:-19530}
    check_service "Milvus" "$host" "$port" \
        "curl -fsS http://${host}:9091/healthz"
}

# Wait for Elasticsearch
wait_for_elasticsearch() {
    local host=${ELASTICSEARCH_HOST:-localhost}
    local port=${ELASTICSEARCH_PORT:-9200}
    check_service "Elasticsearch" "$host" "$port" \
        "curl -fsS http://${host}:${port}/_cluster/health"
}

# Main wait loop
services=(
    "wait_for_themisdb"
    "wait_for_postgresql"
    "wait_for_mongodb"
    "wait_for_redis"
    "wait_for_arangodb"
    "wait_for_neo4j"
    "wait_for_milvus"
    "wait_for_elasticsearch"
)

pending_services=("${services[@]}")

while [ ${#pending_services[@]} -gt 0 ]; do
    CURRENT_TIME=$(date +%s)
    ELAPSED=$((CURRENT_TIME - START_TIME))
    
    if [ $ELAPSED -ge $TIMEOUT ]; then
        echo "Timeout reached! The following services are still not ready:"
        for svc in "${pending_services[@]}"; do
            echo "  - ${svc#wait_for_}"
        done
        exit 1
    fi
    
    new_pending=()
    for svc in "${pending_services[@]}"; do
        if ! $svc 2>/dev/null; then
            new_pending+=("$svc")
        fi
    done
    
    pending_services=("${new_pending[@]}")
    
    if [ ${#pending_services[@]} -gt 0 ]; then
        echo "Still waiting for ${#pending_services[@]} service(s)... (${ELAPSED}s elapsed)"
        sleep 5
    fi
done

echo ""
echo "All databases are ready!"
echo "Total wait time: ${ELAPSED}s"
