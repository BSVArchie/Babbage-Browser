# Developer Notes - Bitcoin Browser

> **Quick Reference:** See [PROJECT_OVERVIEW.md](PROJECT_OVERVIEW.md) for architecture details and [README.md](README.md) for setup instructions.

---

## 🚨🚨 **NEW CRITICAL DISCOVERY: BRC-33 Message Relay Missing!** (2025-10-22 Evening)

### **What Happened:**
After fixing authentication, tested with Coinflip and Thryll apps → still failing!

**Wallet Logs:**
```
[2025-10-22T19:07:22Z INFO] 127.0.0.1 "POST /listMessages HTTP/1.1" 404
[2025-10-22T19:07:22Z INFO] 127.0.0.1 "POST /listMessages HTTP/1.1" 404
[2025-10-22T19:07:22Z INFO] 127.0.0.1 "POST /listMessages HTTP/1.1" 404
```

**App Error:**
```javascript
Error: object null is not iterable (cannot read property Symbol(Symbol.iterator))
```

### **The Discovery:**
Apps are calling **[BRC-33 PeerServ Message Relay](https://bsv.brc.dev/peer-to-peer/0033)** endpoints!

**Missing Endpoints:**
- `/sendMessage` - Send messages to recipients
- `/listMessages` - List messages from inbox ⚠️ **BLOCKING COINFLIP!**
- `/acknowledgeMessage` - Delete acknowledged messages

**Key Insights:**
- ✅ BRC-33 is **NOT part of BRC-100** - separate message relay specification
- ✅ Uses **HTTP POST**, not WebSocket
- ✅ Authentication already working (BRC-31/Authrite) - same as `/.well-known/auth`
- ✅ HTTP interceptor already catching these routes
- ❌ Rust wallet just needs handlers and message storage!

**Documentation Created:**
- See `BRC33_MESSAGE_RELAY_DISCOVERY.md` for comprehensive analysis
- See `BRC100_IMPLEMENTATION_GUIDE.md` for updated implementation plan

**Next Steps:**
1. Read [BRC-33 spec](https://bsv.brc.dev/peer-to-peer/0033)
2. Design message storage system
3. Implement 3 endpoints in Rust wallet
4. Test with Coinflip and Thryll

---

## 🎊 PREVIOUS SUCCESS: Rust Wallet BRC-103/104 Authentication (2025-10-22 Morning)

### **QUINTUPLE BREAKTHROUGH: HMAC Authentication FIXED!**

#### **Breakthrough #1: Fixed the Nonce Bug** ✅
**Problem:** 48-byte nonces (16 random + 32 HMAC) instead of 32 bytes
**Solution:** Simple 32-byte random nonces
**Status:** ✅ FIXED

#### **Breakthrough #2: Implemented `/verifySignature`** ✅
**Problem:** ToolBSV was calling `/verifySignature` to verify OUR signature, but it was stubbed
**Root Cause:** Misunderstood BRC-3 protocol - apps call the WALLET's endpoint
**Solution:** Fully implemented BRC-3 signature verification with BRC-42 child key derivation
**Status:** ✅ IMPLEMENTED & TESTED

#### **Breakthrough #3: Fixed Master Key Consistency** ✅
**Problem:** Using INDEX 0 key for HMAC but MASTER key for authentication
**Solution:** Changed all operations to consistently use master key
**Status:** ✅ FIXED & COMPILED

#### **Breakthrough #4: BRC-42 "self" Counterparty** ✅
**Problem:** HMAC failures persisted due to incorrect "self" interpretation
**Root Cause:** According to [BRC-56](https://bsv.brc.dev/wallet/0056#hmacs), `counterparty="self"` means NOT a two-party interaction
**Solution:** For `counterparty="self"`, use RAW master key (NO BRC-42 derivation for HMAC)
**Status:** ✅ FIXED & COMPILED

#### **Breakthrough #5: KeyID Base64 Encoding** ✅ **[FINAL FIX!]**
**Problem:** HMAC verification still failing with "nonce verification failed"
**Root Cause:** `String::from_utf8_lossy()` was CORRUPTING binary keyID bytes (e.g., byte 227 → �)
**Impact:** Invoice numbers didn't match between createHmac and verifyHmac!
**Solution:** Use `base64::encode()` to preserve ALL bytes exactly
**Status:** ✅ FIXED & COMPILED

**Current Status:**
- ✅ **Fixed**: Nonce generation - standard 32 bytes
- ✅ **Working**: `/getPublicKey` - returns master public key
- ✅ **Fixed**: `/createHmac`, `/verifyHmac` - Base64 keyID encoding!
- ✅ **Working**: BRC-42 child key derivation (passes all test vectors)
- ✅ **Working**: Domain whitelisting (both Go and Rust wallets)
- ✅ **Working**: `/.well-known/auth` - creates signatures correctly
- ✅ **IMPLEMENTED**: `/verifySignature` - BRC-3 compliant verification!
- ✅ **Fixed**: `counterparty="self"` uses raw key (per BRC-56 spec)
- ✅ **Fixed**: KeyID base64 encoding preserves binary data
- 🧪 **Ready to Test**: Full authentication flow with ToolBSV

### What Changed (Oct 22):

#### **Change #1: Fixed Nonce Generation** (lines 128-162)

**File**: `rust-wallet/src/handlers.rs`

**Removed**:
- 48-byte HMAC-based nonce generation (16 random + 32 HMAC)
- Confusing BRC-84 comments (BRC-84 is about linked keys, not nonces)
- Unnecessary HMAC complexity

**Added**:
- Simple 32-byte random nonce generation
- Clear comments explaining why (wallet = client, not high-volume server)
- TODO for nonce tracking (replay attack prevention for later)

#### **Change #2: Implemented `/verifySignature`** (lines 954-1214)

**File**: `rust-wallet/src/handlers.rs`

**Before**: Stub implementation returning `false`

**After**: Full BRC-3 compliant verification:
1. Parses signature verification request (protocolID, keyID, counterparty, signature, data)
2. Computes BRC-43 invoice number
3. Derives our child private key using BRC-42: `our_master_priv + counterparty_pub + invoice`
4. Extracts public key from child private key
5. Verifies signature using ECDSA with derived child public key
6. Returns `{"valid": true/false}`

**Key Insight**: ToolBSV calls OUR `/verifySignature` endpoint to verify signatures WE create! They don't do BRC-42 derivation themselves - they ask us to do it via this endpoint.

#### **Change #3: Fixed Master Key Consistency** (lines 548, 842, 1334)

**File**: `rust-wallet/src/handlers.rs`

**Problem**: Using different keys for different operations:
- Authentication: MASTER key (`020b95...`)
- HMAC operations: INDEX 0 key (`030fe8...`)

**Impact**: BRC-42 derivation produced different child keys, causing HMAC verification failures!

**Fixed**:
1. `/createHmac`: Now uses `get_master_private_key()` instead of `derive_private_key(0)`
2. `/verifyHmac`: Now uses `get_master_private_key()` instead of `derive_private_key(0)`
3. `/createSignature`: Now uses `get_master_private_key()` instead of `derive_private_key(0)`

**Why This Matters**: BRC-42 shared secret = `your_priv * their_pub`. Using different base keys produces different shared secrets and different child keys!

### Next Steps:
1. 🧪 **Test with ToolBSV** - Verify HMAC operations now work
2. ✅ **If successful** - Mark BRC-104 authentication as COMPLETE!
3. 📋 **Later** - Add nonce tracking to prevent replay attacks

---

## ✅ MAJOR ACHIEVEMENTS: Rust Wallet Transaction System (2025-10-16)

### Breakthrough: BSV ForkID SIGHASH Implementation

Successfully implemented **production-ready Rust wallet** with full BSV transaction support!

**What's Working:**
- ✅ Complete BRC-103/104 mutual authentication (working with ToolBSV)
- ✅ HMAC-based nonce verification (`/createHmac`, `/verifyHmac`)
- ✅ Transaction creation with UTXO selection from WhatsOnChain
- ✅ **BSV ForkID SIGHASH** signing algorithm (the breakthrough!)
- ✅ Multi-miner broadcasting (WhatsOnChain + GorillaPool)
- ✅ **Confirmed on-chain transactions**: `7dce601f...` and `155c2539...`

### Critical Technical Discovery: BSV ForkID SIGHASH

**The Problem:** Initial implementations using Legacy Bitcoin SIGHASH and BIP143 (SegWit) failed.

**The Solution:** BSV uses a unique ForkID SIGHASH algorithm that includes:
- `prev_value` (8 bytes) in the preimage - **THE MISSING PIECE**
- SIGHASH_ALL_FORKID = 0x41 (0x01 | 0x40)
- Double SHA256 (SHA256d) for final hash

**Reference:** Discovered by examining BSV Go SDK source code:
- `github.com/bsv-blockchain/go-sdk@v1.2.9/transaction/signaturehash.go`
- Function: `CalcInputPreimage()`

See [RUST_WALLET_SESSION_SUMMARY.md](RUST_WALLET_SESSION_SUMMARY.md) for complete technical details.

---

## 🏗️ Current Architecture

### Two Wallet Implementations (Development/Testing)

**⚠️ Important:** Both wallets use **port 3301** - only ONE can run at a time.

```
┌──────────────────────┐  ┌──────────────────────┐
│   Go Wallet          │  │   Rust Wallet        │
│   (Port 3301)        │  │   (Port 3301)        │
│                      │  │                      │
│ • BSV Go SDK         │  │ • Custom BSV Crypto  │
│ • BIP44 HD Wallet    │  │ • BRC-103/104 Auth   │
│ • Production Ready   │  │ • Transactions Work  │
└──────────┬───────────┘  └──────────┬───────────┘
           │                         │
           └────────┬────────────────┘
                    │
         Shared wallet.json Storage
      (%APPDATA%/BabbageBrowser/wallet/)
```

**Why Two Implementations?**
- Testing different languages (Go vs Rust)
- Go leverages official BSV SDK
- Rust provides custom BRC-100 implementation
- Will choose ONE for production

### System Components:

```
┌─────────────────────────────────────────┐
│  CEF Browser Shell (C++)                │
│  • HTTP Request Interceptor             │
│  • Domain Whitelist Integration         │
│  • Routes to Port 3301                  │
└──────────────┬──────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────┐
│  Wallet Backend (Port 3301)             │
│  [Go OR Rust - pick one]                │
│  • BRC-100 Authentication               │
│  • Transaction Signing                  │
│  • Multi-Miner Broadcasting             │
└──────────────┬──────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────┐
│  Bitcoin SV Network                     │
│  • WhatsOnChain API                     │
│  • GorillaPool mAPI                     │
└─────────────────────────────────────────┘
```

**Key Architecture Details:**
- **Process-Per-Overlay**: Each overlay (settings, wallet, backup) runs in isolated CEF subprocess
- **Domain Whitelist**: C++ HTTP interceptor checks whitelist before allowing requests
- **Async CEF HTTP Client**: Thread-safe HTTP communication using CEF's native methods

For complete architecture documentation, see [PROJECT_OVERVIEW.md](PROJECT_OVERVIEW.md).

---

## 📁 Current File Structure

```
babbage-browser/
├── cef-native/              # C++ CEF browser shell
│   ├── src/
│   │   ├── core/
│   │   │   └── HttpRequestInterceptor.cpp  # HTTP interception
│   │   └── handlers/
│   │       └── simple_handler.cpp          # CEF event handlers
│   └── build/               # CMake build artifacts
│
├── frontend/                # React + Vite UI
│   └── src/
│       ├── components/      # Wallet UI components
│       ├── hooks/           # React hooks
│       └── types/           # TypeScript types
│
├── go-wallet/              # Go wallet implementation ✅
│   ├── main.go             # HTTP server
│   ├── hd_wallet.go        # BIP44 HD wallet
│   ├── transaction_builder.go
│   ├── transaction_broadcaster.go
│   └── brc100_api.go       # BRC-100 endpoints
│
├── rust-wallet/            # Rust wallet implementation 🔧
│   ├── src/
│   │   ├── main.rs         # Actix-web server
│   │   ├── handlers.rs     # BRC-100 endpoints (2171 lines)
│   │   ├── json_storage.rs # wallet.json management
│   │   ├── crypto/         # BRC-42/43 implementations
│   │   │   ├── brc42.rs
│   │   │   └── brc43.rs
│   │   └── transaction/    # Transaction signing
│   │       ├── types.rs
│   │       └── sighash.rs  # BSV ForkID SIGHASH
│   └── Cargo.toml
│
└── reference/              # Reference implementations
    ├── ts-brc100/          # TypeScript SDK reference
    └── go-wallet-toolbox/  # Go SDK reference
```

---

## 🎯 Development Priorities

### Immediate (Current Session):
1. **Fix BRC-104 Signature Verification** - Debug signature verification failure
2. **Implement `/verifySignature`** - Complete BRC-84 signature verification
3. **Test with ToolBSV** - Validate complete authentication flow

### Short-term (Next 2-3 Sessions):
1. **Frontend Integration** - Connect React UI to Rust wallet endpoints
2. **Transaction UI** - Complete send/receive flows in browser
3. **Balance Display** - Real-time balance updates with USD conversion
4. **Transaction History** - Display past transactions

### Medium-term (Next Month):
1. **Domain Approval Modal** - Replace placeholder with real modal
2. **Wallet UI Polish** - Improve design and user experience
3. **Error Handling** - Comprehensive error messages and recovery
4. **Testing** - Complete end-to-end testing with multiple BRC-100 sites

### Long-term (Production):
1. **Consolidate Wallets** - Choose Go OR Rust, remove the other
2. **Security Audit** - Professional security review
3. **Performance Optimization** - Profile and optimize hot paths
4. **Build System** - Production build configuration

---

## 🔑 Key Technical Decisions

### 1. Native Wallet Backend (Not JavaScript)
**Decision:** Wallet operations run in isolated Go/Rust daemon, not in browser JavaScript.

**Rationale:**
- **Security**: Private keys never exposed to render process
- **Process Isolation**: Even if website compromises render process, wallet is safe
- **Memory Protection**: Native processes provide stronger memory protection
- **Attack Surface**: Significantly reduced compared to JavaScript-based wallets

### 2. Process-Per-Overlay Architecture
**Decision:** Each overlay (settings, wallet, backup) runs in separate CEF subprocess.

**Rationale:**
- **State Isolation**: No state pollution between overlays
- **Security**: Process boundaries provide additional security
- **Stability**: Crash in one overlay doesn't affect others
- **Mimics Brave**: Based on Brave Browser's security architecture

### 3. Shared Wallet Storage
**Decision:** Both Go and Rust wallets read/write same `wallet.json` file.

**Rationale:**
- **Development Flexibility**: Easy to switch between implementations
- **Testing**: Compare behavior with identical state
- **Future Migration**: Smooth transition when consolidating to single wallet

### 4. Port 3301 Standard
**Decision:** Use port 3301 for wallet daemon (BRC-100 standard).

**Rationale:**
- **Standard**: Matches other BRC-100 wallets (Metanet Desktop uses 3321)
- **Discovery**: Websites can discover wallet on standard port
- **Compatibility**: Works with existing BRC-100 sites

---

## 🧪 Testing Sites

### ✅ Working Sites:
- **ToolBSV.com** - Standard BRC-100 endpoints working
- **thryll.online** - Domain whitelisting working
- **Rust Wallet Transactions** - Multiple confirmed on-chain transactions

### ❌ Sites with Issues:
- **ToolBSV.com** - BRC-104 signature verification failing (current issue)

### 🎯 Test Coverage:
- ✅ Domain whitelisting
- ✅ HTTP request interception
- ✅ Transaction creation and signing
- ✅ Multi-miner broadcasting
- ✅ UTXO fetching and management
- ❌ Complete BRC-104 authentication flow (debugging)

---

## 📋 Quick Command Reference

### Start Rust Wallet:
```bash
cd rust-wallet
cargo run
# Server starts on http://127.0.0.1:3301
```

### Start Go Wallet:
```bash
cd go-wallet
go run main.go
# Or: ./start-wallet.bat
# Server starts on http://127.0.0.1:3301
```

### Build CEF Browser:
```bash
cd cef-native/build
cmake --build . --config Release
./bin/Release/BitcoinBrowserShell.exe
```

### Start Frontend Dev Server:
```bash
cd frontend
npm install
npm run dev
# Frontend at http://127.0.0.1:5137
```

---

## 🔗 Documentation References

- **[PROJECT_OVERVIEW.md](PROJECT_OVERVIEW.md)** - Complete architecture and design philosophy
- **[README.md](README.md)** - Project overview and setup instructions
- **[RUST_WALLET_SESSION_SUMMARY.md](RUST_WALLET_SESSION_SUMMARY.md)** - Detailed Rust wallet implementation
- **[API_REFERENCES.md](API_REFERENCES.md)** - API endpoint documentation
- **[BUILD_INSTRUCTIONS.md](BUILD_INSTRUCTIONS.md)** - Build system configuration

---

## 🚧 Known Issues

### Active Issues:
1. **BRC-104 Signature Verification** - ToolBSV frontend signature verification failing
2. **`/verifySignature` Endpoint** - Stubbed out, needs BRC-84 implementation

### Deferred Issues:
1. **Overlay HWND Movement** - Overlay windows don't follow main window (low priority)
2. **Transaction History** - Not yet implemented
3. **Advanced Address Management** - Gap limit, pruning, high-volume generation

---

## 📝 Session Notes

### Current Session (2025-10-21):
- **Started**: Debugging BRC-104 signature verification
- **Progress**: Tried multiple fixes for nonce concatenation and key derivation
- **Current**: Still failing signature verification
- **Next**: Add detailed logging to compare with TypeScript SDK

### Previous Session (2025-10-16):
- **Completed**: BSV ForkID SIGHASH implementation
- **Achieved**: Multiple confirmed on-chain transactions
- **Status**: Rust wallet transaction system fully working

---

**Last Updated:** October 22, 2025
**Current Focus:** Rust Wallet BRC-103/104 Authentication Debugging
**Next Session:** Continue signature verification debugging with detailed logging
