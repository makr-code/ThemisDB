"""Repository interfaces for data access layer."""

from abc import ABC, abstractmethod
from typing import List, Optional, Dict
from datetime import datetime
from sqlalchemy.ext.asyncio import AsyncSession

from models import Customer, Subscription, Payment, InstanceTelemetry, PricingTier, SubscriptionStatus, PaymentStatus


class ICustomerRepository(ABC):
    """Interface for customer repository."""
    
    @abstractmethod
    async def create(self, customer: Customer) -> Customer:
        """Create a new customer."""
        pass
    
    @abstractmethod
    async def get_by_id(self, customer_id: int) -> Optional[Customer]:
        """Get customer by ID."""
        pass
    
    @abstractmethod
    async def get_by_email(self, email: str) -> Optional[Customer]:
        """Get customer by email."""
        pass
    
    @abstractmethod
    async def update(self, customer: Customer) -> Customer:
        """Update customer."""
        pass
    
    @abstractmethod
    async def delete(self, customer_id: int) -> bool:
        """Delete customer (soft delete)."""
        pass
    
    @abstractmethod
    async def exists_by_email(self, email: str) -> bool:
        """Check if customer exists by email."""
        pass


class ISubscriptionRepository(ABC):
    """Interface for subscription repository."""
    
    @abstractmethod
    async def create(self, subscription: Subscription) -> Subscription:
        """Create a new subscription."""
        pass
    
    @abstractmethod
    async def get_by_id(self, subscription_id: int) -> Optional[Subscription]:
        """Get subscription by ID."""
        pass
    
    @abstractmethod
    async def get_by_license_key(self, license_key: str) -> Optional[Subscription]:
        """Get subscription by license key."""
        pass
    
    @abstractmethod
    async def get_by_customer_id(self, customer_id: int) -> List[Subscription]:
        """Get all subscriptions for a customer."""
        pass
    
    @abstractmethod
    async def update(self, subscription: Subscription) -> Subscription:
        """Update subscription."""
        pass
    
    @abstractmethod
    async def get_expired_subscriptions(self) -> List[Subscription]:
        """Get all expired but still marked as active subscriptions."""
        pass


class IPaymentRepository(ABC):
    """Interface for payment repository."""
    
    @abstractmethod
    async def create(self, payment: Payment) -> Payment:
        """Create a new payment."""
        pass
    
    @abstractmethod
    async def get_by_id(self, payment_id: int) -> Optional[Payment]:
        """Get payment by ID."""
        pass
    
    @abstractmethod
    async def get_by_transaction_id(self, transaction_id: str) -> Optional[Payment]:
        """Get payment by transaction ID."""
        pass
    
    @abstractmethod
    async def get_by_customer_id(self, customer_id: int) -> List[Payment]:
        """Get all payments for a customer."""
        pass
    
    @abstractmethod
    async def get_by_subscription_id(self, subscription_id: int) -> List[Payment]:
        """Get all payments for a subscription."""
        pass
    
    @abstractmethod
    async def update(self, payment: Payment) -> Payment:
        """Update payment."""
        pass


class ITelemetryRepository(ABC):
    """Interface for telemetry repository."""
    
    @abstractmethod
    async def create_or_update(self, telemetry: InstanceTelemetry) -> InstanceTelemetry:
        """Create new telemetry record or update existing one."""
        pass
    
    @abstractmethod
    async def get_by_instance_id(self, instance_id: str) -> Optional[InstanceTelemetry]:
        """Get telemetry by instance ID."""
        pass
    
    @abstractmethod
    async def get_by_license_key(self, license_key: str) -> List[InstanceTelemetry]:
        """Get all telemetry records for a license key."""
        pass
    
    @abstractmethod
    async def get_active_instances(self, hours: int = 24) -> List[InstanceTelemetry]:
        """Get instances that reported in the last N hours."""
        pass
    
    @abstractmethod
    async def get_statistics(self) -> Dict:
        """Get aggregated telemetry statistics."""
        pass
    
    @abstractmethod
    async def cleanup_old_records(self, days: int = 90) -> int:
        """Delete telemetry records older than N days."""
        pass
