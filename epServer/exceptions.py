"""Custom exceptions for the pricing server."""


class PricingServerException(Exception):
    """Base exception for all pricing server errors."""
    
    def __init__(self, message: str, code: str = "INTERNAL_ERROR"):
        self.message = message
        self.code = code
        super().__init__(self.message)


class CustomerException(PricingServerException):
    """Base exception for customer-related errors."""
    pass


class CustomerAlreadyExistsException(CustomerException):
    """Raised when trying to create a customer that already exists."""
    
    def __init__(self, email: str):
        super().__init__(
            message=f"Customer with email '{email}' already exists",
            code="CUSTOMER_ALREADY_EXISTS"
        )
        self.email = email


class CustomerNotFoundException(CustomerException):
    """Raised when a customer is not found."""
    
    def __init__(self, identifier: str):
        super().__init__(
            message=f"Customer '{identifier}' not found",
            code="CUSTOMER_NOT_FOUND"
        )
        self.identifier = identifier


class InvalidCredentialsException(CustomerException):
    """Raised when login credentials are invalid."""
    
    def __init__(self):
        super().__init__(
            message="Invalid email or password",
            code="INVALID_CREDENTIALS"
        )


class SubscriptionException(PricingServerException):
    """Base exception for subscription-related errors."""
    pass


class SubscriptionNotFoundException(SubscriptionException):
    """Raised when a subscription is not found."""
    
    def __init__(self, subscription_id: int):
        super().__init__(
            message=f"Subscription {subscription_id} not found",
            code="SUBSCRIPTION_NOT_FOUND"
        )
        self.subscription_id = subscription_id


class InvalidSubscriptionStateException(SubscriptionException):
    """Raised when subscription operation is not valid for current state."""
    
    def __init__(self, current_state: str, operation: str):
        super().__init__(
            message=f"Cannot {operation} subscription in {current_state} state",
            code="INVALID_SUBSCRIPTION_STATE"
        )
        self.current_state = current_state
        self.operation = operation


class PaymentException(PricingServerException):
    """Base exception for payment-related errors."""
    pass


class PaymentNotFoundException(PaymentException):
    """Raised when a payment is not found."""
    
    def __init__(self, payment_id: int):
        super().__init__(
            message=f"Payment {payment_id} not found",
            code="PAYMENT_NOT_FOUND"
        )
        self.payment_id = payment_id


class PaymentVerificationException(PaymentException):
    """Raised when payment verification fails."""
    
    def __init__(self, transaction_id: str, reason: str):
        super().__init__(
            message=f"Payment verification failed for transaction {transaction_id}: {reason}",
            code="PAYMENT_VERIFICATION_FAILED"
        )
        self.transaction_id = transaction_id
        self.reason = reason


class BankingInterfaceException(PaymentException):
    """Raised when banking interface encounters an error."""
    
    def __init__(self, operation: str, details: str):
        super().__init__(
            message=f"Banking interface error during {operation}: {details}",
            code="BANKING_INTERFACE_ERROR"
        )
        self.operation = operation
        self.details = details


class LicenseException(PricingServerException):
    """Base exception for license-related errors."""
    pass


class InvalidLicenseFormatException(LicenseException):
    """Raised when license key format is invalid."""
    
    def __init__(self, license_key: str):
        super().__init__(
            message=f"Invalid license key format: {license_key}",
            code="INVALID_LICENSE_FORMAT"
        )
        self.license_key = license_key


class LicenseNotFoundException(LicenseException):
    """Raised when license is not found."""
    
    def __init__(self, license_key: str):
        super().__init__(
            message=f"License key '{license_key}' not found",
            code="LICENSE_NOT_FOUND"
        )
        self.license_key = license_key


class LicenseExpiredException(LicenseException):
    """Raised when license has expired."""
    
    def __init__(self, license_key: str, expiry_date: str):
        super().__init__(
            message=f"License '{license_key}' expired on {expiry_date}",
            code="LICENSE_EXPIRED"
        )
        self.license_key = license_key
        self.expiry_date = expiry_date


class LicenseSuspendedException(LicenseException):
    """Raised when license is suspended."""
    
    def __init__(self, license_key: str):
        super().__init__(
            message=f"License '{license_key}' has been suspended",
            code="LICENSE_SUSPENDED"
        )
        self.license_key = license_key


class ResourceLimitExceededException(LicenseException):
    """Raised when resource usage exceeds license limits."""
    
    def __init__(self, resource: str, current: int, limit: int):
        super().__init__(
            message=f"{resource} limit exceeded: {current} > {limit}",
            code="RESOURCE_LIMIT_EXCEEDED"
        )
        self.resource = resource
        self.current = current
        self.limit = limit
