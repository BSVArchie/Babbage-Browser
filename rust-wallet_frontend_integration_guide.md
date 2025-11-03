# Rust Wallet Frontend Integration Guide

> **Goal**: Connect the React frontend wallet UI to the Rust wallet backend, replacing the Go wallet integration.

---

## 📋 Table of Contents

1. [Current Architecture Overview](#current-architecture-overview)
2. [Frontend Functionality Analysis](#frontend-functionality-analysis)
3. [CEF Backend Bridge Analysis](#cef-backend-bridge-analysis)
4. [Go Wallet Endpoints (Current)](#go-wallet-endpoints-current)
5. [Rust Wallet Endpoints (Target)](#rust-wallet-endpoints-target)
6. [Integration Mapping](#integration-mapping)
7. [Implementation Tasks](#implementation-tasks)
8. [BEEF vs Raw Transactions](#beef-vs-raw-transactions)

---

## 🏗️ Current Architecture Overview

### System Flow:
```
React Frontend (TypeScript)
    ↓
window.bitcoinBrowser.wallet / address API
    ↓
CEF Message System (CefProcessMessage)
    ↓
C++ WalletService (WinHTTP)
    ↓
Wallet Backend (Go or Rust on port 3301)
```

### Key Components:

1. **Frontend** (`frontend/src/`)
   - `hooks/useBalance.ts` - Balance fetching
   - `hooks/useAddress.ts` - Address generation
   - `hooks/useTransaction.ts` - Transaction sending
   - `bridge/initWindowBridge.ts` - CEF message bridge setup

2. **CEF Backend** (`cef-native/src/`)
   - `core/WalletService.cpp` - HTTP client to wallet backend
   - `handlers/simple_handler.cpp` - CEF message routing
   - Routes messages from frontend → HTTP requests to wallet

3. **Wallet Backends**
   - **Go Wallet** (`go-wallet/main.go`) - Currently used
   - **Rust Wallet** (`rust-wallet/src/main.rs`) - Target implementation

---

## 🎨 Frontend Functionality Analysis

### **1. Get Balance** ✅ **IMPLEMENTED**

**Frontend Hook:** `frontend/src/hooks/useBalance.ts`

**Flow:**
```
useBalance.fetchBalance()
  → window.bitcoinBrowser.wallet.getBalance()
  → CEF: "wallet_get_balance" message
  → WalletService::getBalance()
  → GET /wallet/balance
```

**Frontend Call:**
```typescript
const response = await window.bitcoinBrowser.wallet.getBalance();
// Expected: { balance: number } (satoshis)
```

**Used In:**
- `BalanceDisplay.tsx` - Shows BSV balance and USD value
- `WalletPanelContent.tsx` - Main wallet panel
- Auto-refreshes every 30 seconds (currently disabled for debugging)

---

### **2. Generate Address (Receive)** ✅ **IMPLEMENTED**

**Frontend Hook:** `frontend/src/hooks/useAddress.ts`

**Flow:**
```
useAddress.generateAddress()
  → window.bitcoinBrowser.address.generate()
  → CEF: "address_generate" message
  → WalletService::generateAddress()
  → POST /wallet/address/generate
```

**Frontend Call:**
```typescript
const response = await window.bitcoinBrowser.address.generate();
// Expected: { address: string }
```

**Used In:**
- `WalletPanelContent.tsx` - "Receive" button
- Generates new address and copies to clipboard

---

### **3. Send Transaction** ✅ **IMPLEMENTED**

**Frontend Hook:** `frontend/src/hooks/useTransaction.ts`

**Flow:**
```
useTransaction.sendTransaction()
  → window.bitcoinBrowser.wallet.sendTransaction({ toAddress, amount, feeRate })
  → CEF: "wallet_send_transaction" message
  → WalletService::sendTransaction()
  → POST /transaction/send
```

**Frontend Call:**
```typescript
const response = await window.bitcoinBrowser.wallet.sendTransaction({
  toAddress: string,    // Recipient address
  amount: number,       // Satoshis (converted from BSV in form)
  feeRate: number       // Satoshis per byte (⚠️ Currently sent but NOT used by wallet!)
});
// Expected: { success: true, txid: string, message: string }
```

**⚠️ Fee Rate Issue:**
- Frontend **IS** sending `feeRate` (default: 5, user can select Low/Medium/High: 1/5/10)
- **BUT** rust-wallet's `createAction` uses a **hardcoded fee** of 5000 satoshis (line 1654)
- The wallet is **NOT** using the `feeRate` parameter from the frontend!
- This needs to be fixed to either:
  - Use the `feeRate` parameter to calculate dynamic fees, OR
  - Remove `feeRate` from frontend and let wallet calculate fees automatically

**Used In:**
- `TransactionForm.tsx` - Send transaction form
- `WalletPanelContent.tsx` - "Send" button

---

### **Other Frontend Features (Placeholders)**

These buttons exist but are not yet implemented:
- Transaction History - Placeholder
- Settings - Placeholder
- Other navigation items - Placeholders

---

## 🔌 CEF Backend Bridge Analysis

### **Message Routing** (`simple_handler.cpp`)

The CEF handler receives messages from the frontend and routes them to `WalletService`:

**1. Address Generation:**
```cpp
if (message_name == "address_generate") {
    WalletService walletService;
    nlohmann::json addressData = walletService.generateAddress();
    // Send response back to frontend
}
```

**2. Transaction Sending:**
```cpp
if (message_name == "send_transaction") {
    // Parse transaction data
    WalletService walletService;
    nlohmann::json result = walletService.sendTransaction(transactionData);
    // Send response back
}
```

**3. Balance Fetching:**
```cpp
// Handled via WalletService::getBalance()
// Called from window.bitcoinBrowser.wallet.getBalance()
```

### **WalletService HTTP Client** (`WalletService.cpp`)

All methods use `makeHttpRequest()` to call the wallet backend:

```cpp
nlohmann::json WalletService::getBalance(...) {
    return makeHttpRequest("GET", "/wallet/balance", "");
}

nlohmann::json WalletService::generateAddress() {
    return makeHttpRequest("POST", "/wallet/address/generate", "");
}

nlohmann::json WalletService::sendTransaction(...) {
    return makeHttpRequest("POST", "/transaction/send", transactionData.dump());
}
```

**Base URL:** `http://localhost:3301` (hardcoded in constructor)

---

## 🔵 Go Wallet Endpoints (Current Implementation)

### **1. Get Balance**
**Endpoint:** `GET /wallet/balance`

**Request:**
```
GET /wallet/balance
```

**Response:**
```json
{
  "balance": 1000000  // Total satoshis across all addresses
}
```

**Implementation:** `go-wallet/main.go` (lines 572-590)
- Calls `walletService.walletManager.GetTotalBalance()`
- Fetches UTXOs from WhatsOnChain for all addresses
- Sums all UTXO values

---

### **2. Generate Address**
**Endpoint:** `POST /wallet/address/generate`

**Request:**
```
POST /wallet/address/generate
(empty body)
```

**Response:**
```json
{
  "address": "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa",
  "index": 0,
  "publicKey": "02..."
}
```

**Implementation:** `go-wallet/main.go` (lines 532-553)
- Calls `walletService.walletManager.GetNextAddress()`
- Generates new HD wallet address
- Saves wallet after generating

---

### **3. Send Transaction**
**Endpoint:** `POST /transaction/send`

**Request:**
```json
{
  "toAddress": "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa",
  "amount": 100000,      // Satoshis
  "feeRate": 5           // Satoshis per byte (⚠️ Currently ignored - wallet uses hardcoded 5000 sats)
}
```

**Response:**
```json
{
  "success": true,
  "txid": "7dce601f...",
  "whatsOnChainUrl": "https://whatsonchain.com/tx/7dce601f...",
  "message": "Transaction sent successfully"
}
```

**Implementation:** `go-wallet/main.go` (lines 638-692)
- Creates unsigned transaction (`CreateTransaction`)
- Signs transaction (`SignTransaction`)
- Broadcasts to miners (`BroadcastTransaction`)
- **Returns raw transaction hex** (not BEEF)
- **Uses feeRate parameter** to calculate fees dynamically

---

## 🦀 Rust Wallet Endpoints (Target Implementation)

### **1. Get Balance** ⚠️ **NEEDS IMPLEMENTATION**

**Endpoint:** `GET /wallet/balance`

**Current Implementation:** `rust-wallet/src/handlers.rs` (lines 915-933)
```rust
pub async fn wallet_balance(state: web::Data<AppState>) -> HttpResponse {
    // ❌ Currently returns hardcoded 0!
    HttpResponse::Ok().json(serde_json::json!({
        "balance": 0,
        "addresses": addresses.len()
    }))
}
```

**Status:** ❌ **STUB - Returns 0**

**What It Should Do:**
1. Get all addresses from `JsonStorage`
2. Fetch UTXOs for each address using `utxo_fetcher::fetch_all_utxos()`
3. Sum all UTXO satoshi values
4. Return total balance

**Available Functionality:**
- ✅ `utxo_fetcher::fetch_all_utxos()` exists (lines 123-145)
- ✅ `storage.get_all_addresses()` exists
- ❌ Need to integrate UTXO fetching into balance endpoint

---

### **2. Generate Address** ⚠️ **NEEDS IMPLEMENTATION**

**Endpoint:** `POST /wallet/address/generate`

**Current Implementation:** `rust-wallet/src/handlers.rs` (lines 3005-3007)
```rust
pub async fn generate_address() -> HttpResponse {
    HttpResponse::Ok().json(serde_json::json!({"message": "Not implemented"}))
}
```

**Status:** ❌ **STUB - Returns "Not implemented"**

**What It Should Do:**
1. Get current wallet from `JsonStorage`
2. Get master private key and master public key
3. **Use BRC-42/BRC-43** to derive child public key (NOT BIP44!):
   - Counterparty: Our own master public key (self-derivation)
   - Invoice number: BRC-43 format `"2-receive address-{current_index}"`
   - Derive: `derive_child_public_key(master_privkey, master_pubkey, invoice_number)`
4. Convert derived public key to P2PKH address
5. Add address to wallet addresses array
6. Increment `current_index`
7. Save wallet to file
8. Return address info

**Available Functionality:**
- ✅ `JsonStorage` has address management
- ✅ BRC-42 key derivation exists (`crypto/brc42.rs`)
- ✅ BRC-43 invoice number formatting exists (`crypto/brc43.rs`)
- ✅ P2PKH script generation from public key (used in BRC-29)
- ❌ Need to implement address generation logic with BRC-42/BRC-43

**Reference Implementation:**
- See `createAction` BRC-29 payment logic (lines 1723-1789) for BRC-42 derivation pattern
- Similar approach but with our own public key as counterparty

---

### **3. Send Transaction** ⚠️ **NEEDS IMPLEMENTATION**

**Endpoint:** `POST /transaction/send`

**Current Implementation:** `rust-wallet/src/handlers.rs` (lines 3009-3011)
```rust
pub async fn send_transaction() -> HttpResponse {
    HttpResponse::Ok().json(serde_json::json!({"message": "Not implemented"}))
}
```

**Status:** ❌ **STUB - Returns "Not implemented"**

**What It Should Do:**
1. Accept `{ toAddress, amount, feeRate }`
2. **Use `feeRate` to calculate dynamic fees** (currently hardcoded to 5000 sats!)
3. Fetch UTXOs for all wallet addresses
4. Select UTXOs to cover amount + fees
5. Create unsigned transaction
6. Sign transaction (BSV ForkID SIGHASH)
7. Broadcast to miners (WhatsOnChain + GorillaPool)
8. Return TXID and success status

**⚠️ Fee Calculation Issue:**
- Current `createAction` uses hardcoded fee: `let estimated_fee = 5000;` (line 1654)
- Should calculate: `fee = estimated_tx_size_bytes * feeRate`
- Or use `feeRate` parameter to calculate dynamic fees based on transaction size

**Available Functionality:**
- ✅ `createAction` - Full transaction creation with UTXO selection
- ✅ `signAction` - BSV ForkID SIGHASH signing
- ✅ `processAction` - Complete flow (create + sign + broadcast)
- ✅ BEEF generation with TSC Merkle proofs
- ❌ Need to create wrapper endpoint for simple P2PKH sends

**Key Question:** Should we return **raw transaction** or **BEEF**?
- Go wallet returns raw hex
- Rust wallet has full BEEF support
- See [BEEF vs Raw Transactions](#beef-vs-raw-transactions) section

---

## 🔗 Integration Mapping

### **Mapping Table:**

| Frontend Function | CEF Message | CEF Method | Go Endpoint | Rust Endpoint | Rust Status |
|-------------------|-------------|-----------|-------------|---------------|-------------|
| `getBalance()` | `wallet_get_balance` | `WalletService::getBalance()` | `GET /wallet/balance` | `GET /wallet/balance` | ❌ Stub (returns 0) |
| `address.generate()` | `address_generate` | `WalletService::generateAddress()` | `POST /wallet/address/generate` | `POST /wallet/address/generate` | ❌ Stub (not implemented) |
| `sendTransaction()` | `wallet_send_transaction` | `WalletService::sendTransaction()` | `POST /transaction/send` | `POST /transaction/send` | ❌ Stub (not implemented) |

---

### **Frontend → CEF → Wallet Flow:**

```
┌─────────────────┐
│  React Frontend │
│                 │
│ useBalance.ts   │──┐
│ useAddress.ts   │  │
│ useTransaction  │  │
└─────────────────┘  │
                     │ window.bitcoinBrowser.wallet
                     ▼
┌─────────────────────────────────────────┐
│  CEF Message Bridge                     │
│  (initWindowBridge.ts)                  │
│                                         │
│  wallet.getBalance()                    │
│  address.generate()                     │
│  wallet.sendTransaction()               │
└─────────────────────────────────────────┘
                     │ CefProcessMessage
                     ▼
┌─────────────────────────────────────────┐
│  C++ WalletService                      │
│  (WalletService.cpp)                    │
│                                         │
│  getBalance()                           │
│  generateAddress()                      │
│  sendTransaction()                      │
└─────────────────────────────────────────┘
                     │ WinHTTP
                     │ http://localhost:3301
                     ▼
┌─────────────────────────────────────────┐
│  Rust Wallet Backend                    │
│  (rust-wallet/src/main.rs)              │
│                                         │
│  GET  /wallet/balance      ❌ Stub      │
│  POST /wallet/address/...  ❌ Stub      │
│  POST /transaction/send    ❌ Stub      │
└─────────────────────────────────────────┘
```

---

## ✅ Implementation Tasks

### **Task 1: Implement `/wallet/balance` Endpoint**

**File:** `rust-wallet/src/handlers.rs`

**Current Code:**
```rust
pub async fn wallet_balance(state: web::Data<AppState>) -> HttpResponse {
    let storage = state.storage.lock().unwrap();
    match storage.get_all_addresses() {
        Ok(addresses) => {
            HttpResponse::Ok().json(serde_json::json!({
                "balance": 0,  // ❌ Hardcoded!
                "addresses": addresses.len()
            }))
        }
        Err(e) => { /* error handling */ }
    }
}
```

**Required Changes:**
1. Import `utxo_fetcher` module
2. Call `fetch_all_utxos(addresses)` to get UTXOs
3. Sum all UTXO satoshi values
4. Return total balance

**Implementation:**
```rust
use crate::utxo_fetcher::fetch_all_utxos;

pub async fn wallet_balance(state: web::Data<AppState>) -> HttpResponse {
    let storage = state.storage.lock().unwrap();

    match storage.get_all_addresses() {
        Ok(addresses) => {
            // Fetch UTXOs for all addresses
            match fetch_all_utxos(addresses).await {
                Ok(utxos) => {
                    let total_balance: i64 = utxos.iter().map(|u| u.satoshis).sum();

                    log::info!("💰 Total balance: {} satoshis ({} UTXOs)", total_balance, utxos.len());

                    HttpResponse::Ok().json(serde_json::json!({
                        "balance": total_balance
                    }))
                }
                Err(e) => {
                    log::error!("   Failed to fetch UTXOs: {}", e);
                    HttpResponse::InternalServerError().json(serde_json::json!({
                        "error": e
                    }))
                }
            }
        }
        Err(e) => {
            log::error!("   Failed to get addresses: {}", e);
            HttpResponse::InternalServerError().json(serde_json::json!({
                "error": e
            }))
        }
    }
}
```

**Note:** This endpoint should **ONLY** be callable from the wallet UI, not from external apps. The endpoint is already on a different route (`/wallet/balance`) than BRC-100 endpoints, so it's protected by default.

---

### **Task 2: Implement `/wallet/address/generate` Endpoint**

**File:** `rust-wallet/src/handlers.rs`

**Current Code:**
```rust
pub async fn generate_address() -> HttpResponse {
    HttpResponse::Ok().json(serde_json::json!({"message": "Not implemented"}))
}
```

**Required Changes:**
1. Get wallet state from `AppState`
2. Get master private key and master public key
3. **Use BRC-42/BRC-43** to derive child public key:
   - Counterparty: Our own master public key
   - Invoice number: BRC-43 format `"2-receive address-{current_index}"`
   - Derive: `derive_child_public_key(master_privkey, master_pubkey, invoice_number)`
4. Convert derived public key to P2PKH address
5. Add address to wallet addresses array
6. Increment `current_index`
7. Save wallet to file
8. Return address info

**Implementation Notes:**
- **DO NOT use BIP44** - use BRC-42/BRC-43 instead!
- See `createAction` BRC-29 payment logic (lines 1723-1789) for reference
- Use `crypto::brc42::derive_child_public_key()`
- Use `crypto::brc43::InvoiceNumber::new()` for invoice number
- Use existing P2PKH script generation (same as BRC-29 payments)
- Similar to BRC-29 but with our own public key as counterparty

**Reference:**
- BRC-29 payment derivation: `rust-wallet/src/handlers.rs` lines 1774-1789
- BRC-42 implementation: `rust-wallet/src/crypto/brc42.rs`
- BRC-43 implementation: `rust-wallet/src/crypto/brc43.rs`

---

### **Task 3: Implement `/transaction/send` Endpoint**

**File:** `rust-wallet/src/handlers.rs`

**Current Code:**
```rust
pub async fn send_transaction() -> HttpResponse {
    HttpResponse::Ok().json(serde_json::json!({"message": "Not implemented"}))
}
```

**Request Structure:**
```rust
#[derive(Debug, Deserialize)]
pub struct SendTransactionRequest {
    #[serde(rename = "toAddress")]
    pub to_address: String,
    pub amount: i64,      // Satoshis
    #[serde(rename = "feeRate")]
    pub fee_rate: i64,   // Satoshis per byte
}
```

**Required Changes:**
1. Parse request body
2. Create BRC-100 `createAction` request with single output
3. Call `processAction` internally (or call `createAction` + `signAction` + broadcast)
4. Return response in Go wallet format

**Implementation Options:**

**Option A: Use `processAction` internally**
```rust
pub async fn send_transaction(
    state: web::Data<AppState>,
    body: web::Bytes,
) -> HttpResponse {
    // Parse request
    let req: SendTransactionRequest = serde_json::from_slice(&body)?;

    // ⚠️ TODO: Pass feeRate to createAction somehow
    // Current createAction ignores feeRate and uses hardcoded 5000 sats

    // Create BRC-100 createAction request
    let create_action_req = serde_json::json!({
        "description": "Simple P2PKH send",
        "outputs": [{
            "satoshis": req.amount,
            "address": req.to_address  // Can use address instead of script
        }]
    });

    // Call processAction internally
    let process_result = process_action(state, web::Bytes::from(create_action_req.to_string())).await;

    // Convert to Go wallet response format
    // ...
}
```

**⚠️ Fee Rate Handling:**
- `createAction` currently ignores feeRate and uses hardcoded 5000 satoshis
- Need to either:
  1. Add `feeRate` option to `CreateActionOptions`, OR
  2. Calculate fee dynamically: `fee = estimated_tx_size * feeRate`

**Option B: Reuse existing transaction creation logic**
- Extract UTXO selection logic from `createAction`
- Extract signing logic from `signAction`
- Extract broadcasting logic
- Combine into single endpoint

**Response Format:**
```json
{
  "success": true,
  "txid": "7dce601f...",
  "whatsOnChainUrl": "https://whatsonchain.com/tx/7dce601f...",
  "message": "Transaction sent successfully"
}
```

**BEEF Question:** Should we return raw transaction hex (like Go wallet) or BEEF?
- See [BEEF vs Raw Transactions](#beef-vs-raw-transactions) section

---

## 🔐 Security Considerations

### **Private Endpoints (Wallet UI Only)**

The following endpoints should **NOT** be accessible to external BRC-100 apps:

- ✅ `GET /wallet/balance` - Only for wallet UI
- ✅ `POST /wallet/address/generate` - Only for wallet UI
- ✅ `POST /transaction/send` - Only for wallet UI (simple sends)

**Protection:**
- These endpoints are on `/wallet/*` and `/transaction/send` routes
- Different from BRC-100 standard endpoints (`/createAction`, `/signAction`, etc.)
- External apps should use BRC-100 endpoints (`/createAction`, `/processAction`)
- CEF backend calls these endpoints directly (not via HTTP interceptor)

**Future Enhancement:**
- Add authentication token system for wallet UI endpoints
- Or restrict to `localhost` origin only

---

## 📦 BEEF vs Raw Transactions

### **Question: Should `/transaction/send` return BEEF or raw hex?**

**Current Go Wallet:**
- Returns raw transaction hex
- Broadcasts directly to miners
- No SPV proofs needed for simple sends

**Rust Wallet Capabilities:**
- Full BEEF (BRC-62) support with parent transactions
- TSC Merkle proofs (BRC-67 SPV)
- Atomic BEEF (BRC-95) format
- All implemented in `createAction` / `signAction`

### **Recommendation: Return Raw Transaction (for now)**

**Rationale:**
1. **Compatibility:** Frontend expects raw hex (like Go wallet)
2. **Simplicity:** Simple sends don't need SPV proofs
3. **Performance:** Smaller response size
4. **Future:** Can add BEEF support later if needed

**Implementation:**
```rust
// After signing, extract raw transaction from BEEF
let raw_tx_hex = extract_raw_tx_from_beef(&atomic_beef)?;

// Return raw hex (same format as Go wallet)
HttpResponse::Ok().json(serde_json::json!({
    "success": true,
    "txid": txid,
    "whatsOnChainUrl": format!("https://whatsonchain.com/tx/{}", txid),
    "message": "Transaction sent successfully"
}))
```

**Future Enhancement:**
- Add optional `beef: true` parameter to return BEEF format
- Store BEEF in action history for later SPV validation
- Return both raw hex and BEEF in response

---

## 🧪 Testing Plan

### **1. Test Get Balance**
```bash
# Start rust-wallet
cd rust-wallet
cargo run

# Test endpoint
curl http://localhost:3301/wallet/balance

# Expected: { "balance": <total_satoshis> }
```

### **2. Test Generate Address**
```bash
curl -X POST http://localhost:3301/wallet/address/generate

# Expected: { "address": "...", "index": 0, "publicKey": "..." }
```

### **3. Test Send Transaction**
```bash
curl -X POST http://localhost:3301/transaction/send \
  -H "Content-Type: application/json" \
  -d '{
    "toAddress": "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa",
    "amount": 10000,
    "feeRate": 5
  }'

# Expected: { "success": true, "txid": "...", ... }
```

### **4. Test Frontend Integration**
1. Start rust-wallet
2. Start CEF browser
3. Open wallet panel
4. Test:
   - Balance display updates
   - Receive button generates address
   - Send button creates transaction

---

## 📝 Summary

### **What Works:**
- ✅ Frontend UI components
- ✅ CEF message bridge
- ✅ WalletService HTTP client
- ✅ Rust wallet BRC-100 endpoints (`createAction`, `signAction`, etc.)
- ✅ UTXO fetching infrastructure

### **What Needs Implementation:**
- ❌ `/wallet/balance` - Currently returns 0, needs UTXO fetching
- ❌ `/wallet/address/generate` - Stub, needs BRC-42/BRC-43 address derivation
- ❌ `/transaction/send` - Stub, needs simple P2PKH send wrapper

### **Estimated Effort:**
- **Task 1 (Balance):** 1-2 hours (mostly integration)
- **Task 2 (Address):** 3-4 hours (BRC-42/BRC-43 derivation + wallet.json storage)
- **Task 3 (Send):** 3-5 hours (wrapper + feeRate calculation fix)
- **Bonus Task (FeeRate):** 1-2 hours (fix hardcoded fee in createAction)

**Total: ~8-13 hours**

---

## 🔄 Next Steps

1. **Fix Fee Rate Calculation** - Make `createAction` use `feeRate` parameter instead of hardcoded 5000 sats
2. **Implement `/wallet/balance`** - Fetch and sum UTXOs
3. **Implement `/wallet/address/generate`** - BRC-42/BRC-43 address derivation (NOT BIP44!)
4. **Implement `/transaction/send`** - Simple P2PKH send wrapper with feeRate support
5. **Test with frontend** - Verify all three functions work
6. **Consider BEEF support** - Add optional BEEF return format

---

**Last Updated:** 2025-01-XX
**Status:** Ready for implementation
**Priority:** High (blocks frontend wallet functionality)
