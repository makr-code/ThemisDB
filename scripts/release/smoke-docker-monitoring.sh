#!/usr/bin/env bash
set -euo pipefail

image="${1:?Usage: smoke-docker-monitoring.sh IMAGE}"
container=""
cleanup() {
    if [[ -n "$container" ]]; then
        docker rm -f -v "$container" >/dev/null 2>&1 || :
    fi
}
trap cleanup EXIT

if docker run --rm -e THEMIS_MONITORING_ENABLED=true "$image" --version >/dev/null 2>&1; then
    echo "Monitoring started without an explicit Grafana password" >&2
    exit 1
fi

container="$(docker run -d \
    -e THEMIS_MONITORING_ENABLED=true \
    -e GF_SECURITY_ADMIN_PASSWORD=themis-monitoring-smoke-only \
    --tmpfs /var/lib/grafana:rw,mode=0777 \
    --tmpfs /var/lib/prometheus:rw,mode=0777 \
    "$image")"

ready=false
for ((attempt=0; attempt<180; attempt++)); do
    if [[ "$(docker inspect -f '{{.State.Running}}' "$container" 2>/dev/null)" != true ]]; then
        break
    fi
    if docker exec "$container" sh -c '
        curl -fsS http://127.0.0.1:8080/health >/dev/null &&
        curl -fsS http://127.0.0.1:9091/-/ready >/dev/null &&
        curl -fsS http://127.0.0.1:3000/api/health >/dev/null &&
        curl -fsS http://127.0.0.1:9091/api/v1/targets | grep -q "\"health\":\"up\"" &&
        curl -fsS -u admin:themis-monitoring-smoke-only http://127.0.0.1:3000/api/datasources | grep -q "127.0.0.1:9091" &&
        curl -fsS -u admin:themis-monitoring-smoke-only "http://127.0.0.1:3000/api/datasources/proxy/uid/themis-prometheus/api/v1/query?query=up" | grep -q "\"job\":\"themisdb\"" &&
        curl -fsS -u admin:themis-monitoring-smoke-only http://127.0.0.1:3000/api/dashboards/uid/lora-overview | grep -q "lora-overview"
    ' 2>/dev/null; then
        ready=true
        break
    fi
    sleep 1
done

if [[ "$ready" != true ]]; then
    echo "Monitoring integration did not become ready" >&2
    docker logs "$container" >&2 || :
    exit 1
fi

echo "ThemisDB, Prometheus scrape, Grafana datasource and dashboard are ready"
docker stop --time 30 "$container" >/dev/null
exit_code="$(docker inspect -f '{{.State.ExitCode}}' "$container")"
if [[ "$exit_code" != 143 ]]; then
    echo "Monitoring container did not stop gracefully after SIGTERM (exit $exit_code)" >&2
    docker logs --tail 100 "$container" >&2 || :
    exit 1
fi
docker rm -v "$container" >/dev/null
container="$(docker run -d \
    -e THEMIS_MONITORING_ENABLED=true \
    -e GF_SECURITY_ADMIN_PASSWORD=themis-monitoring-smoke-only \
    --tmpfs /var/lib/grafana:rw,mode=0777 \
    --tmpfs /var/lib/prometheus:rw,mode=0777 \
    "$image")"

killed=false
for ((attempt=0; attempt<60; attempt++)); do
    if docker exec "$container" sh -c '
        for file in /proc/[0-9]*/comm; do
            read -r name < "$file" 2>/dev/null || continue
            if [ "$name" = prometheus ]; then
                pid=${file#/proc/}
                kill -TERM "${pid%/comm}"
                exit 0
            fi
        done
        exit 1
    ' 2>/dev/null; then
        killed=true
        break
    fi
    sleep 1
done
if [[ "$killed" != true ]]; then
    echo "Could not terminate Prometheus to test supervision" >&2
    docker logs "$container" >&2 || :
    exit 1
fi

for ((attempt=0; attempt<60; attempt++)); do
    if [[ "$(docker inspect -f '{{.State.Running}}' "$container")" == false ]]; then
        break
    fi
    sleep 1
done
running="$(docker inspect -f '{{.State.Running}}' "$container")"
exit_code="$(docker inspect -f '{{.State.ExitCode}}' "$container")"
if [[ "$running" != false || "$exit_code" == 0 ]]; then
    echo "Prometheus termination did not fail the container" >&2
    docker logs "$container" >&2 || :
    exit 1
fi
echo "Graceful shutdown and monitoring failure supervision passed"
