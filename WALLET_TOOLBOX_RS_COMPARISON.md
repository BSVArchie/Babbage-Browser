# wallet-toolbox-rs vs Our rust-wallet: Comprehensive Comparison

> **Analysis Date**: 2025-01-XX
> **Purpose**: Compare the forked reference implementation with our current codebase
> **Note**: wallet-toolbox-rs is a Rust port of TypeScript `@bsv/wallet-toolbox`

---

## 🎯 Executive Summary

### wallet-toolbox-rs
- **Purpose**: Rust port of TypeScript wallet-toolbox with "perfect functional parity"
- **Architecture**: Multi-crate workspace (library-based, not HTTP server)
- **Interface**: FFI (C API) and WASM (JavaScript/TypeScript) - **NOT HTTP endpoints**
- **Status**: 95% complete, compiling successfully
- **Structure**: Modular crates for core, storage, services, client bindings

### Our rust-wallet
- **Purpose**: HTTP server exposing BRC-100 endpoints for CEF C++ backend
- **Architecture**: Single binary crate (actix-web HTTP server)
- **Interface**: HTTP POST endpoints on port 3301
- **Status**: Groups A & B complete, Group C in progress
- **Structure**: Flat structure with handlers, crypto, transaction modules

### Key Differences
| Aspect | wallet-toolbox-rs | Our rust-wallet |
|--------|------------------|-----------------|
| **Interface** | FFI/WASM library | HTTP server |
| **Calling Pattern** | Direct function calls | HTTP POST requests |
| **Storage** | Abstracted (SQLite, MySQL, IndexedDB) | JSON file (wallet.json) |
| **Authentication** | Built-in auth manager | HTTP session-based |
| **Transaction Building** | Full SDK with storage | Direct UTXO fetching + action storage |

---

## 📊 BRC-100 Method Coverage by Group

### Group A: Core Identity & Authentication ✅

| Method | Call Code | wallet-toolbox-rs | Our rust-wallet | Notes |
|--------|-----------|-------------------|-----------------|-------|
| `getVersion` | 28 | ✅ `blockchain_queries.rs` | ✅ `handlers.rs` | Both complete |
| `getPublicKey` | 8 | ✅ Via key deriver | ✅ `handlers.rs` | Both complete |
| `isAuthenticated` | 23 | ✅ Auth manager | ✅ `handlers.rs` | Both complete |
| `createHmac` | 13 | ✅ `hmac_operations.rs` | ✅ `handlers.rs` | Both complete |
| `verifyHmac` | 14 | ✅ `hmac_operations.rs` | ✅ `handlers.rs` | Both complete |
| `createSignature` | 15 | ✅ `signature_operations.rs` | ✅ `handlers.rs` | Both complete |
| `verifySignature` | 16 | ✅ `signature_operations.rs` | ✅ `handlers.rs` | Both complete |
| `/.well-known/auth` | - | ❌ Not HTTP-based | ✅ `handlers.rs` | Ours only (HTTP-specific) |

**Status**: ✅ **Both implementations complete for Group A**

---

### Group B: Transaction Operations ✅

| Method | Call Code | wallet-toolbox-rs | Our rust-wallet | Notes |
|--------|-----------|-------------------|-----------------|-------|
| `createAction` | 1 | ✅ `create_action.rs` (1915+ lines) | ✅ `handlers.rs` | Both complete |
| `signAction` | 2 | ✅ `sign_action.rs` (434+ lines) | ✅ `handlers.rs` | Both complete |
| `abortAction` | 3 | ✅ `list_actions.rs` | ✅ `handlers.rs` | Both complete |
| `listActions` | 4 | ✅ `list_actions.rs` | ✅ `handlers.rs` | Both complete |
| `internalizeAction` | 5 | ✅ `internalize_action.rs` | ✅ `handlers.rs` | Both complete |

**Status**: ✅ **Both implementations complete for Group B**

**Key Differences**:
- **wallet-toolbox-rs**: Uses storage layer (SQLite/MySQL) for action history, UTXO management
- **Our rust-wallet**: Uses JSON file (`wallet.json`) and direct WhatsOnChain API calls

---

### Group C: Output/Basket & Certificate Management ❌

| Method | Call Code | wallet-toolbox-rs | Our rust-wallet | Notes |
|--------|-----------|-------------------|-----------------|-------|
| `listOutputs` | 6 | ✅ `list_outputs.rs` (278 lines) | ❌ Not started | **wallet-toolbox-rs has full implementation** |
| `relinquishOutput` | 7 | ✅ `output_management.rs` | ❌ Not started | **wallet-toolbox-rs has implementation** |
| `acquireCertificate` | 17 | ✅ `signer/methods/acquire_direct_certificate.rs` | ❌ Not started | **wallet-toolbox-rs has implementation** |
| `listCertificates` | 18 | ✅ `storage/methods/list_certificates.rs` | ❌ Not started | **wallet-toolbox-rs has implementation** |
| `proveCertificate` | 19 | ✅ `signer/methods/prove_certificate.rs` | ❌ Not started | **wallet-toolbox-rs has implementation** |
| `relinquishCertificate` | 20 | ❓ Likely in output_management | ❌ Not started | Need to verify |
| `discoverByIdentityKey` | 21 | ❌ Not found | ❌ Not started | Both missing |
| `discoverByAttributes` | 22 | ❌ Not found | ❌ Not started | Both missing |
| `waitForAuthentication` | 24 | ❌ Not found | ❌ Not started | Both missing |
| `getHeight` | 25 | ⚠️ `blockchain_queries.rs` (stubbed) | ❌ Not started | Both incomplete |
| `getHeaderForHeight` | 26 | ⚠️ `blockchain_queries.rs` (stubbed) | ❌ Not started | Both incomplete |
| `getNetwork` | 27 | ✅ `blockchain_queries.rs` | ❌ Not started | wallet-toolbox-rs complete |

**Status**: ⚠️ **wallet-toolbox-rs has significant implementation for Group C, but not all methods complete**

**Key Finding**: **wallet-toolbox-rs has `listOutputs` and `relinquishOutput` implemented!** We can reference these.

---

### Group D: Encryption & Advanced Crypto ❌

| Method | Call Code | wallet-toolbox-rs | Our rust-wallet | Notes |
|--------|-----------|-------------------|-----------------|-------|
| `revealCounterpartyKeyLinkage` | 9 | ✅ `key_linkage.rs` | ❌ Not started | **wallet-toolbox-rs has implementation** |
| `revealSpecificKeyLinkage` | 10 | ✅ `key_linkage.rs` | ❌ Not started | **wallet-toolbox-rs has implementation** |
| `encrypt` | 11 | ✅ `encrypt_decrypt.rs` | ❌ Not started | **wallet-toolbox-rs has implementation** |
| `decrypt` | 12 | ✅ `encrypt_decrypt.rs` | ❌ Not started | **wallet-toolbox-rs has implementation** |

**Status**: ✅ **wallet-toolbox-rs has all Group D methods implemented**

---

## 🏗️ Architecture Comparison

### wallet-toolbox-rs Structure

```
crates/
├── wallet-core/           # Core wallet logic (8,500+ lines)
│   ├── methods/           # BRC-100 method implementations
│   │   ├── create_action.rs
│   │   ├── sign_action.rs
│   │   ├── list_outputs.rs  ✅ Group C
│   │   ├── output_management.rs  ✅ Group C
│   │   ├── encrypt_decrypt.rs  ✅ Group D
│   │   ├── key_linkage.rs  ✅ Group D
│   │   ├── hmac_operations.rs
│   │   ├── signature_operations.rs
│   │   └── blockchain_queries.rs
│   ├── managers/           # Wallet management
│   │   ├── wallet_auth_manager.rs
│   │   ├── wallet_permissions_manager/
│   │   └── wallet_settings_manager.rs
│   ├── signer/             # Certificate signing
│   │   └── methods/
│   │       ├── acquire_direct_certificate.rs  ✅ Group C
│   │       └── prove_certificate.rs  ✅ Group C
│   ├── crypto/             # Cryptographic operations
│   ├── keys/                # Key derivation (BRC-42, BRC-43)
│   └── transaction/        # Transaction building
├── wallet-storage/         # Storage abstraction (2,000+ lines)
│   ├── methods/            # Storage operations
│   │   ├── list_outputs_spec_op.rs
│   │   ├── list_certificates.rs  ✅ Group C
│   │   └── ...
│   └── schema/             # Database schema
├── wallet-services/        # External services
│   ├── utxo/               # UTXO fetching
│   ├── broadcaster/        # Transaction broadcasting
│   └── chaintracker/       # Blockchain queries
└── wallet-client/          # FFI bindings
```

### Our rust-wallet Structure

```
rust-wallet/src/
├── main.rs                 # Actix-web server setup
├── handlers.rs            # ALL BRC-100 endpoints (4000+ lines)
├── crypto/
│   ├── brc42.rs          # BRC-42 key derivation
│   ├── brc43.rs          # BRC-43 invoice numbers
│   ├── keys.rs           # Key operations
│   └── signing.rs        # ECDSA signing
├── transaction/
│   ├── types.rs          # Transaction structures
│   └── sighash.rs       # BSV ForkID SIGHASH
├── beef.rs               # BEEF format handling
├── utxo_fetcher.rs       # WhatsOnChain API
├── json_storage.rs       # wallet.json management
├── action_storage.rs     # Action history
└── domain_whitelist.rs   # Domain permissions
```

---

## 🔍 Code Similarity Analysis

### Likely Copied/Adapted Code

Based on structure and naming patterns:

1. **BRC-42 Implementation** (`crypto/brc42.rs`)
   - ✅ Similar structure to wallet-toolbox-rs `keys/brc42.rs`
   - ✅ Same test vectors used
   - ✅ ECDH shared secret computation matches

2. **BRC-43 Implementation** (`crypto/brc43.rs`)
   - ✅ Similar invoice number formatting
   - ✅ Security level enum matches

3. **Transaction Signing** (`crypto/signing.rs`)
   - ✅ Similar ECDSA signing approach
   - ✅ DER encoding with sighash type byte

4. **Key Derivation** (`crypto/keys.rs`)
   - ✅ Public key derivation matches patterns

### Our Original Code

1. **HTTP Handlers** (`handlers.rs`)
   - ✅ Completely original - wallet-toolbox-rs doesn't have HTTP endpoints
   - ✅ BRC-104 authentication flow (`.well-known/auth`)
   - ✅ Session management

2. **BEEF Handling** (`beef.rs`)
   - ✅ Original implementation for atomic BEEF
   - ✅ TSC to BUMP conversion

3. **UTXO Fetcher** (`utxo_fetcher.rs`)
   - ✅ Direct WhatsOnChain API integration
   - ✅ wallet-toolbox-rs uses abstracted service layer

4. **JSON Storage** (`json_storage.rs`)
   - ✅ Simple file-based storage
   - ✅ wallet-toolbox-rs uses SQLite/MySQL/IndexedDB

---

## ⚠️ Build Warnings Analysis

### Source Identification

#### 1. **Unused Imports** (Likely from Copied Code)
- `Scalar` in `brc42.rs` - imported but never used
- Multiple unused imports in `crypto/mod.rs` - re-exports that aren't used
- These are **safe to remove** - likely leftover from initial copy

#### 2. **Deprecated API Usage** (Our Code)
- `base64::encode` / `base64::decode` - deprecated in favor of `Engine`
- `Message::from_slice` - deprecated in favor of `from_digest_slice`
- **These are in our `handlers.rs`** - need to update to new API
- **Location**: `handlers.rs` (lines 132, 168, 419, 431, 453, 698, 748, 2144)

#### 3. **Unused Code** (Mixed)
- **Error variants never constructed** - likely from copied error types
- **Functions never used** - could be copied utilities or future-proofing
- **Fields never read** - struct definitions that might be used later

### Recommendations

1. **Safe to Remove** (Unused imports):
   ```rust
   // src/crypto/brc42.rs:9
   use secp256k1::{Secp256k1, SecretKey, PublicKey}; // Remove Scalar

   // src/crypto/mod.rs:11-22
   // Remove unused re-exports if they're truly not used
   ```

2. **Must Fix** (Deprecated APIs):
   ```rust
   // Replace base64::encode with base64::engine::general_purpose::STANDARD.encode()
   // Replace base64::decode with base64::engine::general_purpose::STANDARD.decode()
   // Replace Message::from_slice with Message::from_digest_slice
   ```

3. **Keep for Now** (Future-proofing):
   - Unused error variants might be needed for error handling
   - Unused functions might be used by future features
   - Unused struct fields might be needed for API compatibility

---

## 📚 Methods Available in wallet-toolbox-rs for Reference

### Group C Methods (We Need Next)

1. **`listOutputs`** ✅
   - File: `crates/wallet-core/src/methods/list_outputs.rs`
   - 278 lines, well-documented
   - Uses storage layer for basket/tag filtering
   - **Can adapt to our JSON storage approach**

2. **`relinquishOutput`** ✅
   - File: `crates/wallet-core/src/methods/output_management.rs`
   - **Can reference for implementation**

3. **Certificate Methods** ✅
   - `acquireCertificate`: `crates/wallet-core/src/signer/methods/acquire_direct_certificate.rs`
   - `proveCertificate`: `crates/wallet-core/src/signer/methods/prove_certificate.rs`
   - `listCertificates`: `crates/wallet-storage/src/methods/list_certificates.rs`

### Group D Methods (Advanced)

1. **`encrypt` / `decrypt`** ✅
   - File: `crates/wallet-core/src/methods/encrypt_decrypt.rs`
   - BRC-2 encryption implementation

2. **Key Linkage** ✅
   - File: `crates/wallet-core/src/methods/key_linkage.rs`
   - BRC-69 key linkage revelation

---

## 🎯 How wallet-toolbox-rs Functions Are Called

### **NOT HTTP Endpoints!**

wallet-toolbox-rs is a **library**, not an HTTP server. Functions are called via:

1. **FFI (Foreign Function Interface)** - C API
   - For native desktop applications
   - See `API_FFI.md`
   - Functions like `wallet_create_action()`, `wallet_sign_action()`

2. **WASM (WebAssembly)** - JavaScript/TypeScript API
   - For web applications
   - See `API_WASM.md`
   - `WalletWeb` class with methods like `createAction()`, `signAction()`

3. **Direct Rust API** - Library functions
   - For other Rust applications
   - Direct function calls like `create_action()`, `sign_action()`

### Comparison with Our Approach

| Aspect | wallet-toolbox-rs | Our rust-wallet |
|--------|-------------------|-----------------|
| **Entry Point** | Library function | HTTP POST endpoint |
| **Call Pattern** | `wallet.createAction(args)` | `POST /createAction` |
| **Transport** | Direct function call | HTTP request/response |
| **Use Case** | Embedded in apps | Standalone service |

---

## 💡 Recommendations

### 1. For Group C Implementation

**Reference wallet-toolbox-rs implementations**:
- ✅ `list_outputs.rs` - Full implementation with filtering
- ✅ `output_management.rs` - Output relinquishing
- ✅ Certificate methods - Certificate management

**Adaptation needed**:
- Replace storage layer calls with our JSON storage
- Replace database queries with in-memory filtering
- Keep HTTP endpoint structure

### 2. For Code Cleanup

**Immediate fixes**:
1. Fix deprecated `base64` API calls (7 instances in `handlers.rs`)
2. Fix deprecated `Message::from_slice` calls (5 instances)
3. Remove unused `Scalar` import from `brc42.rs`

**Safe removals** (after verification):
- Unused imports in `crypto/mod.rs` if truly unused
- Unused error variants if not needed

**Keep for now**:
- Unused functions that might be needed later
- Unused struct fields for API compatibility

### 3. For Testing

**wallet-toolbox-rs tests**:
- Friend wrote tests themselves
- **Not reliable for real-world validation**
- **Must test all code against real BRC-100 apps**

**Our approach**:
- ✅ Already testing with ToolBSV
- ✅ Real-world payment testing
- Continue real-world testing for all new methods

---

## 📝 Summary

### What wallet-toolbox-rs Has That We Don't

1. **Group C**: `listOutputs`, `relinquishOutput`, certificate methods
2. **Group D**: Full encryption and key linkage implementations
3. **Storage Layer**: Abstracted database storage (SQLite/MySQL)
4. **Modular Architecture**: Separate crates for different concerns

### What We Have That wallet-toolbox-rs Doesn't

1. **HTTP Server**: BRC-100 HTTP endpoints
2. **BRC-104 Auth**: `.well-known/auth` endpoint
3. **Session Management**: HTTP session-based authentication
4. **Simplified Storage**: JSON file-based (easier for our use case)
5. **Direct UTXO Fetching**: WhatsOnChain API integration

### Next Steps

1. ✅ **Reference wallet-toolbox-rs for Group C** - Use as implementation guide
2. ✅ **Fix deprecated API warnings** - Update base64 and secp256k1 calls
3. ✅ **Clean up unused imports** - Remove truly unused items
4. ✅ **Start Group C implementation** - Use wallet-toolbox-rs as reference

---

**Last Updated**: 2025-01-XX
**Next Review**: After Group C implementation



