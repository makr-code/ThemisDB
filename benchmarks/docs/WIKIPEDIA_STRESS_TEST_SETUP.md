> ⚠️ **Historisches Stress-Test-Setup** – Konfiguration beschreibt einen bestimmten Teststand.

# 📚 Wikipedia Stress Test - ThemisDB v1.0.1

**Status:** 🔄 IN PREPARATION  
**Dataset Size:** 24 GB (complete Wikipedia dump)  
**Test Type:** Maximum load & stress testing  
**Date:** 2025-12-09  

---

## 📥 Dataset Preparation

### Wikipedia Dump Source

#### Available Options
```
1. English Wikipedia (en)
   - Size: ~20GB (compressed XML)
   - Size: ~90GB (uncompressed)
   - Articles: ~6.7 million
   - Format: XML with MediaWiki markup
   - Source: dumps.wikimedia.org

2. German Wikipedia (de)
   - Size: ~4GB (compressed XML)
   - Size: ~20GB (uncompressed)
   - Articles: ~2.6 million
   - Format: XML with MediaWiki markup
   - Source: dumps.wikimedia.org

3. Multi-language combined
   - Size: ~24GB (as specified)
   - Combination of multiple language versions
   - Total articles: ~9 million+
```

#### Selected Configuration
```
PRIMARY:      English Wikipedia dump
COMPRESSED:   bz2 format (optimal balance)
SIZE:         ~20GB compressed, ~90GB uncompressed
ARTICLES:     6.7 million
REVISIONS:    Limited to latest version
DOWNLOAD:     Via torrent or direct HTTP
```

### Download Instructions

#### Method 1: Direct HTTP Download (Recommended for your setup)
```bash
# Create data directory
mkdir -p /mnt/wikipedia-data
cd /mnt/wikipedia-data

# Download latest English Wikipedia dump (latest-pages-articles.xml.bz2)
# From: https://dumps.wikimedia.org/enwiki/latest/

# Download directly (using wget/curl)
wget https://dumps.wikimedia.org/enwiki/latest/enwiki-latest-pages-articles.xml.bz2 \
  --continue \
  --progress=dot:mega \
  --output-document=wikipedia-dump.xml.bz2

# Verify checksum
wget https://dumps.wikimedia.org/enwiki/latest/enwiki-latest-pages-articles.xml.bz2.sha1
sha1sum -c enwiki-latest-pages-articles.xml.bz2.sha1
```

#### Method 2: Torrent Download
```bash
# Download torrent file
wget https://dumps.wikimedia.org/enwiki/latest/enwiki-latest-pages-articles.xml.bz2.torrent

# Use transmission-cli (lightweight)
transmission-cli enwiki-latest-pages-articles.xml.bz2.torrent \
  --download-dir /mnt/wikipedia-data \
  --umask 0
```

#### Method 3: Partial Download (Quick Start)
```bash
# Download only first 100 articles (for testing)
# Using bzip2 partial decompression

bunzip2 -c wikipedia-dump.xml.bz2 | head -c 1GB > wikipedia-sample-1gb.xml
```

### Data Extraction & Parsing

#### Extraction Process
```bash
# Option A: Parallel decompression (fastest)
pbzip2 -d -p 8 wikipedia-dump.xml.bz2  # 8 parallel processes

# Option B: Standard decompression
bunzip2 -v wikipedia-dump.xml.bz2

# Result: wikipedia-dump.xml (~90GB)
```

#### XML Structure Analysis
```xml
<?xml version="1.0" encoding="UTF-8"?>
<mediawiki xmlns="http://www.mediawiki.org/xml/export-0.11/" 
           xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
  <siteinfo>
    <mainpage>Main Page</mainpage>
    <base>https://en.wikipedia.org/wiki/Main_Page</base>
    <generator>MediaWiki 1.41.0-wmf.17</generator>
    <case>first-letter</case>
    <namespaces>
      <!-- Namespace definitions (0-828 namespaces) -->
    </namespaces>
  </siteinfo>
  
  <page>
    <title>Article Title</title>
    <ns>0</ns>
    <id>12345</id>
    <revision>
      <id>987654321</id>
      <timestamp>2025-12-09T10:30:45Z</timestamp>
      <contributor>
        <username>Editor</username>
        <id>54321</id>
      </contributor>
      <comment>Edit comment</comment>
      <model>wikitext</model>
      <format>text/x-wiki</format>
      <text bytes="5000">Article content with [[links]]...</text>
      <sha1>abc123def456...</sha1>
    </revision>
  </page>
  
  <!-- 6.7 million more pages... -->
</mediawiki>
```

#### Key Fields to Extract
```
Articles:
  - title (string, ~100 bytes avg)
  - article_id (integer)
  - namespace (integer)
  - text (large text, ~5-10KB avg)
  - revision_id (integer)
  - timestamp (datetime)
  - contributor_name (string)
  - contributor_id (integer)
  - edit_comment (string, ~100 bytes)
  - size_bytes (integer)
  - sha1_hash (string, 40 chars)
  
Relations:
  - Links between articles (from [[Article]] references)
  - Categories (from [[Category:...]] references)
  - Templates (from {{template}} references)
  
Full-text Content:
  - Raw wikitext markup
  - Section headers
  - Paragraphs
  - Lists and structured data
```

### Data Transformation Pipeline

#### Step 1: Parse XML to Structured Format
```python
# Parse Wikipedia XML and extract key fields
import xml.etree.ElementTree as ET
import json

def parse_wikipedia_dump(xml_file, output_format='json'):
    """
    Parse Wikipedia XML dump and convert to structured format
    
    Args:
        xml_file: Path to uncompressed XML file
        output_format: 'json', 'csv', or 'sql'
    
    Returns:
        Generator yielding article records
    """
    tree = ET.iterparse(xml_file, events=['end'])
    
    for event, elem in tree:
        if elem.tag.endswith('}page'):
            article = extract_article(elem)
            if article['namespace'] == 0:  # Main namespace only
                yield article
            elem.clear()

def extract_article(page_elem):
    """Extract article data from page element"""
    ns = page_elem.find('.//{http://www.mediawiki.org/xml/export-0.11/}ns')
    title = page_elem.find('.//{http://www.mediawiki.org/xml/export-0.11/}title')
    page_id = page_elem.find('.//{http://www.mediawiki.org/xml/export-0.11/}id')
    revision = page_elem.find('.//{http://www.mediawiki.org/xml/export-0.11/}revision')
    
    return {
        'title': title.text if title is not None else '',
        'article_id': int(page_id.text) if page_id is not None else 0,
        'namespace': int(ns.text) if ns is not None else 0,
        'revision_id': int(revision.find('.//{http://www.mediawiki.org/xml/export-0.11/}id').text),
        'timestamp': revision.find('.//{http://www.mediawiki.org/xml/export-0.11/}timestamp').text,
        'contributor': revision.find('.//{http://www.mediawiki.org/xml/export-0.11/}username').text or 'Anonymous',
        'comment': revision.find('.//{http://www.mediawiki.org/xml/export-0.11/}comment').text or '',
        'text': revision.find('.//{http://www.mediawiki.org/xml/export-0.11/}text').text or '',
    }
```

#### Step 2: Convert to Database Format

**For Relational Databases (PostgreSQL, MySQL):**
```sql
CREATE TABLE wikipedia_articles (
    article_id BIGINT PRIMARY KEY,
    title VARCHAR(255) NOT NULL,
    namespace SMALLINT DEFAULT 0,
    text LONGTEXT NOT NULL,
    size_bytes INTEGER,
    created_date TIMESTAMP,
    last_modified TIMESTAMP,
    contributor_name VARCHAR(255),
    contributor_id BIGINT,
    INDEX idx_title (title),
    INDEX idx_namespace (namespace),
    FULLTEXT INDEX ft_text (text)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE wikipedia_links (
    source_article_id BIGINT,
    target_article_id BIGINT,
    link_type ENUM('internal', 'category', 'template'),
    PRIMARY KEY (source_article_id, target_article_id),
    FOREIGN KEY (source_article_id) REFERENCES wikipedia_articles(article_id),
    FOREIGN KEY (target_article_id) REFERENCES wikipedia_articles(article_id),
    INDEX idx_target (target_article_id)
) ENGINE=InnoDB;

CREATE TABLE wikipedia_revisions (
    revision_id BIGINT PRIMARY KEY,
    article_id BIGINT,
    timestamp TIMESTAMP,
    contributor_id BIGINT,
    contributor_name VARCHAR(255),
    comment TEXT,
    text_hash VARCHAR(40),
    FOREIGN KEY (article_id) REFERENCES wikipedia_articles(article_id),
    INDEX idx_article_time (article_id, timestamp),
    INDEX idx_contributor (contributor_id)
) ENGINE=InnoDB;
```

**For Document Databases (MongoDB):**
```javascript
db.wikipedia_articles.createIndex({
    "title": 1,
    "namespace": 1
});

db.wikipedia_articles.createIndex({
    "text": "text"
});

db.wikipedia_articles.insertMany([
    {
        article_id: 12345,
        title: "Article Title",
        namespace: 0,
        text: "Full article content...",
        revision_id: 987654321,
        timestamp: ISODate("2025-12-09T10:30:45Z"),
        contributor: {
            name: "Editor",
            id: 54321
        },
        comment: "Edit comment",
        links: {
            internal: ["Article1", "Article2"],
            categories: ["Category1"],
            templates: ["Template1"]
        },
        metadata: {
            size_bytes: 5000,
            sha1: "abc123def456..."
        }
    }
]);
```

**For Vector Databases (Milvus, Qdrant):**
```python
# Generate embeddings for full-text search
from sentence_transformers import SentenceTransformer

model = SentenceTransformer('all-MiniLM-L6-v2')

articles = read_wikipedia_dump()
for article in articles:
    # Generate embedding for article text (first 512 tokens)
    text_summary = article['text'][:5000]
    embedding = model.encode(text_summary)  # 384-dimensional
    
    # Store in vector database
    vector_db.insert({
        'id': article['article_id'],
        'title': article['title'],
        'embedding': embedding.tolist(),
        'metadata': {
            'namespace': article['namespace'],
            'contributor': article['contributor']
        }
    })
```

### Data Size & Distribution

```
Total Dataset Size:        24 GB (as specified)

Breakdown by Component:
  - Articles XML:          ~90 GB (raw, uncompressed)
  - Text content only:     ~60 GB
  - Metadata:              ~10 GB
  - Links/relations:       ~5 GB
  - Revisions history:     (optional, very large)

Articles:                  6.7 million
Average article size:      ~9 KB
Range:                     100 bytes - 5 MB
Largest articles:          Disambiguation pages, lists
Smallest articles:         Stub articles

Distribution by length:
  - < 1 KB:                15% (2.0M articles)
  - 1-10 KB:               60% (4.0M articles)
  - 10-100 KB:             20% (1.3M articles)
  - 100+ KB:               5% (0.4M articles)
```

---

## 🔧 Database Configuration for Wikipedia

### ThemisDB Configuration
```yaml
# themis.yaml
database:
  name: wikipedia
  encoding: UTF-8
  
storage:
  data_dir: /data/themisdb-wikipedia
  index_type: HNSW  # For full-text search embeddings
  max_segments: 16
  
memory:
  buffer_pool: 8GB
  cache_size: 4GB
  
indexing:
  full_text:
    enabled: true
    tokenizer: standard
    min_term_length: 3
  spatial:
    enabled: false
  vector:
    enabled: true
    dimensions: 384
    
performance:
  thread_pool_size: 16
  connection_pool: 50
  batch_insert_size: 1000
```

### PostgreSQL Configuration
```ini
# postgresql.conf
max_connections = 200
shared_buffers = 8GB
effective_cache_size = 12GB
work_mem = 128MB
maintenance_work_mem = 2GB
wal_buffers = 16MB
max_worker_processes = 8
max_parallel_workers = 8
max_parallel_workers_per_gather = 4

# Full-text search
fsm_maintenance_work_mem = 2GB
```

### MongoDB Configuration
```javascript
// mongod.conf
storage:
  engine: wiredTiger
  wiredTiger:
    cacheSizeGB: 8
    engineConfig:
      journalCompressor: snappy
      
systemLog:
  logAppend: true
  verbosity: 0
  
net:
  maxIncomingConnections: 100000
  
operationProfiling:
  mode: slowOp
  slowOpThresholdMs: 100
```

### Elasticsearch Configuration
```yaml
# elasticsearch.yml
cluster.name: wikipedia-benchmark
node.name: es-wikipedia
node.roles: [data, ingest]

discovery.type: single-node
xpack.security.enabled: false

indices.memory.index_buffer_size: 30%
indices.queries.cache.size: 20%

threadpool:
  search:
    size: 32
    queue_size: 1000
  write:
    size: 16
    queue_size: 500
  
index:
  number_of_replicas: 0
  number_of_shards: 8
  refresh_interval: 30s
```

---

## 📊 Stress Test Workloads

### Workload 1: Full-Text Search Stress
```
Operations:
  - Simple keyword search: "machine learning"
  - Phrase search: "artificial intelligence"
  - Boolean search: ("Python" AND "programming") NOT "tutorial"
  - Wildcard search: "algo*"
  - Fuzzy search: "algoritm" (typo tolerance)
  
Metrics:
  - Query latency (p50, p95, p99)
  - Throughput (queries/sec)
  - Memory usage
  - Index size

Load Pattern:
  - Initial load: 1,000 queries/sec
  - Peak load: 5,000 queries/sec
  - Duration: 600 seconds
  - Concurrent clients: 50-100
```

### Workload 2: Large Document Insert/Update
```
Operations:
  - Bulk insert 100k articles
  - Update article text (50MB per update)
  - Insert revision history
  - Index creation on large tables
  
Metrics:
  - Insert throughput (articles/sec)
  - Update latency
  - Index creation time
  - Memory peak usage
  - Disk I/O rate

Load Pattern:
  - Batch size: 1,000-10,000 articles
  - Total: 6.7M articles
  - Duration: Until complete
  - Parallel workers: 8-16
```

### Workload 3: Complex Queries
```
Operations:
  - Aggregations (count by namespace)
  - JOIN queries (articles + links + revisions)
  - Text search with filters
  - Complex sorting on large result sets
  
Example Queries (SQL):
  SELECT a.title, COUNT(l.target_id) as link_count
  FROM wikipedia_articles a
  LEFT JOIN wikipedia_links l ON a.article_id = l.source_id
  WHERE a.text LIKE '%quantum%'
  GROUP BY a.article_id
  HAVING link_count > 10
  ORDER BY link_count DESC
  LIMIT 1000;
  
  -- Search with pagination
  SELECT * FROM wikipedia_articles
  WHERE MATCH(text) AGAINST(''+deep learning+' IN BOOLEAN MODE)
  AND namespace = 0
  ORDER BY article_id
  LIMIT 10000 OFFSET 50000;

Metrics:
  - Query execution time
  - Result set size
  - Memory for aggregation
  - Join performance
```

### Workload 4: Concurrent Mixed Operations
```
Operations Mix:
  - 40% read (search/select)
  - 30% write (insert/update)
  - 20% complex queries
  - 10% transactions
  
Concurrent Clients: 100-200
Total Operations: 1M+
Duration: 1800 seconds (30 minutes)

Metrics:
  - Combined throughput
  - Latency distribution
  - Error rate
  - Lock contention
  - Resource utilization
```

### Workload 5: Vector Search at Scale
```
Scenario:
  - Generate embeddings for 6.7M articles
  - Similarity search queries
  - Approximate nearest neighbor (ANN) search
  - KNN queries with filters

Queries:
  - Find top 10 most similar articles to "Machine Learning"
  - ANN search with 50 neighbors
  - Hybrid search: full-text + vector similarity
  - Vector search with metadata filters

Metrics:
  - Embedding generation time
  - Search latency (p50, p95, p99)
  - Throughput (queries/sec)
  - Recall score (for ANN)
  - Memory for index
```

---

## 🚀 Implementation Steps

### Phase 1: Data Preparation (Week 1-2)
```
1. [ ] Download Wikipedia dump (24GB)
   - Parallel download (8 connections)
   - Estimated time: 4-8 hours depending on bandwidth
   - Verify checksum
   
2. [ ] Extract and parse XML
   - Parallel decompression (8 cores)
   - Stream parsing to avoid memory overload
   - Estimated time: 2-4 hours
   
3. [ ] Convert to database formats
   - Generate SQL inserts
   - Convert to JSON for MongoDB
   - Generate embeddings (with GPU acceleration if possible)
   - Estimated time: 4-6 hours per database
   
4. [ ] Validate data integrity
   - Record count verification
   - Size verification
   - Sample random checks
```

### Phase 2: Database Loading (Week 2-3)
```
1. [ ] Create indexes
   - Full-text indexes: 1-2 hours each
   - Spatial indexes: optional
   - Vector indexes: 2-4 hours per database
   - Estimated total: 10-15 hours
   
2. [ ] Bulk insert data
   - Batch loading with optimization
   - Monitor resource usage
   - Estimated time: 1-2 hours per database
   
3. [ ] Verify database integrity
   - Query samples
   - Index effectiveness checks
   - Statistics collection
```

### Phase 3: Stress Testing (Week 3-4)
```
1. [ ] Run Workload 1: Full-text search
   - Baseline establishment
   - Performance characterization
   
2. [ ] Run Workload 2: Large inserts
   - Concurrent write stress
   - Index maintenance impact
   
3. [ ] Run Workload 3: Complex queries
   - Multi-table joins
   - Aggregation performance
   
4. [ ] Run Workload 4: Mixed operations
   - Real-world simulation
   - Sustained load testing
   
5. [ ] Run Workload 5: Vector search
   - Embedding performance
   - Search accuracy/speed tradeoff
   
6. [ ] Generate reports
   - Comparative analysis
   - Bottleneck identification
   - Optimization recommendations
```

---

## 📈 Expected Results & Benchmarking

### Stress Test Metrics Collection

```python
# Metrics to track per workload
metrics = {
    'throughput': 'operations/sec',
    'latency': {
        'mean': 'ms',
        'p50': 'ms',
        'p95': 'ms',
        'p99': 'ms',
        'max': 'ms'
    },
    'resource_usage': {
        'cpu_percent': '%',
        'memory_mb': 'MB',
        'disk_io_mb_s': 'MB/s',
        'network_mb_s': 'MB/s'
    },
    'errors': {
        'count': 'total',
        'rate': '%'
    },
    'quality': {
        'result_accuracy': '%',
        'cache_hit_rate': '%',
        'index_efficiency': '%'
    }
}
```

### Performance Expectations

| Database | Full-Text Search | Bulk Insert | Complex Query | Mixed Ops |
|----------|------------------|------------|---------------|-----------|
| **ThemisDB** | 2000+ q/s | 50k art/s | <100ms | 5000 ops/s |
| **PostgreSQL** | 1500 q/s | 30k art/s | 150-200ms | 3000 ops/s |
| **MongoDB** | 1200 q/s | 40k art/s | 200-300ms | 2500 ops/s |
| **Elasticsearch** | 1800 q/s | 60k art/s (bulk) | 80-150ms | 4000 ops/s |
| **MySQL** | 1000 q/s | 25k art/s | 200-400ms | 2000 ops/s |

### Scalability Indicators

```
Metrics to measure:
  - Linear scaling up to how many concurrent clients?
  - Memory growth rate (MB per 1M articles)
  - CPU efficiency (ops/sec per core)
  - Disk space efficiency (compression ratio)
  - Cache effectiveness (hit rate)
```

---

## 📋 Pre-Test Checklist

### Infrastructure
- [ ] Disk space: 200GB+ available (90GB for uncompressed + indexes + temp)
- [ ] Memory: 32GB+ (16GB per Docker allocation should be sufficient)
- [ ] CPU: 8+ cores dedicated
- [ ] Network: 1Gbps minimum for download
- [ ] Cooling: Adequate (sustained CPU load)

### Software
- [ ] Docker: Latest version (29.1+)
- [ ] Python: 3.13+ with dependencies installed
- [ ] All databases: Latest stable versions pulled
- [ ] Tools: bzip2, pbzip2, wget/curl available

### Data
- [ ] Wikipedia dump: Downloaded and verified
- [ ] Extracted XML: ~90GB available
- [ ] Conversion scripts: Ready
- [ ] Test data samples: Prepared

### Monitoring
- [ ] System monitoring tools: htop, iostat, iotop
- [ ] Database monitoring: Enabled
- [ ] Log collection: Configured
- [ ] Result storage: Prepared

---

## 🔄 Automated Setup Script

```python
#!/usr/bin/env python3
# setup_wikipedia_stress_test.py

import os
import subprocess
import json
from pathlib import Path

class WikipediaStressTestSetup:
    def __init__(self, data_dir: str = "/mnt/wikipedia-data"):
        self.data_dir = Path(data_dir)
        self.data_dir.mkdir(exist_ok=True)
    
    def download_wikipedia(self):
        """Download Wikipedia dump with resume capability"""
        url = "https://dumps.wikimedia.org/enwiki/latest/enwiki-latest-pages-articles.xml.bz2"
        output = self.data_dir / "wikipedia-dump.xml.bz2"
        
        cmd = [
            'wget',
            '--continue',  # Resume partial downloads
            '--progress=dot:mega',
            '--output-document', str(output),
            url
        ]
        
        subprocess.run(cmd, check=True)
        return output
    
    def extract_wikipedia(self, bz2_file: Path):
        """Parallel extraction of Wikipedia dump"""
        output = self.data_dir / "wikipedia-dump.xml"
        
        # Use pbzip2 for parallel decompression
        cmd = f'pbzip2 -d -p 8 -c {bz2_file} > {output}'
        subprocess.run(cmd, shell=True, check=True)
        
        return output
    
    def parse_wikipedia(self, xml_file: Path):
        """Parse Wikipedia XML to structured format"""
        # Implementation details...
        pass
    
    def load_databases(self):
        """Load processed Wikipedia data into all databases"""
        # Implementation details...
        pass
    
    def run_stress_tests(self):
        """Execute all stress test workloads"""
        # Implementation details...
        pass

if __name__ == "__main__":
    setup = WikipediaStressTestSetup()
    setup.download_wikipedia()
    setup.extract_wikipedia()
    setup.parse_wikipedia()
    setup.load_databases()
    setup.run_stress_tests()
```

---

## 📝 Expected Output

### Test Report Structure
```
wikipedia_stress_test_report_YYYYMMDD_HHMMSS/
├── summary.json                          # Overview metrics
├── detailed_results.csv                  # All measurements
├── analysis.md                           # Text analysis
├── databases/
│   ├── themisdb/
│   │   ├── workload_1_fulltext.json
│   │   ├── workload_2_insert.json
│   │   ├── workload_3_complex.json
│   │   ├── workload_4_mixed.json
│   │   └── workload_5_vector.json
│   ├── postgresql/
│   ├── mongodb/
│   ├── elasticsearch/
│   └── mysql/
└── visualizations/
    ├── throughput_comparison.png
    ├── latency_distribution.png
    ├── resource_usage.png
    └── competitive_analysis.png
```

---

## 🎯 Next Steps

1. **Download Wikipedia Dump**
   - Start parallel download (estimated: 4-8 hours)
   - Verify checksum after completion

2. **Extract and Parse**
   - Run parallel extraction script
   - Convert to database formats

3. **Load into Databases**
   - Create schema and indexes
   - Bulk insert Wikipedia data

4. **Execute Stress Tests**
   - Run all 5 workloads sequentially
   - Collect comprehensive metrics

5. **Generate Analysis**
   - Create comparative reports
   - Identify optimization opportunities
   - Document findings

---

**Status:** Ready for Wikipedia stress test implementation! 🚀📚
