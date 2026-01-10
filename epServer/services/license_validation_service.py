"""License validation service for ThemisDB instances.

This service provides license verification for ThemisDB Enterprise/Hyperscaler instances.
ThemisDB servers call this service to validate their licenses at startup and periodically.
"""

from datetime import datetime, timezone
from typing import Optional, Dict, Any
from sqlalchemy import select
from sqlalchemy.ext.asyncio import AsyncSession

from models import Subscription, SubscriptionStatus
from utils.license import validate_license_key_format


class LicenseValidationService:
    """Service for validating ThemisDB licenses."""
    
    @staticmethod
    async def validate_license(
        db: AsyncSession,
        license_key: str,
        server_info: Optional[Dict[str, Any]] = None
    ) -> Dict[str, Any]:
        """
        Validate a license key and return its status and limits.
        
        Args:
            db: Database session
            license_key: The license key to validate
            server_info: Optional server information (hostname, version, etc.)
        
        Returns:
            Dict containing validation result with status, tier, limits, and expiry
        """
        # Validate format first
        if not validate_license_key_format(license_key):
            return {
                "valid": False,
                "status": "invalid_format",
                "message": "License key format is invalid",
                "tier": None,
                "limits": None
            }
        
        # Look up subscription by license key
        result = await db.execute(
            select(Subscription).where(Subscription.license_key == license_key)
        )
        subscription = result.scalar_one_or_none()
        
        if not subscription:
            return {
                "valid": False,
                "status": "not_found",
                "message": "License key not found",
                "tier": None,
                "limits": None
            }
        
        # Check subscription status
        if subscription.status == SubscriptionStatus.CANCELLED:
            return {
                "valid": False,
                "status": "cancelled",
                "message": "License has been cancelled",
                "tier": subscription.tier.value,
                "limits": None
            }
        
        if subscription.status == SubscriptionStatus.SUSPENDED:
            return {
                "valid": False,
                "status": "suspended",
                "message": "License has been suspended",
                "tier": subscription.tier.value,
                "limits": None
            }
        
        if subscription.status == SubscriptionStatus.PENDING:
            return {
                "valid": False,
                "status": "pending_payment",
                "message": "License is pending payment activation",
                "tier": subscription.tier.value,
                "limits": None
            }
        
        # Check if expired
        now = datetime.now(timezone.utc)
        if subscription.end_date and now > subscription.end_date:
            # Auto-update status to expired
            if subscription.status == SubscriptionStatus.ACTIVE:
                subscription.status = SubscriptionStatus.EXPIRED
                await db.flush()
            
            return {
                "valid": False,
                "status": "expired",
                "message": f"License expired on {subscription.end_date.isoformat()}",
                "tier": subscription.tier.value,
                "expiry_date": subscription.end_date.isoformat(),
                "limits": None
            }
        
        # License is valid and active
        limits = {
            "max_nodes": subscription.max_nodes,
            "max_cores": subscription.max_cores,
            "max_storage_tb": subscription.max_storage_tb
        }
        
        response = {
            "valid": True,
            "status": "active",
            "message": "License is valid and active",
            "tier": subscription.tier.value,
            "license_key": license_key,
            "organization": subscription.customer.organization_name if subscription.customer else None,
            "limits": limits,
            "start_date": subscription.start_date.isoformat() if subscription.start_date else None,
            "end_date": subscription.end_date.isoformat() if subscription.end_date else None,
            "days_remaining": (subscription.end_date - now).days if subscription.end_date else None
        }
        
        # Log validation request (optional - for audit purposes)
        # You could add a separate table to track validation requests
        if server_info:
            # Future: Log server_info for monitoring/compliance
            pass
        
        return response
    
    @staticmethod
    async def get_license_info(
        db: AsyncSession,
        license_key: str
    ) -> Optional[Dict[str, Any]]:
        """
        Get detailed license information (for customer portal).
        
        Args:
            db: Database session
            license_key: The license key to query
        
        Returns:
            Dict with full license details or None if not found
        """
        result = await db.execute(
            select(Subscription).where(Subscription.license_key == license_key)
        )
        subscription = result.scalar_one_or_none()
        
        if not subscription:
            return None
        
        return {
            "license_key": license_key,
            "tier": subscription.tier.value,
            "status": subscription.status.value,
            "organization": subscription.customer.organization_name if subscription.customer else None,
            "contact_email": subscription.customer.email if subscription.customer else None,
            "limits": {
                "max_nodes": subscription.max_nodes,
                "max_cores": subscription.max_cores,
                "max_storage_tb": subscription.max_storage_tb
            },
            "pricing": {
                "price_per_month": float(subscription.price_per_month),
                "currency": "EUR"
            },
            "dates": {
                "created": subscription.created_at.isoformat(),
                "start": subscription.start_date.isoformat() if subscription.start_date else None,
                "end": subscription.end_date.isoformat() if subscription.end_date else None
            }
        }
    
    @staticmethod
    async def check_license_limits(
        db: AsyncSession,
        license_key: str,
        current_nodes: int,
        current_cores: Optional[int] = None,
        current_storage_tb: Optional[float] = None
    ) -> Dict[str, Any]:
        """
        Check if current resource usage is within license limits.
        
        Args:
            db: Database session
            license_key: The license key
            current_nodes: Current number of nodes
            current_cores: Current number of cores (optional)
            current_storage_tb: Current storage in TB (optional)
        
        Returns:
            Dict with compliance status for each limit
        """
        validation = await LicenseValidationService.validate_license(db, license_key)
        
        if not validation["valid"]:
            return {
                "compliant": False,
                "reason": validation["message"],
                "limits_check": None
            }
        
        limits = validation["limits"]
        checks = {}
        compliant = True
        
        # Check nodes
        if limits["max_nodes"] != -1:  # -1 means unlimited
            nodes_ok = current_nodes <= limits["max_nodes"]
            checks["nodes"] = {
                "limit": limits["max_nodes"],
                "current": current_nodes,
                "compliant": nodes_ok
            }
            if not nodes_ok:
                compliant = False
        else:
            checks["nodes"] = {
                "limit": "unlimited",
                "current": current_nodes,
                "compliant": True
            }
        
        # Check cores
        if current_cores is not None:
            if limits["max_cores"] != -1:
                cores_ok = current_cores <= limits["max_cores"]
                checks["cores"] = {
                    "limit": limits["max_cores"],
                    "current": current_cores,
                    "compliant": cores_ok
                }
                if not cores_ok:
                    compliant = False
            else:
                checks["cores"] = {
                    "limit": "unlimited",
                    "current": current_cores,
                    "compliant": True
                }
        
        # Check storage
        if current_storage_tb is not None:
            if limits["max_storage_tb"] != -1:
                storage_ok = current_storage_tb <= limits["max_storage_tb"]
                checks["storage_tb"] = {
                    "limit": limits["max_storage_tb"],
                    "current": current_storage_tb,
                    "compliant": storage_ok
                }
                if not storage_ok:
                    compliant = False
            else:
                checks["storage_tb"] = {
                    "limit": "unlimited",
                    "current": current_storage_tb,
                    "compliant": True
                }
        
        return {
            "compliant": compliant,
            "limits_check": checks,
            "tier": validation["tier"]
        }
