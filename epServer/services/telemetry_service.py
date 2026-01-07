"""Telemetry service for tracking ThemisDB instances worldwide."""

from typing import Optional, Dict
from datetime import datetime, timezone
from sqlalchemy.ext.asyncio import AsyncSession

from models import (
    InstanceTelemetry,
    TelemetryHeartbeat,
    TelemetryResponse,
    TelemetryStats
)
from repositories.interfaces import ITelemetryRepository, ISubscriptionRepository
from repositories.implementations import TelemetryRepository, SubscriptionRepository
from exceptions import LicenseNotFoundException, LicenseSuspendedException


class TelemetryService:
    """Service for telemetry collection and analytics using OOP principles."""
    
    def __init__(
        self,
        db: AsyncSession,
        telemetry_repository: Optional[ITelemetryRepository] = None,
        subscription_repository: Optional[ISubscriptionRepository] = None
    ):
        """
        Initialize telemetry service with dependency injection.
        
        Args:
            db: Database session
            telemetry_repository: Telemetry repository (injected for testability)
            subscription_repository: Subscription repository (injected for testability)
        """
        self._db = db
        self._telemetry_repository = telemetry_repository or TelemetryRepository(db)
        self._subscription_repository = subscription_repository or SubscriptionRepository(db)
    
    async def record_heartbeat(
        self,
        heartbeat: TelemetryHeartbeat,
        user_agent: Optional[str] = None,
        ip_address: Optional[str] = None
    ) -> TelemetryResponse:
        """
        Record a heartbeat from a ThemisDB instance.
        
        Args:
            heartbeat: Telemetry heartbeat data
            user_agent: User agent string (optional)
            ip_address: IP address (optional)
            
        Returns:
            Telemetry response with success status
            
        Raises:
            LicenseNotFoundException: If license key is not found
            LicenseSuspendedException: If license is suspended
        """
        # Validate license key
        subscription = await self._subscription_repository.get_by_license_key(
            heartbeat.license_key
        )
        
        if not subscription:
            raise LicenseNotFoundException(heartbeat.license_key)
        
        if subscription.status.value == "suspended":
            raise LicenseSuspendedException(heartbeat.license_key)
        
        # Create or update telemetry record
        telemetry = InstanceTelemetry(
            license_key=heartbeat.license_key,
            instance_id=heartbeat.instance_id,
            hostname=heartbeat.hostname,
            version=heartbeat.version,
            nodes_count=heartbeat.metrics.nodes,
            total_cores=heartbeat.metrics.total_cores,
            used_storage_tb=heartbeat.metrics.used_storage_tb,
            uptime_seconds=heartbeat.metrics.uptime_seconds,
            query_count_24h=heartbeat.metrics.query_count_24h,
            country=heartbeat.country,
            region=heartbeat.region,
            user_agent=user_agent,
            ip_address=ip_address
        )
        
        saved_telemetry = await self._telemetry_repository.create_or_update(telemetry)
        
        # Get instance count for this license
        instances = await self._telemetry_repository.get_by_license_key(heartbeat.license_key)
        
        return TelemetryResponse(
            success=True,
            message="Heartbeat recorded successfully",
            instance_count=len(instances)
        )
    
    async def get_license_instances(self, license_key: str) -> list:
        """
        Get all instances for a license key.
        
        Args:
            license_key: License key
            
        Returns:
            List of instance telemetry records
            
        Raises:
            LicenseNotFoundException: If license key is not found
        """
        # Validate license exists
        subscription = await self._subscription_repository.get_by_license_key(license_key)
        if not subscription:
            raise LicenseNotFoundException(license_key)
        
        return await self._telemetry_repository.get_by_license_key(license_key)
    
    async def get_statistics(self) -> TelemetryStats:
        """
        Get global telemetry statistics.
        
        Returns:
            Telemetry statistics
        """
        stats_dict = await self._telemetry_repository.get_statistics()
        
        return TelemetryStats(
            total_instances=stats_dict["total_instances"],
            active_instances_24h=stats_dict["active_instances_24h"],
            total_nodes=stats_dict["total_nodes"],
            total_cores=stats_dict["total_cores"],
            total_storage_tb=stats_dict["total_storage_tb"],
            versions=stats_dict["versions"],
            countries=stats_dict["countries"],
            by_tier=stats_dict["by_tier"]
        )
    
    async def cleanup_old_data(self, days: int = 90) -> int:
        """
        Clean up telemetry data older than specified days.
        
        Args:
            days: Number of days to retain (default 90)
            
        Returns:
            Number of records deleted
        """
        return await self._telemetry_repository.cleanup_old_records(days)
    
    async def is_instance_active(self, instance_id: str, hours: int = 24) -> bool:
        """
        Check if an instance is active (reported recently).
        
        Args:
            instance_id: Instance ID
            hours: Hours to consider active (default 24)
            
        Returns:
            True if instance is active, False otherwise
        """
        telemetry = await self._telemetry_repository.get_by_instance_id(instance_id)
        
        if not telemetry:
            return False
        
        cutoff = datetime.now(timezone.utc).timestamp() - (hours * 3600)
        last_seen_timestamp = telemetry.last_seen.timestamp()
        
        return last_seen_timestamp >= cutoff
    
    async def get_instance_details(self, instance_id: str) -> Optional[InstanceTelemetry]:
        """
        Get detailed information about a specific instance.
        
        Args:
            instance_id: Instance ID
            
        Returns:
            Instance telemetry record or None
        """
        return await self._telemetry_repository.get_by_instance_id(instance_id)
