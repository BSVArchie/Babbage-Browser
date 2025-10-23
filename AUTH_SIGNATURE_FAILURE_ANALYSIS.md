# Authentication Signature Failure - Root Cause Analysis

> **Date**: October 22, 2025
> **Status**: Signature verification failing at ToolBSV
> **Root Cause**: `/verifySignature` endpoint returning false (stub)

---

## 🔍 What's Actually Happening

### The Flow:

1. ✅ **ToolBSV** calls `/getPublicKey` → Gets our master public key `020b95...`
2. ✅ **ToolBSV** sends `/.well-known/auth` request with their nonce
3. ✅ **We** generate our nonce (32 bytes, random) ✅ FIXED!
4. ✅ **We** create BRC-42 derived child key and sign the concatenated nonces
5. ✅ **We** return signature to ToolBSV
6. ❌ **ToolBSV** calls `/verifySignature` to verify OUR signature → **Gets `false` (stub)** ❌

### From Your Logs:

```rust
// We create the signature successfully:
✅ Signature created (71 bytes, DER format): 3045022100ca1b...

// We return it to ToolBSV:
📤 FULL RESPONSE JSON: {
  "identityKey": "020b95583e18ac933d89a131f399890098dc1b3d4a8abcdde3eec4a7b191d2521e",
  "signature": [48, 69, 2, 33, 0, 202, ...]
}

// Then ToolBSV calls our verifySignature endpoint:
📋 /verifySignature called - STUB: returning false ❌
```

**THE PROBLEM**: ToolBSV is using OUR `/verifySignature` endpoint to verify the signature WE created!

---

## 💡 The Revelation: ToolBSV Uses `/verifySignature`

### What BRC-3 Says:

From [BRC-3 Digital Signature Creation and Verification](https://bsv.brc.dev/wallet/0003):

> **Signature Verification Request**: The signature verification request is a message sent by the **application to the client**. It contains a header with the following information...

**Translation**: Apps (like ToolBSV) call the WALLET's `/verifySignature` endpoint to ask the wallet to verify signatures!

### Why ToolBSV Does This:

Instead of implementing BRC-42 key derivation and ECDSA verification themselves, ToolBSV asks YOUR wallet:

```
ToolBSV: "Hey wallet, can you verify this signature for me?"
Wallet: "Sure! Let me derive the child key and check... false" (stub!)
Tool BSV: "Signature invalid! ❌"
```

---

## 🎯 The Real Issues:

### Issue #1: `/verifySignature` is Stubbed

**Current Implementation**:
```rust
pub async fn verify_signature(...) -> HttpResponse {
    log::info!("📋 /verifySignature called - STUB: returning false");
    HttpResponse::Ok().json(serde_json::json!({"result": false}))
}
```

**What It Should Do**:
1. Parse the signature verification request (protocolID, keyID, counterparty, signature, data)
2. Compute BRC-43 invoice number
3. Use BRC-42 to derive the signer's child public key
4. Verify signature using ECDSA
5. Return `{"result": true}` or `{"result": false}`

---

### Issue #2: Identity Key Consistency (Minor)

Looking at your logs, I see two different public keys mentioned:

```rust
// Master public key (what we return as identity):
020b95583e18ac933d89a131f399890098dc1b3d4a8abcdde3eec4a7b191d2521e

// Derived pubkey at index 0 (from other operations):
030fe8c7d8462cd9476b3e4ae26cf54fdf4cecbb83f088143a1c2f924b6a33e369
```

**The Good News**: Your `/.well-known/auth` handler uses the master key consistently:
- Returns master pubkey as identityKey ✅
- Signs with BRC-42 derived key from master ✅

**The Confusion**: The `030fe8...` key appears in `/createHmac` and `/verifyHmac` logs, NOT in auth. This is from the `derive_private_key_from_mnemonic(0)` function being called for HMAC operations, which is separate from auth.

**Verdict**: Your identity key handling in auth is CORRECT! The confusion is just from other operations.

---

### Issue #3: `verifyHmac` Calls

```rust
📋 /verifyHmac called
   Protocol ID: Array [Number(2), String("server hmac")]
   ...
   ✅ HMAC verification result: true
```

**What This Is**: ToolBSV is verifying the HMACs we created in `/createHmac` calls.

**Why**: Even though we removed HMAC-based nonce generation in `/.well-known/auth`, ToolBSV still uses HMACs for other purposes (nonce verification, etc.).

**Verdict**: These are working correctly! ✅

---

## 📋 BRC-3 Signature Requirements

### What We Need to Sign (BRC-104 Auth):

**Data**: Concatenated base64 nonces: `theirNonce + ourNonce`
```rust
let data_to_sign = base64::decode(theirNonce + ourNonce);
```

**Key Derivation**:
1. Compute invoice number: `2-auth message signature-{theirNonce} {ourNonce}`
2. Use BRC-42 to derive child private key with invoice
3. Sign with derived key (NOT master key directly!)

**Signature Format**: DER format (variable length, typically 70-72 bytes)

### What ToolBSV Does to Verify:

**Option A** (What They're Actually Doing):
```
ToolBSV calls YOUR /verifySignature endpoint:
{
  "protocolID": [2, "auth message signature"],
  "keyID": "{theirNonce} {ourNonce}",
  "counterparty": "self",
  "signature": [48, 69, 2, ...],
  "data": <concatenated nonces>
}

YOUR wallet derives child key and verifies → returns true/false
```

**Option B** (What We Thought They Do):
```
ToolBSV does verification themselves:
- Derives your child public key using BRC-42
- Verifies signature with ECDSA
```

**Reality**: They're using Option A! That's why `/verifySignature` stub is killing us!

---

## ✅ The Solution:

### Step 1: Implement `/verifySignature` Endpoint

**File**: `rust-wallet/src/handlers.rs`

**What It Needs to Do**:

```rust
pub async fn verify_signature(
    state: web::Data<AppState>,
    body: web::Json<VerifySignatureRequest>,
) -> HttpResponse {
    // 1. Parse request
    let protocol_id = parse_protocol_id(&body.protocol_id);
    let key_id = parse_key_id(&body.key_id);
    let counterparty = &body.counterparty;
    let signature_bytes = &body.signature;
    let data = parse_data(&body.data);

    // 2. Compute BRC-43 invoice number
    let invoice = create_invoice(protocol_id, key_id);

    // 3. Derive child public key using BRC-42
    let child_pubkey = if counterparty == "self" {
        // For "self", derive our own child public key
        derive_own_child_pubkey(master_privkey, invoice)
    } else {
        // For other counterparty, derive their child public key
        derive_counterparty_child_pubkey(counterparty_pubkey, invoice)
    };

    // 4. Verify signature with ECDSA
    let is_valid = verify_ecdsa(data, signature_bytes, child_pubkey);

    // 5. Return result
    HttpResponse::Ok().json(serde_json::json!({
        "result": is_valid
    }))
}
```

---

### Step 2: Understanding "counterparty: self"

When ToolBSV calls `/verifySignature` with `counterparty: "self"`, they're asking:

> "Can you verify a signature that YOU created?"

**Why?** Because in BRC-104 mutual authentication:
- We sign with OUR derived key
- They verify using OUR master pubkey + BRC-42 derivation
- But instead of doing the derivation themselves, they ask US to do it!

**The Logic**:
```rust
// When counterparty = "self", we're verifying OUR OWN signature:
// 1. Derive OUR child public key from OUR master private key
// 2. Verify the signature we created
// 3. Return true if valid
```

---

### Step 3: Test Vectors from BRC-3

From [BRC-3](https://bsv.brc.dev/wallet/0003#test-vectors):

```
Identity private key: 0000000000000000000000000000000000000000000000000000000000000001
Counterparty (signer): 0294c479f762f6baa97fbcd4393564c1d7bd8336ebd15928135bbcf575cd1a71a1
Security level: 2
Protocol ID: "BRC3 Test"
Key ID: "42"
Message: "BRC-3 Compliance Validated!"
Signature (DER): [48, 68, 2, 32, 43, 34, 58, ...]
```

We can use this to validate our implementation!

---

## 🔄 Complete Authentication Flow (Corrected):

```
1. ToolBSV → /getPublicKey
   ← Master pubkey: 020b95...

2. ToolBSV → /.well-known/auth { initialNonce: "abc..." }
   We:
   - Generate our nonce: "xyz..."
   - Concatenate: "abc...xyz..."
   - Derive child key via BRC-42 with invoice: "2-auth message signature-abc... xyz..."
   - Sign concatenated nonces with derived key
   ← Response: { initialNonce: "xyz...", yourNonce: "abc...", signature: [...] }

3. ToolBSV → /verifySignature {
      protocolID: [2, "auth message signature"],
      keyID: "abc... xyz...",
      counterparty: "self",  ← They want us to verify OUR signature!
      signature: [...],
      data: <concatenated nonces>
   }
   We:
   - Derive OUR child public key (same as we used for signing)
   - Verify signature
   ← {"result": true} ✅

4. ToolBSV: "Authentication successful!" ✅
```

---

## 🎯 Action Items:

### Immediate (Critical):
1. ✅ Understand the problem - `/verifySignature` stub
2. 📝 Implement `/verifySignature` endpoint with BRC-3 spec
3. 🧪 Test with BRC-3 test vectors
4. 🧪 Test with ToolBSV

### Implementation Priority:
1. **Parse request** - Extract protocolID, keyID, counterparty, signature, data
2. **Compute invoice** - Create BRC-43 invoice number
3. **Derive child pubkey** - Use BRC-42 to derive signer's child public key
4. **Verify signature** - ECDSA verification
5. **Return result** - `{"result": true/false}`

---

## 📊 Current Status Summary:

| Component | Status | Notes |
|-----------|--------|-------|
| Nonce generation | ✅ FIXED | Now 32 bytes random |
| Identity key | ✅ CORRECT | Master key used consistently |
| Signature creation | ✅ WORKING | BRC-42 derived key, DER format |
| Signature format | ✅ CORRECT | DER byte array |
| `/verifySignature` | ❌ **STUB** | **THIS IS THE BLOCKER** |
| `/createHmac` | ✅ WORKING | Used by ToolBSV for nonce verification |
| `/verifyHmac` | ✅ WORKING | ToolBSV verifying our HMACs |

---

**The Bottom Line**: Our signature creation is CORRECT! ToolBSV just can't verify it because our `/verifySignature` endpoint always returns `false`.

**Next Step**: Implement `/verifySignature` according to BRC-3 spec!

---

**Created**: October 22, 2025
**Status**: Root cause identified - `/verifySignature` stub
**Priority**: CRITICAL - Blocking all authentication
