"""Customer management router."""

from typing import Optional
from fastapi import APIRouter, Depends, HTTPException, status
from sqlalchemy.ext.asyncio import AsyncSession
from pydantic import BaseModel

from models import CustomerResponse, Customer
from services import CustomerService
from utils import get_db
from routers.auth import get_current_customer

router = APIRouter(prefix="/customers", tags=["Customers"])


class CustomerUpdate(BaseModel):
    """Customer update model."""
    organization_name: Optional[str] = None
    contact_name: Optional[str] = None
    phone: Optional[str] = None
    country: Optional[str] = None


@router.get("/me", response_model=CustomerResponse)
async def get_my_profile(
    current_customer: Customer = Depends(get_current_customer)
):
    """Get current customer profile."""
    return current_customer


@router.get("/{customer_id}", response_model=CustomerResponse)
async def get_customer(
    customer_id: int,
    current_customer: Customer = Depends(get_current_customer),
    db: AsyncSession = Depends(get_db)
):
    """Get customer by ID (only own profile)."""
    if customer_id != current_customer.id:
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail="Can only access your own profile"
        )
    
    customer = await CustomerService.get_customer_by_id(db, customer_id)
    if not customer:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="Customer not found"
        )
    
    return customer


@router.put("/{customer_id}", response_model=CustomerResponse)
async def update_customer(
    customer_id: int,
    update_data: CustomerUpdate,
    current_customer: Customer = Depends(get_current_customer),
    db: AsyncSession = Depends(get_db)
):
    """Update customer information."""
    if customer_id != current_customer.id:
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail="Can only update your own profile"
        )
    
    customer = await CustomerService.update_customer(
        db,
        customer_id,
        organization_name=update_data.organization_name,
        contact_name=update_data.contact_name,
        phone=update_data.phone,
        country=update_data.country
    )
    
    if not customer:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="Customer not found"
        )
    
    return customer


@router.delete("/{customer_id}", status_code=status.HTTP_204_NO_CONTENT)
async def delete_customer(
    customer_id: int,
    current_customer: Customer = Depends(get_current_customer),
    db: AsyncSession = Depends(get_db)
):
    """Delete customer account (soft delete)."""
    if customer_id != current_customer.id:
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail="Can only delete your own account"
        )
    
    success = await CustomerService.delete_customer(db, customer_id)
    
    if not success:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="Customer not found"
        )
    
    return None
