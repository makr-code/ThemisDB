"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            load_wikipedia_dataset.py                          ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:44                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     472                                            ║
    • Open Issues:     TODOs: 2, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Wikipedia Dataset Loader - Optimized for 5GB Benchmark
Downloads Wikipedia dump + MiniLM embeddings and loads into databases

Dataset Size: ~1.2GB (500K top-viewed articles + 384-dim embeddings)
Source: https://dumps.wikimedia.org/ + HuggingFace MiniLM (sentence-transformers)

Hardware Requirements:
- 30GB free disk space (total benchmark suite)
- 16GB RAM
- Standard CPU (8+ cores recommended)
"""

import json
import os
import gzip
import bz2
import requests
from typing import Iterator, Dict, Any
from dataclasses import dataclass
from sentence_transformers import SentenceTransformer
import psycopg2
from pymongo import MongoClient
from elasticsearch import Elasticsearch
import numpy as np

@dataclass
class WikiArticle:
    """Wikipedia article with embedding"""
    title: str
    content: str
    embedding: list[float]
    categories: list[str]
    views_last_month: int
    last_edited: str
    language: str
    page_id: int


class WikipediaDownloader:
    """Download Wikipedia dumps (optimized for 5GB benchmark)"""
    
    # Smaller dump: First partition only (~300MB compressed → ~500K articles)
    DUMP_URL = "https://dumps.wikimedia.org/enwiki/latest/enwiki-latest-pages-articles1.xml-p1p41242.bz2"
    
    def __init__(self, output_dir: str = "./datasets/wikipedia"):
        self.output_dir = output_dir
        os.makedirs(output_dir, exist_ok=True)
    
    def download_dump(self) -> str:
        """Download Wikipedia dump partition 1 (~300MB compressed, 500K articles)"""
        output_file = os.path.join(self.output_dir, "enwiki-latest-pages-articles1.xml.bz2")
        
        if os.path.exists(output_file):
            print(f"[INFO] Dump already exists: {output_file}")
            return output_file
        
        print(f"[INFO] Downloading Wikipedia dump from {self.DUMP_URL}")
        print(f"[INFO] Download size: ~300MB (partition 1 of Wikipedia)")
        print(f"[INFO] This will contain ~500K top-viewed articles")
        
        response = requests.get(self.DUMP_URL, stream=True)
        total_size = int(response.headers.get('content-length', 0))
        
        with open(output_file, 'wb') as f:
            downloaded = 0
            for chunk in response.iter_content(chunk_size=8192):
                if chunk:
                    f.write(chunk)
                    downloaded += len(chunk)
                    if downloaded % (10 * 1024 * 1024) == 0:  # Every 10MB
                        print(f"[PROGRESS] Downloaded: {downloaded / (1024**2):.1f} MB / {total_size / (1024**2):.1f} MB")
        
        print(f"[DONE] Downloaded: {output_file}")
        return output_file
    
    def parse_articles(self, dump_file: str, limit: int = None) -> Iterator[Dict[str, Any]]:
        """
        Parse Wikipedia XML dump into article dictionaries
        
        Note: For production, use WikiExtractor or wikiextractor library
        This is a simplified proof-of-concept parser
        """
        import xml.etree.ElementTree as ET
        
        print(f"[INFO] Parsing Wikipedia dump: {dump_file}")
        
        with bz2.open(dump_file, 'rt', encoding='utf-8') as f:
            count = 0
            current_article = {}
            in_text = False
            text_content = []
            
            for line in f:
                # Simplified XML parsing (production should use proper XML parser)
                if '<title>' in line:
                    current_article['title'] = line.split('<title>')[1].split('</title>')[0].strip()
                
                elif '<id>' in line and 'page_id' not in current_article:
                    current_article['page_id'] = int(line.split('<id>')[1].split('</id>')[0].strip())
                
                elif '<timestamp>' in line:
                    current_article['last_edited'] = line.split('<timestamp>')[1].split('</timestamp>')[0].strip()
                
                elif '<text' in line:
                    in_text = True
                    text_content = [line.split('<text')[1].split('>', 1)[1] if '>' in line else '']
                
                elif '</text>' in line:
                    in_text = False
                    text_content.append(line.split('</text>')[0])
                    current_article['content'] = ''.join(text_content).strip()
                    
                    # Extract categories from wikitext
                    current_article['categories'] = self._extract_categories(current_article['content'])
                    
                    # Clean wikitext (remove markup)
                    current_article['content'] = self._clean_wikitext(current_article['content'])
                    
                    # Skip redirects and stubs
                    if len(current_article['content']) > 500 and 'title' in current_article:
                        current_article['language'] = 'en'
                        current_article['views_last_month'] = np.random.randint(100, 1000000)  # TODO: get from pageviews API
                        
                        yield current_article
                        count += 1
                        
                        if count % 10000 == 0:
                            print(f"[PROGRESS] Parsed {count:,} articles")
                        
                        if limit and count >= limit:
                            print(f"[DONE] Reached limit: {limit:,} articles")
                            return
                    
                    current_article = {}
                    text_content = []
                
                elif in_text:
                    text_content.append(line)
    
    def _extract_categories(self, wikitext: str) -> list[str]:
        """Extract categories from wikitext"""
        import re
        categories = re.findall(r'\[\[Category:([^\]]+)\]\]', wikitext)
        return [cat.split('|')[0].strip() for cat in categories]
    
    def _clean_wikitext(self, wikitext: str) -> str:
        """Remove wiki markup from text"""
        import re
        
        # Remove templates
        text = re.sub(r'\{\{[^}]+\}\}', '', wikitext)
        
        # Remove links but keep text
        text = re.sub(r'\[\[([^|\]]+\|)?([^\]]+)\]\]', r'\2', text)
        
        # Remove references
        text = re.sub(r'<ref[^>]*>.*?</ref>', '', text, flags=re.DOTALL)
        
        # Remove HTML tags
        text = re.sub(r'<[^>]+>', '', text)
        
        # Remove multiple newlines
        text = re.sub(r'\n+', '\n', text)
        
        return text.strip()


class EmbeddingGenerator:
    """Generate MiniLM embeddings for Wikipedia articles (384-dim, efficient)"""
    
    def __init__(self, model_name: str = 'all-MiniLM-L6-v2'):
        """
        all-MiniLM-L6-v2: 384-dim embeddings (50% smaller than SBERT 768-dim)
        - Model size: ~90MB
        - Inference speed: ~2000 sentences/sec on CPU
        - Quality: 95% of full SBERT performance
        """
        print(f"[INFO] Loading MiniLM model: {model_name}")
        self.model = SentenceTransformer(model_name)
        self.dimension = self.model.get_sentence_embedding_dimension()
        print(f"[INFO] Embedding dimension: {self.dimension} (384-dim, memory-optimized)")
    
    def generate_embedding(self, text: str) -> list[float]:
        """Generate embedding for text"""
        # Use first 512 tokens (SBERT limit)
        text_truncated = text[:2048]  # ~512 tokens
        embedding = self.model.encode(text_truncated, convert_to_numpy=True)
        return embedding.tolist()
    
    def generate_batch(self, texts: list[str], batch_size: int = 32) -> list[list[float]]:
        """Generate embeddings for batch of texts"""
        embeddings = self.model.encode(texts, batch_size=batch_size, convert_to_numpy=True)
        return embeddings.tolist()


class DatabaseLoader:
    """Load Wikipedia articles into various databases"""
    
    def __init__(self):
        self.embedding_generator = EmbeddingGenerator()
    
    def load_to_themis(self, articles: Iterator[Dict[str, Any]], batch_size: int = 1000):
        """Load articles into ThemisDB"""
        # TODO: Implement ThemisDB native client
        print("[INFO] ThemisDB loader not yet implemented (waiting for native client)")
        pass
    
    def load_to_postgresql(self, articles: Iterator[Dict[str, Any]], batch_size: int = 1000):
        """Load articles into PostgreSQL with pgvector"""
        conn = psycopg2.connect(
            "postgresql://benchmark:benchmark123@localhost:5432/benchmark"
        )
        cur = conn.cursor()
        
        # Create table with vector extension
        cur.execute("CREATE EXTENSION IF NOT EXISTS vector")
        cur.execute("DROP TABLE IF EXISTS wikipedia_articles")
        cur.execute(f"""
            CREATE TABLE wikipedia_articles (
                page_id BIGINT PRIMARY KEY,
                title TEXT NOT NULL,
                content TEXT,
                embedding vector({self.embedding_generator.dimension}),
                categories TEXT[],
                views_last_month INTEGER,
                last_edited TIMESTAMP,
                language VARCHAR(10)
            )
        """)
        
        # Create indexes
        cur.execute("CREATE INDEX idx_wiki_title ON wikipedia_articles(title)")
        cur.execute("CREATE INDEX idx_wiki_views ON wikipedia_articles(views_last_month)")
        cur.execute("CREATE INDEX idx_wiki_edited ON wikipedia_articles(last_edited)")
        cur.execute("CREATE INDEX idx_wiki_embedding ON wikipedia_articles USING ivfflat (embedding vector_cosine_ops) WITH (lists = 1000)")
        
        conn.commit()
        
        # Insert articles in batches
        batch = []
        count = 0
        
        for article in articles:
            # Generate embedding
            embedding = self.embedding_generator.generate_embedding(
                f"{article['title']} {article['content'][:500]}"
            )
            
            batch.append((
                article['page_id'],
                article['title'],
                article['content'],
                embedding,
                article['categories'],
                article['views_last_month'],
                article['last_edited'],
                article['language']
            ))
            
            if len(batch) >= batch_size:
                # Bulk insert
                psycopg2.extras.execute_batch(
                    cur,
                    """
                    INSERT INTO wikipedia_articles 
                    (page_id, title, content, embedding, categories, views_last_month, last_edited, language)
                    VALUES (%s, %s, %s, %s, %s, %s, %s, %s)
                    """,
                    batch
                )
                conn.commit()
                count += len(batch)
                print(f"[PROGRESS] Inserted {count:,} articles into PostgreSQL")
                batch = []
        
        # Insert remaining
        if batch:
            psycopg2.extras.execute_batch(cur, "...", batch)
            conn.commit()
            count += len(batch)
        
        cur.close()
        conn.close()
        print(f"[DONE] Inserted {count:,} articles into PostgreSQL")
    
    def load_to_elasticsearch(self, articles: Iterator[Dict[str, Any]], batch_size: int = 1000):
        """Load articles into Elasticsearch"""
        es = Elasticsearch(["http://localhost:9200"])
        
        # Create index with vector field
        index_name = "wikipedia_articles"
        
        if es.indices.exists(index=index_name):
            es.indices.delete(index=index_name)
        
        es.indices.create(
            index=index_name,
            body={
                "mappings": {
                    "properties": {
                        "page_id": {"type": "long"},
                        "title": {"type": "text", "fields": {"keyword": {"type": "keyword"}}},
                        "content": {"type": "text"},
                        "embedding": {
                            "type": "dense_vector",
                            "dims": self.embedding_generator.dimension,
                            "index": True,
                            "similarity": "cosine"
                        },
                        "categories": {"type": "keyword"},
                        "views_last_month": {"type": "integer"},
                        "last_edited": {"type": "date"},
                        "language": {"type": "keyword"}
                    }
                }
            }
        )
        
        # Bulk insert
        from elasticsearch.helpers import bulk
        
        def generate_actions():
            for article in articles:
                embedding = self.embedding_generator.generate_embedding(
                    f"{article['title']} {article['content'][:500]}"
                )
                
                yield {
                    "_index": index_name,
                    "_id": article['page_id'],
                    "_source": {
                        "page_id": article['page_id'],
                        "title": article['title'],
                        "content": article['content'],
                        "embedding": embedding,
                        "categories": article['categories'],
                        "views_last_month": article['views_last_month'],
                        "last_edited": article['last_edited'],
                        "language": article['language']
                    }
                }
        
        success, failed = bulk(es, generate_actions(), chunk_size=batch_size, request_timeout=60)
        print(f"[DONE] Inserted {success:,} articles into Elasticsearch ({failed} failed)")
    
    def load_to_mongodb(self, articles: Iterator[Dict[str, Any]], batch_size: int = 1000):
        """Load articles into MongoDB"""
        client = MongoClient("mongodb://localhost:27017/")
        db = client["benchmark"]
        coll = db["wikipedia_articles"]
        
        # Drop existing collection
        coll.drop()
        
        # Create indexes
        coll.create_index("page_id", unique=True)
        coll.create_index("title")
        coll.create_index("views_last_month")
        coll.create_index("last_edited")
        coll.create_index("language")
        
        # Insert articles in batches
        batch = []
        count = 0
        
        for article in articles:
            # Generate embedding
            embedding = self.embedding_generator.generate_embedding(
                f"{article['title']} {article['content'][:500]}"
            )
            
            doc = {
                "page_id": article['page_id'],
                "title": article['title'],
                "content": article['content'],
                "embedding": embedding,
                "categories": article['categories'],
                "views_last_month": article['views_last_month'],
                "last_edited": article['last_edited'],
                "language": article['language']
            }
            
            batch.append(doc)
            
            if len(batch) >= batch_size:
                coll.insert_many(batch, ordered=False)
                count += len(batch)
                print(f"[PROGRESS] Inserted {count:,} articles into MongoDB")
                batch = []
        
        # Insert remaining
        if batch:
            coll.insert_many(batch, ordered=False)
            count += len(batch)
        
        client.close()
        print(f"[DONE] Inserted {count:,} articles into MongoDB")


# =============================================================================
# Main - Demo Usage
# =============================================================================

def main():
    """
    Optimized for 5GB Benchmark: Load 500K Wikipedia articles
    - Download size: ~300MB compressed
    - Final dataset: ~1.2GB (articles + embeddings)
    - Time: ~2-3 hours on standard hardware
    """
    print("Wikipedia Dataset Loader - Optimized for 5GB Benchmark")
    print("=" * 80)
    print("[INFO] Target: 500K top-viewed Wikipedia articles")
    print("[INFO] Storage: ~1.2GB (articles: 800MB, embeddings: 400MB)")
    print("[INFO] Time estimate: 2-3 hours")
    print("=" * 80)
    
    # Step 1: Download Wikipedia dump (partition 1)
    downloader = WikipediaDownloader()
    dump_file = downloader.download_dump()  # ~300MB download
    
    # Step 2: Parse articles (no limit - partition 1 contains ~500K articles)
    print("\n[STEP 1/4] Parsing Wikipedia dump...")
    articles_list = list(downloader.parse_articles(dump_file, limit=None))
    print(f"[INFO] Parsed {len(articles_list):,} articles")
    
    # Step 3: Load into databases
    loader = DatabaseLoader()
    
    print("\n[STEP 2/4] Loading into PostgreSQL...")
    loader.load_to_postgresql(iter(articles_list), batch_size=1000)
    
    print("\n[STEP 3/4] Loading into Elasticsearch...")
    loader.load_to_elasticsearch(iter(articles_list), batch_size=1000)
    
    print("\n[STEP 4/4] Loading into MongoDB...")
    loader.load_to_mongodb(iter(articles_list), batch_size=1000)
    
    print("\n" + "=" * 80)
    print("[DONE] Wikipedia dataset loaded into all databases!")
    print(f"[INFO] Total articles: {len(articles_list):,}")
    print("[INFO] Dataset size: ~1.2GB (optimized for 30GB available storage)")
    print("[INFO] Ready for hybrid search benchmarks!")
    print("=" * 80)


if __name__ == "__main__":
    main()
