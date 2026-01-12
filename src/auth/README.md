# Authentication Module

Authentication and authorization implementation for ThemisDB.

## Components

- JWT token validation (`jwt_validator.cpp`)
- GSSAPI/Kerberos authentication (`gssapi_authenticator.cpp`) - NEW
- RBAC (Role-Based Access Control)
- Authentication middleware
- User and role management

## Features

- JWT bearer token authentication
- Kerberos/GSSAPI enterprise SSO - NEW
- Role-based permissions
- API key authentication
- Session management
- Principal-to-role mapping

## Documentation

For authentication documentation, see:
- [Kerberos Authentication](../../docs/en/security/KERBEROS_AUTHENTICATION.md) - NEW
- [JWT Validator](../../docs/src/auth/jwt_validator.cpp.md)
- [JWT Documentation](../../docs/auth/jwt.md)
- [RBAC Authorization](../../docs/rbac_authorization.md)
- [RBAC](../../docs/RBAC.md)
