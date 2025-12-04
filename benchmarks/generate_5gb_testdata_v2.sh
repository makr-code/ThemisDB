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
            local random_text=$(head -c 256 </dev/urandom | base64 2>/dev/null | head -c 200)
            
            cat << JSONEOF
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
JSONEOF
            
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
    
    while [ $current_size -lt $target_size ]; do
        dd if=/dev/urandom bs=4096 count=1 2>/dev/null >> "$output_file"
        current_size=$((current_size + 4096))
        
        if [ $((current_size / 1024 / 1024 % 100)) -eq 0 ]; then
            echo "  Generated $(($current_size / 1024 / 1024))MB..." >&2
        fi
    done
    
    echo "  ✓ Binary generated: $(ls -lh "$output_file" | awk '{print $5}')"
}

generate_binary $((1500 * 1024 * 1024)) "$OUTPUT_DIR/data_binary.bin"

# ============================================================================
# Generate Metadata
# ============================================================================
echo ""
echo "4. Generating metadata..."

cat > "$OUTPUT_DIR/TESTDATA_INFO.txt" << 'INFOEOF'
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

═══════════════════════════════════════════════════════════════
INFOEOF

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
echo "  2. Run load tests with: bash ../loadtest_5gb_massive.sh $OUTPUT_DIR"
echo ""
