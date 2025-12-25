"""Data models for the pricing server."""

from datetime import datetime, timezone
from typing import Optional
from enum import Enum
from pydantic import BaseModel, EmailStr, Field
from sqlalchemy import Column, Integer, String, Float, DateTime, Boolean, ForeignKey, Enum as SQLEnum
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import relationship

Base = declarative_base()


class PricingTier(str, Enum):
    """Pricing tier enumeration."""
    COMMUNITY = "community"
    ENTERPRISE = "enterprise"
    HYPERSCALER = "hyperscaler"
    RESELLER = "reseller"


class SubscriptionStatus(str, Enum):
    """Subscription status enumeration."""
    ACTIVE = "active"
    PENDING = "pending"
    CANCELLED = "cancelled"
    EXPIRED = "expired"
    SUSPENDED = "suspended"


class PaymentStatus(str, Enum):
    """Payment status enumeration."""
    PENDING = "pending"
    COMPLETED = "completed"
    FAILED = "failed"
    REFUNDED = "refunded"


# SQLAlchemy Models

class Customer(Base):
    """Customer database model."""
    __tablename__ = "customers"
    
    id = Column(Integer, primary_key=True, index=True)
    email = Column(String, unique=True, index=True, nullable=False)
    hashed_password = Column(String, nullable=False)
    organization_name = Column(String, nullable=False)
    contact_name = Column(String, nullable=False)
    phone = Column(String, nullable=True)
    country = Column(String, nullable=True)
    is_active = Column(Boolean, default=True)
    created_at = Column(DateTime, default=lambda: datetime.now(timezone.utc))
    updated_at = Column(DateTime, default=lambda: datetime.now(timezone.utc), onupdate=lambda: datetime.now(timezone.utc))
    
    # Relationships
    subscriptions = relationship("Subscription", back_populates="customer")
    payments = relationship("Payment", back_populates="customer")


class Subscription(Base):
    """Subscription database model."""
    __tablename__ = "subscriptions"
    
    id = Column(Integer, primary_key=True, index=True)
    customer_id = Column(Integer, ForeignKey("customers.id"), nullable=False)
    tier = Column(SQLEnum(PricingTier), nullable=False)
    status = Column(SQLEnum(SubscriptionStatus), default=SubscriptionStatus.PENDING)
    license_key = Column(String, unique=True, index=True, nullable=False)
    max_nodes = Column(Integer, default=1)
    max_cores = Column(Integer, default=-1)  # -1 = unlimited
    max_storage_tb = Column(Integer, default=-1)  # -1 = unlimited
    price_per_month = Column(Float, nullable=False)
    start_date = Column(DateTime, nullable=True)
    end_date = Column(DateTime, nullable=True)
    created_at = Column(DateTime, default=lambda: datetime.now(timezone.utc))
    updated_at = Column(DateTime, default=lambda: datetime.now(timezone.utc), onupdate=lambda: datetime.now(timezone.utc))
    
    # Relationships
    customer = relationship("Customer", back_populates="subscriptions")
    payments = relationship("Payment", back_populates="subscription")


class Payment(Base):
    """Payment database model."""
    __tablename__ = "payments"
    
    id = Column(Integer, primary_key=True, index=True)
    customer_id = Column(Integer, ForeignKey("customers.id"), nullable=False)
    subscription_id = Column(Integer, ForeignKey("subscriptions.id"), nullable=False)
    amount = Column(Float, nullable=False)
    currency = Column(String, default="EUR")
    status = Column(SQLEnum(PaymentStatus), default=PaymentStatus.PENDING)
    payment_method = Column(String, nullable=True)
    transaction_id = Column(String, unique=True, index=True, nullable=True)
    external_payment_id = Column(String, nullable=True)  # Stripe, bank reference
    created_at = Column(DateTime, default=lambda: datetime.now(timezone.utc))
    updated_at = Column(DateTime, default=lambda: datetime.now(timezone.utc), onupdate=lambda: datetime.now(timezone.utc))
    
    # Relationships
    customer = relationship("Customer", back_populates="payments")
    subscription = relationship("Subscription", back_populates="payments")


# Pydantic Models (API)

class CustomerCreate(BaseModel):
    """Customer creation model."""
    email: EmailStr
    password: str = Field(..., min_length=8)
    organization_name: str = Field(..., min_length=2)
    contact_name: str = Field(..., min_length=2)
    phone: Optional[str] = None
    country: Optional[str] = None


class CustomerResponse(BaseModel):
    """Customer response model."""
    id: int
    email: str
    organization_name: str
    contact_name: str
    phone: Optional[str]
    country: Optional[str]
    is_active: bool
    created_at: datetime
    
    class Config:
        from_attributes = True


class CustomerLogin(BaseModel):
    """Customer login model."""
    email: EmailStr
    password: str


class Token(BaseModel):
    """Authentication token model."""
    access_token: str
    token_type: str = "bearer"


class TokenData(BaseModel):
    """Token data model."""
    email: Optional[str] = None


class SubscriptionCreate(BaseModel):
    """Subscription creation model."""
    tier: PricingTier
    max_nodes: int = 1
    billing_period_months: int = 12


class SubscriptionResponse(BaseModel):
    """Subscription response model."""
    id: int
    customer_id: int
    tier: PricingTier
    status: SubscriptionStatus
    license_key: str
    max_nodes: int
    max_cores: int
    max_storage_tb: int
    price_per_month: float
    start_date: Optional[datetime]
    end_date: Optional[datetime]
    created_at: datetime
    
    class Config:
        from_attributes = True


class PaymentCreate(BaseModel):
    """Payment creation model."""
    subscription_id: int
    amount: float
    currency: str = "EUR"
    payment_method: str


class PaymentResponse(BaseModel):
    """Payment response model."""
    id: int
    customer_id: int
    subscription_id: int
    amount: float
    currency: str
    status: PaymentStatus
    payment_method: Optional[str]
    transaction_id: Optional[str]
    created_at: datetime
    
    class Config:
        from_attributes = True


class PaymentWebhook(BaseModel):
    """Payment webhook notification model."""
    transaction_id: str
    status: PaymentStatus
    amount: float
    currency: str
    external_payment_id: Optional[str] = None
    metadata: Optional[dict] = None


# Telemetry Models

class InstanceTelemetry(Base):
    """Instance telemetry database model."""
    __tablename__ = "instance_telemetry"
    
    id = Column(Integer, primary_key=True, index=True)
    license_key = Column(String, ForeignKey("subscriptions.license_key"), index=True, nullable=False)
    instance_id = Column(String, index=True, nullable=False)  # Unique identifier for the instance
    hostname = Column(String, nullable=True)
    version = Column(String, nullable=False)
    
    # Metrics
    nodes_count = Column(Integer, default=1)
    total_cores = Column(Integer, default=0)
    used_storage_tb = Column(Float, default=0.0)
    uptime_seconds = Column(Integer, default=0)
    query_count_24h = Column(Integer, default=0)
    
    # Geolocation (optional)
    country = Column(String, nullable=True)
    region = Column(String, nullable=True)
    
    # Timestamps
    first_seen = Column(DateTime, default=lambda: datetime.now(timezone.utc))
    last_seen = Column(DateTime, default=lambda: datetime.now(timezone.utc), onupdate=lambda: datetime.now(timezone.utc))
    report_count = Column(Integer, default=1)  # Number of times this instance reported
    
    # User agent and IP (for debugging)
    user_agent = Column(String, nullable=True)
    ip_address = Column(String, nullable=True)


# Pydantic Models for Telemetry API

class TelemetryMetrics(BaseModel):
    """Telemetry metrics model."""
    nodes: int = Field(default=1, ge=1, description="Number of active nodes")
    total_cores: int = Field(default=0, ge=0, description="Total CPU cores")
    used_storage_tb: float = Field(default=0.0, ge=0.0, description="Used storage in TB")
    uptime_seconds: int = Field(default=0, ge=0, description="Instance uptime in seconds")
    query_count_24h: int = Field(default=0, ge=0, description="Query count in last 24h")


class TelemetryHeartbeat(BaseModel):
    """Telemetry heartbeat model from ThemisDB instances."""
    license_key: str = Field(..., description="License key for validation")
    instance_id: str = Field(..., description="Unique instance identifier (UUID recommended)")
    hostname: Optional[str] = Field(None, description="Instance hostname")
    version: str = Field(..., description="ThemisDB version (e.g., '1.5.0')")
    metrics: TelemetryMetrics = Field(..., description="Instance metrics")
    country: Optional[str] = Field(None, description="Country code (e.g., 'DE')")
    region: Optional[str] = Field(None, description="Region (e.g., 'eu-west-1')")


class TelemetryResponse(BaseModel):
    """Telemetry heartbeat response."""
    success: bool
    message: str
    instance_count: Optional[int] = None  # Total instances for this license


class TelemetryStats(BaseModel):
    """Telemetry statistics response."""
    total_instances: int
    active_instances_24h: int
    total_nodes: int
    total_cores: int
    total_storage_tb: float
    versions: dict  # version -> count mapping
    countries: dict  # country -> count mapping
    by_tier: dict  # tier -> instance count mapping
