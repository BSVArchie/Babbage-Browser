# `/transaction/send` Implementation Outline

## Summary

**Difficulty:** Medium (mostly integration, some BEEF parsing)

**Estimated Time:** 2-4 hours

**Approach:** Reuse `processAction` internally and convert formats

---

## What We Have ✅

1. **`processAction`** - Complete flow: create → sign → broadcast
2. **`beef.rs`** - Has `from_atomic_beef_bytes()` to parse Atomic BEEF
3. **`broadcast_transaction()`** - Broadcasts to GorillaPool + WhatsOnChain
4. **Transaction signing** - BSV ForkID SIGHASH working

---

## What We Need to Do

### **Step 1: Request Parsing** (15 min)
- Add `SendTransactionRequest` struct
- Parse `{ toAddress, amount, feeRate }`

### **Step 2: Convert to BRC-100 Format** (15 min)
- Create `ProcessActionRequest` with single output
- Set `address` field (not `script`)
- Set `broadcast: true`

### **Step 3: Call processAction** (10 min)
- Call `processAction` with converted request
- Extract `ProcessActionResponse`

### **Step 4: Get Raw Transaction** (15 min) ✅ SIMPLIFIED!

**Option A: Extract from BEEF** (if we use processAction)
- Parse Atomic BEEF using `Beef::from_atomic_beef_bytes()`
- Get main transaction: `beef.main_transaction()` or `beef.transactions.last()`
- Convert to hex string

**Option B: Store raw hex in signAction** ⭐ BETTER APPROACH
- Modify `SignActionResponse` to include `raw_tx_hex` field
- `signAction` already has `signed_tx_hex` available (line 2522)
- Return both BEEF and raw hex in response
- Use raw hex for broadcasting

**Option C: Call signAction logic directly** ⭐ SIMPLEST
- Extract transaction creation logic
- Call signing logic directly (get `signed_tx_hex` before BEEF wrapping)
- Use raw hex for broadcasting
- Skip BEEF entirely for simple sends (or create BEEF separately if needed)

### **Step 5: Broadcast Transaction** (15 min)
- Call `broadcast_transaction()` with raw hex
- Handle errors gracefully (may have already broadcast)

### **Step 6: Return Go Wallet Format** (10 min)
- Format: `{ success, txid, whatsOnChainUrl, message }`

### **Step 7: Error Handling** (30 min)
- Handle createAction errors
- Handle signAction errors
- Handle broadcast errors
- Return user-friendly messages

---

## Key Code Snippets

### **Extract Raw TX from Atomic BEEF:**
```rust
use crate::beef::Beef;

fn extract_raw_tx_from_atomic_beef(beef_hex: &str) -> Result<String, String> {
    // Decode hex to bytes
    let beef_bytes = hex::decode(beef_hex)
        .map_err(|e| format!("Invalid BEEF hex: {}", e))?;

    // Parse Atomic BEEF
    let (_txid, beef) = Beef::from_atomic_beef_bytes(&beef_bytes)
        .map_err(|e| format!("Failed to parse BEEF: {}", e))?;

    // Main transaction is the LAST one in the transactions array
    let main_tx = beef.main_transaction()
        .ok_or("No main transaction in BEEF")?;

    // Convert to hex
    Ok(hex::encode(main_tx))
}
```

**Alternative: Store Raw Hex in signAction** (Better for future)
- Modify `SignActionResponse` to include `raw_tx_hex: Option<String>`
- Return both BEEF and raw hex
- Use raw hex for broadcasting without parsing

### **Main Endpoint:**
```rust
pub async fn send_transaction(
    state: web::Data<AppState>,
    body: web::Bytes,
) -> HttpResponse {
    // 1. Parse SendTransactionRequest
    // 2. Convert to ProcessActionRequest
    // 3. Call processAction
    // 4. Extract TXID and raw_tx (Atomic BEEF)
    // 5. Extract raw hex from BEEF
    // 6. Broadcast (if needed)
    // 7. Return Go wallet format
}
```

---

## Challenges

### **Challenge 1: Raw Transaction vs BEEF** ✅ SOLVED!

**Discovery:**
- `signAction` creates `signed_tx_hex` (raw hex) at line 2522
- Then wraps it in BEEF and only returns BEEF hex
- Miners expect raw hex, NOT BEEF

**Question:** Do miners accept BEEF?
- **Answer:** It depends on the API endpoint:
  - **mAPI (current)**: ❌ NO - Expects raw transaction hex
    - GorillaPool mAPI: `{ "rawtx": "<raw_hex>" }`
    - WhatsOnChain: `{ "txhex": "<raw_hex>" }`
  - **ARC API**: ⚠️ MAYBE - May accept BEEF (requires API keys)
    - GorillaPool ARC: `POST /v1/tx` (requires auth)
    - Taal ARC: `POST /v1/tx` (requires auth)
    - Format unclear from docs - needs testing

**Our Current Implementation:**
- Uses **mAPI** (no API keys required)
- **mAPI expects raw hex** - not BEEF
- **ARC may accept BEEF** but requires API keys (not configured)

**Best Solution:**
- Extract raw hex from BEEF (works with current mAPI setup)
- No changes to existing code
- No API key management needed
- Can add ARC support later if needed

**Recommendation:** Extract from BEEF (Option 1) - simplest, works now

### **Challenge 2: Fee Rate** ⚠️ DEFERRED
- **Current:** Hardcoded 5000 satoshis in `createAction`
- **Solution:** Leave for now, document limitation
- **Future:** Add feeRate to `CreateActionOptions`

### **Challenge 3: Double Broadcasting**
- **Issue:** `processAction` may already broadcast
- **Solution:** Check if broadcast succeeded, handle errors gracefully

---

## Testing Checklist

- [ ] Basic send works
- [ ] Transaction appears on WhatsOnChain
- [ ] Insufficient funds error handled
- [ ] Invalid address error handled
- [ ] Response format matches Go wallet
- [ ] BEEF parsing works correctly
- [ ] Broadcasting to both miners works

---

## Implementation Order

1. **Phase 1:** Basic send (Steps 1-3, 6) - 1 hour
   - Test: Transaction creates and signs

2. **Phase 2:** BEEF extraction (Step 4) - 30 min
   - Test: Raw hex extracted correctly

3. **Phase 3:** Broadcasting (Step 5) - 15 min
   - Test: Transaction broadcasts successfully

4. **Phase 4:** Error handling (Step 7) - 30 min
   - Test: All error cases handled

**Total: ~2.25 hours** (plus testing time)

---

## Ready to Start?

The plan is clear and the infrastructure exists. The main complexity is BEEF parsing, which we already have functions for.

**Recommendation:** Start with Phase 1, test it, then continue with phases 2-4.
