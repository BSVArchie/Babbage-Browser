# Testing ToolBSV Authentication (After Nonce Fix)

> **Status**: Nonce bug fixed! Ready to test with ToolBSV.

---

## 🎯 What Was Fixed

**Problem**: 48-byte nonce (16 random + 32 HMAC) causing signature verification failure
**Solution**: Simple 32-byte random nonce
**File Changed**: `rust-wallet/src/handlers.rs` (lines 128-162)

---

## 🚀 Testing Steps

### Step 1: Start the Rust Wallet

```powershell
cd rust-wallet
cargo run --release
```

**Expected Output**:
```
🦀 Bitcoin Browser Wallet (Rust)
=================================

📁 Wallet path: C:\Users\...\AppData\Roaming\BabbageBrowser\wallet\wallet.json
✅ Wallet loaded successfully
   Addresses: X
   Current index: X
   Current address: ...
   Current pubkey: ...
✅ Domain whitelist manager initialized

🌐 Starting HTTP server...
   Port: 3301
   URL: http://localhost:3301

📋 Available endpoints:
   GET  /health
   POST /.well-known/auth
   ...

✅ Server ready - CEF browser can now connect!
```

### Step 2: Start the CEF Browser

```powershell
cd cef-native\build\bin\Release
.\BitcoinBrowserShell.exe
```

**Expected**: Browser opens with React frontend

### Step 3: Navigate to ToolBSV

In the browser address bar:
```
https://toolbsv.com
```

### Step 4: Watch the Logs

Keep an eye on the Rust wallet terminal. When ToolBSV authenticates, you should see:

**Expected Logs**:
```
🔐 Babbage auth request received
   Identity key from request: ...
   Initial nonce: ...
   Message type: initialRequest
   Generated our nonce (32 bytes, random): ...
   Our MASTER identity key: ...
   Data to sign (64 bytes from concatenated base64 nonces)
   BRC-43 invoice number: 2-auth message signature-...
   ✅ MASTER private key retrieved for BRC-42 signing
   Using BRC-42 (ECDH-based derivation)...
   ✅ BRC-42 child key derived successfully
   Data hash (32 bytes): ...
   ✅ Signature created (XX bytes, DER format): ...
✅ Returning auth response with BRC-42 signature
   📤 Response fields: initialNonce=[ourNew], yourNonce=[theirInitialEchoed]
```

**Key Things to Check**:
- ✅ Nonce is 32 bytes (should see "32 bytes, random")
- ✅ Signature is created successfully
- ✅ Response is sent

### Step 5: Check ToolBSV Frontend

In the browser console (F12), you should see:

**Expected (Success)**:
```
✅ Authentication successful
✅ Signature verified
```

**If You Still See Error**:
```
❌ Unable to verify initial response signature
```

Then we need to debug further (but this should work now!)

---

## 🔍 Debugging if Authentication Fails

### Check Nonce Length:

In the Rust wallet logs, find:
```
Generated our nonce (32 bytes, random): ...
```

The hex string should be **64 characters** (32 bytes * 2)
The base64 string should be **44 characters** (32 bytes encoded)

### Check Signature Format:

In the logs, find:
```
✅ Signature created (XX bytes, DER format): ...
```

DER signatures are variable length (typically 70-72 bytes)

### Check Response:

The log should show:
```
📤 FULL RESPONSE JSON: {
  "version": "0.1",
  "messageType": "initialResponse",
  "identityKey": "...",
  "initialNonce": "...",  // Our nonce (32 bytes base64)
  "yourNonce": "...",     // Their nonce echoed back
  "signature": [48, 69, 2, ...]  // DER signature as byte array
}
```

---

## ✅ Success Criteria

**Authentication is working when**:
1. ✅ Rust wallet generates 32-byte nonce
2. ✅ Signature is created successfully
3. ✅ ToolBSV frontend accepts the signature
4. ✅ No "Unable to verify" errors in console
5. ✅ You can access authenticated features on ToolBSV

---

## 📝 What to Test After Authentication Works

Once authentication is successful:

1. **Basic Interaction**: Try using ToolBSV features
2. **Multiple Sessions**: Close and reopen ToolBSV, verify auth still works
3. **Other BRC-100 Sites**: Test with thryll.online, peerpay.babbage.systems
4. **Transaction Flow**: If ToolBSV requests transactions, test `createAction`/`signAction`

---

## 🐛 Common Issues

### Issue: "Nonce too long"
**Symptom**: ToolBSV complains about nonce length
**Check**: Verify nonce is exactly 32 bytes (44 chars base64)
**Solution**: Should be fixed now!

### Issue: "Signature verification failed"
**Symptom**: ToolBSV can't verify signature
**Possible Causes**:
- Wrong key used for signing (should be BRC-42 derived key)
- Wrong data signed (should be concatenated base64 nonces)
- Wrong signature format (should be DER as byte array)

### Issue: "CORS error"
**Symptom**: ToolBSV can't reach wallet
**Check**: CEF HTTP interceptor and domain whitelist
**Solution**: Verify thryll.online is in domain whitelist

---

## 📊 Expected vs Actual

### Before Fix:
```
❌ Nonce: 48 bytes (16 random + 32 HMAC)
❌ Base64: 64 characters (too long!)
❌ ToolBSV: "Unable to verify initial response signature"
```

### After Fix (Now):
```
✅ Nonce: 32 bytes (random)
✅ Base64: 44 characters (correct!)
✅ ToolBSV: Should accept signature ✅
```

---

## 🎉 When It Works

You'll know authentication is successful when:

1. **No Console Errors**: ToolBSV frontend shows no signature errors
2. **Authenticated Features**: You can access wallet-required features
3. **Transaction Creation**: ToolBSV can request transactions from wallet
4. **Smooth Flow**: Everything just works™

---

## 📞 Next Steps After Success

1. **Mark as Complete**: Update `BRC100_IMPLEMENTATION_GUIDE.md`
2. **Test Other Sites**: Try thryll.online, peerpay.babbage.systems
3. **Add Nonce Tracking**: Implement replay attack prevention (later)
4. **Move to Next Group**: Start implementing Group B methods (transactions already work!)

---

**Created**: October 22, 2025
**Status**: Ready to test!
**Expected Result**: ToolBSV authentication should work! 🎉
