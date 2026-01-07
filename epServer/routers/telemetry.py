"""Telemetry API endpoints for ThemisDB instance tracking."""

from fastapi import APIRouter, Depends, HTTPException, status, Request
from sqlalchemy.ext.asyncio import AsyncSession

from models import TelemetryHeartbeat, TelemetryResponse, TelemetryStats
from services.telemetry_service import TelemetryService
from utils.database import get_db
from exceptions import (
    LicenseNotFoundException,
    LicenseSuspendedException,
    LicenseExpiredException
)

router = APIRouter(
    prefix="/telemetry",
    tags=["telemetry"],
    responses={404: {"description": "Not found"}},
)


@router.post(
    "/heartbeat",
    response_model=TelemetryResponse,
    status_code=status.HTTP_200_OK,
    summary="Report instance heartbeat",
    description="""
    Record a heartbeat from a ThemisDB instance with metrics.
    
    This endpoint allows ThemisDB instances to periodically report their
    status and metrics to the central server for monitoring and analytics.
    
    **Recommended interval:** Every 5-15 minutes
    
    **Privacy:** Only essential metrics are collected. No query content
    or sensitive customer data is transmitted.
    """
)
async def report_heartbeat(
    heartbeat: TelemetryHeartbeat,
    request: Request,
    db: AsyncSession = Depends(get_db)
):
    """
    Report instance heartbeat with metrics.
    
    Args:
        heartbeat: Telemetry heartbeat data
        request: HTTP request for user agent and IP
        db: Database session
        
    Returns:
        Telemetry response with success status
    """
    try:
        service = TelemetryService(db)
        
        # Extract user agent and IP
        user_agent = request.headers.get("user-agent")
        # Get real IP if behind proxy
        ip_address = request.headers.get("x-forwarded-for") or request.client.host
        
        response = await service.record_heartbeat(
            heartbeat,
            user_agent=user_agent,
            ip_address=ip_address
        )
        
        await db.commit()
        
        return response
    
    except LicenseNotFoundException as e:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail=f"License key not found: {e.license_key}"
        )
    except LicenseSuspendedException as e:
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail=f"License suspended: {e.license_key}"
        )
    except Exception as e:
        await db.rollback()
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail=f"Failed to record heartbeat: {str(e)}"
        )


@router.get(
    "/statistics",
    response_model=TelemetryStats,
    summary="Get global telemetry statistics",
    description="""
    Get aggregated statistics about all ThemisDB instances worldwide.
    
    Includes:
    - Total and active instance counts
    - Total nodes, cores, and storage
    - Version distribution
    - Geographic distribution
    - Distribution by pricing tier
    """
)
async def get_statistics(
    db: AsyncSession = Depends(get_db)
):
    """
    Get global telemetry statistics.
    
    Args:
        db: Database session
        
    Returns:
        Telemetry statistics
    """
    try:
        service = TelemetryService(db)
        return await service.get_statistics()
    except Exception as e:
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail=f"Failed to get statistics: {str(e)}"
        )


@router.get(
    "/license/{license_key}/instances",
    summary="Get instances for a license",
    description="""
    Get all instances associated with a specific license key.
    
    Shows all ThemisDB instances that have reported using this license,
    including their current status, versions, and metrics.
    """
)
async def get_license_instances(
    license_key: str,
    db: AsyncSession = Depends(get_db)
):
    """
    Get all instances for a license key.
    
    Args:
        license_key: License key
        db: Database session
        
    Returns:
        List of instance telemetry records
    """
    try:
        service = TelemetryService(db)
        instances = await service.get_license_instances(license_key)
        
        # Convert to dict for response
        result = []
        for inst in instances:
            result.append({
                "instance_id": inst.instance_id,
                "hostname": inst.hostname,
                "version": inst.version,
                "nodes_count": inst.nodes_count,
                "total_cores": inst.total_cores,
                "used_storage_tb": inst.used_storage_tb,
                "uptime_seconds": inst.uptime_seconds,
                "query_count_24h": inst.query_count_24h,
                "country": inst.country,
                "region": inst.region,
                "first_seen": inst.first_seen.isoformat(),
                "last_seen": inst.last_seen.isoformat(),
                "report_count": inst.report_count
            })
        
        return result
    
    except LicenseNotFoundException as e:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail=f"License key not found: {e.license_key}"
        )
    except Exception as e:
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail=f"Failed to get instances: {str(e)}"
        )


@router.get(
    "/instance/{instance_id}",
    summary="Get instance details",
    description="""
    Get detailed information about a specific ThemisDB instance.
    """
)
async def get_instance_details(
    instance_id: str,
    db: AsyncSession = Depends(get_db)
):
    """
    Get detailed information about a specific instance.
    
    Args:
        instance_id: Instance ID
        db: Database session
        
    Returns:
        Instance telemetry details
    """
    try:
        service = TelemetryService(db)
        instance = await service.get_instance_details(instance_id)
        
        if not instance:
            raise HTTPException(
                status_code=status.HTTP_404_NOT_FOUND,
                detail=f"Instance not found: {instance_id}"
            )
        
        return {
            "instance_id": instance.instance_id,
            "license_key": instance.license_key,
            "hostname": instance.hostname,
            "version": instance.version,
            "nodes_count": instance.nodes_count,
            "total_cores": instance.total_cores,
            "used_storage_tb": instance.used_storage_tb,
            "uptime_seconds": instance.uptime_seconds,
            "query_count_24h": instance.query_count_24h,
            "country": instance.country,
            "region": instance.region,
            "first_seen": instance.first_seen.isoformat(),
            "last_seen": instance.last_seen.isoformat(),
            "report_count": instance.report_count,
            "is_active": await service.is_instance_active(instance_id)
        }
    
    except HTTPException:
        raise
    except Exception as e:
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail=f"Failed to get instance details: {str(e)}"
        )


@router.post(
    "/cleanup",
    summary="Clean up old telemetry data",
    description="""
    Delete telemetry records older than specified days.
    
    **Admin endpoint** - should be protected in production.
    """
)
async def cleanup_old_data(
    days: int = 90,
    db: AsyncSession = Depends(get_db)
):
    """
    Clean up telemetry data older than specified days.
    
    Args:
        days: Number of days to retain (default 90)
        db: Database session
        
    Returns:
        Number of records deleted
    """
    try:
        service = TelemetryService(db)
        deleted_count = await service.cleanup_old_data(days)
        await db.commit()
        
        return {
            "success": True,
            "deleted_count": deleted_count,
            "message": f"Deleted {deleted_count} records older than {days} days"
        }
    
    except Exception as e:
        await db.rollback()
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail=f"Failed to cleanup old data: {str(e)}"
        )
