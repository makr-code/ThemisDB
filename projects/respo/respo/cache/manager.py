"""Cache manager for RESPO."""

from dataclasses import dataclass, field
from typing import Any, Callable, Optional, TypeVar, Union

from .backends import CacheBackend, LRUCache, RedisCache, hash_key


T = TypeVar("T")


@dataclass
class CacheConfig:
    """Configuration for cache manager."""
    
    backend: str = "lru"  # "lru" or "redis"
    max_size: int = 1000  # For LRU cache
    default_ttl: Optional[int] = 3600  # seconds
    redis_url: str = "redis://localhost:6379"
    redis_prefix: str = "respo:"
    
    # Cache namespaces TTLs
    embedding_ttl: int = 86400  # 24 hours
    llm_response_ttl: int = 3600  # 1 hour
    search_ttl: int = 300  # 5 minutes
    
    # Enable/disable caching per type
    cache_embeddings: bool = True
    cache_llm_responses: bool = True
    cache_search_results: bool = True


class CacheManager:
    """Unified cache manager with multiple namespaces."""
    
    def __init__(self, config: Optional[CacheConfig] = None):
        self.config = config or CacheConfig()
        self._backends: dict[str, CacheBackend] = {}
        self._default_backend = self._create_backend()
    
    def _create_backend(self, namespace: Optional[str] = None) -> CacheBackend:
        """Create a cache backend based on config."""
        if self.config.backend == "redis":
            prefix = self.config.redis_prefix
            if namespace:
                prefix = f"{prefix}{namespace}:"
            return RedisCache(
                url=self.config.redis_url,
                prefix=prefix,
                default_ttl=self.config.default_ttl,
            )
        else:
            return LRUCache(
                max_size=self.config.max_size,
                default_ttl=self.config.default_ttl,
            )
    
    def get_backend(self, namespace: str = "default") -> CacheBackend:
        """Get or create a cache backend for a namespace."""
        if namespace not in self._backends:
            self._backends[namespace] = self._create_backend(namespace)
        return self._backends[namespace]
    
    async def get(
        self,
        key: str,
        namespace: str = "default",
    ) -> Optional[Any]:
        """Get value from cache."""
        backend = self.get_backend(namespace)
        return await backend.get(key)
    
    async def set(
        self,
        key: str,
        value: Any,
        namespace: str = "default",
        ttl: Optional[int] = None,
    ) -> None:
        """Set value in cache."""
        backend = self.get_backend(namespace)
        await backend.set(key, value, ttl)
    
    async def delete(
        self,
        key: str,
        namespace: str = "default",
    ) -> bool:
        """Delete value from cache."""
        backend = self.get_backend(namespace)
        return await backend.delete(key)
    
    async def exists(
        self,
        key: str,
        namespace: str = "default",
    ) -> bool:
        """Check if key exists in cache."""
        backend = self.get_backend(namespace)
        return await backend.exists(key)
    
    async def clear(self, namespace: Optional[str] = None) -> None:
        """Clear cache entries."""
        if namespace:
            backend = self.get_backend(namespace)
            await backend.clear()
        else:
            for backend in self._backends.values():
                await backend.clear()
    
    def stats(self) -> dict:
        """Get statistics for all cache backends."""
        return {
            namespace: backend.stats()
            for namespace, backend in self._backends.items()
        }
    
    # Convenience methods for specific cache types
    
    async def get_embedding(self, text: str) -> Optional[list[float]]:
        """Get cached embedding for text."""
        if not self.config.cache_embeddings:
            return None
        key = hash_key("embedding", text)
        return await self.get(key, namespace="embeddings")
    
    async def set_embedding(self, text: str, embedding: list[float]) -> None:
        """Cache embedding for text."""
        if not self.config.cache_embeddings:
            return
        key = hash_key("embedding", text)
        await self.set(
            key,
            embedding,
            namespace="embeddings",
            ttl=self.config.embedding_ttl,
        )
    
    async def get_llm_response(
        self,
        prompt: str,
        model: str,
        **kwargs,
    ) -> Optional[str]:
        """Get cached LLM response."""
        if not self.config.cache_llm_responses:
            return None
        key = hash_key("llm", prompt, model, **kwargs)
        return await self.get(key, namespace="llm")
    
    async def set_llm_response(
        self,
        prompt: str,
        model: str,
        response: str,
        **kwargs,
    ) -> None:
        """Cache LLM response."""
        if not self.config.cache_llm_responses:
            return
        key = hash_key("llm", prompt, model, **kwargs)
        await self.set(
            key,
            response,
            namespace="llm",
            ttl=self.config.llm_response_ttl,
        )
    
    async def get_search_results(
        self,
        query: str,
        **kwargs,
    ) -> Optional[list]:
        """Get cached search results."""
        if not self.config.cache_search_results:
            return None
        key = hash_key("search", query, **kwargs)
        return await self.get(key, namespace="search")
    
    async def set_search_results(
        self,
        query: str,
        results: list,
        **kwargs,
    ) -> None:
        """Cache search results."""
        if not self.config.cache_search_results:
            return
        key = hash_key("search", query, **kwargs)
        await self.set(
            key,
            results,
            namespace="search",
            ttl=self.config.search_ttl,
        )


def cached(
    namespace: str = "default",
    ttl: Optional[int] = None,
    key_func: Optional[Callable[..., str]] = None,
):
    """Decorator for caching function results."""
    def decorator(func: Callable[..., T]) -> Callable[..., T]:
        async def wrapper(self, *args, **kwargs) -> T:
            # Get cache manager from self or create new one
            cache = getattr(self, "_cache", None)
            if cache is None:
                return await func(self, *args, **kwargs)
            
            # Generate cache key
            if key_func:
                key = key_func(*args, **kwargs)
            else:
                key = hash_key(func.__name__, *args, **kwargs)
            
            # Try to get from cache
            result = await cache.get(key, namespace=namespace)
            if result is not None:
                return result
            
            # Call function and cache result
            result = await func(self, *args, **kwargs)
            await cache.set(key, result, namespace=namespace, ttl=ttl)
            
            return result
        
        wrapper.__name__ = func.__name__
        wrapper.__doc__ = func.__doc__
        return wrapper
    
    return decorator
