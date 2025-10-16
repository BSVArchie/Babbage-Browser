use actix_web::{web, HttpResponse};
use serde::{Deserialize, Serialize};
use crate::AppState;
use crate::crypto::brc42::{derive_child_private_key, derive_child_public_key};
use crate::crypto::brc43::{InvoiceNumber, SecurityLevel, normalize_protocol_id};
use crate::crypto::signing::{sign_ecdsa, sha256, hmac_sha256, verify_hmac_sha256};
use rand::Rng;

// Health check
pub async fn health() -> HttpResponse {
    HttpResponse::Ok().json(serde_json::json!({
        "status": "ok",
        "version": "0.1.0-rust",
        "backend": "Rust wallet-toolbox-rs"
    }))
}

// BRC-100 status check
pub async fn brc100_status() -> HttpResponse {
    log::info!("📋 /brc100/status called");
    HttpResponse::Ok().json(serde_json::json!({
        "available": true,          // For CEF isAvailable() check
        "service": "BRC-100",
        "version": "1.0.0",
        "status": "operational",
        "timestamp": chrono::Utc::now().to_rfc3339(),
        "components": {
            "identity": "operational",
            "authentication": "operational",
            "session": "operational",
            "beef": "operational",
            "spv": "operational"
        }
    }))
}

// /getVersion - BRC-100 endpoint
pub async fn get_version() -> HttpResponse {
    log::info!("📋 /getVersion called");
    HttpResponse::Ok().json(serde_json::json!({
        "version": "BitcoinBrowserWallet-Rust v0.0.1",
        "capabilities": [
            "getVersion",
            "getPublicKey",
            "createSignature",
            "isAuthenticated",
            "createAction",
            "signAction",
            "processAction"
        ],
        "brc100": true,
        "timestamp": chrono::Utc::now()
    }))
}

// /getPublicKey - BRC-100 endpoint
pub async fn get_public_key(state: web::Data<AppState>) -> HttpResponse {
    log::info!("📋 /getPublicKey called");
    let storage = state.storage.lock().unwrap();

    match storage.get_current_address() {
        Ok(addr) => {
            log::info!("   Returning public key for address: {}", addr.address);
            HttpResponse::Ok().json(serde_json::json!({
                "publicKey": addr.public_key,
                "address": addr.address,
                "index": addr.index
            }))
        }
        Err(e) => {
            log::error!("   Failed to get address: {}", e);
            HttpResponse::InternalServerError().json(serde_json::json!({
                "error": e
            }))
        }
    }
}

// /isAuthenticated - BRC-100 endpoint
pub async fn is_authenticated() -> HttpResponse {
    log::info!("📋 /isAuthenticated called");
    HttpResponse::Ok().json(serde_json::json!({
        "authenticated": true
    }))
}

// Authentication request structure
#[derive(Debug, Deserialize)]
pub struct AuthRequest {
    pub version: String,
    #[serde(rename = "messageType")]
    pub message_type: String,
    #[serde(rename = "identityKey")]
    pub identity_key: String,
    #[serde(rename = "initialNonce")]
    pub initial_nonce: String,
}

// /.well-known/auth - Babbage authentication
pub async fn well_known_auth(
    state: web::Data<AppState>,
    req: web::Json<AuthRequest>,
) -> HttpResponse {
    log::info!("🔐 Babbage auth request received");
    log::info!("   Identity key: {}", req.identity_key);
    log::info!("   Initial nonce: {}", req.initial_nonce);
    log::info!("   Message type: {}", req.message_type);

    // Get current wallet address
    let storage = state.storage.lock().unwrap();
    let addr = match storage.get_current_address() {
        Ok(a) => a.clone(),
        Err(e) => {
            log::error!("   Failed to get address: {}", e);
            return HttpResponse::InternalServerError().body(e);
        }
    };
    drop(storage);

    log::info!("   Our identity key: {}", addr.public_key);

    // Generate our nonce (per @bsv/sdk createNonce implementation)
    // 1. Generate 16 random bytes for first half
    let mut first_half = [0u8; 16];
    rand::thread_rng().fill(&mut first_half);

    // 2. Create HMAC of the first half (this will be 64 bytes)
    // Re-derive private key for HMAC
    let storage = state.storage.lock().unwrap();
    let hmac_privkey = match storage.derive_private_key(0) {
        Ok(key) => key,
        Err(e) => {
            log::error!("   Failed to derive key for nonce HMAC: {}", e);
            drop(storage);
            return HttpResponse::InternalServerError().json(serde_json::json!({
                "error": format!("Key derivation error: {}", e)
            }));
        }
    };
    drop(storage);

    // Create HMAC using protocolID [2, "server hmac"] and keyID = UTF-8(firstHalf)
    use hmac::{Hmac, Mac};
    use sha2::Sha256;
    type HmacSha256 = Hmac<Sha256>;

    let key_id_str = String::from_utf8_lossy(&first_half);
    let invoice_for_nonce = format!("2-server hmac-{}", key_id_str);

    let mut mac = HmacSha256::new_from_slice(&hmac_privkey)
        .expect("HMAC can take key of any size");
    mac.update(&first_half);
    let hmac_result = mac.finalize().into_bytes();

    // 3. Concatenate: firstHalf (16 bytes) + hmac (32 bytes) = 48 bytes total
    let mut nonce_bytes = Vec::new();
    nonce_bytes.extend_from_slice(&first_half);
    nonce_bytes.extend_from_slice(&hmac_result);

    let our_nonce = base64::encode(&nonce_bytes);
    log::info!("   Generated our nonce ({} bytes: 16 random + 32 HMAC-SHA256): {}", nonce_bytes.len(), our_nonce);

    // Decode each nonce separately, then concatenate the bytes (per @bsv/sdk Utils.toArray)
    let their_nonce_bytes = match base64::decode(&req.initial_nonce) {
        Ok(bytes) => bytes,
        Err(e) => {
            log::error!("   Failed to decode their nonce: {}", e);
            return HttpResponse::BadRequest().json(serde_json::json!({
                "error": format!("Invalid initial nonce encoding: {}", e)
            }));
        }
    };

    let our_nonce_bytes_decoded = match base64::decode(&our_nonce) {
        Ok(bytes) => bytes,
        Err(e) => {
            log::error!("   Failed to decode our nonce: {}", e);
            return HttpResponse::InternalServerError().json(serde_json::json!({
                "error": format!("Invalid nonce generation: {}", e)
            }));
        }
    };

    // Concatenate the DECODED bytes
    let mut data_to_sign = Vec::new();
    data_to_sign.extend_from_slice(&their_nonce_bytes);
    data_to_sign.extend_from_slice(&our_nonce_bytes_decoded);

    log::info!("   Data to sign ({} bytes: {} + {} decoded bytes)",
        data_to_sign.len(), their_nonce_bytes.len(), our_nonce_bytes_decoded.len());

    // Create BRC-43 invoice number: "2-auth message signature-theirNonce ourNonce"
    let protocol_id = match normalize_protocol_id("auth message signature") {
        Ok(p) => p,
        Err(e) => {
            log::error!("   Failed to normalize protocol ID: {}", e);
            return HttpResponse::InternalServerError().json(serde_json::json!({
                "error": format!("Protocol ID error: {}", e)
            }));
        }
    };

    let key_id = format!("{} {}", req.initial_nonce, our_nonce);
    let invoice_number = match InvoiceNumber::new(
        SecurityLevel::CounterpartyLevel,
        protocol_id,
        key_id
    ) {
        Ok(inv) => inv.to_string(),
        Err(e) => {
            log::error!("   Failed to create invoice number: {}", e);
            return HttpResponse::InternalServerError().json(serde_json::json!({
                "error": format!("Invoice number error: {}", e)
            }));
        }
    };

    log::info!("   BRC-43 invoice number: {}", invoice_number);

    // Derive private key from mnemonic (for address index 0)
    let storage = state.storage.lock().unwrap();
    let private_key_bytes = match storage.derive_private_key(0) {
        Ok(key) => {
            log::info!("   ✅ Private key derived from mnemonic");
            key
        },
        Err(e) => {
            log::error!("   Failed to derive private key: {}", e);
            drop(storage);
            return HttpResponse::InternalServerError().json(serde_json::json!({
                "error": format!("Key derivation error: {}", e)
            }));
        }
    };
    drop(storage);

    // Decode counterparty public key (identity key)
    let counterparty_pubkey_bytes = match hex::decode(&req.identity_key) {
        Ok(b) => b,
        Err(e) => {
            log::error!("   Failed to decode counterparty public key: {}", e);
            return HttpResponse::BadRequest().json(serde_json::json!({
                "error": "Invalid identity key"
            }));
        }
    };

    log::info!("   Deriving BRC-42 child private key...");

    // Derive child private key using BRC-42
    let child_private_key = match derive_child_private_key(
        &private_key_bytes,
        &counterparty_pubkey_bytes,
        &invoice_number
    ) {
        Ok(key) => key,
        Err(e) => {
            log::error!("   BRC-42 derivation failed: {}", e);
            return HttpResponse::InternalServerError().json(serde_json::json!({
                "error": format!("Key derivation error: {}", e)
            }));
        }
    };

    log::info!("   ✅ BRC-42 child key derived successfully");

    // Hash the data (SHA-256)
    let data_hash = sha256(&data_to_sign);
    log::info!("   Data hash (32 bytes): {}", hex::encode(&data_hash));

    // Sign the hash with the derived key
    // Try COMPACT format (R + S, 64 bytes) instead of DER
    use secp256k1::{Secp256k1, SecretKey as Secp256k1SecretKey, Message};

    let secp = Secp256k1::new();
    let secret = Secp256k1SecretKey::from_slice(&child_private_key)
        .map_err(|e| {
            log::error!("   Invalid private key: {}", e);
            e
        }).unwrap();

    let message = Message::from_slice(&data_hash)
        .map_err(|e| {
            log::error!("   Invalid message hash: {}", e);
            e
        }).unwrap();

    let signature = secp.sign_ecdsa(&message, &secret);

    // Extract R and S as 32-byte values (compact format)
    let sig_bytes = signature.serialize_compact();  // Returns [u8; 64]

    let signature_hex = hex::encode(&sig_bytes);
    log::info!("   ✅ Signature created ({} bytes, COMPACT R+S): {}", sig_bytes.len(), signature_hex);

    // Return BRC-104 compliant response
    // Per BRC-103 spec section 6.1:
    // - "initialNonce": B_Nonce (our new session nonce)
    // - "yourNonce": A_Nonce (their initial nonce echoed back)
    let response = serde_json::json!({
        "version": "0.1",
        "messageType": "initialResponse",
        "identityKey": addr.public_key,
        "initialNonce": our_nonce,             // Our new nonce (B_Nonce)
        "yourNonce": req.initial_nonce,        // Their initial nonce echoed back (A_Nonce)
        "signature": signature_hex
    });

    log::info!("✅ Returning auth response with BRC-42 signature");
    log::info!("   📤 Response fields: initialNonce=[ourNew], yourNonce=[theirInitialEchoed]");
    log::info!("   📤 FULL RESPONSE JSON: {}", serde_json::to_string_pretty(&response).unwrap_or_else(|_| "error".to_string()));

    HttpResponse::Ok().json(response)
}

// Request structure for /createHmac
#[derive(Debug, Deserialize)]
pub struct CreateHmacRequest {
    #[serde(rename = "protocolID")]
    pub protocol_id: serde_json::Value, // Can be [number, string] or string
    #[serde(rename = "keyID")]
    pub key_id: String,
    pub data: serde_json::Value, // Can be array of bytes OR base64 string
    #[serde(rename = "counterparty")]
    pub counterparty: serde_json::Value, // Can be "self" or hex public key
}

// Response structure for /createHmac
#[derive(Debug, Serialize)]
pub struct CreateHmacResponse {
    pub hmac: Vec<u8>, // Array of bytes (not hex string)
}

// /createHmac - BRC-100 endpoint for creating HMAC
pub async fn create_hmac(
    state: web::Data<AppState>,
    body: web::Bytes,
) -> HttpResponse {
    log::info!("📋 /createHmac called");
    log::info!("   Raw body: {}", String::from_utf8_lossy(&body));

    // Try to parse JSON
    let req: CreateHmacRequest = match serde_json::from_slice(&body) {
        Ok(r) => r,
        Err(e) => {
            log::error!("   JSON parse error: {}", e);
            return HttpResponse::BadRequest().json(serde_json::json!({
                "error": format!("Invalid JSON: {}", e)
            }));
        }
    };

    log::info!("   Protocol ID: {:?}", req.protocol_id);
    log::info!("   Key ID: {}", req.key_id);
    log::info!("   Counterparty: {:?}", req.counterparty);
    log::info!("   Data: {:?}", req.data);

    // Parse data (can be array of bytes or base64 string)
    let data_bytes: Vec<u8> = match &req.data {
        serde_json::Value::Array(arr) => {
            // Array of numbers
            arr.iter()
                .filter_map(|v| v.as_u64().map(|n| n as u8))
                .collect()
        },
        serde_json::Value::String(s) => {
            // Base64 string
            match base64::decode(s) {
                Ok(b) => b,
                Err(e) => {
                    log::error!("   Failed to decode base64 data: {}", e);
                    return HttpResponse::BadRequest().json(serde_json::json!({
                        "error": format!("Invalid base64 data: {}", e)
                    }));
                }
            }
        },
        _ => {
            log::error!("   Data must be array or base64 string");
            return HttpResponse::BadRequest().json(serde_json::json!({
                "error": "Data must be array or base64 string"
            }));
        }
    };

    log::info!("   Data bytes length: {}", data_bytes.len());

    // Parse counterparty (can be "self" or hex public key)
    let counterparty_hex: Option<String> = match &req.counterparty {
        serde_json::Value::String(s) if s == "self" => {
            log::info!("   No counterparty (self)");
            None
        },
        serde_json::Value::String(s) => {
            log::info!("   Counterparty public key: {}", s);
            Some(s.clone())
        },
        _ => None
    };

    // Parse protocol ID (can be [level, "name"] or just "name")
    let protocol_id_str = match &req.protocol_id {
        serde_json::Value::Array(arr) => {
            if arr.len() >= 2 {
                if let Some(name) = arr[1].as_str() {
                    name.to_string()
                } else {
                    log::error!("   Invalid protocol ID array format");
                    return HttpResponse::BadRequest().json(serde_json::json!({
                        "error": "Invalid protocol ID format"
                    }));
                }
            } else {
                log::error!("   Protocol ID array too short");
                return HttpResponse::BadRequest().json(serde_json::json!({
                    "error": "Protocol ID array too short"
                }));
            }
        },
        serde_json::Value::String(s) => s.clone(),
        _ => {
            log::error!("   Protocol ID must be array or string");
            return HttpResponse::BadRequest().json(serde_json::json!({
                "error": "Protocol ID must be array or string"
            }));
        }
    };

    // Normalize protocol ID
    let protocol_id = match normalize_protocol_id(&protocol_id_str) {
        Ok(p) => p,
        Err(e) => {
            log::error!("   Failed to normalize protocol ID: {}", e);
            return HttpResponse::BadRequest().json(serde_json::json!({
                "error": format!("Invalid protocol ID: {}", e)
            }));
        }
    };


    // Create BRC-43 invoice number
    let security_level = if counterparty_hex.is_some() {
        SecurityLevel::CounterpartyLevel
    } else {
        SecurityLevel::CounterpartyLevel // Default to level 2
    };

    let invoice_number = match InvoiceNumber::new(
        security_level,
        protocol_id,
        req.key_id.clone()
    ) {
        Ok(inv) => inv.to_string(),
        Err(e) => {
            log::error!("   Failed to create invoice number: {}", e);
            return HttpResponse::InternalServerError().json(serde_json::json!({
                "error": format!("Invoice number error: {}", e)
            }));
        }
    };

    log::info!("   BRC-43 invoice number: {}", invoice_number);

    // Derive private key from mnemonic (for address index 0)
    let storage = state.storage.lock().unwrap();
    let private_key_bytes = match storage.derive_private_key(0) {
        Ok(key) => {
            log::info!("   ✅ Private key derived from mnemonic (createHmac)");
            key
        },
        Err(e) => {
            log::error!("   Failed to derive private key: {}", e);
            drop(storage);
            return HttpResponse::InternalServerError().json(serde_json::json!({
                "error": format!("Key derivation error: {}", e)
            }));
        }
    };
    drop(storage);

    // Determine HMAC key based on whether counterparty is provided
    let hmac_key = if let Some(counterparty_hex) = &counterparty_hex {
        // BRC-42: Derive child key for mutual authentication
        let counterparty_bytes = match hex::decode(counterparty_hex) {
            Ok(b) => b,
            Err(e) => {
                log::error!("   Failed to decode counterparty key: {}", e);
                return HttpResponse::BadRequest().json(serde_json::json!({
                    "error": "Invalid counterparty key"
                }));
            }
        };

        log::info!("   Deriving BRC-42 child key for HMAC...");
        match derive_child_private_key(&private_key_bytes, &counterparty_bytes, &invoice_number) {
            Ok(key) => {
                log::info!("   ✅ BRC-42 child key derived");
                key
            },
            Err(e) => {
                log::error!("   BRC-42 derivation failed: {}", e);
                return HttpResponse::InternalServerError().json(serde_json::json!({
                    "error": format!("Key derivation error: {}", e)
                }));
            }
        }
    } else {
        // Simple HMAC with wallet's private key (no BRC-42 derivation)
        log::info!("   Using wallet private key for HMAC (no counterparty)");
        private_key_bytes
    };

    // Compute HMAC-SHA256
    let hmac_result = hmac_sha256(&hmac_key, &data_bytes);
    let hmac_hex = hex::encode(&hmac_result);

    log::info!("   ✅ HMAC created: {}...", &hmac_hex[..std::cmp::min(32, hmac_hex.len())]);

    // Return HMAC as array of bytes (not hex string) for SDK compatibility
    HttpResponse::Ok().json(CreateHmacResponse { hmac: hmac_result.to_vec() })
}

// Request structure for /verifyHmac
#[derive(Debug, Deserialize)]
pub struct VerifyHmacRequest {
    #[serde(rename = "protocolID")]
    pub protocol_id: serde_json::Value, // Can be [number, string] or string
    #[serde(rename = "keyID")]
    pub key_id: String,
    pub data: serde_json::Value, // Can be array of bytes OR base64 string
    pub hmac: serde_json::Value, // Can be array of bytes OR hex string
    #[serde(rename = "counterparty")]
    pub counterparty: serde_json::Value, // Can be "self" or hex public key
}

// Response structure for /verifyHmac
#[derive(Debug, Serialize)]
pub struct VerifyHmacResponse {
    pub valid: bool,
}

// /verifyHmac - BRC-100 endpoint for verifying HMAC
pub async fn verify_hmac(
    state: web::Data<AppState>,
    body: web::Bytes,
) -> HttpResponse {
    log::info!("📋 /verifyHmac called");
    log::info!("   Raw body: {}", String::from_utf8_lossy(&body));

    // Try to parse JSON
    let req: VerifyHmacRequest = match serde_json::from_slice(&body) {
        Ok(r) => r,
        Err(e) => {
            log::error!("   JSON parse error: {}", e);
            return HttpResponse::BadRequest().json(serde_json::json!({
                "error": format!("Invalid JSON: {}", e)
            }));
        }
    };

    log::info!("   Protocol ID: {:?}", req.protocol_id);
    log::info!("   Key ID: {}", req.key_id);
    log::info!("   Counterparty: {:?}", req.counterparty);
    log::info!("   HMAC: {:?}", req.hmac);

    // Parse HMAC (can be array of bytes or hex string)
    let expected_hmac: Vec<u8> = match &req.hmac {
        serde_json::Value::Array(arr) => {
            // Array of numbers
            arr.iter()
                .filter_map(|v| v.as_u64().map(|n| n as u8))
                .collect()
        },
        serde_json::Value::String(s) => {
            // Hex string
            match hex::decode(s) {
                Ok(b) => b,
                Err(e) => {
                    log::error!("   Failed to decode HMAC hex: {}", e);
                    return HttpResponse::BadRequest().json(serde_json::json!({
                        "error": format!("Invalid HMAC hex: {}", e)
                    }));
                }
            }
        },
        _ => {
            log::error!("   HMAC must be array or hex string");
            return HttpResponse::BadRequest().json(serde_json::json!({
                "error": "HMAC must be array or hex string"
            }));
        }
    };

    log::info!("   HMAC bytes length: {}", expected_hmac.len());

    // Parse data (can be array of bytes or base64 string)
    let data_bytes: Vec<u8> = match &req.data {
        serde_json::Value::Array(arr) => {
            // Array of numbers
            arr.iter()
                .filter_map(|v| v.as_u64().map(|n| n as u8))
                .collect()
        },
        serde_json::Value::String(s) => {
            // Base64 string
            match base64::decode(s) {
                Ok(b) => b,
                Err(e) => {
                    log::error!("   Failed to decode base64 data: {}", e);
                    return HttpResponse::BadRequest().json(serde_json::json!({
                        "error": format!("Invalid base64 data: {}", e)
                    }));
                }
            }
        },
        _ => {
            log::error!("   Data must be array or base64 string");
            return HttpResponse::BadRequest().json(serde_json::json!({
                "error": "Data must be array or base64 string"
            }));
        }
    };

    log::info!("   Data bytes length: {}", data_bytes.len());

    // Parse counterparty (can be "self" or hex public key)
    let counterparty_hex: Option<String> = match &req.counterparty {
        serde_json::Value::String(s) if s == "self" => {
            log::info!("   No counterparty (self)");
            None
        },
        serde_json::Value::String(s) => {
            log::info!("   Counterparty public key: {}", s);
            Some(s.clone())
        },
        _ => None
    };

    // Parse protocol ID (can be [level, "name"] or just "name")
    let protocol_id_str = match &req.protocol_id {
        serde_json::Value::Array(arr) => {
            if arr.len() >= 2 {
                if let Some(name) = arr[1].as_str() {
                    name.to_string()
                } else {
                    log::error!("   Invalid protocol ID array format");
                    return HttpResponse::BadRequest().json(serde_json::json!({
                        "error": "Invalid protocol ID format"
                    }));
                }
            } else {
                log::error!("   Protocol ID array too short");
                return HttpResponse::BadRequest().json(serde_json::json!({
                    "error": "Protocol ID array too short"
                }));
            }
        },
        serde_json::Value::String(s) => s.clone(),
        _ => {
            log::error!("   Protocol ID must be array or string");
            return HttpResponse::BadRequest().json(serde_json::json!({
                "error": "Protocol ID must be array or string"
            }));
        }
    };

    // Normalize protocol ID
    let protocol_id = match normalize_protocol_id(&protocol_id_str) {
        Ok(p) => p,
        Err(e) => {
            log::error!("   Failed to normalize protocol ID: {}", e);
            return HttpResponse::BadRequest().json(serde_json::json!({
                "error": format!("Invalid protocol ID: {}", e)
            }));
        }
    };

    // Create BRC-43 invoice number
    let security_level = if counterparty_hex.is_some() {
        SecurityLevel::CounterpartyLevel
    } else {
        SecurityLevel::CounterpartyLevel // Default to level 2
    };

    let invoice_number = match InvoiceNumber::new(
        security_level,
        protocol_id,
        req.key_id.clone()
    ) {
        Ok(inv) => inv.to_string(),
        Err(e) => {
            log::error!("   Failed to create invoice number: {}", e);
            return HttpResponse::InternalServerError().json(serde_json::json!({
                "error": format!("Invoice number error: {}", e)
            }));
        }
    };

    log::info!("   BRC-43 invoice number: {}", invoice_number);

    // Derive private key from mnemonic (for address index 0)
    let storage = state.storage.lock().unwrap();
    let private_key_bytes = match storage.derive_private_key(0) {
        Ok(key) => {
            log::info!("   ✅ Private key derived from mnemonic (verifyHmac)");
            key
        },
        Err(e) => {
            log::error!("   Failed to derive private key: {}", e);
            drop(storage);
            return HttpResponse::InternalServerError().json(serde_json::json!({
                "error": format!("Key derivation error: {}", e)
            }));
        }
    };
    drop(storage);

    // Determine HMAC key based on whether counterparty is provided
    let hmac_key = if let Some(counterparty_hex) = &counterparty_hex {
        // BRC-42: Derive child key for mutual authentication
        let counterparty_bytes = match hex::decode(counterparty_hex) {
            Ok(b) => b,
            Err(e) => {
                log::error!("   Failed to decode counterparty key: {}", e);
                return HttpResponse::BadRequest().json(serde_json::json!({
                    "error": "Invalid counterparty key"
                }));
            }
        };

        log::info!("   Deriving BRC-42 child key for HMAC verification...");
        match derive_child_private_key(&private_key_bytes, &counterparty_bytes, &invoice_number) {
            Ok(key) => {
                log::info!("   ✅ BRC-42 child key derived");
                key
            },
            Err(e) => {
                log::error!("   BRC-42 derivation failed: {}", e);
                return HttpResponse::InternalServerError().json(serde_json::json!({
                    "error": format!("Key derivation error: {}", e)
                }));
            }
        }
    } else {
        // Simple HMAC with wallet's private key (no BRC-42 derivation)
        log::info!("   Using wallet private key for HMAC verification (no counterparty)");
        private_key_bytes
    };

    // Verify HMAC
    let is_valid = verify_hmac_sha256(&hmac_key, &data_bytes, &expected_hmac);

    log::info!("   ✅ HMAC verification result: {}", is_valid);

    HttpResponse::Ok().json(VerifyHmacResponse { valid: is_valid })
}

// Wallet status endpoint
pub async fn wallet_status(state: web::Data<AppState>) -> HttpResponse {
    let storage = state.storage.lock().unwrap();
    let exists = storage.get_wallet().is_ok();

    log::info!("📋 Wallet status: exists={}", exists);

    HttpResponse::Ok().json(serde_json::json!({
        "exists": exists
    }))
}

// Wallet balance endpoint
pub async fn wallet_balance(state: web::Data<AppState>) -> HttpResponse {
    let storage = state.storage.lock().unwrap();

    match storage.get_all_addresses() {
        Ok(addresses) => {
            log::info!("📋 Balance check for {} addresses", addresses.len());
            HttpResponse::Ok().json(serde_json::json!({
                "balance": 0,
                "addresses": addresses.len()
            }))
        }
        Err(e) => {
            log::error!("   Failed to get addresses: {}", e);
            HttpResponse::InternalServerError().json(serde_json::json!({
                "error": e
            }))
        }
    }
}

// Request structure for /verifySignature
#[derive(Debug, Deserialize)]
pub struct VerifySignatureRequest {
    #[serde(rename = "data")]
    pub data: Option<serde_json::Value>, // Array of bytes
    #[serde(rename = "hashToDirectlyVerify")]
    pub hash_to_directly_verify: Option<serde_json::Value>, // Array of bytes
    #[serde(rename = "signature")]
    pub signature: serde_json::Value, // Array of bytes (64-byte compact R+S)
    #[serde(rename = "protocolID")]
    pub protocol_id: serde_json::Value, // [2, "auth message signature"]
    #[serde(rename = "keyID")]
    pub key_id: String, // e.g., "nonce1 nonce2"
    #[serde(rename = "counterparty")]
    pub counterparty: String, // Public key hex
}

// Response structure for /verifySignature
#[derive(Debug, Serialize)]
pub struct VerifySignatureResponse {
    pub valid: bool,
}

// /verifySignature - BRC-77 endpoint for verifying ECDSA signatures
pub async fn verify_signature(
    state: web::Data<AppState>,
    body: web::Bytes,
) -> HttpResponse {
    log::info!("📋 /verifySignature called");

    // Parse JSON body
    let body_str = String::from_utf8_lossy(&body);
    log::info!("   Raw body: {}", body_str);

    let req: VerifySignatureRequest = match serde_json::from_slice(&body) {
        Ok(r) => r,
        Err(e) => {
            log::error!("   JSON parse error: {}", e);
            return HttpResponse::BadRequest().json(serde_json::json!({
                "error": format!("Invalid JSON: {}", e)
            }));
        }
    };

    // Parse signature bytes (can be array of bytes OR hex string)
    let signature_bytes = match &req.signature {
        serde_json::Value::Array(arr) => {
            arr.iter().filter_map(|v| v.as_u64().map(|n| n as u8)).collect::<Vec<u8>>()
        }
        serde_json::Value::String(hex_str) => {
            match hex::decode(hex_str) {
                Ok(bytes) => bytes,
                Err(e) => {
                    return HttpResponse::BadRequest().json(serde_json::json!({
                        "error": format!("Invalid signature hex: {}", e)
                    }));
                }
            }
        }
        _ => {
            return HttpResponse::BadRequest().json(serde_json::json!({
                "error": "Signature must be an array of bytes or hex string"
            }));
        }
    };

    if signature_bytes.len() != 64 {
        return HttpResponse::BadRequest().json(serde_json::json!({
            "error": format!("Signature must be 64 bytes (compact R+S), got {}", signature_bytes.len())
        }));
    }

    log::info!("   Signature: {} bytes", signature_bytes.len());

    // Get the hash to verify
    let data_hash = if let Some(hash) = &req.hash_to_directly_verify {
        // Use provided hash directly
        match hash {
            serde_json::Value::Array(arr) => {
                arr.iter().filter_map(|v| v.as_u64().map(|n| n as u8)).collect::<Vec<u8>>()
            }
            _ => {
                return HttpResponse::BadRequest().json(serde_json::json!({
                    "error": "hashToDirectlyVerify must be an array of bytes"
                }));
            }
        }
    } else if let Some(data) = &req.data {
        // Hash the data
        let data_bytes = match data {
            serde_json::Value::Array(arr) => {
                arr.iter().filter_map(|v| v.as_u64().map(|n| n as u8)).collect::<Vec<u8>>()
            }
            _ => {
                return HttpResponse::BadRequest().json(serde_json::json!({
                    "error": "data must be an array of bytes"
                }));
            }
        };
        sha256(&data_bytes)
    } else {
        return HttpResponse::BadRequest().json(serde_json::json!({
            "error": "Either 'data' or 'hashToDirectlyVerify' must be provided"
        }));
    };

    if data_hash.len() != 32 {
        return HttpResponse::BadRequest().json(serde_json::json!({
            "error": format!("Hash must be 32 bytes, got {}", data_hash.len())
        }));
    }

    // Parse protocol ID (similar to createHmac)
    let protocol_id_str = if let serde_json::Value::Array(arr) = &req.protocol_id {
        if arr.len() == 2 {
            if let (Some(level), Some(name)) = (arr[0].as_u64(), arr[1].as_str()) {
                format!("{}-{}", level, name)
            } else {
                return HttpResponse::BadRequest().json(serde_json::json!({
                    "error": "Invalid protocolID format"
                }));
            }
        } else {
            return HttpResponse::BadRequest().json(serde_json::json!({
                "error": "protocolID must be [securityLevel, protocolName]"
            }));
        }
    } else {
        return HttpResponse::BadRequest().json(serde_json::json!({
            "error": "protocolID must be an array"
        }));
    };

    log::info!("   Protocol ID: {}", protocol_id_str);
    log::info!("   Key ID: {}", req.key_id);
    log::info!("   Counterparty: {}", req.counterparty);

    // Create BRC-43 invoice number
    let invoice = format!("{}-{}", protocol_id_str, req.key_id);
    log::info!("   BRC-43 invoice: {}", invoice);

    // Decode counterparty public key from hex
    let counterparty_pubkey = match hex::decode(&req.counterparty) {
        Ok(bytes) => bytes,
        Err(e) => {
            return HttpResponse::BadRequest().json(serde_json::json!({
                "error": format!("Invalid counterparty public key hex: {}", e)
            }));
        }
    };

    if counterparty_pubkey.len() != 33 {
        return HttpResponse::BadRequest().json(serde_json::json!({
            "error": format!("Counterparty public key must be 33 bytes, got {}", counterparty_pubkey.len())
        }));
    }

    // Get our private key (needed for BRC-42 shared secret computation)
    let storage = state.storage.lock().unwrap();
    let private_key_bytes = match storage.derive_private_key(0) {
        Ok(key) => key,
        Err(e) => {
            drop(storage);
            log::error!("   Failed to derive private key: {}", e);
            return HttpResponse::InternalServerError().json(serde_json::json!({
                "error": "Failed to derive private key"
            }));
        }
    };
    drop(storage);

    log::info!("   ✅ Private key derived from mnemonic");

    // Derive BRC-42 child public key for verification
    // From the verifier's perspective, we derive the child public key that the signer used
    let child_pubkey = match derive_child_public_key(&private_key_bytes, &counterparty_pubkey, &invoice) {
        Ok(key) => key,
        Err(e) => {
            log::error!("   Failed to derive child public key: {}", e);
            return HttpResponse::InternalServerError().json(serde_json::json!({
                "error": format!("Failed to derive child public key: {}", e)
            }));
        }
    };

    log::info!("   ✅ BRC-42 child public key derived for verification");
    log::info!("   Child pubkey: {}", hex::encode(&child_pubkey));

    // Verify ECDSA signature using secp256k1
    use secp256k1::{Secp256k1, Message, ecdsa::Signature, PublicKey};

    let secp = Secp256k1::new();

    let message = match Message::from_slice(&data_hash) {
        Ok(msg) => msg,
        Err(e) => {
            log::error!("   Invalid message hash: {}", e);
            return HttpResponse::BadRequest().json(serde_json::json!({
                "error": format!("Invalid message hash: {}", e)
            }));
        }
    };

    let pubkey = match PublicKey::from_slice(&child_pubkey) {
        Ok(pk) => pk,
        Err(e) => {
            log::error!("   Invalid child public key: {}", e);
            return HttpResponse::InternalServerError().json(serde_json::json!({
                "error": format!("Invalid child public key: {}", e)
            }));
        }
    };

    let signature = match Signature::from_compact(&signature_bytes) {
        Ok(sig) => sig,
        Err(e) => {
            log::error!("   Invalid signature format: {}", e);
            return HttpResponse::BadRequest().json(serde_json::json!({
                "error": format!("Invalid signature format: {}", e)
            }));
        }
    };

    // Verify the signature
    let valid = secp.verify_ecdsa(&message, &signature, &pubkey).is_ok();

    log::info!("   ✅ Signature verification result: {}", valid);

    HttpResponse::Ok().json(VerifySignatureResponse { valid })
}

// Request structure for /createSignature
#[derive(Debug, Deserialize)]
pub struct CreateSignatureRequest {
    #[serde(rename = "protocolID")]
    pub protocol_id: serde_json::Value, // [2, "protocol name"]
    #[serde(rename = "keyID")]
    pub key_id: String,
    #[serde(rename = "counterparty")]
    pub counterparty: serde_json::Value, // "self", "anyone", or hex pubkey
    #[serde(rename = "data")]
    pub data: serde_json::Value, // Array of bytes to sign
}

// Response structure for /createSignature
#[derive(Debug, Serialize)]
pub struct CreateSignatureResponse {
    pub signature: Vec<u8>, // DER-encoded signature
}

// /createSignature - BRC-3 endpoint for creating ECDSA signatures
pub async fn create_signature(
    state: web::Data<AppState>,
    body: web::Bytes,
) -> HttpResponse {
    log::info!("📋 /createSignature called");

    // Parse JSON body
    let body_str = String::from_utf8_lossy(&body);
    log::info!("   Raw body: {}", body_str);

    let req: CreateSignatureRequest = match serde_json::from_slice(&body) {
        Ok(r) => r,
        Err(e) => {
            log::error!("   JSON parse error: {}", e);
            return HttpResponse::BadRequest().json(serde_json::json!({
                "error": format!("Invalid JSON: {}", e)
            }));
        }
    };

    // Parse protocol ID
    let protocol_id_str = if let serde_json::Value::Array(arr) = &req.protocol_id {
        if arr.len() == 2 {
            if let (Some(level), Some(name)) = (arr[0].as_u64(), arr[1].as_str()) {
                format!("{}-{}", level, name)
            } else {
                return HttpResponse::BadRequest().json(serde_json::json!({
                    "error": "Invalid protocolID format"
                }));
            }
        } else {
            return HttpResponse::BadRequest().json(serde_json::json!({
                "error": "protocolID must be [securityLevel, protocolName]"
            }));
        }
    } else {
        return HttpResponse::BadRequest().json(serde_json::json!({
            "error": "protocolID must be an array"
        }));
    };

    log::info!("   Protocol ID: {}", protocol_id_str);
    log::info!("   Key ID: {}", req.key_id);
    log::info!("   Counterparty: {:?}", req.counterparty);

    // Parse data bytes
    let data_bytes = match &req.data {
        serde_json::Value::Array(arr) => {
            arr.iter().filter_map(|v| v.as_u64().map(|n| n as u8)).collect::<Vec<u8>>()
        }
        _ => {
            return HttpResponse::BadRequest().json(serde_json::json!({
                "error": "data must be an array of bytes"
            }));
        }
    };

    log::info!("   Data bytes length: {}", data_bytes.len());

    // Create BRC-43 invoice number
    let invoice = format!("{}-{}", protocol_id_str, req.key_id);
    log::info!("   BRC-43 invoice: {}", invoice);

    // Get counterparty public key (if not "self" or "anyone")
    let counterparty_pubkey = match &req.counterparty {
        serde_json::Value::String(s) if s == "self" || s == "anyone" => {
            log::info!("   No counterparty ({})", s);
            None
        }
        serde_json::Value::String(hex_pubkey) => {
            match hex::decode(hex_pubkey) {
                Ok(bytes) if bytes.len() == 33 => {
                    log::info!("   Counterparty pubkey: {}", hex_pubkey);
                    Some(bytes)
                }
                Ok(bytes) => {
                    return HttpResponse::BadRequest().json(serde_json::json!({
                        "error": format!("Counterparty public key must be 33 bytes, got {}", bytes.len())
                    }));
                }
                Err(e) => {
                    return HttpResponse::BadRequest().json(serde_json::json!({
                        "error": format!("Invalid counterparty hex: {}", e)
                    }));
                }
            }
        }
        _ => {
            return HttpResponse::BadRequest().json(serde_json::json!({
                "error": "counterparty must be a string"
            }));
        }
    };

    // Get our private key
    let storage = state.storage.lock().unwrap();
    let private_key_bytes = match storage.derive_private_key(0) {
        Ok(key) => key,
        Err(e) => {
            drop(storage);
            log::error!("   Failed to derive private key: {}", e);
            return HttpResponse::InternalServerError().json(serde_json::json!({
                "error": "Failed to derive private key"
            }));
        }
    };
    drop(storage);

    log::info!("   ✅ Private key derived from mnemonic");

    // Derive BRC-42 child private key
    let child_privkey = if let Some(counterparty_pub) = counterparty_pubkey {
        // BRC-42 derivation with counterparty
        match derive_child_private_key(&private_key_bytes, &counterparty_pub, &invoice) {
            Ok(key) => key,
            Err(e) => {
                log::error!("   Failed to derive BRC-42 child key: {}", e);
                return HttpResponse::InternalServerError().json(serde_json::json!({
                    "error": format!("Failed to derive child key: {}", e)
                }));
            }
        }
    } else {
        // Simple derivation (no counterparty)
        log::info!("   Using wallet private key for signature (no counterparty)");
        private_key_bytes
    };

    log::info!("   ✅ Child private key derived");

    // Hash the data with SHA256
    let data_hash = sha256(&data_bytes);
    log::info!("   Data hash (32 bytes): {}", hex::encode(&data_hash));

    // Sign with ECDSA
    use secp256k1::{Secp256k1, Message, SecretKey};

    let secp = Secp256k1::new();

    let secret = match SecretKey::from_slice(&child_privkey) {
        Ok(key) => key,
        Err(e) => {
            log::error!("   Invalid private key: {}", e);
            return HttpResponse::InternalServerError().json(serde_json::json!({
                "error": "Invalid private key"
            }));
        }
    };

    let message = match Message::from_slice(&data_hash) {
        Ok(msg) => msg,
        Err(e) => {
            log::error!("   Invalid message hash: {}", e);
            return HttpResponse::InternalServerError().json(serde_json::json!({
                "error": "Invalid message hash"
            }));
        }
    };

    let signature = secp.sign_ecdsa(&message, &secret);

    // Serialize as DER format (not compact!)
    let signature_der = signature.serialize_der();
    let signature_hex = hex::encode(&signature_der);

    log::info!("   ✅ Signature created ({} bytes, DER format): {}", signature_der.len(), signature_hex);

    HttpResponse::Ok().json(CreateSignatureResponse {
        signature: signature_der.to_vec()
    })
}

// ============================================================================
// Transaction Action Endpoints (BRC-1)
// ============================================================================

use crate::transaction::{Transaction, TxInput, TxOutput, OutPoint};
use crate::utxo_fetcher::{fetch_all_utxos, UTXO};
use std::collections::HashMap;
use std::sync::Mutex as StdMutex;
use once_cell::sync::Lazy;

// Pending transaction with metadata
#[derive(Debug, Clone)]
struct PendingTransaction {
    tx: Transaction,
    input_utxos: Vec<UTXO>, // UTXOs being spent (for signing)
}

// In-memory storage for pending transactions
static PENDING_TRANSACTIONS: Lazy<StdMutex<HashMap<String, PendingTransaction>>> =
    Lazy::new(|| StdMutex::new(HashMap::new()));

// Request structure for /createAction
#[derive(Debug, Serialize, Deserialize)]
pub struct CreateActionRequest {
    #[serde(rename = "outputs")]
    pub outputs: Vec<CreateActionOutput>,

    #[serde(rename = "description")]
    pub description: Option<String>,

    #[serde(rename = "options")]
    pub options: Option<CreateActionOptions>,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct CreateActionOutput {
    #[serde(rename = "satoshis")]
    pub satoshis: Option<i64>,

    #[serde(rename = "script")]
    pub script: String, // Hex-encoded locking script
}

#[derive(Debug, Serialize, Deserialize)]
pub struct CreateActionOptions {
    #[serde(rename = "returnTXIDOnly")]
    pub return_txid_only: Option<bool>,
}

// Response structure for /createAction
#[derive(Debug, Serialize, Deserialize)]
pub struct CreateActionResponse {
    pub txid: Option<String>,
    pub reference: String,
    #[serde(rename = "rawTx")]
    pub raw_tx: Option<String>,
}

// /createAction - Build unsigned transaction
pub async fn create_action(
    state: web::Data<AppState>,
    body: web::Bytes,
) -> HttpResponse {
    log::info!("📋 /createAction called");

    // Parse request
    let req: CreateActionRequest = match serde_json::from_slice(&body) {
        Ok(r) => r,
        Err(e) => {
            log::error!("   JSON parse error: {}", e);
            return HttpResponse::BadRequest().json(serde_json::json!({
                "error": format!("Invalid JSON: {}", e)
            }));
        }
    };

    log::info!("   Description: {:?}", req.description);
    log::info!("   Outputs: {}", req.outputs.len());

    // Calculate total output amount
    let mut total_output: i64 = 0;
    for (i, output) in req.outputs.iter().enumerate() {
        if let Some(sats) = output.satoshis {
            total_output += sats;
            log::info!("   Output {}: {} satoshis", i, sats);
        }
    }

    log::info!("   Total output amount: {} satoshis", total_output);

    // Estimate fee (rough calculation: ~200 bytes per input + output + overhead)
    let estimated_fee = 5000; // Increased fee: 5000 sats (~22 sat/byte for 225 byte tx)
    let total_needed = total_output + estimated_fee;

    log::info!("   Estimated fee: {} satoshis", estimated_fee);
    log::info!("   Total needed: {} satoshis", total_needed);

    // Fetch UTXOs from WhatsOnChain
    let storage = state.storage.lock().unwrap();
    let addresses = match storage.get_all_addresses() {
        Ok(addrs) => addrs.to_vec(),
        Err(e) => {
            drop(storage);
            log::error!("   Failed to get addresses: {}", e);
            return HttpResponse::InternalServerError().json(serde_json::json!({
                "error": "Failed to get addresses"
            }));
        }
    };
    drop(storage);

    log::info!("   Checking {} addresses for UTXOs...", addresses.len());

    let all_utxos = match fetch_all_utxos(&addresses).await {
        Ok(utxos) => utxos,
        Err(e) => {
            log::error!("   Failed to fetch UTXOs: {}", e);
            return HttpResponse::InternalServerError().json(serde_json::json!({
                "error": format!("Failed to fetch UTXOs: {}", e)
            }));
        }
    };

    if all_utxos.is_empty() {
        log::error!("   No UTXOs available");
        return HttpResponse::BadRequest().json(serde_json::json!({
            "error": "Insufficient funds: no UTXOs available"
        }));
    }

    // Select UTXOs to cover the amount
    let selected_utxos = select_utxos(&all_utxos, total_needed);

    if selected_utxos.is_empty() {
        log::error!("   Insufficient funds");
        return HttpResponse::BadRequest().json(serde_json::json!({
            "error": format!("Insufficient funds: need {} sats, have {} sats",
                total_needed,
                all_utxos.iter().map(|u| u.satoshis).sum::<i64>()
            )
        }));
    }

    let total_input: i64 = selected_utxos.iter().map(|u| u.satoshis).sum();
    log::info!("   Selected {} UTXOs ({} satoshis)", selected_utxos.len(), total_input);

    // Build transaction
    let mut tx = Transaction::new();

    // Add inputs (unsigned)
    for utxo in &selected_utxos {
        let outpoint = OutPoint::new(utxo.txid.clone(), utxo.vout);
        tx.add_input(TxInput::new(outpoint));
    }

    // Add requested outputs
    for output in &req.outputs {
        let script_bytes = match hex::decode(&output.script) {
            Ok(bytes) => bytes,
            Err(e) => {
                return HttpResponse::BadRequest().json(serde_json::json!({
                    "error": format!("Invalid output script hex: {}", e)
                }));
            }
        };

        let satoshis = output.satoshis.unwrap_or(0);
        tx.add_output(TxOutput::new(satoshis, script_bytes));
    }

    // Calculate change
    let change = total_input - total_output - estimated_fee;
    log::info!("   Change: {} satoshis", change);

    if change > 546 { // Dust limit
        // Get first address for change
        let storage = state.storage.lock().unwrap();
        let change_addr = match storage.get_current_address() {
            Ok(addr) => addr.clone(),
            Err(e) => {
                drop(storage);
                return HttpResponse::InternalServerError().json(serde_json::json!({
                    "error": format!("Failed to get change address: {}", e)
                }));
            }
        };
        drop(storage);

        // Build P2PKH script for change
        use crate::transaction::Script;
        use sha2::{Sha256, Digest};
        use ripemd::{Ripemd160, Digest as RipemdDigest};

        // Decode public key and hash it
        let pubkey_bytes = match hex::decode(&change_addr.public_key) {
            Ok(bytes) => bytes,
            Err(_) => {
                return HttpResponse::InternalServerError().json(serde_json::json!({
                    "error": "Invalid public key"
                }));
            }
        };

        // SHA256 then RIPEMD160
        let sha_hash = Sha256::digest(&pubkey_bytes);
        let pubkey_hash = Ripemd160::digest(&sha_hash);

        let change_script = match Script::p2pkh_locking_script(&pubkey_hash) {
            Ok(script) => script,
            Err(e) => {
                return HttpResponse::InternalServerError().json(serde_json::json!({
                    "error": format!("Failed to create change script: {}", e)
                }));
            }
        };

        tx.add_output(TxOutput::new(change, change_script.bytes));
        log::info!("   Added change output: {} satoshis", change);
    } else if change > 0 {
        log::info!("   Change below dust limit ({}), adding to fee", change);
    }

    // Calculate txid
    let txid = match tx.txid() {
        Ok(id) => id,
        Err(e) => {
            return HttpResponse::InternalServerError().json(serde_json::json!({
                "error": format!("Failed to calculate txid: {}", e)
            }));
        }
    };

    // Generate reference ID
    let reference = format!("action-{}", uuid::Uuid::new_v4());

    // Store transaction in memory with UTXO metadata for signing
    {
        let mut pending = PENDING_TRANSACTIONS.lock().unwrap();
        pending.insert(reference.clone(), PendingTransaction {
            tx,
            input_utxos: selected_utxos,
        });
    }

    log::info!("   ✅ Transaction created: {}", txid);
    log::info!("   Reference: {}", reference);

    HttpResponse::Ok().json(CreateActionResponse {
        txid: Some(txid),
        reference,
        raw_tx: None,
    })
}

// Select UTXOs to cover required amount (simple greedy algorithm)
fn select_utxos(available: &[UTXO], amount_needed: i64) -> Vec<UTXO> {
    let mut selected = Vec::new();
    let mut total: i64 = 0;

    // Sort by value (largest first) for efficiency
    let mut sorted_utxos = available.to_vec();
    sorted_utxos.sort_by(|a, b| b.satoshis.cmp(&a.satoshis));

    for utxo in sorted_utxos {
        selected.push(utxo.clone());
        total += utxo.satoshis;

        if total >= amount_needed {
            break;
        }
    }

    if total < amount_needed {
        // Not enough funds
        return Vec::new();
    }

    selected
}

// Request structure for /signAction
#[derive(Debug, Serialize, Deserialize)]
pub struct SignActionRequest {
    #[serde(rename = "reference")]
    pub reference: String,

    #[serde(rename = "spends")]
    pub spends: Option<serde_json::Value>, // Not used in simple implementation
}

// Response structure for /signAction
#[derive(Debug, Serialize, Deserialize)]
pub struct SignActionResponse {
    pub txid: String,
    #[serde(rename = "rawTx")]
    pub raw_tx: String,
}

// /signAction - Sign transaction inputs
pub async fn sign_action(
    state: web::Data<AppState>,
    body: web::Bytes,
) -> HttpResponse {
    log::info!("📋 /signAction called");

    // Parse request
    let req: SignActionRequest = match serde_json::from_slice(&body) {
        Ok(r) => r,
        Err(e) => {
            log::error!("   JSON parse error: {}", e);
            return HttpResponse::BadRequest().json(serde_json::json!({
                "error": format!("Invalid JSON: {}", e)
            }));
        }
    };

    log::info!("   Reference: {}", req.reference);

    // Retrieve pending transaction
    let pending_tx = {
        let pending = PENDING_TRANSACTIONS.lock().unwrap();
        match pending.get(&req.reference) {
            Some(ptx) => ptx.clone(),
            None => {
                log::error!("   Transaction not found: {}", req.reference);
                return HttpResponse::NotFound().json(serde_json::json!({
                    "error": "Transaction reference not found"
                }));
            }
        }
    };

    let mut tx = pending_tx.tx;
    let input_utxos = pending_tx.input_utxos;

    log::info!("   Signing {} inputs...", tx.inputs.len());

    // Sign each input
    for (i, input_utxo) in input_utxos.iter().enumerate() {
        log::info!("   Signing input {}: {}:{} (address index {})",
            i, input_utxo.txid, input_utxo.vout, input_utxo.address_index);

        // Get the private key for THIS specific address (not always index 0!)
        let storage = state.storage.lock().unwrap();
        let private_key_bytes = match storage.derive_private_key(input_utxo.address_index) {
            Ok(key) => key,
            Err(e) => {
                drop(storage);
                log::error!("   Failed to derive private key for address index {}: {}", input_utxo.address_index, e);
                return HttpResponse::InternalServerError().json(serde_json::json!({
                    "error": format!("Failed to derive private key for address {}", input_utxo.address_index)
                }));
            }
        };
        drop(storage);

        // Decode prev script
        let prev_script = match hex::decode(&input_utxo.script) {
            Ok(bytes) => bytes,
            Err(e) => {
                log::error!("   Invalid script hex: {}", e);
                return HttpResponse::InternalServerError().json(serde_json::json!({
                    "error": format!("Invalid script hex for input {}: {}", i, e)
                }));
            }
        };

        // Calculate SIGHASH
        use crate::transaction::{calculate_sighash, SIGHASH_ALL_FORKID, Script};

        let sighash: Vec<u8> = match calculate_sighash(&tx, i, &prev_script, input_utxo.satoshis, SIGHASH_ALL_FORKID) {
            Ok(hash) => hash,
            Err(e) => {
                log::error!("   Failed to calculate sighash: {}", e);
                return HttpResponse::InternalServerError().json(serde_json::json!({
                    "error": format!("Failed to calculate sighash for input {}: {}", i, e)
                }));
            }
        };

        log::info!("   SIGHASH: {}", hex::encode(&sighash));

        // Sign with ECDSA
        use secp256k1::{Secp256k1, Message, SecretKey};

        let secp = Secp256k1::new();
        let secret = match SecretKey::from_slice(&private_key_bytes) {
            Ok(key) => key,
            Err(e) => {
                log::error!("   Invalid private key: {}", e);
                return HttpResponse::InternalServerError().json(serde_json::json!({
                    "error": "Invalid private key"
                }));
            }
        };

        let message = match Message::from_slice(&sighash) {
            Ok(msg) => msg,
            Err(e) => {
                log::error!("   Invalid sighash: {}", e);
                return HttpResponse::InternalServerError().json(serde_json::json!({
                    "error": "Invalid sighash"
                }));
            }
        };

        let signature = secp.sign_ecdsa(&message, &secret);

        // Serialize signature as DER + sighash byte
        let mut sig_der = signature.serialize_der().to_vec();
        sig_der.push(SIGHASH_ALL_FORKID as u8); // Append sighash type byte

        log::info!("   Signature ({} bytes): {}", sig_der.len(), hex::encode(&sig_der));

        // Get public key
        use secp256k1::PublicKey;
        let pubkey = PublicKey::from_secret_key(&secp, &secret);
        let pubkey_bytes = pubkey.serialize();

        log::info!("   Public key length: {} bytes", pubkey_bytes.len());
        log::info!("   Public key: {}", hex::encode(&pubkey_bytes));
        log::info!("   Private key (first 8 bytes): {}...", hex::encode(&private_key_bytes[..8]));

        // Build unlocking script: <signature> <pubkey>
        let unlocking_script = Script::p2pkh_unlocking_script(&sig_der, &pubkey_bytes);

        // Update input with unlocking script
        tx.inputs[i].set_script(unlocking_script.bytes);

        log::info!("   ✅ Input {} signed", i);
    }

    // Serialize signed transaction
    let raw_tx = match tx.to_hex() {
        Ok(hex) => hex,
        Err(e) => {
            log::error!("   Failed to serialize transaction: {}", e);
            return HttpResponse::InternalServerError().json(serde_json::json!({
                "error": format!("Failed to serialize transaction: {}", e)
            }));
        }
    };

    // Calculate final txid
    let txid = match tx.txid() {
        Ok(id) => id,
        Err(e) => {
            log::error!("   Failed to calculate txid: {}", e);
            return HttpResponse::InternalServerError().json(serde_json::json!({
                "error": format!("Failed to calculate txid: {}", e)
            }));
        }
    };

    log::info!("   ✅ Transaction signed: {}", txid);
    log::info!("   Raw TX length: {} bytes", raw_tx.len() / 2);

    HttpResponse::Ok().json(SignActionResponse {
        txid,
        raw_tx,
    })
}

// Request structure for /processAction
#[derive(Debug, Deserialize)]
pub struct ProcessActionRequest {
    #[serde(rename = "outputs")]
    pub outputs: Vec<CreateActionOutput>,

    #[serde(rename = "description")]
    pub description: Option<String>,

    #[serde(rename = "broadcast")]
    pub broadcast: Option<bool>,
}

// Response structure for /processAction
#[derive(Debug, Serialize)]
pub struct ProcessActionResponse {
    pub txid: String,
    pub status: String,
    #[serde(rename = "rawTx")]
    pub raw_tx: Option<String>,
}

// /processAction - Complete transaction flow (create + sign + broadcast)
pub async fn process_action(
    state: web::Data<AppState>,
    body: web::Bytes,
) -> HttpResponse {
    log::info!("📋 /processAction called");

    // Parse request
    let req: ProcessActionRequest = match serde_json::from_slice(&body) {
        Ok(r) => r,
        Err(e) => {
            log::error!("   JSON parse error: {}", e);
            return HttpResponse::BadRequest().json(serde_json::json!({
                "error": format!("Invalid JSON: {}", e)
            }));
        }
    };

    let should_broadcast = req.broadcast.unwrap_or(true);
    log::info!("   Broadcast: {}", should_broadcast);

    // Step 1: Create action (build unsigned transaction)
    let create_req = CreateActionRequest {
        outputs: req.outputs,
        description: req.description,
        options: Some(CreateActionOptions {
            return_txid_only: Some(false),
        }),
    };

    let create_body = serde_json::to_vec(&create_req).unwrap();
    let create_response = create_action(state.clone(), web::Bytes::from(create_body)).await;

    // Extract reference from create response
    let create_json: CreateActionResponse = match create_response.status().is_success() {
        true => {
            let body_bytes = actix_web::body::to_bytes(create_response.into_body()).await.unwrap();
            serde_json::from_slice(&body_bytes).unwrap()
        }
        false => {
            log::error!("   createAction failed");
            return create_response;
        }
    };

    let reference = create_json.reference;
    log::info!("   Created transaction: {}", reference);

    // Step 2: Sign action
    let sign_req = SignActionRequest {
        reference: reference.clone(),
        spends: None,
    };

    let sign_body = serde_json::to_vec(&sign_req).unwrap();
    let sign_response = sign_action(state.clone(), web::Bytes::from(sign_body)).await;

    // Extract txid and rawTx from sign response
    let sign_json: SignActionResponse = match sign_response.status().is_success() {
        true => {
            let body_bytes = actix_web::body::to_bytes(sign_response.into_body()).await.unwrap();
            serde_json::from_slice(&body_bytes).unwrap()
        }
        false => {
            log::error!("   signAction failed");
            return sign_response;
        }
    };

    let txid = sign_json.txid.clone();
    let raw_tx = sign_json.raw_tx.clone();

    log::info!("   Signed transaction: {}", txid);

    // Step 3: Broadcast (if requested)
    let status = if should_broadcast {
        log::info!("   Broadcasting to network...");

        match broadcast_transaction(&raw_tx).await {
            Ok(_) => {
                log::info!("   ✅ Transaction broadcast successful!");
                "completed"
            }
            Err(e) => {
                log::error!("   ❌ Broadcast failed: {}", e);
                "failed"
            }
        }
    } else {
        log::info!("   Skipping broadcast (noSend option)");
        "nosend"
    };

    HttpResponse::Ok().json(ProcessActionResponse {
        txid,
        status: status.to_string(),
        raw_tx: Some(raw_tx),
    })
}

// Broadcast transaction to BSV network (multiple broadcasters for redundancy)
async fn broadcast_transaction(raw_tx_hex: &str) -> Result<String, String> {
    let client = reqwest::Client::new();
    let mut success_count = 0;
    let mut last_error = String::new();

    // Broadcaster 1: GorillaPool
    log::info!("   📡 Broadcasting to GorillaPool...");
    match broadcast_to_gorillapool(&client, raw_tx_hex).await {
        Ok(response) => {
            log::info!("   ✅ GorillaPool accepted: {}", response);
            success_count += 1;
        }
        Err(e) => {
            log::warn!("   ⚠️ GorillaPool failed: {}", e);
            last_error = e;
        }
    }

    // Broadcaster 2: WhatsOnChain
    log::info!("   📡 Broadcasting to WhatsOnChain...");
    match broadcast_to_whatsonchain(&client, raw_tx_hex).await {
        Ok(response) => {
            log::info!("   ✅ WhatsOnChain accepted: {}", response);
            success_count += 1;
        }
        Err(e) => {
            log::warn!("   ⚠️ WhatsOnChain failed: {}", e);
            last_error = e;
        }
    }

    if success_count > 0 {
        log::info!("   🎉 Broadcast successful to {} service(s)", success_count);
        Ok(format!("Broadcast to {} service(s)", success_count))
    } else {
        log::error!("   ❌ All broadcasters failed!");
        Err(format!("All broadcasters failed. Last error: {}", last_error))
    }
}

// Broadcast to GorillaPool
async fn broadcast_to_gorillapool(client: &reqwest::Client, raw_tx_hex: &str) -> Result<String, String> {
    let url = "https://mapi.gorillapool.io/mapi/tx";

    let body = serde_json::json!({
        "rawtx": raw_tx_hex
    });

    let response = client.post(url)
        .header("Content-Type", "application/json")
        .json(&body)
        .send()
        .await
        .map_err(|e| format!("HTTP error: {}", e))?;

    let status = response.status();
    let text = response.text().await.unwrap_or_default();

    if status.is_success() {
        Ok(text)
    } else {
        Err(format!("{} - {}", status, text))
    }
}

// Broadcast to WhatsOnChain
async fn broadcast_to_whatsonchain(client: &reqwest::Client, raw_tx_hex: &str) -> Result<String, String> {
    let url = "https://api.whatsonchain.com/v1/bsv/main/tx/raw";

    let body = serde_json::json!({
        "txhex": raw_tx_hex
    });

    let response = client.post(url)
        .header("Content-Type", "application/json")
        .json(&body)
        .send()
        .await
        .map_err(|e| format!("HTTP error: {}", e))?;

    let status = response.status();
    let text = response.text().await.unwrap_or_default();

    if status.is_success() {
        Ok(text)
    } else {
        Err(format!("{} - {}", status, text))
    }
}

pub async fn generate_address() -> HttpResponse {
    HttpResponse::Ok().json(serde_json::json!({"message": "Not implemented"}))
}

pub async fn send_transaction() -> HttpResponse {
    HttpResponse::Ok().json(serde_json::json!({"message": "Not implemented"}))
}

pub async fn check_domain() -> HttpResponse {
    HttpResponse::Ok().json(serde_json::json!({"whitelisted": true}))
}

pub async fn add_domain() -> HttpResponse {
    HttpResponse::Ok().json(serde_json::json!({"success": true}))
}
