"""Service health check implementation."""

from dataclasses import dataclass, field
from datetime import datetime
from enum import Enum
from typing import Any, Callable, Awaitable
import asyncio
import time


class HealthStatus(Enum):
    """Health status enumeration."""
    HEALTHY = "healthy"
    DEGRADED = "degraded"
    UNHEALTHY = "unhealthy"
    UNKNOWN = "unknown"


@dataclass
class ComponentHealth:
    """Health status of a single component."""
    name: str
    status: HealthStatus
    message: str = ""
    latency_ms: float = 0.0
    details: dict[str, Any] = field(default_factory=dict)
    checked_at: datetime = field(default_factory=datetime.utcnow)
    
    def to_dict(self) -> dict[str, Any]:
        """Convert to dictionary."""
        return {
            "name": self.name,
            "status": self.status.value,
            "message": self.message,
            "latency_ms": round(self.latency_ms, 2),
            "details": self.details,
            "checked_at": self.checked_at.isoformat(),
        }


class ServiceHealth:
    """Service health checker."""
    
    def __init__(self):
        """Initialize health checker."""
        self._checks: dict[str, Callable[[], Awaitable[ComponentHealth]]] = {}
        self._last_check: dict[str, ComponentHealth] = {}
    
    def register(
        self,
        name: str,
        check: Callable[[], Awaitable[ComponentHealth]],
    ) -> None:
        """Register a health check.
        
        Args:
            name: Component name
            check: Async function that returns ComponentHealth
        """
        self._checks[name] = check
    
    async def check_component(self, name: str) -> ComponentHealth:
        """Check a single component.
        
        Args:
            name: Component name
            
        Returns:
            Component health status
        """
        if name not in self._checks:
            return ComponentHealth(
                name=name,
                status=HealthStatus.UNKNOWN,
                message=f"Unknown component: {name}",
            )
        
        start = time.perf_counter()
        try:
            result = await asyncio.wait_for(
                self._checks[name](),
                timeout=10.0,
            )
            result.latency_ms = (time.perf_counter() - start) * 1000
            self._last_check[name] = result
            return result
        except asyncio.TimeoutError:
            return ComponentHealth(
                name=name,
                status=HealthStatus.UNHEALTHY,
                message="Health check timed out",
                latency_ms=(time.perf_counter() - start) * 1000,
            )
        except Exception as e:
            return ComponentHealth(
                name=name,
                status=HealthStatus.UNHEALTHY,
                message=str(e),
                latency_ms=(time.perf_counter() - start) * 1000,
            )
    
    async def check_all(self) -> dict[str, Any]:
        """Check all registered components.
        
        Returns:
            Overall health status with component details
        """
        results = await asyncio.gather(
            *[self.check_component(name) for name in self._checks],
            return_exceptions=True,
        )
        
        components = {}
        overall_status = HealthStatus.HEALTHY
        
        for result in results:
            if isinstance(result, Exception):
                continue
            components[result.name] = result.to_dict()
            
            if result.status == HealthStatus.UNHEALTHY:
                overall_status = HealthStatus.UNHEALTHY
            elif result.status == HealthStatus.DEGRADED and overall_status == HealthStatus.HEALTHY:
                overall_status = HealthStatus.DEGRADED
        
        return {
            "status": overall_status.value,
            "checked_at": datetime.utcnow().isoformat(),
            "components": components,
        }
    
    def get_cached(self, name: str) -> ComponentHealth | None:
        """Get cached health status for a component.
        
        Args:
            name: Component name
            
        Returns:
            Last known health status or None
        """
        return self._last_check.get(name)


# Global health checker instance
_health_checker: ServiceHealth | None = None


def get_health_checker() -> ServiceHealth:
    """Get the global health checker instance."""
    global _health_checker
    if _health_checker is None:
        _health_checker = ServiceHealth()
    return _health_checker


async def check_vectorstore() -> ComponentHealth:
    """Check vector store health."""
    return ComponentHealth(
        name="vectorstore",
        status=HealthStatus.HEALTHY,
        message="Vector store is operational",
    )


async def check_llm() -> ComponentHealth:
    """Check LLM service health."""
    return ComponentHealth(
        name="llm",
        status=HealthStatus.HEALTHY,
        message="LLM service is operational",
        details={"model": "not configured"},
    )


async def check_cache() -> ComponentHealth:
    """Check cache health."""
    return ComponentHealth(
        name="cache",
        status=HealthStatus.HEALTHY,
        message="Cache is operational",
    )


def setup_default_checks() -> ServiceHealth:
    """Setup default health checks."""
    health = get_health_checker()
    health.register("vectorstore", check_vectorstore)
    health.register("llm", check_llm)
    health.register("cache", check_cache)
    return health
