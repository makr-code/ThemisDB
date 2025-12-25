"""Payment management router."""

from typing import List
from fastapi import APIRouter, Depends, HTTPException, status
from sqlalchemy.ext.asyncio import AsyncSession

from models import (
    PaymentCreate,
    PaymentResponse,
    PaymentWebhook,
    Customer
)
from services import PaymentService
from utils import get_db
from routers.auth import get_current_customer

router = APIRouter(prefix="/payments", tags=["Payments"])

payment_service = PaymentService()


@router.post("", response_model=PaymentResponse, status_code=status.HTTP_201_CREATED)
async def create_payment(
    payment_data: PaymentCreate,
    current_customer: Customer = Depends(get_current_customer),
    db: AsyncSession = Depends(get_db)
):
    """Create a new payment."""
    try:
        payment = await payment_service.create_payment(
            db, current_customer.id, payment_data
        )
        return payment
    except ValueError as e:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail=str(e)
        )


@router.post("/{payment_id}/initiate")
async def initiate_payment(
    payment_id: int,
    current_customer: Customer = Depends(get_current_customer),
    db: AsyncSession = Depends(get_db)
):
    """Initiate payment with banking system."""
    payment = await payment_service.get_payment_by_id(db, payment_id)
    
    if not payment:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="Payment not found"
        )
    
    if payment.customer_id != current_customer.id:
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail="Can only initiate your own payments"
        )
    
    try:
        result = await payment_service.initiate_payment(
            db, payment_id, current_customer.email
        )
        return result
    except ValueError as e:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail=str(e)
        )


@router.get("", response_model=List[PaymentResponse])
async def get_my_payments(
    current_customer: Customer = Depends(get_current_customer),
    db: AsyncSession = Depends(get_db)
):
    """Get all payments for current customer."""
    payments = await payment_service.get_customer_payments(db, current_customer.id)
    return payments


@router.get("/{payment_id}", response_model=PaymentResponse)
async def get_payment(
    payment_id: int,
    current_customer: Customer = Depends(get_current_customer),
    db: AsyncSession = Depends(get_db)
):
    """Get payment by ID."""
    payment = await payment_service.get_payment_by_id(db, payment_id)
    
    if not payment:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="Payment not found"
        )
    
    if payment.customer_id != current_customer.id:
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail="Can only access your own payments"
        )
    
    return payment


@router.post("/{payment_id}/verify", response_model=PaymentResponse)
async def verify_payment(
    payment_id: int,
    current_customer: Customer = Depends(get_current_customer),
    db: AsyncSession = Depends(get_db)
):
    """Manually verify payment with banking system."""
    payment = await payment_service.get_payment_by_id(db, payment_id)
    
    if not payment:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="Payment not found"
        )
    
    if payment.customer_id != current_customer.id:
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail="Can only verify your own payments"
        )
    
    verified_payment = await payment_service.verify_payment(db, payment.transaction_id)
    return verified_payment


@router.post("/webhook", status_code=status.HTTP_200_OK)
async def payment_webhook(
    webhook_data: PaymentWebhook,
    db: AsyncSession = Depends(get_db)
):
    """
    Payment webhook endpoint for banking system notifications.
    
    This endpoint is called by the banking system when payment status changes.
    It automatically activates subscriptions on successful payment.
    """
    payment = await payment_service.handle_webhook(
        db,
        webhook_data.transaction_id,
        webhook_data.status.value,
        webhook_data.external_payment_id
    )
    
    if not payment:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="Payment not found"
        )
    
    return {
        "message": "Webhook processed successfully",
        "payment_id": payment.id,
        "status": payment.status.value
    }
