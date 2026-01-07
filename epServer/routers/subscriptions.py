"""Subscription management router."""

from typing import List
from fastapi import APIRouter, Depends, HTTPException, status
from sqlalchemy.ext.asyncio import AsyncSession

from models import (
    SubscriptionCreate,
    SubscriptionResponse,
    Customer
)
from services import SubscriptionService
from utils import get_db
from routers.auth import get_current_customer

router = APIRouter(prefix="/subscriptions", tags=["Subscriptions"])


@router.post("", response_model=SubscriptionResponse, status_code=status.HTTP_201_CREATED)
async def create_subscription(
    subscription_data: SubscriptionCreate,
    current_customer: Customer = Depends(get_current_customer),
    db: AsyncSession = Depends(get_db)
):
    """Create a new subscription."""
    try:
        subscription = await SubscriptionService.create_subscription(
            db,
            current_customer.id,
            subscription_data,
            current_customer.email
        )
        return subscription
    except ValueError as e:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail=str(e)
        )


@router.get("", response_model=List[SubscriptionResponse])
async def get_my_subscriptions(
    current_customer: Customer = Depends(get_current_customer),
    db: AsyncSession = Depends(get_db)
):
    """Get all subscriptions for current customer."""
    subscriptions = await SubscriptionService.get_customer_subscriptions(
        db, current_customer.id
    )
    return subscriptions


@router.get("/{subscription_id}", response_model=SubscriptionResponse)
async def get_subscription(
    subscription_id: int,
    current_customer: Customer = Depends(get_current_customer),
    db: AsyncSession = Depends(get_db)
):
    """Get subscription by ID."""
    subscription = await SubscriptionService.get_subscription_by_id(db, subscription_id)
    
    if not subscription:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="Subscription not found"
        )
    
    if subscription.customer_id != current_customer.id:
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail="Can only access your own subscriptions"
        )
    
    return subscription


@router.post("/{subscription_id}/cancel", response_model=SubscriptionResponse)
async def cancel_subscription(
    subscription_id: int,
    current_customer: Customer = Depends(get_current_customer),
    db: AsyncSession = Depends(get_db)
):
    """Cancel a subscription."""
    subscription = await SubscriptionService.get_subscription_by_id(db, subscription_id)
    
    if not subscription:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="Subscription not found"
        )
    
    if subscription.customer_id != current_customer.id:
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail="Can only cancel your own subscriptions"
        )
    
    cancelled_subscription = await SubscriptionService.cancel_subscription(db, subscription_id)
    return cancelled_subscription


@router.get("/{subscription_id}/status")
async def check_subscription_status(
    subscription_id: int,
    current_customer: Customer = Depends(get_current_customer),
    db: AsyncSession = Depends(get_db)
):
    """Check subscription status and expiry."""
    subscription = await SubscriptionService.get_subscription_by_id(db, subscription_id)
    
    if not subscription:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="Subscription not found"
        )
    
    if subscription.customer_id != current_customer.id:
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail="Can only check your own subscriptions"
        )
    
    # Check if expired
    is_expired = await SubscriptionService.check_subscription_expiry(db, subscription_id)
    
    return {
        "subscription_id": subscription.id,
        "status": subscription.status.value,
        "tier": subscription.tier.value,
        "is_expired": is_expired,
        "start_date": subscription.start_date,
        "end_date": subscription.end_date
    }
