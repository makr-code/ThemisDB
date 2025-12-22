"""Repository implementations for data access layer."""

from datetime import datetime, timezone
from typing import List, Optional
from sqlalchemy import select
from sqlalchemy.ext.asyncio import AsyncSession

from models import Customer, Subscription, Payment, SubscriptionStatus
from repositories.interfaces import ICustomerRepository, ISubscriptionRepository, IPaymentRepository


class CustomerRepository(ICustomerRepository):
    """SQLAlchemy implementation of customer repository."""
    
    def __init__(self, db: AsyncSession):
        self._db = db
    
    async def create(self, customer: Customer) -> Customer:
        """Create a new customer."""
        self._db.add(customer)
        await self._db.flush()
        await self._db.refresh(customer)
        return customer
    
    async def get_by_id(self, customer_id: int) -> Optional[Customer]:
        """Get customer by ID."""
        result = await self._db.execute(
            select(Customer).where(Customer.id == customer_id)
        )
        return result.scalar_one_or_none()
    
    async def get_by_email(self, email: str) -> Optional[Customer]:
        """Get customer by email."""
        result = await self._db.execute(
            select(Customer).where(Customer.email == email)
        )
        return result.scalar_one_or_none()
    
    async def update(self, customer: Customer) -> Customer:
        """Update customer."""
        await self._db.flush()
        await self._db.refresh(customer)
        return customer
    
    async def delete(self, customer_id: int) -> bool:
        """Delete customer (soft delete)."""
        customer = await self.get_by_id(customer_id)
        if not customer:
            return False
        
        customer.is_active = False
        await self._db.flush()
        return True
    
    async def exists_by_email(self, email: str) -> bool:
        """Check if customer exists by email."""
        customer = await self.get_by_email(email)
        return customer is not None


class SubscriptionRepository(ISubscriptionRepository):
    """SQLAlchemy implementation of subscription repository."""
    
    def __init__(self, db: AsyncSession):
        self._db = db
    
    async def create(self, subscription: Subscription) -> Subscription:
        """Create a new subscription."""
        self._db.add(subscription)
        await self._db.flush()
        await self._db.refresh(subscription)
        return subscription
    
    async def get_by_id(self, subscription_id: int) -> Optional[Subscription]:
        """Get subscription by ID."""
        result = await self._db.execute(
            select(Subscription).where(Subscription.id == subscription_id)
        )
        return result.scalar_one_or_none()
    
    async def get_by_license_key(self, license_key: str) -> Optional[Subscription]:
        """Get subscription by license key."""
        result = await self._db.execute(
            select(Subscription).where(Subscription.license_key == license_key)
        )
        return result.scalar_one_or_none()
    
    async def get_by_customer_id(self, customer_id: int) -> List[Subscription]:
        """Get all subscriptions for a customer."""
        result = await self._db.execute(
            select(Subscription).where(Subscription.customer_id == customer_id)
        )
        return list(result.scalars().all())
    
    async def update(self, subscription: Subscription) -> Subscription:
        """Update subscription."""
        await self._db.flush()
        await self._db.refresh(subscription)
        return subscription
    
    async def get_expired_subscriptions(self) -> List[Subscription]:
        """Get all expired but still marked as active subscriptions."""
        now = datetime.now(timezone.utc)
        result = await self._db.execute(
            select(Subscription).where(
                Subscription.status == SubscriptionStatus.ACTIVE,
                Subscription.end_date < now
            )
        )
        return list(result.scalars().all())


class PaymentRepository(IPaymentRepository):
    """SQLAlchemy implementation of payment repository."""
    
    def __init__(self, db: AsyncSession):
        self._db = db
    
    async def create(self, payment: Payment) -> Payment:
        """Create a new payment."""
        self._db.add(payment)
        await self._db.flush()
        await self._db.refresh(payment)
        return payment
    
    async def get_by_id(self, payment_id: int) -> Optional[Payment]:
        """Get payment by ID."""
        result = await self._db.execute(
            select(Payment).where(Payment.id == payment_id)
        )
        return result.scalar_one_or_none()
    
    async def get_by_transaction_id(self, transaction_id: str) -> Optional[Payment]:
        """Get payment by transaction ID."""
        result = await self._db.execute(
            select(Payment).where(Payment.transaction_id == transaction_id)
        )
        return result.scalar_one_or_none()
    
    async def get_by_customer_id(self, customer_id: int) -> List[Payment]:
        """Get all payments for a customer."""
        result = await self._db.execute(
            select(Payment).where(Payment.customer_id == customer_id)
        )
        return list(result.scalars().all())
    
    async def get_by_subscription_id(self, subscription_id: int) -> List[Payment]:
        """Get all payments for a subscription."""
        result = await self._db.execute(
            select(Payment).where(Payment.subscription_id == subscription_id)
        )
        return list(result.scalars().all())
    
    async def update(self, payment: Payment) -> Payment:
        """Update payment."""
        await self._db.flush()
        await self._db.refresh(payment)
        return payment
