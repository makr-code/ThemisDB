"""License validation API endpoints.

These endpoints are called by ThemisDB instances to validate their licenses.
"""

from fastapi import APIRouter, Depends, HTTPException, status, Header
from sqlalchemy.ext.asyncio import AsyncSession
from pydantic import BaseModel
from typing import Optional

from services.license_validation_service import LicenseValidationService
from utils import get_db

router = APIRouter(prefix="/license", tags=["License Validation"])


class LicenseValidationRequest(BaseModel):
    """License validation request model."""
    license_key: str
    server_hostname: Optional[str] = None
    server_version: Optional[str] = None
    server_nodes: Optional[int] = None


class LicenseLimitsCheckRequest(BaseModel):
    """License limits check request model."""
    license_key: str
    current_nodes: int
    current_cores: Optional[int] = None
    current_storage_tb: Optional[float] = None


@router.post("/validate")
async def validate_license(
    request: LicenseValidationRequest,
    db: AsyncSession = Depends(get_db)
):
    """
    Validate a ThemisDB license.
    
    This endpoint is called by ThemisDB instances to verify their license
    at startup and periodically during operation.
    
    Returns license status, tier, limits, and expiry information.
    """
    server_info = None
    if request.server_hostname or request.server_version:
        server_info = {
            "hostname": request.server_hostname,
            "version": request.server_version,
            "nodes": request.server_nodes
        }
    
    result = await LicenseValidationService.validate_license(
        db, 
        request.license_key,
        server_info
    )
    
    # Return appropriate HTTP status code based on validation result
    if not result["valid"]:
        status_code = status.HTTP_403_FORBIDDEN
        if result["status"] == "not_found":
            status_code = status.HTTP_404_NOT_FOUND
        elif result["status"] == "invalid_format":
            status_code = status.HTTP_400_BAD_REQUEST
        
        return {
            **result,
            "http_status": status_code
        }
    
    return result


@router.get("/validate/{license_key}")
async def validate_license_get(
    license_key: str,
    server_hostname: Optional[str] = Header(None, alias="X-Server-Hostname"),
    server_version: Optional[str] = Header(None, alias="X-Server-Version"),
    db: AsyncSession = Depends(get_db)
):
    """
    Validate a license using GET request (simpler for some clients).
    
    License key is provided in the URL path.
    Server information can be provided via headers.
    """
    server_info = None
    if server_hostname or server_version:
        server_info = {
            "hostname": server_hostname,
            "version": server_version
        }
    
    result = await LicenseValidationService.validate_license(
        db,
        license_key,
        server_info
    )
    
    if not result["valid"]:
        status_code = status.HTTP_403_FORBIDDEN
        if result["status"] == "not_found":
            raise HTTPException(
                status_code=status.HTTP_404_NOT_FOUND,
                detail=result["message"]
            )
        elif result["status"] == "invalid_format":
            raise HTTPException(
                status_code=status.HTTP_400_BAD_REQUEST,
                detail=result["message"]
            )
        
        raise HTTPException(
            status_code=status_code,
            detail=result["message"]
        )
    
    return result


@router.get("/info/{license_key}")
async def get_license_info(
    license_key: str,
    db: AsyncSession = Depends(get_db)
):
    """
    Get detailed license information.
    
    Returns full license details including organization, limits, and pricing.
    """
    info = await LicenseValidationService.get_license_info(db, license_key)
    
    if not info:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="License not found"
        )
    
    return info


@router.post("/check-limits")
async def check_license_limits(
    request: LicenseLimitsCheckRequest,
    db: AsyncSession = Depends(get_db)
):
    """
    Check if current resource usage is within license limits.
    
    ThemisDB instances can call this to verify they are compliant
    with their license restrictions.
    """
    result = await LicenseValidationService.check_license_limits(
        db,
        request.license_key,
        request.current_nodes,
        request.current_cores,
        request.current_storage_tb
    )
    
    if not result.get("compliant", False):
        return {
            **result,
            "warning": "Current resource usage exceeds license limits"
        }
    
    return result


@router.get("/health")
async def license_service_health():
    """
    Health check endpoint for license validation service.
    
    ThemisDB instances can use this to verify the license service is available.
    """
    return {
        "status": "healthy",
        "service": "license_validation",
        "version": "1.0.0"
    }
