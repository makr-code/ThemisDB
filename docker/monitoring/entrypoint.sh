#!/usr/bin/env bash
set -u

case "${THEMIS_MONITORING_ENABLED:-false}" in
    false|0) exec "$@" ;;
    true|1) ;;
    *) echo "Invalid THEMIS_MONITORING_ENABLED: use true or false" >&2; exit 1 ;;
esac

if [[ -z "${GF_SECURITY_ADMIN_PASSWORD:-}" && -z "${GF_SECURITY_ADMIN_PASSWORD__FILE:-}" ]]; then
    echo "Monitoring requires GF_SECURITY_ADMIN_PASSWORD or GF_SECURITY_ADMIN_PASSWORD__FILE" >&2
    exit 1
fi

scrape_port="${THEMIS_MONITORING_SCRAPE_PORT:-${THEMIS_PORT:-8080}}"
if [[ ! "$scrape_port" =~ ^[0-9]{1,5}$ ]] || (( 10#$scrape_port < 1 || 10#$scrape_port > 65535 )); then
    echo "Invalid monitoring scrape port: $scrape_port" >&2
    exit 1
fi

config_dir="$(mktemp -d)" || exit 1
cat > "$config_dir/prometheus.yml" <<EOF
global:
  scrape_interval: 10s
scrape_configs:
  - job_name: themisdb
    static_configs:
      - targets: ["127.0.0.1:$scrape_port"]
EOF

export GF_PATHS_HOME=/usr/share/grafana
export GF_PATHS_CONFIG=/etc/grafana/grafana.ini
export GF_PATHS_DATA=/var/lib/grafana
export GF_PATHS_LOGS=/var/log/grafana
export GF_PATHS_PLUGINS=/var/lib/grafana/plugins
export GF_PATHS_PROVISIONING=/etc/themis-monitoring/grafana
export GF_PLUGINS_PREINSTALL_DISABLED="${GF_PLUGINS_PREINSTALL_DISABLED:-true}"
export GF_PLUGINS_PREINSTALL_AUTO_UPDATE="${GF_PLUGINS_PREINSTALL_AUTO_UPDATE:-false}"
export GF_ANALYTICS_REPORTING_ENABLED="${GF_ANALYTICS_REPORTING_ENABLED:-false}"
export GF_ANALYTICS_CHECK_FOR_UPDATES="${GF_ANALYTICS_CHECK_FOR_UPDATES:-false}"
export GF_ANALYTICS_CHECK_FOR_PLUGIN_UPDATES="${GF_ANALYTICS_CHECK_FOR_PLUGIN_UPDATES:-false}"
export PATH="/usr/share/grafana/bin:$PATH"

pids=()
cleanup() {
    trap - TERM INT
    if ((${#pids[@]})); then
        kill "${pids[@]}" 2>/dev/null || :
        wait "${pids[@]}" 2>/dev/null || :
    fi
    rm -f "$config_dir/prometheus.yml"
    rmdir "$config_dir"
}
trap 'cleanup; exit 143' TERM
trap 'cleanup; exit 130' INT

"$@" &
pids+=("$!")
/usr/local/bin/prometheus \
    --config.file="$config_dir/prometheus.yml" \
    --storage.tsdb.path=/var/lib/prometheus \
    --web.listen-address=0.0.0.0:9091 &
pids+=("$!")
/usr/local/bin/grafana-upstream-run &
pids+=("$!")

status=0
wait -n -p exited "${pids[@]}" || status=$?
if [[ "${exited:-}" != "${pids[0]}" && "$status" -eq 0 ]]; then
    status=1
fi
echo "Container process ${exited:-unknown} exited (status $status); stopping remaining processes" >&2
cleanup
exit "$status"
