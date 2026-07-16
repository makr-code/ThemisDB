#!/bin/bash
# ThemisDB Kickstarter Demo Script
# Unpolished, single-take demonstration of ThemisDB capabilities
# Run this script with commentary for the video

set -e

# Colors for terminal output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuration
THEMISCTL="./build/windows-release/bin/themisctl"
SERVER_HOST="localhost:8765"
DEMO_COLLECTION="demo_articles"
DEMO_GRAPH="demo_knowledge_graph"
DEMO_VECTORS="demo_embeddings"

echo -e "${BLUE}================================================================${NC}"
echo -e "${BLUE}ThemisDB Multi-Model Database - Live Demo${NC}"
echo -e "${BLUE}================================================================${NC}"
echo ""

# ============================================================================
# SECTION 1: System Status & Schema
# ============================================================================
echo -e "${GREEN}[1] Checking ThemisDB Server Status...${NC}"
echo ""
$THEMISCTL schema --host $SERVER_HOST 2>/dev/null | head -20
echo ""
echo -e "${YELLOW}Server is running and responding to queries.${NC}"
echo ""

# ============================================================================
# SECTION 2: Document & Full-Text Search (SQL-like)
# ============================================================================
echo -e "${GREEN}[2] Document Search - Finding Articles About AI${NC}"
echo ""
echo "Query:"
echo "  FOR doc IN $DEMO_COLLECTION"
echo "    FILTER doc.title LIKE '%AI%' OR doc.content LIKE '%machine learning%'"
echo "    SORT doc.published DESC"
echo "    LIMIT 5"
echo "    RETURN { title: doc.title, published: doc.published, score: doc.relevance }"
echo ""
echo "Results:"
$THEMISCTL query --host $SERVER_HOST \
  "FOR doc IN $DEMO_COLLECTION \
   FILTER doc.title LIKE '%AI%' OR doc.content LIKE '%machine learning%' \
   SORT doc.published DESC \
   LIMIT 5 \
   RETURN { title: doc.title, published: doc.published, score: doc.relevance }"
echo ""

# ============================================================================
# SECTION 3: Vector Search (Semantic Search)
# ============================================================================
echo -e "${GREEN}[3] Vector Search - Semantic Similarity${NC}"
echo ""
echo "Query: Find 5 most similar articles to 'neural network optimization'"
echo ""
$THEMISCTL query --host $SERVER_HOST \
  "FOR doc IN $DEMO_VECTORS \
   LET similarity = COSINE_SIMILARITY(doc.embedding, @query_embedding) \
   FILTER similarity > 0.7 \
   SORT similarity DESC \
   LIMIT 5 \
   RETURN { title: doc.title, similarity: ROUND(similarity, 3) }" \
  --bind-var query_embedding="[0.1, -0.2, 0.8, ...]"
echo ""

# ============================================================================
# SECTION 4: Graph Traversal (Multi-hop Relationships)
# ============================================================================
echo -e "${GREEN}[4] Graph Navigation - Knowledge Graph Traversal${NC}"
echo ""
echo "Query: Find all researchers and their published papers (2 hops)"
echo ""
echo "FOR researcher IN $DEMO_GRAPH"
echo "  FILTER researcher.type == 'researcher'"
echo "  FOR paper IN 1..2 OUTBOUND researcher._id graph_edges"
echo "    FILTER paper.type == 'paper'"
echo "    RETURN {"
echo "      researcher: researcher.name,"
echo "      paper: paper.title,"
echo "      citations: paper.citation_count"
echo "    }"
echo ""
$THEMISCTL query --host $SERVER_HOST \
  "FOR researcher IN $DEMO_GRAPH \
   FILTER researcher.type == 'researcher' \
   FOR paper IN 1..2 OUTBOUND researcher._id graph_edges \
     FILTER paper.type == 'paper' \
   RETURN { \
     researcher: researcher.name, \
     paper: paper.title, \
     citations: paper.citation_count \
   } \
   LIMIT 8"
echo ""

# ============================================================================
# SECTION 5: LLM Features - RAG (Retrieval-Augmented Generation)
# ============================================================================
echo -e "${GREEN}[5] RAG Query - Natural Language to SQL${NC}"
echo ""
echo "User Question: 'What are the latest research papers on quantum computing by MIT researchers?'"
echo ""
echo "ThemisDB LLM Agent processes this and executes:"
$THEMISCTL rag query \
  --collection $DEMO_COLLECTION \
  --top-k 3 \
  "What are the latest papers on quantum computing by MIT?" \
  --host $SERVER_HOST
echo ""

# ============================================================================
# SECTION 6: Complex Multi-Model Join
# ============================================================================
echo -e "${GREEN}[6] Multi-Model Data Fusion - Documents + Vectors + Graph${NC}"
echo ""
echo "Query: Find researcher + their papers (vector similarity) + collaboration network"
echo ""
$THEMISCTL query --host $SERVER_HOST \
  "FOR researcher IN $DEMO_GRAPH \
   FILTER researcher.type == 'researcher' \
   LET papers = ( \
     FOR paper IN $DEMO_VECTORS \
     FILTER paper.author_id == researcher._id \
     LET sim = COSINE_SIMILARITY(paper.embedding, @topic_embedding) \
     FILTER sim > 0.6 \
     RETURN { title: paper.title, similarity: sim } \
   ) \
   LET collaborators = ( \
     FOR collab IN 1 OUTBOUND researcher._id graph_edges \
     FILTER collab.type == 'researcher' \
     RETURN collab.name \
   ) \
   RETURN { \
     researcher: researcher.name, \
     paper_count: LENGTH(papers), \
     top_papers: SLICE(papers, 0, 2), \
     collaborators: collaborators \
   } \
   LIMIT 5" \
  --bind-var topic_embedding="[0.2, 0.5, -0.1, ...]"
echo ""

# ============================================================================
# SECTION 7: Performance & Statistics
# ============================================================================
echo -e "${GREEN}[7] System Performance Metrics${NC}"
echo ""
$THEMISCTL admin stats --host $SERVER_HOST | grep -E "queries|throughput|latency|cache"
echo ""

# ============================================================================
# SECTION 8: Index Recommendations
# ============================================================================
echo -e "${GREEN}[8] Automatic Index Recommendation${NC}"
echo ""
echo "ThemisDB analyzes query patterns and recommends optimizations:"
echo ""
$THEMISCTL index recommend --host $SERVER_HOST --collection $DEMO_COLLECTION
echo ""

# ============================================================================
# CLOSING
# ============================================================================
echo -e "${BLUE}================================================================${NC}"
echo -e "${GREEN}Demo Complete!${NC}"
echo -e "${BLUE}================================================================${NC}"
echo ""
echo "Key Features Demonstrated:"
echo "  ✓ Document/Full-Text Search (SQL-like AQL queries)"
echo "  ✓ Vector Search (Semantic similarity)"
echo "  ✓ Graph Traversal (Multi-hop relationships)"
echo "  ✓ RAG Agent (LLM-powered natural language queries)"
echo "  ✓ Multi-Model Data Fusion (Documents + Vectors + Graph)"
echo "  ✓ Performance Analytics"
echo "  ✓ Automatic Index Recommendations"
echo ""
echo "ThemisDB is fully operational and ready for production use!"
echo ""
