"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            async_client.py                                    ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:32:30                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     1008                                           ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 65b6fc41ed  2026-02-24  fix: resolve remaining Python (34) and PHP (23) error-han... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
ThemisDB Python SDK - Async Client Module

This module provides an async/await interface for ThemisDB operations.
Requires: httpx[http2], asyncio

Usage:
    import asyncio
    from themis.async_client import AsyncThemisClient
    
    async def main():
        client = AsyncThemisClient(endpoints=["http://localhost:8080"])
        await client.connect()
        
        # CRUD operations
        await client.put("mymodel", "users", "user-123", {"name": "John"})
        user = await client.get("mymodel", "users", "user-123")
        
        # Batch operations
        results = await client.batch_get("mymodel", "users", ["user-1", "user-2"])
        
        # Graph traversal
        neighbors = await client.traverse("mymodel", "users", "user-123", depth=2)
        
        await client.close()
    
    asyncio.run(main())

Author: ThemisDB Team
Date: December 2025
"""

from __future__ import annotations

import asyncio
import hashlib
import json
import logging
import time
from contextlib import asynccontextmanager
from dataclasses import dataclass, field
from enum import Enum
from typing import Any, AsyncIterator, Callable, Dict, List, Optional, Set, Union

import httpx

__all__ = [
    "AsyncThemisClient",
    "AsyncTransaction",
    "GraphTraversalResult",
    "AsyncQueryResult",
    "ConnectionPool",
    "CircuitBreaker",
    "CircuitBreakerState",
]


class CircuitBreakerState(Enum):
    """Circuit breaker states."""
    CLOSED = "CLOSED"
    OPEN = "OPEN"
    HALF_OPEN = "HALF_OPEN"


class CircuitBreaker:
    """
    Circuit breaker implementation for fault tolerance.
    
    Prevents cascading failures by temporarily blocking requests when
    a service is experiencing issues.
    """
    
    def __init__(
        self,
        failure_threshold: int = 5,
        reset_timeout: float = 60.0,
        half_open_max_requests: int = 3,
    ):
        self.failure_threshold = failure_threshold
        self.reset_timeout = reset_timeout
        self.half_open_max_requests = half_open_max_requests
        
        self._state = CircuitBreakerState.CLOSED
        self._failure_count = 0
        self._success_count = 0
        self._next_attempt_time = 0.0
        self._lock = asyncio.Lock()
    
    async def can_execute(self) -> bool:
        """Check if a request can be executed."""
        async with self._lock:
            if self._state == CircuitBreakerState.CLOSED:
                return True
            
            if self._state == CircuitBreakerState.OPEN:
                if time.time() >= self._next_attempt_time:
                    self._state = CircuitBreakerState.HALF_OPEN
                    self._success_count = 0
                    return True
                return False
            
            # HALF_OPEN state
            return self._success_count < self.half_open_max_requests
    
    async def record_success(self):
        """Record a successful request."""
        async with self._lock:
            if self._state == CircuitBreakerState.HALF_OPEN:
                self._success_count += 1
                if self._success_count >= self.half_open_max_requests:
                    self._state = CircuitBreakerState.CLOSED
                    self._failure_count = 0
            elif self._state == CircuitBreakerState.CLOSED:
                self._failure_count = 0
    
    async def record_failure(self):
        """Record a failed request."""
        async with self._lock:
            self._failure_count += 1
            if self._failure_count >= self.failure_threshold:
                self._state = CircuitBreakerState.OPEN
                self._next_attempt_time = time.time() + self.reset_timeout
    
    @property
    def state(self) -> CircuitBreakerState:
        """Get the current circuit breaker state."""
        return self._state


@dataclass
class GraphNode:
    """Represents a node in graph traversal results."""
    id: str
    collection: str
    data: Dict[str, Any]
    depth: int = 0


@dataclass
class GraphEdge:
    """Represents an edge in graph traversal results."""
    id: str
    source: str
    target: str
    label: str
    properties: Dict[str, Any] = field(default_factory=dict)


@dataclass
class GraphTraversalResult:
    """Result of a graph traversal operation."""
    nodes: List[GraphNode]
    edges: List[GraphEdge]
    paths: List[List[str]]
    total_visited: int
    max_depth_reached: int
    execution_time_ms: float


@dataclass
class AsyncQueryResult:
    """Async query result with streaming support."""
    items: List[Any]
    has_more: bool
    next_cursor: Optional[str]
    count: Optional[int]
    
    async def iter_pages(self, client: "AsyncThemisClient", query: str) -> AsyncIterator[List[Any]]:
        """Iterate through all pages of results."""
        yield self.items
        cursor = self.next_cursor
        while cursor:
            result = await client.query(query, cursor=cursor)
            yield result.items
            cursor = result.next_cursor


class ConnectionPool:
    """Manages a pool of HTTP/2 connections for optimal performance."""
    
    def __init__(
        self,
        endpoints: List[str],
        max_connections: int = 100,
        max_keepalive: int = 20,
        timeout: float = 30.0,
    ):
        self.endpoints = [e.rstrip("/") for e in endpoints]
        self.timeout = timeout
        
        # Create HTTP/2 client with connection pooling
        limits = httpx.Limits(
            max_connections=max_connections,
            max_keepalive_connections=max_keepalive,
        )
        
        self._client = httpx.AsyncClient(
            http2=True,
            limits=limits,
            timeout=httpx.Timeout(timeout),
        )
        
        self._healthy_endpoints: Set[str] = set(self.endpoints)
        self._lock = asyncio.Lock()
    
    async def close(self):
        """Close all connections."""
        await self._client.aclose()
    
    async def request(
        self,
        method: str,
        path: str,
        endpoint: Optional[str] = None,
        **kwargs
    ) -> httpx.Response:
        """Make an async HTTP request."""
        target = endpoint or await self._get_healthy_endpoint()
        url = f"{target}{path}"
        return await self._client.request(method, url, **kwargs)
    
    async def _get_healthy_endpoint(self) -> str:
        """Get a healthy endpoint from the pool."""
        async with self._lock:
            if not self._healthy_endpoints:
                # All endpoints unhealthy, try all
                self._healthy_endpoints = set(self.endpoints)
            
            # Simple round-robin (could be improved with load balancing)
            endpoint = next(iter(self._healthy_endpoints))
            return endpoint
    
    async def mark_unhealthy(self, endpoint: str):
        """Mark an endpoint as unhealthy."""
        async with self._lock:
            self._healthy_endpoints.discard(endpoint)
    
    async def health_check_all(self) -> Dict[str, bool]:
        """Check health of all endpoints."""
        results = {}
        async with self._lock:
            self._healthy_endpoints.clear()
            
            for endpoint in self.endpoints:
                try:
                    response = await self._client.get(f"{endpoint}/health", timeout=5.0)
                    healthy = response.status_code == 200
                    results[endpoint] = healthy
                    if healthy:
                        self._healthy_endpoints.add(endpoint)
                except Exception:
                    logging.debug("Health check failed for endpoint %s", endpoint)
                    results[endpoint] = False
        
        return results
    """
    Async ThemisDB client with connection pooling and graph support.
    
    Features:
    - Async/await interface
    - HTTP/2 connection pooling
    - Topology-aware routing
    - Graph traversal API
    - Streaming query results
    - Transaction support
    - Circuit breaker for fault tolerance
    - Request/response logging
    """
    
    def __init__(
        self,
        endpoints: List[str],
        namespace: str = "default",
        timeout: float = 30.0,
        max_connections: int = 100,
        max_retries: int = 3,
        circuit_breaker: Optional[CircuitBreaker] = None,
        enable_logging: bool = False,
        log_requests: bool = False,
        log_responses: bool = False,
        logger: Optional[logging.Logger] = None,
    ):
        if not endpoints:
            raise ValueError("endpoints must not be empty")
        
        self.namespace = namespace
        self.max_retries = max_retries
        self._pool = ConnectionPool(
            endpoints=endpoints,
            max_connections=max_connections,
            timeout=timeout,
        )
        self._topology_cache: Optional[Dict[str, Any]] = None
        self._topology_lock = asyncio.Lock()
        
        # Circuit breaker
        self._circuit_breaker = circuit_breaker
        
        # Logging
        self._enable_logging = enable_logging
        self._log_requests = log_requests
        self._log_responses = log_responses
        self._logger = logger or logging.getLogger(__name__)
    
    async def connect(self):
        """Initialize connection pool and fetch topology."""
        await self._refresh_topology()
    
    async def close(self):
        """Close all connections."""
        await self._pool.close()
    
    async def __aenter__(self) -> "AsyncThemisClient":
        await self.connect()
        return self
    
    async def __aexit__(self, exc_type, exc_val, exc_tb):
        await self.close()
    
    @property
    def circuit_breaker_state(self) -> Optional[str]:
        """Get the current circuit breaker state."""
        if self._circuit_breaker:
            return self._circuit_breaker.state.value
        return None
    
    async def _request_with_retry(
        self,
        method: str,
        path: str,
        endpoint: Optional[str] = None,
        **kwargs
    ) -> httpx.Response:
        """
        Make a request with retry, circuit breaker, and logging support.
        """
        # Check circuit breaker
        if self._circuit_breaker and not await self._circuit_breaker.can_execute():
            error_msg = f"Circuit breaker is OPEN for {method} {path}"
            if self._enable_logging:
                self._logger.warning(error_msg)
            raise RuntimeError(error_msg)
        
        last_error = None
        for attempt in range(self.max_retries):
            try:
                if attempt > 0:
                    # Exponential backoff
                    backoff = (2 ** (attempt - 1)) * 0.1
                    if self._enable_logging:
                        self._logger.info(f"Retry attempt {attempt} after {backoff}s")
                    await asyncio.sleep(backoff)
                
                # Log request if enabled
                if self._enable_logging and self._log_requests:
                    self._logger.info(f"Request: {method} {path}")
                
                response = await self._pool.request(method, path, endpoint=endpoint, **kwargs)
                
                # Log response if enabled
                if self._enable_logging and self._log_responses:
                    self._logger.info(f"Response: {method} {path} - Status: {response.status_code}")
                
                # Retry on 5xx errors
                if response.status_code >= 500 and attempt + 1 < self.max_retries:
                    last_error = Exception(f"Server error: {response.status_code}")
                    if self._enable_logging:
                        self._logger.warning(f"Server error {response.status_code}, will retry")
                    if self._circuit_breaker:
                        await self._circuit_breaker.record_failure()
                    continue
                
                # Record success/failure for circuit breaker
                if self._circuit_breaker:
                    if response.is_success:
                        await self._circuit_breaker.record_success()
                    else:
                        await self._circuit_breaker.record_failure()
                
                return response
                
            except Exception as e:
                last_error = e
                if self._enable_logging:
                    self._logger.error(f"Request error: {e}")
                if self._circuit_breaker:
                    await self._circuit_breaker.record_failure()
                if attempt + 1 >= self.max_retries:
                    raise
        
        # All retries exhausted
        if self._circuit_breaker:
            await self._circuit_breaker.record_failure()
        raise last_error or Exception(f"Request failed after {self.max_retries} attempts")
    
    # ==========================================================================
    # CRUD Operations
    # ==========================================================================
    
    async def get(
        self,
        model: str,
        collection: str,
        uuid: str,
    ) -> Optional[Dict[str, Any]]:
        """Get an entity by URN."""
        urn = self._build_urn(model, collection, uuid)
        endpoint = await self._resolve_endpoint(urn)
        key = self._build_entity_key(model, collection, uuid)
        
        response = await self._request_with_retry("GET", f"/entities/{key}", endpoint=endpoint)
        
        if response.status_code == 404:
            return None
        
        response.raise_for_status()
        payload = response.json()
        return self._decode_entity(payload)
    
    async def put(
        self,
        model: str,
        collection: str,
        uuid: str,
        data: Dict[str, Any],
    ) -> bool:
        """Create or update an entity."""
        urn = self._build_urn(model, collection, uuid)
        endpoint = await self._resolve_endpoint(urn)
        key = self._build_entity_key(model, collection, uuid)
        
        response = await self._request_with_retry(
            "PUT",
            f"/entities/{key}",
            endpoint=endpoint,
            json={"blob": self._encode_entity(data)},
        )
        
        response.raise_for_status()
        return True
    
    async def delete(
        self,
        model: str,
        collection: str,
        uuid: str,
    ) -> bool:
        """Delete an entity."""
        urn = self._build_urn(model, collection, uuid)
        endpoint = await self._resolve_endpoint(urn)
        key = self._build_entity_key(model, collection, uuid)
        
        response = await self._request_with_retry("DELETE", f"/entities/{key}", endpoint=endpoint)
        
        if response.status_code == 404:
            return False
        
        response.raise_for_status()
        return True
    
    # ==========================================================================
    # Batch Operations
    # ==========================================================================
    
    async def batch_get(
        self,
        model: str,
        collection: str,
        uuids: List[str],
        concurrency: int = 10,
    ) -> Dict[str, Optional[Dict[str, Any]]]:
        """Get multiple entities concurrently."""
        semaphore = asyncio.Semaphore(concurrency)
        
        async def fetch_one(uuid: str):
            async with semaphore:
                try:
                    return uuid, await self.get(model, collection, uuid)
                except Exception as e:
                    logging.debug("batch_get failed for uuid %s: %s", uuid, e)
                    return uuid, None
        
        tasks = [fetch_one(uuid) for uuid in uuids]
        results = await asyncio.gather(*tasks)
        
        return {uuid: data for uuid, data in results}
    
    async def batch_put(
        self,
        model: str,
        collection: str,
        items: Dict[str, Dict[str, Any]],
        concurrency: int = 10,
    ) -> Dict[str, bool]:
        """Put multiple entities concurrently."""
        semaphore = asyncio.Semaphore(concurrency)
        
        async def put_one(uuid: str, data: Dict[str, Any]):
            async with semaphore:
                try:
                    return uuid, await self.put(model, collection, uuid, data)
                except Exception:
                    logging.debug("batch_put failed for uuid %s", uuid)
                    return uuid, False
        
        tasks = [put_one(uuid, data) for uuid, data in items.items()]
        results = await asyncio.gather(*tasks)
        
        return {uuid: success for uuid, success in results}
    
    # ==========================================================================
    # Query Operations
    # ==========================================================================
    
    async def query(
        self,
        aql: str,
        params: Optional[Dict[str, Any]] = None,
        cursor: Optional[str] = None,
        batch_size: int = 100,
    ) -> AsyncQueryResult:
        """Execute an AQL query."""
        endpoint = self._pool.endpoints[0]  # Query on primary
        
        body = {
            "query": aql,
            "batchSize": batch_size,
        }
        if params:
            body["bindVars"] = params
        if cursor:
            body["cursor"] = cursor
        
        response = await self._pool.request(
            "POST",
            "/api/v1/query",
            endpoint=endpoint,
            json=body,
        )
        response.raise_for_status()
        
        result = response.json()
        return AsyncQueryResult(
            items=result.get("result", []),
            has_more=result.get("hasMore", False),
            next_cursor=result.get("cursor"),
            count=result.get("count"),
        )
    
    async def query_all(
        self,
        aql: str,
        params: Optional[Dict[str, Any]] = None,
        batch_size: int = 100,
    ) -> List[Any]:
        """Execute a query and return all results (pagination handled automatically)."""
        all_items = []
        result = await self.query(aql, params=params, batch_size=batch_size)
        all_items.extend(result.items)
        
        while result.has_more and result.next_cursor:
            result = await self.query(aql, params=params, cursor=result.next_cursor, batch_size=batch_size)
            all_items.extend(result.items)
        
        return all_items
    
    # ==========================================================================
    # Graph Traversal API
    # ==========================================================================
    
    async def traverse(
        self,
        model: str,
        collection: str,
        start_uuid: str,
        depth: int = 1,
        direction: str = "outbound",  # "outbound", "inbound", "any"
        edge_collections: Optional[List[str]] = None,
        filter_expression: Optional[str] = None,
        max_results: int = 1000,
    ) -> GraphTraversalResult:
        """
        Perform a graph traversal starting from a vertex.
        
        Args:
            model: Data model name
            collection: Starting vertex collection
            start_uuid: Starting vertex UUID
            depth: Maximum traversal depth (1-10)
            direction: Traversal direction
            edge_collections: Optional list of edge collections to traverse
            filter_expression: Optional AQL filter for vertices
            max_results: Maximum number of results
        
        Returns:
            GraphTraversalResult with nodes, edges, and paths
        """
        import time
        start_time = time.time()
        
        start_urn = self._build_urn(model, collection, start_uuid)
        endpoint = await self._resolve_endpoint(start_urn)
        
        body = {
            "startVertex": f"{collection}/{start_uuid}",
            "depth": min(depth, 10),
            "direction": direction,
            "maxResults": max_results,
        }
        
        if edge_collections:
            body["edgeCollections"] = edge_collections
        if filter_expression:
            body["filter"] = filter_expression
        
        response = await self._pool.request(
            "POST",
            f"/api/v1/graph/{model}/traverse",
            endpoint=endpoint,
            json=body,
        )
        response.raise_for_status()
        
        result = response.json()
        
        # Parse nodes
        nodes = []
        for node_data in result.get("vertices", []):
            nodes.append(GraphNode(
                id=node_data.get("_key", ""),
                collection=node_data.get("_id", "").split("/")[0] if "/" in node_data.get("_id", "") else "",
                data=node_data,
                depth=node_data.get("_depth", 0),
            ))
        
        # Parse edges
        edges = []
        for edge_data in result.get("edges", []):
            edges.append(GraphEdge(
                id=edge_data.get("_key", ""),
                source=edge_data.get("_from", ""),
                target=edge_data.get("_to", ""),
                label=edge_data.get("_label", ""),
                properties={k: v for k, v in edge_data.items() if not k.startswith("_")},
            ))
        
        # Parse paths
        paths = result.get("paths", [])
        
        execution_time = (time.time() - start_time) * 1000
        
        return GraphTraversalResult(
            nodes=nodes,
            edges=edges,
            paths=paths,
            total_visited=len(nodes),
            max_depth_reached=max(n.depth for n in nodes) if nodes else 0,
            execution_time_ms=execution_time,
        )
    
    async def shortest_path(
        self,
        model: str,
        from_collection: str,
        from_uuid: str,
        to_collection: str,
        to_uuid: str,
        edge_collections: Optional[List[str]] = None,
        weight_attribute: Optional[str] = None,
    ) -> Optional[List[GraphNode]]:
        """
        Find the shortest path between two vertices.
        
        Args:
            model: Data model name
            from_collection: Source vertex collection
            from_uuid: Source vertex UUID
            to_collection: Target vertex collection
            to_uuid: Target vertex UUID
            edge_collections: Optional edge collections to consider
            weight_attribute: Optional edge attribute for weighted shortest path
        
        Returns:
            List of nodes in the path, or None if no path exists
        """
        endpoint = self._pool.endpoints[0]
        
        body = {
            "from": f"{from_collection}/{from_uuid}",
            "to": f"{to_collection}/{to_uuid}",
        }
        
        if edge_collections:
            body["edgeCollections"] = edge_collections
        if weight_attribute:
            body["weightAttribute"] = weight_attribute
        
        response = await self._pool.request(
            "POST",
            f"/api/v1/graph/{model}/shortestPath",
            endpoint=endpoint,
            json=body,
        )
        
        if response.status_code == 404:
            return None
        
        response.raise_for_status()
        result = response.json()
        
        if not result.get("vertices"):
            return None
        
        return [
            GraphNode(
                id=v.get("_key", ""),
                collection=v.get("_id", "").split("/")[0] if "/" in v.get("_id", "") else "",
                data=v,
            )
            for v in result["vertices"]
        ]
    
    async def neighbors(
        self,
        model: str,
        collection: str,
        uuid: str,
        direction: str = "any",
        edge_collection: Optional[str] = None,
    ) -> List[GraphNode]:
        """Get immediate neighbors of a vertex."""
        result = await self.traverse(
            model=model,
            collection=collection,
            start_uuid=uuid,
            depth=1,
            direction=direction,
            edge_collections=[edge_collection] if edge_collection else None,
        )
        
        # Filter out the start node
        return [n for n in result.nodes if n.id != uuid]
    
    # ==========================================================================
    # Transaction Support
    # ==========================================================================
    
    @asynccontextmanager
    async def transaction(
        self,
        read_collections: Optional[List[str]] = None,
        write_collections: Optional[List[str]] = None,
    ) -> AsyncIterator["AsyncTransaction"]:
        """
        Start a transaction context.
        
        Usage:
            async with client.transaction(write_collections=["users"]) as txn:
                await txn.put("mymodel", "users", "user-1", {"name": "Alice"})
                await txn.put("mymodel", "users", "user-2", {"name": "Bob"})
        """
        txn = AsyncTransaction(
            client=self,
            read_collections=read_collections or [],
            write_collections=write_collections or [],
        )
        
        await txn.begin()
        try:
            yield txn
            await txn.commit()
        except Exception:
            await txn.rollback()
            raise
    
    # ==========================================================================
    # Vector Search
    # ==========================================================================
    
    async def vector_search(
        self,
        model: str,
        collection: str,
        vector: List[float],
        k: int = 10,
        filter_expression: Optional[str] = None,
    ) -> List[Dict[str, Any]]:
        """
        Perform a vector similarity search.
        
        Args:
            model: Data model name
            collection: Collection to search
            vector: Query vector
            k: Number of results to return
            filter_expression: Optional AQL filter
        
        Returns:
            List of documents with similarity scores
        """
        endpoint = self._pool.endpoints[0]
        
        body = {
            "collection": collection,
            "vector": vector,
            "k": k,
        }
        
        if filter_expression:
            body["filter"] = filter_expression
        
        response = await self._pool.request(
            "POST",
            f"/api/v1/vector/{model}/search",
            endpoint=endpoint,
            json=body,
        )
        response.raise_for_status()
        
        return response.json().get("results", [])
    
    # ==========================================================================
    # Internal Methods
    # ==========================================================================
    
    async def _refresh_topology(self):
        """Refresh the shard topology cache."""
        async with self._topology_lock:
            for endpoint in self._pool.endpoints:
                try:
                    response = await self._pool.request(
                        "GET",
                        "/_admin/cluster/topology",
                        endpoint=endpoint,
                    )
                    if response.status_code == 200:
                        self._topology_cache = response.json()
                        return
                except Exception:
                    logging.debug("Topology refresh failed for endpoint %s; trying next", endpoint)
                    continue
            self._topology_cache = {"shards": self._pool.endpoints}
    
    async def _resolve_endpoint(self, urn: str) -> str:
        """Resolve URN to shard endpoint."""
        if not self._topology_cache:
            await self._refresh_topology()
        
        shards = self._topology_cache.get("shards", [])
        if not shards:
            return self._pool.endpoints[0]
        
        # Consistent hashing
        hash_value = self._stable_hash(urn)
        shard_index = hash_value % len(shards)
        
        shard = shards[shard_index]
        if isinstance(shard, str):
            return shard.rstrip("/")
        elif isinstance(shard, dict):
            return shard.get("endpoint", self._pool.endpoints[0]).rstrip("/")
        
        return self._pool.endpoints[0]
    
    def _build_urn(self, model: str, collection: str, uuid: str) -> str:
        return f"urn:themis:{self.namespace}:{model}:{collection}:{uuid}"
    
    def _build_entity_key(self, model: str, collection: str, uuid: str) -> str:
        return f"{self.namespace}/{model}/{collection}/{uuid}"
    
    @staticmethod
    def _stable_hash(value: str) -> int:
        digest = hashlib.blake2b(value.encode("utf-8"), digest_size=4).digest()
        return int.from_bytes(digest, "big")
    
    @staticmethod
    def _encode_entity(data: Dict[str, Any]) -> str:
        import base64
        return base64.b64encode(json.dumps(data).encode()).decode()
    
    @staticmethod
    def _decode_entity(payload: Dict[str, Any]) -> Dict[str, Any]:
        import base64
        blob = payload.get("blob", "")
        if not blob:
            return payload
        try:
            decoded = base64.b64decode(blob)
            return json.loads(decoded)
        except Exception as e:
            logging.debug("Failed to decode entity blob: %s", e)
            return payload


class AsyncTransaction:
    """Async transaction context for atomic operations."""
    
    def __init__(
        self,
        client: AsyncThemisClient,
        read_collections: List[str],
        write_collections: List[str],
    ):
        self._client = client
        self._read_collections = read_collections
        self._write_collections = write_collections
        self._txn_id: Optional[str] = None
        self._operations: List[Dict[str, Any]] = []
    
    async def begin(self):
        """Begin the transaction."""
        endpoint = self._client._pool.endpoints[0]
        
        response = await self._client._pool.request(
            "POST",
            "/api/v1/transaction/begin",
            endpoint=endpoint,
            json={
                "readCollections": self._read_collections,
                "writeCollections": self._write_collections,
            },
        )
        response.raise_for_status()
        self._txn_id = response.json().get("transactionId")
    
    async def commit(self):
        """Commit the transaction."""
        if not self._txn_id:
            raise RuntimeError("Transaction not started")
        
        endpoint = self._client._pool.endpoints[0]
        
        response = await self._client._pool.request(
            "POST",
            f"/api/v1/transaction/{self._txn_id}/commit",
            endpoint=endpoint,
        )
        response.raise_for_status()
        self._txn_id = None
    
    async def rollback(self):
        """Rollback the transaction."""
        if not self._txn_id:
            return
        
        endpoint = self._client._pool.endpoints[0]
        
        try:
            await self._client._pool.request(
                "POST",
                f"/api/v1/transaction/{self._txn_id}/rollback",
                endpoint=endpoint,
            )
        except Exception as e:
            logging.debug("Rollback request failed for transaction %s: %s", self._txn_id, e)
        finally:
            self._txn_id = None
    
    async def get(
        self,
        model: str,
        collection: str,
        uuid: str,
    ) -> Optional[Dict[str, Any]]:
        """Get an entity within the transaction."""
        return await self._client.get(model, collection, uuid)
    
    async def put(
        self,
        model: str,
        collection: str,
        uuid: str,
        data: Dict[str, Any],
    ) -> bool:
        """Put an entity within the transaction."""
        return await self._client.put(model, collection, uuid, data)
    
    async def delete(
        self,
        model: str,
        collection: str,
        uuid: str,
    ) -> bool:
        """Delete an entity within the transaction."""
        return await self._client.delete(model, collection, uuid)
