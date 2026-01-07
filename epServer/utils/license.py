"""License key generation utilities."""

import secrets
import hashlib
from datetime import datetime, timezone


def generate_license_key(customer_email: str, tier: str) -> str:
    """Generate a unique license key for a subscription.
    
    Format: THEMIS-{TIER}-{HASH}-{RANDOM}
    Example: THEMIS-ENT-A1B2C3D4-E5F6G7H8
    """
    # Create a hash of customer email + tier + timestamp
    timestamp = datetime.now(timezone.utc).isoformat()
    data = f"{customer_email}:{tier}:{timestamp}"
    hash_digest = hashlib.sha256(data.encode()).hexdigest()[:8].upper()
    
    # Generate random component
    random_part = secrets.token_hex(4).upper()
    
    # Map tier to short code
    tier_codes = {
        "community": "COM",
        "enterprise": "ENT",
        "hyperscaler": "HYP",
        "reseller": "RES"
    }
    tier_code = tier_codes.get(tier.lower(), "UNK")
    
    return f"THEMIS-{tier_code}-{hash_digest}-{random_part}"


def validate_license_key_format(license_key: str) -> bool:
    """Validate license key format."""
    if not license_key.startswith("THEMIS-"):
        return False
    
    parts = license_key.split("-")
    if len(parts) != 4:
        return False
    
    # Check tier code
    valid_tiers = ["COM", "ENT", "HYP", "RES"]
    if parts[1] not in valid_tiers:
        return False
    
    # Check hash length (8 chars)
    if len(parts[2]) != 8:
        return False
    
    # Check random part length (8 chars)
    if len(parts[3]) != 8:
        return False
    
    return True
