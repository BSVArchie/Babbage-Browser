# 🎯 Next Steps: BRC-33 Message Relay Implementation

**Created**: October 22, 2025
**Status**: Ready to implement
**Priority**: 🔴 **CRITICAL** - Blocking Coinflip, Thryll, and other apps

---

## ✅ **What We've Done**

1. ✅ Identified the root cause: Missing BRC-33 endpoints
2. ✅ Updated `BRC100_IMPLEMENTATION_GUIDE.md` with BRC-33 section
3. ✅ Created comprehensive `BRC33_MESSAGE_RELAY_DISCOVERY.md` document
4. ✅ Updated `DEVELOPER_NOTES.md` with discovery
5. ✅ Analyzed existing architecture (Go WebSocket vs BRC-33 HTTP)

---

## 📋 **Your Questions - Answered**

### **Q: These endpoints need to be added?**
✅ **YES** - 3 HTTP POST endpoints:
- `/sendMessage`
- `/listMessages` ← **CRITICAL** - apps are waiting for this!
- `/acknowledgeMessage`

### **Q: We started this in our Go wallet?**
❌ **NO** - The `go-wallet/brc100/websocket/` is a **different** WebSocket server for real-time notifications, **NOT** BRC-33 message relay. BRC-33 uses **HTTP POST**, not WebSocket.

### **Q: How is this initiated?**
✅ **Simple HTTP requests** - apps just make authenticated POST requests:
```http
POST /listMessages HTTP/1.1
Host: localhost:3301
X-Authrite-Identity-Key: 020b95...
X-Authrite-Signature: ...
Content-Type: application/json

{
  "messageBox": "coinflip_inbox"
}
```

### **Q: How will we do this in Rust?**
✅ **Add 3 new HTTP handlers** in `rust-wallet/src/handlers.rs` (same file as other endpoints):
```rust
pub async fn send_message(...) -> impl Responder { ... }
pub async fn list_messages(...) -> impl Responder { ... }
pub async fn acknowledge_message(...) -> impl Responder { ... }
```

### **Q: We create server in our CEF C++ backend on port 3302?**
❓ **UNCLEAR** - Port 3302 purpose is unknown. Could be:
- WebSocket server for real-time notifications (optional enhancement)
- BRC-34 federation (multi-server routing)
- Something else?

**Recommendation**: **Ignore port 3302 for now**. Implement BRC-33 as HTTP POST on port 3301 (Rust wallet).

---

## 📚 **Documentation to Study (In Order)**

### 1. **[BRC-33: PeerServ Message Relay](https://bsv.brc.dev/peer-to-peer/0033)** 🔴 **READ FIRST**
**Time**: 30-45 minutes
**Focus**:
- The 3 endpoints (request/response formats)
- Message structure
- "Not for long-term storage" limitation
- Message authenticity section

### 2. **[BRC-31: Authrite Authentication](https://bsv.brc.dev/peer-to-peer/0031)** ✅ **Already Done!**
**Status**: We already implemented this for `/.well-known/auth`
**Note**: BRC-33 uses the same authentication

### 3. **[BRC-34: CHIP Federation](https://bsv.brc.dev/peer-to-peer/0034)** 🟡 **Optional - Skip for MVP**
**Time**: 20 minutes (skim)
**Purpose**: Multi-server message routing
**Question**: Is this what port 3302 is for?

### 4. **[BRC-77: Message Signatures](https://bsv.brc.dev/peer-to-peer/0077)** 🟡 **Optional - Skip for MVP**
**Time**: 15 minutes (skim)
**Purpose**: Optional message body signatures
**Note**: Probably not needed for basic implementation

---

## 🎯 **Implementation Roadmap**

### **Today (2-3 hours)**

#### Step 1: Read BRC-33 Spec (1 hour)
```bash
# Open in browser:
https://bsv.brc.dev/peer-to-peer/0033

# Answer these questions while reading:
1. What's the exact request format for each endpoint?
2. What response fields are required?
3. How are message boxes named? (convention?)
4. Who can send to whose message box?
5. When are messages deleted?
```

#### Step 2: Design Storage System (30 minutes)
**Decision Points**:
- [ ] Storage type: In-memory HashMap? JSON file? SQLite?
- [ ] Message structure fields
- [ ] Message retention policy
- [ ] Max messages per inbox?

**Recommended**: Start with in-memory HashMap, add persistence later

#### Step 3: Answer Architecture Questions (30 minutes)
```
Key Questions (from BRC33_MESSAGE_RELAY_DISCOVERY.md):
1. Where to store messages?
2. How long to keep them?
3. Who can access what?
4. Do we need federation now? (answer: NO)
5. Do we need WebSocket push? (answer: NO, start with polling)
```

### **Tomorrow (4-5 hours)**

#### Step 4: Create Message Storage Module (1 hour)
```bash
# Create new file:
rust-wallet/src/message_relay.rs

# Implement:
- MessageStore struct
- Message struct
- send_message()
- list_messages()
- acknowledge_messages()
```

#### Step 5: Implement Handlers (2 hours)
```bash
# Edit: rust-wallet/src/handlers.rs

# Add 3 new handlers:
- send_message()
- list_messages()
- acknowledge_message()

# Use existing auth middleware (already working!)
```

#### Step 6: Add Routes (30 minutes)
```bash
# Edit: rust-wallet/src/main.rs

# Add routes:
.route("/sendMessage", web::post().to(send_message))
.route("/listMessages", web::post().to(list_messages))
.route("/acknowledgeMessage", web::post().to(acknowledge_message))
```

#### Step 7: Test (1 hour)
```bash
# 1. Start Rust wallet
cd rust-wallet
cargo run --release

# 2. Open browser, navigate to Coinflip
# 3. Check wallet logs - expect:
#    [INFO] 📋 /listMessages called for box: coinflip_inbox
#    [INFO] 📬 Found 0 messages
#    [INFO] 127.0.0.1 "POST /listMessages HTTP/1.1" 200

# 4. Success = No more 404 errors!
```

---

## 🏗️ **Architecture Decision Tree**

### **Port 3301 (Rust Wallet HTTP Server)** ⭐ **RECOMMENDED**
```
Pros:
✅ Simple - all in one place
✅ Authentication already working
✅ Rust performance and safety
✅ Easy to debug

Cons:
❌ Need to implement storage
❌ No built-in federation
```

**Verdict**: **DO THIS FIRST** - simplest path to working apps

### **Port 3302 (CEF C++ WebSocket)** ⏸️ **HOLD**
```
Pros:
✅ Could provide real-time push notifications
✅ Might support federation (BRC-34)

Cons:
❌ Purpose unclear
❌ More complexity
❌ C++ implementation needed

Questions:
❓ What is this server for?
❓ Is it already running?
❓ Does it have message handling code?
```

**Verdict**: **INVESTIGATE LATER** - not critical for MVP

### **Go Wallet Delegation** ⏸️ **HOLD**
```
Pros:
✅ Go SDK might have message relay features

Cons:
❌ Complex routing (Rust → Go)
❌ Two processes running
❌ Go websocket is NOT BRC-33
```

**Verdict**: **SKIP** - unnecessary complexity

---

## 📊 **Success Metrics**

### Minimum Viable Implementation:
- [ ] `/listMessages` returns 200 (even if empty array)
- [ ] Coinflip loads without errors
- [ ] Thryll loads without errors
- [ ] Can send/receive messages (basic test)

### Full Implementation:
- [ ] All 3 endpoints working correctly
- [ ] Messages persist across requests
- [ ] Multiple message boxes supported
- [ ] Authentication properly enforced
- [ ] Messages saved to disk

### Future Enhancements:
- [ ] WebSocket push notifications (port 3302?)
- [ ] BRC-34 federation support
- [ ] Message encryption (BRC-2)
- [ ] Admin UI to view message boxes

---

## 🚀 **Let's Get Started!**

### **Right Now**:
1. Open [BRC-33 spec](https://bsv.brc.dev/peer-to-peer/0033) in browser
2. Read it carefully (30-45 min)
3. Take notes on request/response formats
4. Answer architecture questions
5. Come back here for implementation steps

### **Files to Create/Edit**:
```
rust-wallet/src/message_relay.rs   ← NEW: Storage system
rust-wallet/src/handlers.rs        ← EDIT: Add 3 handlers
rust-wallet/src/main.rs             ← EDIT: Add 3 routes
```

### **Expected Timeline**:
- **Day 1**: Study + Design (2-3 hours)
- **Day 2**: Implementation + Testing (4-5 hours)
- **Day 3**: Polish + Real-world testing (2-3 hours)

**Total**: ~10 hours to working BRC-33 implementation 🎯

---

## 📞 **Need Help?**

Refer to these documents:
- `BRC33_MESSAGE_RELAY_DISCOVERY.md` - Comprehensive analysis
- `BRC100_IMPLEMENTATION_GUIDE.md` - Updated roadmap
- `DEVELOPER_NOTES.md` - Current session notes

**Good luck! 🚀**
