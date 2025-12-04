#!/bin/bash
# Generate 5GB of diverse test data for polyglot database comparison
# Formats: JSON (Documents), CSV (Relational), Binary (BSON-like)

set -e

OUTPUT_DIR="testdata_5gb_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$OUTPUT_DIR"

TARGET_SIZE_GB=5
TARGET_SIZE_BYTES=$((TARGET_SIZE_GB * 1024 * 1024 * 1024))

echo "=== ThemisDB 5GB Testdata Generator ==="
echo "Target: ${TARGET_SIZE_GB}GB of diverse data"
echo "Output: $OUTPUT_DIR"
echo ""

# ============================================================================
# 1. Generate JSON Document Dataset (2GB)
# ============================================================================
echo "1. Generating JSON Dataset (2GB)..."

generate_json() {
    local target_size=$1
    local output_file=$2
    local current_size=0
    local record_count=0
    
    {
        echo "["
        
        while [ $current_size -lt $target_size ]; do
            local id=$((record_count + 1))
            local timestamp=$(date +%s)000
            local random_int=$((RANDOM * RANDOM % 1000000))
            local random_text=$(head -c 256 </dev/urandom | base64)
            
            cat << EOF
  {
    "id": $id,
    "timestamp": $timestamp,
    "user_id": $((id % 10000 + 1)),
    "event_type": "event_$(($id % 5))",
    "metrics": {
      "cpu": $((RANDOM % 100)),
      "memory": $((RANDOM % 100)),
      "disk": $((RANDOM % 100)),
      "network": $((RANDOM % 1000))
    },
    "tags": [
      "tag_$(($id % 100))",
      "tag_$(($id % 50))",
      "tag_$(($id % 10))"
    ],
    "data": "$random_text",
    "nested": {
      "level2": {
        "level3": {
          "value": $random_int
        }
      }
    }
  }$([ $((current_size + 2048)) -lt $target_size ] && echo "," || echo "")"
            
            current_size=$((current_size + 2048))
            record_count=$((record_count + 1))
            
            if [ $((record_count % 10000)) -eq 0 ]; then
                echo "  Generated $record_count records ($(($current_size / 1024 / 1024))MB)..." >&2
            fi
        done
        
        echo "]"
    } > "$output_file"
    
    echo "  ✓ JSON generated: $(ls -lh "$output_file" | awk '{print $5}') ($record_count records)"
}

generate_json $((2 * 1024 * 1024 * 1024)) "$OUTPUT_DIR/data_documents.json"

# ============================================================================
# 2. Generate CSV Dataset (1.5GB)
# ============================================================================
echo ""
echo "2. Generating CSV Dataset (1.5GB)..."

generate_csv() {
    local target_size=$1
    local output_file=$2
    local current_size=0
    local record_count=0
    
    {
        # Header
        echo "id,timestamp,user_id,event_type,cpu,memory,disk,network,tags,data_hash"
        
        while [ $current_size -lt $target_size ]; do
            local id=$((record_count + 1))
            local timestamp=$(date +%s)000
            local tags="tag_$(($id % 100)),tag_$(($id % 50)),tag_$(($id % 10))"
            local data_hash=$(echo "$id:$timestamp" | md5sum | awk '{print $1}')
            
            echo "$id,$timestamp,$((id % 10000 + 1)),event_$(($id % 5)),$((RANDOM % 100)),$((RANDOM % 100)),$((RANDOM % 100)),$((RANDOM % 1000)),$tags,$data_hash"
            
            current_size=$((current_size + 256))
            record_count=$((record_count + 1))
            
            if [ $((record_count % 100000)) -eq 0 ]; then
                echo "  Generated $record_count records ($(($current_size / 1024 / 1024))MB)..." >&2
            fi
        done
    } > "$output_file"
    
    echo "  ✓ CSV generated: $(ls -lh "$output_file" | awk '{print $5}') ($record_count records)"
}

generate_csv $((1500 * 1024 * 1024)) "$OUTPUT_DIR/data_relational.csv"

# ============================================================================
# 3. Generate Mixed Binary Dataset (1.5GB)
# ============================================================================
echo ""
echo "3. Generating Mixed Binary Dataset (1.5GB)..."

generate_binary() {
    local target_size=$1
    local output_file=$2
    local current_size=0
    
    {
        while [ $current_size -lt $target_size ]; do
            # Generate random 4KB chunks of binary data
            dd if=/dev/urandom bs=4096 count=1 2>/dev/null
            current_size=$((current_size + 4096))
            
            if [ $((current_size / 1024 / 1024 % 100)) -eq 0 ]; then
                echo "  Generated $(($current_size / 1024 / 1024))MB..." >&2
            fi
        done
    } > "$output_file"
    
    echo "  ✓ Binary generated: $(ls -lh "$output_file" | awk '{print $5}')"
}

generate_binary $((1500 * 1024 * 1024)) "$OUTPUT_DIR/data_binary.bin"

# ============================================================================
# Generate Statistics and Metadata
# ============================================================================
echo ""
echo "4. Generating metadata..."

cat > "$OUTPUT_DIR/TESTDATA_INFO.txt" << 'EOF'
═══════════════════════════════════════════════════════════════
        ThemisDB 5GB Testdata Package - Polyglot Comparison
═══════════════════════════════════════════════════════════════

Dataset Overview:
─────────────────
Total Size: 5GB
Components:
  • data_documents.json   - 2GB  (Unstructured documents, 1M+ records)
  • data_relational.csv   - 1.5GB (Tabular data, 5M+ records)
  • data_binary.bin       - 1.5GB (Raw binary/blob data)

Data Characteristics:
───────────────────
• Nested structures (4-level deep objects)
• Time-series data (timestamps, metrics)
• Multiple data types (strings, numbers, arrays)
• Real-world scale (millions of records)

Use Cases:
──────────
1. Document Database Testing (MongoDB, ThemisDB)
   - Complex nested queries
   - Full-text search simulation
   - Array operations

2. Relational Database Testing (PostgreSQL)
   - Join operations
   - Aggregations
   - Index performance

3. Binary/Blob Storage Testing (S3, Azure Blob, Redis)
   - Large object handling
   - Compression efficiency
   - Stream processing

Database Setup Commands:
────────────────────────

ThemisDB (Wire Protocol):
  time curl -X POST http://localhost:8765/bulk-insert \
    -H "Content-Type: application/json" \
    -d @data_documents.json

PostgreSQL:
  psql -U postgres -d benchmark_db -c "COPY data FROM 'data_relational.csv' WITH CSV HEADER"

MongoDB:
  mongoimport --db benchmark_db --collection data --type json --file data_documents.json

Redis:
  cat data_binary.bin | redis-cli -x SET large_dataset

ElasticSearch:
  curl -X POST "localhost:9200/data/_bulk?pretty" --data-binary @data_documents.json

═══════════════════════════════════════════════════════════════
EOF

cat > "$OUTPUT_DIR/LOAD_SCRIPTS.sh" << 'EOF'
#!/bin/bash
# Database loading scripts for 5GB testdata

TESTDATA_DIR="$(pwd)"

# ============== ThemisDB HTTP REST ===============
load_themis_http() {
    echo "Loading into ThemisDB HTTP REST..."
    time curl -X POST http://localhost:8765/bulk-insert \
        -H "Content-Type: application/json" \
        -d "$(cat $TESTDATA_DIR/data_documents.json)" \
        -v 2>&1 | tee themis_http_load.log
}

# ============== ThemisDB Wire Protocol ===============
load_themis_wire() {
    echo "Loading into ThemisDB Wire Protocol..."
    # Will use Python client for Wire Protocol
    python3 << 'PYTHON_EOF'
import socket
import struct
import time

def load_wire_protocol(host, port, data_file):
    start = time.time()
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((host, port))
        
        with open(data_file, 'rb') as f:
            chunk_size = 64 * 1024
            bytes_sent = 0
            while True:
                chunk = f.read(chunk_size)
                if not chunk:
                    break
                sock.sendall(chunk)
                bytes_sent += len(chunk)
                if bytes_sent % (10 * 1024 * 1024) == 0:
                    print(f"Sent {bytes_sent / (1024*1024):.1f}MB...")
        
        sock.close()
        elapsed = time.time() - start
        print(f"\n✓ Wire Protocol load complete: {bytes_sent / (1024*1024):.1f}MB in {elapsed:.2f}s")
        print(f"  Throughput: {bytes_sent / (1024*1024*elapsed):.1f} MB/s")
    except Exception as e:
        print(f"Error: {e}")

load_wire_protocol('localhost', 8766, '$TESTDATA_DIR/data_binary.bin')
PYTHON_EOF
}

# ============== PostgreSQL ===============
load_postgres() {
    echo "Loading into PostgreSQL..."
    psql -h localhost -U postgres -d benchmark_db << SQL
CREATE TABLE IF NOT EXISTS data (
    id SERIAL PRIMARY KEY,
    timestamp BIGINT,
    user_id INTEGER,
    event_type VARCHAR(50),
    cpu INTEGER,
    memory INTEGER,
    disk INTEGER,
    network INTEGER,
    tags TEXT,
    data_hash VARCHAR(32)
);

\COPY data(id,timestamp,user_id,event_type,cpu,memory,disk,network,tags,data_hash) 
FROM '$TESTDATA_DIR/data_relational.csv' WITH (FORMAT csv, HEADER true);

CREATE INDEX idx_user_id ON data(user_id);
CREATE INDEX idx_timestamp ON data(timestamp);
SQL
}

# ============== MongoDB ===============
load_mongodb() {
    echo "Loading into MongoDB..."
    mongoimport \
        --host localhost \
        --db benchmark_db \
        --collection data \
        --type json \
        --file "$TESTDATA_DIR/data_documents.json" \
        --batchSize 10000
}

# ============== ElasticSearch ===============
load_elasticsearch() {
    echo "Loading into ElasticSearch..."
    curl -X POST "localhost:9200/data/_bulk?pretty" \
        --header "Content-Type: application/json" \
        --data-binary "@$TESTDATA_DIR/data_documents.json"
}

# ============== Main Menu ===============
echo "Select database to load:"
echo "1) ThemisDB HTTP"
echo "2) ThemisDB Wire Protocol"
echo "3) PostgreSQL"
echo "4) MongoDB"
echo "5) ElasticSearch"
echo "6) All (sequential)"

read -p "Choice: " choice

case $choice in
    1) load_themis_http ;;
    2) load_themis_wire ;;
    3) load_postgres ;;
    4) load_mongodb ;;
    5) load_elasticsearch ;;
    6) 
        load_themis_http
        load_themis_wire
        load_postgres
        load_mongodb
        load_elasticsearch
        ;;
    *) echo "Invalid choice" ;;
esac
EOF

chmod +x "$OUTPUT_DIR/LOAD_SCRIPTS.sh"

# ============================================================================
# Final Summary
# ============================================================================
echo ""
echo "✓ Testdata generation complete!"
echo ""
echo "Generated files in: $OUTPUT_DIR"
ls -lh "$OUTPUT_DIR/"
echo ""
echo "Total size: $(du -sh "$OUTPUT_DIR" | awk '{print $1}')"
echo ""
echo "Next steps:"
echo "  1. cd $OUTPUT_DIR"
echo "  2. ./LOAD_SCRIPTS.sh  # Select database to load"
echo "  3. Monitor with: docker stats themis-wire"
echo ""
