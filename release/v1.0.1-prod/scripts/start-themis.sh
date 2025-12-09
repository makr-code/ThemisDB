#!/bin/bash
# THEMIS Server Startup Script (Linux/QNAP)
# Version: 1.0.1

set -e

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Directories
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
THEMIS_ROOT="$(dirname "$SCRIPT_DIR")"
BIN_DIR="$THEMIS_ROOT/bin"
DATA_DIR="$THEMIS_ROOT/data"
LOGS_DIR="$THEMIS_ROOT/logs"
CACHE_DIR="$THEMIS_ROOT/cache"
CONFIG_FILE="$THEMIS_ROOT/config.json"
PID_FILE="/tmp/themis_server.pid"

# Function to print messages
print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to create required directories
setup_directories() {
    print_info "Setting up directories..."
    
    for dir in "$DATA_DIR" "$LOGS_DIR" "$CACHE_DIR"; do
        if [ ! -d "$dir" ]; then
            mkdir -p "$dir"
            chmod 755 "$dir"
            print_info "Created: $dir"
        fi
    done
}

# Function to verify configuration
verify_config() {
    if [ ! -f "$CONFIG_FILE" ]; then
        print_error "Configuration file not found: $CONFIG_FILE"
        return 1
    fi
    print_info "Configuration verified: $CONFIG_FILE"
    return 0
}

# Function to verify binary
verify_binary() {
    if [ ! -f "$BIN_DIR/themis_server" ]; then
        print_error "Binary not found: $BIN_DIR/themis_server"
        return 1
    fi
    if [ ! -x "$BIN_DIR/themis_server" ]; then
        print_warn "Binary not executable, fixing permissions..."
        chmod +x "$BIN_DIR/themis_server"
    fi
    print_info "Binary verified: $BIN_DIR/themis_server"
    return 0
}

# Function to check if port is available
check_port() {
    local port=${1:-8080}
    if netstat -tuln 2>/dev/null | grep -q ":$port "; then
        print_error "Port $port is already in use"
        return 1
    fi
    print_info "Port $port is available"
    return 0
}

# Function to start server
start_server() {
    print_info "Starting THEMIS Server v1.0.1..."
    
    # Change to root directory for relative paths
    cd "$THEMIS_ROOT"
    
    # Start server in background
    "$BIN_DIR/themis_server" > "$LOGS_DIR/application.log" 2>&1 &
    local pid=$!
    echo $pid > "$PID_FILE"
    
    print_info "Server started with PID: $pid"
    
    # Wait a moment for startup
    sleep 2
    
    # Check if process is still running
    if ! kill -0 $pid 2>/dev/null; then
        print_error "Server failed to start. Check logs:"
        tail -20 "$LOGS_DIR/application.log"
        return 1
    fi
    
    return 0
}

# Function to verify server is running
verify_startup() {
    print_info "Verifying server startup..."
    
    local retries=5
    local count=0
    
    while [ $count -lt $retries ]; do
        if curl -s http://localhost:8080/health > /dev/null 2>&1; then
            print_info "✓ Server is healthy"
            return 0
        fi
        count=$((count + 1))
        if [ $count -lt $retries ]; then
            print_warn "Waiting for server... (attempt $count/$retries)"
            sleep 1
        fi
    done
    
    print_error "Server did not respond to health check after $retries seconds"
    return 1
}

# Function to show server info
show_info() {
    echo ""
    echo "=========================================="
    echo "  THEMIS Server Started Successfully"
    echo "=========================================="
    echo "Version:        1.0.1"
    echo "Root:           $THEMIS_ROOT"
    echo "Binary:         $BIN_DIR/themis_server"
    echo "Configuration:  $CONFIG_FILE"
    echo "Data:           $DATA_DIR"
    echo "Logs:           $LOGS_DIR"
    echo ""
    echo "Server available at:"
    echo "  HTTP:  http://localhost:8080"
    echo "  gRPC:  grpc://localhost:50051"
    echo ""
    echo "Useful commands:"
    echo "  Check health:    curl http://localhost:8080/health"
    echo "  View logs:       tail -f $LOGS_DIR/application.log"
    echo "  Stop server:     kill $(cat $PID_FILE)"
    echo "  View metrics:    curl http://localhost:8080/metrics"
    echo "=========================================="
    echo ""
}

# Main execution
main() {
    # Parse arguments
    case "${1:-start}" in
        start)
            print_info "THEMIS v1.0.1 Server Startup"
            setup_directories
            verify_config || exit 1
            verify_binary || exit 1
            check_port 8080 || exit 1
            start_server || exit 1
            verify_startup || exit 1
            show_info
            ;;
        foreground)
            print_info "THEMIS v1.0.1 Server (Foreground Mode)"
            setup_directories
            verify_config || exit 1
            verify_binary || exit 1
            check_port 8080 || exit 1
            cd "$THEMIS_ROOT"
            print_info "Starting in foreground... (Press Ctrl+C to stop)"
            "$BIN_DIR/themis_server"
            ;;
        stop)
            if [ -f "$PID_FILE" ]; then
                local pid=$(cat "$PID_FILE")
                if kill -0 $pid 2>/dev/null; then
                    print_info "Stopping server (PID: $pid)..."
                    kill -TERM $pid
                    sleep 1
                    rm -f "$PID_FILE"
                    print_info "Server stopped"
                else
                    print_warn "Server not running"
                    rm -f "$PID_FILE"
                fi
            else
                print_warn "No PID file found"
            fi
            ;;
        status)
            if [ -f "$PID_FILE" ]; then
                local pid=$(cat "$PID_FILE")
                if kill -0 $pid 2>/dev/null; then
                    print_info "Server is running (PID: $pid)"
                    curl -s http://localhost:8080/health | jq . || echo "Health check failed"
                else
                    print_error "Server not running (stale PID file)"
                fi
            else
                print_error "Server not running"
            fi
            ;;
        *)
            echo "Usage: $0 {start|foreground|stop|status}"
            echo ""
            echo "Commands:"
            echo "  start      - Start server in background"
            echo "  foreground - Start server in foreground (for debugging)"
            echo "  stop       - Stop the running server"
            echo "  status     - Show server status"
            exit 1
            ;;
    esac
}

main "$@"
