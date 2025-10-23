# Nonce Tracking Implementation Guide

> **When to implement**: After authentication is working, add replay attack prevention.

---

## 🎯 TL;DR - Use Simple Random Nonces

**For your wallet**: Use simple random nonces with basic tracking.

**Why?**:
- ✅ Your wallet = client (not high-volume server)
- ✅ 1-10 concurrent sessions (not 100,000)
- ✅ Storing 10 nonces = trivial memory
- ✅ Simple = fewer bugs

**When to use HMAC-based nonces**: Only if you're building a server handling 100k+ sessions/day.

---

## 🔐 Why Track Nonces?

### The Replay Attack:

```
1. Alice authenticates with ToolBSV:
   Alice → ToolBSV: { nonce: "abc123", signature: "valid_sig" }

2. Attacker captures this message

3. Later, attacker replays:
   Attacker → ToolBSV: { nonce: "abc123", signature: "valid_sig" }

4. Without nonce tracking:
   ToolBSV verifies signature ✅
   ToolBSV accepts authentication ✅
   Attacker gains access! ❌ SECURITY BREACH!

5. With nonce tracking:
   ToolBSV checks: "Have I seen 'abc123' before?"
   Yes → Reject as replay attack ✅
```

**Nonces ensure**: Each authentication is unique and can't be replayed.

---

## 📊 Two Approaches Compared:

### Approach 1: Simple Random (Recommended for Wallets)

```rust
// Generate
let nonce: [u8; 32] = rand::random();

// Store in HashSet
used_nonces.insert(nonce);

// Later, check
if used_nonces.contains(&received_nonce) {
    return Err("Replay attack!");
}
```

**Memory**: 32 bytes per nonce
**Complexity**: Low
**Best for**: Clients, low-volume apps
**Your case**: ✅ Perfect fit!

---

### Approach 2: HMAC-Based (For High-Volume Servers)

```rust
// Generate
let nonce = hmac_sha256(secret, their_pubkey + timestamp);

// Later, verify by re-computing
let expected = hmac_sha256(secret, their_pubkey + timestamp);
if nonce != expected {
    return Err("Invalid nonce!");
}
```

**Memory**: None (re-compute to verify)
**Complexity**: High (need timestamp handling)
**Best for**: High-volume servers (100k+ sessions)
**Your case**: ❌ Overkill!

---

## ✅ Recommended Implementation for Rust Wallet

### Phase 1: Simple Nonce Generation (Fix Auth First!)

**File**: `rust-wallet/src/handlers.rs` in `/.well-known/auth` handler

```rust
// Generate simple random 32-byte nonce
let our_nonce_bytes: [u8; 32] = rand::random();
let our_nonce = base64::encode(&our_nonce_bytes);

// Return in response
{
    "initialNonce": our_nonce,
    "yourNonce": their_nonce,
    "signature": signature_bytes
}
```

**Result**: ✅ Authentication works with ToolBSV

---

### Phase 2: Add Nonce Tracking (Later!)

**File**: `rust-wallet/src/nonce_tracker.rs` (new file)

```rust
use std::collections::HashMap;
use std::sync::Mutex;
use std::time::{SystemTime, UNIX_EPOCH};
use once_cell::sync::Lazy;

/// Tracks used nonces to prevent replay attacks
pub struct NonceTracker {
    nonces: HashMap<String, u64>, // nonce_base64 -> timestamp
}

impl NonceTracker {
    pub fn new() -> Self {
        NonceTracker {
            nonces: HashMap::new(),
        }
    }

    /// Generate a new random nonce and store it
    pub fn generate_nonce(&mut self) -> String {
        let nonce_bytes: [u8; 32] = rand::random();
        let nonce_base64 = base64::encode(&nonce_bytes);

        let now = Self::current_timestamp();
        self.nonces.insert(nonce_base64.clone(), now);

        // Cleanup old nonces (older than 1 hour)
        self.cleanup_old_nonces(3600);

        nonce_base64
    }

    /// Check if nonce has been used before
    pub fn is_nonce_used(&self, nonce: &str) -> bool {
        self.nonces.contains_key(nonce)
    }

    /// Mark a nonce as used (for received nonces)
    pub fn mark_nonce_used(&mut self, nonce: String) {
        let now = Self::current_timestamp();
        self.nonces.insert(nonce, now);

        // Cleanup old nonces
        self.cleanup_old_nonces(3600);
    }

    /// Remove nonces older than max_age_seconds
    fn cleanup_old_nonces(&mut self, max_age_seconds: u64) {
        let now = Self::current_timestamp();

        self.nonces.retain(|_, timestamp| {
            now - timestamp < max_age_seconds
        });
    }

    /// Get current Unix timestamp
    fn current_timestamp() -> u64 {
        SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap()
            .as_secs()
    }
}

/// Global nonce tracker (singleton)
static NONCE_TRACKER: Lazy<Mutex<NonceTracker>> = Lazy::new(|| {
    Mutex::new(NonceTracker::new())
});

/// Public API: Generate a new nonce
pub fn generate_nonce() -> String {
    NONCE_TRACKER.lock().unwrap().generate_nonce()
}

/// Public API: Check if nonce has been used
pub fn is_nonce_used(nonce: &str) -> bool {
    NONCE_TRACKER.lock().unwrap().is_nonce_used(nonce)
}

/// Public API: Mark nonce as used
pub fn mark_nonce_used(nonce: String) {
    NONCE_TRACKER.lock().unwrap().mark_nonce_used(nonce);
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_nonce_generation() {
        let mut tracker = NonceTracker::new();
        let nonce1 = tracker.generate_nonce();
        let nonce2 = tracker.generate_nonce();

        // Nonces should be unique
        assert_ne!(nonce1, nonce2);

        // Both should be base64 encoded 32 bytes = 44 chars
        assert_eq!(nonce1.len(), 44);
        assert_eq!(nonce2.len(), 44);
    }

    #[test]
    fn test_nonce_tracking() {
        let mut tracker = NonceTracker::new();
        let nonce = tracker.generate_nonce();

        // Should be marked as used
        assert!(tracker.is_nonce_used(&nonce));

        // Different nonce should not be used
        assert!(!tracker.is_nonce_used("different_nonce"));
    }

    #[test]
    fn test_nonce_expiration() {
        let mut tracker = NonceTracker::new();

        // Add a nonce with old timestamp
        let old_nonce = "old_nonce_base64";
        tracker.nonces.insert(old_nonce.to_string(), 0); // Timestamp = 0 (very old)

        // Cleanup should remove it
        tracker.cleanup_old_nonces(3600);

        assert!(!tracker.is_nonce_used(old_nonce));
    }
}
```

---

### Phase 2: Update Handlers

**File**: `rust-wallet/src/handlers.rs`

**Add to top**:
```rust
mod nonce_tracker;
use nonce_tracker::{generate_nonce, is_nonce_used, mark_nonce_used};
```

**In `/.well-known/auth` handler**:
```rust
// Generate nonce using tracker
let our_nonce = generate_nonce();

// Check if their nonce was already used (replay attack prevention)
if is_nonce_used(&req.initial_nonce) {
    log::warn!("⚠️ Replay attack detected! Nonce already used: {}", req.initial_nonce);
    return HttpResponse::BadRequest().json(serde_json::json!({
        "error": "Invalid nonce - possible replay attack"
    }));
}

// Mark their nonce as used
mark_nonce_used(req.initial_nonce.clone());

// ... rest of authentication ...
```

---

### Phase 2: Update main.rs

**File**: `rust-wallet/src/main.rs`

**Add module**:
```rust
mod nonce_tracker;
```

---

## 🧪 Testing Nonce Tracking

### Test 1: Normal Authentication
```rust
#[test]
fn test_normal_auth() {
    let nonce1 = generate_nonce();
    let nonce2 = generate_nonce();

    // Both should work
    assert!(!is_nonce_used(&nonce1));
    assert!(!is_nonce_used(&nonce2));

    // Mark as used
    mark_nonce_used(nonce1.clone());

    // Now it should be detected
    assert!(is_nonce_used(&nonce1));
}
```

### Test 2: Replay Attack Detection
```rust
#[test]
fn test_replay_attack() {
    let nonce = generate_nonce();

    // First use - OK
    assert!(!is_nonce_used(&nonce));
    mark_nonce_used(nonce.clone());

    // Second use - should detect replay
    assert!(is_nonce_used(&nonce));
}
```

### Test 3: Nonce Expiration
```rust
#[test]
fn test_nonce_expiration() {
    // Add nonce with old timestamp
    // Wait or manually set old timestamp
    // Verify it gets cleaned up after 1 hour
}
```

---

## 📈 Memory Usage Analysis

### For Your Wallet:

**Assumptions**:
- 10 concurrent authentication sessions
- Each session lasts 1 hour
- Cleanup runs every hour

**Memory Usage**:
```
Per nonce: 32 bytes (nonce) + 8 bytes (timestamp) + overhead = ~50 bytes
10 nonces: 500 bytes
1000 nonces (worst case): 50 KB
```

**Conclusion**: Trivial memory usage! Don't worry about it.

---

### For High-Volume Server:

**Assumptions**:
- 100,000 sessions per day
- Average session duration: 1 hour
- Concurrent sessions: ~4,166

**Memory Usage**:
```
Per nonce: ~50 bytes
4,166 concurrent: 208 KB
100,000 per day: 5 MB
```

**Conclusion**: Still manageable, but now HMAC-based nonces make sense.

---

## 🎯 When to Implement What:

### Now (Phase 1): Simple Random Nonces
- ✅ Generate random 32-byte nonces
- ✅ Get authentication working
- ✅ Test with ToolBSV

### Later (Phase 2): Add Tracking
- ✅ Implement nonce tracker
- ✅ Prevent replay attacks
- ✅ Add expiration cleanup

### Much Later (Phase 3): Optimize if Needed
- ⚠️ Only if you become a high-volume service
- ⚠️ Consider HMAC-based nonces then
- ⚠️ But you probably won't need this!

---

## 🚨 Important Notes:

1. **Phase 1 is Enough for Now**:
   - Simple random nonces work perfectly
   - Get authentication working first
   - Add tracking later if you want

2. **Nonce Tracking ≠ HMAC Nonces**:
   - Nonce tracking = storing used nonces
   - HMAC nonces = generating nonces with HMAC
   - Both solve replay attacks differently

3. **Your Wallet Doesn't Need HMAC Nonces**:
   - You're a client (not a server)
   - Low session volume
   - Simple approach wins

4. **Security Priority**:
   - Phase 1 (random nonces) = Good enough for testing
   - Phase 2 (nonce tracking) = Important for production
   - Phase 3 (HMAC nonces) = Unnecessary for your use case

---

## ✅ Summary:

**Use This**: Simple 32-byte random nonces
**Add Later**: Basic nonce tracking (HashMap)
**Don't Use**: HMAC-based nonces (overkill for wallets)

**Your current priority**: Get authentication working with ToolBSV using simple random nonces! 🚀

---

**Created**: October 22, 2025
**Status**: Implementation guide for later
**Priority**: Low (after auth works)
