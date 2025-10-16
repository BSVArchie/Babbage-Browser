# Rust Transaction Implementation Plan

## ✅ STATUS: COMPLETED (2025-10-16)

All transaction endpoints have been successfully implemented and tested with real BSV mainnet transactions!

---

## 🎉 Implementation Complete

### ✅ What We Have Working:
- **Authentication**: ✅ Complete BRC-103/104 handshake with ToolBSV
- **Endpoints**: ✅ All required endpoints implemented and working
  - `/getVersion`, `/getPublicKey`, `/isAuthenticated`
  - `/createHmac`, `/verifyHmac` (HMAC-based nonce verification)
  - `/verifySignature`, `/createSignature` (BRC-42 message signing)
  - `/.well-known/auth` (BRC-104 mutual authentication)
  - `/createAction` (Transaction building with UTXO selection)
  - `/signAction` (Transaction signing with BSV ForkID SIGHASH)
  - `/processAction` (Full transaction orchestration)
- **Crypto**: ✅ BRC-42 key derivation, BRC-43 invoice numbers, ECDSA signing/verification, HMAC
- **Wallet Storage**: ✅ Shared `wallet.json` with Go wallet
- **Transaction System**: ✅ **COMPLETE AND WORKING**
  - BSV ForkID SIGHASH implementation
  - P2PKH script generation and unlocking
  - Multi-miner broadcasting (WhatsOnChain + GorillaPool)
  - On-chain transaction confirmation

### 🎯 Confirmed Mainnet Transactions:
- `7dce601f2477d6024e9674eaac169773e31a0bd3d10c8c59e27649ba80124633` ✅
- `155c2539ea7f6bcc757d5f19374ad45f32bfa1a35c359f4ac3421602f84f60b9` ✅

### 🔑 Key Technical Achievements:
1. **Correct SIGHASH Algorithm**: Implemented BSV ForkID SIGHASH (not BIP143 or legacy)
2. **Preimage Format**: Version + hashPrevouts + hashSequence + input + prev_value + sequence + hashOutputs + locktime + sighash_type
3. **Double SHA256**: Proper SHA256d for SIGHASH calculation
4. **UTXO Fetching**: On-demand fetching from WhatsOnChain with P2PKH script generation
5. **Address Index Tracking**: Correct private key derivation for each UTXO owner
6. **Multi-Miner Broadcasting**: Simultaneous broadcast to WhatsOnChain and GorillaPool

---

## 2. ToolBSV Transaction Flow

When a user sends BSV on ToolBSV:

1. **ToolBSV calls `/createAction`** with:
   ```json
   {
     "outputs": [{"satoshis": 1000, "script": "..."}],
     "description": "Payment to merchant",
     "options": {"returnTXIDOnly": true}
   }
   ```
   **Returns:** `{ "txid": "...", "reference": "..." }`

2. **ToolBSV calls `/signAction`** with:
   ```json
   {
     "reference": "...",
     "spends": {}
   }
   ```
   **Returns:** `{ "txid": "...", "rawTx": "..." }`

3. **ToolBSV calls `/processAction`** with:
   ```json
   {
     "outputs": [...],
     "broadcast": true
   }
   ```
   **Returns:** `{ "txid": "...", "status": "completed" }`

---

## 3. Source Code Mapping

### From wallet-toolbox-rs:

| Module | Location | Lines | Purpose |
|--------|----------|-------|---------|
| `create_action.rs` | `wallet-core/src/methods/` | 2,070 | Transaction building |
| `sign_action.rs` | `wallet-core/src/methods/` | 526 | Transaction signing |
| `process_action.rs` | `wallet-core/src/methods/` | 121 | Orchestration |
| `transaction/` | `wallet-core/src/transaction/` | Multiple | TX structures |
| `beef/` | `wallet-core/src/beef/` | Unknown | BEEF format |
| `signer/` | `wallet-core/src/signer/` | Multiple | Signing logic |

### Dependencies:
- `wallet-storage` trait - **Need to adapt for wallet.json**
- `secp256k1` - ✅ Already have
- `sha2`, `ripemd` - ✅ Already have
- Transaction broadcaster service - **Need HTTP client**

---

## 4. Architecture Decision: Simplified vs Full Port

### Option A: Full Port (Complex)
- Copy entire `wallet-core` crate
- Implement `WalletStorageProvider` trait for `wallet.json`
- Support full BEEF, output baskets, labels, etc.
- **Pros:** Feature-complete, scalable
- **Cons:** Weeks of work, complex database layer

### Option B: Simplified Port (Recommended) ✅
- Extract only transaction building/signing logic
- Simplify to work with `wallet.json` structure
- Focus on core functionality first
- Add BEEF/SPV later if needed
- **Pros:** Faster, testable incrementally
- **Cons:** May need refactoring later

---

## 5. Simplified Implementation Plan

### ✅ Phase 1: Transaction Structures - COMPLETED
**Goal:** Define basic transaction types

**Implemented:**
- ✅ `rust-wallet/src/transaction/mod.rs` - Module exports and re-exports
- ✅ `rust-wallet/src/transaction/types.rs` - Complete transaction structures
  - `Transaction`, `TxInput`, `TxOutput`
  - `OutPoint`, `Script`
  - Serialization/deserialization
  - P2PKH script generation

### ✅ Phase 2: `/createAction` - Transaction Building - COMPLETED
**Goal:** Build unsigned transactions from outputs

**Implemented:**
1. ✅ Parse `CreateActionRequest` (outputs, description, options)
2. ✅ **Fetch UTXOs from WhatsOnChain** for all addresses (on-demand)
3. ✅ Calculate total output satoshis + fees
4. ✅ Select UTXOs to cover amount (largest-first algorithm)
5. ✅ Create change output if needed (dust limit: 546 satoshis)
6. ✅ Build unsigned transaction structure
7. ✅ Store in memory (HashMap with UUID references)
8. ✅ Return reference ID

**UTXO Fetching:**
- ✅ On-demand fetching from WhatsOnChain API
- ✅ P2PKH script generation (WhatsOnChain doesn't return scripts)
- ✅ Address index tracking for correct key derivation

**Files created:**
- ✅ `rust-wallet/src/utxo_fetcher.rs` - WhatsOnChain UTXO fetching
- ✅ `rust-wallet/src/handlers.rs` - `create_action()` handler

### ✅ Phase 3: `/signAction` - Transaction Signing - COMPLETED
**Goal:** Sign transaction inputs

**Implemented:**
1. ✅ Parse `SignActionRequest` (reference, spends)
2. ✅ Retrieve unsigned transaction from memory
3. ✅ For each input:
   - Get UTXO being spent
   - Derive private key for address owner
   - Calculate BSV ForkID SIGHASH (correct algorithm!)
   - Create ECDSA signature with secp256k1
   - Build P2PKH unlocking script (signature + pubkey)
4. ✅ Build final signed transaction
5. ✅ Return txid and rawTx

**CRITICAL FIX:**
- ✅ **BSV ForkID SIGHASH**: Replaced BIP143 (SegWit) with correct BSV algorithm
- ✅ **Preimage includes prev_value**: Added missing 8-byte value field
- ✅ **Double SHA256**: Proper SHA256d calculation
- ✅ **SIGHASH_ALL_FORKID = 0x41**: Correct signature flag

**Files created:**
- ✅ `rust-wallet/src/transaction/sighash.rs` - BSV ForkID SIGHASH implementation
- ✅ `rust-wallet/src/handlers.rs` - `sign_action()` handler

### ✅ Phase 4: `/processAction` - Broadcasting - COMPLETED
**Goal:** Broadcast to blockchain

**Implemented:**
1. ✅ Parse `ProcessActionRequest`
2. ✅ Call `createAction` internally
3. ✅ Call `signAction` internally
4. ✅ If `broadcast: true`:
   - POST to WhatsOnChain broadcaster
   - POST to GorillaPool mAPI
   - Handle responses
5. ✅ Return txid and status

**Broadcasting:**
- ✅ WhatsOnChain API: `https://api.whatsonchain.com/v1/bsv/main/tx/raw`
- ✅ GorillaPool mAPI: `https://mapi.gorillapool.io/mapi/tx`
- ❌ TAAL ARC: Removed (requires authentication)

**Files modified:**
- ✅ `rust-wallet/src/handlers.rs` - `process_action()` and broadcast functions

### ✅ Phase 5: Testing & Integration - COMPLETED
**Goal:** Test with ToolBSV and verify on-chain

**Testing Results:**
1. ✅ ToolBSV payment flow works end-to-end
2. ✅ Transactions confirmed on BSV mainnet
3. ✅ GorillaPool and WhatsOnChain both accepting transactions
4. ✅ Proper error handling and logging

**Confirmed Transactions:**
- `7dce601f2477d6024e9674eaac169773e31a0bd3d10c8c59e27649ba80124633`
- `155c2539ea7f6bcc757d5f19374ad45f32bfa1a35c359f4ac3421602f84f60b9`

---

## 6. Technical Details

### UTXO Selection Algorithm (Simple)
```rust
fn select_utxos(available: Vec<UTXO>, amount: u64) -> Vec<UTXO> {
    // Sort by value (largest first)
    // Pick UTXOs until we have enough
    // Return selected UTXOs
}
```

### Fee Calculation
```rust
fn calculate_fee(inputs: usize, outputs: usize) -> u64 {
    let size = inputs * 148 + outputs * 34 + 10; // Rough estimate
    size * 50 // 50 satoshis per byte
}
```

### Change Output
```rust
fn create_change(total_in: u64, total_out: u64, fee: u64) -> Option<TxOutput> {
    let change = total_in - total_out - fee;
    if change > 546 { // Dust limit
        Some(TxOutput { satoshis: change, script: change_script })
    } else {
        None // Add to fee
    }
}
```

### BSV ForkID SIGHASH Calculation ✅ IMPLEMENTED
```rust
const SIGHASH_ALL: u32 = 0x01;
const SIGHASH_FORKID: u32 = 0x40;
const SIGHASH_ALL_FORKID: u32 = 0x41; // Standard BSV signature flag

// BSV ForkID SIGHASH preimage format (based on BSV Go SDK)
fn calculate_sighash(tx: &Transaction, input_index: usize, prev_script: &[u8], prev_value: i64) -> [u8; 32] {
    // 1. Version (4 bytes, little-endian)
    // 2. hashPrevouts (32 bytes) - Double SHA256 of all input outpoints
    // 3. hashSequence (32 bytes) - Double SHA256 of all input sequences
    // 4. Input txid (32 bytes, reversed for wire format)
    // 5. Input vout (4 bytes, little-endian)
    // 6. Previous script length + script (varint + bytes)
    // 7. Previous output value (8 bytes, little-endian) ← CRITICAL!
    // 8. Sequence (4 bytes, little-endian)
    // 9. hashOutputs (32 bytes) - Double SHA256 of all outputs
    // 10. Locktime (4 bytes, little-endian)
    // 11. SIGHASH type (4 bytes, little-endian)
    //
    // Final: Double SHA256 (SHA256d) of entire preimage
}
```

**CRITICAL LEARNINGS:**
- ❌ **BIP143 is for SegWit**: Bitcoin Core's BIP143, which BSV does NOT support
- ✅ **BSV ForkID SIGHASH**: Modified SIGHASH with ForkID replay protection (after UAHF)
- ✅ **prev_value is required**: The 8-byte previous output value MUST be in the preimage
- ✅ **Double SHA256**: BSV uses SHA256d (double SHA256), not single SHA256
- ✅ **Based on Go SDK**: Our implementation matches `github.com/bsv-blockchain/go-sdk@v1.2.9`

---

## 7. Current `wallet.json` Structure

```json
{
  "mnemonic": "...",
  "addresses": [
    {
      "address": "1Fo6jvP...",
      "public_key": "030fe8c7...",
      "wif": "L4z...",
      "index": 0,
      "utxos": [
        {
          "txid": "abc123...",
          "vout": 0,
          "satoshis": 100000,
          "script": "76a914..."
        }
      ]
    }
  ]
}
```

**Modifications needed:**
- Track spent UTXOs (add `spent: bool` flag)
- Track pending transactions
- Update after broadcasts

---

## 8. Implementation Checklist - ALL COMPLETED ✅

### ✅ Pre-requisites (DONE)
- [x] BRC-42 key derivation
- [x] BRC-43 invoice numbers
- [x] ECDSA signing
- [x] JSON wallet storage
- [x] HTTP server framework

### ✅ Phase 1: Core Transaction Types (COMPLETED)
- [x] Create `rust-wallet/src/transaction/mod.rs`
- [x] Define `Transaction`, `TxInput`, `TxOutput` structs
- [x] Implement serialization (to/from hex)
- [x] Add basic validation

### ✅ Phase 2: `/createAction` Implementation (COMPLETED)
- [x] Add `CreateActionRequest`/`Response` structs
- [x] Implement UTXO selection logic (largest-first)
- [x] Implement fee calculation (5000 satoshis base fee)
- [x] Implement change output creation (dust limit: 546 satoshis)
- [x] Build unsigned transaction
- [x] Store in memory (HashMap<String, PendingTransaction>)
- [x] Test with ToolBSV ✅

### ✅ Phase 3: `/signAction` Implementation (COMPLETED)
- [x] Add `SignActionRequest`/`Response` structs
- [x] Implement BSV ForkID SIGHASH calculation (corrected from BIP143!)
- [x] Derive signing keys for each input (using address_index)
- [x] Generate ECDSA signatures with secp256k1
- [x] Build P2PKH unlocking scripts (signature + pubkey)
- [x] Construct final signed transaction
- [x] Test with ToolBSV ✅ - **Transactions confirmed on-chain!**

### ✅ Phase 4: `/processAction` Implementation (COMPLETED)
- [x] Add `ProcessActionRequest`/`Response` structs
- [x] Orchestrate createAction + signAction
- [x] Implement HTTP broadcaster (WhatsOnChain + GorillaPool)
- [x] Handle broadcast responses with proper parsing
- [x] Test end-to-end transaction ✅ - **Multiple successful broadcasts!**

### 🔮 Phase 5: BEEF & SPV (Future - Not Required for Basic Functionality)
- [ ] Port BEEF structures (if ToolBSV requires it)
- [ ] Implement SPV proof creation
- [ ] Add merkle path verification
- [ ] Integrate with createAction

---

## 9. Testing Strategy

### Test 1: Simple Send (Wallet Panel)
1. Click "Send" in wallet panel
2. Enter amount and address
3. Verify transaction builds
4. Verify transaction signs
5. Verify broadcast succeeds
6. Check on blockchain explorer

### Test 2: ToolBSV Payment
1. Trigger payment flow on ToolBSV
2. Verify `/createAction` called correctly
3. Verify `/signAction` called correctly
4. Verify `/processAction` broadcasts
5. Check ToolBSV shows success

### Test 3: Change Handling
1. Send amount requiring change
2. Verify change output created
3. Verify change returns to wallet
4. Verify UTXO tracking updated

---

## 10. Risks & Mitigation

### Risk 1: BEEF Required Immediately
**Mitigation:** Check ToolBSV request logs - if they require BEEF in `createAction`, add basic BEEF wrapper

### Risk 2: Complex UTXO Tracking
**Mitigation:** Start simple (in-memory), migrate to proper database later

### Risk 3: Broadcasting Failures
**Mitigation:** Test with small amounts first, add retry logic, use multiple broadcasters

### Risk 4: Fee Calculation Errors
**Mitigation:** Use conservative estimates initially, refine later

---

## 11. Key Questions to Answer During Implementation

1. **Does ToolBSV require BEEF format immediately?** (Check logs)
2. **What SIGHASH type does BSV use?** (Answer: SIGHASH_ALL | SIGHASH_FORKID = 0x41)
3. **Which broadcaster to use?** (WhatsOnChain vs ARC vs both)
4. **Do we need output baskets?** (Probably not initially)
5. **How to handle concurrent transaction creation?** (Lock UTXOs)

---

## 12. Estimated Timeline

- **Phase 1 (Transaction Types)**: 4-6 hours
- **Phase 2 (createAction)**: 8-12 hours
- **Phase 3 (signAction)**: 6-8 hours
- **Phase 4 (processAction)**: 4-6 hours
- **Phase 5 (Testing & Fixes)**: 8-12 hours

**Total:** 30-44 hours of focused work (~1 week)

---

## 13. Success Criteria - ALL ACHIEVED ✅

✅ **Milestone 1:** Build unsigned transaction from outputs - **COMPLETED**
✅ **Milestone 2:** Sign transaction with correct SIGHASH - **COMPLETED**
✅ **Milestone 3:** Broadcast successfully to network - **COMPLETED**
  - Transaction: `7dce601f2477d6024e9674eaac169773e31a0bd3d10c8c59e27649ba80124633`
  - Transaction: `155c2539ea7f6bcc757d5f19374ad45f32bfa1a35c359f4ac3421602f84f60b9`
✅ **Milestone 4:** ToolBSV payment completes end-to-end - **COMPLETED**
✅ **Milestone 5:** Multi-miner broadcasting works - **COMPLETED**
  - WhatsOnChain: ✅ Working
  - GorillaPool: ✅ Working
  - TAAL ARC: ❌ Removed (requires authentication)

---

## 14. Dual Wallet Architecture - Current State

### **Two Wallet Implementations:**

#### Go Wallet (Port 3301)
**Location:** `go-wallet/`
**Purpose:** Production-ready wallet using official BSV SDK
**Technology:** Go with `github.com/bsv-blockchain/go-sdk@v1.2.9`
**Status:** ✅ Production-ready

**Features:**
- HD wallet with BIP44 derivation
- Full BSV Go SDK integration
- Transaction creation, signing, broadcasting
- UTXO management
- BRC-100 authentication endpoints

#### Rust Wallet (Port 3301)
**Location:** `rust-wallet/`
**Purpose:** Custom BRC-100 implementation for testing
**Technology:** Rust with Actix-web, custom cryptography
**Status:** ✅ Transaction signing working

**Features:**
- BRC-103/104 mutual authentication
- Custom BSV ForkID SIGHASH implementation
- Transaction creation, signing, broadcasting
- On-demand UTXO fetching from WhatsOnChain
- Multi-miner broadcasting

**Important:**
- **Both use port 3301** - Only ONE can run at a time
- **Shared wallet.json** - `%APPDATA%/BabbageBrowser/wallet/wallet.json`
- **Testing phase** - Comparing both for production decision

---

## 15. Implementation Journey & Key Debugging Insights

### The SIGHASH Algorithm Discovery

**Initial Attempts (Failed):**
1. **Legacy Bitcoin SIGHASH**: First implementation used original Bitcoin SIGHASH
   - Modified transaction copy with cleared scriptSigs
   - Double SHA256 of modified transaction
   - **Result:** `mandatory-script-verify-flag-failed` errors

2. **BIP143 SIGHASH (SegWit)**: Second attempt implemented BIP143
   - Assumed BSV used BIP143 like Bitcoin Cash
   - Implemented hashPrevouts, hashSequence, hashOutputs
   - **Result:** Still failed - BIP143 is for SegWit, which BSV doesn't support!

3. **BSV ForkID SIGHASH (Success!)**: Final implementation based on Go SDK
   - Studied `github.com/bsv-blockchain/go-sdk@v1.2.9` source code
   - Discovered BSV uses modified SIGHASH with ForkID (0x40) flag
   - **Critical insight:** Preimage must include prev_value (8 bytes)
   - **Result:** ✅ Transactions signing and broadcasting successfully!

### Critical Debugging Discoveries

**Issue 1: Missing prev_value in Preimage**
- **Symptom:** Signature validation failures on broadcast
- **Root Cause:** Preimage was 174 bytes instead of 182 bytes
- **Fix:** Added 8-byte previous output value to preimage
- **Learning:** BSV ForkID SIGHASH requires prev_value, unlike legacy SIGHASH

**Issue 2: WhatsOnChain Doesn't Return Scripts**
- **Symptom:** `prev_script length: 0` in SIGHASH logs
- **Root Cause:** WhatsOnChain `/address/{address}/unspent` doesn't return `script` field
- **Fix:** Implemented `generate_p2pkh_script_from_address()` helper
- **Learning:** Must decode address and generate P2PKH script manually

**Issue 3: Address Index Tracking**
- **Symptom:** Signing with wrong private key (index 0 instead of actual owner)
- **Root Cause:** UTXO struct didn't track which address owned it
- **Fix:** Added `address_index` field to UTXO struct
- **Learning:** Must track ownership for correct key derivation

**Issue 4: GorillaPool URL**
- **Symptom:** 404 Not Found from GorillaPool
- **Root Cause:** Wrong subdomain (`api.` instead of `mapi.`)
- **Fix:** Changed URL to `https://mapi.gorillapool.io/mapi/tx`
- **Learning:** Always cross-reference with working implementations (Go wallet)

### Technical Insights

**BSV vs Bitcoin Core:**
- BSV does NOT use SegWit (no BIP143)
- BSV uses ForkID SIGHASH for replay protection (UAHF)
- SIGHASH_ALL_FORKID = 0x41 is the standard flag
- Previous output value is part of the preimage (unlike legacy)

**SIGHASH Preimage Components (182 bytes total for SIGHASH_ALL_FORKID):**
1. Version (4 bytes)
2. hashPrevouts (32 bytes) - if not SIGHASH_ANYONECANPAY
3. hashSequence (32 bytes) - if SIGHASH_ALL
4. Input txid (32 bytes, reversed)
5. Input vout (4 bytes)
6. Previous script (varint + 25 bytes for P2PKH)
7. **Previous output value (8 bytes)** ← Critical difference!
8. Sequence (4 bytes)
9. hashOutputs (32 bytes) - if SIGHASH_ALL
10. Locktime (4 bytes)
11. SIGHASH type (4 bytes)

**Total:** 4 + 32 + 32 + 32 + 4 + 1 + 25 + 8 + 4 + 32 + 4 + 4 = 182 bytes

**Reference Implementation:**
All SIGHASH logic based on `github.com/bsv-blockchain/go-sdk@v1.2.9/transaction/signaturehash.go`

---

## Appendix: Code References

### BSV Go SDK (Reference Implementation):
- `github.com/bsv-blockchain/go-sdk@v1.2.9/transaction/signaturehash.go` - BSV SIGHASH
- `github.com/bsv-blockchain/go-sdk@v1.2.9/transaction/transaction.go` - tx.Sign()
- `github.com/bsv-blockchain/go-sdk@v1.2.9/transaction/template/p2pkh/p2pkh.go` - P2PKH signing
- `github.com/bsv-blockchain/go-sdk@v1.2.9/transaction/sighash/flag.go` - SIGHASH flags

### Our Go Wallet (Working Reference):
- `go-wallet/transaction_builder.go` - UTXO selection, fee calculation
- `go-wallet/transaction_broadcaster.go` - Multi-miner broadcasting
- `go-wallet/utxo_manager.go` - UTXO tracking

### Our Rust Wallet (Implemented):
- `rust-wallet/src/transaction/sighash.rs` - BSV ForkID SIGHASH (based on Go SDK)
- `rust-wallet/src/transaction/types.rs` - Transaction structures
- `rust-wallet/src/utxo_fetcher.rs` - WhatsOnChain UTXO fetching
- `rust-wallet/src/handlers.rs` - All BRC-100 endpoints

### BRC Specifications:
- BRC-3: Digital Signature Creation and Verification
- BRC-12: Raw Transaction Format
- BRC-30: Background Evaluation Extended Format (BEEF)
- BRC-42: BSV Key Derivation Scheme (BKDS)
- BRC-43: Security Levels & Protocol IDs
- BRC-103: Peer-to-Peer Mutual Authentication
- BRC-104: HTTP Transport for BRC-103
