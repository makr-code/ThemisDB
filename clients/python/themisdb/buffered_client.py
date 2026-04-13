"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            buffered_client.py                                 ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:12:39                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     418                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
ThemisDB Buffered Client - Python SDK

Provides high-performance buffered operations for ThemisDB time series,
vector index, and graph operations via HTTP REST API or binary protocol.

Example:
    >>> from themisdb import BufferedClient
    >>> client = BufferedClient('localhost', 8080, protocol='http')
    >>> client.put_ts_buffered('cpu.usage', 'server01', 1700000000, 75.5)
    >>> stats = client.get_buffer_stats()
    >>> client.flush_buffers('all')
"""

import json
import struct
import socket
from typing import Dict, List, Optional, Any, Literal
from enum import Enum
import msgpack
import requests


class Protocol(Enum):
    """Protocol types supported by ThemisDB"""
    HTTP = "http"
    BINARY = "binary"


class BufferedClient:
    """
    ThemisDB Buffered Client for high-performance data ingestion.
    
    Supports both HTTP REST API and binary protocol for buffered operations
    on time series, vector index, and property graph data.
    
    Args:
        host: ThemisDB server hostname
        port: Server port (8080 for HTTP, 9090 for binary)
        protocol: Communication protocol ('http' or 'binary')
        timeout: Request timeout in seconds
    
    Example:
        >>> client = BufferedClient('localhost', 8080)
        >>> client.put_ts_buffered('cpu.usage', 'server01', 1700000000, 75.5)
    """
    
    def __init__(
        self,
        host: str = 'localhost',
        port: int = 8080,
        protocol: Literal['http', 'binary'] = 'http',
        timeout: int = 30
    ):
        self.host = host
        self.port = port
        self.protocol = Protocol(protocol)
        self.timeout = timeout
        self.base_url = f"http://{host}:{port}" if self.protocol == Protocol.HTTP else None
        self.socket = None
        
        if self.protocol == Protocol.BINARY:
            self._connect_binary()
    
    def _connect_binary(self):
        """Establish binary protocol connection"""
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.settimeout(self.timeout)
        self.socket.connect((self.host, self.port))
    
    def close(self):
        """Close connection"""
        if self.socket:
            self.socket.close()
            self.socket = None
    
    def __enter__(self):
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()
    
    # HTTP Protocol Methods
    
    def _http_post(self, endpoint: str, payload: Dict[str, Any]) -> Dict[str, Any]:
        """Send HTTP POST request"""
        url = f"{self.base_url}{endpoint}"
        response = requests.post(
            url,
            json=payload,
            timeout=self.timeout,
            headers={'Content-Type': 'application/json'}
        )
        response.raise_for_status()
        return response.json()
    
    def _http_get(self, endpoint: str) -> Dict[str, Any]:
        """Send HTTP GET request"""
        url = f"{self.base_url}{endpoint}"
        response = requests.get(url, timeout=self.timeout)
        response.raise_for_status()
        return response.json()
    
    # Binary Protocol Methods
    
    def _binary_send(self, opcode: int, payload: Dict[str, Any]) -> Dict[str, Any]:
        """Send binary protocol message"""
        if not self.socket:
            self._connect_binary()
        
        # Encode payload with MessagePack
        payload_bytes = msgpack.packb(payload)
        
        # Create message: [opcode:1][length:4][payload:N]
        message = struct.pack('!BI', opcode, len(payload_bytes)) + payload_bytes
        self.socket.sendall(message)
        
        # Read response: [status:1][length:4][payload:N]
        status_byte = self.socket.recv(1)
        if not status_byte:
            raise ConnectionError("Connection closed by server")
        
        status = struct.unpack('!B', status_byte)[0]
        
        if status == 0x00:  # Success
            length_bytes = self.socket.recv(4)
            if len(length_bytes) < 4:
                return {}
            
            length = struct.unpack('!I', length_bytes)[0]
            if length == 0:
                return {}
            
            response_bytes = b''
            while len(response_bytes) < length:
                chunk = self.socket.recv(length - len(response_bytes))
                if not chunk:
                    raise ConnectionError("Connection closed while reading response")
                response_bytes += chunk
            
            return msgpack.unpackb(response_bytes)
        else:
            # Error response
            raise RuntimeError(f"Server returned error status: 0x{status:02x}")
    
    # Time Series Operations
    
    def put_ts_buffered(
        self,
        metric: str,
        entity: str,
        timestamp: int,
        value: float
    ) -> Dict[str, Any]:
        """
        Insert a buffered time series data point.
        
        Args:
            metric: Metric name (e.g., 'cpu.usage')
            entity: Entity identifier (e.g., 'server01')
            timestamp: Unix timestamp in seconds
            value: Metric value
        
        Returns:
            Response dictionary with status
        
        Example:
            >>> client.put_ts_buffered('cpu.usage', 'server01', 1700000000, 75.5)
        """
        payload = {
            'metric': metric,
            'entity': entity,
            'timestamp': timestamp,
            'value': value
        }
        
        if self.protocol == Protocol.HTTP:
            return self._http_post('/ts/put/buffered', payload)
        else:
            return self._binary_send(0x70, payload)  # TS_PUT_BUFFERED
    
    def put_ts_buffered_batch(
        self,
        points: List[Dict[str, Any]]
    ) -> Dict[str, Any]:
        """
        Insert a batch of buffered time series data points.
        
        Args:
            points: List of data points, each with keys: metric, entity, timestamp, value
        
        Returns:
            Response dictionary with status
        
        Example:
            >>> points = [
            ...     {'metric': 'cpu', 'entity': 'server01', 'timestamp': 1700000000, 'value': 75},
            ...     {'metric': 'cpu', 'entity': 'server02', 'timestamp': 1700000000, 'value': 82}
            ... ]
            >>> client.put_ts_buffered_batch(points)
        """
        payload = {'points': points}
        
        if self.protocol == Protocol.HTTP:
            return self._http_post('/ts/put/buffered/batch', payload)
        else:
            return self._binary_send(0x71, payload)  # TS_PUT_BUFFERED_BATCH
    
    # Vector Index Operations
    
    def add_vector_buffered(
        self,
        pk: str,
        embedding: List[float],
        metadata: Optional[Dict[str, Any]] = None
    ) -> Dict[str, Any]:
        """
        Add a buffered vector to the index.
        
        Args:
            pk: Primary key (document ID)
            embedding: Vector embedding (list of floats)
            metadata: Optional metadata dictionary
        
        Returns:
            Response dictionary with status
        
        Example:
            >>> client.add_vector_buffered('doc123', [0.1, 0.2, 0.3], {'title': 'Test'})
        """
        payload = {
            'pk': pk,
            'embedding': embedding,
            'metadata': metadata or {}
        }
        
        if self.protocol == Protocol.HTTP:
            return self._http_post('/vectors/add/buffered', payload)
        else:
            return self._binary_send(0x72, payload)  # VECTOR_ADD_BUFFERED
    
    def update_vector_buffered(
        self,
        pk: str,
        embedding: List[float],
        metadata: Optional[Dict[str, Any]] = None
    ) -> Dict[str, Any]:
        """
        Update a buffered vector in the index.
        
        Args:
            pk: Primary key (document ID)
            embedding: New vector embedding
            metadata: Optional new metadata
        
        Returns:
            Response dictionary with status
        """
        payload = {
            'pk': pk,
            'embedding': embedding,
            'metadata': metadata or {}
        }
        
        if self.protocol == Protocol.HTTP:
            return self._http_post('/vectors/update/buffered', payload)
        else:
            return self._binary_send(0x73, payload)  # VECTOR_UPDATE_BUFFERED
    
    def remove_vector_buffered(self, pk: str) -> Dict[str, Any]:
        """
        Remove a buffered vector from the index.
        
        Args:
            pk: Primary key (document ID)
        
        Returns:
            Response dictionary with status
        """
        payload = {'pk': pk}
        
        if self.protocol == Protocol.HTTP:
            return self._http_post('/vectors/remove/buffered', payload)
        else:
            return self._binary_send(0x74, payload)  # VECTOR_REMOVE_BUFFERED
    
    # Property Graph Operations
    
    def add_graph_node_buffered(
        self,
        graph_id: str,
        pk: str,
        properties: Dict[str, Any]
    ) -> Dict[str, Any]:
        """
        Add a buffered node to a property graph.
        
        Args:
            graph_id: Graph identifier
            pk: Node primary key
            properties: Node properties dictionary
        
        Returns:
            Response dictionary with status
        
        Example:
            >>> client.add_graph_node_buffered('social', 'user123', {'name': 'Alice'})
        """
        payload = {
            'graph_id': graph_id,
            'type': 'node',
            'pk': pk,
            'properties': properties
        }
        
        if self.protocol == Protocol.HTTP:
            return self._http_post('/graph/add/buffered', payload)
        else:
            return self._binary_send(0x75, payload)  # GRAPH_NODE_BUFFERED
    
    def add_graph_edge_buffered(
        self,
        graph_id: str,
        pk: str,
        from_node: str,
        to_node: str,
        properties: Optional[Dict[str, Any]] = None
    ) -> Dict[str, Any]:
        """
        Add a buffered edge to a property graph.
        
        Args:
            graph_id: Graph identifier
            pk: Edge primary key
            from_node: Source node ID
            to_node: Target node ID
            properties: Optional edge properties
        
        Returns:
            Response dictionary with status
        
        Example:
            >>> client.add_graph_edge_buffered('social', 'edge1', 'user123', 'user456', {'type': 'follows'})
        """
        payload = {
            'graph_id': graph_id,
            'type': 'edge',
            'pk': pk,
            'from': from_node,
            'to': to_node,
            'properties': properties or {}
        }
        
        if self.protocol == Protocol.HTTP:
            return self._http_post('/graph/add/buffered', payload)
        else:
            return self._binary_send(0x76, payload)  # GRAPH_EDGE_BUFFERED
    
    # Buffer Management
    
    def get_buffer_stats(self) -> Dict[str, Any]:
        """
        Get current buffer statistics.
        
        Returns:
            Dictionary containing buffer statistics for all buffers
        
        Example:
            >>> stats = client.get_buffer_stats()
            >>> print(f"Points buffered: {stats['ts_buffer']['points_buffered']}")
        """
        if self.protocol == Protocol.HTTP:
            return self._http_get('/buffer/stats')
        else:
            return self._binary_send(0x77, {})  # BUFFER_STATS
    
    def flush_buffers(self, buffer: Literal['all', 'ts', 'vector', 'graph'] = 'all') -> Dict[str, Any]:
        """
        Manually flush buffers to persistent storage.
        
        Args:
            buffer: Which buffer to flush ('all', 'ts', 'vector', or 'graph')
        
        Returns:
            Response dictionary with flush results
        
        Example:
            >>> client.flush_buffers('all')
        """
        payload = {'buffer': buffer}
        
        if self.protocol == Protocol.HTTP:
            return self._http_post('/buffer/flush', payload)
        else:
            return self._binary_send(0x78, payload)  # BUFFER_FLUSH
