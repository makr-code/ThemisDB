#!/usr/bin/env python3
"""
Demo: ThemisDB AQL Query Emulation via Entities API
Showcases the 6 demo queries using the Entities API (since AQL query support is limited)
"""

import json
import requests
from typing import Any, Dict, List
from urllib.parse import urljoin

# Server config
API_BASE = "http://localhost:8765"
ENTITIES_ENDPOINT = urljoin(API_BASE, "/entities/")

def get_entity(key: str) -> Dict[str, Any]:
    """Retrieve an entity by key."""
    resp = requests.get(f"{ENTITIES_ENDPOINT}{key}")
    if resp.status_code == 200:
        data = resp.json()
        blob = json.loads(data.get("blob", "{}"))
        return blob
    return {}

def list_entities(collection: str) -> List[Dict[str, Any]]:
    """List all entities from a collection (via entity key pattern)."""
    # For demo: hardcoded keys
    if collection == "customers":
        keys = [f"customers:C{i:03d}" for i in range(1, 6)]
    elif collection == "products":
        keys = [f"products:P{i:03d}" for i in range(1, 6)]
    elif collection == "orders":
        keys = [f"orders:O{i:03d}" for i in range(1, 7)]
    else:
        return []
    
    entities = []
    for key in keys:
        entity = get_entity(key)
        if entity:
            entities.append(entity)
    return entities

def query_1_relational_join():
    """Query 1: Relational JOIN - customers + orders + products (filter: country == 'DE')"""
    print("\n" + "="*80)
    print("QUERY 1: Relational Join (customers + orders + products)")
    print("="*80)
    
    orders = list_entities("orders")
    customers = list_entities("customers")
    products = list_entities("products")
    
    # Join logic
    results = []
    for o in orders:
        for c in customers:
            if c.get("_key") == o.get("customer_id") and c.get("country") == "DE":
                for p in products:
                    if p.get("_key") == o.get("product_id"):
                        results.append({
                            "order_id": o.get("order_id"),
                            "customer": c.get("name"),
                            "product": p.get("name"),
                            "quantity": o.get("quantity"),
                            "total_amount": o.get("total_amount"),
                            "status": o.get("status")
                        })
    
    # Sort by order_date DESC
    results.sort(key=lambda x: x["order_id"], reverse=True)
    
    print(f"Results: {len(results)} rows")
    for row in results:
        print(f"  {row}")
    return results

def query_2_vector_search():
    """Query 2: Vector Search - semantic similarity (requires LLM embeddings)"""
    print("\n" + "="*80)
    print("QUERY 2: Vector Search (simulated - LLM embeddings not available)")
    print("="*80)
    
    products = list_entities("products")
    
    # Simulated: filter by keyword matching instead of vector similarity
    query_text = "renewable energy battery storage"
    keywords = query_text.lower().split()
    
    results = []
    for p in products:
        desc = (p.get("description", "") + " " + p.get("name", "")).lower()
        match_count = sum(1 for kw in keywords if kw in desc)
        if match_count > 0:
            results.append({
                "product_id": p.get("product_id"),
                "name": p.get("name"),
                "description": p.get("description"),
                "price": p.get("price"),
                "similarity_score": match_count / len(keywords)  # pseudo-score
            })
    
    results.sort(key=lambda x: x["similarity_score"], reverse=True)
    
    print(f"Results (keyword-based fallback): {len(results)} rows")
    for row in results:
        print(f"  {row}")
    return results

def query_3_graph_relationships():
    """Query 3: Graph Traversal - customer -> order -> product"""
    print("\n" + "="*80)
    print("QUERY 3: Graph Relationships (traversal simulation)")
    print("="*80)
    
    customers = list_entities("customers")
    orders = list_entities("orders")
    products = list_entities("products")
    
    # Build relationship graph
    results = []
    for c in customers:
        c_key = c.get("_key")
        customer_orders = [o for o in orders if o.get("customer_id") == c_key]
        
        if customer_orders:
            total_spent = 0
            for o in customer_orders:
                total_spent += o.get("total_amount", 0)
            
            results.append({
                "customer_id": c_key,
                "customer_name": c.get("name"),
                "country": c.get("country"),
                "order_count": len(customer_orders),
                "total_spent": total_spent
            })
    
    results.sort(key=lambda x: x["total_spent"], reverse=True)
    
    print(f"Results: {len(results)} customers with orders")
    for row in results:
        print(f"  {row}")
    return results

def query_4_timeseries_aggregation():
    """Query 4: Time-series Aggregation by week"""
    print("\n" + "="*80)
    print("QUERY 4: Time-Series Aggregation (weekly)")
    print("="*80)
    
    orders = list_entities("orders")
    
    # Group by week (simulated)
    from datetime import datetime
    
    weekly_stats = {}
    for o in orders:
        order_date = o.get("order_date", "")
        try:
            date = datetime.fromisoformat(order_date)
            week_start = date - timedelta(days=date.weekday())
            week_key = week_start.strftime("%Y-%m-%d")
        except:
            week_key = "unknown"
        
        if week_key not in weekly_stats:
            weekly_stats[week_key] = {
                "week_start": week_key,
                "order_count": 0,
                "total_revenue": 0,
                "orders": []
            }
        
        weekly_stats[week_key]["order_count"] += 1
        weekly_stats[week_key]["total_revenue"] += o.get("total_amount", 0)
        weekly_stats[week_key]["orders"].append(o)
    
    results = sorted(weekly_stats.values(), key=lambda x: x["week_start"], reverse=True)
    
    # Compute averages
    for row in results:
        row["avg_order_value"] = round(row["total_revenue"] / row["order_count"], 2) if row["order_count"] > 0 else 0
    
    print(f"Results: {len(results)} weeks of aggregated data")
    for row in results:
        print(f"  Week {row['week_start']}: {row['order_count']} orders, revenue ${row['total_revenue']:.2f}, avg ${row['avg_order_value']:.2f}")
    
    return results

def query_5_llm_inference():
    """Query 5: Inline LLM INFER (simulated - LLM not available)"""
    print("\n" + "="*80)
    print("QUERY 5: Inline LLM INFER (simulated - LLM not available)")
    print("="*80)
    
    products = list_entities("products")
    
    # Simulate LLM output for P001
    p001 = next((p for p in products if p.get("product_id") == "P001"), None)
    if p001:
        # Simulated LLM output
        simulated_pitch = (
            "High-performance solar energy solution perfect for residential installations. "
            "400W peak output, weather-resistant design, 25-year warranty. "
            "Ideal for reducing energy costs."
        )
        
        result = {
            "product": p001.get("name"),
            "price": p001.get("price"),
            "pitch": simulated_pitch,
            "note": "LLM inference not available - using simulated output"
        }
        
        print(f"Results (simulated LLM output):")
        print(f"  {result}")
        return [result]
    
    return []

def query_6_rag_hallucination_detection():
    """Query 6: RAG with hallucination detection (simulated)"""
    print("\n" + "="*80)
    print("QUERY 6: RAG with Hallucination Detection (simulated)")
    print("="*80)
    
    products = list_entities("products")
    
    # Question: "What energy solutions do we offer?"
    # Top 5 products by relevance
    question = "What energy solutions do we offer?"
    
    results = []
    for p in products:
        # Simulate RAG retrieval and scoring
        relevance_score = 0.8 + (len(p.get("description", "")) / 1000)  # pseudo-scoring
        results.append({
            "product_id": p.get("product_id"),
            "name": p.get("name"),
            "category": p.get("category"),
            "description": p.get("description"),
            "relevance": min(1.0, relevance_score),
            "hallucination_check": "passed" if relevance_score >= 0.75 else "flagged"
        })
    
    results = results[:5]  # TOP 5
    results.sort(key=lambda x: x["relevance"], reverse=True)
    
    print(f"Question: '{question}'")
    print(f"Results (TOP 5 with hallucination check):")
    for row in results:
        print(f"  {row}")
    
    return results

def main():
    """Run all demo queries."""
    print("\n")
    print("╔" + "="*78 + "╗")
    print("║" + " "*78 + "║")
    print("║" + "  ThemisDB Kickstarter Demo - 6 Query Showcases (v1.0)".center(78) + "║")
    print("║" + " "*78 + "║")
    print("╚" + "="*78 + "╝")
    
    try:
        # Test connection
        health = requests.get(f"{API_BASE}/health").json()
        print(f"\n✓ Server healthy: {health['status']} (uptime: {health['uptime_seconds']}s)")
        print(f"  License: {health['license']['edition']}")
    except Exception as e:
        print(f"\n✗ Server connection failed: {e}")
        return
    
    try:
        from datetime import timedelta
    except:
        timedelta = None
    
    # Run all queries
    queries = [
        ("Query 1: Relational Join", query_1_relational_join),
        ("Query 2: Vector Search", query_2_vector_search),
        ("Query 3: Graph Relationships", query_3_graph_relationships),
        ("Query 4: Time-Series Aggregation", query_4_timeseries_aggregation),
        ("Query 5: LLM Inference", query_5_llm_inference),
        ("Query 6: RAG + Hallucination Detection", query_6_rag_hallucination_detection),
    ]
    
    for name, query_func in queries:
        try:
            query_func()
        except Exception as e:
            print(f"\n✗ {name} failed: {e}")
    
    print("\n" + "="*80)
    print("Demo Complete!")
    print("="*80 + "\n")

if __name__ == "__main__":
    main()
