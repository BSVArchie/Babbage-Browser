# `/transaction/send` Implementation Plan

## Overview

Implement a simple P2PKH send endpoint that wraps the existing BRC-100 transaction infrastructure.

**Request Format (Simple):**
```json
{
  "toAddress": "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa",
  "amount": 100000,      // Satoshis
  "feeRate": 5           // Satoshis per byte (currently ignored - hardcoded fee)
}
```

**Response Format (Go Wallet Compatible):**
```json
{
  "success": true,
  "txid": "7dce601f...",
  "whatsOnChainUrl": "https://whatsonchain.com/tx/7dce601f...",
  "message": "Transaction sent successfully"
}
```

---

## Analysis: What We Already Have

### ✅ Existing Infrastructure

1. **Transaction Creation** (`createAction`)
   - ✅ UTXO fetching and selection
   - ✅ Transaction building
   - ✅ Change output generation
   - ⚠️ **Hardcoded fee**: 5000 satoshis (ignores feeRate)

2. **Transaction Signing** (`signAction`)
   - ✅ BSV ForkID SIGHASH
   - ✅ Multi-input signing
   - ✅ Atomic BEEF generation (BRC-95)
   - ✅ TSC Merkle proofs

3. **Transaction Broadcasting** (`broadcast_transaction`)
   - ✅ GorillaPool broadcaster
   - ✅ WhatsOnChain broadcaster
   - ✅ Multi-miner redundancy
   - ⚠️ **Expects raw hex**, but `signAction` returns **Atomic BEEF** (binary)

4. **Complete Flow** (`processAction`)
   - ✅ Calls `createAction` → `signAction` → broadcast
   - ✅ Handles all the complexity
   - ⚠️ **Different request/response format** (BRC-100 vs simple)

---

## Implementation Strategy

### **Option A: Reuse `processAction` Internally** ⭐ RECOMMENDED

**Pros:**
- ✅ Reuses existing, tested code
- ✅ Less code duplication
- ✅ Automatically gets all features (BEEF, SPV proofs, etc.)
- ✅ Easier to maintain

**Cons:**
- ⚠️ Need to convert request/response formats
- ⚠️ Need to extract raw tx from Atomic BEEF for broadcasting

**Steps:**
1. Parse simple request: `{ toAddress, amount, feeRate }`
2. Convert to `ProcessActionRequest`:
   - Create single output with `address` field
   - Set `broadcast: true`
3. Call `processAction` internally
4. Extract TXID and raw transaction from response
5. Convert Atomic BEEF to raw hex (if needed for broadcasting)
6. Broadcast transaction (if not already done by processAction)
7. Convert to Go wallet response format

---

### **Option B: Direct Function Calls**

**Pros:**
- ✅ More control over each step
- ✅ Can handle feeRate parameter
- ✅ Simpler response format

**Cons:**
- ❌ Code duplication
- ❌ More complex error handling
- ❌ Need to manage transaction state between calls

**Steps:**
1. Parse request
2. Call `createAction` logic directly
3. Call `signAction` logic directly
4. Extract raw transaction from BEEF
5. Call `broadcast_transaction`
6. Return Go wallet format

---

## Recommended Approach: Option A (with modifications)

### **Step 1: Request Parsing**

```rust
#[derive(Debug, Deserialize)]
pub struct SendTransactionRequest {
    #[serde(rename = "toAddress")]
    pub to_address: String,
    pub amount: i64,      // Satoshis
    #[serde(rename = "feeRate")]
    pub fee_rate: i64,    // Satoshis per byte (currently ignored)
}
```

### **Step 2: Convert to ProcessActionRequest**

```rust
// Create ProcessActionRequest with single output
let process_req = ProcessActionRequest {
    outputs: vec![CreateActionOutput {
        satoshis: Some(req.amount),
        address: Some(req.to_address),
        script: None,
        custom_instructions: None,
        output_description: Some("Simple P2PKH send".to_string()),
    }],
    description: Some("Simple wallet send".to_string()),
    labels: None,
    broadcast: Some(true),  // Always broadcast
};

// Convert to JSON body
let process_body = serde_json::to_vec(&process_req)?;
```

### **Step 3: Call processAction**

```rust
let process_response = process_action(state.clone(), web::Bytes::from(process_body)).await;
```

### **Step 4: Extract Response**

```rust
// Parse ProcessActionResponse
let process_json: ProcessActionResponse = match process_response.status().is_success() {
    true => {
        let body_bytes = actix_web::body::to_bytes(process_response.into_body()).await?;
        serde_json::from_slice(&body_bytes)?
    }
    false => {
        // Return error
    }
};

let txid = process_json.txid;
let raw_tx_beef = process_json.raw_tx;  // This is Atomic BEEF (binary)
```

### **Step 5: Extract Raw Transaction from BEEF**

**⚠️ CRITICAL:** `signAction` returns **Atomic BEEF** (BRC-95 format):
```
[4 bytes] 0x01 0x01 0x01 0x01  ← Magic prefix
[32 bytes] Subject TXID         ← Transaction being validated
[variable] Standard BEEF        ← Parent transactions + Main transaction
```

**We need to extract the raw transaction hex from the BEEF.**

**Options:**
1. **Parse BEEF** - Extract main transaction from BEEF structure
2. **Use BEEF directly** - Broadcast BEEF (if miners accept it)
3. **Store in signAction** - Modify `signAction` to also return raw hex

**Best Option:** Parse BEEF to extract raw transaction
- Check `rust-wallet/src/beef.rs` for BEEF parsing
- Extract the main transaction from the BEEF structure
- Convert to hex string

### **Step 6: Broadcast Transaction**

```rust
// Extract raw hex from BEEF
let raw_tx_hex = extract_raw_tx_from_beef(&raw_tx_beef)?;

// Broadcast (processAction may have already done this, but we'll do it explicitly)
match broadcast_transaction(&raw_tx_hex).await {
    Ok(_) => {
        log::info!("   ✅ Transaction broadcast successful");
    }
    Err(e) => {
        log::error!("   ⚠️ Broadcast error (may have already succeeded): {}", e);
        // Continue anyway - processAction may have already broadcast
    }
}
```

### **Step 7: Return Go Wallet Format**

```rust
HttpResponse::Ok().json(serde_json::json!({
    "success": true,
    "txid": txid,
    "whatsOnChainUrl": format!("https://whatsonchain.com/tx/{}", txid),
    "message": "Transaction sent successfully"
}))
```

---

## Challenges & Solutions

### **Challenge 1: BEEF vs Raw Hex**

**Problem:** `signAction` returns Atomic BEEF (binary), but `broadcast_transaction` expects raw hex.

**Solution:**
- Check if `beef.rs` has parsing functions
- Extract main transaction from BEEF
- Convert to hex string

**Alternative:** If BEEF parsing is complex, we could:
- Modify `signAction` to return both BEEF and raw hex
- Or: Call broadcast with BEEF (if miners accept it)

### **Challenge 2: Fee Rate**

**Problem:** `createAction` uses hardcoded 5000 satoshis fee, ignores `feeRate`.

**Solution (for now):**
- Leave feeRate in request (for future use)
- Accept that fee is hardcoded
- Document this limitation
- Add TODO for future enhancement

### **Challenge 3: Error Handling**

**Problem:** `processAction` may fail at different stages (create, sign, broadcast).

**Solution:**
- Check HTTP status code
- Parse error messages from BRC-100 responses
- Convert to user-friendly error messages
- Return appropriate HTTP status codes

### **Challenge 4: Double Broadcasting**

**Problem:** `processAction` may already broadcast, then we broadcast again.

**Solution:**
- Check if `processAction` actually broadcasts (it should with `broadcast: true`)
- If it does, skip our broadcast step
- Or: Let `processAction` handle all broadcasting

---

## Implementation Steps (Detailed)

### **Phase 1: Basic Implementation**

1. ✅ Add `SendTransactionRequest` struct
2. ✅ Implement request parsing
3. ✅ Convert to `ProcessActionRequest`
4. ✅ Call `processAction`
5. ✅ Extract TXID from response
6. ✅ Return basic Go wallet format (without URL)

**Test:** Simple send should work, but may not broadcast correctly.

---

### **Phase 2: BEEF Extraction**

1. ✅ Check `beef.rs` for parsing functions
2. ✅ Implement `extract_raw_tx_from_beef()` helper
3. ✅ Extract raw transaction from Atomic BEEF
4. ✅ Convert to hex string
5. ✅ Use for broadcasting

**Test:** Transaction should broadcast successfully.

---

### **Phase 3: Error Handling**

1. ✅ Handle `createAction` failures
2. ✅ Handle `signAction` failures
3. ✅ Handle broadcast failures
4. ✅ Return appropriate error messages

**Test:** Error cases should return clear messages.

---

### **Phase 4: Polish**

1. ✅ Add WhatsOnChain URL
2. ✅ Improve logging
3. ✅ Add validation (address format, amount > 0, etc.)
4. ✅ Update documentation

**Test:** Complete flow should match Go wallet behavior.

---

## Code Structure

```rust
// Request structure
#[derive(Debug, Deserialize)]
pub struct SendTransactionRequest {
    #[serde(rename = "toAddress")]
    pub to_address: String,
    pub amount: i64,
    #[serde(rename = "feeRate")]
    pub fee_rate: i64,  // Currently ignored
}

// Helper: Extract raw transaction from Atomic BEEF
fn extract_raw_tx_from_beef(beef_bytes: &[u8]) -> Result<String, String> {
    // Parse Atomic BEEF format
    // Extract main transaction
    // Return hex string
}

// Main endpoint
pub async fn send_transaction(
    state: web::Data<AppState>,
    body: web::Bytes,
) -> HttpResponse {
    // 1. Parse request
    // 2. Convert to ProcessActionRequest
    // 3. Call processAction
    // 4. Extract TXID and raw tx
    // 5. Extract raw hex from BEEF
    // 6. Broadcast (if needed)
    // 7. Return Go wallet format
}
```

---

## Testing Plan

### **Test 1: Basic Send**
```bash
curl -X POST http://localhost:3301/transaction/send \
  -H "Content-Type: application/json" \
  -d '{
    "toAddress": "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa",
    "amount": 10000,
    "feeRate": 5
  }'
```

**Expected:** `{ "success": true, "txid": "...", ... }`

### **Test 2: Insufficient Funds**
```bash
curl -X POST http://localhost:3301/transaction/send \
  -H "Content-Type: application/json" \
  -d '{
    "toAddress": "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa",
    "amount": 999999999999,
    "feeRate": 5
  }'
```

**Expected:** Error message about insufficient funds

### **Test 3: Invalid Address**
```bash
curl -X POST http://localhost:3301/transaction/send \
  -H "Content-Type: application/json" \
  -d '{
    "toAddress": "invalid",
    "amount": 10000,
    "feeRate": 5
  }'
```

**Expected:** Error message about invalid address

---

## Estimated Complexity

**Difficulty:** Medium

**Reasons:**
- ✅ Most infrastructure exists
- ⚠️ Need to handle BEEF format conversion
- ⚠️ Need to convert between request/response formats
- ✅ Error handling is straightforward

**Time Estimate:** 2-4 hours
- Phase 1 (Basic): 1 hour
- Phase 2 (BEEF): 1 hour
- Phase 3 (Errors): 30 minutes
- Phase 4 (Polish): 30 minutes

---

## Next Steps

1. **Review this plan** - Confirm approach
2. **Check BEEF parsing** - See what's available in `beef.rs`
3. **Implement Phase 1** - Basic send without broadcasting
4. **Test Phase 1** - Verify transaction creation and signing
5. **Implement Phase 2** - BEEF extraction and broadcasting
6. **Test Phase 2** - Verify complete flow
7. **Implement Phase 3 & 4** - Error handling and polish

---

**Last Updated:** 2025-01-XX
**Status:** Ready for implementation
**Priority:** High (completes frontend integration)
