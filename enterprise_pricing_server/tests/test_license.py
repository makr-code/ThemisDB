"""Tests for license key generation."""

from enterprise_pricing_server.utils.license import generate_license_key, validate_license_key_format


def test_generate_license_key():
    """Test license key generation."""
    key = generate_license_key("test@example.com", "enterprise")
    
    assert key.startswith("THEMIS-ENT-")
    assert len(key.split("-")) == 4
    

def test_validate_license_key_format():
    """Test license key format validation."""
    # Valid keys
    assert validate_license_key_format("THEMIS-ENT-A1B2C3D4-E5F6G7H8") is True
    assert validate_license_key_format("THEMIS-HYP-12345678-ABCDEFGH") is True
    
    # Invalid keys
    assert validate_license_key_format("INVALID-KEY") is False
    assert validate_license_key_format("THEMIS-ENT-SHORT") is False
    assert validate_license_key_format("THEMIS-XXX-12345678-ABCDEFGH") is False
    assert validate_license_key_format("NOTTHEMIS-ENT-12345678-ABCDEFGH") is False
