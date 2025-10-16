///! Cryptographic operations for Bitcoin wallet
///!
///! Self-contained crypto module with no external wallet-core dependencies.

pub mod brc42;
pub mod brc43;
pub mod keys;
pub mod signing;

// Re-export commonly used items
pub use brc42::{Brc42Error, compute_shared_secret, derive_child_private_key, derive_child_public_key};
pub use brc43::{SecurityLevel, InvoiceNumber, normalize_protocol_id};
pub use keys::{KeyDerivationError, derive_public_key, derive_public_key_uncompressed};
pub use signing::{
    SigningError,
    sign_ecdsa,
    verify_signature,
    sha256,
    double_sha256,
    hmac_sha256,
    verify_hmac_sha256
};
