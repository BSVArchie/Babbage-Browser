# CRITICAL FIX: Master Key Consistency Across All Operations

> **Date**: October 22, 2025
> **Status**: FIXED - All operations now use master key
> **Priority**: CRITICAL - Was causing HMAC verification failures

---

## 🔍 The Problem Discovered

After fixing `/verifySignature`, authentication was working, but ToolBSV's backend was rejecting requests with:

```
HTTP 500 - {"description":"HMAC is not valid"}
```

### Root Cause: Key Mismatch

**For Authentication** (`/.well-known/auth`):
```rust
✅ MASTER private key retrieved
Our MASTER identity key: 020b95583e18ac933d89a131f399890098dc1b3d4a8abcdde3eec4a7b191d2521e
```

**For HMAC Operations** (`/createHmac`, `/verifyHmac`, `/createSignature`):
```rust
❌ Derived pubkey from mnemonic (index 0): 030fe8c7d8462cd9476b3e4ae26cf54fdf4cecbb83f088143a1c2f924b6a33e369
```

**The Issue**:
- Authentication used **MASTER key** (`020b95...`)
- HMAC operations used **INDEX 0 derived key** (`030fe8...`)

When ToolBSV tried to verify HMACs/signatures, the derived child keys didn't match because different base keys were used!

---

## ✅ The Fix

Changed all operations to use `get_master_private_key()` instead of `derive_private_key(0)`:

### 1. `/createHmac` Handler (line ~548)

**Before**:
```rust
let private_key_bytes = match storage.derive_private_key(0) {
    Ok(key) => {
        log::info!("   ✅ Private key derived from mnemonic (createHmac)");
        key
    },
    ...
};
```

**After**:
```rust
let private_key_bytes = match storage.get_master_private_key() {
    Ok(key) => {
        log::info!("   ✅ MASTER private key retrieved for HMAC (createHmac)");
        key
    },
    ...
};
```

### 2. `/verifyHmac` Handler (line ~842)

**Before**:
```rust
let private_key_bytes = match storage.derive_private_key(0) {
    Ok(key) => {
        log::info!("   ✅ Private key derived from mnemonic (verifyHmac)");
        key
    },
    ...
};
```

**After**:
```rust
let private_key_bytes = match storage.get_master_private_key() {
    Ok(key) => {
        log::info!("   ✅ MASTER private key retrieved for HMAC (verifyHmac)");
        key
    },
    ...
};
```

### 3. `/createSignature` Handler (line ~1334)

**Before**:
```rust
let private_key_bytes = match storage.derive_private_key(0) {
    Ok(key) => key,
    Err(e) => { ... }
};
log::info!("   ✅ Private key derived from mnemonic");
```

**After**:
```rust
let private_key_bytes = match storage.get_master_private_key() {
    Ok(key) => key,
    Err(e) => { ... }
};
log::info!("   ✅ MASTER private key retrieved for signature (createSignature)");
```

---

## 🎯 Why This Matters

### BRC-42 Key Derivation Works Like This:

1. **Compute shared secret**: `your_priv * their_pub`
2. **Compute HMAC**: `HMAC(shared_secret, invoice_number)`
3. **Derive child key**: `your_priv + HMAC_result`

**The Critical Part**: The shared secret depends on which private key you use!

- Using `master_priv * counterparty_pub` → shared secret A
- Using `index0_priv * counterparty_pub` → shared secret B (DIFFERENT!)

When you create an HMAC with index0 key but ToolBSV tries to verify it assuming master key, the derived child keys don't match!

---

## 📊 Impact on Authentication Flow

### Full Flow (Now Fixed):

1. **ToolBSV** → `/getPublicKey`
   - You return: `020b95...` (MASTER pubkey)

2. **ToolBSV** → `/.well-known/auth`
   - You sign with: BRC-42 derived from MASTER key ✅

3. **ToolBSV** → `/verifySignature` (verify your signature)
   - You verify using: BRC-42 derived from MASTER key ✅
   - **Result**: `true` ✅

4. **ToolBSV** → `/createHmac` (create HMAC for backend)
   - You create HMAC with: BRC-42 derived from MASTER key ✅

5. **ToolBSV Backend** verifies HMAC
   - Uses YOUR master pubkey (`020b95...`)
   - Derives child key with BRC-42
   - **Result**: HMAC matches! ✅

6. **ToolBSV** → `/createSignature` (sign requests to backend)
   - You sign with: BRC-42 derived from MASTER key ✅

---

## 🧪 Expected Results After Fix

### In Wallet Logs:

**Before** (wrong):
```
🔍 DEBUG: Derived pubkey from mnemonic (index 0): 030fe8...
✅ Private key derived from mnemonic (createHmac)
```

**After** (correct):
```
✅ MASTER private key retrieved for HMAC (createHmac)
```

### In Browser Console:

**Before** (wrong):
```javascript
HTTP 500 - {"description":"HMAC is not valid"}
```

**After** (correct):
```javascript
✅ Identity token retrieved successfully!
✅ BSV price loaded!
✅ Video/image history loaded!
```

---

## 🔑 Key Takeaways

### The BRC-42/43 Rule:

**ALWAYS use the same base private key for all operations with a given identity!**

- If you return master pubkey as your identity → use master privkey for everything
- If you return derived pubkey as your identity → use that derived privkey for everything

**Our Choice**: We return **master pubkey** as identity, so we must use **master privkey** for:
- ✅ Authentication signatures (`/.well-known/auth`)
- ✅ HMAC creation (`/createHmac`)
- ✅ HMAC verification (`/verifyHmac`)
- ✅ Message signatures (`/createSignature`)
- ✅ Signature verification (`/verifySignature`)

---

## 📝 Files Changed

### `rust-wallet/src/handlers.rs`

**Lines changed**:
- ~548-562: `/createHmac` - Use master key
- ~842-856: `/verifyHmac` - Use master key
- ~1334-1347: `/createSignature` - Use master key

**Build Status**: ✅ Compiles successfully (7.32s)

---

## 🚀 Next Steps

### Test Plan:

1. ✅ Build wallet with fix
2. 🧪 Start wallet: `cargo run --release`
3. 🧪 Start browser and navigate to ToolBSV
4. 🧪 Attempt authentication

### Success Indicators:

✅ No "HMAC is not valid" errors
✅ Identity token retrieved successfully
✅ BSV price displays
✅ ToolBSV features work (video/image history, etc.)

---

## 📚 Related Issues

- **Breakthrough #1**: Fixed 48-byte nonce bug (earlier today)
- **Breakthrough #2**: Implemented `/verifySignature` (earlier today)
- **Breakthrough #3**: Fixed master key consistency (THIS FIX!)

---

**This was the final piece! Authentication should now be FULLY WORKING!** 🎉

---

**Created**: October 22, 2025
**Status**: FIXED - Ready for testing
**Priority**: CRITICAL - Unblocks all BRC-100 authenticated operations
