"""Customer management service with proper OOP and dependency injection."""

from typing import Optional
from sqlalchemy.ext.asyncio import AsyncSession

from models import Customer, CustomerCreate
from repositories.interfaces import ICustomerRepository
from repositories.implementations import CustomerRepository
from utils.security import get_password_hash, verify_password
from exceptions import (
    CustomerAlreadyExistsException,
    CustomerNotFoundException,
    InvalidCredentialsException
)


class CustomerService:
    """Service for customer management operations using OOP principles."""
    
    def __init__(self, db: AsyncSession, repository: Optional[ICustomerRepository] = None):
        """
        Initialize customer service with dependency injection.
        
        Args:
            db: Database session
            repository: Customer repository (injected for testability)
        """
        self._db = db
        self._repository = repository or CustomerRepository(db)
    
    async def create_customer(self, customer_data: CustomerCreate) -> Customer:
        """
        Create a new customer.
        
        Args:
            customer_data: Customer creation data
            
        Returns:
            Created customer
            
        Raises:
            CustomerAlreadyExistsException: If customer with email already exists
        """
        # Check if customer already exists
        if await self._repository.exists_by_email(customer_data.email):
            raise CustomerAlreadyExistsException(customer_data.email)
        
        # Create new customer with hashed password
        hashed_password = get_password_hash(customer_data.password)
        customer = Customer(
            email=customer_data.email,
            hashed_password=hashed_password,
            organization_name=customer_data.organization_name,
            contact_name=customer_data.contact_name,
            phone=customer_data.phone,
            country=customer_data.country
        )
        
        return await self._repository.create(customer)
    
    async def get_customer_by_id(self, customer_id: int) -> Customer:
        """
        Get customer by ID.
        
        Args:
            customer_id: Customer ID
            
        Returns:
            Customer
            
        Raises:
            CustomerNotFoundException: If customer not found
        """
        customer = await self._repository.get_by_id(customer_id)
        if not customer:
            raise CustomerNotFoundException(str(customer_id))
        
        return customer
    
    async def get_customer_by_email(self, email: str) -> Customer:
        """
        Get customer by email.
        
        Args:
            email: Customer email
            
        Returns:
            Customer
            
        Raises:
            CustomerNotFoundException: If customer not found
        """
        customer = await self._repository.get_by_email(email)
        if not customer:
            raise CustomerNotFoundException(email)
        
        return customer
    
    async def authenticate_customer(self, email: str, password: str) -> Customer:
        """
        Authenticate a customer.
        
        Args:
            email: Customer email
            password: Plain text password
            
        Returns:
            Authenticated customer
            
        Raises:
            InvalidCredentialsException: If credentials are invalid
        """
        customer = await self._repository.get_by_email(email)
        
        if not customer:
            raise InvalidCredentialsException()
        
        if not verify_password(password, customer.hashed_password):
            raise InvalidCredentialsException()
        
        if not customer.is_active:
            raise InvalidCredentialsException()
        
        return customer
    
    async def update_customer(
        self,
        customer_id: int,
        organization_name: Optional[str] = None,
        contact_name: Optional[str] = None,
        phone: Optional[str] = None,
        country: Optional[str] = None
    ) -> Customer:
        """
        Update customer information.
        
        Args:
            customer_id: Customer ID
            organization_name: New organization name (optional)
            contact_name: New contact name (optional)
            phone: New phone (optional)
            country: New country (optional)
            
        Returns:
            Updated customer
            
        Raises:
            CustomerNotFoundException: If customer not found
        """
        customer = await self.get_customer_by_id(customer_id)
        
        # Update fields if provided
        if organization_name is not None:
            customer.organization_name = organization_name
        if contact_name is not None:
            customer.contact_name = contact_name
        if phone is not None:
            customer.phone = phone
        if country is not None:
            customer.country = country
        
        return await self._repository.update(customer)
    
    async def delete_customer(self, customer_id: int) -> bool:
        """
        Delete a customer (soft delete).
        
        Args:
            customer_id: Customer ID
            
        Returns:
            True if deleted, False if not found
        """
        return await self._repository.delete(customer_id)
    
    async def is_active(self, customer_id: int) -> bool:
        """
        Check if customer is active.
        
        Args:
            customer_id: Customer ID
            
        Returns:
            True if active, False otherwise
            
        Raises:
            CustomerNotFoundException: If customer not found
        """
        customer = await self.get_customer_by_id(customer_id)
        return customer.is_active
