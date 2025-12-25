"""Payment processing service with proper OOP and dependency injection."""

from abc import ABC, abstractmethod
from datetime import datetime, timezone
from typing import Optional, Dict, Any, List
from sqlalchemy.ext.asyncio import AsyncSession
import httpx
import secrets

from models import (
    Payment,
    PaymentCreate,
    PaymentStatus
)
from repositories.interfaces import IPaymentRepository
from repositories.implementations import PaymentRepository
from services.subscription_service_v2 import SubscriptionService
from exceptions import (
    PaymentNotFoundException,
    PaymentVerificationException,
    BankingInterfaceException,
    SubscriptionNotFoundException
)


class IBankingProvider(ABC):
    """Abstract interface for banking/payment providers."""
    
    @abstractmethod
    async def verify_payment(self, transaction_id: str) -> Dict[str, Any]:
        """Verify a payment transaction."""
        pass
    
    @abstractmethod
    async def initiate_payment(
        self,
        amount: float,
        currency: str,
        customer_email: str,
        description: str
    ) -> Dict[str, Any]:
        """Initiate a new payment."""
        pass
    
    @abstractmethod
    async def refund_payment(self, transaction_id: str, amount: Optional[float] = None) -> Dict[str, Any]:
        """Refund a payment."""
        pass


class MockBankingProvider(IBankingProvider):
    """Mock banking provider for testing."""
    
    async def verify_payment(self, transaction_id: str) -> Dict[str, Any]:
        """Verify payment with mock response."""
        return {
            "verified": True,
            "status": "completed",
            "amount": 5000.0,
            "currency": "EUR",
            "timestamp": datetime.now(timezone.utc).isoformat()
        }
    
    async def initiate_payment(
        self,
        amount: float,
        currency: str,
        customer_email: str,
        description: str
    ) -> Dict[str, Any]:
        """Initiate payment with mock response."""
        transaction_id = f"MOCK-{secrets.token_hex(8).upper()}"
        return {
            "success": True,
            "transaction_id": transaction_id,
            "payment_url": f"https://payment.example.com/pay/{transaction_id}",
            "expires_at": (datetime.now(timezone.utc).timestamp() + 3600)
        }
    
    async def refund_payment(self, transaction_id: str, amount: Optional[float] = None) -> Dict[str, Any]:
        """Refund payment with mock response."""
        return {
            "success": True,
            "refund_id": f"REFUND-{secrets.token_hex(8).upper()}",
            "amount": amount,
            "status": "refunded"
        }


class RealBankingProvider(IBankingProvider):
    """Real banking provider implementation."""
    
    def __init__(self, api_url: str, api_key: str):
        """
        Initialize banking provider.
        
        Args:
            api_url: Banking API URL
            api_key: API key for authentication
        """
        self._api_url = api_url
        self._api_key = api_key
        self._client = httpx.AsyncClient(timeout=30.0)
    
    async def verify_payment(self, transaction_id: str) -> Dict[str, Any]:
        """Verify payment with banking system."""
        try:
            response = await self._client.post(
                f"{self._api_url}/verify",
                json={"transaction_id": transaction_id},
                headers={"Authorization": f"Bearer {self._api_key}"}
            )
            response.raise_for_status()
            return response.json()
        except httpx.HTTPError as e:
            raise BankingInterfaceException("verify_payment", str(e))
    
    async def initiate_payment(
        self,
        amount: float,
        currency: str,
        customer_email: str,
        description: str
    ) -> Dict[str, Any]:
        """Initiate payment with banking system."""
        try:
            response = await self._client.post(
                f"{self._api_url}/initiate",
                json={
                    "amount": amount,
                    "currency": currency,
                    "customer_email": customer_email,
                    "description": description
                },
                headers={"Authorization": f"Bearer {self._api_key}"}
            )
            response.raise_for_status()
            return response.json()
        except httpx.HTTPError as e:
            raise BankingInterfaceException("initiate_payment", str(e))
    
    async def refund_payment(self, transaction_id: str, amount: Optional[float] = None) -> Dict[str, Any]:
        """Refund payment with banking system."""
        try:
            payload = {"transaction_id": transaction_id}
            if amount is not None:
                payload["amount"] = amount
            
            response = await self._client.post(
                f"{self._api_url}/refund",
                json=payload,
                headers={"Authorization": f"Bearer {self._api_key}"}
            )
            response.raise_for_status()
            return response.json()
        except httpx.HTTPError as e:
            raise BankingInterfaceException("refund_payment", str(e))
    
    async def close(self):
        """Close the HTTP client."""
        await self._client.aclose()


class PaymentService:
    """Service for payment processing operations using OOP principles."""
    
    def __init__(
        self,
        db: AsyncSession,
        subscription_service: SubscriptionService,
        repository: Optional[IPaymentRepository] = None,
        banking_provider: Optional[IBankingProvider] = None
    ):
        """
        Initialize payment service with dependency injection.
        
        Args:
            db: Database session
            subscription_service: Subscription service (injected)
            repository: Payment repository (injected for testability)
            banking_provider: Banking provider implementation (injected for testability)
        """
        self._db = db
        self._subscription_service = subscription_service
        self._repository = repository or PaymentRepository(db)
        self._banking_provider = banking_provider or MockBankingProvider()
    
    async def create_payment(
        self,
        customer_id: int,
        payment_data: PaymentCreate
    ) -> Payment:
        """
        Create a new payment record.
        
        Args:
            customer_id: Customer ID
            payment_data: Payment creation data
            
        Returns:
            Created payment
            
        Raises:
            SubscriptionNotFoundException: If subscription not found
        """
        # Verify subscription exists and belongs to customer
        subscription = await self._subscription_service.get_subscription_by_id(
            payment_data.subscription_id
        )
        
        if subscription.customer_id != customer_id:
            raise SubscriptionNotFoundException(payment_data.subscription_id)
        
        # Generate transaction ID
        transaction_id = f"TX-{secrets.token_hex(12).upper()}"
        
        # Create payment record
        payment = Payment(
            customer_id=customer_id,
            subscription_id=payment_data.subscription_id,
            amount=payment_data.amount,
            currency=payment_data.currency,
            status=PaymentStatus.PENDING,
            payment_method=payment_data.payment_method,
            transaction_id=transaction_id
        )
        
        return await self._repository.create(payment)
    
    async def get_payment_by_id(self, payment_id: int) -> Payment:
        """
        Get payment by ID.
        
        Args:
            payment_id: Payment ID
            
        Returns:
            Payment
            
        Raises:
            PaymentNotFoundException: If payment not found
        """
        payment = await self._repository.get_by_id(payment_id)
        if not payment:
            raise PaymentNotFoundException(payment_id)
        
        return payment
    
    async def get_customer_payments(self, customer_id: int) -> List[Payment]:
        """
        Get all payments for a customer.
        
        Args:
            customer_id: Customer ID
            
        Returns:
            List of payments
        """
        return await self._repository.get_by_customer_id(customer_id)
    
    async def initiate_payment(
        self,
        payment_id: int,
        customer_email: str
    ) -> Dict[str, Any]:
        """
        Initiate payment with banking system.
        
        Args:
            payment_id: Payment ID
            customer_email: Customer email
            
        Returns:
            Payment initiation result
            
        Raises:
            PaymentNotFoundException: If payment not found
        """
        payment = await self.get_payment_by_id(payment_id)
        
        # Get subscription details
        subscription = await self._subscription_service.get_subscription_by_id(
            payment.subscription_id
        )
        
        description = f"ThemisDB {subscription.tier.value.title()} Subscription"
        
        # Initiate payment with banking system
        result = await self._banking_provider.initiate_payment(
            amount=payment.amount,
            currency=payment.currency,
            customer_email=customer_email,
            description=description
        )
        
        # Store external payment ID if provided
        if "external_id" in result:
            payment.external_payment_id = result["external_id"]
            await self._repository.update(payment)
        
        return result
    
    async def verify_payment(self, transaction_id: str) -> Payment:
        """
        Verify a payment with banking system and update status.
        
        Args:
            transaction_id: Transaction ID
            
        Returns:
            Updated payment
            
        Raises:
            PaymentNotFoundException: If payment not found
        """
        # Get payment by transaction ID
        payment = await self._repository.get_by_transaction_id(transaction_id)
        if not payment:
            raise PaymentNotFoundException(0)
        
        # Verify with banking system
        try:
            verification = await self._banking_provider.verify_payment(transaction_id)
        except BankingInterfaceException as e:
            raise PaymentVerificationException(transaction_id, str(e))
        
        # Update payment status
        if verification.get("verified") and verification.get("status") == "completed":
            payment.status = PaymentStatus.COMPLETED
            
            # Activate subscription automatically
            await self._subscription_service.activate_subscription(
                payment.subscription_id,
                duration_months=12
            )
        else:
            payment.status = PaymentStatus.FAILED
        
        return await self._repository.update(payment)
    
    async def handle_webhook(
        self,
        transaction_id: str,
        status: str,
        external_payment_id: Optional[str] = None
    ) -> Payment:
        """
        Handle payment webhook from banking system.
        
        Args:
            transaction_id: Transaction ID
            status: Payment status from webhook
            external_payment_id: External payment ID (optional)
            
        Returns:
            Updated payment
            
        Raises:
            PaymentNotFoundException: If payment not found
        """
        # Get payment by transaction ID
        payment = await self._repository.get_by_transaction_id(transaction_id)
        if not payment:
            raise PaymentNotFoundException(0)
        
        # Update payment status
        if status == "completed":
            payment.status = PaymentStatus.COMPLETED
            
            # Activate subscription automatically
            await self._subscription_service.activate_subscription(
                payment.subscription_id,
                duration_months=12
            )
        elif status == "failed":
            payment.status = PaymentStatus.FAILED
        elif status == "refunded":
            payment.status = PaymentStatus.REFUNDED
            
            # Cancel subscription on refund
            await self._subscription_service.cancel_subscription(payment.subscription_id)
        
        if external_payment_id:
            payment.external_payment_id = external_payment_id
        
        return await self._repository.update(payment)
    
    async def refund_payment(
        self,
        payment_id: int,
        amount: Optional[float] = None
    ) -> Payment:
        """
        Refund a payment.
        
        Args:
            payment_id: Payment ID
            amount: Amount to refund (None for full refund)
            
        Returns:
            Updated payment
            
        Raises:
            PaymentNotFoundException: If payment not found
        """
        payment = await self.get_payment_by_id(payment_id)
        
        # Initiate refund with banking system
        await self._banking_provider.refund_payment(
            payment.transaction_id,
            amount
        )
        
        # Update payment status
        payment.status = PaymentStatus.REFUNDED
        
        # Cancel associated subscription
        await self._subscription_service.cancel_subscription(payment.subscription_id)
        
        return await self._repository.update(payment)
