"""Payment processing service with banking interface."""

from datetime import datetime
from typing import Optional, Dict, Any
from sqlalchemy import select
from sqlalchemy.ext.asyncio import AsyncSession
import httpx
import secrets

from models import (
    Payment,
    PaymentCreate,
    PaymentStatus,
    Subscription
)
from config import settings
from services.subscription_service import SubscriptionService


class BankingInterface:
    """Abstract banking interface for payment verification."""
    
    def __init__(self, api_url: Optional[str] = None, api_key: Optional[str] = None):
        """Initialize banking interface."""
        self.api_url = api_url or settings.banking_api_url
        self.api_key = api_key or settings.banking_api_key
    
    async def verify_payment(self, transaction_id: str) -> Dict[str, Any]:
        """Verify payment with banking system.
        
        This is a mock implementation. In production, this would connect to
        actual banking APIs (SEPA, SWIFT, etc.) or payment gateways.
        """
        if not self.api_url or not self.api_key:
            # Mock response for testing
            return {
                "verified": True,
                "status": "completed",
                "amount": 5000.0,
                "currency": "EUR",
                "timestamp": datetime.utcnow().isoformat()
            }
        
        # Real implementation would call banking API
        async with httpx.AsyncClient() as client:
            try:
                response = await client.post(
                    f"{self.api_url}/verify",
                    json={"transaction_id": transaction_id},
                    headers={"Authorization": f"Bearer {self.api_key}"},
                    timeout=30.0
                )
                response.raise_for_status()
                return response.json()
            except httpx.HTTPError as e:
                return {
                    "verified": False,
                    "error": str(e)
                }
    
    async def initiate_payment(
        self,
        amount: float,
        currency: str,
        customer_email: str,
        description: str
    ) -> Dict[str, Any]:
        """Initiate a payment request.
        
        This would typically create a payment intent with Stripe, PayPal,
        or direct banking integration (SEPA Direct Debit, etc.)
        """
        if not self.api_url or not self.api_key:
            # Mock response for testing
            transaction_id = f"MOCK-{secrets.token_hex(8).upper()}"
            return {
                "success": True,
                "transaction_id": transaction_id,
                "payment_url": f"https://payment.example.com/pay/{transaction_id}",
                "expires_at": (datetime.utcnow().timestamp() + 3600)
            }
        
        # Real implementation would call banking API
        async with httpx.AsyncClient() as client:
            try:
                response = await client.post(
                    f"{self.api_url}/initiate",
                    json={
                        "amount": amount,
                        "currency": currency,
                        "customer_email": customer_email,
                        "description": description
                    },
                    headers={"Authorization": f"Bearer {self.api_key}"},
                    timeout=30.0
                )
                response.raise_for_status()
                return response.json()
            except httpx.HTTPError as e:
                return {
                    "success": False,
                    "error": str(e)
                }


class PaymentService:
    """Service for payment processing operations."""
    
    def __init__(self):
        """Initialize payment service."""
        self.banking = BankingInterface()
    
    async def create_payment(
        self,
        db: AsyncSession,
        customer_id: int,
        payment_data: PaymentCreate
    ) -> Payment:
        """Create a new payment record."""
        # Verify subscription exists
        subscription = await SubscriptionService.get_subscription_by_id(
            db, payment_data.subscription_id
        )
        
        if not subscription:
            raise ValueError("Subscription not found")
        
        if subscription.customer_id != customer_id:
            raise ValueError("Subscription does not belong to this customer")
        
        # Generate transaction ID
        transaction_id = f"TX-{secrets.token_hex(12).upper()}"
        
        # Create payment record
        db_payment = Payment(
            customer_id=customer_id,
            subscription_id=payment_data.subscription_id,
            amount=payment_data.amount,
            currency=payment_data.currency,
            status=PaymentStatus.PENDING,
            payment_method=payment_data.payment_method,
            transaction_id=transaction_id
        )
        
        db.add(db_payment)
        await db.flush()
        await db.refresh(db_payment)
        
        return db_payment
    
    async def initiate_payment(
        self,
        db: AsyncSession,
        payment_id: int,
        customer_email: str
    ) -> Dict[str, Any]:
        """Initiate payment with banking system."""
        payment = await self.get_payment_by_id(db, payment_id)
        
        if not payment:
            raise ValueError("Payment not found")
        
        # Get subscription details
        subscription = await SubscriptionService.get_subscription_by_id(
            db, payment.subscription_id
        )
        
        description = f"ThemisDB {subscription.tier.value.title()} Subscription"
        
        # Initiate payment with banking system
        result = await self.banking.initiate_payment(
            amount=payment.amount,
            currency=payment.currency,
            customer_email=customer_email,
            description=description
        )
        
        if result.get("success"):
            # Store external payment ID if provided
            if "external_id" in result:
                payment.external_payment_id = result["external_id"]
                await db.flush()
        
        return result
    
    async def verify_payment(
        self,
        db: AsyncSession,
        transaction_id: str
    ) -> Optional[Payment]:
        """Verify a payment with banking system and update status."""
        # Get payment by transaction ID
        result = await db.execute(
            select(Payment).where(Payment.transaction_id == transaction_id)
        )
        payment = result.scalar_one_or_none()
        
        if not payment:
            return None
        
        # Verify with banking system
        verification = await self.banking.verify_payment(transaction_id)
        
        if verification.get("verified") and verification.get("status") == "completed":
            payment.status = PaymentStatus.COMPLETED
            
            # Activate subscription automatically
            await SubscriptionService.activate_subscription(
                db, payment.subscription_id, duration_months=12
            )
        else:
            payment.status = PaymentStatus.FAILED
        
        await db.flush()
        await db.refresh(payment)
        
        return payment
    
    async def get_payment_by_id(self, db: AsyncSession, payment_id: int) -> Optional[Payment]:
        """Get payment by ID."""
        result = await db.execute(
            select(Payment).where(Payment.id == payment_id)
        )
        return result.scalar_one_or_none()
    
    async def get_customer_payments(self, db: AsyncSession, customer_id: int):
        """Get all payments for a customer."""
        result = await db.execute(
            select(Payment).where(Payment.customer_id == customer_id)
        )
        return result.scalars().all()
    
    async def handle_webhook(
        self,
        db: AsyncSession,
        transaction_id: str,
        status: str,
        external_payment_id: Optional[str] = None
    ) -> Optional[Payment]:
        """Handle payment webhook from banking system."""
        # Get payment by transaction ID
        result = await db.execute(
            select(Payment).where(Payment.transaction_id == transaction_id)
        )
        payment = result.scalar_one_or_none()
        
        if not payment:
            return None
        
        # Update payment status
        if status == "completed":
            payment.status = PaymentStatus.COMPLETED
            
            # Activate subscription automatically
            await SubscriptionService.activate_subscription(
                db, payment.subscription_id, duration_months=12
            )
        elif status == "failed":
            payment.status = PaymentStatus.FAILED
        elif status == "refunded":
            payment.status = PaymentStatus.REFUNDED
            
            # Cancel subscription on refund
            await SubscriptionService.cancel_subscription(db, payment.subscription_id)
        
        if external_payment_id:
            payment.external_payment_id = external_payment_id
        
        await db.flush()
        await db.refresh(payment)
        
        return payment
