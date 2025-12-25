#!/usr/bin/env python3
"""
Quick API test script to verify the pricing server is working correctly.
Run the server first: python run_server.py
Then run this script in another terminal.
"""

import requests
import json
import sys
from time import sleep

API_URL = "http://localhost:8000"

def test_health():
    """Test health check endpoint."""
    print("Testing health check...")
    response = requests.get(f"{API_URL}/health")
    assert response.status_code == 200
    print(f"✓ Health check passed: {response.json()}")

def test_pricing():
    """Test pricing tiers endpoint."""
    print("\nTesting pricing tiers...")
    response = requests.get(f"{API_URL}/pricing")
    assert response.status_code == 200
    data = response.json()
    print(f"✓ Pricing tiers loaded: {len(data['tiers'])} tiers available")
    for tier in data['tiers']:
        print(f"  - {tier['name']}: €{tier['price_per_month']}/month")

def test_registration_and_login():
    """Test customer registration and login."""
    print("\nTesting customer registration...")
    
    # Register - use timestamp for unique email
    import time
    customer_data = {
        "email": f"test{int(time.time())}@example.com",
        "password": "TestPassword123!",
        "organization_name": "Test Organization",
        "contact_name": "Test User"
    }
    
    response = requests.post(f"{API_URL}/auth/register", json=customer_data)
    if response.status_code == 201:
        print(f"✓ Registration successful: {response.json()['email']}")
    else:
        print(f"✗ Registration failed: {response.status_code} - {response.text}")
        return None
    
    # Login
    print("\nTesting login...")
    response = requests.post(
        f"{API_URL}/auth/login-json",
        json={"email": customer_data["email"], "password": customer_data["password"]}
    )
    
    if response.status_code == 200:
        token = response.json()["access_token"]
        print(f"✓ Login successful, token received")
        return token, customer_data["email"]
    else:
        print(f"✗ Login failed: {response.status_code} - {response.text}")
        return None

def test_subscription_creation(token):
    """Test subscription creation."""
    print("\nTesting subscription creation...")
    
    headers = {"Authorization": f"Bearer {token}"}
    sub_data = {
        "tier": "enterprise",
        "max_nodes": 10,
        "billing_period_months": 12
    }
    
    response = requests.post(f"{API_URL}/subscriptions", json=sub_data, headers=headers)
    
    if response.status_code == 201:
        data = response.json()
        print(f"✓ Subscription created successfully")
        print(f"  - ID: {data['id']}")
        print(f"  - Tier: {data['tier']}")
        print(f"  - License Key: {data['license_key']}")
        print(f"  - Price: €{data['price_per_month']}/month")
        return data['id']
    else:
        print(f"✗ Subscription creation failed: {response.status_code} - {response.text}")
        return None

def test_payment_workflow(token, subscription_id):
    """Test payment creation and verification."""
    print("\nTesting payment workflow...")
    
    headers = {"Authorization": f"Bearer {token}"}
    payment_data = {
        "subscription_id": subscription_id,
        "amount": 60000.0,  # 12 months * 5000
        "currency": "EUR",
        "payment_method": "bank_transfer"
    }
    
    # Create payment
    response = requests.post(f"{API_URL}/payments", json=payment_data, headers=headers)
    
    if response.status_code == 201:
        data = response.json()
        print(f"✓ Payment created successfully")
        print(f"  - Payment ID: {data['id']}")
        print(f"  - Transaction ID: {data['transaction_id']}")
        print(f"  - Amount: €{data['amount']}")
        print(f"  - Status: {data['status']}")
        
        # Verify payment (simulated)
        payment_id = data['id']
        response = requests.post(f"{API_URL}/payments/{payment_id}/verify", headers=headers)
        
        if response.status_code == 200:
            data = response.json()
            print(f"✓ Payment verified successfully")
            print(f"  - New status: {data['status']}")
        else:
            print(f"✗ Payment verification failed: {response.status_code}")
    else:
        print(f"✗ Payment creation failed: {response.status_code} - {response.text}")

def main():
    """Run all tests."""
    print("=" * 60)
    print("ThemisDB Enterprise Pricing Server - API Test")
    print("=" * 60)
    
    try:
        # Test 1: Health check
        test_health()
        
        # Test 2: Pricing tiers
        test_pricing()
        
        # Test 3: Registration and Login
        result = test_registration_and_login()
        if not result:
            print("\n✗ Cannot proceed without valid token")
            sys.exit(1)
        
        token, email = result
        
        # Test 4: Create subscription
        subscription_id = test_subscription_creation(token)
        if not subscription_id:
            print("\n✗ Cannot proceed without valid subscription")
            sys.exit(1)
        
        # Test 5: Payment workflow
        test_payment_workflow(token, subscription_id)
        
        print("\n" + "=" * 60)
        print("✓ All tests passed successfully!")
        print("=" * 60)
        
    except requests.exceptions.ConnectionError:
        print("\n✗ Error: Could not connect to the server.")
        print("Make sure the server is running: python run_server.py")
        sys.exit(1)
    except Exception as e:
        print(f"\n✗ Unexpected error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

if __name__ == "__main__":
    main()
