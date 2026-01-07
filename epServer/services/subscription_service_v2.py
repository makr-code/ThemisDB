"""Subscription management service with proper OOP and dependency injection."""

from datetime import datetime, timedelta, timezone
from typing import List, Optional, Dict
from sqlalchemy.ext.asyncio import AsyncSession

from models import (
    Subscription,
    SubscriptionCreate,
    SubscriptionStatus,
    PricingTier
)
from repositories.interfaces import ISubscriptionRepository
from repositories.implementations import SubscriptionRepository
from utils.license import generate_license_key
from exceptions import (
    SubscriptionNotFoundException,
    InvalidSubscriptionStateException
)


class PricingConfiguration:
    """Configuration for pricing tiers."""
    
    def __init__(
        self,
        community_price: float = 0.0,
        enterprise_price: float = 5000.0,
        hyperscaler_price: float = 25000.0,
        reseller_price: float = 15000.0
    ):
        self._prices = {
            PricingTier.COMMUNITY: community_price,
            PricingTier.ENTERPRISE: enterprise_price,
            PricingTier.HYPERSCALER: hyperscaler_price,
            PricingTier.RESELLER: reseller_price
        }
    
    def get_price(self, tier: PricingTier) -> float:
        """Get price for a tier."""
        return self._prices.get(tier, 0.0)
    
    def get_all_prices(self) -> Dict[PricingTier, float]:
        """Get all tier prices."""
        return self._prices.copy()


class ResourceLimitCalculator:
    """Calculator for resource limits based on pricing tier."""
    
    @staticmethod
    def calculate_limits(tier: PricingTier, requested_nodes: int) -> Dict[str, int]:
        """
        Calculate resource limits for a subscription tier.
        
        Args:
            tier: Pricing tier
            requested_nodes: Number of nodes requested
            
        Returns:
            Dictionary with max_nodes, max_cores, max_storage_tb
        """
        if tier == PricingTier.COMMUNITY:
            return {
                "max_nodes": 1,
                "max_cores": 8,
                "max_storage_tb": 1
            }
        elif tier == PricingTier.ENTERPRISE:
            return {
                "max_nodes": min(requested_nodes, 100),
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
                "max_nodes": requested_nodes,
                "max_cores": -1,  # unlimited
                "max_storage_tb": -1  # unlimited
            }
        
        # Default (Community)
        return {
            "max_nodes": 1,
            "max_cores": 8,
            "max_storage_tb": 1
        }


class SubscriptionService:
    """Service for subscription management operations using OOP principles."""
    
    def __init__(
        self,
        db: AsyncSession,
        repository: Optional[ISubscriptionRepository] = None,
        pricing_config: Optional[PricingConfiguration] = None,
        limit_calculator: Optional[ResourceLimitCalculator] = None
    ):
        """
        Initialize subscription service with dependency injection.
        
        Args:
            db: Database session
            repository: Subscription repository (injected for testability)
            pricing_config: Pricing configuration (injected for testability)
            limit_calculator: Resource limit calculator (injected for testability)
        """
        self._db = db
        self._repository = repository or SubscriptionRepository(db)
        self._pricing_config = pricing_config or PricingConfiguration()
        self._limit_calculator = limit_calculator or ResourceLimitCalculator()
    
    async def create_subscription(
        self,
        customer_id: int,
        subscription_data: SubscriptionCreate,
        customer_email: str
    ) -> Subscription:
        """
        Create a new subscription.
        
        Args:
            customer_id: Customer ID
            subscription_data: Subscription creation data
            customer_email: Customer email for license key generation
            
        Returns:
            Created subscription
        """
        # Get price and limits
        price = self._pricing_config.get_price(subscription_data.tier)
        limits = self._limit_calculator.calculate_limits(
            subscription_data.tier,
            subscription_data.max_nodes
        )
        
        # Generate license key
        license_key = generate_license_key(customer_email, subscription_data.tier.value)
        
        # Create subscription
        subscription = Subscription(
            customer_id=customer_id,
            tier=subscription_data.tier,
            status=SubscriptionStatus.PENDING,  # Pending until payment
            license_key=license_key,
            max_nodes=limits["max_nodes"],
            max_cores=limits["max_cores"],
            max_storage_tb=limits["max_storage_tb"],
            price_per_month=price
        )
        
        return await self._repository.create(subscription)
    
    async def get_subscription_by_id(self, subscription_id: int) -> Subscription:
        """
        Get subscription by ID.
        
        Args:
            subscription_id: Subscription ID
            
        Returns:
            Subscription
            
        Raises:
            SubscriptionNotFoundException: If subscription not found
        """
        subscription = await self._repository.get_by_id(subscription_id)
        if not subscription:
            raise SubscriptionNotFoundException(subscription_id)
        
        return subscription
    
    async def get_subscription_by_license_key(self, license_key: str) -> Subscription:
        """
        Get subscription by license key.
        
        Args:
            license_key: License key
            
        Returns:
            Subscription
            
        Raises:
            SubscriptionNotFoundException: If subscription not found
        """
        subscription = await self._repository.get_by_license_key(license_key)
        if not subscription:
            raise SubscriptionNotFoundException(0)  # Use 0 since we don't have ID
        
        return subscription
    
    async def get_customer_subscriptions(self, customer_id: int) -> List[Subscription]:
        """
        Get all subscriptions for a customer.
        
        Args:
            customer_id: Customer ID
            
        Returns:
            List of subscriptions
        """
        return await self._repository.get_by_customer_id(customer_id)
    
    async def activate_subscription(
        self,
        subscription_id: int,
        duration_months: int = 12
    ) -> Subscription:
        """
        Activate a subscription after successful payment.
        
        Args:
            subscription_id: Subscription ID
            duration_months: Duration in months
            
        Returns:
            Activated subscription
            
        Raises:
            SubscriptionNotFoundException: If subscription not found
        """
        subscription = await self.get_subscription_by_id(subscription_id)
        
        subscription.status = SubscriptionStatus.ACTIVE
        subscription.start_date = datetime.now(timezone.utc)
        subscription.end_date = datetime.now(timezone.utc) + timedelta(days=30 * duration_months)
        
        return await self._repository.update(subscription)
    
    async def cancel_subscription(self, subscription_id: int) -> Subscription:
        """
        Cancel a subscription.
        
        Args:
            subscription_id: Subscription ID
            
        Returns:
            Cancelled subscription
            
        Raises:
            SubscriptionNotFoundException: If subscription not found
        """
        subscription = await self.get_subscription_by_id(subscription_id)
        
        subscription.status = SubscriptionStatus.CANCELLED
        
        return await self._repository.update(subscription)
    
    async def suspend_subscription(self, subscription_id: int) -> Subscription:
        """
        Suspend a subscription.
        
        Args:
            subscription_id: Subscription ID
            
        Returns:
            Suspended subscription
            
        Raises:
            SubscriptionNotFoundException: If subscription not found
        """
        subscription = await self.get_subscription_by_id(subscription_id)
        
        subscription.status = SubscriptionStatus.SUSPENDED
        
        return await self._repository.update(subscription)
    
    async def check_and_expire_subscriptions(self) -> int:
        """
        Check all active subscriptions and expire those past end_date.
        
        Returns:
            Number of subscriptions expired
        """
        expired_subscriptions = await self._repository.get_expired_subscriptions()
        
        for subscription in expired_subscriptions:
            subscription.status = SubscriptionStatus.EXPIRED
            await self._repository.update(subscription)
        
        return len(expired_subscriptions)
    
    async def is_subscription_active(self, subscription_id: int) -> bool:
        """
        Check if subscription is active and not expired.
        
        Args:
            subscription_id: Subscription ID
            
        Returns:
            True if active, False otherwise
            
        Raises:
            SubscriptionNotFoundException: If subscription not found
        """
        subscription = await self.get_subscription_by_id(subscription_id)
        
        if subscription.status != SubscriptionStatus.ACTIVE:
            return False
        
        if subscription.end_date and datetime.now(timezone.utc) > subscription.end_date:
            return False
        
        return True
