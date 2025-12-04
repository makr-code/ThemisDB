#!/bin/bash
# Real-time monitoring dashboard for ThemisDB benchmarks
# Shows: CPU, Memory, Network, Connections, Throughput

THEMIS_HTTP="http://localhost:8765"
THEMIS_WIRE_PORT="8766"

clear

while true; do
    clear
    
    echo "╔════════════════════════════════════════════════════════════════════════════╗"
    echo "║             ThemisDB Real-Time Monitoring Dashboard                        ║"
    echo "╚════════════════════════════════════════════════════════════════════════════╝"
    echo ""
    echo "Timestamp: $(date '+%Y-%m-%d %H:%M:%S')"
    echo ""
    
    # ========== Container Status ==========
    echo "┌─ Docker Container Status ─────────────────────────────────────────────────┐"
    docker stats themis-wire --no-stream 2>/dev/null | tail -2 || echo "Container not found"
    echo "└──────────────────────────────────────────────────────────────────────────┘"
    echo ""
    
    # ========== HTTP Health Check ==========
    echo "┌─ HTTP REST API Status ────────────────────────────────────────────────────┐"
    health=$(curl -s -w "%{http_code}" -o /tmp/health.json "$THEMIS_HTTP/health" 2>/dev/null)
    if [ "$health" = "200" ]; then
        echo "✓ HTTP Server: HEALTHY (200 OK)"
        cat /tmp/health.json | jq '.' 2>/dev/null || echo "  $(cat /tmp/health.json)"
    else
        echo "✗ HTTP Server: UNAVAILABLE (HTTP $health)"
    fi
    echo "└──────────────────────────────────────────────────────────────────────────┘"
    echo ""
    
    # ========== Wire Protocol Status ==========
    echo "┌─ Wire Protocol Status ────────────────────────────────────────────────────┐"
    if timeout 1 bash -c "echo '' | nc -q 0 localhost $THEMIS_WIRE_PORT" 2>/dev/null; then
        echo "✓ Wire Protocol: LISTENING (Port $THEMIS_WIRE_PORT)"
    else
        echo "✗ Wire Protocol: NOT LISTENING"
    fi
    echo "└──────────────────────────────────────────────────────────────────────────┘"
    echo ""
    
    # ========== Performance Snapshot ==========
    echo "┌─ Recent Benchmark Results ────────────────────────────────────────────────┐"
    if [ -d "benchmark_results_20251204_194321" ]; then
        echo "Latest Benchmark (Connection Establishment Time):"
        head -5 benchmark_results_20251204_194321/connection_times.csv 2>/dev/null | \
            awk -F',' '{printf "  %-20s %10s μs\n", $1, $2}' || echo "  (Results loading...)"
    else
        echo "  (No benchmark results yet)"
    fi
    echo "└──────────────────────────────────────────────────────────────────────────┘"
    echo ""
    
    # ========== System Info ==========
    echo "┌─ System Information ──────────────────────────────────────────────────────┐"
    echo "  CPU Usage: $(top -bn1 2>/dev/null | grep "Cpu(s)" | awk '{print $2}' || echo "N/A")"
    echo "  Memory Used: $(free -h 2>/dev/null | awk 'NR==2 {print $3}' || echo "N/A")"
    echo "  Network (Docker): $(docker exec themis-wire netstat -s 2>/dev/null | grep -i "packets" | head -2 || echo "N/A")"
    echo "└──────────────────────────────────────────────────────────────────────────┘"
    echo ""
    
    # ========== Active Tests ==========
    echo "┌─ Active Benchmark Processes ──────────────────────────────────────────────┐"
    ps aux | grep -E "benchmark_suite|loadtest_5gb|stress_test" | grep -v grep | \
        awk '{printf "  %s (PID: %d)\n", $11, $2}' || echo "  (No active tests)"
    echo "└──────────────────────────────────────────────────────────────────────────┘"
    echo ""
    
    # ========== Quick Commands ==========
    echo "┌─ Available Commands ──────────────────────────────────────────────────────┐"
    echo "  [1] View HTTP logs:              tail -f /var/log/themis_http.log"
    echo "  [2] View Wire Protocol logs:     docker exec themis-wire tail -f /app/wire.log"
    echo "  [3] Check connections:           netstat -an | grep 876"
    echo "  [4] View memory usage:           docker exec themis-wire free -h"
    echo "  [Q] Quit"
    echo "└──────────────────────────────────────────────────────────────────────────┘"
    echo ""
    echo "Press Ctrl+C to exit or wait for next refresh (30s)..."
    
    sleep 30
done
