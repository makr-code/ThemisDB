"""Customer management service."""

from typing import Optional
from sqlalchemy import select
from sqlalchemy.ext.asyncio import AsyncSession

from enterprise_pricing_server.models import Customer, CustomerCreate, CustomerResponse
from enterprise_pricing_server.utils.security import get_password_hash, verify_password


class CustomerService:
    """Service for customer management operations."""
    
    @staticmethod
    async def create_customer(db: AsyncSession, customer_data: CustomerCreate) -> Customer:
        """Create a new customer."""
        # Check if customer already exists
        result = await db.execute(
            select(Customer).where(Customer.email == customer_data.email)
        )
        existing_customer = result.scalar_one_or_none()
        
        if existing_customer:
            raise ValueError("Customer with this email already exists")
        
        # Create new customer
        hashed_password = get_password_hash(customer_data.password)
        db_customer = Customer(
            email=customer_data.email,
            hashed_password=hashed_password,
            organization_name=customer_data.organization_name,
            contact_name=customer_data.contact_name,
            phone=customer_data.phone,
            country=customer_data.country
        )
        
        db.add(db_customer)
        await db.flush()
        await db.refresh(db_customer)
        
        return db_customer
    
    @staticmethod
    async def get_customer_by_id(db: AsyncSession, customer_id: int) -> Optional[Customer]:
        """Get customer by ID."""
        result = await db.execute(
            select(Customer).where(Customer.id == customer_id)
        )
        return result.scalar_one_or_none()
    
    @staticmethod
    async def get_customer_by_email(db: AsyncSession, email: str) -> Optional[Customer]:
        """Get customer by email."""
        result = await db.execute(
            select(Customer).where(Customer.email == email)
        )
        return result.scalar_one_or_none()
    
    @staticmethod
    async def authenticate_customer(db: AsyncSession, email: str, password: str) -> Optional[Customer]:
        """Authenticate a customer."""
        customer = await CustomerService.get_customer_by_email(db, email)
        
        if not customer:
            return None
        
        if not verify_password(password, customer.hashed_password):
            return None
        
        if not customer.is_active:
            return None
        
        return customer
    
    @staticmethod
    async def update_customer(
        db: AsyncSession,
        customer_id: int,
        organization_name: Optional[str] = None,
        contact_name: Optional[str] = None,
        phone: Optional[str] = None,
        country: Optional[str] = None
    ) -> Optional[Customer]:
        """Update customer information."""
        customer = await CustomerService.get_customer_by_id(db, customer_id)
        
        if not customer:
            return None
        
        if organization_name:
            customer.organization_name = organization_name
        if contact_name:
            customer.contact_name = contact_name
        if phone is not None:
            customer.phone = phone
        if country is not None:
            customer.country = country
        
        await db.flush()
        await db.refresh(customer)
        
        return customer
    
    @staticmethod
    async def delete_customer(db: AsyncSession, customer_id: int) -> bool:
        """Delete a customer (soft delete by setting is_active to False)."""
        customer = await CustomerService.get_customer_by_id(db, customer_id)
        
        if not customer:
            return False
        
        customer.is_active = False
        await db.flush()
        
        return True
