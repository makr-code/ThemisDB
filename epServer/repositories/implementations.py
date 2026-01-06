"""Repository implementations for data access layer."""

from datetime import datetime, timezone, timedelta
from typing import List, Optional, Dict
from sqlalchemy import select, func, and_
from sqlalchemy.ext.asyncio import AsyncSession

from models import Customer, Subscription, Payment, InstanceTelemetry, SubscriptionStatus, PricingTier
from repositories.interfaces import ICustomerRepository, ISubscriptionRepository, IPaymentRepository, ITelemetryRepository


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


class TelemetryRepository(ITelemetryRepository):
    """SQLAlchemy implementation of telemetry repository."""
    
    def __init__(self, db: AsyncSession):
        self._db = db
    
    async def create_or_update(self, telemetry: InstanceTelemetry) -> InstanceTelemetry:
        """Create new telemetry record or update existing one."""
        # Check if instance already exists
        result = await self._db.execute(
            select(InstanceTelemetry).where(
                InstanceTelemetry.instance_id == telemetry.instance_id
            )
        )
        existing = result.scalar_one_or_none()
        
        if existing:
            # Update existing record
            existing.license_key = telemetry.license_key
            existing.hostname = telemetry.hostname
            existing.version = telemetry.version
            existing.nodes_count = telemetry.nodes_count
            existing.total_cores = telemetry.total_cores
            existing.used_storage_tb = telemetry.used_storage_tb
            existing.uptime_seconds = telemetry.uptime_seconds
            existing.query_count_24h = telemetry.query_count_24h
            existing.country = telemetry.country
            existing.region = telemetry.region
            existing.last_seen = datetime.now(timezone.utc)
            existing.report_count += 1
            existing.user_agent = telemetry.user_agent
            existing.ip_address = telemetry.ip_address
            
            await self._db.flush()
            await self._db.refresh(existing)
            return existing
        else:
            # Create new record
            self._db.add(telemetry)
            await self._db.flush()
            await self._db.refresh(telemetry)
            return telemetry
    
    async def get_by_instance_id(self, instance_id: str) -> Optional[InstanceTelemetry]:
        """Get telemetry by instance ID."""
        result = await self._db.execute(
            select(InstanceTelemetry).where(InstanceTelemetry.instance_id == instance_id)
        )
        return result.scalar_one_or_none()
    
    async def get_by_license_key(self, license_key: str) -> List[InstanceTelemetry]:
        """Get all telemetry records for a license key."""
        result = await self._db.execute(
            select(InstanceTelemetry).where(InstanceTelemetry.license_key == license_key)
        )
        return list(result.scalars().all())
    
    async def get_active_instances(self, hours: int = 24) -> List[InstanceTelemetry]:
        """Get instances that reported in the last N hours."""
        cutoff = datetime.now(timezone.utc) - timedelta(hours=hours)
        result = await self._db.execute(
            select(InstanceTelemetry).where(InstanceTelemetry.last_seen >= cutoff)
        )
        return list(result.scalars().all())
    
    async def get_statistics(self) -> Dict:
        """Get aggregated telemetry statistics."""
        # Total instances
        total_result = await self._db.execute(
            select(func.count(InstanceTelemetry.id))
        )
        total_instances = total_result.scalar() or 0
        
        # Active instances in last 24h
        cutoff_24h = datetime.now(timezone.utc) - timedelta(hours=24)
        active_result = await self._db.execute(
            select(func.count(InstanceTelemetry.id)).where(
                InstanceTelemetry.last_seen >= cutoff_24h
            )
        )
        active_instances_24h = active_result.scalar() or 0
        
        # Get active instances for aggregation
        active_instances = await self.get_active_instances(24)
        
        # Aggregate metrics
        total_nodes = sum(inst.nodes_count for inst in active_instances)
        total_cores = sum(inst.total_cores for inst in active_instances)
        total_storage_tb = sum(inst.used_storage_tb for inst in active_instances)
        
        # Version distribution
        versions = {}
        for inst in active_instances:
            versions[inst.version] = versions.get(inst.version, 0) + 1
        
        # Country distribution
        countries = {}
        for inst in active_instances:
            if inst.country:
                countries[inst.country] = countries.get(inst.country, 0) + 1
        
        # Get tier distribution (requires joining with subscriptions)
        tier_result = await self._db.execute(
            select(Subscription.tier, func.count(InstanceTelemetry.id))
            .join(Subscription, InstanceTelemetry.license_key == Subscription.license_key)
            .where(InstanceTelemetry.last_seen >= cutoff_24h)
            .group_by(Subscription.tier)
        )
        by_tier = {tier.value: count for tier, count in tier_result.all()}
        
        return {
            "total_instances": total_instances,
            "active_instances_24h": active_instances_24h,
            "total_nodes": total_nodes,
            "total_cores": total_cores,
            "total_storage_tb": round(total_storage_tb, 2),
            "versions": versions,
            "countries": countries,
            "by_tier": by_tier
        }
    
    async def cleanup_old_records(self, days: int = 90) -> int:
        """Delete telemetry records older than N days."""
        cutoff = datetime.now(timezone.utc) - timedelta(days=days)
        
        # Get count before deletion
        count_result = await self._db.execute(
            select(func.count(InstanceTelemetry.id)).where(
                InstanceTelemetry.last_seen < cutoff
            )
        )
        count = count_result.scalar() or 0
        
        # Delete old records
        if count > 0:
            result = await self._db.execute(
                select(InstanceTelemetry).where(InstanceTelemetry.last_seen < cutoff)
            )
            old_records = result.scalars().all()
            for record in old_records:
                await self._db.delete(record)
            await self._db.flush()
        
        return count
