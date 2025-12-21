"""Main FastAPI application for ThemisDB Enterprise Pricing Server."""

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from contextlib import asynccontextmanager

from enterprise_pricing_server.config import settings
from enterprise_pricing_server.utils.database import init_db
from enterprise_pricing_server.routers import auth, customers, subscriptions, payments


@asynccontextmanager
async def lifespan(app: FastAPI):
    """Application lifespan manager."""
    # Startup
    await init_db()
    yield
    # Shutdown
    pass


# Create FastAPI app
app = FastAPI(
    title=settings.app_name,
    version=settings.version,
    description="""
    ThemisDB Enterprise Pricing Server
    
    A comprehensive subscription and payment management system for ThemisDB
    Enterprise and Hyperscaler editions.
    
    Features:
    - Customer registration and authentication (CRUD)
    - Subscription management (Enterprise, Hyperscaler, Reseller tiers)
    - Payment verification with banking interface
    - Automatic subscription activation on payment
    - Webhook support for payment notifications
    """,
    lifespan=lifespan,
    docs_url="/docs",
    redoc_url="/redoc"
)

# CORS middleware
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],  # Configure appropriately in production
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Include routers
app.include_router(auth.router)
app.include_router(customers.router)
app.include_router(subscriptions.router)
app.include_router(payments.router)


@app.get("/")
async def root():
    """Root endpoint."""
    return {
        "name": settings.app_name,
        "version": settings.version,
        "status": "operational",
        "docs": "/docs",
        "redoc": "/redoc"
    }


@app.get("/health")
async def health_check():
    """Health check endpoint."""
    return {
        "status": "healthy",
        "version": settings.version
    }


@app.get("/pricing")
async def get_pricing_tiers():
    """Get pricing information for all tiers."""
    return {
        "tiers": [
            {
                "name": "Community",
                "price_per_month": settings.community_price,
                "currency": "EUR",
                "features": [
                    "Single-node deployment",
                    "Up to 8 worker threads",
                    "Single GPU support",
                    "All core database features"
                ],
                "limits": {
                    "max_nodes": 1,
                    "max_cores": 8,
                    "max_storage_tb": 1
                }
            },
            {
                "name": "Enterprise",
                "price_per_month": settings.enterprise_price,
                "currency": "EUR",
                "features": [
                    "Horizontal scaling (up to 100 nodes)",
                    "Advanced analytics (OLAP/CEP)",
                    "High availability & replication",
                    "Advanced security (HSM, RBAC)",
                    "Multi-GPU support",
                    "Priority support"
                ],
                "limits": {
                    "max_nodes": 100,
                    "max_cores": -1,
                    "max_storage_tb": -1
                }
            },
            {
                "name": "Hyperscaler",
                "price_per_month": settings.hyperscaler_price,
                "currency": "EUR",
                "features": [
                    "Unlimited nodes",
                    "All Enterprise features",
                    "Kubernetes operator",
                    "Multi-datacenter deployment",
                    "Dedicated support",
                    "Custom SLA"
                ],
                "limits": {
                    "max_nodes": -1,
                    "max_cores": -1,
                    "max_storage_tb": -1
                }
            },
            {
                "name": "Reseller",
                "price_per_month": settings.reseller_price,
                "currency": "EUR",
                "features": [
                    "Embed in commercial applications",
                    "Flexible node limits",
                    "White-label options",
                    "Reseller support"
                ],
                "limits": {
                    "max_nodes": "configurable",
                    "max_cores": -1,
                    "max_storage_tb": -1
                }
            }
        ]
    }


if __name__ == "__main__":
    import uvicorn
    uvicorn.run(
        "app:app",
        host=settings.host,
        port=settings.port,
        reload=settings.debug
    )
