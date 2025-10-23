# BRC Authentication Study Guide

> **Goal**: Understand the BRC-103/104 mutual authentication flow and fix our signature creation bug.

---

## 🎯 The Real Problem

**What's Actually Failing**:
- ToolBSV's JavaScript code cannot verify the signature WE create in `/.well-known/auth`
- The bug is in OUR signature creation, not in a `verifySignature` endpoint

**What We Confused**:
- ❌ We thought `verifySignature` endpoint (Call Code 33) was failing
- ✅ Actually, ToolBSV is verifying OUR signature and it's wrong

---

## 🤔 Critical Questions to Answer

### 1. What is Mutual Authentication?

**Questions**:
- In BRC-103, who authenticates whom?
- Does each party verify the other's signature?
- What is the complete handshake flow?
- When does wallet verify app? When does app verify wallet?

**Where to Find Answers**:
- [ ] BRC-103 specification
- [ ] TypeScript SDK `Peer.ts` implementation
- [ ] Our own logs from ToolBSV attempts

### 2. What Should `/.well-known/auth` Actually Do?

**Questions**:
- What data should we sign?
- What key should we use to sign (master vs derived)?
- What signature format (DER vs compact)?
- How should we encode the signature (hex, base64, byte array)?
- What should the complete response look like?

**Where to Find Answers**:
- [ ] BRC-104 specification (HTTP Transport)
- [ ] TypeScript SDK's auth implementation
- [ ] ToolBSV's frontend verification code

### 3. What is `verifySignature` Endpoint Actually For?

**Questions**:
- Is this part of the mutual auth flow?
- When would an app call this endpoint?
- Is it just a utility ("Hey wallet, verify this signature for me")?
- Do we need to implement it to pass auth with ToolBSV?

**Where to Find Answers**:
- [ ] BRC-100 specification (Call Code 33)
- [ ] BRC-3 specification (Digital Signatures)
- [ ] Real-world app examples

### 4. What's Wrong with Our Current Signature?

**🔍 THE SMOKING GUN - Found in `handlers.rs` lines 199-212:**

```rust
// BRC-84: For "self" counterparty, HMAC the nonce using our MASTER public key as the key
// This creates a verifiable signature that proves we generated this nonce
let mut mac = HmacSha256::new_from_slice(&master_pubkey_bytes)
    .expect("HMAC can take key of any size");
mac.update(&first_half);
let hmac_result = mac.finalize().into_bytes();

// 3. Concatenate: firstHalf (16 bytes) + hmac (32 bytes) = 48 bytes total
let mut nonce_bytes = Vec::new();
nonce_bytes.extend_from_slice(&first_half);
nonce_bytes.extend_from_slice(&hmac_result);

let our_nonce = base64::encode(&nonce_bytes);
```

**THE PROBLEM:**
- ✅ You correctly implemented BRC-103's OPTIONAL HMAC-based nonce generation
- ❌ But it's creating a 48-byte nonce (16 random + 32 HMAC)
- ❌ ToolBSV expects a 32-byte nonce!
- ❌ This breaks the signature verification

**What You Should Do Instead:**
```rust
// Simple random 32-byte nonce (what worked before!)
let our_nonce_bytes = generate_random_32_bytes();
let our_nonce = base64::encode(&our_nonce_bytes);
```

**The HMAC-based nonce is OPTIONAL and for YOUR convenience, not required by the protocol!**

---

## 📚 Documentation Reading List

### Priority 1: Authentication Flow (Read First!)

#### BRC-103: Peer-to-Peer Mutual Authentication
**URL**: https://bsv.brc.dev/peer-to-peer/0103

**What to Learn**:
- [ ] Complete authentication handshake steps
- [ ] Who signs what and when
- [ ] Who verifies what and when
- [ ] Nonce generation and exchange
- [ ] Signature creation requirements

**Key Sections to Focus On**:
- Abstract and motivation
- Authentication flow diagram
- Message formats
- Signature requirements

#### BRC-104: HTTP Transport for BRC-103
**URL**: https://bsv.brc.dev/peer-to-peer/0104

**What to Learn**:
- [ ] `/.well-known/auth` endpoint specification
- [ ] Request format (what app sends us)
- [ ] Response format (what we send back)
- [ ] Exact signature format and encoding

**Key Sections to Focus On**:
- Endpoint specification
- Request/response examples
- Signature field format
- Error handling

#### BRC-3: Digital Signature Creation and Verification
**URL**: https://bsv.brc.dev/transactions/0003

**What to Learn**:
- [ ] How to create proper BSV signatures
- [ ] DER vs compact signature formats
- [ ] Signature encoding (hex, base64, bytes)
- [ ] Verification process

**Key Sections to Focus On**:
- `createSignature` specification
- `verifySignature` specification
- Signature format requirements
- Examples

### Priority 2: Key Derivation (Review)

#### BRC-42: BSV Key Derivation Scheme (BKDS)
**URL**: https://bsv.brc.dev/key-derivation/0042

**What to Review**:
- [ ] ECDH shared secret calculation
- [ ] HMAC-based key derivation
- [ ] Child private key calculation
- [ ] Our implementation correctness

**We Already Implemented This** - but may have bugs!
- File: `rust-wallet/src/crypto/brc42.rs`
- Test vectors passing ✅
- But still failing in practice ❌

#### BRC-43: Security Levels, Protocol IDs, Key IDs
**URL**: https://bsv.brc.dev/key-derivation/0043

**What to Review**:
- [ ] Invoice number format
- [ ] Protocol ID for "auth message signature"
- [ ] Security level for authentication
- [ ] Counterparty identification

**Current Implementation**:
```rust
// Invoice: "2-auth message signature-{initialNonce} {sessionNonce}"
// Security Level: 2
// Protocol ID: "auth message signature"
```

### Priority 3: Reference Implementations

#### TypeScript SDK - Peer.ts
**Location**: `reference/ts-brc100/node_modules/@bsv/sdk/src/auth/Peer.ts`

**What to Study**:
- [ ] How they create signatures in auth flow
- [ ] How they verify incoming signatures
- [ ] Key derivation in practice
- [ ] Nonce handling

**Approach**:
- Read their `initialResponse` method
- Read their `verifyInitialResponse` method
- Compare with our Rust implementation

#### BSV Go SDK
**URL**: https://github.com/bsv-blockchain/go-sdk

**What to Study**:
- [ ] Signature creation implementation
- [ ] Key derivation implementation
- [ ] Compare with our Rust crypto

---

## 🔬 Debugging Strategy

### Step 1: Understand the Flow (Today)
1. Read BRC-103 to understand mutual auth
2. Read BRC-104 to understand HTTP transport
3. Draw a sequence diagram of the flow
4. Identify exactly where our bug is

### Step 2: Compare Implementations (Today)
1. Add extensive logging to our `/.well-known/auth` handler
2. Log every intermediate value:
   - Nonces (both theirs and ours)
   - Concatenated data
   - Key used for signing
   - Signature bytes
   - Final response
3. Compare with TypeScript SDK's implementation
4. Find the discrepancy

### Step 3: Fix and Test (Today/Tomorrow)
1. Fix the bug in our signature creation
2. Test with ToolBSV
3. Verify signature is accepted
4. Document the solution

---

## 📊 Current Understanding (To Be Updated)

### What We Think We Know:
- ✅ We derive a child key using BRC-42 (ECDH + HMAC)
- ✅ We sign: `theirNonce + ourNonce` (concatenated base64)
- ✅ We use DER signature format
- ✅ We return signature as byte array
- ⚠️ ToolBSV rejects our signature

### What We Need to Verify:
- ❓ Should we use master key or derived key?
- ❓ Should we concatenate base64 strings or decoded bytes?
- ❓ Is DER format correct or should it be compact?
- ❓ Should we encode as hex, base64, or byte array?
- ❓ Is our BRC-42 implementation correct?

### What We Recently Changed:
1. **Oct 21**: Used master key instead of `m/0` child key
2. **Oct 21**: Changed from compact to DER signature format
3. **Oct 21**: Changed signature field to byte array
4. **Oct 21**: Fixed nonce concatenation order
5. **Still failing** ❌

---

## 🎯 Success Criteria

**We'll know we understand the flow when**:
1. ✅ We can explain the complete BRC-103/104 handshake
2. ✅ We know exactly what data to sign and with what key
3. ✅ We know what format and encoding to use
4. ✅ We understand when `verifySignature` endpoint is needed (if at all)

**We'll know our bug is fixed when**:
1. ✅ ToolBSV accepts our signature
2. ✅ Complete authentication handshake succeeds
3. ✅ We can access authenticated features on ToolBSV

---

## 📝 Notes Section (Fill in as we learn)

### BRC-103 Notes:
- [To be filled after reading]

### BRC-104 Notes:
- [To be filled after reading]

### BRC-3 Notes:
- [To be filled after reading]

### Discoveries:
- [Document any "aha!" moments here]

---

**Created**: October 22, 2025
**Status**: Ready to start studying
**Next Action**: Read BRC-103 specification
