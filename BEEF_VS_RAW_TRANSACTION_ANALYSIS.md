# BEEF vs Raw Transaction Analysis

## Question: Can We Send BEEF to Miners?

### **Answer: It Depends on the API Endpoint**

**Current Implementation Uses:**
- **GorillaPool mAPI**: `https://mapi.gorillapool.io/mapi/tx` - Expects **raw hex** ❌
- **WhatsOnChain**: `https://api.whatsonchain.com/v1/bsv/main/tx/raw` - Expects **raw hex** ❌

**ARC API (Alternative):**
- **GorillaPool ARC**: `https://arc.gorillapool.io/v1/tx` - **May accept BEEF?** ⚠️
- **Taal ARC**: `https://api.taal.com/arc/v1/tx` - **May accept BEEF?** ⚠️
- **Requires API keys** for authentication

**Conclusion:** Our current implementation uses mAPI (not ARC), which expects raw hex.

---

## What Miners Expect

### **Current Implementation (mAPI - No API Keys Required):**

**WhatsOnChain API:**
```
POST https://api.whatsonchain.com/v1/bsv/main/tx/raw
Content-Type: application/json

{
  "txhex": "0100000001..."  ← Raw transaction hex
}
```

**GorillaPool mAPI:**
```
POST https://mapi.gorillapool.io/mapi/tx
Content-Type: application/json

{
  "rawtx": "0100000001..."  ← Raw transaction hex
}
```

**Both mAPI endpoints expect:** Raw transaction hex string, NOT BEEF format.

---

### **ARC API (Requires API Keys):**

According to [GorillaPool ARC documentation](https://docs.gorillapool.io/arc/#/):
- **ARC endpoint**: `POST /v1/tx` (requires authentication)
- **May accept BEEF format** - needs verification
- **Requires API keys** - Bearer token or Api-Key header
- **Different from mAPI** - ARC is a separate API

**ARC Transaction Request:**
```json
POST /v1/tx
Authorization: Bearer {token} or Api-Key: {key}
Content-Type: application/json

{
  "rawTx": "..."  ← Format unclear - may accept BEEF or raw hex
}
```

**Note:** ARC documentation doesn't explicitly state BEEF support. It may accept BEEF, but this needs testing with actual API keys and verification.

---

## Current Implementation Issue

### **What `signAction` Does:**
1. ✅ Creates signed transaction: `signed_tx_hex` (raw hex) - **line 2522**
2. ✅ Wraps in BEEF: Creates Atomic BEEF format
3. ❌ **Only returns BEEF hex** in `raw_tx` field - **line 2775**

### **What `processAction` Does:**
1. Gets BEEF hex from `signAction`
2. Passes BEEF hex to `broadcast_transaction(&raw_tx)` - **line 2888**
3. Broadcast function tries to send BEEF to miners ❌

**Problem:** Miners will reject BEEF format!

---

## Solutions

### **Option 1: Extract Raw TX from BEEF** ⭐ RECOMMENDED (No Changes)

**Pros:**
- ✅ No changes to existing `signAction`
- ✅ Works with current code
- ✅ BEEF parsing already implemented

**Cons:**
- ⚠️ Extra parsing step (negligible overhead)

**Implementation:**
```rust
fn extract_raw_tx_from_atomic_beef(beef_hex: &str) -> Result<String, String> {
    let beef_bytes = hex::decode(beef_hex)?;
    let (_txid, beef) = Beef::from_atomic_beef_bytes(&beef_bytes)?;
    let main_tx = beef.main_transaction().ok_or("No main tx")?;
    Ok(hex::encode(main_tx))
}
```

**Time:** 15 minutes

---

### **Option 2: Modify `SignActionResponse`** ⭐ BETTER LONG-TERM

**Pros:**
- ✅ No parsing needed
- ✅ More efficient
- ✅ Both formats available for different use cases

**Cons:**
- ⚠️ Requires changing `signAction` response format
- ⚠️ May break existing code expecting only BEEF

**Implementation:**
```rust
#[derive(Debug, Serialize, Deserialize)]
pub struct SignActionResponse {
    pub txid: String,
    #[serde(rename = "rawTx")]
    pub raw_tx: String,  // Atomic BEEF (for BRC-100 apps)
    #[serde(rename = "rawTxHex")]
    pub raw_tx_hex: Option<String>,  // Raw transaction (for broadcasting)
}
```

Then in `signAction`:
```rust
HttpResponse::Ok().json(SignActionResponse {
    txid,
    raw_tx: beef_hex,  // BEEF format
    raw_tx_hex: Some(signed_tx_hex),  // Raw transaction
})
```

**Time:** 30 minutes (includes testing)

---

### **Option 3: Skip BEEF for Simple Sends**

**Pros:**
- ✅ Fastest for simple P2PKH sends
- ✅ No BEEF overhead
- ✅ Direct raw transaction

**Cons:**
- ❌ No SPV proofs
- ❌ Different code path
- ❌ Still need BEEF for BRC-100 apps

**Implementation:**
- Call `createAction` logic directly
- Call signing logic directly
- Get `signed_tx_hex` before BEEF wrapping
- Broadcast raw hex

**Time:** 1-2 hours (requires extracting logic)

---

## Recommendation

**For `/transaction/send` endpoint: Use Option 1 (Extract from BEEF)** ⭐

**Why:**
- ✅ Quickest to implement
- ✅ No changes to existing code (won't break `signAction`)
- ✅ Works with current mAPI endpoints (no API keys needed)
- ✅ BEEF parsing already exists and works
- ✅ Avoids API key management complexity

**About ARC API:**
- ARC may accept BEEF format (needs verification)
- ARC requires API keys for authentication
- We don't currently have API keys configured
- mAPI works without API keys (uses raw hex)

**Future Enhancement:**
- Add ARC API support with BEEF format (if confirmed)
- Configure API keys for ARC endpoints
- Keep mAPI as fallback (no API keys required)

---

## Testing BEEF vs Raw

**Test 1: Try sending BEEF to miner**
```bash
# This will likely fail
curl -X POST https://api.whatsonchain.com/v1/bsv/main/tx/raw \
  -H "Content-Type: application/json" \
  -d '{"txhex": "<atomic_beef_hex>"}'
```

**Expected:** Error response (miner doesn't recognize BEEF format)

**Test 2: Send raw transaction**
```bash
# This should work
curl -X POST https://api.whatsonchain.com/v1/bsv/main/tx/raw \
  -H "Content-Type: application/json" \
  -d '{"txhex": "<raw_tx_hex>"}'
```

**Expected:** Success response with TXID

---

## Conclusion

**Current Implementation (mAPI):**
- ❌ **Does NOT accept BEEF** - expects raw transaction hex
- ✅ **No API keys required** - works immediately
- ✅ **Currently in use** - `broadcast_to_gorillapool()` uses mAPI

**ARC API (Alternative):**
- ⚠️ **May accept BEEF** - needs verification
- ❌ **Requires API keys** - authentication needed
- ❌ **Not currently configured** - would need setup

**Best approach for `/transaction/send`:**
1. Use `processAction` to get BEEF
2. Extract raw transaction from BEEF using existing parser
3. Broadcast raw hex to mAPI endpoints (current implementation)
4. Return Go wallet format response

**Why Extract from BEEF (Option 1):**
- ✅ Works with current mAPI endpoints (no changes needed)
- ✅ No API key management required
- ✅ Won't break existing `signAction` functionality
- ✅ BEEF parsing already exists

**Future:** If we want to use ARC API with BEEF:
- Get API keys for GorillaPool ARC / Taal ARC
- Test if ARC accepts BEEF format
- Add ARC broadcaster as alternative/fallback
- Keep mAPI as primary (no keys needed)
