"""Subscription management service."""

from datetime import datetime, timedelta, timezone
from typing import List, Optional
from sqlalchemy import select
from sqlalchemy.ext.asyncio import AsyncSession

from models import (
    Subscription,
    SubscriptionCreate,
    SubscriptionStatus,
    PricingTier
)
from config import settings
from utils.license import generate_license_key


class SubscriptionService:
    """Service for subscription management operations."""
    
    @staticmethod
    def _get_tier_price(tier: PricingTier) -> float:
        """Get price for a subscription tier."""
        prices = {
            PricingTier.COMMUNITY: settings.community_price,
            PricingTier.ENTERPRISE: settings.enterprise_price,
            PricingTier.HYPERSCALER: settings.hyperscaler_price,
            PricingTier.RESELLER: settings.reseller_price
        }
        return prices.get(tier, 0.0)
    
    @staticmethod
    def _get_tier_limits(tier: PricingTier, max_nodes: int) -> dict:
        """Get resource limits for a subscription tier."""
        if tier == PricingTier.COMMUNITY:
            return {
                "max_nodes": 1,
                "max_cores": 8,
                "max_storage_tb": 1
            }
        elif tier == PricingTier.ENTERPRISE:
            return {
                "max_nodes": min(max_nodes, 100),
                "max_cores": -1,  # unlimited
                "max_storage_tb": -1  # unlimited
            }
        elif tier == PricingTier.HYPERSCALER:
            return {
                "max_nodes": -1,  # unlimited
                "max_cores": -1,  # unlimited
                "max_storage_tb": -1  # unlimited
            }
        elif tier == PricingTier.RESELLER:
            return {
                "max_nodes": max_nodes,
                "max_cores": -1,  # unlimited
                "max_storage_tb": -1  # unlimited
            }
        
        return {"max_nodes": 1, "max_cores": 8, "max_storage_tb": 1}
    
    @staticmethod
    async def create_subscription(
        db: AsyncSession,
        customer_id: int,
        subscription_data: SubscriptionCreate,
        customer_email: str
    ) -> Subscription:
        """Create a new subscription."""
        # Get price and limits
        price = SubscriptionService._get_tier_price(subscription_data.tier)
        limits = SubscriptionService._get_tier_limits(subscription_data.tier, subscription_data.max_nodes)
        
        # Generate license key
        license_key = generate_license_key(customer_email, subscription_data.tier.value)
        
        # Create subscription
        db_subscription = Subscription(
            customer_id=customer_id,
            tier=subscription_data.tier,
            status=SubscriptionStatus.PENDING,  # Pending until payment
            license_key=license_key,
            max_nodes=limits["max_nodes"],
            max_cores=limits["max_cores"],
            max_storage_tb=limits["max_storage_tb"],
            price_per_month=price
        )
        
        db.add(db_subscription)
        await db.flush()
        await db.refresh(db_subscription)
        
        return db_subscription
    
    @staticmethod
    async def get_subscription_by_id(db: AsyncSession, subscription_id: int) -> Optional[Subscription]:
        """Get subscription by ID."""
        result = await db.execute(
            select(Subscription).where(Subscription.id == subscription_id)
        )
        return result.scalar_one_or_none()
    
    @staticmethod
    async def get_customer_subscriptions(db: AsyncSession, customer_id: int) -> List[Subscription]:
        """Get all subscriptions for a customer."""
        result = await db.execute(
            select(Subscription).where(Subscription.customer_id == customer_id)
        )
        return result.scalars().all()
    
    @staticmethod
    async def activate_subscription(
        db: AsyncSession,
        subscription_id: int,
        duration_months: int = 12
    ) -> Optional[Subscription]:
        """Activate a subscription after successful payment."""
        subscription = await SubscriptionService.get_subscription_by_id(db, subscription_id)
        
        if not subscription:
            return None
        
        subscription.status = SubscriptionStatus.ACTIVE
        subscription.start_date = datetime.now(timezone.utc)
        subscription.end_date = datetime.now(timezone.utc) + timedelta(days=30 * duration_months)
        
        await db.flush()
        await db.refresh(subscription)
        
        return subscription
    
    @staticmethod
    async def cancel_subscription(db: AsyncSession, subscription_id: int) -> Optional[Subscription]:
        """Cancel a subscription."""
        subscription = await SubscriptionService.get_subscription_by_id(db, subscription_id)
        
        if not subscription:
            return None
        
        subscription.status = SubscriptionStatus.CANCELLED
        
        await db.flush()
        await db.refresh(subscription)
        
        return subscription
    
    @staticmethod
    async def check_subscription_expiry(db: AsyncSession, subscription_id: int) -> bool:
        """Check if subscription has expired and update status."""
        subscription = await SubscriptionService.get_subscription_by_id(db, subscription_id)
        
        if not subscription or not subscription.end_date:
            return False
        
        if datetime.now(timezone.utc) > subscription.end_date and subscription.status == SubscriptionStatus.ACTIVE:
            subscription.status = SubscriptionStatus.EXPIRED
            await db.flush()
            return True
        
        return False
