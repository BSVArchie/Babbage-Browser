# BRC-103 HMAC Confusion - Root Cause & Fix

## 🎯 TL;DR - The Fix

**Problem**: You created a 48-byte nonce using HMAC. ToolBSV expects 32 bytes.

**Solution**: Go back to simple 32-byte random nonces.

```rust
// REMOVE this HMAC-based nonce generation:
let mut mac = HmacSha256::new_from_slice(&master_pubkey_bytes)...
let nonce_bytes = first_half + hmac_result; // 48 bytes!

// REPLACE with simple random:
let our_nonce_bytes: [u8; 32] = rand::random();
let our_nonce = base64::encode(&our_nonce_bytes);
```

---

## 📖 The Three HMAC Concepts (Untangled)

### 1. **HMAC-Based Nonce Generation** (BRC-103 Section 6.2)
**What**: Optional way to generate nonces using HMAC
**Where**: [BRC-103 Section 6.2](https://bsv.brc.dev/peer-to-peer/0103#6-2-nonce-creation-and-verification)
**Quote**:
> "Optionally, one may use an **HMAC-based** approach to bind nonces to their key, avoiding the need to keep track of all nonces that they created."

**What This Means**:
- This is for YOUR internal implementation
- You CAN use HMAC to generate nonces deterministically
- Helps you avoid storing every nonce
- **But it's completely optional!**
- **The peer doesn't know or care how you generated the nonce**

**Standard Approach** (what most implementations use):
```rust
// Simple random 32-byte nonce - works perfectly!
let our_nonce: [u8; 32] = rand::random();
let our_nonce_base64 = base64::encode(&our_nonce);
```

**HMAC Approach** (optional, for nonce verification without storage):
```rust
// Generate nonce using HMAC - allows later verification
let our_nonce = hmac_sha256(master_key, their_nonce + timestamp);
// Later, you can re-compute to verify you created this nonce
```

**Key Point**: Whether you use random or HMAC, the peer just sees a 32-byte value!

---

### 2. **`createHmac` Endpoint** (BRC-100 Call Code 30)
**What**: Wallet endpoint that APPS call to create HMACs
**Where**: [BRC-100 Call Code 30](https://bsv.brc.dev/wallet/0100#createhmac)
**Purpose**: Utility function - apps ask wallet to create HMAC for them

**When Apps Call This**:
```javascript
// App wants wallet to create an HMAC
const hmac = await window.bitcoinBrowser.createHmac({
    protocolID: [1, "my-protocol"],
    keyID: "some-key",
    data: messageBytes,
    counterparty: "self"
});
```

**NOT related to `/.well-known/auth` authentication!**

---

### 3. **`verifyHmac` Endpoint** (BRC-100 Call Code 31)
**What**: Wallet endpoint that APPS call to verify HMACs
**Where**: [BRC-100 Call Code 31](https://bsv.brc.dev/wallet/0100#verifyhmac)
**Purpose**: Utility function - apps ask wallet to verify an HMAC

**When Apps Call This**:
```javascript
// App wants wallet to verify an HMAC
const isValid = await window.bitcoinBrowser.verifyHmac({
    protocolID: [1, "my-protocol"],
    keyID: "some-key"],
    data: messageBytes,
    hmac: hmacBytes,
    counterparty: "self"
});
```

**Also NOT related to `/.well-known/auth` authentication!**

---

## 🐛 What Actually Happened (Your Journey)

### Phase 1: Working Auth ✅
```rust
// Your original /.well-known/auth implementation
let our_nonce: [u8; 32] = rand::random();  // Simple 32-byte random
let our_nonce_base64 = base64::encode(&our_nonce);

let data_to_sign = format!("{}{}", their_nonce_base64, our_nonce_base64);
let signature = sign_with_derived_key(data_to_sign);

// Response
{
    "initialNonce": our_nonce_base64,  // 32 bytes encoded
    "yourNonce": their_nonce_base64,
    "signature": signature_bytes
}
```

**Result**: ✅ ToolBSV signature verification PASSED!

---

### Phase 2: Saw HMAC Errors ⚠️
You saw errors mentioning HMAC in logs. Possible scenarios:

**Scenario A**: ToolBSV was calling `createHmac` or `verifyHmac` endpoints
- These are separate BRC-100 endpoints
- Not related to authentication
- Just utility functions for apps

**Scenario B**: Error logs mentioned HMAC-based verification
- ToolBSV might use HMAC internally for something
- But not for nonce generation in auth!

---

### Phase 3: Misunderstood the Problem ❌
You thought: "HMAC failure means I should use HMAC for nonces"

So you implemented BRC-103 Section 6.2's OPTIONAL HMAC-based nonce generation:

```rust
// BROKEN: HMAC-based nonce (48 bytes!)
let first_half: [u8; 16] = rand::random();  // 16 bytes random
let mut mac = HmacSha256::new_from_slice(&master_pubkey_bytes).unwrap();
mac.update(&first_half);
let hmac_result = mac.finalize().into_bytes();  // 32 bytes HMAC

// Concatenate: 16 + 32 = 48 bytes
let mut nonce_bytes = Vec::new();
nonce_bytes.extend_from_slice(&first_half);      // 16 bytes
nonce_bytes.extend_from_slice(&hmac_result);    // 32 bytes
// Total: 48 bytes!

let our_nonce_base64 = base64::encode(&nonce_bytes);  // Encoding 48 bytes!
```

**Problem**:
- ✅ HMAC-based nonce generation is valid per BRC-103
- ❌ But you created a 48-byte nonce (16 random + 32 HMAC)
- ❌ Standard is 32 bytes total
- ❌ ToolBSV expects 32 bytes
- ❌ Breaks signature verification

---

### Phase 4: Auth Broke 💥
```
ToolBSV: "Unable to verify initial response signature"
```

**Why It Broke**:
1. Your nonce is now 48 bytes instead of 32 bytes
2. When ToolBSV concatenates `theirNonce (32) + yourNonce (48)` = 80 bytes
3. ToolBSV expects `theirNonce (32) + yourNonce (32)` = 64 bytes
4. The signature verification fails because data lengths don't match

---

## ✅ The Fix

### Step 1: Remove HMAC-Based Nonce Generation

**File**: `rust-wallet/src/handlers.rs`
**Lines**: ~176-212

**REMOVE**:
```rust
// Generate cryptographically secure random 256-bit value for FIRST HALF of nonce
let first_half: [u8; 16] = rand::random();
log::info!("   Generated first half of nonce (16 bytes): {}", hex::encode(&first_half));

// ... HMAC code ...

// 3. Concatenate: firstHalf (16 bytes) + hmac (32 bytes) = 48 bytes total
let mut nonce_bytes = Vec::new();
nonce_bytes.extend_from_slice(&first_half);
nonce_bytes.extend_from_slice(&hmac_result);
```

**REPLACE WITH**:
```rust
// Generate simple random 32-byte nonce (standard approach for wallets)
// NOTE: For a wallet client with low session volume, simple random nonces are ideal.
// HMAC-based nonces are only needed for high-volume servers (100k+ sessions).
let our_nonce_bytes: [u8; 32] = rand::random();
log::info!("   Generated our nonce (32 bytes, random): {}", hex::encode(&our_nonce_bytes));

// TODO: Add nonce tracking to prevent replay attacks
// Store nonce with timestamp and expire after 1 hour
```

### Step 2: Update Base64 Encoding

**CHANGE**:
```rust
let our_nonce = base64::encode(&nonce_bytes);  // Was encoding 48 bytes
```

**TO**:
```rust
let our_nonce = base64::encode(&our_nonce_bytes);  // Now encoding 32 bytes
```

### Step 3: Remove Misleading BRC-84 Comments

Your code has comments mentioning "BRC-84" for HMAC-based nonce generation. This is confusing:

- **BRC-84** is about **Linked Key Derivation** (different concept)
- **BRC-103 Section 6.2** describes HMAC-based nonce generation

Remove or fix those comments.

---

## 🧪 Testing After Fix

### Test 1: Verify Nonce Length
```rust
let our_nonce_bytes: [u8; 32] = rand::random();
assert_eq!(our_nonce_bytes.len(), 32);  // Must be 32 bytes!
```

### Test 2: Verify Base64 Length
```rust
let our_nonce_base64 = base64::encode(&our_nonce_bytes);
// Base64 of 32 bytes = 44 characters (32 * 4/3 = 42.67 → 44 with padding)
assert_eq!(our_nonce_base64.len(), 44);
```

### Test 3: Test with ToolBSV
1. Restart Rust wallet
2. Visit ToolBSV.com
3. Check logs for "✅ Signature verified"
4. Verify authentication completes

---

## 📚 When to Use HMAC-Based Nonces

**Use HMAC-based nonces when**:
- You need to verify later that you created a nonce
- You don't want to store every nonce
- You're implementing a server handling thousands of sessions

**Example Use Case**:
```rust
// Server generates nonce using HMAC
let nonce = hmac_sha256(server_secret, client_id + timestamp);

// Later, when client responds, server can verify:
let expected_nonce = hmac_sha256(server_secret, client_id + timestamp);
if nonce == expected_nonce {
    // Valid! We created this nonce.
}
// No need to store nonces in database!
```

**For your wallet**:
- You're a client, not a high-volume server
- You only need a few concurrent sessions
- Simple random nonces are perfectly fine!
- **Don't overcomplicate it!**

---

## 🎯 Understanding the `createHmac`/`verifyHmac` Endpoints

These endpoints are **separate features** for apps to use:

### When Apps Use `createHmac`:
```javascript
// App scenario: Signing API requests to a server
const apiKey = "my-api-key";
const requestData = "GET /api/user/123";
const timestamp = Date.now();

// App asks wallet to create HMAC using derived key
const hmac = await window.bitcoinBrowser.createHmac({
    protocolID: [1, "api-auth"],
    keyID: apiKey,
    data: requestData + timestamp,
    counterparty: serverPublicKey
});

// App includes HMAC in API request header
fetch('/api/user/123', {
    headers: {
        'X-Timestamp': timestamp,
        'X-HMAC': hmac
    }
});
```

### When Apps Use `verifyHmac`:
```javascript
// App scenario: Verifying server responses
const serverResponse = await fetch('/api/user/123');
const serverHmac = serverResponse.headers.get('X-HMAC');
const responseData = await serverResponse.text();

// App asks wallet to verify HMAC
const isValid = await window.bitcoinBrowser.verifyHmac({
    protocolID: [1, "api-auth"],
    keyID: apiKey,
    data: responseData,
    hmac: serverHmac,
    counterparty: serverPublicKey
});

if (isValid) {
    // Response is authentic!
}
```

**These have NOTHING to do with `/.well-known/auth` authentication!**

---

## 🎓 Key Takeaways

### 1. **BRC-103 vs BRC-104**
- **BRC-103**: The mutual authentication protocol (what to do)
- **BRC-104**: HTTP transport for BRC-103 (how to do it)
- **`/.well-known/auth`**: BRC-104 implementation of BRC-103 handshake

### 2. **Three Different HMAC Concepts**
- **HMAC-based nonce generation**: Optional internal implementation (BRC-103 6.2)
- **`createHmac` endpoint**: BRC-100 utility for apps (Call Code 30)
- **`verifyHmac` endpoint**: BRC-100 utility for apps (Call Code 31)

### 3. **The Standard Nonce Format**
- **Size**: 32 bytes (256 bits)
- **Generation**: Random OR HMAC-based (your choice)
- **Encoding**: Base64 (44 characters)
- **ToolBSV expects**: 32 bytes!

### 4. **The Fix**
- Remove HMAC-based nonce generation
- Use simple 32-byte random nonces
- Your signature was probably correct all along!
- The issue was just nonce length (48 vs 32 bytes)

---

## ✅ Action Items

### Immediate (Fix Authentication):
- [ ] Remove HMAC-based nonce generation from `/.well-known/auth`
- [ ] Use simple 32-byte random nonces
- [ ] Test with ToolBSV
- [ ] Verify signature passes
- [ ] Mark BRC-104 authentication as working ✅

### Later (Add Replay Protection):
- [ ] Implement nonce tracking (HashMap with timestamps)
- [ ] Add nonce expiration (1 hour)
- [ ] Add cleanup routine for old nonces
- [ ] Test replay attack prevention

**Note**: Nonce tracking isn't critical for initial testing with ToolBSV since you're not under attack yet. Get authentication working first, then add replay protection.

---

**Created**: October 22, 2025
**Status**: Root cause identified, fix documented
**Estimated Fix Time**: 10 minutes
**Expected Result**: ToolBSV authentication will work! 🎉
