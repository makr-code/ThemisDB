"""Repository module for data access layer."""

from repositories.interfaces import (
    ICustomerRepository,
    ISubscriptionRepository,
    IPaymentRepository,
    ITelemetryRepository
)
from repositories.implementations import (
    CustomerRepository,
    SubscriptionRepository,
    PaymentRepository,
    TelemetryRepository
)

__all__ = [
    # Interfaces
    "ICustomerRepository",
    "ISubscriptionRepository",
    "IPaymentRepository",
    "ITelemetryRepository",
    # Implementations
    "CustomerRepository",
    "SubscriptionRepository",
    "PaymentRepository",
    "TelemetryRepository"
]
