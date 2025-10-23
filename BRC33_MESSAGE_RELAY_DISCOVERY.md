# 🎯 BRC-33 Message Relay Discovery - Critical Findings

**Date**: October 22, 2025
**Issue**: Coinflip and Thryll apps failing with "object null is not iterable"
**Root Cause**: Missing BRC-33 PeerServ Message Relay endpoints

---

## 🔍 **What We Discovered**

### The Problem:
```rust
// Coinflip wallet logs:
[2025-10-22T19:07:22Z INFO] 127.0.0.1 "POST /listMessages HTTP/1.1" 404 0 "-"
[2025-10-22T19:07:22Z INFO] 127.0.0.1 "POST /listMessages HTTP/1.1" 404 0 "-"
[2025-10-22T19:07:22Z INFO] 127.0.0.1 "POST /listMessages HTTP/1.1" 404 0 "-"
```

**Translation**: Coinflip is calling `/listMessages` endpoint, but our Rust wallet doesn't have it, returning **404 Not Found**.

### The JavaScript Error:
```javascript
Error: object null is not iterable (cannot read property Symbol(Symbol.iterator))
```

**What's happening**:
1. Coinflip calls `/listMessages`
2. Gets 404 (empty response body)
3. Tries to iterate over `response.messages` array
4. Gets `null` instead → JavaScript crashes

---

## 📚 **BRC-33 PeerServ Message Relay Interface**

### What is BRC-33?
**[BRC-33](https://bsv.brc.dev/peer-to-peer/0033)** defines a **message inbox system** for asynchronous peer-to-peer communication:
- Like an email inbox for BSV applications
- Messages stored temporarily until recipient retrieves them
- Useful when peers can't establish direct connections

### NOT Part of BRC-100!
**BRC-100**: Wallet-to-Application interface (28 methods for transactions, signatures, keys)
**BRC-33**: Separate message relay service that some apps expect wallets to provide

### The 3 Required Endpoints:

#### 1. `/sendMessage` - Send a message to a recipient's inbox
```json
// Request:
{
  "message": {
    "recipient": "02...",  // Recipient's public key
    "messageBox": "coinflip_inbox",  // Which inbox
    "body": "hello"  // Message content
  }
}

// Response:
{
  "status": "success"
}
```

#### 2. `/listMessages` - List messages from an inbox ⚠️ **BLOCKING COINFLIP!**
```json
// Request:
{
  "messageBox": "coinflip_inbox"
}

// Response:
{
  "status": "success",
  "messages": [
    {
      "messageId": 3301,
      "body": "hello",
      "sender": "02..."
    }
  ]
}
```

#### 3. `/acknowledgeMessage` - Delete received messages
```json
// Request:
{
  "messageIds": [3301, 3302]
}

// Response:
{
  "status": "success"
}
```

---

## 🏗️ **Current Architecture Analysis**

### What We Have:

#### Rust Wallet (Port 3301) - HTTP Server
```
✅ BRC-100 endpoints (createAction, signAction, etc.)
✅ /.well-known/auth (BRC-103/104 authentication)
✅ /createHmac, /verifyHmac, /createSignature, /verifySignature
❌ /sendMessage (BRC-33)
❌ /listMessages (BRC-33) ← BLOCKING APPS!
❌ /acknowledgeMessage (BRC-33)
```

#### Go Wallet WebSocket (go-wallet/brc100/websocket/)
```
✅ Custom WebSocket server for real-time notifications
✅ Authentication messages, session status, ping/pong
❌ NOT BRC-33 (different protocol)
```

#### CEF C++ Backend WebSocket (Port 3302)
```
❓ Purpose unclear - federation? real-time notifications?
❓ Relationship to BRC-33?
```

### HTTP Interceptor (CEF C++)
```cpp
// Already catching these routes:
url.find("/listMessages") != std::string::npos ||
url.find("/sendMessage") != std::string::npos ||
url.find("/acknowledgeMessage") != std::string::npos
```
✅ Interceptor is working! Routes are caught and forwarded to wallet.
❌ Wallet doesn't have handlers, returns 404.

---

## ❓ **Questions We Need to Answer**

### Priority 1: Storage Design
1. **Where to store messages?**
   - In-memory HashMap? (simple, lost on restart)
   - JSON file? (persistent, easy)
   - SQLite? (scalable, queryable)

2. **Storage structure:**
```rust
struct MessageStore {
    // Key: identity_key, Value: MessageBoxes
    users: HashMap<String, HashMap<String, Vec<Message>>>
}

struct Message {
    message_id: u64,
    sender: String,       // Sender's public key
    body: String,         // Message content
    timestamp: DateTime,
}
```

3. **Retention policy:**
   - How long to keep messages? (spec says "not for long-term storage")
   - Delete after acknowledgment only?
   - Max messages per inbox?

### Priority 2: Authentication
1. **Who is authenticated?**
   - ✅ Sender: Authenticated via BRC-31 (same as `/.well-known/auth`)
   - ✅ Recipient: Authenticated when listing/acknowledging messages
   - ❓ Do we verify message body signatures? (BRC-77)

2. **Message box security:**
   - Only recipient can list their messages?
   - Can anyone send to any message box?
   - Message box naming conventions?

### Priority 3: Architecture Decisions
1. **Port 3302 WebSocket Purpose?**
   - Real-time notifications when new messages arrive?
   - BRC-34 federation between servers?
   - Should we use it or ignore for now?

2. **Rust vs Go Implementation?**
   - ✅ **Recommendation**: Implement in Rust (port 3301) as HTTP POST
   - Keep it simple with the HTTP-only approach
   - Consider WebSocket notifications later

3. **Federation (BRC-34/35)?**
   - ❓ Do we need it now? (probably not)
   - ❓ Can we start local-only and add federation later? (yes!)

---

## 🛠️ **Recommended Implementation Plan**

### Phase 0: BRC-33 Basic Implementation (2-3 days)

#### Day 1: Study & Design
- [ ] Read **[BRC-33 spec](https://bsv.brc.dev/peer-to-peer/0033)** completely (1 hour)
- [ ] Design storage system - start with in-memory HashMap (1 hour)
- [ ] Define Rust structs and types (1 hour)
- [ ] Review authentication flow (already working!) (30 min)

#### Day 2: Implement Core Functionality
- [ ] Create message storage module (`rust-wallet/src/message_relay.rs`)
- [ ] Implement `/sendMessage` endpoint
- [ ] Implement `/listMessages` endpoint
- [ ] Implement `/acknowledgeMessage` endpoint
- [ ] Add routes to `main.rs`

#### Day 3: Test & Fix
- [ ] Test with Coinflip - expect 200 responses!
- [ ] Test with Thryll
- [ ] Verify message flow end-to-end
- [ ] Add persistence (save to JSON file)

### Minimal Implementation (Start Here!)

```rust
// rust-wallet/src/message_relay.rs

use std::collections::HashMap;
use std::sync::{Arc, Mutex};
use serde::{Deserialize, Serialize};

#[derive(Clone, Serialize, Deserialize)]
pub struct Message {
    pub message_id: u64,
    pub sender: String,      // Public key
    pub body: String,
    pub timestamp: i64,
}

#[derive(Clone)]
pub struct MessageStore {
    // Map: recipient_pubkey -> message_box_name -> messages
    messages: Arc<Mutex<HashMap<String, HashMap<String, Vec<Message>>>>>,
    next_id: Arc<Mutex<u64>>,
}

impl MessageStore {
    pub fn new() -> Self {
        Self {
            messages: Arc::new(Mutex::new(HashMap::new())),
            next_id: Arc::new(Mutex::new(1)),
        }
    }

    pub fn send_message(&self, recipient: &str, message_box: &str, sender: &str, body: &str) -> u64 {
        let mut messages = self.messages.lock().unwrap();
        let mut next_id = self.next_id.lock().unwrap();

        let message = Message {
            message_id: *next_id,
            sender: sender.to_string(),
            body: body.to_string(),
            timestamp: chrono::Utc::now().timestamp(),
        };

        messages
            .entry(recipient.to_string())
            .or_insert_with(HashMap::new)
            .entry(message_box.to_string())
            .or_insert_with(Vec::new)
            .push(message);

        *next_id += 1;
        *next_id - 1
    }

    pub fn list_messages(&self, recipient: &str, message_box: &str) -> Vec<Message> {
        let messages = self.messages.lock().unwrap();
        messages
            .get(recipient)
            .and_then(|boxes| boxes.get(message_box))
            .cloned()
            .unwrap_or_default()
    }

    pub fn acknowledge_messages(&self, recipient: &str, message_box: &str, message_ids: &[u64]) {
        let mut messages = self.messages.lock().unwrap();
        if let Some(boxes) = messages.get_mut(recipient) {
            if let Some(msgs) = boxes.get_mut(message_box) {
                msgs.retain(|m| !message_ids.contains(&m.message_id));
            }
        }
    }
}
```

### Handler Implementation:

```rust
// rust-wallet/src/handlers.rs

/// BRC-33: Send a message to a recipient's message box
pub async fn send_message(
    data: web::Data<AppState>,
    req: web::Json<SendMessageRequest>,
) -> impl Responder {
    log::info!("📋 /sendMessage called");

    // Sender is already authenticated via BRC-31 middleware
    let sender = ""; // TODO: Get from auth context

    let message_id = data.message_store.send_message(
        &req.message.recipient,
        &req.message.message_box,
        sender,
        &req.message.body,
    );

    log::info!("✅ Message {} sent to {}/{}",
        message_id, req.message.recipient, req.message.message_box);

    web::Json(json!({ "status": "success" }))
}

/// BRC-33: List messages from message box
pub async fn list_messages(
    data: web::Data<AppState>,
    req: web::Json<ListMessagesRequest>,
) -> impl Responder {
    log::info!("📋 /listMessages called for box: {}", req.message_box);

    // Caller is authenticated, get their public key
    let recipient = ""; // TODO: Get from auth context

    let messages = data.message_store.list_messages(recipient, &req.message_box);

    log::info!("📬 Found {} messages", messages.len());

    web::Json(json!({
        "status": "success",
        "messages": messages
    }))
}

/// BRC-33: Acknowledge (delete) messages
pub async fn acknowledge_message(
    data: web::Data<AppState>,
    req: web::Json<AckMessageRequest>,
) -> impl Responder {
    log::info!("📋 /acknowledgeMessage called for {} messages", req.message_ids.len());

    let recipient = ""; // TODO: Get from auth context
    let message_box = ""; // TODO: Get from request or context

    data.message_store.acknowledge_messages(recipient, message_box, &req.message_ids);

    log::info!("✅ Messages acknowledged and deleted");

    web::Json(json!({ "status": "success" }))
}
```

---

## 🔄 **Integration with Existing Architecture**

### How Authentication Works (Already Implemented! ✅)

BRC-33 uses **BRC-31 (Authrite)** authentication - the same system we already implemented for `/.well-known/auth`:

1. **App sends authenticated request**:
```http
POST /listMessages HTTP/1.1
X-Authrite-Identity-Key: 020b95583e18ac933d89a131f399890098dc1b3d4a8abcdde3eec4a7b191d2521e
X-Authrite-Nonce: ...
X-Authrite-Signature: ...
Content-Type: application/json

{
  "messageBox": "coinflip_inbox"
}
```

2. **Wallet verifies authentication** (using existing code!)
3. **Wallet returns messages for that identity**

### The Key Insight:
**We already have the authentication layer working!** We just need to add the 3 message storage endpoints.

---

## 📋 **Action Items (Next Steps)**

### Immediate (Today):
1. ✅ Update `BRC100_IMPLEMENTATION_GUIDE.md` with BRC-33 info
2. ✅ Create this discovery document
3. [ ] Read [BRC-33 spec](https://bsv.brc.dev/peer-to-peer/0033) in detail
4. [ ] Answer architecture questions above
5. [ ] Design storage system

### Tomorrow:
6. [ ] Create `rust-wallet/src/message_relay.rs` module
7. [ ] Implement 3 BRC-33 endpoints in `handlers.rs`
8. [ ] Add routes to `main.rs`
9. [ ] Test with Coinflip - see 200 responses! 🎉

### Later:
10. [ ] Add persistence (save to JSON)
11. [ ] Consider WebSocket notifications (port 3302?)
12. [ ] Research BRC-34 federation (optional)

---

## 📚 **Documentation to Study**

### Must Read (Order matters!):
1. **[BRC-33: PeerServ Message Relay](https://bsv.brc.dev/peer-to-peer/0033)** ← START HERE
   - Understand the 3 endpoints
   - Request/response formats
   - Message authenticity section
   - Limitations (not for long-term storage)

2. **[BRC-31: Authrite Authentication](https://bsv.brc.dev/peer-to-peer/0031)** ← Already implemented! ✅
   - Review how authentication works
   - Confirm we're compatible

### Optional (Read Later):
3. **[BRC-34: CHIP Federation](https://bsv.brc.dev/peer-to-peer/0034)**
   - Multi-server message routing
   - Skip for MVP

4. **[BRC-35: CLAP Lookup](https://bsv.brc.dev/peer-to-peer/0035)**
   - Service discovery
   - Skip for MVP

5. **[BRC-77: Message Signatures](https://bsv.brc.dev/peer-to-peer/0077)**
   - Optional signature verification
   - May not need for basic impl

---

## 🎯 **Success Criteria**

### When BRC-33 is Working:
✅ Coinflip loads without errors
✅ `/listMessages` returns 200 with empty array (at minimum)
✅ Thryll works correctly
✅ Apps can send/receive messages through our wallet
✅ Messages persist across calls (at least in-memory)

### Bonus Points:
🎁 Messages saved to disk (survive restart)
🎁 WebSocket notifications when new messages arrive
🎁 BRC-34 federation support
🎁 Message encryption (BRC-2)

---

**Next Step**: Read the [BRC-33 spec](https://bsv.brc.dev/peer-to-peer/0033) and answer the architecture questions!
