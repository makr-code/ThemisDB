"""Cache backends for RESPO."""

import hashlib
import json
import time
from abc import ABC, abstractmethod
from collections import OrderedDict
from dataclasses import dataclass
from typing import Any, Optional


class CacheBackend(ABC):
    """Abstract base class for cache backends."""
    
    @abstractmethod
    async def get(self, key: str) -> Optional[Any]:
        """Get value from cache."""
        pass
    
    @abstractmethod
    async def set(self, key: str, value: Any, ttl: Optional[int] = None) -> None:
        """Set value in cache."""
        pass
    
    @abstractmethod
    async def delete(self, key: str) -> bool:
        """Delete value from cache."""
        pass
    
    @abstractmethod
    async def exists(self, key: str) -> bool:
        """Check if key exists in cache."""
        pass
    
    @abstractmethod
    async def clear(self) -> None:
        """Clear all cache entries."""
        pass
    
    @abstractmethod
    def stats(self) -> dict:
        """Get cache statistics."""
        pass


@dataclass
class CacheEntry:
    """Cache entry with value and metadata."""
    value: Any
    created_at: float
    expires_at: Optional[float] = None
    
    def is_expired(self) -> bool:
        """Check if entry is expired."""
        if self.expires_at is None:
            return False
        return time.time() > self.expires_at


class LRUCache(CacheBackend):
    """In-memory LRU cache backend."""
    
    def __init__(self, max_size: int = 1000, default_ttl: Optional[int] = None):
        self.max_size = max_size
        self.default_ttl = default_ttl
        self._cache: OrderedDict[str, CacheEntry] = OrderedDict()
        self._hits = 0
        self._misses = 0
    
    async def get(self, key: str) -> Optional[Any]:
        """Get value from cache with LRU update."""
        if key not in self._cache:
            self._misses += 1
            return None
        
        entry = self._cache[key]
        
        # Check expiration
        if entry.is_expired():
            del self._cache[key]
            self._misses += 1
            return None
        
        # Move to end (most recently used)
        self._cache.move_to_end(key)
        self._hits += 1
        return entry.value
    
    async def set(self, key: str, value: Any, ttl: Optional[int] = None) -> None:
        """Set value in cache with optional TTL."""
        # Use default TTL if not specified
        if ttl is None:
            ttl = self.default_ttl
        
        expires_at = None
        if ttl is not None:
            expires_at = time.time() + ttl
        
        entry = CacheEntry(
            value=value,
            created_at=time.time(),
            expires_at=expires_at
        )
        
        # Remove oldest if at capacity
        if len(self._cache) >= self.max_size and key not in self._cache:
            self._cache.popitem(last=False)
        
        self._cache[key] = entry
        self._cache.move_to_end(key)
    
    async def delete(self, key: str) -> bool:
        """Delete value from cache."""
        if key in self._cache:
            del self._cache[key]
            return True
        return False
    
    async def exists(self, key: str) -> bool:
        """Check if key exists and is not expired."""
        if key not in self._cache:
            return False
        entry = self._cache[key]
        if entry.is_expired():
            del self._cache[key]
            return False
        return True
    
    async def clear(self) -> None:
        """Clear all cache entries."""
        self._cache.clear()
        self._hits = 0
        self._misses = 0
    
    def stats(self) -> dict:
        """Get cache statistics."""
        total = self._hits + self._misses
        hit_rate = self._hits / total if total > 0 else 0.0
        return {
            "backend": "lru",
            "size": len(self._cache),
            "max_size": self.max_size,
            "hits": self._hits,
            "misses": self._misses,
            "hit_rate": hit_rate,
        }


class RedisCache(CacheBackend):
    """Redis cache backend."""
    
    def __init__(
        self,
        url: str = "redis://localhost:6379",
        prefix: str = "respo:",
        default_ttl: Optional[int] = 3600,
    ):
        self.url = url
        self.prefix = prefix
        self.default_ttl = default_ttl
        self._client = None
        self._hits = 0
        self._misses = 0
    
    async def _get_client(self):
        """Get or create Redis client."""
        if self._client is None:
            try:
                import redis.asyncio as redis
                self._client = redis.from_url(self.url)
            except ImportError:
                raise ImportError("redis package required for Redis cache backend")
        return self._client
    
    def _make_key(self, key: str) -> str:
        """Create prefixed key."""
        return f"{self.prefix}{key}"
    
    async def get(self, key: str) -> Optional[Any]:
        """Get value from Redis."""
        client = await self._get_client()
        data = await client.get(self._make_key(key))
        
        if data is None:
            self._misses += 1
            return None
        
        self._hits += 1
        return json.loads(data)
    
    async def set(self, key: str, value: Any, ttl: Optional[int] = None) -> None:
        """Set value in Redis with optional TTL."""
        client = await self._get_client()
        
        if ttl is None:
            ttl = self.default_ttl
        
        data = json.dumps(value)
        
        if ttl is not None:
            await client.setex(self._make_key(key), ttl, data)
        else:
            await client.set(self._make_key(key), data)
    
    async def delete(self, key: str) -> bool:
        """Delete value from Redis."""
        client = await self._get_client()
        result = await client.delete(self._make_key(key))
        return result > 0
    
    async def exists(self, key: str) -> bool:
        """Check if key exists in Redis."""
        client = await self._get_client()
        return await client.exists(self._make_key(key)) > 0
    
    async def clear(self) -> None:
        """Clear all cache entries with prefix."""
        client = await self._get_client()
        cursor = 0
        while True:
            cursor, keys = await client.scan(cursor, match=f"{self.prefix}*")
            if keys:
                await client.delete(*keys)
            if cursor == 0:
                break
        self._hits = 0
        self._misses = 0
    
    def stats(self) -> dict:
        """Get cache statistics."""
        total = self._hits + self._misses
        hit_rate = self._hits / total if total > 0 else 0.0
        return {
            "backend": "redis",
            "url": self.url,
            "prefix": self.prefix,
            "hits": self._hits,
            "misses": self._misses,
            "hit_rate": hit_rate,
        }


def hash_key(*args, **kwargs) -> str:
    """Create a hash key from arguments."""
    data = json.dumps({"args": args, "kwargs": kwargs}, sort_keys=True, default=str)
    return hashlib.sha256(data.encode()).hexdigest()[:16]
